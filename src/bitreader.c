/* Copyright (c) 2026 Riverside Research */
/* Bit-parsing operations for Hammer.
 * Copyright (C) 2012  Meredith L. Patterson, Dan "TQ" Hirsch
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "hammer.h"
#include "internal.h"
#ifndef RTEMS_BUILD
#include "test_suite.h"
#endif // #ifndef RTEMS_BUILD

#include <stdint.h>
#include <stdio.h>

#define LSB(range) (0 : range)
#define MSB(range) (1 : range)
#define LDB(range, i) (((i) >> LSB(range)) & ((1 << (MSB(range) - LSB(range) + 1)) - 1))

int64_t h_read_bits(HInputStream *state, size_t count, char signed_p) {
    HAMMER_ASSERT(count <= 64);
    size_t original_count = count;
    uint64_t out = 0;
    size_t offset = 0;
    size_t final_shift = 0;
    uint64_t msb = signed_p && original_count > 0 && original_count <= 64
                       ? (UINT64_C(1) << (original_count - 1))
                       : 0;

    // Limit reads by both the physical storage and the logical bit boundary.
    size_t current = h_input_stream_pos(state);
    size_t physical_end = h_input_stream_physical_length(state);
    size_t logical_end = h_input_stream_length(state);
    size_t bits_left = 0;
    if (current < physical_end)
        bits_left = physical_end - current;
    if (current >= logical_end)
        bits_left = 0;
    else if (logical_end - current < bits_left)
        bits_left = logical_end - current;

    if (bits_left < count) {
        if (state->endianness & BYTE_BIG_ENDIAN)
            final_shift = count - bits_left;
        else
            final_shift = 0;
        count = bits_left;
        state->overrun = true;
    } else {
        final_shift = 0;
    }

    if ((state->bit_offset & 0x7) == 0 && (count & 0x7) == 0 && (state->margin == 0)) {
        // fast path
        if (state->endianness & BYTE_BIG_ENDIAN) {
            while (count > 0) {
                count -= 8;
                out = (out << 8) | state->input[state->index++];
            }
        } else {
            size_t i;
            for (i = 0; count > 0; i += 8) {
                count -= 8;
                uint64_t byte = state->input[state->index++];
                if (i < 64)
                    out |= byte << i;
            }
        }
    } else {
        while (count) {
            size_t segment, segment_len;
            // Read a segment...
            if (state->endianness & BIT_BIG_ENDIAN) {
                if (count + state->bit_offset + state->margin >= 8) {
                    segment_len = 8u - state->bit_offset - state->margin;
                    segment =
                        (state->input[state->index] >> state->margin) & ((1 << segment_len) - 1);
                    state->index++;
                    state->bit_offset = 0;
                    state->margin = 0;
                } else {
                    segment_len = count;
                    state->bit_offset += (uint8_t)count;
                    segment = (state->input[state->index] >> (8 - state->bit_offset)) &
                              ((1 << segment_len) - 1);
                }
            } else { // BIT_LITTLE_ENDIAN
                if (count + state->bit_offset + state->margin >= 8) {
                    segment_len = 8u - state->bit_offset - state->margin;
                    segment = (state->input[state->index] >> state->bit_offset) &
                              ((1 << segment_len) - 1);
                    state->index++;
                    state->bit_offset = 0;
                    state->margin = 0;
                } else {
                    segment_len = count;
                    segment = (state->input[state->index] >> state->bit_offset) &
                              ((1 << segment_len) - 1);
                    state->bit_offset += (uint8_t)segment_len;
                }
            }

            // have a valid segment; time to assemble the byte
            if (state->endianness & BYTE_BIG_ENDIAN) {
                out = (out << segment_len | segment);
            } else { // BYTE_LITTLE_ENDIAN
                if (offset < 64)
                    out |= (uint64_t)segment << offset;
                offset += segment_len;
            }
            count -= segment_len;
        }
    }
    if (final_shift >= 64)
        out = 0;
    else
        out <<= final_shift;
    return (int64_t)((out ^ msb) - msb); // perform sign extension
}

void h_skip_bits(HInputStream *stream, size_t count) {
    size_t left;

    if (count == 0)
        return;

    if (stream->overrun)
        return;

    size_t current = h_input_stream_pos(stream);
    size_t physical_end = h_input_stream_physical_length(stream);
    size_t logical_end = h_input_stream_length(stream);
    size_t physical_left = current < physical_end ? physical_end - current : 0;
    size_t logical_left = current < logical_end ? logical_end - current : 0;
    size_t available = physical_left < logical_left ? physical_left : logical_left;

    /*
     * A partial logical final byte cannot be represented by the old
     * byte-oriented skip loop once it reaches the boundary. Canonicalize the
     * position there and report an overrun only when the caller crossed it.
     */
    if (available < physical_left) {
        if (count >= available) {
            h_seek_bits(stream, logical_end);
            if (count > available)
                stream->overrun = true;
            return;
        }
    }

    if (available == 0) {
        stream->overrun = true;
        return;
    }

    // consume from a partial byte?
    left = (size_t)(8 - stream->bit_offset - stream->margin);
    if (count < left) {
        size_t new_offset = (size_t)stream->bit_offset + count;
        HAMMER_ASSERT(new_offset < 8U);

        stream->bit_offset = (uint8_t)new_offset;
        return;
    }
    if (left < 8) {
        stream->index += 1;
        stream->bit_offset = 0;
        stream->margin = 0;
        count -= left;
    }
    HAMMER_ASSERT(stream->bit_offset == 0);
    HAMMER_ASSERT(stream->margin == 0);

    // consume full bytes
    left = stream->length - stream->index;
    if (count / 8 <= left) {
        stream->index += count / 8;
        count = count % 8;
    } else {
        stream->index = stream->length;
        stream->overrun = true;
        return;
    }
    HAMMER_ASSERT(count < 8U);

    // final partial byte
    if (count > 0 && stream->index == stream->length)
        stream->overrun = true;
    else
        stream->bit_offset = (uint8_t)count;
}

void h_seek_bits(HInputStream *stream, size_t pos) {
    size_t pos_index = pos / 8;
    size_t pos_offset = pos % 8;
    size_t stream_start = stream->pos;
    size_t stream_start_bits = stream_start > SIZE_MAX / 8 ? SIZE_MAX : stream_start * 8;
    size_t logical_end = h_input_stream_length(stream);
    size_t physical_end = h_input_stream_physical_length(stream);

    /* Seek past either end to the logical boundary and mark the operation bad. */
    if (pos > logical_end || pos > physical_end || pos < stream_start_bits) {
        size_t boundary = logical_end < physical_end ? logical_end : physical_end;
        size_t boundary_index = boundary / 8;
        stream->index = boundary_index >= stream->pos ? boundary_index - stream->pos : 0;
        stream->bit_offset = (uint8_t)(boundary % 8);
        stream->margin = 0;
        stream->overrun = true;
        return;
    }

    stream->index = pos_index - stream->pos;
    assert(pos_offset < 8U);
    stream->bit_offset = (uint8_t)pos_offset;
    stream->margin = 0;
}

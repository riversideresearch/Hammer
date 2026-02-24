#ifndef DNS_PARSER_H
#define DNS_PARSER_H

#include <stdint.h>
#include <stddef.h>
#include <hammer/hammer.h>

typedef struct {
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
    uint16_t packet_len;
    uint16_t curr_storage;
} DNSContext;

HParser *headerParser(DNSContext *ctx);
HParser *bodyParser(DNSContext *ctx);

#endif

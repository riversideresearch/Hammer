#ifndef SLOBALLOC_H_SEEN
#define SLOBALLOC_H_SEEN

#include <stddef.h>

typedef struct slob SLOB;

/**
 * @brief Initialize a SLOB allocator over a caller‑supplied memory region.
 * @param mem  Pointer to a contiguous memory region that will hold the
 *             allocator metadata and all future allocations.
 * @param size Total size in bytes of the region pointed to by @p mem.
 * @return A pointer to the initialized SLOB structure on success,
 *         or NULL if the region is too small or would overflow.
 * @note All allocations returned by sloballoc(), slobrealloc(), and
 *       slobfree() must originate from this same SLOB instance.
 */
SLOB *slobinit(void *mem, size_t size);

/**
 * @brief Allocates a new SLOB block
 * @param slob The allocator instance that owns the block.
 * @param size Size of the intended allocation
 * @return A pointer belonging to @param slob in the resized block on success, or NULL on failure.
 * @note if size is too small or 0, the size will be set to fitblock bytes
 */
void *sloballoc(SLOB *slob, size_t size);

/**
 * @brief Reallocate a previously allocated SLOB block.
 *        This function implements the SLOB allocator's equivalent of
 *        standard C realloc(), with the same high‑level semantics:
 * @param slob  The allocator instance that owns the block.
 * @param p     Pointer previously returned by sloballoc() or slobrealloc().
 * @param size  New size in bytes.
 * @return A pointer belonging to @param slob in the resized block on success, or NULL on failure.
 *         On failure, the original block (if any) is left untouched.
 * @note If @param p is NULL, this behaves like sloballoc(slob, size).
 *       If @param size is 0, the block is freed and NULL is returned.
 */
void *slobrealloc(SLOB *slob, void *p, size_t size);

/**
 * @brief Free backend configuration
 * @param slob The allocator instance that owns the block.
 * @param p    Pointer previously returned by sloballoc() or slobrealloc().
 */
void slobfree(SLOB *slob, void *p);

/**
 * @brief consistency check (verify internal invariants)
 * @return 0 on success,
 */
int slobcheck(SLOB *slob);

#endif // SLOBALLOC_H_SEEN

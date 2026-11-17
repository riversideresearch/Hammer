/* Copyright (c) 2026 Riverside Research */
#ifndef HAMMER_TSEARCH__H
#define HAMMER_TSEARCH__H

typedef enum { h_preorder, h_postorder, h_endorder, h_leaf } HVisit;

/* find or insert datum into search tree */
void *h_tsearch(const void *vkey, void **vrootp, int (*compar)(const void *, const void *));

/* delete node with given key */
void *h_tdelete(const void *vkey, void **vrootp, int (*compar)(const void *, const void *));

/* Walk the nodes of a tree */
void h_twalk(const void *vroot, void (*action)(const void *, HVisit, int));

/* find a node, or return 0 */
void *h_tfind(const void *vkey, void *const *vrootp, int (*compar)(const void *, const void *));

#endif /* HAMMER_TSEARCH__H */

#include <stdlib.h>
#include "trie.h"

struct Trie *Trie_advanceAdd(struct Trie *n, const int ch) {
  struct Trie_internal *t;
  if (n == NULL) return NULL;
  int hi = ch >> 4 & 0xf;
  int lo = ch & 0xf;
  t = n->branch[hi];
  if (t == NULL) {
    t = calloc(1, sizeof(struct Trie_internal));
    n->branch[hi] = t;
  }
  struct Trie *n2 = t->branch[lo];
  if (n2 == NULL) {
    n2 = Trie_create();
    n2->parent = n;
    t->branch[lo] = n2;
    n2->depth = n->depth + 1;
  }
  return n2;
}

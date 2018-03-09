#include <stdlib.h>
#include "trie.h"

struct Trie *Trie_create(void) {
  struct Trie *t = calloc(1, sizeof(struct Trie));
  return t;
}

struct Trie *Trie_advance(struct Trie *n, const int ch) {
  if (n == NULL) return NULL;
  int hi = ch >> 4 & 0xf;
  int lo = ch & 0xf;
  struct Trie_internal *n2 = n->branch[hi];
  if (n2 == NULL) return NULL;
  return n2->branch[lo];
}

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
  }
  return n2;
}

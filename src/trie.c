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
  n = n->branch[hi];
  if (n == NULL) return NULL;
  n = n->branch[lo];
  return n;
}

struct Trie *Trie_advanceAdd(struct Trie *n, const int ch) {
  struct Trie *t;
  if (n == NULL) return NULL;
  int hi = ch >> 4 & 0xf;
  int lo = ch & 0xf;
  t = n->branch[hi];
  if (t == NULL) {
    t = Trie_create();
    t->parent = n;
    n->branch[hi] = t;
  }
  n = t;
  t = n->branch[lo];
  if (t == NULL) {
    t = Trie_create();
    t->parent = n;
    n->branch[lo] = t;
  }
  return t;
}

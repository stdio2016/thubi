#pragma once
#ifndef TRIE_INCLUDED
#define TRIE_INCLUDED
#include <stdint.h>
#include <stddef.h>

struct Trie_internal;

struct Trie {
  int depth;
  int id;
  struct Trie *parent;
  struct Trie_internal *branch[16];
  struct Trie *dict;
  void *payload;
};

struct Trie_internal {
  struct Trie *branch[16];
};

static inline struct Trie *Trie_create(void) {
  struct Trie *t = calloc(1, sizeof(struct Trie));
  return t;
}

static inline struct Trie *Trie_advance(struct Trie *n, const int ch) {
  if (n == NULL) return NULL;
  int hi = ch >> 4 & 0xf;
  int lo = ch & 0xf;
  struct Trie_internal *n2 = n->branch[hi];
  if (n2 == NULL) return NULL;
  return n2->branch[lo];
}

struct Trie *Trie_advanceAdd(struct Trie *n, const int ch);

#endif

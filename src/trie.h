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

struct Trie *Trie_create(void);

struct Trie *Trie_advance(struct Trie *n, const int ch);

struct Trie *Trie_advanceAdd(struct Trie *n, const int ch);

#endif

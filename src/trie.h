#pragma once
#ifndef TRIE_INCLUDED
#define TRIE_INCLUDED
#include <stdint.h>
#include <stddef.h>

struct Trie {
  int depth;
  struct Trie *parent;
  struct Trie *branch[16];
  union {
    void *v;
    int n;
  } payload;
};

struct Trie *Trie_create(void);

struct Trie *Trie_advance(struct Trie *n, const int ch);

struct Trie *Trie_advanceAdd(struct Trie *n, const int ch);

#endif

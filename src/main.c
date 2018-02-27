#include <stdbool.h>
#include <stdio.h>
#include "trie.h"

int main(void)
{
  struct Trie *t = Trie_create(), *u = t;
  int ch;
  bool cr = false;
  while ((ch = getchar()) != EOF) {
    if (ch == '\n') {
      if (cr) puts("new");
      else puts("old");
      cr = false;
      u = t;
    }
    else {
      struct Trie *v = Trie_advance(u, ch);
      if (v == NULL) {
        u = Trie_advanceAdd(u, ch);
        cr = true;
      }
      else u = v;
    }
  }
  return 0;
}

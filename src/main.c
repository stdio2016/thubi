#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "trie.h"
#define OutOfMemory()  do { fprintf(stderr, "out of memory\n"); exit(1); } while (0)

unsigned char *LineBuf;
size_t LineBufSize = 0, LineBufCap = 6;
struct Trie *Symbols;
int SymbolCounter = 256 + 2;
struct Trie *AcTrie;

int getLine(FILE *f) {
  int ch = fgetc(f);
  LineBufSize = 0;
  while (ch != EOF && ch != '\n') {
    LineBuf[LineBufSize++] = ch;
    if (LineBufSize >= LineBufCap) {
      LineBufCap *= 2;
      LineBuf = realloc(LineBuf, LineBufCap);
      if (LineBuf == 0) OutOfMemory();
    }
    ch = fgetc(f);
  }
  LineBuf[LineBufSize] = '\0';
  if (LineBufSize > 0 && LineBuf[LineBufSize-1] == '\r') { // windows newline
    --LineBufSize;
    LineBuf[LineBufSize] = '\0';
  }
  if (ch == EOF) return EOF; // last line
  if (ch == '\n') {
    /*ch = fgetc(f);
    if (ch == EOF) { // last line
      return EOF;
    }
    ungetc(ch, f);*/
  }
  return 1;
}

int defineSymbol(char *sym, int len, int id) {
  struct Trie *t = Symbols;
  int i;
  bool err = false;
  for (i = 0; i < len; i++) {
    if (t->payload.n & 1) { // prefix is another symbol
      fprintf(stderr, "symbol \"\\%s\" is not prefix-free\n", sym);
      err = true;
      break;
    }
    t->payload.n += 2;
    t = Trie_advanceAdd(t, sym[i]);
  }
  if (!err) {
    if (t->payload.n == 0) { // user defined symbol
      printf("symbol %s is %d\n", sym, id);
      t->payload.n = id<<1 | 1;
    }
    else {
      if (t->payload.n & 1) { // delete symbol
        t->payload.n = 0;
        for (i = 0; i < len; i++) {
          t = t->parent;
          t->payload.n -= 4;
        }
        return -1;
      }
      else { // existing symbol
        fprintf(stderr, "symbol \"\\%s\" is not prefix-free\n", sym);
        err = true;
      }
    }
  }
  if (err) {
    for (i = i; i > 0; i--) {
      t = t->parent;
      t->payload.n -= 2;
    }
  }
  return !err;
}

void parseFile(FILE *f) {
  LineBuf = calloc(LineBufCap, 1);
  if (LineBuf == NULL) OutOfMemory();
  int ended, state = 0;
  do {
    ended = getLine(f);
    if (state == 0) {
      if (LineBufSize == 0) {
        state = 2;
      }
      else if (LineBuf[0] == ':') {
        printf("from %s\n", LineBuf+1);
        state = 1;
      }
      else if (LineBuf[0] == '\\') {
        if (defineSymbol(LineBuf + 1, LineBufSize - 1, SymbolCounter) == 1) {
          SymbolCounter++;
        }
      }
      else if (LineBuf[0] == '#') {
        // skip comment
      }
      else {
        fprintf(stderr, "unknown lhs production \"%s\"\n", LineBuf);
      }
    }
    else if (state == 1) {
      if (LineBuf[0] == '=') {
        printf("to %s\n", LineBuf+1);
        state = 0;
      }
      else if (LineBuf[0] == '~') {
        printf("print %s\n", LineBuf+1);
        state = 0;
      }
      else if (LineBuf[0] == '#') {
        // skip comment
      }
      else if (strcmp(LineBuf, ":::") == 0) {
        printf("input\n");
        state = 0;
      }
      else {
        fprintf(stderr, "unknown rhs production \"%s\"\n", LineBuf);
        state = 0;
      }
    }
    else if (state == 2 && (ended != EOF || LineBufSize > 0)) {
      printf("init str %s\n", LineBuf);
    }
  } while (ended != EOF) ;
  free(LineBuf);
}

int main(void)
{
  FILE *f = stdin;
  if (f == NULL) {
    return 1;
  }
  Symbols = Trie_create();
  if (Symbols == NULL) OutOfMemory();
  AcTrie = Trie_create();
  if (AcTrie == NULL) OutOfMemory();
  parseFile(f);
  fclose(f);
  return 0;
}

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "trie.h"
#include "StringBuffer.h"
#define OutOfMemory()  do { fprintf(stderr, "out of memory\n"); exit(1); } while (0)
#define MAX_BUILTIN_SYMBOL  258
#define SYMBOL_B 256
#define SYMBOL_S 257

unsigned char *LineBuf;
size_t LineBufSize = 0, LineBufCap = 6;
struct StringBuffer RuleBuf;
struct Trie *Symbols;
int SymbolCounter = MAX_BUILTIN_SYMBOL;
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

int defineSymbol(const char *sym, int len, int id) {
  struct Trie *t = Symbols;
  int i;
  bool err = false;
  for (i = 0; i < len; i++) {
    if (!isprint(sym[i])) {
      fprintf(stderr, "symbol contains invalid character\n");
      err = true;
      break;
    }
    if (t->payload.n & 1) { // prefix is another symbol
      fprintf(stderr, "symbol \"\\%s\" is not prefix-free\n", sym);
      err = true;
      break;
    }
    t->payload.n += 2;
    t = Trie_advanceAdd(t, sym[i]);
  }
  if (!err) {
    if (t->payload.n & 1 && t->payload.n < MAX_BUILTIN_SYMBOL * 2) { // builtin symbol
      err = true;
      printf("builtin symbol \\%s cannot be unidentified\n", sym);
    }
    else if (t->payload.n == 0) { // user defined symbol
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
      else { // is prefix of some symbol
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

void addBuiltinSymbols(void) {
  char symName[] = "\\nrtfave";
  char escaped[] = "\\\n\r\t\f\a\v\x1B";
  int i;
  for (i = 0; i < 8; i++) {
    defineSymbol(&symName[i], 1, escaped[i]);
  }
  defineSymbol("b", 1, SYMBOL_B);
  defineSymbol("s", 1, SYMBOL_S);
  char s[5];
  for (i = 0; i < 256; i++) {
    sprintf(s, "%.3o", i);
    defineSymbol(s, 3, i);
  }
  char hex[22] = "0123456789abcdefABCDEF";
  s[0] = 'x';
  for (i = 0; i < 22; i++) {
    int j, ih;
    s[1] = hex[i];
    ih = i >= 16 ? i-6 : i;
    for (j = 0; j < 22; j++) {
      s[2] = hex[j];
      int jh = j >= 16 ? j-6 : j;
      defineSymbol(s, 3, ih * 16 + jh);
    }
  }
}

int parseLineEscape() {
  StrBuf_clear(&RuleBuf);
  size_t i, symbolStart;
  struct Trie *a = NULL;
  for (i = 1; i < LineBufSize; i++) {
    unsigned ch;
    if (a == NULL) {
      ch = LineBuf[i];
      if (ch == '\\') {
        symbolStart = i;
        a = Symbols;
      }
      else {
        if (ch >= 0x80) {
          StrBuf_appendChar(&RuleBuf, 0xc0 | ch>>6);
          StrBuf_appendChar(&RuleBuf, 0x80 | ch & 0x3f);
        }
        else {
          StrBuf_appendChar(&RuleBuf, ch);
        }
      }
    }
    else {
      a = Trie_advance(a, LineBuf[i]);
      if (a == NULL || a->payload.n == 0) {
        fprintf(stderr, "unknown symbol \"");
        size_t j;
        for (j = symbolStart; j <= i; j++) {
          fputc(LineBuf[j], stderr);
        }
        fprintf(stderr, "\"\n");
        return 0;
      }
      ch = a->payload.n;
      if (ch & 1) {
        ch >>= 1;
        if (ch >= 0x80) {
          int y = 2, i;
          char ut[7];
          while (y < 6 && ch > 2<<(y*5)-1) {
            y++; 
          }
          ut[0] = (0xff ^ 0xff>>y) | ch >> (y*6-6);
          for (i = 1; i < y; i++) {
            ut[i] = 0x80 | (ch >> ((y-i-1)*6) & 0x3f);
          }
          ut[y] = '\0';
          StrBuf_append(&RuleBuf, ut);
        }
        else {
          StrBuf_appendChar(&RuleBuf, ch);
        }
        a = NULL;
      }
    }
  }
  if (a != NULL) {
    fprintf(stderr, "unknown symbol \"");
    size_t j;
    for (j = symbolStart; j <= i; j++) {
      fputc(LineBuf[j], stderr);
    }
    fprintf(stderr, "\"\n");
    return 0;
  }
  return 1;
}

void parseFile(FILE *f) {
  LineBuf = calloc(LineBufCap, 1);
  if (LineBuf == NULL) OutOfMemory();
  StrBuf_init(&RuleBuf);
  int ended, state = 0;
  do {
    ended = getLine(f);
    if (state == 0) {
      if (LineBufSize == 0) {
        state = 2;
      }
      else if (LineBuf[0] == ':') {
        printf("from %s\n", LineBuf+1);
        parseLineEscape();
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
        parseLineEscape();
        state = 0;
      }
      else if (LineBuf[0] == '~') {
        printf("print %s\n", LineBuf+1);
        parseLineEscape();
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
  StrBuf_destroy(&RuleBuf);
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
  addBuiltinSymbols();
  parseFile(f);
  fclose(f);
  return 0;
}

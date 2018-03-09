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

enum ThubiRuleMode {
  ReplaceRule, PrintRule, InputRule
};

struct ThubiRule {
  size_t lhslen;
  char *rhs;
  size_t rhslen;
  enum ThubiRuleMode mode;
  struct ThubiRule *next;
};

char *LineBuf;
size_t LineBufSize = 0, LineBufCap = 6;
struct StringBuffer RuleBuf;
struct Trie *Symbols;
int SymbolCounter = MAX_BUILTIN_SYMBOL;
struct Trie *AcTrie;
int AcStateCount = 0;
struct Trie **AcStates;

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
    if (t->id & 1) { // prefix is another symbol
      fprintf(stderr, "symbol \"\\%s\" is not prefix-free\n", sym);
      err = true;
      break;
    }
    t->id += 2;
    t = Trie_advanceAdd(t, sym[i]);
  }
  if (!err) {
    if (t->id & 1 && t->id < MAX_BUILTIN_SYMBOL * 2) { // builtin symbol
      err = true;
      printf("builtin symbol \\%s cannot be unidentified\n", sym);
    }
    else if (t->id == 0) { // user defined symbol
      t->id = id<<1 | 1;
    }
    else {
      if (t->id & 1) { // delete symbol
        t->id = 0;
        for (i = 0; i < len; i++) {
          t = t->parent;
          t->id -= 4;
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
      t->id -= 2;
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

int parseLineEscape(int start) {
  size_t i, symbolStart;
  struct Trie *a = NULL;
  for (i = start; i < LineBufSize; i++) {
    unsigned ch;
    if (a == NULL) {
      ch = (unsigned char) LineBuf[i];
      if (ch == '\\') {
        symbolStart = i;
        a = Symbols;
      }
      else {
        if (ch >= 0x80) {
          StrBuf_appendChar(&RuleBuf, 0xc0 | (ch>>6));
          StrBuf_appendChar(&RuleBuf, 0x80 | (ch & 0x3f));
        }
        else {
          StrBuf_appendChar(&RuleBuf, ch);
        }
      }
    }
    else {
      a = Trie_advance(a, LineBuf[i]);
      if (a == NULL || a->id == 0) {
        fprintf(stderr, "unknown symbol \"");
        size_t j;
        for (j = symbolStart; j <= i; j++) {
          fputc(LineBuf[j], stderr);
        }
        fprintf(stderr, "\"\n");
        return 0;
      }
      ch = a->id;
      if (ch & 1) {
        ch >>= 1;
        if (ch >= 0x80) {
          int y = 2, i;
          char ut[7];
          while (y < 6 && ch > 2<<((y*5)-1)) {
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

struct Trie *addRuleLhs(const char *str, size_t len) {
  size_t i;
  struct Trie *a = AcTrie;
  for (i = 0; i < len; i++) {
    a = Trie_advanceAdd(a, str[i]);
    if (a->id == 0) {
      a->id = AcStateCount++;
    }
  }
  return a;
}

void addRuleRhs(struct Trie *lhs, const char *str, size_t len, int mode) {
  struct ThubiRule *r = malloc(sizeof(struct ThubiRule));
  if (r == NULL) OutOfMemory();
  r->lhslen = lhs->depth;
  if (str == NULL) {
    r->rhs = NULL;
  }
  else {
    r->rhs = malloc(len+1);
    if (r->rhs == NULL) OutOfMemory();
    memcpy(r->rhs, str, len);
    r->rhs[len] = '\0';
  }
  r->rhslen = len;
  r->mode = mode;
  struct ThubiRule *ac = lhs->payload;
  r->next = ac;
  lhs->payload = r;
}

void parseFile(FILE *f) {
  LineBuf = calloc(LineBufCap, 1);
  if (LineBuf == NULL) OutOfMemory();
  StrBuf_init(&RuleBuf);
  struct Trie *lhs;
  int ended, state = 0;
  do {
    ended = getLine(f);
    if (state == 0) {
      StrBuf_clear(&RuleBuf);
      if (LineBufSize == 0) {
        state = 2;
      }
      else if (LineBuf[0] == ':') {
        printf("from %s\n", LineBuf+1);
        parseLineEscape(1);
        lhs = addRuleLhs(RuleBuf.buf, RuleBuf.size);
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
      StrBuf_clear(&RuleBuf);
      if (LineBuf[0] == '=') {
        printf("to %s\n", LineBuf+1);
        parseLineEscape(1);
        addRuleRhs(lhs, RuleBuf.buf, RuleBuf.size, ReplaceRule);
        state = 0;
      }
      else if (LineBuf[0] == '~') {
        printf("print %s\n", LineBuf+1);
        parseLineEscape(1);
        addRuleRhs(lhs, RuleBuf.buf, RuleBuf.size, PrintRule);
        state = 0;
      }
      else if (LineBuf[0] == '#') {
        // skip comment
      }
      else if (strcmp(LineBuf, ":::") == 0) {
        printf("input\n");
        addRuleRhs(lhs, NULL, 0, ReplaceRule);
        state = 0;
      }
      else {
        fprintf(stderr, "unknown rhs production \"%s\"\n", LineBuf);
        state = 0;
      }
    }
    else if (state == 2 && (ended != EOF || LineBufSize > 0)) {
      printf("init str %s\n", LineBuf);
      parseLineEscape(0);
    }
  } while (ended != EOF) ;
  free(LineBuf);
  StrBuf_destroy(&RuleBuf);
}

void buildAcAutomata() {
  // Trie->parent becomes fail pointer
  AcStates = malloc(sizeof(struct Trie *) * AcStateCount);
  int i, qend = 1;
  const int n = AcStateCount;
  AcStates[0] = AcTrie;
  for (i = 0; i < n; i++) {
    struct Trie *me = AcStates[i];
    int hi, lo;
    for (hi = 0; hi < 16; hi++) {
      struct Trie_internal *u = me->branch[hi];
      if (u != NULL) {
        for (lo = 0; lo < 16; lo++) {
          struct Trie *child = u->branch[lo];
          int ch = hi<<4 | lo;
          if (child != NULL) {
            struct Trie *find, *fail = NULL;
            AcStates[qend] = child;
            child->id = qend;
            find = me->parent;
            while (find != NULL) {
              fail = Trie_advance(find, ch);
              if (fail != NULL) break;
              find = find->parent;
            }
            if (fail == NULL) fail = AcTrie;
            child->parent = fail;
            struct ThubiRule *r = fail->payload;
            if (r != NULL) child->dict = fail;
            else child->dict = fail->dict;
            qend++;
          }
        }
      }
    }
  }
}

void showTrie(struct Trie *t, int indent) {
  int hi, lo;
  for (hi = 0; hi < 16; hi++) {
    struct Trie_internal *ti = t->branch[hi];
    if (ti == NULL) continue;
    for (lo = 0; lo < 16; lo++) {
      struct Trie *t2 = ti->branch[lo];
      if (t2 == NULL) continue;
      printf("%*s", indent*2, "");
      printf("#%d '%c'", t2->id, hi<<4|lo);
      if (t2->parent != NULL) printf(" parent %d", t2->parent->id);
      if (t2->dict != NULL) printf(" dict %d", t2->dict->id);
      if (t2->payload != NULL) printf(" match");
      puts("");
      showTrie(t2, indent+1);
    }
  }
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
  AcStateCount++;
  addBuiltinSymbols();
  parseFile(f);
  fclose(f);
  buildAcAutomata();
  showTrie(AcTrie, 0);
  return 0;
}

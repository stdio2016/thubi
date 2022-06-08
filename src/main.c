#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
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
  size_t i, symbolStart = 0;
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
          while (y < 6 && ch >= 2<<(y*5)) {
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
  struct Trie *lhs;
  int ended, state = 0;
  do {
    ended = getLine(f);
    if (state == 0) {
      StrBuf_clear(&RuleBuf);
      if (LineBufSize == 0) {
        state = 2;
        StrBuf_append(&RuleBuf, "\xc4\x80"); // \b
      }
      else if (LineBuf[0] == ':') {
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
        parseLineEscape(1);
        addRuleRhs(lhs, RuleBuf.buf, RuleBuf.size, ReplaceRule);
        state = 0;
      }
      else if (LineBuf[0] == '~') {
        parseLineEscape(1);
        addRuleRhs(lhs, RuleBuf.buf, RuleBuf.size, PrintRule);
        state = 0;
      }
      else if (LineBuf[0] == '#') {
        // skip comment
      }
      else if (strcmp(LineBuf, ":::") == 0) {
        addRuleRhs(lhs, NULL, 0, InputRule);
        state = 0;
      }
      else {
        fprintf(stderr, "unknown rhs production \"%s\"\n", LineBuf);
        state = 0;
      }
    }
    else if (state == 2 && (ended != EOF || LineBufSize > 0)) {
      parseLineEscape(0);
    }
  } while (ended != EOF) ;
  free(LineBuf);
  if (state != 2) {
    StrBuf_clear(&RuleBuf);
    StrBuf_append(&RuleBuf, "\xc4\x80"); // \b
  }
  StrBuf_append(&RuleBuf, "\xc4\x81"); // \s
}

void parseThueFile(FILE *f) {
  LineBuf = calloc(LineBufCap, 1);
  if (LineBuf == NULL) OutOfMemory();
  struct Trie *lhs;
  int ended, state = 0;
  StrBuf_clear(&RuleBuf);
  do {
    ended = getLine(f);
    if (state == 0) {
      if (LineBufSize == 0) {
        // empty line
        continue;
      }
      else {
        char *where = strstr(LineBuf, "::=");
        if (!where) {
          continue;
        }
        if (LineBufSize == 3) {
          state = 1;
        }
        else {
          lhs = addRuleLhs(LineBuf, where - LineBuf);
          if (where[3] == '~') {
            addRuleRhs(lhs, where+4, LineBufSize - (where+4 - LineBuf), PrintRule);
          }
          else if (strcmp(where+3, ":::") == 0) {
            addRuleRhs(lhs, NULL, 0, InputRule);
          }
          else {
            addRuleRhs(lhs, where+3, LineBufSize - (where+3 - LineBuf), ReplaceRule);
          }
        }
      }
    }
    else if (state == 1) {
      StrBuf_append(&RuleBuf, LineBuf);
    }
  } while (ended != EOF) ;
  free(LineBuf);
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

struct Match {
  size_t pos;
  struct ThubiRule *rule;
};

int getUserInputLine() {
  fflush(stdout);
  int ch = getchar();
  StrBuf_clear(&RuleBuf);
  while (ch != EOF && ch != '\n') {
    if (ch > 0x80) {
      StrBuf_appendChar(&RuleBuf, 0xc0 | ch>>6);
      StrBuf_appendChar(&RuleBuf, 0x80 | (ch & 0x3f));
    }
    else StrBuf_appendChar(&RuleBuf, ch);
    ch = getchar();
  }
  if (ch == EOF) return EOF; // last line
  return 1;
}

void run(bool thueMode) {
  size_t i, matchCap = 5;
  struct Trie *state = AcTrie;
  struct Match *matches = malloc(sizeof(struct Match) * matchCap);
  struct StringBuffer ms;
  bool eof = false, outputFirstChar = false;
  ms.capacity = RuleBuf.capacity;
  ms.size = RuleBuf.size;
  ms.buf = calloc(ms.capacity, 1);
  memcpy(ms.buf, RuleBuf.buf, RuleBuf.size);
  while (1) {
    // show string
    /*for (i = 0; i < ms.size; i++) {
      if (isprint(ms.buf[i])) putchar(ms.buf[i]);
      else printf("\\x%.02x", (unsigned char)ms.buf[i]);
    }
    puts("");*/
    // find match
    size_t matchCount = 0;
    state = AcTrie;
    for (i = 0; i < ms.size; i++) {
      char ch = ms.buf[i];
      struct Trie *t = Trie_advance(state, ch);
      while (t == NULL && state != AcTrie) {
        state = state->parent;
        if (state == NULL) state = AcTrie;
        t = Trie_advance(state, ch);
      }
      if (t == NULL) t = AcTrie;
      state = t;
      while (t != NULL) {
        struct Trie *td = t;
        while (td != NULL && td->payload != NULL) {
          struct ThubiRule *tr = td->payload;
          while (tr != NULL) {
            matches[matchCount].pos = i+1;
            matches[matchCount].rule = tr;
            matchCount++;
            if (matchCount >= matchCap) {
              matchCap *= 2;
              matches = realloc(matches, sizeof(struct Match) * matchCap);
            }
            tr = tr->next;
          }
          td = td->dict;
        }
        break;
      }
    }
    int ch;
    if (!thueMode && ms.size > 0) { // can output char
      ch = (unsigned char) ms.buf[0];
      outputFirstChar = ch < 0x80 || (ch >= 0xc2 && ch < 0xc4) || (ch == 0xc4 && ms.buf[1] == -127);
    }
    else outputFirstChar = false;
    if (matchCount + outputFirstChar == 0) {
      // Thue stops when no more rule can be applied
      if (thueMode) break;
      // input one char
      if (eof) break;
      fflush(stdout);
      int ch = getchar();
      if (ch == EOF) { ch = SYMBOL_S; eof = true; }
      size_t newsize = ms.size + (ch >= 0x80 ? 2 : 1);
      if (newsize > ms.capacity) {
        ms.capacity *= 2;
        ms.buf = realloc(ms.buf, ms.capacity);
      }
      if (ch < 0x80) {
        ms.buf[ms.size] = ch;
      }
      else {
        ms.buf[ms.size] = 0xc0 | ch>>6;
        ms.buf[ms.size+1] = 0x80 | (ch&0x3f);
      }
      ms.size = newsize;
      continue;
    }
    // rewrite string
    size_t r = rand() % (matchCount + outputFirstChar);
    if (r == matchCount && outputFirstChar) {
      // output first char
      int pos = 0;
      if (ch == 0xc4 && ms.buf[1] == -127) { // \s
        break; // stop program execution
      }
      if (ch >= 0xc2 && ch < 0xc4) { // 128~255
        putchar(((ch<<6) & 0xc0) | (ms.buf[1] & 0x3f));
        pos = 2;
      }
      else if (ch < 0x80) {
        putchar(ch);
        pos = 1;
      }
      memmove(ms.buf, ms.buf + pos, ms.size - pos);
      ms.size -= pos;
      continue;
    }
    struct ThubiRule *tr = matches[r].rule;
    size_t pos = matches[r].pos;
    size_t rhslen = tr->rhslen;
    if (tr->mode == PrintRule) {
      rhslen = 0;
      if (tr->rhslen > 0) {
        for (i = 0; i < tr->rhslen; i++) {
          int ch = (unsigned char) tr->rhs[i];
          if (ch >= 0xc2 && ch < 0xc4) { // 128~255
            putchar(((ch<<6) & 0xc0) | (tr->rhs[i+1] & 0x3f));
            i++;
          }
          else if (ch < 0x80) {
            putchar(ch);
          }
        }
      }
      else puts("");
    }
    else if (tr->mode == InputRule) {
      if (eof) {
        rhslen = 0;
      }
      else {
        eof = getUserInputLine() == EOF;
        rhslen = RuleBuf.size;
      }
    }
    if (ms.size + rhslen - tr->lhslen > ms.capacity) {
      ms.capacity *= 2;
      ms.buf = realloc(ms.buf, ms.capacity);
    }
    if (rhslen != tr->lhslen) {
      memmove(ms.buf + pos + rhslen - tr->lhslen, ms.buf + pos, ms.size - pos);
    }
    if (tr->mode == InputRule) {
      memcpy(ms.buf + pos - tr->lhslen, RuleBuf.buf, rhslen);
    }
    else if (tr->mode == ReplaceRule) {
      memcpy(ms.buf + pos - tr->lhslen, tr->rhs, rhslen);
    }
    ms.size += rhslen - tr->lhslen;
  }
  free(matches);
  StrBuf_destroy(&ms);
}

int main(int argc, char *argv[])
{
  FILE *f;
  const char *filename = NULL;
  bool thueMode = false;
  int i;
  for (i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      if (strcmp(argv[i], "-thue") == 0) {
        thueMode = true;
      }
    }
    else {
      filename = argv[i];
      break;
    }
  }
  if (filename == NULL) {
    fprintf(stderr, "usage: %s [-thue] [thubi program name]\n", argv[0]);
    return 1;
  }
  f = fopen(filename, "r");
  if (f == NULL) {
    fprintf(stderr, "unable to open file %s\n", filename);
    return 1;
  }
  Symbols = Trie_create();
  if (Symbols == NULL) OutOfMemory();
  AcTrie = Trie_create();
  if (AcTrie == NULL) OutOfMemory();
  AcStateCount++;
  addBuiltinSymbols();
  StrBuf_init(&RuleBuf);
  if (thueMode) {
    parseThueFile(f);
  } else {
    parseFile(f);
  }
  fclose(f);
  buildAcAutomata();
  srand(time(NULL));
  run(thueMode);
  StrBuf_destroy(&RuleBuf);
  return 0;
}

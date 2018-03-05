#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "trie.h"
#define OutOfMemory()  do { fprintf(stderr, "out of memory\n"); exit(1); } while (0)

unsigned char *LineBuf;
size_t LineBufSize = 0, LineBufCap = 6;

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
        printf("define %s\n", LineBuf);
      }
      else if (LineBuf[0] == '#') {
        printf("comment %s\n", LineBuf);
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
        printf("comment %s\n", LineBuf);
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
  parseFile(f);
  fclose(f);
  return 0;
}

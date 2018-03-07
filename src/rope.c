#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rope.h"
#define OutOfMemory()  do { fprintf(stderr, "out of memory\n"); exit(1); } while (0)
#define DEPTH(x)  ((x)->type == FlatRopeType ? 0 : (x)->con.depth)

enum RopeType {
  FlatRopeType, ConcatRopeType, FreedRopeType
};

struct FlatRope {
  char type;
  char s_len;
  char data[0];
};

struct ConcatRope {
  char type;
  char left_len;
  char depth;
  char what;
  int len;
  ROPE left;
  ROPE right;
};

union Rope {
  struct FlatRope flat;
  struct ConcatRope con;
  struct {
    char type;
    char u1,u2,u3;
    union Rope *next;
  };
};

static const int MAX_FLAT_LEN = sizeof(union Rope) - 3;
union Rope *freeNodes, *freeNodesTop;

ROPE Rope_create() {
  ROPE t = freeNodesTop;
  if (t == NULL) OutOfMemory();
  freeNodesTop = freeNodesTop->next;
  return t;
}

void Rope_free(ROPE r){
  if (r->type == FreedRopeType) {
    perror("double free\n");
    exit(1);
  }
  r->type = FreedRopeType;
  r->next = freeNodesTop;
  freeNodesTop = r;
}

ROPE Rope_createFromString(const char *data, int size) {
  ROPE a = Rope_create();
  if (size <= MAX_FLAT_LEN) {
    a->flat.type = FlatRopeType;
    a->flat.s_len = size;
    memcpy(a->flat.data, data, size);
    a->flat.data[size] = '\0';
    return a;
  }
  else {
    a->con.type = ConcatRopeType;
    a->con.len = size;
    int n = size/2;
    a->con.left = Rope_createFromString(data, n);
    a->con.right = Rope_createFromString(data + n, size - n);
    int d = DEPTH(a->con.left), dr = DEPTH(a->con.right);
    if (dr > d) d = dr;
    a->con.depth = d + 1;
  }
  return a;
}

void Rope_dump(ROPE r, int indent) {
  int i;
  printf("%*s", indent*2, "");
  switch (r->type) {
    case FlatRopeType:
      printf("flat: ");
      fwrite(r->flat.data, 1, r->flat.s_len, stdout);
      puts("");
      break;
    case ConcatRopeType:
      printf("concat %d %d\n", DEPTH(r), r->con.len);
      Rope_dump(r->con.left, indent+1);
      Rope_dump(r->con.right, indent+1);
      break;
  }
}

int main() {
  int n = 1 << 20;
  freeNodes = malloc(sizeof(union Rope) * n);
  freeNodes[0].type = FreedRopeType;
  for (int i = 1; i < n; i++) {
    freeNodes[i].type = FreedRopeType;
    freeNodes[i-1].next = &freeNodes[i];
  }
  freeNodesTop = freeNodes;
  ROPE a = Rope_createFromString("12345678901234567890123456789012345678901234", 44);
  Rope_dump(a, 0);
  getchar();
  free(freeNodes);
}

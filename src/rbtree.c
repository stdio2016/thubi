#include <stdio.h>
#include <stdlib.h>
#define COLOR(t) ((t)->depth & 1)
#define BLACK 1
#define RED   0

struct RBTree {
    struct RBTree *left, *right;
    int depth, size;
    long long data;
};

struct RBTree nilData = {&nilData, &nilData, BLACK, 0, 0};
struct RBTree *nil = &nilData;
typedef struct RBTree *S2T;

S2T RBTree_create(int data) {
    S2T t = malloc(sizeof(struct RBTree));
    t->left = nil;
    t->right = nil;
    t->depth = RED;
    t->size = 1;
    t->data = data;
    return t;
}

void RBTree_update(S2T node) {
    node->size = node->left->size + node->right->size + 1;
    int deep = node->left->depth >> 1;
    int color = COLOR(node);
    node->depth = (deep + color) << 1 | color;
}

void RBTree_rotate_right(S2T *tree) {
    S2T t = *tree;
    S2T child = t->left;
    t->left = child->right;
    child->right = t;
    *tree = child;
    RBTree_update(t);
    RBTree_update(child);
}

void RBTree_rotate_left(S2T *tree) {
    S2T t = *tree;
    S2T child = t->right;
    t->right = child->left;
    child->left = t;
    *tree = child;
    RBTree_update(t);
    RBTree_update(child);
}

static int RBTree_insert_helper(S2T *root, int pos, int data) {
    S2T t = *root;
    if (t == nil) {
        *root = RBTree_create(data);
        return 0;
    }
    int y;
    if (pos <= t->left->size) { // go left or right?
        y = RBTree_insert_helper(&t->left, pos, data);
        RBTree_update(t);
        if (y == 0) {
            if (COLOR(t) == BLACK) return 100; // case 2
            return 1;
        }
        else if (y != 100) {
            if (COLOR(t->right) == RED) { // case 5
                t->depth ^= 1; // black -> red
                t->left->depth += 3; // red -> black
                t->right->depth += 3; // red -> black
                return 0;
            }
            else {
                if (y == 1) { // case 3
                    t->depth ^= 1;
                    t->left->depth ^= 1;
                    RBTree_rotate_right(root);
                    return 100;
                }
                else if (y == 2) { // case 4
                    t->left->right->depth ^= 1;
                    RBTree_rotate_left(&t->left);
                    t->depth ^= 1;
                    RBTree_rotate_right(root);
                    return 100;
                }
            }
        }
    }
    else {
        y = RBTree_insert_helper(&t->right, pos - t->left->size - 1, data);
        RBTree_update(t);
        if (y == 0) {
            if (COLOR(t) == BLACK) return 100; // case 2
            return 2;
        }
        else if (y != 100) {
            if (COLOR(t->left) == RED) { // case 5
                t->depth ^= 1; // black -> red
                t->left->depth += 3; // red -> black
                t->right->depth += 3; // red -> black
                return 0;
            }
            else {
                if (y == 2) { // case 3
                    t->depth ^= 1;
                    t->right->depth ^= 1;
                    RBTree_rotate_left(root);
                    return 100;
                }
                else if (y == 1) { // case 4
                    t->right->left->depth ^= 1;
                    RBTree_rotate_right(&t->right);
                    t->depth ^= 1;
                    RBTree_rotate_left(root);
                    return 100;
                }
            }
        }
    }
    return 100;
}

void RBTree_insert(S2T *root, int pos, int data) {
    RBTree_insert_helper(root, pos, data);
    if (COLOR(*root) == RED) { // case 1
        (*root)->depth += 3;
    }
}

void RBTree_show(S2T tree, int indent) {
    if (tree == nil) {
        //printf("%*s", indent*2, "");
        //puts("nil");
    }
    else {
        RBTree_show(tree->left, indent+1);
        printf("%*s", indent*2, "");
        printf("%s data %ld\n", COLOR(tree) == BLACK ? "bla" : "red", tree->data);
        RBTree_show(tree->right, indent+1);
    }
}

int main(void) {
    S2T n = nil;
    int i;
    int pos, data;
    while (scanf("%d %d", &pos, &data) == 2) {
        RBTree_insert(&n, pos, data);
        RBTree_show(n, 0);
    }
    return 0;
}

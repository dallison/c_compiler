//
//  main.c
//  tree_test
//
//  Created by David Allison on 12/31/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#include <stdio.h>
#include "binary_tree.h"
#include <stdlib.h>
#include <assert.h>

typedef struct Node {
  BinaryTreeNode header;
  int key;
  int value;
} Node;

static int InsertCompare(BinaryTreeNode* node1, BinaryTreeNode* node2) {
  Node* v1 = (Node*)node1;
  Node* v2 = (Node*)node2;
  return v1->key - v2->key;
}

static int SearchCompare(BinaryTreeNode* node, void* key) {
  Node* v = (Node*)node;
  return v->key - (int)key;
}

static void Destructor(BinaryTreeNode* node, void* data) {
}

static void PrintNode(Node* node, int indent) {
  for (int i = 0; i < indent * 2; i++) {
    printf("%s", " ");
  }
  Node* parent = (Node*)node->header.parent;
  printf("%d: %d [%s] (%d)\n", node->key,
         node->value,
         node->header.color == kBinaryTreeNodeRed ? "RED" : "BLACK",
         parent == NULL ? -1 : parent->key);
}

static void Printer(BinaryTreeNode* node, int depth, void* data) {
  PrintNode((Node*)node, depth);
}

void PrintBinaryTree(BinaryTree* t) {
  BinaryTreeTraverse(t, Printer, NULL);
}

static void Insert(BinaryTree* t, int k, int v) {
  Node* node = malloc(sizeof(Node));
  BinaryTreeNodeInit(&node->header);
  node->key = k;
  node->value = v;
  BinaryTreeInsert(t, &node->header);
}

int main(int argc, const char * argv[]) {
  BinaryTree t;
  BinaryTreeInit(&t, InsertCompare, SearchCompare, Destructor);
#if 0
  Insert(&t, 11);
  Insert(&t, 2);
  Insert(&t, 14);
  Insert(&t, 15);
  Insert(&t, 7);
  Insert(&t, 1);
  Insert(&t, 5);
  Insert(&t, 8);
  PrintBinaryTree(&t);
  
  Insert(&t, 16);
  PrintBinaryTree(&t);
#endif
  const int N = 10;
  for (int i = 0; i < N; i++) {
    printf("*** %d\n", i);
    Insert(&t, i, i * 5);
    PrintBinaryTree(&t);
  }
  for (int i = 0; i < N; i++) {
    BinaryTreeNode* n = BinaryTreeSearch(&t, (void*)i);
    assert(n != NULL);
  }
}

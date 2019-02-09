//
//  main.c
//  map_test
//
//  Created by David Allison on 12/17/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include "map.h"
#include <stdio.h>
#include <string.h>

int CompareString(const void* a, const void* b) {
  MapKeyValue* kv1 = (MapKeyValue*)a;
  MapKeyValue* kv2 = (MapKeyValue*)b;
  return strcmp(kv1->key, kv2->key);
}

void Print(const MapKeyValue* kv) {
  printf("%s: %d", (char*)kv->key, (int)kv->value);
}


int CompareInt(const void* a, const void* b) {
  MapKeyValue* kv1 = (MapKeyValue*)a;
  MapKeyValue* kv2 = (MapKeyValue*)b;
  return kv1->key - kv2->key;
}

void PrintInt(const MapKeyValue* kv) {
  printf("%d: %d", (int)kv->key, (int)kv->value);
}

int main(int argc, const char * argv[]) {
  // Map with integer keys to test the binary insertion.
  Map intmap;
  MapInit(&intmap, CompareInt);
  
  // First 4 use linear insertion.
  MapInsert(&intmap, (void*)30, (void*)300);
  MapInsert(&intmap, (void*)10, (void*)100);
  MapInsert(&intmap, (void*)40, (void*)400);
  MapInsert(&intmap, (void*)20, (void*)200);

  // These will be inserted using a binary insert.
  MapInsert(&intmap, (void*)5, (void*)50);
  MapInsert(&intmap, (void*)25, (void*)250);
  MapInsert(&intmap, (void*)50, (void*)500);
  MapInsert(&intmap, (void*)3, (void*)30);
  MapInsert(&intmap, (void*)51, (void*)510);
  MapInsert(&intmap, (void*)10, (void*)10000);  // Replacement.

  MapPrint(&intmap, PrintInt);
  printf("\n");

  // Map with string keys to test retrieval.
  Map map;
  MapInit(&map, CompareString);

  MapInsert(&map, "San Ramon", (void*)50000);
  MapPrint(&map, Print);
  printf("\n");

  MapInsert(&map, "Danville", (void*)30000);
  MapPrint(&map, Print);
  printf("\n");

  MapInsert(&map, "Walnut Creek", (void*)100000);
  MapPrint(&map, Print);
  printf("\n");

  MapInsert(&map, "Walnut Creek", (void*)200000);
  MapPrint(&map, Print);
  printf("\n");

  MapInsert(&map, "Sunnyvale", (void*)300000);
  MapPrint(&map, Print);
  printf("\n");

  void* r = MapFind(&map, "San Ramon");
  if (r == NULL) {
    abort();
  }
  printf("%d\n", (int)r);

  MapRemove(&map, "Danville");
  MapPrint(&map, Print);
  printf("\n");

  MapRemove(&map, "Walnut Creek");
  MapPrint(&map, Print);
  printf("\n");

}

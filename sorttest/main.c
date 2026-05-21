#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void Swap(void* p, void* q, size_t size) {
  char* a = p;
  char* b = q;

  for (size_t i = 0; i < size; i++) {
    char tmp = *a;
    *a = *b;
    *b = tmp;
    a++;
    b++;
  }
}

void PrintInt(void* base, size_t i) {
  printf("%d: %d\n", i, *(int*)(base+i));
}

static void Quicksort(void *base, size_t size, ssize_t first,
                      ssize_t last,
           int (*compar)(const void *, const void *)) {
#if 1
  if (first < last) {
    size_t x = (first + last) / 2;
    x -= x % size;
    ssize_t i = first;
    ssize_t j = last;
  
    printf("x: %zd\n", x);
    printf("---------\n");
    do {
      for (int a = first; a <= last; a += size) {
        PrintInt(base, a);
      }
      printf("");
      while (compar(base+i, base+x) < 0) {
        i += size;
      }
      while (compar(base+j, base+x) > 0) {
        j -= size;
      }
      if (i <= j) {
        // Swap base[i] and base[j];
        if (i != j) {
          printf("swapping %zd and %zd\n", i, j);
          // If we move the pivot, keep track of where it went.
          if (x == i) {
            x = j;
          } else if (x == j) {
            x = i;
          }
          Swap(base+i, base+j, size);
        }
        i += size;
        j -= size;
      }
    } while (i <= j);
    Quicksort(base, size, first, j, compar);
    Quicksort(base, size, i, last, compar);
  }
#else
  if (first < last) {
    size_t pivot = (first + last) / 2;
    pivot -= pivot % size;
    ssize_t i = first;
    ssize_t j = last;

    printf("pivot: %zd\n", pivot);
    while (i < j) {
      while(compar(base+i, base+pivot) <= 0 && i < last) {
        i += size;
      }
      while(compar(base+j, base+pivot) > 0) {
        j -= size;
      }
      if (i < j) {
        printf("swapping %zd and %zd\n", i, j);
        Swap(base+i, base+j, size);
      }
    }

    if (pivot != j) {
      Swap(base+pivot, base+j, size);
    }
    Quicksort(base, size, first, j - size, compar);
    Quicksort(base, size, j + size, last, compar);
   }
#endif
}
  
void Qsort(void *base, size_t nmemb, size_t size,
           int (*compar)(const void *, const void *)) {
  Quicksort(base, size, 0, nmemb * size - size, compar);
}
  
int Compare(const void* a, const void* b) {
  int i1 = *(int*)a;
  int i2 = *(int*)b;
  // printf("compare %d and %d\n", i1, i2);
  return i1 - i2;
}

void *Bsearch(const void *key, const void *base,
              size_t nmemb, size_t size,
              int (*compar)(const void *
                            , const void *)) {
  size_t low = 0;
  size_t len = nmemb * size;
  size_t high = len;              // One beyond end of array.
  while (low < high) {
    size_t mid = low + (high - low) / 2;   // Offset to middle.
    mid -= mid % size;                     // Aligned down to size.
    // printf("comparing %d and %d\n", *(int*)key, *(int*)(base+mid));
    int v = compar(key, base+mid);
    if (v == 0) {
      // Found.
      return (void*)base + mid;
    }
    if (v < 0) {
      high = mid;
    } else {
      low = mid + size;
    }
  }
  return NULL;
}

#define LEN 20
int main() {
  int numbers[LEN] = {5,1,7,9,2,4,7,6,3,8,1,-5,3,8,5,1,45,7,8,4};
//  for (int i = 0; i < LEN; i++) {
//    numbers[i] = rand();
//  }
  
  int a[LEN];
  memcpy(a, numbers, LEN*sizeof(int));
 
  int b[LEN];
  memcpy(b, numbers, LEN*sizeof(int));
  

  qsort(a, LEN, sizeof(int), Compare);
  Qsort(b, LEN, sizeof(int), Compare);

  if (memcmp(a, b, LEN * sizeof(int)) != 0) {
    printf("before\n");
    for (int i = 0; i < LEN; i++) {
      printf("%d: %d\n", i * sizeof(int), numbers[i]);
    }
    
    printf("\na after\n");
    for (int i = 0; i < LEN; i++) {
      printf("%d\n", a[i]);
    }
    
    printf("\nb after\n");
    for (int i = 0; i < LEN; i++) {
      printf("%d\n", b[i]);
    }
  } else {
    int index = a[rand() % LEN];
    void* v = Bsearch(&index, a, LEN, sizeof(int), Compare);
    if (v == NULL) {
      printf("NOT FOUND\n");
      exit(1);
    }
    if (*(int*)v != index) {
      printf("VALUE DIFF %d/%d\n", *(int*)v, index);
      exit(1);
    }
    printf("PASS\n");
  }
}


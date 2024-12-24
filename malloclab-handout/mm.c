/*
 * my mm.c
 * Implemented with segregate list, each list use first-fit policy
 * and each free list insert after the head node. I think it is both
 * efficient and space-less
 */
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "memlib.h"
#include "mm.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};
/* single word (4) or double word (8) alignment */

#define ALIGNMENT 8
/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))
// Basic constants and macros
#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1 << 12)
#define MAX(x, y) ((x) > (y) ? (x) : (y));
#define PACK(size, alloc) ((size) | (alloc))
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))
#define GET_POINT(p) (*(char **)(p))
#define PUT_POINT(p, val) (*(char **)(p) = (val))
#define PREP(bp) (bp)
#define SUCP(bp) ((char *)(bp) + WSIZE)

// global variable
char *heap_listp;
char **list_32;
char **list_64;
char **list_128;
char **list_256;
char **list_512;
char **list_1024;
char **list_2048;
char **list_infinite;

// helper function
static void insert_list(void *bp, void **list);
static void *choose_list(size_t size);
static void *coalesce(void *bp);
static void *extend_heap(size_t size);
static char *head_pointer(char *bp);
static void isinCorrectfreelist();
static void heap_checker();
static void pop_block(char *bp);
static void *find_block(size_t size);
static void place(char *bp, size_t size);
#define CHECKHEAP(lineno)                                                      \
  printf("%d  in function %s\n", __LINE__, __func__);                          \
  heap_checker();
/*
 * mm_init - initialize the malloc package.
 */
// TODO:initialize
int mm_init(void) {
  if ((heap_listp = mem_sbrk(12 * WSIZE)) == (void *)-1) {
    return -1;
  }
  PUT(heap_listp, 0);
  PUT_POINT(heap_listp + WSIZE, NULL);
  list_32 = (char **)(heap_listp + WSIZE);
  PUT_POINT(list_32 + 1, NULL);
  list_64 = list_32 + 1;
  PUT_POINT(list_32 + 2, NULL);
  list_128 = list_64 + 1;
  PUT_POINT(list_32 + 3, NULL);
  list_256 = list_128 + 1;
  PUT_POINT(list_32 + 4, NULL);
  list_512 = list_256 + 1;
  PUT_POINT(list_32 + 5, NULL);
  list_1024 = list_512 + 1;
  PUT_POINT(list_32 + 6, NULL);
  list_2048 = list_1024 + 1;
  PUT_POINT(list_32 + 7, NULL);
  list_infinite = list_2048 + 1;
  PUT(list_infinite + 1, PACK(8, 1));
  PUT(list_infinite + 2, PACK(8, 1));
  PUT(list_infinite + 3, PACK(0, 1));
  heap_listp = (char *)(list_infinite + 2);
  if (extend_heap(CHUNKSIZE) == NULL) {
    return -1;
  }
  return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {
  size_t asize;
  char *bp;
  size_t extendsize;
  asize = ALIGN(size + DSIZE);
  if ((bp = find_block(asize)) != NULL) {
    pop_block(bp);
    place(bp, asize);
    return bp;
  };
  extendsize = MAX(asize, CHUNKSIZE);
  if ((bp = extend_heap(extendsize)) == NULL) {
    return NULL;
  }
  pop_block(bp);
  place(bp, asize);
  printf("malloc a block which size is %zu\n", size);
  CHECKHEAP(lineno)
  return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr) {
  PUT(HDRP(ptr), PACK(GET_SIZE(HDRP(ptr)), 0));
  PUT(FTRP(ptr), PACK(GET_SIZE(HDRP(ptr)), 0));
  coalesce(ptr);
  insert_list(ptr, choose_list(GET_SIZE(HDRP(ptr))));
  printf("free a block\n");
  CHECKHEAP(lineno)
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size) {
  void *oldptr = ptr;
  void *newptr;
  size_t copySize;

  newptr = mm_malloc(size);
  if (newptr == NULL)
    return NULL;
  copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
  if (size < copySize)
    copySize = size;
  memcpy(newptr, oldptr, copySize);
  mm_free(oldptr);
  return newptr;
}

// head insertion methodstatic void insert_list(void *bp, void **list) {}
static void insert_list(void *bp, void **list) {
  char *head = *list;
  *list = (char *)bp;
  PUT_POINT(SUCP(bp), head);
  PUT_POINT(PREP(bp), (char *)list);
  if (head) {
    PUT_POINT(PREP(GET_POINT(SUCP(bp))), bp);
  }
  PUT(HDRP(bp), PACK(GET_SIZE(HDRP(bp)), 0));
  PUT(FTRP(bp), PACK(GET_SIZE(HDRP(bp)), 0));
  CHECKHEAP(lineno)
}

static void pop_block(char *bp) {
  // if the node is the first node
  int isheadnode = (GET_POINT(PREP(bp)) < heap_listp);
  int nextisnull = (GET_POINT(SUCP(bp)) == NULL);
  if (isheadnode && !nextisnull) {
    PUT_POINT(PREP(bp), GET_POINT(SUCP(bp)));
    PUT_POINT(PREP(GET_POINT(SUCP(bp))), GET_POINT(PREP(bp)));
    PUT(HDRP(bp), PACK(GET_SIZE(HDRP(bp)), 1));
    PUT(FTRP(bp), PACK(GET_SIZE(HDRP(bp)), 1));
  }
  if (!isheadnode && nextisnull) {
    PUT_POINT(SUCP(GET_POINT(PREP(bp))), NULL);
    PUT(HDRP(bp), PACK(GET_SIZE(HDRP(bp)), 1));
    PUT(FTRP(bp), PACK(GET_SIZE(HDRP(bp)), 1));
  }
  if (isheadnode && nextisnull) {
    PUT_POINT(GET_POINT(PREP(bp)), NULL);
    PUT(HDRP(bp), PACK(GET_SIZE(HDRP(bp)), 1));
    PUT(FTRP(bp), PACK(GET_SIZE(HDRP(bp)), 1));
  } else {
    GET_POINT(SUCP(GET_POINT(PREP(bp)))) = GET_POINT(SUCP(bp));
    GET_POINT(PREP(GET_POINT(SUCP(bp)))) = GET_POINT(PREP(bp));
    PUT(HDRP(bp), PACK(GET_SIZE(HDRP(bp)), 1));
    PUT(FTRP(bp), PACK(GET_SIZE(HDRP(bp)), 1));
  }
  CHECKHEAP(lineno);
}

static void *choose_list(size_t size) {
  if (size >= 16 && size < 32) {
    return list_32;
  }
  if (size >= 32 && size < 64) {
    return list_64;
  }
  if (size >= 64 && size < 128) {
    return list_128;
  }
  if (size >= 128 && size < 256) {
    return list_256;
  }
  if (size >= 256 && size < 512) {
    return list_512;
  }
  if (size >= 512 && size < 1024) {
    return list_1024;
  }
  if (size >= 1024 && size < 2048) {
    return list_2048;
  }
  if (size >= 2048) {
    return list_infinite;
  }
  return NULL;
}

static void *coalesce(void *bp) {
  size_t pre_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
  size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
  size_t size = GET_SIZE(HDRP(bp));

  if (pre_alloc && next_alloc) {
    return bp;
  } else if (pre_alloc && !next_alloc) {
    size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
    pop_block(NEXT_BLKP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    insert_list(bp, choose_list(size));
  } else if (!pre_alloc && next_alloc) {
    size += GET_SIZE(HDRP(PREV_BLKP(bp)));
    pop_block(PREV_BLKP(bp));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    PUT(FTRP(PREV_BLKP(bp)), PACK(size, 0));
    bp = PREV_BLKP(bp);
    insert_list(bp, choose_list(size));
  } else {
    size += GET_SIZE(FTRP(NEXT_BLKP(bp))) + GET_SIZE(HDRP(PREV_BLKP(bp)));
    pop_block(NEXT_BLKP(bp));
    pop_block(PREV_BLKP(bp));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
    bp = PREV_BLKP(bp);
    insert_list(bp, choose_list(size));
  }
  CHECKHEAP(lineno)
  return bp;
}

static void *extend_heap(size_t size) {
  char *bp;
  if ((long)(bp = mem_sbrk((size))) == -1) {
    return NULL;
  }

  PUT(HDRP(bp), PACK(size, 0));
  PUT(FTRP(bp), PACK(size, 0));
  PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
  PUT_POINT(PREP(bp), NULL);
  PUT_POINT(SUCP(bp), NULL);
  bp = coalesce(bp);
  insert_list(bp, choose_list(size));
  CHECKHEAP(lineno)
  return bp;
}

static char *head_pointer(char *bp) {
  if (GET_ALLOC(HDRP(bp))) {
    printf("it is not free block\n");
    return NULL;
  }
  // if bp equal head_pointer,return bp
  while (bp >= heap_listp) {
    bp = GET_POINT(PREP(bp));
  }
  return bp;
}

static void isinCorrectfreelist() {
  char *bp = heap_listp;
  while (GET_SIZE(HDRP(bp))) {
    if (!GET_ALLOC(HDRP(bp))) {
      if (!(head_pointer(bp) == choose_list(GET_SIZE(HDRP(bp))))) {
        printf("the free block is not in correct list\n");
      }
    }
    bp = NEXT_BLKP(bp);
  }
}

static void *find_block(size_t size) {
  for (char **list = choose_list(size);
       (void *)list != (void *)(heap_listp - WSIZE); list++) {
    char *bp = *list;
    while (bp != NULL) {
      if (GET_SIZE(HDRP(bp)) >= size) {
        return bp;
      } else {
        bp = GET_POINT(SUCP(bp));
      }
    }
  }
  return NULL;
}

static void place(char *bp, size_t size) {
  int asize = GET_SIZE(HDRP(bp)) - size;
  if (asize >= (size_t)(2 * DSIZE)) {
    PUT(HDRP(bp), PACK(size, 1));
    PUT(FTRP(bp), PACK(size, 1));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(asize, 0));
    PUT(FTRP(NEXT_BLKP(bp)), PACK(asize, 0));
    insert_list(NEXT_BLKP(bp), choose_list(asize));
  }
  PUT(HDRP(bp), PACK(GET_SIZE(HDRP(bp)), 1));
  PUT(FTRP(bp), PACK(GET_SIZE(HDRP(bp)), 1));
}

static void inRange() {
  for (char **list = list_32; (void *)list != (void *)(heap_listp - WSIZE);
       list++) {
    char *cur = *list;
    while (cur) {
      if (cur > heap_listp && (void *)cur <= mem_heap_hi()) {
        cur = GET_POINT(SUCP(cur));
      } else {
        printf("block in free list is not in heap\n");
      }
    }
  }
}

static void heap_checker() {
  isinCorrectfreelist();
  inRange();
}

/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Jin Hoon Bang",
    /* First member's email address */
    "jinhoonbang@u.northwestern.edu",
    /* Second member's full name (leave blank if none) */
    "Ethan Suh",
    /* Second member's email address (leave blank if none) */
    "dysuh@u.northwestern.edu"
};

/* single word (4) or double word (8) alignment */
#define WSIZE 4
#define ALIGNMENT 8
#define CHUNKSIZE (1<<12)
#define SEG_LIST_SIZE 12

//#define PACK(size, alloc) ((size | alloc))
#define PACK(size, prev_ftr, alloc) ((size | prev_ftr << 1 | alloc))

#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define GET_PREV_ALLOC(p) (GET(p) & 0x2)

#define HEADER_P(bp) ((char *)(bp) - WSIZE)
#define FOOTER_P(bp) ((char *)(bp) + GET_SIZE(HEADER_P(bp)) - ALIGNMENT)

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - ALIGNMENT)))

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

void *free_lists[SEG_LIST_SIZE];

static void* extend_heap(size_t words);
static void* coalesce(void* bp);

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
	return NULL;
    else {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
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

static void* extend_heap(size_t words) {
	char *bp;
	size_t size;

	size = (words % 2) ? (words + 1)*WSIZE : words * WSIZE;
	if ((long)(bp = mem_sbrk(size)) == -1)
		return NULL;
	
	PUT(HEADER_P(bp), PACK(size, 0, 0));
	//PUT linked list pointers
	PUT(FOOTER_P(bp), PACK(size, 0, 0));
	PUT(HEADER_P(NEXT_BLKP(bp)), PACK(size, 0, 1));

	//add coalesce and insert to free list
	
	return bp;
}

static void* coalesce(void* bp) {
	size_t prev_alloc = GET_PREV_ALLOC(HEADER_P(bp));
	size_t next_alloc = GET_ALLOC(HEADER_P(NEXT_BLKP(bp)));
	size_t size = GET_SIZE(HEADER_P(bp));

	// prev allocated, next allocated
	if (prev_alloc && next_alloc) {
		return bp;
	}
	// prev allocated, next free
	else if (prev_alloc && !next_alloc) {
		size += GET_SIZE(HEADER_P(NEXT_BLKP(bp)));
		PUT(HEADER_P(bp), PACK(size, prev_alloc, 0));
		PUT(FOOTER_P(bp), PACK(size, 0, 0));
	}
	// prev free, next allocated
	else if (!prev_alloc && next_alloc) {
		size += GET_SIZE(HEADER_P(PREV_BLKP(bp)));
		PUT(HEADER_P(PREV_BLKP(bp)), PACK(size, GET_PREV_ALLOC(HEADER_P(PREV_BLKP(bp))), 0));
		PUT(FOOTER_P(bp), PACK(size, 0, 0));
		bp = PREV_BLKP(bp);
	}
	// prev free, next free
	else {
		size += GET_SIZE(HEADER_P(PREV_BLKP(bp))) + GET_SIZE(FOOTER_P(NEXT_BLKP(bp)));
		PUT(HEADER_P(PREV_BLKP(bp)), PACK(size, GET_PREV_ALLOC(HEADER_P(PREV_BLKP(bp))), 0));
		PUT(FOOTER_P(NEXT_BLKP(bp)), PACK(size, 0, 0));
		bp = PREV_BLKP(bp);
	}

	return bp;
}












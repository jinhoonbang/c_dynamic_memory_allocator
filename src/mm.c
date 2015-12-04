/* * mm-naive.c - The fastest, least memory-efficient malloc package.  * 
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
    "je",
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
#define SEG_LIST_SIZE 20

#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define PACK(size, prev_alloc, alloc) ((size | prev_alloc << 1 | alloc))

#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define GET_PREV_ALLOC(p) (GET(p) & 0x2)

#define HEADER_P(bp) ((char *)(bp) - WSIZE)
#define FOOTER_P(bp) ((char *)(bp) + GET_SIZE(HEADER_P(bp)) - ALIGNMENT)

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - ALIGNMENT)))

#define SET_PREV_NODE(bp, prev_node) (*(unsigned int *)(bp) = (unsigned int)(prev_node))
#define SET_NEXT_NODE(bp, next_node) (*(unsigned int *)(bp + WSIZE) = (unsigned int)(next_node))

#define GET_PREV_NODE(bp) ((char *)(bp))
#define GET_NEXT_NODE(bp) ((char *)(bp + WSIZE))

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

void *free_lists[SEG_LIST_SIZE];

static void* extend_heap(size_t words);
static void* coalesce(void* bp);
static void* insert_node(void* bp);
static void* delete_node(void* bp);

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
	//declare pointer to char
	void *heap_start;
	void *bp;

	//initialize segregated free lists
	int i;
	for (i = 0; i < SEG_LIST_SIZE; i++)
		free_lists[i] = NULL;
	//(long)(heap_start = mem_sbrk(4 * WSIZE))== -1
	if ((heap_start = mem_sbrk(4*WSIZE)) == (void *)-1)
		return -1;
	
	//change prev_alignment to 0 or 1?? 
	PUT(heap_start, 0);
	PUT(heap_start + (1*WSIZE), PACK(ALIGNMENT, 1, 1));
	PUT(heap_start + (2*WSIZE), PACK(ALIGNMENT, 0, 1));
	PUT(heap_start + (3*WSIZE), PACK(0, 1, 1));
	heap_start += ALIGNMENT;

	bp = extend_heap(CHUNKSIZE/WSIZE);
	if (bp  == NULL)
		return -1;
	
	insert_node(bp);
	return 0;
}

/*
 * takes argument size and pad it to satisfy align req. 
 * add header and footer 
 * find matching free space from segregated_free_list
 * split free space if the required block is smaller by ALIGNMENT
 * extend heap if there is no matching free space in segregated_free_list
 *
 */
void *mm_malloc(size_t size)
{
    // int newsize = ALIGN(size + SIZE_T_SIZE);
    // void *p = mem_sbrk(newsize);
    // if (p == (void *)-1)
	// return NULL;
    // else {
    //     *(size_t *)p = size;
    //     return (void *)((char *)p + SIZE_T_SIZE);
    // }
	
	size_t asize;
	size_t extendsize;
	void *bp;

	if (size == 0)
		return NULL;

	if (size <= ALIGNMENT)
		asize = 2 * ALIGNMENT;
	else
		asize = ALIGNMENT * ((size + (ALIGNMENT) + (ALIGNMENT-1))/ALIGNMENT);

	//0x2 --> 0x1
	//finds matching entry from  for asize
	int i;
	int bp_i = 0;

	for (i = 0; i < SEG_LIST_SIZE; i++) {
		if ((0x1<<i) >= asize && free_lists[i] != NULL) {
			bp_i = i;
			break;
		}
	}

	bp = delete_node(free_lists[bp_i]);

	if (bp != NULL) {
		PUT(HEADER_P(bp), PACK(asize, GET_PREV_ALLOC(HEADER_P(bp)), 1));
		PUT(FOOTER_P(bp), PACK(asize, 0, 1));
		return bp;
	}
	
	extendsize = MAX(asize, CHUNKSIZE);
	if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
		return NULL;
	PUT(HEADER_P(bp), PACK(asize, GET_PREV_ALLOC(HEADER_P(bp)), 1));
	PUT(FOOTER_P(bp), PACK(asize, 0, 1));
	insert_node(bp)

	//what if we have to split? free  (CHUNKSIZE - asize)?
	
	return bp;
}

/*
 * mm_free - Freeing a block does something.
 */
void mm_free(void *bp)
{
	size_t size = GET_SIZE(HEADER_P(bp));

	PUT(HEADER_P(bp), PACK(size, GET_PREV_ALLOC(HEADER_P(bp)), 0));
	PUT(FOOTER_P(bp), PACK(size, 0, 0));
	//coalesce(bp);
	insert_node(bp);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 *
 *
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

/*
 * argument is number of words, not number of bytes.  
 *
 */

static void *extend_heap(size_t words)
{
  	char *bp;
  	size_t size;

  	size = (words % 2) ? (words + 1)*WSIZE : words * WSIZE;
  	if ((long)(bp = mem_sbrk(size)) == -1)
		return NULL;
  
    //how do we know PREV_ALLOC is 0?
  	PUT(HEADER_P(bp), PACK(size, 1, 0));
  	//PUT linked list pointers
  	PUT(FOOTER_P(bp), PACK(size, 0, 0));
  	PUT(HEADER_P(NEXT_BLKP(bp)), PACK(size, 0, 1));
  
  	//add coalesce and insert to free list  		
  	//bp = coalesce(bp);
	return bp;
}

static void *coalesce(void* bp) 
{
	size_t prev_alloc = GET_PREV_ALLOC(HEADER_P(bp));
  	size_t next_alloc = GET_ALLOC(HEADER_P(NEXT_BLKP(bp)));
  	size_t size = GET_SIZE(HEADER_P(bp));
  
  	// prev allocated, next allocated
  	if (prev_alloc && next_alloc) {
  		return bp;
  	}
  	// prev allocated, next free
  	else if (prev_alloc && !next_alloc) {
		delete_node(NEXT_BLKP(bp));
  		size += GET_SIZE(HEADER_P(NEXT_BLKP(bp)));
  		PUT(HEADER_P(bp), PACK(size, prev_alloc, 0));
  		PUT(FOOTER_P(bp), PACK(size, 0, 0));
  	}
  	// prev free, next allocated
  	else if (!prev_alloc && next_alloc) {
  		delete_node(PREV_BLKP(bp));
		size += GET_SIZE(HEADER_P(PREV_BLKP(bp)));
  		PUT(HEADER_P(PREV_BLKP(bp)), PACK(size, GET_PREV_ALLOC(HEADER_P(PREV_BLKP(bp))), 0));
  		PUT(FOOTER_P(bp), PACK(size, 0, 0));
  		bp = PREV_BLKP(bp);
  	}
  	// prev free, next free
  	else {
  	    delete_node(PREV_BLKP(bp)); 	
		delete_node(NEXT_BLKP(bp));
		size += GET_SIZE(HEADER_P(PREV_BLKP(bp))) + GET_SIZE(FOOTER_P(NEXT_BLKP(bp)));
  		PUT(HEADER_P(PREV_BLKP(bp)), PACK(size, GET_PREV_ALLOC(HEADER_P(PREV_BLKP(bp))), 0));
  		PUT(FOOTER_P(NEXT_BLKP(bp)), PACK(size, 0, 0));
  		bp = PREV_BLKP(bp);
  	}
  
  	return bp;
}
/* 
 * takes argument bp (block pointer), check its header for size, and
 * insert bp to the front of the linked list of the corresponding index 
 * in segregated_free_list
 */

static void* insert_node(void* bp)
{
	int bp_i = 0;
	size_t size = GET_SIZE(HEADER_P(bp));

	int i;
	for (i = 0; i < SEG_LIST_SIZE; i++) {
		if ((0x1<<i) >= size) {
			bp_i = i;
			break;
		}
	}
	
	//maybe check if bp is NULL?
	if (free_lists[bp_i] == NULL) {
		free_lists[bp_i] = bp;
		SET_PREV_NODE(free_lists[bp_i], NULL);
		SET_NEXT_NODE(free_lists[bp_i], NULL);
	}
	else {
		SET_NEXT_NODE(bp, free_lists[bp_i]);
		SET_PREV_NODE(free_lists[bp_i], bp);
		free_lists[bp_i] = bp;
	}
	
	return bp;
}

/*
 * takes argument bp, check its header for size, iterate through corresponding
 * free list to find 
 */

static void* delete_node(void* bp)
{
	if (bp == NULL) {
		return NULL;
	}
	else if (GET_PREV_NODE(bp) == NULL && GET_NEXT_NODE(bp) == NULL) {
		;
	}
	else if (GET_PREV_NODE(bp) == NULL && GET_NEXT_NODE(bp) != NULL) {
		SET_PREV_NODE(GET_NEXT_NODE(bp), NULL);
	}
	else if (GET_PREV_NODE(bp) != NULL && GET_NEXT_NODE(bp) == NULL) {
		SET_NEXT_NODE(GET_PREV_NODE(bp), NULL);
	}
	else { 
		SET_PREV_NODE(GET_NEXT_NODE(bp), GET_PREV_NODE(bp));
		SET_NEXT_NODE(GET_PREV_NODE(bp), GET_NEXT_NODE(bp));
	}

	return bp;
}

/*
int testCnt = 0;
int testFailed = 0;

void fail(int passed, int line) {
	testCnt++;
	if (!passed) {
		printf("test case on line %i failed\n", line);
		testFailed++;
	}
}

int main() {
	printf("hello. its me.");
}
*/

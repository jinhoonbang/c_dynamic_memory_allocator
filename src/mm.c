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
    "no jam",
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

#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define PACK(size, alloc) ((size) | (alloc))

#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

#define HEADER_P(bp) ((char *)(bp) - WSIZE)
#define FOOTER_P(bp) ((char *)(bp) + GET_SIZE(HEADER_P(bp)) - ALIGNMENT)

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - ALIGNMENT)))

#define SET_PREV_NODE(bp, prev_node) (*(unsigned int *)(bp + WSIZE) = (unsigned int)(prev_node))
#define SET_NEXT_NODE(bp, next_node) (*(unsigned int *)(bp) = (unsigned int)(next_node))

#define GET_PREV_NODE(bp) ((char *)(bp + WSIZE))
#define GET_NEXT_NODE(bp) ((char *)(bp))

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

static char *heap_start;
static char *free_list;
static char *cur_free;

static void* extend_heap(size_t words);
static void* coalesce(void* bp);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);
//static void insert_node(char* bp);
//static void delete_node(char* bp);
//static void print_free_lists();
//void print_free_list();
//static void print_pointer(void* ptr);




/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{	
	heap_start = NULL;
	free_list = NULL;

	void* bp;
	if ((heap_start = mem_sbrk(4*WSIZE)) == (void *)-1)
		return -1;
    	
	//change prev_alignment to 0 or 1?? 
	PUT(heap_start, 0);
	PUT(heap_start + (1*WSIZE), PACK(ALIGNMENT, 1));
	PUT(heap_start + (2*WSIZE), PACK(ALIGNMENT, 1));
	PUT(heap_start + (3*WSIZE), PACK(0, 1));
	heap_start += ALIGNMENT;
	

	cur_free = heap_start;
	free_list = heap_start;

	bp = extend_heap(CHUNKSIZE/WSIZE);
	if (bp  == NULL)
		return -1;
	
	//insert_node(bp); //NOT NECESSARY
	//should be casted?
	
	return 0;
}

/*
 * takes argument size and pad it to satisfy align req. 
 * find matching free space from segregated_free_list
 * add header and footer 
 * split free space if the required block is smaller by ALIGNMENT
 * extend heap if there is no matching free space in segregated_free_list
 * by ALIGNMENT
 * extend heap if there is no matching free space in segregated_free_list
 *
 */
void *mm_malloc(size_t size)

{
	size_t asize;
	size_t extendsize;
	//void *bp = NULL;
	char *bp;

	if (heap_start == 0)
		mm_init();

	if (size == 0)
		return NULL;

	if (size <= ALIGNMENT)
		asize = 2 * ALIGNMENT;
	else
		asize = ALIGNMENT * ((size + (ALIGNMENT) + (ALIGNMENT-1))/ALIGNMENT);

	//finds matching entry from free lists for asize
	
	if ((bp = find_fit(asize)) != NULL){
		place(bp, asize);
		return bp;
	}
	
	extendsize = MAX(asize, CHUNKSIZE);
	if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
		return NULL;

	//printf("%p", bp);
	place(bp, asize);	
	return bp;
}

/*
 * mm_free - Freeing a block does something.
 */
void mm_free(void *bp)
{
	if (bp == 0)
		return;

	if (heap_start == 0)
		mm_init();

	size_t size = GET_SIZE(HEADER_P(bp));

	PUT(HEADER_P(bp), PACK(size, 0));
	PUT(FOOTER_P(bp), PACK(size, 0));
	coalesce(bp);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 *
 *
 */
void *mm_realloc(void *bp, size_t size)
{
    void *new_bp;
	size_t asize;
    size_t bsize;
	unsigned int csize;
	unsigned int dsize;
	if (size <= ALIGNMENT)
		asize = 2 * ALIGNMENT;
	else
		asize = ALIGNMENT * ((size + (ALIGNMENT) + (ALIGNMENT-1))/ALIGNMENT);

    if (size == 0){
		mm_free(bp);
		return 0;
	} 

	if (bp == NULL){
		return mm_malloc(size);
	}

	csize = GET_SIZE(HEADER_P(bp));

	if (asize < csize){
		dsize = csize - asize;

		if (dsize < 2 * ALIGNMENT){
			return bp;
		}
		else{
			PUT(HEADER_P(bp), PACK(asize, 1));
			PUT(FOOTER_P(bp), PACK(asize, 1));
			void *next_bp = NEXT_BLKP(bp);
			PUT(HEADER_P(next_bp), PACK(dsize, 1));
			PUT(FOOTER_P(next_bp), PACK(dsize, 1));
			mm_free(next_bp);
			return bp;
		}
	}


    new_bp = mm_malloc(size);
    if (new_bp == NULL)
      return 0;

	bsize = GET_SIZE(HEADER_P(bp));
    if (size < bsize){
		bsize = size;
	}
    memcpy(new_bp, bp, bsize);
    mm_free(bp);

    return new_bp;
}

/*
 * argument is number of words, not number of bytes.  
 *
 */
static void *extend_heap(size_t words)
{	
	//void *bp;
  	char *bp;
  	size_t size;

  	size = (words % 2) ? (words + 1)*WSIZE : words * WSIZE;
  	if ((long)(bp = mem_sbrk(size)) == -1)
		return NULL;
  
  	PUT(HEADER_P(bp), PACK(size, 0));
  	PUT(FOOTER_P(bp), PACK(size, 0));
  	PUT(HEADER_P(NEXT_BLKP(bp)), PACK(0, 1));
	//printf("%p", bp);
	//insert_node(bp);
	return coalesce(bp);
}

static void *coalesce(void* bp) 
{
	size_t prev_alloc = GET_ALLOC(FOOTER_P(PREV_BLKP(bp)));
  	size_t next_alloc = GET_ALLOC(HEADER_P(NEXT_BLKP(bp)));
  	size_t size = GET_SIZE(HEADER_P(bp));
  
  	// prev allocated, next allocated
  	if (prev_alloc && next_alloc) {
		return bp;
  	}

  	// prev allocated, next free
  	if (prev_alloc && !next_alloc) {
  		size += GET_SIZE(HEADER_P(NEXT_BLKP(bp)));
  		PUT(HEADER_P(bp), PACK(size, 0));
  		PUT(FOOTER_P(bp), PACK(size, 0));
		//delete_node(GET_NEXT_NODE(bp));
  	}
  	// prev free, next allocated
  	else if (!prev_alloc && next_alloc) {
		size += GET_SIZE(HEADER_P(PREV_BLKP(bp)));
  		PUT(FOOTER_P(bp), PACK(size, 0));	
  		PUT(HEADER_P(PREV_BLKP(bp)), PACK(size, 0));
		//delete_node(bp);
		bp = PREV_BLKP(bp);
		//insert_node(bp);
  	}
  	// prev free, next free
  	else {
		size += GET_SIZE(HEADER_P(PREV_BLKP(bp))) + GET_SIZE(FOOTER_P(NEXT_BLKP(bp)));
  		PUT(HEADER_P(PREV_BLKP(bp)), PACK(size, 0));
  		PUT(FOOTER_P(NEXT_BLKP(bp)), PACK(size, 0));
  		//delete_node(bp);
		//delete_node(GET_NEXT_NODE(bp));
		bp = PREV_BLKP(bp);
		//insert_node(bp);
  	}
	
	if ((cur_free > (char *)bp) && (cur_free < NEXT_BLKP(bp))) {
		cur_free = bp;
	}

  	return bp;
}
/*
 * takes argument bp 
 * iterate through free_list
 * place bp so that free_list is increasing in address order
 *
static void insert_node(char* bp){	
	//printf("%p", bp);
	char* curr_node = free_list;
	char* prev_node;

	if (curr_node == NULL)
		return;

	while (curr_node != NULL && curr_node < bp){
		prev_node = curr_node;
		curr_node = GET_NEXT_NODE(curr_node);
	}

	//if curr_node is NULL, place bp at the end
	if (curr_node == NULL){
		SET_NEXT_NODE(prev_node, bp);
		SET_PREV_NODE(bp, prev_node);
		SET_NEXT_NODE(bp, NULL);
		return;
	}

	//curr_node is now bigger than bp
	SET_NEXT_NODE(bp, curr_node);
	SET_PREV_NODE(bp, GET_PREV_NODE(curr_node));
	SET_PREV_NODE(curr_node, bp);
	SET_NEXT_NODE(GET_PREV_NODE(curr_node), bp);

	//print_free_list();
	return;
}
*/

/*
 * take bp as argument
 * set prev node of bp to next node of bp and vice versa
 *
static void delete_node(char* bp){
	//printf("%p", bp);
	//SET_NEXT_NODE(bp, NULL);
	//SET_PREV_NODE(bp, NULL);
	if (bp == NULL)
		return;
	
	char* prev_node = GET_PREV_NODE(bp);
	char* next_node = (GET_NEXT_NODE(bp));

	SET_NEXT_NODE(prev_node, next_node);
	SET_PREV_NODE(next_node, prev_node);
	SET_NEXT_NODE(bp, NULL);
	SET_PREV_NODE(bp, NULL);

	//print_free_list();
	return;
}
*/

/* 
 * takes argument bp (block pointer), check its header for size, and
 * insert bp to the front of the linked list of the corresponding index 
 * in segregated_free_list
 
static void insert_node(void* bp)
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

	if (bp_i == 0)
		bp_i = SEG_LIST_SIZE - 1;
	
	if (bp == NULL)
		return;

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
	
	//print_free_lists();
	return;
	//return bp;
}
*/

/*
 * takes argument bp, check its header for size, iterate through corresponding
 * free list to find 
 
static void delete_node(void* bp)
{
	if (bp == NULL) {
		//return NULL;
		;
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

	//print_free_lists();
	return;
	//return bp;
}
*/


/*
 *
 */

/*
static void *find_fit(size_t asize){
	char* bp; 

	//for (bp = free_list; GET_SIZE(HEADER_P(bp)) > 0; bp = GET_NEXT_NODE(bp)){
	for (bp = heap_start; GET_SIZE(HEADER_P(bp)) > 0; bp = NEXT_BLKP(bp)){
		if (!GET_ALLOC(HEADER_P(bp)) && (asize <= GET_SIZE(HEADER_P(bp)))){
			return bp;
		}
	}
	return NULL;
}
*/

static void *find_fit(size_t asize){
	char *old_free = cur_free;
	unsigned int size;

	for (; GET_SIZE(HEADER_P(cur_free))>0 ; cur_free = NEXT_BLKP(cur_free)){
		size = GET_SIZE(HEADER_P(cur_free));

		if (!GET_ALLOC(HEADER_P(cur_free)) && (asize <= GET_SIZE(HEADER_P(cur_free)))) {
			return cur_free;
		}
	}

	for ( cur_free = heap_start; cur_free < old_free; cur_free = NEXT_BLKP(cur_free)) {
		if (!GET_ALLOC(HEADER_P(cur_free)) && (asize <= GET_SIZE(HEADER_P(cur_free)))) {
			return cur_free;
		}
	}

	return NULL;

}

/*
 * get free block with size greater than asize,
 * set it as allocated block
 * 
 */

static void place(void *bp, size_t asize){
	size_t csize = GET_SIZE(HEADER_P(bp));

	if ((csize - asize) >= (2*ALIGNMENT)){
		PUT(HEADER_P(bp), PACK(asize, 1));
		PUT(FOOTER_P(bp), PACK(asize, 1));
		//delete_node(bp);
		bp = NEXT_BLKP(bp);
		PUT(HEADER_P(bp), PACK(csize - asize, 0));
		PUT(FOOTER_P(bp), PACK(csize - asize, 0));
		//insert_node(bp);
	}
	else {
		PUT(HEADER_P(bp), PACK(csize, 1));
		PUT(FOOTER_P(bp), PACK(csize, 1));
		//delete_node(bp);
	}
}

void print_free_list() {
	char* currnode = free_list;
	while ( currnode != NULL) {
		printf("%p", currnode);
		currnode = GET_NEXT_NODE(currnode);
	}
}
void print_heap_start() {
	printf("%p", heap_start);
}
/*
static void print_pointer(void* ptr){
	printf("%p", (unsigned int *)ptr);
}

void print_free_lists(){
	int i;
	void* currnode;
	for (i = 0; i < SEG_LIST_SIZE; i++) {
		currnode = free_lists[i];
		printf("%p", currnode);
		while (GET_NEXT_NODE(currnode) != NULL) {
			currnode = GET_NEXT_NODE(currnode);
			printf("%p", currnode);
		}
	}
}
*/

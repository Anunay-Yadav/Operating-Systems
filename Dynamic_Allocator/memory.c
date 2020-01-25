#define _GNU_SOURCE

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include "memory.h"
#include <math.h>

#define PAGE_SIZE 4096
#define BUCKET_SIZE 9
#define BUCKET_START 4

// page metadata data container
typedef struct page_metadata{ 
	int allocation_size;
	long long free_bytes_available;
} page_metadata;

// node which points to each section
typedef struct node{
	struct node* left;
	struct node* right;
} node;

// array of linked list to hold the sections of each page
node* bucket[BUCKET_SIZE];

// alloc from ram : only takes size which are multiple of PAGESIZE
static void *alloc_from_ram(size_t size)
{
	assert((size % PAGE_SIZE) == 0 && "size must be multiples of 4096");
	void* base = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0);
	if (base == MAP_FAILED)
	{
		printf("Unable to allocate RAM space\n");
		exit(0);
	}
	return base;
}

// free
static void free_ram(void *addr, size_t size)
{
	munmap(addr, size);
}

// gets the address of page metadata by setting first 12 bits to 0.
// each page metadata address is a multiple of 4096
void* get_metadata_addr(void* ptr){
	uintptr_t k = (uintptr_t) ptr;
	k = k & ((uintptr_t)((~((1 << 12) - 1))));
	return (void*) k;
}

// pushes in front of linked list  in bucket[bucket_size]
void push(int bucket_size,node* temp){
	if(bucket[bucket_size] == NULL){
		bucket[bucket_size] = temp;
		temp -> left = NULL;
		temp -> right = NULL;
	}
	else{
		node* t = bucket[bucket_size];
		t -> left = temp;
		temp -> right = t;
		bucket[bucket_size] = temp;
	}	
}

// returns max of two integers
int max(int a,int b){
	if(a > b) return a;
	return b;
}

// returns min of two integers
int min(int a,int b){
	if(a < b) return a;
	return b;
}

// updates page_metadata after allocating that page to user
void update_dec(void* ptr){
	void* p = get_metadata_addr(ptr);
	page_metadata* l = (page_metadata*) p;
	l -> free_bytes_available = (int) (l -> free_bytes_available - l -> allocation_size);
	if(l -> free_bytes_available < 0) assert(0);
}

// returns integer k such that 2^k >= s and k is smallest.
int just_larger(size_t s){
	int k = max(log2((int) s),4);
	if((1 << k) == (int) s) return k;
	return k + 1;
}

// pops from the front of linked list in bucket[bucket_size].
void* pop(int bucket_size){
	if(bucket[bucket_size] == NULL) assert(0);
	node* ret = bucket[bucket_size];
	bucket[bucket_size] = ret -> right;
	update_dec(ret);
	return (void*) ret;
}

/*
	updates page metadata and removes all sections if free bytes == PAGESIZE else pushes in front of the linked list after freeing.
*/
void myfree(void* ptr){
	void* p = get_metadata_addr(ptr);
	page_metadata* l = (page_metadata*) p;
	if(l -> allocation_size > 4080){ // if allocation size is greater than 4080
		free_ram(p,l -> allocation_size);
		return;
	}
	l -> free_bytes_available = (l -> free_bytes_available + l -> allocation_size); // updating free bytes
	int pow = just_larger(l -> allocation_size); // get the appropriate bucket size
	if(l -> free_bytes_available == (PAGE_SIZE)){ // case where whole page is empty
		node* temp = bucket[pow-BUCKET_START];
		node* prev = temp;
		int count = 0;
		if(temp == NULL) assert(l -> allocation_size == 4080 || l -> allocation_size == 2048); // check for trivial cases
		while(temp != NULL){ // traversing whole linked list
			if(l == ((page_metadata*) get_metadata_addr((void*) temp)) ) { // if section of same page is found
				count++;
				if(prev != temp){
					prev -> right = temp -> right; // remove from between
					temp = temp -> right;
				}
				else{ 
					bucket[pow-BUCKET_START] = temp -> right; //remove from start
					prev = temp -> right;
					temp = temp -> right;
				}
				continue;
			}
			prev = temp;
			temp = temp -> right;
		}
		int cnt = (l -> allocation_size != 4080);
		assert(count == ((l -> free_bytes_available) / (l -> allocation_size)) - cnt - 1); // check if whole page is removed or not
		free_ram(p,4096);
		return;
	}
	push(pow - BUCKET_START,(node*) ptr);// push back to linked list if whole page size is not free
}

// adds page metadata container in front of free allocated memory 
void *add_page_meta(void* ptr,int allocation_size,long long free_bytes_available){
	page_metadata* p = (page_metadata *) ptr;
	p -> allocation_size = allocation_size;
	p -> free_bytes_available = free_bytes_available;
	return (ptr + 16);
}

// allocates memory just greater than size
// checks whether bucket is initially empty if it is it requests for page size and adds it to the linked list 
// else pops the first element from the linked list and allocates that memory to user.
void *mymalloc(size_t size)
{
	if(size > PAGE_SIZE - 16){ // case where size > bucket size 
		int cnt = (size%PAGE_SIZE > 4080) ? PAGE_SIZE : 0;
		void* ptr = add_page_meta(alloc_from_ram(size + (PAGE_SIZE - (size%PAGE_SIZE)) + cnt),size + (PAGE_SIZE - (size%PAGE_SIZE)) + cnt,(PAGE_SIZE - (size%PAGE_SIZE)) + cnt - 16);
		return ptr;
	}
	int pow = (just_larger(size)); // to get the bucket size
	if(bucket[pow-BUCKET_START] == NULL){
		void* ptr;
		int k = (1 << pow);
		if(pow != 12) 
			ptr = add_page_meta(alloc_from_ram(PAGE_SIZE),1 << pow,PAGE_SIZE); // case bucket size < 4080
		else
			ptr = add_page_meta(alloc_from_ram(PAGE_SIZE),(1 << pow) - 16,(PAGE_SIZE)) ; // case bucket size = 4080
		int step = (1 << pow);
		int count = 0;
		if(pow != 12){ // case bucket size < 4080
			ptr += step - 16;
			count += step;
			while(count + step <= PAGE_SIZE){
				push(pow-BUCKET_START,(node*) ptr); // push that ptr to linked list
				ptr += step; // shift ptr by step
				count += step;
			}
		}
		else{ // case bucket size == 4080
			step -= 16;
			while(count + step <= PAGE_SIZE - 16 ){
				push(pow-BUCKET_START,(node*) ptr); // push that ptr to linked list
				ptr += step; // shift ptr by step
				count += step;
			}	
		}
	}
	return pop(pow-BUCKET_START);
}
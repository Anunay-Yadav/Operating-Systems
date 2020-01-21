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

typedef struct page_metadata{
	int allocation_size;
	long long free_bytes_available;
} page_metadata;

typedef struct node{
	struct node* left;
	struct node* right;
} node;

node* bucket[BUCKET_SIZE];

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

static void free_ram(void *addr, size_t size)
{
	munmap(addr, size);
}

void* get_metadata_addr(void* ptr){
	uintptr_t k = (uintptr_t) ptr;
	k = k & ((uintptr_t)((~((1 << 12) - 1))));
	return (void*) k;
}

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
		bucket[bucket_size] = t;
	}
}

void update_dec(void* ptr){
	void* p = get_metadata_addr(ptr);
	page_metadata* l = (page_metadata*) p;
	l -> free_bytes_available -= l -> allocation_size;
}

int max(int a,int b){
	if(a > b) return a;
	return b;
}

int just_larger(size_t s){
	int k = max(log2((int) s),4);
	if((1 << k) == (int) s) return k;
	return k + 1;
}

void* pop(int bucket_size){
	if(bucket[bucket_size] == NULL) return NULL;
	node* ret = bucket[bucket_size];
	bucket[bucket_size] = ret -> right;
	update_dec(ret);
	return (void*) ret;
}

void update_add(void* ptr){
	void* p = get_metadata_addr(ptr);
	page_metadata* l = (page_metadata*) p;
	if(l -> allocation_size > 4096){
		free_ram(p,l -> allocation_size);
		return;
	}
	l -> free_bytes_available += l -> allocation_size;
	if(l -> free_bytes_available >= 4080){
		int pow = just_larger(l -> allocation_size);
		int step = (1 << pow);
		int count = 0;
		while(count < PAGE_SIZE - 16){
			pop(pow-4);
			count += step;
		}
		free_ram(p,4096);
		return;
	}
	push(just_larger(l -> allocation_size),(node*) ptr);
}

void myfree(void *ptr)
{
	update_add(ptr);
}

void *add_page_meta(void* ptr,int allocation_size,long long free_bytes_available){
	page_metadata* p = (page_metadata *) ptr;
	p -> allocation_size = allocation_size;
	p -> free_bytes_available = free_bytes_available;
	return (ptr + 16);
}

void *mymalloc(size_t size)
{
	if(size >= PAGE_SIZE - 16){
		int cnt = (size%PAGE_SIZE > 4080) ? PAGE_SIZE : 0;
		void* ptr = add_page_meta(alloc_from_ram(size + (PAGE_SIZE - (size%PAGE_SIZE)) + cnt),size + (PAGE_SIZE - (size%PAGE_SIZE)) + cnt - 16,(PAGE_SIZE - (size%PAGE_SIZE)) + cnt - 16);
		return ptr;
	}
	else{
		int pow = (just_larger(size));
		if(bucket[pow-4] == NULL){
			void* ptr = add_page_meta(alloc_from_ram(PAGE_SIZE),1 << pow,PAGE_SIZE - 16);
			int step = (1 << pow);
			int count = 0;
			while(count < PAGE_SIZE - 16){
				push(pow-4,(node*) ptr);
				ptr += step;
				count += step;
			}
		}
		return pop(pow-4);
	}
}
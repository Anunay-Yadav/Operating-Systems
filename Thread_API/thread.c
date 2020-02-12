//apt-get install gcc-multilib

#include "thread.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#define SIZE 4096

// thread metadata
struct thread {
	void *esp;
	void *start;
	struct thread *next;
	struct thread *prev;
};

struct lock_data{
	struct lock* lock;
	struct lock_data* next;
};

struct thread *ready_list = NULL;     // ready list
struct thread *cur_thread = NULL;     // current thread
struct lock_data * lock_list = NULL;
struct thread* dead_list = NULL;

// defined in context.s
void context_switch(struct thread *prev, struct thread *next);

// insert the input thread to the end of the ready list.
static void push_back(struct thread *t)
{
	struct thread *temp = ready_list;
	if(temp == NULL){
		ready_list = t;
		t -> prev = ready_list;
		t -> next = ready_list;
		return;
	}
	struct thread* last = temp -> prev;
	last -> next = t;
	t -> prev = last;
	t -> next = ready_list;
	ready_list -> prev = t;
}
static void push_unique(struct lock *t)
{
	struct lock_data *temp = lock_list;
	if(temp == NULL){
		lock_list = (struct lock_data*) malloc(sizeof(struct lock_data));
		lock_list -> lock = t;
		lock_list -> next = NULL;
		return;
	}
	while(temp -> next != NULL){
		if(temp -> lock == t) {
			return;
		}
		temp = temp -> next;
	}	
	struct lock_data* p =(struct lock_data*) malloc(sizeof(struct lock_data));
	p -> lock = t;
	temp -> next = p;
}

// remove the first thread from the ready list and return to caller.
static struct thread *pop_front()
{
	if(ready_list == NULL)
		return NULL;
	struct thread *ret = ready_list;
	if(ready_list -> next == ready_list)
	{
		ready_list = NULL;
		return ret;
	}
	struct thread *temp = ready_list -> prev;
	temp -> next = ready_list -> next;
	ready_list -> next -> prev = temp;
	ready_list = ready_list -> next;
	return ret;
}
static struct thread *pop_front_dead()
{
	if(dead_list == NULL)
		return NULL;
	struct thread *ret = dead_list;
	if(dead_list -> next == dead_list)
	{
		dead_list = NULL;
		return ret;
	}
	struct thread *temp = dead_list -> prev;
	temp -> next = dead_list -> next;
	dead_list -> next -> prev = temp;
	dead_list = dead_list -> next;
	return ret;
}

// the next thread to schedule is the first thread in the ready list.
// obtain the next thread from the ready list and call context_switch.
static void schedule()
{
	if(ready_list == NULL) return;
	struct thread* prev = cur_thread;
	struct thread* next = pop_front();
	cur_thread = next;
	context_switch(prev,next);
}

// push the cur_thread to the end of the ready list and call schedule
// if cur_thread is null, allocate struct thread for cur_thread
static void schedule1()
{

	if(cur_thread == NULL){
		cur_thread = (struct thread *) malloc(sizeof(struct thread));
	}
	push_back(cur_thread);
	schedule();
}	

// allocate stack and struct thread for new thread
// save the callee-saved registers and parameters on the stack
// set the return address to the target thread
// save the stack pointer in struct thread
// push the current thread to the end of the ready list
void create_thread(func_t func, void *param)
{
	struct thread* new_thread = (struct thread*) malloc(sizeof(struct thread));
	unsigned *stack = (unsigned*) malloc(SIZE);
	new_thread -> start = stack;
	stack += 1024;
	if(param != NULL){
		stack--;
		*stack = ((unsigned) param);
	}
	stack--;
	*stack = 0;
	stack--;
	*stack = ((unsigned) func);
	stack -= 4;
	new_thread -> esp = stack;
	push_back(new_thread);
}

// call schedule1
void thread_yield()
{
	schedule1();
}

void push_dead(struct thread* t){
	struct thread *temp = dead_list;
	if(temp == NULL){
		dead_list = t;
		t -> prev = dead_list;
		t -> next = dead_list;
		return;
	}
	struct thread* last = temp -> prev;
	last -> next = t;
	t -> prev = last;
	t -> next = dead_list;
	dead_list -> prev = t;
}

// call schedule
void thread_exit()
{
	struct thread* dead = pop_front_dead();
	while(dead != NULL){
		free(dead -> start);
		free(dead);
		dead = pop_front_dead();
	}
	push_dead(cur_thread);
	schedule();
}

// call schedule1 until ready_list is null
int check(struct lock_data* locks){
	if(locks == NULL) return 0;
	struct lock_data* temp = locks;
	while(temp != NULL){
		if(temp -> lock -> wait_list != NULL) return 1;
		temp = temp -> next;
	}
	return 0;
}
void wait_for_all()
{
	while(ready_list != NULL || check(lock_list)) schedule1();
}

void sleep(struct lock *lock)
{
	struct thread* cur = (struct thread*) lock -> wait_list;
	push_unique(lock);
	struct thread *temp = cur;
	if(temp == NULL){
		lock -> wait_list = cur_thread;
		cur_thread -> prev = (struct thread*) lock -> wait_list;
		cur_thread -> next = (struct thread*) lock -> wait_list;
		return;
	}
	struct thread* last = temp -> prev;
	last -> next = cur_thread;
	cur_thread -> prev = last;
	cur_thread -> next = (struct thread*) lock -> wait_list;
	((struct thread*) lock -> wait_list )-> prev = cur_thread;
	schedule();
}

void wakeup(struct lock *lock)
{
	struct thread* cur = (struct thread*) lock -> wait_list;
	if(cur != NULL){
		struct thread* temp = (struct thread*) lock -> wait_list;
		struct thread* ret = temp;
		if(temp != temp -> next){
			temp -> prev -> next = temp -> next;
			temp -> next -> prev = temp -> prev;
			lock -> wait_list = temp -> next;
		}
		else
			lock -> wait_list = NULL;
		push_back(ret);
	}
}

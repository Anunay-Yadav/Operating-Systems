//apt-get install gcc-multilib

#include "thread.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#define SIZE 4096

// thread metadata
struct thread {
	void *esp;
	struct thread *next;
	struct thread *prev;
};

struct thread *ready_list = NULL;     // ready list
struct thread *cur_thread = NULL;     // current thread

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
	while(temp -> next != ready_list)
		temp = temp -> next;
	temp -> next = t;
	t -> prev = temp;
	t -> next = ready_list;
	ready_list -> prev = t;
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

// call schedule
void thread_exit()
{
	schedule();
}

// call schedule1 until ready_list is null
void wait_for_all()
{
	while(ready_list != NULL) schedule1();
}

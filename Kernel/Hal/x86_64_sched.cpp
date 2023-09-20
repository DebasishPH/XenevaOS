/**
* BSD 2-Clause License
*
* Copyright (c) 2022-2023, Manas Kamal Choudhury
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice, this
*    list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
**/

#include <Hal/x86_64_sched.h>
#include <Hal/x86_64_hal.h>
#include <Hal/Hal.h>
#include <Hal/x86_64_cpu.h>
#include <Hal/x86_64_lowlevel.h>
#include <Sync/spinlock.h>
#include <Hal/serial.h>
#include <Mm/kmalloc.h>
#include <Mm/vmmngr.h>
#include <Hal/pcpu.h>
#include <Mm/pmmngr.h>
#include <string.h>
#include <_null.h>
#include <aucon.h>

AuThread* thread_list_head;
AuThread* thread_list_last;
AuThread* blocked_thr_head;
AuThread* blocked_thr_last;
AuThread* trash_thr_head;
AuThread* trash_thr_last;

bool _x86_64_sched_enable;
static uint16_t thread_id;
AuThread* _idle_thr;
Spinlock *_idle_lock;
bool _x86_64_sched_init;

extern "C" int save_context(AuThread *t, void *tss);
extern "C" void execute_idle(AuThread* t, void* tss);
uint64_t AuMapKStack(uint64_t *cr3);

void AuThreadInsert(AuThread* new_task) {
	new_task->next = NULL;
	new_task->prev = NULL;

	if (thread_list_head == NULL) {
		thread_list_last = new_task;
		thread_list_head = new_task;
	}
	else {
		thread_list_last->next = new_task;
		new_task->prev = thread_list_last;
	}
	thread_list_last = new_task;
}

/**
* AuThreadDelete -- remove a thread from thread list
* @param thread -- thread address to remove
*/
void AuThreadDelete(AuThread* thread) {

	if (thread_list_head == NULL)
		return;

	if (thread == thread_list_head) {
		thread_list_head = thread_list_head->next;
	}
	else {
		thread->prev->next = thread->next;
	}

	if (thread == thread_list_last) {
		thread_list_last = thread->prev;
	}
	else {
		thread->next->prev = thread->prev;
	}

	/* donot free the thread, cuz when thread needs
	* to move from runnable queue to blocked queue
	* same address is used, rather call 'free'
	* externally
	*/
}


/**
* Insert a thread to thread list
* @param new_task -- new thread address
*/
void AuThreadInsertBlock(AuThread* new_task) {
	new_task->next = NULL;
	new_task->prev = NULL;

	if (blocked_thr_head == NULL) {
		blocked_thr_last = new_task;
		blocked_thr_head = new_task;
	}
	else {
		blocked_thr_last->next = new_task;
		new_task->prev = blocked_thr_last;
	}
	blocked_thr_last = new_task;
}

/**
* AuThreadDeleteBlock -- remove a thread from thread list
* @param thread -- thread address to remove
*/
void AuThreadDeleteBlock(AuThread* thread) {

	if (blocked_thr_head == NULL)
		return;

	if (thread == blocked_thr_head) {
		blocked_thr_head = blocked_thr_head->next;
	}
	else {
		thread->prev->next = thread->next;
	}

	if (thread == blocked_thr_last) {
		blocked_thr_last = thread->prev;
	}
	else {
		thread->next->prev = thread->prev;
	}
}


/**
* Insert a thread to trash list
* @param new_task -- new thread address
*/
void AuThreadInsertTrash(AuThread* new_task) {
	new_task->next = NULL;
	new_task->prev = NULL;

	if (trash_thr_head == NULL) {
		trash_thr_last = new_task;
		trash_thr_head = new_task;
	}
	else {
		trash_thr_last->next = new_task;
		new_task->prev = trash_thr_last;
	}
	trash_thr_last = new_task;
}

/**
* AuThreadDeleteTrash -- remove a thread from thread list
* @param thread -- thread address to remove
*/
void AuThreadDeleteTrash(AuThread* thread) {

	if (trash_thr_head == NULL)
		return;

	if (thread == trash_thr_head) {
		trash_thr_head = trash_thr_head->next;
	}
	else {
		thread->prev->next = thread->next;
	}

	if (thread == trash_thr_last) {
		trash_thr_last = thread->prev;
	}
	else {
		thread->next->prev = thread->prev;
	}
}


/**
* ! Creates a kernel mode thread
*  @param entry -- Entry point address
*  @param stack -- Stack address
*  @param cr3 -- the top most page map level address
*  @param name -- name of the thread
*  @param priority -- (currently unused) thread's priority
**/
AuThread* AuCreateKthread(void(*entry) (uint64_t), uint64_t stack, uint64_t cr3, char *name)
{
	AuThread *t = (AuThread*)kmalloc(sizeof(AuThread));
	memset(t, 0, sizeof(AuThread));
	t->frame.r15 = 0;
	t->frame.r14 = 0;
	t->frame.r13 = 0;
	t->frame.r12 = 0;
	t->frame.r11 = 0;
	t->frame.r10 = 0;
	t->frame.r9 = 0;
	t->frame.r8 = 0;
	t->frame.rbp = stack;
	t->frame.rdi = 0;
	t->frame.rsi = 0;
	t->frame.rdx = 0;
	t->frame.rcx = 0;
	t->frame.rbx = 0;
	t->frame.rax = 0;
	t->frame.rip = (uint64_t)entry;
	t->frame.cs = 0x08;
	t->frame.rflags = 0x202;
	t->frame.rsp = stack;
	t->frame.ss = 0x10;
	t->frame.kern_esp = stack;
	t->user_stack = stack;
	t->quanta = 0;
	t->frame.cr3 = cr3;
	t->priviledge = THREAD_LEVEL_KERNEL;
	t->state = THREAD_STATE_READY;
	t->frame.ds = t->frame.es = t->frame.fs = t->frame.gs = SEGVAL(GDT_ENTRY_KERNEL_DATA, 0);
	t->frame.user_ = 0;

	t->syscall_param.param1 = t->syscall_param.param2 = 0;
	t->syscall_param.param3 = t->syscall_param.param4 = 0;
	t->syscall_param.param5 = t->syscall_param.param6 = 0;

	/* process will fill uentry */
	t->uentry = NULL; 
	memset(t->name, 0,16);
	strcpy(t->name, name);
	t->id = thread_id++;

	t->fx_state = (uint8_t*)kmalloc(512);

	/* align the fx_state memory to 16-byte boundary */
	size_t fx_addr = (size_t)t->fx_state;
	if ((size_t)t->fx_state & (16 - 1)) 
		t->fx_state = (uint8_t*)(fx_addr | 16 - 1) + 1;
	
	memset(t->fx_state, 0, 512);

	t->mxcsr = 0x1f80;
	AuThreadInsert(t);
	return t;
}


/**
*  Creates a user mode thread
*  @param entry -- Entry point address
*  @param stack -- Stack address
*  @param cr3 -- the top most page map level address
*  @param name -- name of the thread
*  @param priority -- (currently unused) thread's priority
*/
AuThread* AuCreateUthread(void(*entry) (void*), uint64_t stack, uint64_t cr3, char *name)
{
	AuThread *t = (AuThread*)kmalloc(sizeof(AuThread));
	memset(t, 0, sizeof(AuThread));
	t->frame.r15 = 0;
	t->frame.r14 = 0;
	t->frame.r13 = 0;
	t->frame.r12 = 0;
	t->frame.r11 = 0;
	t->frame.r10 = 0;
	t->frame.r9 = 0;
	t->frame.r8 = 0;
	t->frame.rbp = stack;
	t->frame.rdi = 0;
	t->frame.rsi = 0;
	t->frame.rdx = 0;
	t->frame.rcx = 0;
	t->frame.rbx = 0;
	t->frame.rax = 0;
	t->frame.rip = (uint64_t)entry;
	t->frame.cs = SEGVAL(GDT_ENTRY_USER_CODE, 3);
	t->frame.rflags = 0x202;
	t->frame.rsp = stack;
	t->frame.ss = SEGVAL(GDT_ENTRY_USER_DATA, 3);
	t->frame.ds = t->frame.es = t->frame.fs = t->frame.gs = SEGVAL(GDT_ENTRY_USER_DATA, 3);
	uint64_t k_esp = AuMapKStack((uint64_t*)cr3);
	/** Kernel stack is important for syscall or interruption in the system **/
	t->frame.kern_esp = k_esp;
	AuTextOut("FRAME USER KERN ESP -> %x \n", t->frame.kern_esp);
	t->user_stack = stack;
	t->frame.cr3 = V2P(cr3);
	t->frame.user_ = 1;
	memset(t->name, 0, 8);
	strncpy(t->name, name, 8);
	t->id = thread_id++;
	t->quanta = 0;

	t->fx_state = (uint8_t*)kmalloc(512);
	memset(t->fx_state, 0, 512);
	t->mxcsr = 0x1f80;
	t->priviledge = THREAD_LEVEL_USER;
	t->state = THREAD_STATE_READY;
	AuThreadInsert(t);
	return 0;
}

/*
 * AuKThreadCopy -- copies the context of dest
 * thread to source thread
 * @param dest -- destination thread
 * @param src -- source thread
 */
void AuKThreadCopy(AuThread* dest, AuThread* src) {
	dest->frame.r15 = src->frame.r15;
	dest->frame.r14 = src->frame.r14;
	dest->frame.r13 = src->frame.r13;
	dest->frame.r12 = src->frame.r12;
	dest->frame.r11 = src->frame.r11;
	dest->frame.r10 = src->frame.r10;
	dest->frame.r9 = src->frame.r9;
	dest->frame.r8 = src->frame.r8;
	dest->frame.rdi = src->frame.rdi;
	dest->frame.rsi = src->frame.rsi;
	dest->frame.rdx = src->frame.rdx;
	dest->frame.rcx = src->frame.rcx;
	dest->frame.rbx = src->frame.rbx;
	dest->frame.rax = src->frame.rax;
	dest->frame.rip = src->frame.rip;
	dest->frame.cs = src->frame.cs;
	dest->frame.rflags = src->frame.rflags;
	dest->frame.ss = src->frame.ss;
	dest->frame.ds = src->frame.ds;
	dest->frame.es = src->frame.es;
	dest->frame.fs = src->frame.fs;
	dest->frame.gs = src->frame.gs;
}


void AuIdleThread(uint64_t t) {
	SeTextOut("_idle id -> %d  \r\n", AuPerCPUGetCpuID());
	while (1) {
	}
}


/*
 * AuSchedulerInitialise -- initialise the core scheduler
 */
void AuSchedulerInitialise() {
	thread_id = 0;
	thread_list_head = NULL;
	thread_list_last = NULL;
	blocked_thr_head = NULL;
	blocked_thr_last = NULL;
	trash_thr_head = NULL;
	trash_thr_last = NULL;
	_x86_64_sched_enable = true;
	_x86_64_sched_init = false;
	_idle_lock = AuCreateSpinlock(false);
	AuThread *idle_ = AuCreateKthread(AuIdleThread, (uint64_t)P2V((uint64_t)AuPmmngrAlloc() + 4096), x64_read_cr3(), "Idle");
	_idle_thr = idle_;
	AuPerCPUSetCurrentThread(idle_);
}

/* 
 * AuSchedulerInitAp -- pin the idle thread to per cpu 
 * current thread pointer 
 */
void AuSchedulerInitAp() {
	AuPerCPUSetCurrentThread(_idle_thr);
}


/*
 * AuNextThread -- get the next thread to run
 */
void AuNextThread() {
	AuThread* thread = AuPerCPUGetCurrentThread();
	do {
		if (thread->state == THREAD_STATE_SLEEP) {
			if (thread->quanta == 0) {
				thread->state = THREAD_STATE_READY;
			}
			thread->quanta--;
		}
		thread = thread->next;
		if (thread == NULL) {
			thread = thread_list_head;
		}
	} while (thread->state != THREAD_STATE_READY);

end:
	//current_thread = thread;
	AuPerCPUSetCurrentThread(thread);
}

extern "C" uint64_t x64_get_rsp();

/*
 * x8664SchedulerISR -- scheduler core
 */
void x8664SchedulerISR(size_t v, void* param) {
	x64_cli();
	interrupt_stack_frame *frame = (interrupt_stack_frame*)param;

	if (_x86_64_sched_enable == false)
		goto sched_end;

	TSS *ktss = AuPerCPUGetKernelTSS();
	AuThread* current_thread = AuPerCPUGetCurrentThread();
	if (save_context(current_thread, ktss) == 0) {
		current_thread->frame.cr3 = x64_read_cr3();

		current_thread->frame.kern_esp = x64_get_kstack(ktss);

		if (x86_64_is_cpu_fxsave_supported())
			x64_fxsave(current_thread->fx_state);

		
		AuNextThread();
		current_thread = AuPerCPUGetCurrentThread();
		
		AuInterruptEnd(0);
		
		if (x86_64_is_cpu_fxsave_supported())
			x64_fxrstor(current_thread->fx_state);

		x64_set_kstack(ktss, current_thread->frame.kern_esp);

		x64_ldmxcsr(&current_thread->mxcsr);

		execute_idle(current_thread, ktss);
	}

sched_end:
	AuInterruptEnd(0);
}


/*
 * AuSchedulerStart -- start the scheduler service
 */
void AuSchedulerStart() {
	_x86_64_sched_init = true;
    setvect(0x40,x8664SchedulerISR);
	AuThread* current_thread = AuPerCPUGetCurrentThread();
	execute_idle(current_thread, x86_64_get_tss());
}

/*
 * Allocate kernel stack
 * @param cr3 -- root page map level, it should be 
 * converted to linear virtual address
 */
uint64_t AuMapKStack(uint64_t *cr3) {
	uint64_t location = KERNEL_STACK_LOCATION;

	for (int i = 0; i < 8192 / 4096; i++) {
		void* p = AuPmmngrAlloc();
		AuMapPageEx(cr3, (uint64_t)p, location + i * 4096, 0);
	}

	return (location + 8192);
}

extern "C" void AuPrintStack() {
	SeTextOut("CR3 -> %x \r\n", x64_read_cr3());
}

/*
 * AuGetCurrentThread -- gets the running thread
 * from per_cpu_data, currently done only non-multi
 * processing
 */
AuThread* AuGetCurrentThread() {
	return AuPerCPUGetCurrentThread();
}


/*
 * AuBlockThread -- blocks a thread and insert it to
 * block list
 */
AU_EXTERN AU_EXPORT void AuBlockThread(AuThread *thread) {
	thread->state = THREAD_STATE_BLOCKED;
	AuThreadDelete(thread);
	AuThreadInsertBlock(thread);
}

/*
* AuSleepThread -- sleeps a thread 
*/
AU_EXTERN AU_EXPORT void AuSleepThread(AuThread *thread, uint64_t ms) {
	thread->state = THREAD_STATE_SLEEP;
	thread->quanta = ms;
}


/*
 * AuUnblockThread -- unblocks a thread and insert it to
 * ready list
 * @param t -- pointer to thread
 */
AU_EXTERN AU_EXPORT void AuUnblockThread(AuThread *t) {
	t->state = THREAD_STATE_READY;
	bool found_ = false;
	for (AuThread *thr = blocked_thr_head; thr != NULL; thr = thr->next) {
		if (thr == t) {
			AuThreadDeleteBlock(thr);
			found_ = true;
			break;
		}
	}
	if (found_)
		AuThreadInsert(t);
}

/* 
 * AuThreadMoveToTrash -- move given thread to
 * trash
 * @param t -- Thread to move to trash
 */
void AuThreadMoveToTrash(AuThread* t) {
	t->state = THREAD_STATE_KILLABLE;

	/* search the thread in ready queue*/
	for (AuThread* ready_queue_ = thread_list_head; ready_queue_ != NULL; ready_queue_ = ready_queue_->next) {
		if (ready_queue_ == t) 
			AuThreadDelete(t);
	}

	/* search the thread in block queue*/
	for (AuThread* block_queue_ = blocked_thr_head; block_queue_ != NULL; block_queue_ = block_queue_->next) {
		if (block_queue_ == t)
			AuThreadDeleteBlock(t);
	}

	/* insert it in the trash list */
	AuThreadInsertTrash(t);
}

/*
 * AuThreadCleanTrash -- removes a particular thread
 * from trash list
 * @param t -- > thread to remove
 */
void AuThreadCleanTrash(AuThread* t) {
	AuThreadDeleteTrash(t);
}

/*
 * AuThreadFindByID -- finds a thread by its id from
 * ready queue
 * @param id -- id of the thread
 */
AuThread* AuThreadFindByID(uint16_t id) {
	for (AuThread* ready_queue_ = thread_list_head; ready_queue_ != NULL; ready_queue_ = ready_queue_->next) {
		if (ready_queue_->id == id)
			return ready_queue_;
	}
}

/*
 * AuForceScheduler -- force the scheduler
 * to switch next thread
 */
AU_EXTERN AU_EXPORT void AuForceScheduler() {
	x64_force_sched();
}


bool AuIsSchedulerInitialised() {
	return _x86_64_sched_init;
}

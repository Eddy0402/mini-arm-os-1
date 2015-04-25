#include "config.h"
#include "os.h"
#include <stddef.h>
#include <stdint.h>
#include "reg.h"
#include "asm.h"


/* USART TXE Flag
 * This flag is cleared when data is written to USARTx_DR and
 * set when that data is transferred to the TDR
 */
#define USART_FLAG_TXE	((uint16_t) 0x0080)

void systick_init(void)
{
	/* SysTick configuration */
	*SYSTICK_LOAD = 7200000;
	*SYSTICK_VAL = 1;
	*SYSTICK_CTRL = 0x07;
}

void usart_init(void)
{
	*(RCC_APB2ENR) |= (uint32_t)(0x00000001 | 0x00000004);
	*(RCC_APB1ENR) |= (uint32_t)(0x00020000);

	/* USART2 Configuration, Rx->PA3, Tx->PA2 */
	*(GPIOA_CRL) = 0x00004B00;
	*(GPIOA_CRH) = 0x44444444;
	*(GPIOA_ODR) = 0x00000000;
	*(GPIOA_BSRR) = 0x00000000;
	*(GPIOA_BRR) = 0x00000000;

	*(USART2_CR1) = 0x0000000C;
	*(USART2_CR2) = 0x00000000;
	*(USART2_CR3) = 0x00000000;
	*(USART2_CR1) |= 0x2000;
}

void print_str(const char *str)
{
	while (*str) {
		while (!(*(USART2_SR) & USART_FLAG_TXE));
		*(USART2_DR) = (*str & 0xFF);
		str++;
	}
}

/* Exception return behavior */
#define HANDLER_MSP	0xFFFFFFF1
#define THREAD_MSP	0xFFFFFFF9
#define THREAD_PSP	0xFFFFFFFD

struct list_head{
	struct list_head *next, *previous;
};

#define list_entry(ptr, type, member) \
	((type *)((char *)(ptr) - (unsigned long)(&((type *)0)->member)))

#define list_next(ptr, type, member) \
	(list_entry((ptr)->member.next, type, member))

#define list_previous(ptr, type, member) \
	(list_entry((ptr)->member.previous, type, member))

#define list_add(list, ptr, type, member) \
	do{ \
		if((list) == NULL){ \
			list = ptr;\
			(ptr)->member.next = &(ptr)->member; \
			(ptr)->member.previous = &(ptr)->member; \
		}else{ \
			(ptr)->member.next = &(list)->task_list; \
			(ptr)->member.previous = list->task_list.previous; \
			(list)->member.previous->next = &(ptr)->task_list; \
			(list)->member.next = &(ptr)->task_list; \
		} \
	}while(0)

struct tcb{
	volatile unsigned int *stacktop;
	volatile unsigned int stack[STACK_SIZE];
	int remain_tick;
	int reload_tick;
	struct list_head task_list;
};

static struct tcb tasks[TASK_LIMIT];

struct tcb *ready_list[MAX_PRIORITY] = {};

volatile struct tcb *current_task = NULL;
volatile struct tcb *next_task = NULL;

/* XXX: temporary design, manage pid by os, instead of user program */
static unsigned int task_count = 0;

int create_task(void (*start)(void), int priority, int reload_tick)
{
	if (task_count == TASK_LIMIT) return ENOMEM;
	int this_taskid = task_count++;

	tasks[this_taskid].stacktop = tasks[this_taskid].stack;

	/* End of stack, minus what we are about to push */
	tasks[this_taskid].stacktop += STACK_SIZE - 16;
	/* Task function */
	tasks[this_taskid].stacktop[14] = (unsigned int) start;
	/* PSR Thumb bit */
	tasks[this_taskid].stacktop[15] = (unsigned int) 0x01000000;

	tasks[this_taskid].remain_tick = reload_tick;
	tasks[this_taskid].reload_tick = reload_tick;
	list_add(ready_list[priority],
			 &tasks[this_taskid],
			   struct tcb,
			   task_list);

	return this_taskid;
}

void start_schedular(void)
{
	print_str("\nOS: Start round-robin scheduler!\n");

	for(int priority = MAX_PRIORITY - 1; priority >= 0;--priority){
		if(ready_list[priority] != NULL){
			current_task = ready_list[priority];
			break;
		}
	}

	if(current_task == NULL){
		print_str("No task to be run!");
		while(1);
	}

	/* enable interrupt and SVC for first task */
	__asm__("cpsie i");
	__asm__("svc 0");
}

volatile unsigned *switch_context()
{
	return (current_task = next_task)->stacktop;
}

void systick_handler(void)
{
	if(--current_task->remain_tick == 0){
		current_task->remain_tick = current_task->reload_tick; // reload tick
		next_task = list_next(current_task, struct tcb, task_list);
		if(next_task != current_task){
			/* Send a PendSV */
			*( ( volatile unsigned long *) 0xe000ed04 ) = 0x10000000;
		}
	}
}


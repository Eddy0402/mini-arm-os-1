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

static unsigned int user_stacks[TASK_LIMIT][STACK_SIZE];
unsigned int *user_stacktop[TASK_LIMIT];
unsigned int current_task = 0;

/* XXX: temporary design, manage pid by os, instead of user program*/
static unsigned int task_count = 0;

/* Initilize user task stack and execute it one time */
/* XXX: Implementation of task creation is a little bit tricky. In fact,
 * after the second time we called `activate()` which is returning from
 * exception. But the first time we called `activate()` which is not returning
 * from exception. Thus, we have to set different `lr` value.
 * First time, we should set function address to `lr` directly. And after the
 * second time, we should set `THREAD_PSP` to `lr` so that exception return
 * works correctly.
 * http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0552a/Babefdjc.html
 */
int create_task(void (*start)(void))
{
	if (task_count == TASK_LIMIT) return ENOMEM;
	int this_taskid = task_count++;

	unsigned int *stack = user_stacks[this_taskid];

	stack += STACK_SIZE - 16; /* End of stack, minus what we are about to push */
	stack[14] = (unsigned int) start;
	stack[15] = (unsigned int) 0x01000000; /* PSR Thumb bit */
	user_stacktop[this_taskid] = stack;

	return this_taskid;
}

void start_schedular(void)
{
	print_str("\nOS: Start round-robin scheduler!\n");

	/* enable interrupt */
	__asm__("cpsie i");
	__asm__("svc 0");
}

unsigned *switch_context()
{
	/* Select next task */
	current_task = (current_task == task_count - 1) ? 0 : current_task + 1;
	return user_stacktop[current_task];
}

void systick_handler(void)
{
	/* Send a PendSV */
	*( ( volatile unsigned long *) 0xe000ed04 ) = 0x10000000;
}


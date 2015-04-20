#include "config.h"
#include "os.h"

void delay(volatile int count)
{
	count *= 50000;
	while (count--);
}

void task1_func(void)
{
	print_str("task1: Created!\n");
	print_str("task1: Now, return to kernel mode\n");
	while (1) {
		print_str("task1: Running...\n");
		delay(1000);
	}
}

void task2_func(void)
{
	print_str("task2: Created!\n");
	print_str("task2: Now, return to kernel mode\n");
	while (1) {
		print_str("task2: Running...\n");
		delay(1000);
	}
}

int main(void)
{
	systick_init();
	usart_init();

	int task1 = create_task(&task1_func);
	int task2 = create_task(&task2_func);

	(void)task1; // currently not used
	(void)task2; // currently not used

	start_schedular();

	while (1) {
		/* Will not reach here since start_schedular never return. */
	}
}


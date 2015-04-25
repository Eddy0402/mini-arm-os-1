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
	while (1) {
		print_str("task1: Running...\n");
		delay(1000);
	}
}

void task2_func(void)
{
	print_str("task2: Created!\n");
	while (1) {
		print_str("task2: Running...\n");
		delay(1000);
	}
}

void task3_func(void)
{
	print_str("task3: Created!\n");
	while (1) {
		print_str("task3: Running...\n");
		delay(1000);
	}
}

int main(void)
{
	systick_init();
	usart_init();

	int task1 = create_task(&task1_func, 1, 10);
	int task2 = create_task(&task2_func, 2, 1);
	int task3 = create_task(&task3_func, 2, 10);

	(void)task1; // currently not used
	(void)task2; // currently not used
	(void)task3; // currently not used

	start_schedular();

	while (1) {
		/* Will not reach here since start_schedular never return. */
	}
}


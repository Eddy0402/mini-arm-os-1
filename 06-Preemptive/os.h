#ifndef __OS_H__
#define __OS_H__

#define ENOMEM -1

/* TODO: move to portable layer */
void systick_init();
void usart_init();

int create_task(void (*start)(void));
void start_schedular();

void print_str(const char *str);

#endif


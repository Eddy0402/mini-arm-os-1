.syntax unified

.align 2
.STACKTOP:
.word user_stacktop
.CURRTASK:
.word current_task

.text

.global pendsv_handler
.type pendsv_handler, %function
pendsv_handler: /* Do context switch */
	mrs r0, psp
	stmdb r0!, {r4-r11}

	/* Get the stack top of the current task and store the new one */
	ldr r3, .STACKTOP
	ldr r2, .CURRTASK
	ldr r2, [r2]
	str r0, [r3, r2, lsl #2]

	/* switch the context in critical section */
	push {lr}
	mov r1, #0
	msr basepri, r1
	bl switch_context
	mov r1, #0
	msr basepri, r1
	pop {lr}

	/* switch_context returns new context's stack top */
	ldmia r0!, {r4-r11}
	msr psp, r0

	/* Return to Thread mode with PSP */
	bx lr
.size	pendsv_handler, .-pendsv_handler

.global svc_handler
.type svc_handler, %function
svc_handler: /* Start first task */
	/* save kernel state */
	mrs ip, psr
	push {r4-r11, ip, lr}

	/* Get the stack top of the first task */
	ldr r3, .STACKTOP
	ldr r2, .CURRTASK
	ldr r2, [r2]
	ldr r0, [r3, r2]
	
	/* Pop registers not automatically saved */
	ldmia r0!, {r4-r11}

	/* switch to process stack */
	msr psp, r0

	/* Use PSP, unprivileged mode*/
	mov r0, #3
	msr control, r0
	isb

	/* Return to Thread mode with PSP */
	orr lr, #0xd
	bx lr
.size svc_handler, .-svc_handler



#include <stdint.h>
#include<stdio.h>
#include "main.h"
extern void initialise_monitor_handles(void);

void task1_handler(void);
void task2_handler(void);
void task3_handler(void);
void task4_handler(void);

void init_systick_timer(uint32_t tick_hz);
__attribute__((naked)) void init_scheduler_stack(uint32_t sched_top_of_stack);
void init_tasks_stack(void);
__attribute__((naked)) void switch_sp_to_psp(void);

uint32_t psp_of_tasks[MAX_TASKS]={T1_STACK_START,T2_STACK_START,T3_STACK_START,T4_STACK_START};
uint32_t task_handlers[MAX_TASKS];
uint8_t current_task=0;
//1. DEFINE MACROS
//2. SYSTICK OF 1MS, processor clock= systick timer count clock
int main(void)
{
	initialise_monitor_handles();
	//0. enable processor faults there are none in cortex m0
	//1. init MSP for sched stack
	init_scheduler_stack(SCHED_STACK_START);
	//2. capture addresses of different task handlers
	task_handlers[0]= (uint32_t)task1_handler;
	task_handlers[1]= (uint32_t)task2_handler;
	task_handlers[2]= (uint32_t)task3_handler;
	task_handlers[3]= (uint32_t)task4_handler;
	//3. store dummy frames
	init_tasks_stack();
	//4. generate exceptions of systick
	init_systick_timer(TICK_HZ);

	//5. change stack pointer to PSP till here SP was MSP
	switch_sp_to_psp();


	task1_handler();
	for(;;);
}

void task1_handler(void){

	while(1){
		printf("This is task 1\n");
	}
}
void task2_handler(void){

	while(1){
		printf("This is task 2\n");
	}
}
void task3_handler(void){

	while(1){
		printf("This is task 3\n");
	}
}
void task4_handler(void){

	while(1){
		printf("This is task 4\n");
	}
}
void init_systick_timer(uint32_t tick_hz){
	uint32_t *pSRVR = (uint32_t *) 0xE000E014;
	uint32_t *pSCSR = (uint32_t *) 0xE000E010;
	uint32_t *pSCVR = (uint32_t *)0xE000E018;
	*pSCVR = 0;  // Writing any value clears it to 0
	uint32_t count_value=(SYSTICK_TIM_CLK/tick_hz)-1;
	//clear value of RVR
	*pSRVR &= ~(0x00FFFFFF);
	//load the value in RVR
	*pSRVR |= count_value;
	//do some settings, configure CSR
	//TICKINT(1) CLK SOURCE(2) ENABLE(0)
	*pSCSR |= (1<<1); //enable Systick exception request
	*pSCSR |= (1<<2); //indicates clock source, processor clock source

	//enable systcik
	*pSCSR |= (1<<0);

}
//SP is special register so use inline assembly and naked fucntion to skip epilogue prilogue
__attribute__((naked)) void init_scheduler_stack(uint32_t sched_top_of_stack){
	//__asm volatile("MSR MSP,R0");
	//1. init sp as MSP
	__asm volatile("MSR MSP,%0": : "r" (sched_top_of_stack) : );
	__asm volatile("BX LR");//bx copies value of LR into PC
}
uint32_t get_psp_value(void){
	return psp_of_tasks[current_task];
}

void save_psp_value(uint32_t current_psp_value){
	psp_of_tasks[current_task]=current_psp_value;
}

void update_next_task(void){
	current_task++;
	current_task %= MAX_TASKS;//ROUND ROBIN FASION
}
__attribute__((naked)) void switch_sp_to_psp(void){
	//1. intialise PSP with TASK1 stack start address
	__asm volatile ("MOV R1, LR");	//preserve LR which conencts back to main()
	//get the value of psp of current task
	__asm volatile ("BL get_psp_value");//LR value will get curupted
	__asm volatile ("MSR PSP,R0");//init PSP
	__asm volatile ("MOV LR, R1");	//preserve LR which conencts back to main()

	//2. change SP to PSP using CONTROL register
	__asm volatile ("MOV R0,#0X02");
	__asm volatile ("MSR CONTROL,R0");

	__asm volatile ("BX LR");

	//NOTE: In Cortex-M0, the POP instruction only allows popping from registers R0-R7. but push is allowed from page 71 in cortex m0 user guide
}

__attribute__((naked)) void SysTick_Handler(void)
{
	/*Save the context of current task*/

	//1. Get current running tasks PSP value
	__asm volatile("MRS R0,PSP");
	//2. Using that PSP value store SF2(R4 to R11)
	//push instr cant be used here as in hanlder mode msp is used
	//store register value to memory
	//STMDB- decrement and then store
	//__asm volatile("STMDB R0!,{R4-R11}");// ! IS AN OPTIONAL WRITEBACK SUFFIX, IF ! IS PRESENT THE FINAL ADDRESS, THAT IS LOADED FROM OR STORED TO IS WRITTEN BACK INTO Rn
	//2. Using that PSP value store SF2(R4 to R11)
	//Cortex-M0 doesn't support STMDB, so we'll use individual store instructions with manual SP adjustment
	__asm volatile(
			"SUB R0, R0, #32\n"    // Decrement PSP by 32 bytes (for R4-R11)
			"STR R4, [R0, #0]\n"   // Store R4
			"STR R5, [R0, #4]\n"   // Store R5
			"STR R6, [R0, #8]\n"   // Store R6
			"STR R7, [R0, #12]\n"  // Store R7
			"MOV R1, R8\n"         // Move R8 to R1 temporarily (Cortex-M0 can only STR low registers directly)
			"STR R1, [R0, #16]\n"  // Store R8 (via R1)
			"MOV R1, R9\n"         // Move R9 to R1
			"STR R1, [R0, #20]\n"  // Store R9 (via R1)
			"MOV R1, R10\n"        // Move R10 to R1
			"STR R1, [R0, #24]\n"  // Store R10 (via R1)
			"MOV R1, R11\n"        // Move R11 to R1
			"STR R1, [R0, #28]\n"  // Store R11 (via R1)
	);

    __asm volatile("MOV R12, LR");          // Save LR to R7 (which is callee-saved)	//3. Save the current value of PSP
	__asm volatile("BL save_psp_value");//accroding to AAPCS R0 holds current PSP tasks PSP value

	/*Retrieve the context of next task*/

	//1. Decide next task to run
	__asm volatile("BL update_next_task");
	//2. Get its past PSP value
	__asm volatile("BL get_psp_value");
	//3. USing that PSP value retrieve SF2(R4 to R11), MEMORY TO REGSITER
	//__asm volatile("LDMIA R0!,{R4-R11}");
	//Cortex-M0 doesn't support LDMIA, so we'll use individual load instructions
	__asm volatile(
			"LDR R4, [R0, #0]\n"   // Load R4
	        "LDR R5, [R0, #4]\n"   // Load R5
	        "LDR R6, [R0, #8]\n"   // Load R6
	        "LDR R7, [R0, #12]\n"  // Load R7
	        "LDR R1, [R0, #16]\n"  // Load saved R8 value to R1
	        "MOV R8, R1\n"         // Move from R1 to R8
	        "LDR R1, [R0, #20]\n"  // Load saved R9 value to R1
	        "MOV R9, R1\n"         // Move from R1 to R9
	        "LDR R1, [R0, #24]\n"  // Load saved R10 value to R1
	        "MOV R10, R1\n"        // Move from R1 to R10
	        "LDR R1, [R0, #28]\n"  // Load saved R11 value to R1
	        "MOV R11, R1\n"        // Move from R1 to R11
	        "ADD R0, R0, #32\n"    // Increment PSP by 32 bytes
	);

	//4. Update PSP and exit
	__asm volatile("MSR PSP,R0");
	//NOW PSP POINTS TO NEXT SF1 frame of next updated task, therefore execution will go to next updated task
    __asm volatile("MOV LR, R12");
    __asm volatile("BX LR");
}
void init_tasks_stack(void){
	uint32_t *pPSP;
	for(int i=0;i<MAX_TASKS;i++){
		pPSP=(uint32_t *) psp_of_tasks[i];
		//FD stack i.e decrement first and then store
		pPSP--;
		*pPSP=DUMMY_XPSR;//0x01000000

		pPSP--; //PC
		*pPSP=task_handlers[i];

		pPSP--; //LR
		*pPSP=0xFFFFFFFD;//thread mode and use PSP

		for(int j=0;j<13;j++){
			pPSP--;
			*pPSP=0;
		}
		psp_of_tasks[i]=(uint32_t)pPSP;//preserve value of PSP
	}

}



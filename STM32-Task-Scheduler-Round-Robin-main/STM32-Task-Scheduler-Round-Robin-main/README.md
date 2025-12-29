# STM32 Task Scheduler - Round Robin

A custom-built round-robin task scheduler for the STM32F072RBT6, implemented completely from scratch by referencing the Cortex‑M0 user guide. Demonstrating manual context switching and real-time task management using the SysTick handler.

![image](https://github.com/user-attachments/assets/964c4424-69ae-461c-ae25-d04be23ab91e)
![image](https://github.com/user-attachments/assets/668cde6e-0e9e-4cf8-949a-ac5dd31a38f5)
---

## Technical Details

**SysTick Handler Usage:**  
At the core of this scheduler is the SysTick handler, which triggers every 1 ms to perform context switching. The handler saves the context of the current task by manually storing registers (R4–R11) on the task's Process Stack Pointer (PSP). It then selects the next task in a cyclic order, restores its context, and updates the PSP accordingly. Due to Cortex‑M0’s limitations (e.g., absence of multi-register load/store for high registers), all context save and restore operations are implemented using inline assembly with precise control over the processor registers. This approach ensures that each task resumes execution exactly where it left off.

**Key Techniques Implemented:**

- **Manual Context Switching:**  
  Using inline assembly to save and restore critical registers (R4–R11) without relying on hardware-supported multi-register instructions. This method provides full control over task execution and stack management.

- **Custom Task Stack Initialization:**  
  Each task stack is initialized with a dummy frame containing essential values like xPSR, PC, and LR. This setup ensures that, upon context restoration, the task resumes in thread mode with the correct execution context.

- **Round Robin Scheduling:**  
  The scheduler cycles through tasks using a simple modulo operation, ensuring each task gets a fair share of CPU time. This deterministic switching mechanism is ideal for systems requiring predictable and balanced task execution.

- **Debug and Trace via Semihosting:**  
  Debug messages printed to the console provide real-time feedback on task execution and context switching, with placeholders available for screenshots of console output to demonstrate system behavior during operation.


## **Execution Flow of the Round Robin Scheduler**

This table describes how the scheduler initializes and performs task switching using **SysTick_Handler**, explaining why we need a **dummy stack** and **manual register saving/loading** due to Cortex-M0’s limitations.

---

### **Execution Flow from `main()`**
| **Step** | **Function** | **Purpose** |
|----------|-------------|-------------|
| 1 | `initialise_monitor_handles()` | Enables semi-hosting for debugging (optional). |
| 2 | **(No processor faults to enable in Cortex-M0)** | Cortex-M0 does not have configurable fault handlers. |
| 3 | `init_scheduler_stack(SCHED_STACK_START)` | Sets up **Main Stack Pointer (MSP)** for the scheduler. |
| 4 | `task_handlers[0] = (uint32_t)task1_handler;`<br>`task_handlers[1] = (uint32_t)task2_handler;`<br>`task_handlers[2] = (uint32_t)task3_handler;`<br>`task_handlers[3] = (uint32_t)task4_handler;` | Stores the function addresses of tasks in an array. |
| 5 | `init_tasks_stack()` | **Creates a dummy stack for each task:**<br> - Simulates an exception return frame.<br> - Ensures tasks start execution as if they were interrupted. |
| 6 | `init_systick_timer(TICK_HZ)` | Sets up **SysTick Timer** to generate periodic interrupts for task switching. |
| 7 | `switch_sp_to_psp()` | **Switches the Stack Pointer (SP) from MSP to PSP** before starting the first task. |
| 8 | `task1_handler()` | Begins executing Task 1. |
| 9 | `for(;;);` | Keeps the program running indefinitely. |

---

### **Task 1 → Task 2 Switch (Triggered by SysTick)**
| **Step** | **Function** | **Purpose** |
|----------|-------------|-------------|
| 1 | `SysTick_Handler()` | **SysTick Timer interrupt triggers a context switch.** |
| 2 | `MRS R0, PSP` | **Retrieve current PSP (Task 1’s stack pointer)** before switching tasks. |
| 3 | **Store R4–R11 manually** | - Cortex-M0 lacks **STMDB (Store Multiple Decrement Before)**, so we use **individual `STR` instructions**.<br> - **Why?** These registers are **callee-saved** (must be preserved).<br> - **How?** Using PSP, we **manually push** R4–R11 to stack. |
| 4 | `MOV R12, LR` | Save **LR (EXC_RETURN value)** since it is needed for returning. |
| 5 | `save_psp_value()` | **Stores the updated PSP** (after saving R4–R11) for Task 1. |
| 6 | `update_next_task()` | **Selects the next task (Task 2) to run** based on Round Robin scheduling. |
| 7 | `get_psp_value()` | Retrieves **Task 2’s PSP** (previous stack pointer). |
| 8 | **Restore R4–R11 manually** | - Cortex-M0 lacks **LDMIA (Load Multiple Increment After)**, so we use **individual `LDR` instructions**.<br> - **Why?** These registers were saved before switching.<br> - **How?** Using PSP, we **manually pop** R4–R11. |
| 9 | `MSR PSP, R0` | **Update PSP to point to Task 2’s stack**. |
| 10 | `MOV LR, R12` | Restore **LR** (EXC_RETURN value) before exiting handler. |
| 11 | `BX LR` | **Exit SysTick_Handler**, resuming Task 2. |

---

### **Why Are These Steps Necessary?**
1. **Why do we need a dummy stack?**  
   - Each task starts execution **as if it was interrupted**.  
   - The stack frame must contain **R0–R3, R12, LR, PC, xPSR** so the CPU can "resume" execution.  
   - Otherwise, task execution will crash due to an invalid stack frame.  

2. **Why do we manually store/restore R4–R11?**  
   - Cortex-M0 lacks **STMDB/LDMIA**, forcing us to use **manual `STR` and `LDR` instructions**.  
   - We **use R1 as an intermediate register** because Cortex-M0 **can only store/load R0–R7 directly**.  

3. **Why do we save LR in R12?**  
   - The **EXC_RETURN value** in LR determines if we return to **Thread Mode using PSP** or MSP.  
   - If lost, the system **won’t know how to return to the task properly**.  

---

This cycle repeats indefinitely, ensuring fair CPU time for each task using **Round Robin scheduling**.

This project’s execution flow and design leverage a combination of standard C functions and naked functions (which omit the automatic prologue/epilogue) to achieve low-level context switching. The numbering below follows the order in which functions are executed and interact:

1. **main()**  
   *Type: Standard C function*  
   **Role:**  
   - Entry point of the program.
   - Initializes debugging support via `initialise_monitor_handles()`.
   - Sets up the scheduler’s main stack by calling **init_scheduler_stack()**.
   - Captures task handler addresses for each task.
   - Initializes each task’s stack using **init_tasks_stack()**.
   - Configures the SysTick timer with **init_systick_timer()** for a 1ms tick.
   - Switches the CPU’s active stack pointer from MSP to PSP using **switch_sp_to_psp()**.
   - Launches the first task by calling one of the task handlers (e.g., **task1_handler()**).

2. **initialise_monitor_handles()**  
   *Type: External function (provided by the debug environment)*  
   **Role:**  
   - Sets up semihosting, allowing console output for debugging purposes.

3. **init_scheduler_stack(uint32_t sched_top_of_stack)**  
   *Type: Naked function*  
   **Role:**  
   - Directly initializes the Main Stack Pointer (MSP) to a predefined scheduler stack location without compiler-added register saving.

4. **init_tasks_stack(void)**  
   *Type: Standard C function*  
   **Role:**  
   - Iterates over all tasks to initialize their individual stacks.
   - Creates a dummy stack frame for each task that includes:
     - The initial xPSR value.
     - The task’s PC (set to the task handler’s address).
     - The LR value (set to an EXC_RETURN constant for thread mode using PSP).
   - Ensures proper stack alignment for reliable execution.

5. **init_systick_timer(uint32_t tick_hz)**  
   *Type: Standard C function*  
   **Role:**  
   - Configures the SysTick timer registers (RVR, CSR, and CVR) to generate periodic interrupts at a rate defined by `tick_hz` (1ms in this case).
   - Enables the SysTick interrupt and selects the processor clock as the source.

6. **switch_sp_to_psp(void)**  
   *Type: Naked function*  
   **Role:**  
   - Switches the active stack pointer from MSP (used during initialization) to PSP (used by tasks).
   - Retrieves the current task’s PSP value, sets the PSP register, and updates the CONTROL register accordingly—all while preserving LR manually.

7. **Task Handlers (task1_handler, task2_handler, task3_handler, task4_handler)**  
   *Type: Standard C functions*  
   **Role:**  
   - Represent individual tasks that execute continuously (e.g., printing a message).
   - Provide a simple workload to demonstrate that context switching occurs as intended.

8. **SysTick_Handler(void)**  
   *Type: Naked function*  
   **Role:**  
   - Serves as the interrupt service routine for the SysTick timer.
   - **Context Saving:**  
     - Reads the current PSP.
     - Manually decrements the PSP and stores registers R4 through R11 (using inline assembly) to save the context of the running task.
   - **Task Switching:**  
     - Calls helper functions to save the current task’s PSP, update the task index (round-robin), and retrieve the next task’s PSP.
   - **Context Restoring:**  
     - Loads registers R4–R11 from the new task’s stack.
     - Adjusts the PSP back and updates the PSP register.
   - Returns control to the new task by restoring the saved LR.

9. **save_psp_value(uint32_t current_psp_value)**  
   *Type: Standard C function*  
   **Role:**  
   - Saves the current task’s Process Stack Pointer (PSP) into an array that maintains the context for all tasks.

10. **update_next_task(void)**  
    *Type: Standard C function*  
    **Role:**  
    - Updates the index of the current task using a modulo operation, ensuring a cyclic (round-robin) scheduling mechanism.

11. **get_psp_value(void)**  
    *Type: Standard C function*  
    **Role:**  
    - Retrieves the stored PSP value for the currently active task from the task context array.

---

## Usage

1. **Development Environment:**  
   Open the project in STM32CubeIDE. The project is fully configured for the STM32F072RBT6.

2. **Building the Project:**  
   Simply build the project using STM32CubeIDE’s build functionality. The makefile and project configuration are set up to compile the scheduler along with its context switching routines.

3. **Flashing and Debugging:**  
   Connect your STM32 programmer/debugger, then flash the firmware onto the microcontroller. The semihosting output can be viewed in the debug console, showing task switching in action.

4. **Console Output:**  

![image](https://github.com/user-attachments/assets/964c4424-69ae-461c-ae25-d04be23ab91e)
![image](https://github.com/user-attachments/assets/668cde6e-0e9e-4cf8-949a-ac5dd31a38f5)


   The console output demonstrates the cyclic execution of tasks, with clear, periodic messages from each task handler.

---

## References

- **Cortex‑M0 Technical Reference Manual**
- **STM32F072RBT6 Datasheet**
- **STM32CubeIDE Documentation**

---

### **Understanding AAPCS and How I Used It in This Project (Cortex-M0 Specific)**  

#### **What is AAPCS?**  
The **ARM Architecture Procedure Call Standard (AAPCS)** defines how functions pass arguments, return values, and manage registers in ARM systems. It ensures consistent function calls across assembly and C code. The key rules are:  

1. **Function Arguments & Return Values:**  
   - The first **four arguments** are passed in **R0–R3**.  
   - Any additional arguments are pushed onto the stack.  
   - The function’s return value is placed in **R0**.  

2. **Caller-Saved & Callee-Saved Registers:**  
   - **R0–R3 and R12** are **caller-saved**, meaning they may be modified by a function call.  
   - **R4–R11** are **callee-saved**, meaning they must be preserved by a function if used.  

3. **Stack & Exception Handling:**  
   - The **stack pointer (SP) must be 8-byte aligned** before calling a function.  
   - On exception entry (e.g., SysTick interrupt), the CPU **automatically** pushes:  
     - **R0–R3** (used for function arguments)  
     - **R12** (temporary register, caller-saved)  
     - **LR (Link Register)** (return address)  
     - **PC (Program Counter)** (execution address at time of interrupt)  
     - **xPSR (Program Status Register)** (status flags, interrupt status, etc.)  
   - This ensures that when the exception exits, execution resumes correctly.  

---

### **How I Took Advantage of AAPCS in This Project**  

1. **Using R0 to Pass PSP for Context Switching**  
   - Since AAPCS requires the first function argument to be in **R0**, I passed the Process Stack Pointer (PSP) this way in functions like:  
     - `get_psp_value()` → Returns the PSP of the current task  
     - `save_psp_value()` → Stores PSP before switching tasks  
     - `update_next_task()` → Selects the next task’s PSP  
   - This avoids stack-based argument passing, reducing overhead.  

2. **Manual Register Saving & Restoring in SysTick_Handler**  
   - Cortex-M0 **does not support** `STMDB` (Store Multiple Decrement Before) and `LDMIA` (Load Multiple Increment After), which are normally used for fast register saves.  
   - Instead, I manually store **R4–R11** to the stack using `STR` (store) and restore them using `LDR` (load).  
   - I then subtract **32 bytes from PSP** (8 registers × 4 bytes) before saving and add **32 bytes** back when restoring.  
   - This ensures all necessary registers are preserved for task switching.  

3. **Saving & Restoring LR Using R12**  
   - **R12 is caller-saved**, meaning it isn’t preserved across function calls by AAPCS.  
   - I took advantage of this by **saving LR into R12** before switching tasks.  
   - When restoring context, took **move R12 back into LR**, ensuring the function resumes correctly.  

4. **Handling xPSR, PC, and LR During Task Switching**  
   - Since the **CPU automatically saves xPSR, PC, and LR** on exception entry, I don’t manually store them.  
   - When restoring context, I ensure the stack is correctly positioned so that the **EXC_RETURN instruction restores xPSR, PC, and LR** correctly.  
   - This guarantees that the new task resumes execution at the right instruction with the correct status flags.  

---

### **Why This Approach?**  
- **Follows AAPCS:** Ensures compatibility with function calling conventions.  
- **Minimizes Overhead:** Uses `R0` for function arguments instead of stack-based passing.  
- **Works Around Cortex-M0 Limitations:** Avoids `STMDB/LDMIA` by manually handling register saves.  
- **Ensures Correct Task Resumption:** By properly handling **xPSR, PC, and LR**, I make sure tasks resume without corruption.  

By following AAPCS conventions while overcoming Cortex-M0’s limitations, I achieved an **efficient round-robin scheduler** that switches tasks seamlessly.

## Challenges Faced

Developing this scheduler required a deep understanding of the ARM Architecture Procedure Call Standard (AAPCS), which specifies that function arguments are passed in registers R0–R3—this design leverages R0 to receive the PSP value as an input argument in functions like `get_psp_value()`, ensuring seamless data flow without additional memory operations.

Cortex-M0 lacks STMDB/LDMIA, forcing me to manually save/restore R4–R11 using individual STR/LDR instructions. Compounding this, STR/LDR can only access R0–R7, so I had to use R1 as an intermediate register to save/restore R8–R11. Additionally, since AAPCS automatically saves R0–R3, R12, LR, PC, and xPSR, I leveraged R12 to store LR, ensuring proper task resumption.

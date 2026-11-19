# ARM Cortex-M4 Architecture & Processor Core

## 1. Core Overview
The ARM Cortex-M4 is a high-performance 32-bit RISC processor core designed for deeply embedded microcontroller applications. Key architectural features include:
- **Harvard Architecture**: Separate instruction (I-Code) and data (D-Code) buses enabling simultaneous instruction fetch and memory access.
- **Thumb-2 Instruction Set**: Combines high code density of 16-bit instructions with performance of 32-bit instructions.
- **3-Stage Pipeline**: Fetch, Decode, and Execute stages with branch speculation.
- **Integrated FPU & DSP Extensions**: Hardware single-precision floating point unit (IEEE 754) and single-cycle MAC (Multiply-Accumulate) instructions.
- **Nested Vectored Interrupt Controller (NVIC)**: Hardware-managed interrupt handling with low latency (12 clock cycles typical).

---

## 2. Register Bank Model

The Cortex-M4 provides 16 general-purpose and special core registers:

| Register | Name | Role & Functionality |
|:---:|:---:|:---|
| **R0–R12** | General Purpose Registers | Data operations and address computations. In AAPCS calling convention, R0–R3 pass arguments and return values; R4–R11 are callee-saved; R12 is intra-procedure-call scratch. |
| **R13 (SP)** | Stack Pointer | Banked as **MSP** (Main Stack Pointer) and **PSP** (Process Stack Pointer). Selected via `CONTROL` register bit 1. |
| **R14 (LR)** | Link Register | Holds the return address for subroutines and exception return codes (`EXC_RETURN`). |
| **R15 (PC)** | Program Counter | Points to current instruction address $+ 4$ (due to pipeline offset). Bit 0 is always 1 indicating Thumb execution. |
| **xPSR** | Program Status Register | Composite view of **APSR** (flags N, Z, C, V, Q), **IPSR** (current active exception number), and **EPSR** (Thumb execution state bit T). |

---

## 3. Dual Stack Pointers (MSP vs PSP)
- **MSP (Main Stack Pointer)**: Default stack pointer used upon reset and for all exception/interrupt handlers.
- **PSP (Process Stack Pointer)**: Optional secondary stack used by application tasks in RTOS environments. Isolating thread stacks prevents application stack overflows from corrupting OS kernel handlers.

---

## 4. Interrupt Architecture & Exception Handling
- **Vector Table**: Located at `0x08000000` (or relocated via `SCB->VTOR`). Vector 0 holds Initial MSP value; Vector 1 holds Reset Handler address.
- **Automatic Hardware Stacking**: On exception entry, the processor automatically pushes 8 registers onto the current stack: `R0, R1, R2, R3, R12, LR, PC, xPSR`.
- **Tail-Chaining**: If a new interrupt is pending upon completion of an ISR, the processor skips unstacking and re-stacking, achieving a fast 6-cycle transition.

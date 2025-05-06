# 🧠 Microkernel Operating System with Intel 8080 Emulator

This project simulates a microkernel-based operating system using a custom-built Intel 8080 CPU emulator. It demonstrates core OS concepts including multitasking, virtual memory, context switching, and system calls — all using a mix of C++ and 8080 Assembly.

---

## 📚 Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Architecture](#architecture)
- [Project Structure](#project-structure)
- [Getting Started](#getting-started)
- [How It Works](#how-it-works)
- [Sample Programs](#sample-programs)
- [Educational Value](#educational-value)

---

## 📝 Overview

This is an educational operating system built around an Intel 8080 emulator. The system supports multitasking and runs user programs written in Intel 8080 assembly. It features:

- An emulator for Intel 8080 (written in C++).
- A simple microkernel to handle scheduling and memory.
- A virtual memory system with paging and page replacement.
- A system call interface to support user interaction.

---

## ✅ Features

- 🧱 Microkernel architecture
- 🖥️ Intel 8080 CPU simulation
- 🔁 Round-robin process scheduling
- 📦 Virtual memory with paging (1 KB pages)
- 🔄 FIFO page replacement algorithm
- ⏱️ Timer-based context switching
- 📞 System calls: I/O, memory access, process creation/termination

---

## 🧩 Architecture

```text
+-------------------------+
|  Intel 8080 Emulator    | ← C++ Core CPU emulator
+-------------------------+
|     Microkernel ASM     | ← Initializes memory, sets up processes
+-------------------------+
|     System Call Layer   | ← Implemented in C++ (gtuos.cpp)
+-------------------------+
|     Memory Management   | ← Virtual → Physical, paging, swap
+-------------------------+
|    User Programs (.com) | ← Written in Intel 8080 Assembly
+-------------------------+
```
## 📂 Project Structure

```text
.
├── 8080emu.cpp / 8080emuCPP.h    # Intel 8080 emulator
├── memory.cpp / memory.h         # Physical and virtual memory manager
├── gtuos.cpp / gtuos.h           # System call handler
├── MicroKernel.asm               # Microkernel assembly code
├── *.asm                         # User-level programs (Sum.asm, Primes.asm)
├── *.com                         # Compiled 8080 binaries
```
## 🚀 Getting Started

### 1. Clone the Repository

```bash
git clone https://github.com/onurpolattimur/Microkernel-Operating-System-with-i8080.git
cd Microkernel-Operating-System-with-i8080
```
### 2. Build the Emulator

If no Makefile exists, compile manually using the following command:

```bash
g++ -std=c++11 -o GTUOS 8080emu.cpp memory.cpp gtuos.cpp main.cpp
```
### 3. Assemble Sample Programs

Use an online Intel 8080 assembler like:

🔗 http://www.sensi.org/~svo/i8080/

To assemble:

- Open a `.asm` file (e.g., `Sum.asm`) in a text editor.
- Paste its content into the assembler.
- Assemble the program and download the resulting `.com` binary.
- Place the `.com` file in the same directory as the emulator.

---

### 4. Run the Emulator

Use the following command to run your assembled program:

```bash
./GTUOS Sum.com 0
```
---

## 🔧 How It Works

- The emulator loads and executes Intel 8080 `.com` programs.
- User programs make system calls using the `TRAP` instruction.
- The OS handles each system call via `gtuos.cpp` (e.g., `PRINT_B`, `READ_STR`, `EXEC`, etc.).
- A simulated timer interrupt triggers context switching.
- A round-robin scheduler rotates execution between active processes.
- The memory manager handles:
  - Virtual-to-physical address mapping,
  - Page table maintenance,
  - Page faults and FIFO page replacement.

---

## 🧪 Sample Programs

- 📄 `Sum.asm`: Adds a sequence of numbers and displays the result.
- 📄 `Primes.asm`: Finds and prints all prime numbers up to a user-defined number.
- 📄 `Collatz.asm`: Computes and displays the Collatz sequence for a given number.

Each program:

- Uses `TRAP` instructions to access OS services.
- Runs in its own virtual memory space.
- Demonstrates real-world operating system features like:
  - Process isolation,
  - System call interfaces,
  - Cooperative multitasking.

---

## 🎓 Educational Value

This project is ideal for:

- 🧠 Learning how CPU emulation works at the instruction cycle level.
- 🛠️ Understanding system calls, software interrupts, and traps.
- 🧰 Exploring concepts of virtual memory and paging.
- 🔁 Simulating multitasking, process scheduling, and kernel/user separation.

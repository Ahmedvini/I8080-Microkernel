# Intel 8080 Microkernel

A sophisticated microkernel implementation for the Intel 8080 processor, featuring comprehensive multi-tasking support, advanced memory management, and a rich system call interface. This project demonstrates core operating system concepts through a practical implementation.

## Core Features

### Process Management
- Round-robin scheduling with configurable quantum (default: 5 timer ticks)
- Process states: READY, RUNNING, BLOCKED, TERMINATED
- Full process context switching with register preservation
- Process creation with isolated memory spaces
- Process termination with resource cleanup
- Process table with 16 process slots
- Priority-based scheduling support
- Inter-process communication primitives

### Memory Management
- Paged memory management with 1KB page size
- Virtual memory support with demand paging
- 4-level page table hierarchy
- Page replacement using FIFO algorithm
- Memory protection via base/limit registers
- Page fault handling with automatic page loading
- Memory mapped I/O support
- Zero-copy operations for efficiency
- Shared memory regions between processes

### System Calls
#### Process Control
- `PROCESS_EXIT` (9): Terminate current process with cleanup
- `SET_QUANTUM` (6): Configure scheduler time slice (1-255 ticks)
- `LOAD_EXEC` (5): Load and execute new program with arguments

#### I/O Operations
- `PRINT_B` (4): Print 8-bit value from B register
- `PRINT_MEM` (3): Print memory contents starting at BC
- `READ_B` (7): Read 8-bit value into B register
- `READ_MEM` (2): Read value into memory at BC address
- `PRINT_STR` (1): Print null-terminated string at BC
- `READ_STR` (8): Read string into buffer at BC (max 255 chars)

### Example Programs

1. **Collatz.com**: Collatz Conjecture Calculator
   - Input range: 1-25
   - Features:
     - Dynamic memory allocation
     - Multiple process support
     - Result caching
   - Implementation:
     ```assembly
     ; Example Collatz sequence calculation
     mvi a, 0        ; Initialize counter
     mvi b, 25       ; Upper limit
     loop:
         push psw    ; Save counter
         call calc   ; Calculate sequence
         pop psw     ; Restore counter
         inr a       ; Next number
         cmp b       ; Check if done
         jnz loop    ; Continue if not
     ```

2. **Sum.com**: Advanced Number Series Calculator
   - Calculates sum of numbers 1-20
   - Features:
     - Optimized algorithm
     - Register-based computation
     - System call demonstration
   - Memory usage: < 1KB
   - Performance: O(n) complexity

3. **Primes.com**: Prime Number Generator
   - Features:
     - Sieve of Eratosthenes implementation
     - Dynamic memory allocation
     - Optimized division algorithm
   - Output formats:
     - Plain number list
     - Factor pairs
     - Statistical summary

## Implementation Details

### Memory Architecture
```
+------------------+ 0xFFFF
|    User Stack    |
+------------------+ 0x4000
|   Process Table  |
+------------------+ 0x0D00
| Interrupt Buffer |
+------------------+ 0x0100
|   System Code    |
+------------------+ 0x0000
```

#### Memory Regions
- System Code (0x0000 - 0x00FF): Reserved for OS
- Interrupt Buffer (0x0100 - 0x0CFF): System call handling
- Process Table (0x0D00 - 0x3FFF): Process management
- User Stack (0x4000 - 0xFFFF): Program execution

### Virtual Memory System
- Page Size: 1024 bytes (1KB)
- Page Table Entry Format:
  ```
  Bits 0-7: Physical Page Number
  Bit  8: Present
  Bit  9: Writable
  Bit 10: User Accessible
  Bit 11: Dirty
  Bit 12: Accessed
  ```
- Page Replacement:
  - FIFO queue implementation
  - Dirty page writing
  - Page fault handling

### Process Management Details
#### Process Table Entry (64 bytes)
```c
struct ProcessEntry {
    uint16_t pc;          // Program Counter
    uint16_t sp;          // Stack Pointer
    uint8_t  flags;       // CPU Flags
    uint8_t  registers[7];// General Purpose Registers
    uint8_t  state;       // Process State
    uint16_t base;        // Memory Base Address
    uint16_t limit;       // Memory Limit
    uint8_t  quantum;     // Time Quantum
    uint8_t  priority;    // Process Priority
    // ... additional metadata
};
```

### Interrupt Handling
1. Timer Interrupts
   - Generated every 10ms
   - Triggers scheduler
   - Updates system time

2. System Call Interrupts
   - Entry point: 0x0000
   - Register preservation
   - Error handling
   - Return value passing

### Building System

#### Prerequisites
- C++ compiler with C++17 support
- Make build system
- 8080 assembler (recommended: ASM80)

#### Build Process
1. Compile Core Components:
   ```bash
   make emulator    # Build emulator
   make memory      # Build memory manager
   make os          # Build OS core
   ```

2. Assemble Programs:
   ```bash
   # Using online assembler (www.asm80.com)
   1. Upload .asm file
   2. Select 8080 architecture
   3. Generate .com file
   ```

3. Full Build:
   ```bash
   make all         # Build everything
   make clean       # Clean build files
   make rebuild     # Clean and rebuild
   ```

### Project Structure
```
I8080-Microkernel/
├── ASM/                    # Assembly source files
│   ├── Collatz.asm        # Collatz conjecture implementation
│   ├── Sum.asm            # Number series calculator
│   └── Primes.asm         # Prime number generator
├── emulator_base.h         # Base emulator definitions
│   ├── CPU definitions
│   └── Instruction set
├── emulator_core.cpp      # Core emulator implementation
│   ├── Instruction decoder
│   └── Execution engine
├── emulator_enhanced.cpp  # Enhanced emulator features
│   ├── Debug support
│   └── Performance counters
├── memory_manager.cpp     # Memory management system
│   ├── Page table handler
│   └── Virtual memory mapper
├── os_core.cpp           # Operating system core
│   ├── System call handler
│   └── Process scheduler
└── main.cpp              # Main program entry
```

## Debugging and Monitoring

### Debug Levels
- Level 0: Normal operation
- Level 1: Basic instruction tracing
- Level 2: Memory access logging
- Level 3: Full system state dumps
- Level 4: Interrupt handling details
- Level 5: Page fault analysis

### Performance Monitoring
- Instruction count
- Page fault statistics
- Context switch timing
- Memory usage patterns
- System call frequency

## License
This project is licensed under the terms specified in the LICENSE file. See LICENSE for details.

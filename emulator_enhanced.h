#ifndef CPU8080_ENHANCED_H
#define CPU8080_ENHANCED_H

#include <cstdint>
#include <cstdio>
#include <vector>
#include <queue>
#include <memory>
#include <stdexcept>
#include <array>
#include <algorithm>
#include <iostream>
#include <string>
#include <unordered_map>
#include "emulator_base.h"
#include "memory_manager.h"

// Forward declarations
class MemoryCache;

/**
 * @brief Pre-calculated parity lookup table for fast parity checking
 * Index: 8-bit value to check
 * Value: 1 if even parity, 0 if odd parity
 */
extern const uint8_t PARITY_TABLE[256];

/**
 * @brief Custom exception class for emulator-specific errors
 */
class EmulatorException : public std::runtime_error {
public:
    enum class ErrorCode {
        INVALID_OPCODE,        ///< Attempted to execute invalid/unimplemented opcode
        MEMORY_ACCESS_VIOLATION, ///< Attempted to access invalid memory address
        STACK_OVERFLOW,        ///< Stack operation would overflow memory
        INVALID_INTERRUPT      ///< Invalid interrupt operation
    };

    EmulatorException(ErrorCode code, const char* msg) 
        : std::runtime_error(msg), code(code) {}
    
    ErrorCode getCode() const { return code; }

private:
    ErrorCode code;
};

/**
 * @brief Instruction tracing facility for debugging and analysis
 * Maintains a circular buffer of recent instruction executions
 */
class InstructionTracer {
public:
    struct TraceEntry {
        uint16_t pc;          ///< Program counter
        uint8_t opcode;       ///< Executed opcode
        State8080 state;      ///< CPU state snapshot
        uint64_t cycle;       ///< Cycle count at execution
    };

    /**
     * @brief Add a new trace entry
     * @param pc Program counter value
     * @param opcode Executed instruction
     * @param state CPU state
     * @param cycle Current cycle count
     */
    void addTrace(uint16_t pc, uint8_t opcode, const State8080& state, uint64_t cycle);

    /**
     * @brief Save trace buffer to a file in human-readable format
     * @param filename Output file path
     * @throws EmulatorException if file cannot be opened
     */
    void dumpTrace(const char* filename);
    
    void clear() { trace_buffer.clear(); }
    void setMaxEntries(size_t max) { max_entries = max; }

private:
    std::vector<TraceEntry> trace_buffer;
    size_t max_entries = 1000;
};

/**
 * @brief Priority-based interrupt controller
 * Manages interrupt requests with priority levels
 */
class InterruptController {
public:
    struct InterruptRequest {
        uint8_t code;         ///< Interrupt vector
        uint8_t priority;     ///< Priority level (0-255, higher = more priority)
        bool pending;         ///< Whether interrupt is pending
    };

    /**
     * @brief Queue a new interrupt request
     * @param code Interrupt vector
     * @param priority Priority level
     */
    void queueInterrupt(uint8_t code, uint8_t priority);
    
    bool hasInterrupt() const { return !int_queue.empty(); }
    
    /**
     * @brief Get the next pending interrupt
     * @return Next interrupt request
     * @throws EmulatorException if no interrupts pending
     */
    InterruptRequest getNextInterrupt();
    
    void clear() { while(!int_queue.empty()) int_queue.pop(); }

private:
    std::queue<InterruptRequest> int_queue;
};

/**
 * @brief Memory bank controller for extended memory support
 * Implements bank switching and memory mapping
 */
class MemoryBankController {
public:
    /**
     * @brief Initialize memory bank controller
     * @param num_banks Number of memory banks
     * @param bank_size Size of each bank in bytes
     */
    MemoryBankController(size_t num_banks = 4, size_t bank_size = 0x4000);
    
    /**
     * @brief Switch active memory bank
     * @param bank Bank number to switch to
     * @throws EmulatorException if bank number invalid
     */
    void switchBank(uint8_t bank);
    
    uint8_t getCurrentBank() const { return currentBank; }
    
    /**
     * @brief Map memory bank to address space
     * @param address Base address for mapping
     * @param bank Bank number to map
     * @throws EmulatorException if bank number invalid
     */
    void mapMemory(uint16_t address, uint8_t bank);
    
    /**
     * @brief Read byte from current bank
     * @param address Address within bank
     * @return Byte value
     * @throws EmulatorException if address invalid
     */
    uint8_t read(uint16_t address);
    
    /**
     * @brief Write byte to current bank
     * @param address Address within bank
     * @param value Byte to write
     * @throws EmulatorException if address invalid
     */
    void write(uint16_t address, uint8_t value);

private:
    std::vector<std::unique_ptr<uint8_t[]>> banks;
    uint8_t currentBank;
    size_t bankSize;
    
    // Memory mapping table
    struct BankMapping {
        uint8_t bank;         ///< Bank number
        uint16_t baseAddr;    ///< Base address in CPU space
        uint16_t size;        ///< Mapping size
        bool readOnly;        ///< Read-only mapping
    };
    std::vector<BankMapping> mappings;
};

// Instruction timing
struct InstructionTiming {
    uint8_t base_cycles;
    uint8_t condition_cycles;
    uint8_t memory_cycles;
};

extern const InstructionTiming TIMING_TABLE[256];

// State management
class StateManager {
public:
    void saveState(const char* filename, const State8080& state, const MemoryBase* memory);
    void loadState(const char* filename, State8080& state, MemoryBase* memory);
    void createSnapshot(const char* description, const State8080& state, const MemoryBase* memory);
    void restoreSnapshot(const char* description, State8080& state, MemoryBase* memory);

private:
    struct Snapshot {
        std::string description;
        State8080 state;
        std::vector<uint8_t> memory;
    };
    std::vector<Snapshot> snapshots;
};

// Instruction profiling
struct InstructionProfile {
    uint64_t execution_count;
    uint64_t total_cycles;
    uint64_t max_cycles;
    uint64_t cache_misses;
};

class Profiler {
public:
    void recordExecution(uint8_t opcode, uint64_t cycles, bool cache_miss = false);
    void reset();
    void generateReport(const char* filename);
    const InstructionProfile& getProfile(uint8_t opcode) const { return profiles[opcode]; }

private:
    std::array<InstructionProfile, 256> profiles{};
};

// Testing framework
class EmulatorTest {
public:
    EmulatorTest();
    void testArithmetic();
    void testLogic();
    void testBranching();
    void testMemoryOps();
    void testInterrupts();
    void runAllTests();

private:
    State8080 state;
    std::unique_ptr<MemoryBase> memory;
    std::unique_ptr<CPU8080> cpu;
    
    void setUp();
    void tearDown();
    void assertCondition(bool condition, const char* message);
};

// Enhanced CPU8080 class
class EnhancedCPU8080 : public CPU8080 {
public:
    EnhancedCPU8080(State8080* state, MemoryBase* memory);
    
    // Enhanced features
    void enableTracing(bool enable = true);
    void enableProfiling(bool enable = true);
    void enableBanking(bool enable = true);
    
    // Override main emulation function
    virtual unsigned Emulate8080p(int debug = 0);
    
    // Access to enhancement features
    InstructionTracer& getTracer() { return tracer; }
    InterruptController& getInterruptController() { return intController; }
    MemoryBankController& getMemoryBankController() { return memoryBanking; }
    StateManager& getStateManager() { return stateManager; }
    Profiler& getProfiler() { return profiler; }

private:
    bool tracingEnabled;
    bool profilingEnabled;
    bool bankingEnabled;
    
    InstructionTracer tracer;
    InterruptController intController;
    MemoryBankController memoryBanking;
    StateManager stateManager;
    Profiler profiler;

    // Cache-related members
    struct CachedInstruction {
        uint8_t length;
        uint8_t cycles;
        bool affects_flags;
        uint8_t result_a;
        uint8_t result_b;
        uint8_t result_c;
        uint8_t result_d;
        uint8_t result_e;
        uint8_t result_h;
        uint8_t result_l;
    };
    
    std::unordered_map<uint16_t, CachedInstruction> instructionCache;
    std::unique_ptr<MemoryCache> memoryCache;
    uint32_t cacheFlushCounter = 0;
};

#endif 
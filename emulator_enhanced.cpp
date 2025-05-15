#include "emulator_enhanced.h"
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <unordered_map>

// Pre-calculated parity lookup table
const uint8_t PARITY_TABLE[256] = {
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1
};

// Performance optimization: Cache for frequently accessed memory locations
class MemoryCache {
private:
    static constexpr size_t CACHE_SIZE = 256;
    struct CacheEntry {
        uint16_t address;
        uint8_t value;
        bool valid;
        bool dirty;
    };
    std::array<CacheEntry, CACHE_SIZE> cache;
    MemoryBase* memory;

public:
    MemoryCache(MemoryBase* mem) : memory(mem) {
        for (auto& entry : cache) {
            entry.valid = false;
            entry.dirty = false;
        }
    }

    uint8_t read(uint16_t address) {
        size_t index = address & (CACHE_SIZE - 1);
        auto& entry = cache[index];
        
        if (entry.valid && entry.address == address) {
            return entry.value;
        }

        // Cache miss
        if (entry.valid && entry.dirty) {
            memory->at(entry.address) = entry.value;
        }

        entry.address = address;
        entry.value = memory->at(address);
        entry.valid = true;
        entry.dirty = false;
        
        return entry.value;
    }

    void write(uint16_t address, uint8_t value) {
        try {
            size_t index = address & (CACHE_SIZE - 1);
            auto& entry = cache[index];

            entry.address = address;
            entry.value = value;
            entry.valid = true;
            entry.dirty = true;
        } catch (const std::exception& e) {
            throw EmulatorException(EmulatorException::ErrorCode::MEMORY_ACCESS_VIOLATION,
                                  "Cache write failed");
        }
    }

    void flush() {
        try {
            for (auto& entry : cache) {
                if (entry.valid && entry.dirty) {
                    memory->at(entry.address) = entry.value;
                    entry.dirty = false;
                }
            }
        } catch (const std::exception& e) {
            throw EmulatorException(EmulatorException::ErrorCode::MEMORY_ACCESS_VIOLATION,
                                  "Cache flush failed");
        }
    }
};

// Performance optimization: Instruction decoder lookup table
struct DecodedInstruction {
    uint8_t length;      // Instruction length in bytes
    uint8_t cycles;      // Base cycle count
    bool affects_flags;  // Whether instruction affects flags
    bool memory_access;  // Whether instruction accesses memory
};

// Helper function to create instruction table
static std::array<DecodedInstruction, 256> createInstructionTable() {
    std::array<DecodedInstruction, 256> table;
    
    // Initialize all entries with default values
    for (auto& entry : table) {
        entry = {1, 4, false, false};  // Default: 1 byte, 4 cycles, no flags, no memory
    }
    
    // Set specific instruction properties
    // Data Transfer Group
    table[0x00] = {1, 4, false, false};  // NOP
    table[0x01] = {3, 10, false, false}; // LXI B,word
    table[0x02] = {1, 7, false, true};   // STAX B
    table[0x03] = {1, 5, false, false};  // INX B
    table[0x04] = {1, 5, true, false};   // INR B
    table[0x05] = {1, 5, true, false};   // DCR B
    table[0x06] = {2, 7, false, false};  // MVI B,byte
    table[0x07] = {1, 4, true, false};   // RLC
    table[0x08] = {1, 4, false, false};  // NOP
    table[0x09] = {1, 10, false, false}; // DAD B
    table[0x0A] = {1, 7, false, true};   // LDAX B
    table[0x0B] = {1, 5, false, false};  // DCX B
    table[0x0C] = {1, 5, true, false};   // INR C
    table[0x0D] = {1, 5, true, false};   // DCR C
    table[0x0E] = {2, 7, false, false};  // MVI C,byte
    table[0x0F] = {1, 4, true, false};   // RRC
    
    // More Data Transfer
    table[0x11] = {3, 10, false, false}; // LXI D,word
    table[0x12] = {1, 7, false, true};   // STAX D
    table[0x13] = {1, 5, false, false};  // INX D
    table[0x14] = {1, 5, true, false};   // INR D
    table[0x15] = {1, 5, true, false};   // DCR D
    table[0x16] = {2, 7, false, false};  // MVI D,byte
    table[0x17] = {1, 4, true, false};   // RAL
    
    // Memory and Register Operations
    table[0x21] = {3, 10, false, false}; // LXI H,word
    table[0x22] = {3, 16, false, true};  // SHLD addr
    table[0x23] = {1, 5, false, false};  // INX H
    table[0x24] = {1, 5, true, false};   // INR H
    table[0x25] = {1, 5, true, false};   // DCR H
    table[0x26] = {2, 7, false, false};  // MVI H,byte
    table[0x27] = {1, 4, true, false};   // DAA
    
    // Stack Operations
    table[0x31] = {3, 10, false, false}; // LXI SP,word
    table[0x32] = {3, 13, false, true};  // STA addr
    table[0x33] = {1, 5, false, false};  // INX SP
    table[0x34] = {1, 10, true, true};   // INR M
    table[0x35] = {1, 10, true, true};   // DCR M
    table[0x36] = {2, 10, false, true};  // MVI M,byte
    table[0x37] = {1, 4, true, false};   // STC
    
    // Register Move Instructions
    for (int i = 0x40; i <= 0x7F; i++) {
        bool isMemoryOp = ((i & 0x07) == 0x06) || ((i & 0x3F) == 0x36);
        table[static_cast<uint8_t>(i)] = {1, static_cast<uint8_t>(isMemoryOp ? 7 : 5), false, isMemoryOp};
    }
    
    // Arithmetic Group
    for (int i = 0x80; i <= 0x87; i++) {
        bool isMemoryOp = (i & 0x07) == 0x06;
        table[static_cast<uint8_t>(i)] = {1, static_cast<uint8_t>(isMemoryOp ? 7 : 4), true, isMemoryOp};
    }
    for (int i = 0x88; i <= 0x8F; i++) {
        bool isMemoryOp = (i & 0x07) == 0x06;
        table[static_cast<uint8_t>(i)] = {1, static_cast<uint8_t>(isMemoryOp ? 7 : 4), true, isMemoryOp};
    }
    for (int i = 0x90; i <= 0x97; i++) {
        bool isMemoryOp = (i & 0x07) == 0x06;
        table[static_cast<uint8_t>(i)] = {1, static_cast<uint8_t>(isMemoryOp ? 7 : 4), true, isMemoryOp};
    }
    for (int i = 0x98; i <= 0x9F; i++) {
        bool isMemoryOp = (i & 0x07) == 0x06;
        table[static_cast<uint8_t>(i)] = {1, static_cast<uint8_t>(isMemoryOp ? 7 : 4), true, isMemoryOp};
    }
    
    // Logical Group
    for (int i = 0xA0; i <= 0xA7; i++) {
        bool isMemoryOp = (i & 0x07) == 0x06;
        table[static_cast<uint8_t>(i)] = {1, static_cast<uint8_t>(isMemoryOp ? 7 : 4), true, isMemoryOp};
    }
    for (int i = 0xA8; i <= 0xAF; i++) {
        bool isMemoryOp = (i & 0x07) == 0x06;
        table[static_cast<uint8_t>(i)] = {1, static_cast<uint8_t>(isMemoryOp ? 7 : 4), true, isMemoryOp};
    }
    for (int i = 0xB0; i <= 0xB7; i++) {
        bool isMemoryOp = (i & 0x07) == 0x06;
        table[static_cast<uint8_t>(i)] = {1, static_cast<uint8_t>(isMemoryOp ? 7 : 4), true, isMemoryOp};
    }
    for (int i = 0xB8; i <= 0xBF; i++) {
        bool isMemoryOp = (i & 0x07) == 0x06;
        table[static_cast<uint8_t>(i)] = {1, static_cast<uint8_t>(isMemoryOp ? 7 : 4), true, isMemoryOp};
    }
    
    // Branch Group
    table[0xC0] = {1, 11, false, false}; // RNZ
    table[0xC1] = {1, 10, false, false}; // POP B
    table[0xC2] = {3, 10, false, false}; // JNZ addr
    table[0xC3] = {3, 10, false, false}; // JMP addr
    table[0xC4] = {3, 17, false, false}; // CNZ addr
    table[0xC5] = {1, 11, false, false}; // PUSH B
    table[0xC6] = {2, 7, true, false};   // ADI byte
    table[0xC7] = {1, 11, false, false}; // RST 0
    
    // More Branch Instructions
    table[0xC8] = {1, 11, false, false}; // RZ
    table[0xC9] = {1, 10, false, false}; // RET
    table[0xCA] = {3, 10, false, false}; // JZ addr
    table[0xCC] = {3, 17, false, false}; // CZ addr
    table[0xCD] = {3, 17, false, false}; // CALL addr
    table[0xCE] = {2, 7, true, false};   // ACI byte
    table[0xCF] = {1, 11, false, false}; // RST 1
    
    // I/O and Special Instructions
    table[0xD3] = {2, 10, false, false}; // OUT port
    table[0xDB] = {2, 10, false, false}; // IN port
    table[0xE3] = {1, 18, false, false}; // XTHL
    table[0xE9] = {1, 5, false, false};  // PCHL
    table[0xEB] = {1, 5, false, false};  // XCHG
    table[0xF3] = {1, 4, false, false};  // DI
    table[0xFB] = {1, 4, false, false};  // EI
    
    return table;
}

// Initialize instruction table
static const std::array<DecodedInstruction, 256> INSTRUCTION_TABLE = createInstructionTable();

// Instruction Tracer Implementation
void InstructionTracer::dumpTrace(const char* filename) {
    std::ofstream file(filename);
    if (!file) {
        throw EmulatorException(EmulatorException::ErrorCode::INVALID_OPCODE, 
                              "Failed to open trace file");
    }

    file << "PC    | Opcode | A  B  C  D  E  H  L  | Flags | Cycle\n";
    file << "------+--------+--------------------+-------+-------\n";

    for (const auto& entry : trace_buffer) {
        file << std::hex << std::setw(4) << std::setfill('0') << entry.pc << " | "
             << std::setw(2) << static_cast<int>(entry.opcode) << "     | "
             << std::setw(2) << static_cast<int>(entry.state.a) << " "
             << std::setw(2) << static_cast<int>(entry.state.b) << " "
             << std::setw(2) << static_cast<int>(entry.state.c) << " "
             << std::setw(2) << static_cast<int>(entry.state.d) << " "
             << std::setw(2) << static_cast<int>(entry.state.e) << " "
             << std::setw(2) << static_cast<int>(entry.state.h) << " "
             << std::setw(2) << static_cast<int>(entry.state.l) << " | "
             << (entry.state.cc.z ? 'Z' : '.') 
             << (entry.state.cc.s ? 'S' : '.')
             << (entry.state.cc.p ? 'P' : '.')
             << (entry.state.cc.cy ? 'C' : '.')
             << (entry.state.cc.ac ? 'A' : '.') << " | "
             << std::dec << entry.cycle << "\n";
    }
}

void InstructionTracer::addTrace(uint16_t pc, uint8_t opcode, const State8080& state, uint64_t cycle) {
    TraceEntry entry{pc, opcode, state, cycle};
    trace_buffer.push_back(entry);
    if (trace_buffer.size() > max_entries) {
        trace_buffer.erase(trace_buffer.begin());
    }
}

// Interrupt Controller Implementation
void InterruptController::queueInterrupt(uint8_t code, uint8_t priority) {
    InterruptRequest req{code, priority, true};
    int_queue.push(req);
}

InterruptController::InterruptRequest InterruptController::getNextInterrupt() {
    if (int_queue.empty()) {
        throw EmulatorException(EmulatorException::ErrorCode::INVALID_INTERRUPT,
                              "No pending interrupts");
    }
    auto req = int_queue.front();
    int_queue.pop();
    return req;
}

// Memory Bank Controller Implementation
MemoryBankController::MemoryBankController(size_t num_banks, size_t bank_size)
    : currentBank(0), bankSize(bank_size) {
    banks.reserve(num_banks);
    for (size_t i = 0; i < num_banks; i++) {
        banks.push_back(std::make_unique<uint8_t[]>(bank_size));
    }
}

void MemoryBankController::switchBank(uint8_t bank) {
    try {
        if (bank >= banks.size()) {
            throw EmulatorException(EmulatorException::ErrorCode::MEMORY_ACCESS_VIOLATION,
                                  "Invalid bank number");
        }
        
        // Flush any pending writes in current bank
        for (auto& entry : mappings) {
            if (entry.bank == currentBank) {
                // Flush changes before switching
                // Implementation depends on your memory management
            }
        }
        
        currentBank = bank;
    } catch (const std::bad_alloc& e) {
        throw EmulatorException(EmulatorException::ErrorCode::MEMORY_ACCESS_VIOLATION,
                              "Memory allocation failed during bank switch");
    } catch (const std::exception& e) {
        throw EmulatorException(EmulatorException::ErrorCode::MEMORY_ACCESS_VIOLATION,
                              e.what());
    }
}

void MemoryBankController::mapMemory(uint16_t address, uint8_t bank) {
    try {
        if (bank >= banks.size()) {
            throw EmulatorException(EmulatorException::ErrorCode::MEMORY_ACCESS_VIOLATION,
                                  "Invalid bank number");
        }

        // Check for overlapping mappings
        for (const auto& mapping : mappings) {
            if ((address >= mapping.baseAddr && 
                 address < mapping.baseAddr + mapping.size) ||
                (address + bankSize > mapping.baseAddr && 
                 address + bankSize <= mapping.baseAddr + mapping.size)) {
                throw EmulatorException(EmulatorException::ErrorCode::MEMORY_ACCESS_VIOLATION,
                                      "Memory mapping overlap");
            }
        }

        // Validate address range
        if (address + bankSize > 0xFFFF) {
            throw EmulatorException(EmulatorException::ErrorCode::MEMORY_ACCESS_VIOLATION,
                                  "Memory mapping exceeds address space");
        }

        // Add new mapping
        BankMapping mapping{
            bank,
            address,
            static_cast<uint16_t>(bankSize),
            false  // Default to read-write
        };
        mappings.push_back(mapping);
    } catch (const std::bad_alloc& e) {
        throw EmulatorException(EmulatorException::ErrorCode::MEMORY_ACCESS_VIOLATION,
                              "Memory allocation failed during mapping");
    } catch (const std::exception& e) {
        throw EmulatorException(EmulatorException::ErrorCode::MEMORY_ACCESS_VIOLATION,
                              e.what());
    }
}

uint8_t MemoryBankController::read(uint16_t address) {
    try {
        // Check mapped regions first
        for (const auto& mapping : mappings) {
            if (address >= mapping.baseAddr && 
                address < mapping.baseAddr + mapping.size) {
                uint16_t bankOffset = address - mapping.baseAddr;
                if (bankOffset >= bankSize) {
                    throw EmulatorException(EmulatorException::ErrorCode::MEMORY_ACCESS_VIOLATION,
                                          "Bank offset out of range");
                }
                return banks[mapping.bank][bankOffset];
            }
        }

        // If not mapped, use current bank
        if (address >= bankSize) {
            throw EmulatorException(EmulatorException::ErrorCode::MEMORY_ACCESS_VIOLATION,
                                  "Address out of range");
        }
        return banks[currentBank][address];
    } catch (const std::out_of_range& e) {
        throw EmulatorException(EmulatorException::ErrorCode::MEMORY_ACCESS_VIOLATION,
                              "Invalid memory access");
    } catch (const std::exception& e) {
        throw EmulatorException(EmulatorException::ErrorCode::MEMORY_ACCESS_VIOLATION,
                              e.what());
    }
}

void MemoryBankController::write(uint16_t address, uint8_t value) {
    try {
        // Check mapped regions first
        for (const auto& mapping : mappings) {
            if (address >= mapping.baseAddr && 
                address < mapping.baseAddr + mapping.size) {
                if (mapping.readOnly) {
                    throw EmulatorException(EmulatorException::ErrorCode::MEMORY_ACCESS_VIOLATION,
                                          "Write to read-only memory");
                }
                uint16_t bankOffset = address - mapping.baseAddr;
                if (bankOffset >= bankSize) {
                    throw EmulatorException(EmulatorException::ErrorCode::MEMORY_ACCESS_VIOLATION,
                                          "Bank offset out of range");
                }
                banks[mapping.bank][bankOffset] = value;
                return;
            }
        }

        // If not mapped, use current bank
        if (address >= bankSize) {
            throw EmulatorException(EmulatorException::ErrorCode::MEMORY_ACCESS_VIOLATION,
                                  "Address out of range");
        }
        banks[currentBank][address] = value;
    } catch (const std::out_of_range& e) {
        throw EmulatorException(EmulatorException::ErrorCode::MEMORY_ACCESS_VIOLATION,
                              "Invalid memory access");
    } catch (const std::exception& e) {
        throw EmulatorException(EmulatorException::ErrorCode::MEMORY_ACCESS_VIOLATION,
                              e.what());
    }
}

// State Manager Implementation
void StateManager::saveState(const char* filename, const State8080& state, const MemoryBase* memory) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        throw EmulatorException(EmulatorException::ErrorCode::INVALID_OPCODE,
                              "Failed to open state file");
    }

    // Save CPU state
    file.write(reinterpret_cast<const char*>(&state), sizeof(State8080));

    // Save memory contents
    // This assumes memory is contiguous - modify if using banking
    for (uint32_t i = 0; i < 0x10000; i++) {
        // Use const_cast since we know we're only reading
        file.put(const_cast<MemoryBase*>(memory)->at(i));
    }
}

void StateManager::loadState(const char* filename, State8080& state, MemoryBase* memory) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        throw EmulatorException(EmulatorException::ErrorCode::INVALID_OPCODE,
                              "Failed to open state file");
    }

    // Load CPU state
    file.read(reinterpret_cast<char*>(&state), sizeof(State8080));

    // Load memory contents
    for (uint32_t i = 0; i < 0x10000; i++) {
        memory->at(i) = file.get();
    }
}

void StateManager::createSnapshot(const char* description, const State8080& state, const MemoryBase* memory) {
    Snapshot snapshot;
    snapshot.description = description;
    snapshot.state = state;
    
    // Copy memory contents
    snapshot.memory.reserve(0x10000);
    for (uint32_t i = 0; i < 0x10000; i++) {
        // Use const_cast since we know we're only reading
        snapshot.memory.push_back(const_cast<MemoryBase*>(memory)->at(i));
    }
    
    snapshots.push_back(std::move(snapshot));
}

void StateManager::restoreSnapshot(const char* description, State8080& state, MemoryBase* memory) {
    auto it = std::find_if(snapshots.begin(), snapshots.end(),
                          [description](const Snapshot& s) { 
                              return s.description == description; 
                          });
    
    if (it == snapshots.end()) {
        throw EmulatorException(EmulatorException::ErrorCode::INVALID_OPCODE,
                              "Snapshot not found");
    }
    
    state = it->state;
    for (uint32_t i = 0; i < 0x10000; i++) {
        memory->at(i) = it->memory[i];
    }
}

// Profiler Implementation
void Profiler::recordExecution(uint8_t opcode, uint64_t cycles, bool cache_miss) {
    auto& profile = profiles[opcode];
    profile.execution_count++;
    profile.total_cycles += cycles;
    profile.max_cycles = std::max(profile.max_cycles, cycles);
    if (cache_miss) profile.cache_misses++;
}

void Profiler::reset() {
    for (auto& profile : profiles) {
        profile = InstructionProfile{};
    }
}

void Profiler::generateReport(const char* filename) {
    std::ofstream file(filename);
    if (!file) {
        throw EmulatorException(EmulatorException::ErrorCode::INVALID_OPCODE,
                              "Failed to open profile report file");
    }

    file << "Opcode | Count | Total Cycles | Avg Cycles | Max Cycles | Cache Misses\n";
    file << "-------+-------+--------------+------------+------------+-------------\n";

    for (size_t i = 0; i < 256; i++) {
        const auto& profile = profiles[i];
        if (profile.execution_count > 0) {
            file << std::hex << std::setw(2) << std::setfill('0') << i << "     | "
                 << std::dec << std::setw(7) << profile.execution_count << " | "
                 << std::setw(12) << profile.total_cycles << " | "
                 << std::setw(10) << (profile.total_cycles / profile.execution_count) << " | "
                 << std::setw(10) << profile.max_cycles << " | "
                 << std::setw(11) << profile.cache_misses << "\n";
        }
    }
}

// Enhanced CPU8080 Implementation with optimizations
EnhancedCPU8080::EnhancedCPU8080(State8080* state, MemoryBase* memory)
    : CPU8080(memory),  // Base class only takes memory
      tracingEnabled(false),
      profilingEnabled(false),
      bankingEnabled(false),
      memoryBanking(4, 0x4000),
      memoryCache(std::make_unique<MemoryCache>(memory)),
      cacheFlushCounter(0) {
    
    // Set state after base class initialization
    this->state = state;
    
    // Initialize instruction cache
    instructionCache.reserve(1024);  // Pre-allocate space for commonly used code
}

void EnhancedCPU8080::enableTracing(bool enable) {
    tracingEnabled = enable;
    if (!enable) {
        tracer.clear();
    }
}

void EnhancedCPU8080::enableProfiling(bool enable) {
    profilingEnabled = enable;
    if (!enable) {
        profiler.reset();
    }
}

void EnhancedCPU8080::enableBanking(bool enable) {
    bankingEnabled = enable;
}

unsigned EnhancedCPU8080::Emulate8080p(int debug) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Get the current instruction
    uint8_t *opcode = &this->memory->at(this->state->pc);
    uint64_t current_cycle = 0;
    
    // Check instruction cache first
    if (!debug) {
        auto cache_it = instructionCache.find(this->state->pc);
        if (cache_it != instructionCache.end()) {
            const auto& cached = cache_it->second;
            
            // Skip execution if instruction doesn't affect flags and no memory access
            if (!cached.affects_flags && !INSTRUCTION_TABLE[*opcode].memory_access) {
                // Fast path - apply cached results
                this->state->a = cached.result_a;
                this->state->b = cached.result_b;
                this->state->c = cached.result_c;
                this->state->d = cached.result_d;
                this->state->e = cached.result_e;
                this->state->h = cached.result_h;
                this->state->l = cached.result_l;
                
                this->state->pc += cached.length;
                current_cycle = cached.cycles;
                
                // Record profiling if enabled
                if (profilingEnabled) {
                    auto end_time = std::chrono::high_resolution_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>
                                  (end_time - start_time).count();
                    profiler.recordExecution(*opcode, current_cycle, false);
                }
                
                return current_cycle;
            }
        }
    }
    
    // Cache miss or debug mode - execute normally
    current_cycle = CPU8080::Emulate8080p(debug);
    
    // Cache the result if appropriate
    if (!debug && instructionCache.size() < 1024) {
        CachedInstruction cached;
        cached.length = INSTRUCTION_TABLE[*opcode].length;
        cached.cycles = current_cycle;
        cached.affects_flags = INSTRUCTION_TABLE[*opcode].affects_flags;
        cached.result_a = this->state->a;
        cached.result_b = this->state->b;
        cached.result_c = this->state->c;
        cached.result_d = this->state->d;
        cached.result_e = this->state->e;
        cached.result_h = this->state->h;
        cached.result_l = this->state->l;
        
        instructionCache[this->state->pc - cached.length] = cached;
    }
    
    // Record trace if enabled
    if (tracingEnabled) {
        tracer.addTrace(this->state->pc, *opcode, *this->state, current_cycle);
    }
    
    // Record profiling if enabled
    if (profilingEnabled) {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>
                       (end_time - start_time).count();
        bool cache_miss = duration > (current_cycle * 10);
        profiler.recordExecution(*opcode, current_cycle, cache_miss);
    }
    
    // Flush memory cache periodically
    if (++cacheFlushCounter >= 1000) {
        memoryCache->flush();
        cacheFlushCounter = 0;
    }
    
    return current_cycle;
}

// Testing Framework Implementation
EmulatorTest::EmulatorTest() {
    setUp();
}

void EmulatorTest::setUp() {
    memory = std::make_unique<Memory>(0x10000);
    state = State8080{};
    cpu = std::make_unique<EnhancedCPU8080>(&state, memory.get());
}

void EmulatorTest::tearDown() {
    memory.reset();
    cpu.reset();
}

void EmulatorTest::assertCondition(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void EmulatorTest::testArithmetic() {
    // Test ADD instructions
    state.a = 0x05;
    state.b = 0x03;
    memory->at(0) = 0x80;  // ADD B
    cpu->Emulate8080p(0);
    assertCondition(state.a == 0x08, "ADD B failed");
    assertCondition(!state.cc.cy, "Carry flag incorrectly set");
    
    // Test ADD with carry
    state.a = 0xFF;
    state.b = 0x01;
    memory->at(0) = 0x80;  // ADD B
    cpu->Emulate8080p(0);
    assertCondition(state.a == 0x00, "ADD B with carry failed");
    assertCondition(state.cc.cy, "Carry flag not set");
    assertCondition(state.cc.z, "Zero flag not set");
    
    // Test SUB instructions
    state.a = 0x05;
    state.b = 0x03;
    memory->at(0) = 0x90;  // SUB B
    cpu->Emulate8080p(0);
    assertCondition(state.a == 0x02, "SUB B failed");
    
    // Test SUB with borrow
    state.a = 0x00;
    state.b = 0x01;
    memory->at(0) = 0x90;  // SUB B
    cpu->Emulate8080p(0);
    assertCondition(state.a == 0xFF, "SUB B with borrow failed");
    assertCondition(state.cc.cy, "Carry flag not set");
    
    // Test DAA (Decimal Adjust Accumulator)
    state.a = 0x9B;
    memory->at(0) = 0x27;  // DAA
    cpu->Emulate8080p(0);
    assertCondition(state.a == 0x01, "DAA failed");
    assertCondition(state.cc.cy, "DAA carry flag not set");
}

void EmulatorTest::testLogic() {
    // Test AND instructions
    state.a = 0x0F;
    state.b = 0x0A;
    memory->at(0) = 0xA0;  // ANA B
    cpu->Emulate8080p(0);
    assertCondition(state.a == 0x0A, "ANA B failed");
    
    // Test OR instructions
    state.a = 0x0F;
    state.b = 0xF0;
    memory->at(0) = 0xB0;  // ORA B
    cpu->Emulate8080p(0);
    assertCondition(state.a == 0xFF, "ORA B failed");
    
    // Test XOR instructions
    state.a = 0xFF;
    state.b = 0x0F;
    memory->at(0) = 0xA8;  // XRA B
    cpu->Emulate8080p(0);
    assertCondition(state.a == 0xF0, "XRA B failed");
    
    // Test CMA (Complement Accumulator)
    state.a = 0xAA;
    memory->at(0) = 0x2F;  // CMA
    cpu->Emulate8080p(0);
    assertCondition(state.a == 0x55, "CMA failed");
}

void EmulatorTest::testBranching() {
    // Test JMP instruction
    memory->at(0) = 0xC3;  // JMP
    memory->at(1) = 0x10;  // Low byte of address
    memory->at(2) = 0x00;  // High byte of address
    cpu->Emulate8080p(0);
    assertCondition(state.pc == 0x0010, "JMP failed");
    
    // Test conditional jumps
    // JZ when zero flag is set
    state.cc.z = 1;
    memory->at(0x0010) = 0xCA;  // JZ
    memory->at(0x0011) = 0x20;  // Low byte
    memory->at(0x0012) = 0x00;  // High byte
    cpu->Emulate8080p(0);
    assertCondition(state.pc == 0x0020, "JZ (taken) failed");
    
    // JZ when zero flag is clear
    state.cc.z = 0;
    memory->at(0x0020) = 0xCA;  // JZ
    memory->at(0x0021) = 0x30;  // Low byte
    memory->at(0x0022) = 0x00;  // High byte
    cpu->Emulate8080p(0);
    assertCondition(state.pc == 0x0023, "JZ (not taken) failed");
    
    // Test CALL and RET
    memory->at(0x0023) = 0xCD;  // CALL
    memory->at(0x0024) = 0x40;  // Low byte
    memory->at(0x0025) = 0x00;  // High byte
    uint16_t oldSP = state.sp;
    cpu->Emulate8080p(0);
    assertCondition(state.pc == 0x0040, "CALL failed");
    assertCondition(state.sp == oldSP - 2, "CALL stack push failed");
    
    memory->at(0x0040) = 0xC9;  // RET
    cpu->Emulate8080p(0);
    assertCondition(state.pc == 0x0026, "RET failed");
    assertCondition(state.sp == oldSP, "RET stack pop failed");
}

void EmulatorTest::testMemoryOps() {
    // Test LDA instruction
    memory->at(0) = 0x3A;  // LDA
    memory->at(1) = 0x10;  // Low byte of address
    memory->at(2) = 0x00;  // High byte of address
    memory->at(0x0010) = 0x55;  // Test value
    cpu->Emulate8080p(0);
    assertCondition(state.a == 0x55, "LDA failed");
    
    // Test STA instruction
    state.a = 0xAA;
    memory->at(0) = 0x32;  // STA
    memory->at(1) = 0x20;  // Low byte of address
    memory->at(2) = 0x00;  // High byte of address
    cpu->Emulate8080p(0);
    assertCondition(memory->at(0x0020) == 0xAA, "STA failed");
    
    // Test LHLD instruction
    memory->at(0) = 0x2A;  // LHLD
    memory->at(1) = 0x30;  // Low byte of address
    memory->at(2) = 0x00;  // High byte of address
    memory->at(0x0030) = 0x78;  // Low byte of value
    memory->at(0x0031) = 0x56;  // High byte of value
    cpu->Emulate8080p(0);
    assertCondition(state.l == 0x78, "LHLD (low) failed");
    assertCondition(state.h == 0x56, "LHLD (high) failed");
    
    // Test SHLD instruction
    state.h = 0x34;
    state.l = 0x12;
    memory->at(0) = 0x22;  // SHLD
    memory->at(1) = 0x40;  // Low byte of address
    memory->at(2) = 0x00;  // High byte of address
    cpu->Emulate8080p(0);
    assertCondition(memory->at(0x0040) == 0x12, "SHLD (low) failed");
    assertCondition(memory->at(0x0041) == 0x34, "SHLD (high) failed");
}

void EmulatorTest::testInterrupts() {
    // Test interrupt handling
    state.int_enable = 1;
    uint16_t oldPC = state.pc;
    uint16_t oldSP = state.sp;
    
    cpu->raiseInterrupt(0x08);
    cpu->Emulate8080p(0);
    
    // Check interrupt vector
    assertCondition(state.pc == 0x0008, "Interrupt vector failed");
    // Check stack push
    assertCondition(state.sp == oldSP - 2, "Interrupt stack push failed");
    // Check return address
    uint16_t retAddr = (memory->at(oldSP - 1) << 8) | memory->at(oldSP - 2);
    assertCondition(retAddr == oldPC + 1, "Interrupt return address failed");
    
    // Test interrupt disable
    state.int_enable = 0;
    oldPC = state.pc;
    cpu->raiseInterrupt(0x10);
    cpu->Emulate8080p(0);
    assertCondition(state.pc == oldPC + 1, "Disabled interrupt was taken");
}

void EmulatorTest::runAllTests() {
    try {
        testArithmetic();
        testLogic();
        testBranching();
        testMemoryOps();
        testInterrupts();
        std::cout << "All tests passed successfully!\n";
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << "\n";
    }
} 

 
#ifndef H_8080EMUCPP
#define H_8080EMUCPP

#include <cstdio>
#include <cstring>
#include <cstdlib>

#if defined __UINT32_MAX__ or UINT32_MAX
  #include <inttypes.h>
#else
  typedef unsigned char uint8_t;
  typedef unsigned short uint16_t;
  typedef unsigned long uint32_t;
  typedef unsigned long long uint64_t;
#endif
#include "memoryBase.h"
//#include <sys/time>

//Some code cares that these flags are in exact 
// right bits when.  For instance, some code
// "pops" values into the PSW that they didn't push.
//
typedef struct ConditionCodes {
	uint8_t		cy:1;
	uint8_t		pad:1;
	uint8_t		p:1;
	uint8_t		pad2:1;
	uint8_t		ac:1;
	uint8_t		pad3:1;
	uint8_t		z:1;
	uint8_t		s:1;
} ConditionCodes;

typedef struct State8080 {
	uint8_t		a;
	uint8_t		b;
	uint8_t		c;
	uint8_t		d;
	uint8_t		e;
	uint8_t		h;
	uint8_t		l;
	uint16_t	sp;
	uint16_t	pc;
//	uint8_t		*memory;
	struct ConditionCodes		cc;
	uint8_t		int_enable;

} State8080;



class CPU8080 {
	friend class GTUOS;
public:
	uint8_t interrupt = 0;  // Interrupt
	uint8_t interrupt_code =0; // Interrupt code Unnecessary
	uint8_t quantum = 80;  // Round Robin quantum
	uint8_t scheduler_timer = 0; //Current Execution Time
	uint8_t initialized = 0;
	uint16_t int_buffer = 256; // Interrupt Buffer Address
       CPU8080(MemoryBase *mem);        
		~CPU8080();
        unsigned Emulate8080p(int debug = 0);
        void ClearInterrupt();
	void raiseInterrupt(uint8_t code);
	void dispatchScheduler();
        bool isHalted() const;
        bool isSystemCall() const;
	uint16_t getInterruptBufferAddress(){return int_buffer;}
	void setInterruptBufferAddress(uint16_t address){int_buffer = address;}
	void setQuantum(uint8_t quant);
	void onInterrupt();
	void ReadFileIntoMemoryAt(const char* filename, uint32_t offset);
private:
		void operator=(const CPU8080 & o) {}
		CPU8080(const CPU8080 & o) {}

        State8080 * state;
        MemoryBase * memory;
	unsigned char * lastOpcode;
};

#endif



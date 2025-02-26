#include <stddef.h>
#include <stdint.h>

volatile uint32_t cons_buff[5];

static inline void HC_out32(uint16_t port, uint32_t val){
	__asm__ volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

void
	__attribute__((noreturn))
	__attribute__((section(".start")))
	_start(void)
{
	
	/* Write code here */
	uint32_t base = (uint32_t)((uintptr_t)cons_buff);
	HC_out32(0xF1, base);
	while(1){
		HC_out32(0xF3, 0);
	}
}

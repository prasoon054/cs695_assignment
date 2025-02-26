#include <stddef.h>
#include <stdint.h>

volatile uint32_t prod_buff[5];

static inline void HC_out32(uint16_t port, uint32_t val)
{
	__asm__ volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

void
	__attribute__((noreturn))
	__attribute__((section(".start")))
	_start(void)
{
	/* write code here */
	uint32_t base = (uint32_t)((uintptr_t)prod_buff);
	HC_out32(0xF0, base);
	uint32_t counter = 0;
	while (1)
	{
		for (int i = 0; i < 5; i++)
		{
			prod_buff[i] = counter++;
		}
		HC_out32(0xF2, 0);
	}
}

#include <stddef.h>
#include <stdint.h>

struct buffer_state
{
	int prod_p;
	int cons_p;
	int count;
	int buf[20];
};

volatile struct buffer_state *state = (volatile struct buffer_state *)0x400;

static inline uint32_t rdtsc(void)
{
	uint32_t lo, hi;
	asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
	return lo;
}

static inline void HC_in32(uint16_t port, uint32_t *val)
{
	asm volatile("inl %1, %0" : "=a"(*val) : "Nd"(port));
}

static inline void HC_out32(uint16_t port, uint32_t val)
{
	asm volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

void
	__attribute__((noreturn))
	__attribute__((section(".start")))
	_start(void)
{
	while (1)
	{
		uint32_t dummy;
		HC_in32(0xF4, &dummy);
		uint32_t rand_val = rdtsc();
		int produce_count = rand_val % 11;
		int available = 20 - state->count;
		if (produce_count > available)
			produce_count = available;
		for (int i = 0; i < produce_count; i++)
		{
			uint32_t item = rdtsc() % 100;
			state->buf[state->prod_p] = item;
			state->prod_p = (state->prod_p + 1) % 20;
			state->count++;
		}
		HC_out32(0xF2, 0);
	}
}

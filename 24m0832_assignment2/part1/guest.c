#include <stddef.h>
#include <stdint.h>

static void outb(uint16_t port, uint8_t value)
{
	asm("outb %0,%1" : /* empty */ : "a"(value), "Nd"(port) : "memory");
}

void HC_print8bit(uint8_t val)
{
	outb(0xE9, val);
}

void HC_print32bit(uint32_t val)
{
	// val++;
	/* Write code here */
	asm volatile("outl %0,%1" : : "a"(val), "Nd"(0xEA) : "memory");
}

uint32_t HC_numExits()
{
	uint32_t val = 0;
	/* Write code here */
	asm volatile(
        "movl $0, %%eax\n\t"
        "outl %%eax, %1\n\t"
        "inl %1, %%eax\n\t"
        "movl %%eax, %0\n\t"
        : "=r"(val)
        : "Nd"(0xEB)
        : "eax", "memory"
    );
	return val;
}

void HC_printStr(char *str)
{
	// str++;
	/* Write code here */
	asm volatile("outl %0, %w1" : : "a"((uint32_t)(uintptr_t)str), "Nd"(0xEC) : "memory");
}

char *HC_numExitsByType()
{
	/* Write code here */
	// return NULL;
	uint32_t ret;
    asm volatile(
        "movl $0, %%eax\n\t"
        "outl %%eax, %1\n\t" 
        "inl %1, %%eax\n\t"
        "movl %%eax, %0\n\t"
        : "=r"(ret)
        : "Nd"(0xED)
        : "eax", "memory"
    );
    return (char *)(uintptr_t)ret;
}

uint32_t HC_gvaToHva(uint32_t gva)
{
	uint32_t hva = 0;
	/* Write code here */
	asm volatile(
		"movl %1, %%eax\n\t"
		"outl %%eax, %w2\n\t"
        "inl %w2, %%eax\n\t"
        "movl %%eax, %0\n\t"
        : "=r"(hva)
        : "r"(gva), "Nd"(0xEE)
        : "eax", "memory"
	);
	return hva;
}

void
	__attribute__((noreturn))
	__attribute__((section(".start")))
	_start(void)
{
	const char *p;

	for (p = "Hello 695!\n"; *p; ++p)
		HC_print8bit(*p);

	/*----------Don't modify this section. We will use grading script---------*/
	/*---Your submission will fail the testcases if you modify this section---*/
	HC_print32bit(2048);
	HC_print32bit(4294967295);

	uint32_t num_exits_a, num_exits_b;
	num_exits_a = HC_numExits();

	char *str = "CS695 Assignment 2\n";
	HC_printStr(str);

	num_exits_b = HC_numExits();

	HC_print32bit(num_exits_a);
	HC_print32bit(num_exits_b);

	char *firststr = HC_numExitsByType();
	uint32_t hva;
	hva = HC_gvaToHva(1024);
	HC_print32bit(hva);
	hva = HC_gvaToHva(4294967295);
	HC_print32bit(hva);
	char *secondstr = HC_numExitsByType();

	HC_printStr(firststr);
	HC_printStr(secondstr);
	/*------------------------------------------------------------------------*/

	*(long *)0x400 = 42;

	for (;;)
		asm("hlt" : /* empty */ : "a"(42) : "memory");
}

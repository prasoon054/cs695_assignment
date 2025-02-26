#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include <stdint.h>
#include <linux/kvm.h>

/* CR0 bits */
#define CR0_PE 1u
#define CR0_MP (1U << 1)
#define CR0_EM (1U << 2)
#define CR0_TS (1U << 3)
#define CR0_ET (1U << 4)
#define CR0_NE (1U << 5)
#define CR0_WP (1U << 16)
#define CR0_AM (1U << 18)
#define CR0_NW (1U << 29)
#define CR0_CD (1U << 30)
#define CR0_PG (1U << 31)

/* CR4 bits */
#define CR4_VME 1
#define CR4_PVI (1U << 1)
#define CR4_TSD (1U << 2)
#define CR4_DE (1U << 3)
#define CR4_PSE (1U << 4)
#define CR4_PAE (1U << 5)
#define CR4_MCE (1U << 6)
#define CR4_PGE (1U << 7)
#define CR4_PCE (1U << 8)
#define CR4_OSFXSR (1U << 8)
#define CR4_OSXMMEXCPT (1U << 10)
#define CR4_UMIP (1U << 11)
#define CR4_VMXE (1U << 13)
#define CR4_SMXE (1U << 14)
#define CR4_FSGSBASE (1U << 16)
#define CR4_PCIDE (1U << 17)
#define CR4_OSXSAVE (1U << 18)
#define CR4_SMEP (1U << 20)
#define CR4_SMAP (1U << 21)

#define EFER_SCE 1
#define EFER_LME (1U << 8)
#define EFER_LMA (1U << 10)
#define EFER_NXE (1U << 11)

/* 32-bit page directory entry bits */
#define PDE32_PRESENT 1
#define PDE32_RW (1U << 1)
#define PDE32_USER (1U << 2)
#define PDE32_PS (1U << 7)

/* 64-bit page * entry bits */
#define PDE64_PRESENT 1
#define PDE64_RW (1U << 1)
#define PDE64_USER (1U << 2)
#define PDE64_ACCESSED (1U << 5)
#define PDE64_DIRTY (1U << 6)
#define PDE64_PS (1U << 7)
#define PDE64_G (1U << 8)

struct buffer_state
{
	int prod_p;
	int cons_p;
	int count;
	int buff[20];
};

struct vm
{
	int dev_fd;
	int vm_fd;
	char *mem;
};

struct vcpu
{
	int vcpu_fd;
	struct kvm_run *kvm_run;
};

/* Data from sched.txt */
char sched_order[100];

void vm_init(struct vm *vm, size_t mem_size)
{
	int kvm_version;
	struct kvm_userspace_memory_region memreg;

	vm->dev_fd = open("/dev/kvm", O_RDWR);
	if (vm->dev_fd < 0)
	{
		perror("open /dev/kvm");
		exit(1);
	}

	kvm_version = ioctl(vm->dev_fd, KVM_GET_API_VERSION, 0);
	if (kvm_version < 0)
	{
		perror("KVM_GET_API_VERSION");
		exit(1);
	}

	if (kvm_version != KVM_API_VERSION)
	{
		fprintf(stderr, "Got KVM api version %d, expected %d\n",
				kvm_version, KVM_API_VERSION);
		exit(1);
	}

	vm->vm_fd = ioctl(vm->dev_fd, KVM_CREATE_VM, 0);
	if (vm->vm_fd < 0)
	{
		perror("KVM_CREATE_VM");
		exit(1);
	}

	if (ioctl(vm->vm_fd, KVM_SET_TSS_ADDR, 0xfffbd000) < 0)
	{
		perror("KVM_SET_TSS_ADDR");
		exit(1);
	}

	vm->mem = mmap(NULL, mem_size, PROT_READ | PROT_WRITE,
				   MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
	if (vm->mem == MAP_FAILED)
	{
		perror("mmap mem");
		exit(1);
	}

	madvise(vm->mem, mem_size, MADV_MERGEABLE);

	memreg.slot = 0;
	memreg.flags = 0;
	memreg.guest_phys_addr = 0;
	memreg.memory_size = mem_size;
	memreg.userspace_addr = (unsigned long)vm->mem;
	if (ioctl(vm->vm_fd, KVM_SET_USER_MEMORY_REGION, &memreg) < 0)
	{
		perror("KVM_SET_USER_MEMORY_REGION");
		exit(1);
	}
}

void vcpu_init(struct vm *vm, struct vcpu *vcpu)
{
	int vcpu_mmap_size;

	vcpu->vcpu_fd = ioctl(vm->vm_fd, KVM_CREATE_VCPU, 0);
	if (vcpu->vcpu_fd < 0)
	{
		perror("KVM_CREATE_VCPU");
		exit(1);
	}

	vcpu_mmap_size = ioctl(vm->dev_fd, KVM_GET_VCPU_MMAP_SIZE, 0);
	if (vcpu_mmap_size <= 0)
	{
		perror("KVM_GET_VCPU_MMAP_SIZE");
		exit(1);
	}

	vcpu->kvm_run = mmap(NULL, vcpu_mmap_size, PROT_READ | PROT_WRITE,
						 MAP_SHARED, vcpu->vcpu_fd, 0);
	if (vcpu->kvm_run == MAP_FAILED)
	{
		perror("mmap kvm_run");
		exit(1);
	}
}

/*	Modify this function to complete part 2.4 */
/*	sched_order holds the scheduling order for the VMs
	each VM when "scheduled" should read producer-consumer
	buffer state, act on it and convey the new state back
	to the hypervisor.
*/
int run_vm(struct vm *vm1, struct vm *vm2, struct vcpu *vcpu1, struct vcpu *vcpu2, size_t sz)
{
	int ret, i;
	struct buffer_state shared_state;
	shared_state.prod_p = 0;
	shared_state.cons_p = 0;
	shared_state.count = 0;
	memset(shared_state.buff, 0, sizeof(shared_state.buff));
	int sched_len = strlen(sched_order);
	for (i = 0; i < sched_len; i++)
	{
		char sched = sched_order[i];
		if (sched == '1')
		{
			memcpy(vm1->mem + 0x400, &shared_state, sizeof(shared_state));
			while (1)
			{
				ret = ioctl(vcpu1->vcpu_fd, KVM_RUN, 0);
				if (ret < 0)
				{
					perror("KVM_RUN on producer");
					exit(1);
				}
				if (vcpu1->kvm_run->exit_reason == KVM_EXIT_IO && vcpu1->kvm_run->io.direction == KVM_EXIT_IO_OUT && vcpu1->kvm_run->io.port == 0xF2)
				{
					break;
				}
			}
			struct buffer_state new_state;
			memcpy(&new_state, vm1->mem + 0x400, sizeof(new_state));
			int produced = new_state.count - shared_state.count;
			printf("VMFD: %d Produced %d values:", vm1->vm_fd, produced);
			if (produced > 0)
			{
				int start = shared_state.prod_p;
				for (int j = 0; j < produced; j++)
				{
					int index = (start + j) % 20;
					printf(" %d", new_state.buff[index]);
				}
			}
			printf("\n");
			shared_state = new_state;
			printf("HYPVSR: [");
			for (int j = 0; j < shared_state.count; j++)
			{
				int index = (shared_state.cons_p + j) % 20;
				printf("%d ", shared_state.buff[index]);
			}
			printf("]\n");
			memcpy(vm2->mem + 0x400, &shared_state, sizeof(shared_state));
		}
		else if (sched == '2')
		{
			memcpy(vm2->mem + 0x400, &shared_state, sizeof(shared_state));
			while (1)
			{
				ret = ioctl(vcpu2->vcpu_fd, KVM_RUN, 0);
				if (ret < 0)
				{
					perror("KVM_RUN on consumer");
					exit(1);
				}
				if (vcpu2->kvm_run->exit_reason == KVM_EXIT_IO && vcpu2->kvm_run->io.direction == KVM_EXIT_IO_OUT && vcpu2->kvm_run->io.port == 0xF3)
				{
					break;
				}
			}
			struct buffer_state new_state;
			memcpy(&new_state, vm2->mem + 0x400, sizeof(new_state));
			int consumed = shared_state.count - new_state.count;
			printf("VMFD: %d Consumed %d values:", vm2->vm_fd, consumed);
			if (consumed > 0)
			{
				int start = shared_state.cons_p;
				for (int j = 0; j < consumed; j++)
				{
					int index = (start + j) % 20;
					printf(" %d", shared_state.buff[index]);
				}
			}
			printf("\n");
			shared_state = new_state;
			printf("HYPVSR: [");
			for (int j = 0; j < shared_state.count; j++)
			{
				int index = (shared_state.cons_p + j) % 20;
				printf("%d ", shared_state.buff[index]);
			}
			printf("]\n");
			memcpy(vm1->mem + 0x400, &shared_state, sizeof(shared_state));
		}
	}
	return 0;
}

static void setup_protected_mode(struct kvm_sregs *sregs)
{
	struct kvm_segment seg = {
		.base = 0,
		.limit = 0xffffffff,
		.selector = 1 << 3,
		.present = 1,
		.type = 11, /* Code: execute, read, accessed */
		.dpl = 0,
		.db = 1,
		.s = 1, /* Code/data */
		.l = 0,
		.g = 1, /* 4KB granularity */
	};

	sregs->cr0 |= CR0_PE; /* enter protected mode */

	sregs->cs = seg;

	seg.type = 3; /* Data: read/write, accessed */
	seg.selector = 2 << 3;
	sregs->ds = sregs->es = sregs->fs = sregs->gs = sregs->ss = seg;
}

extern const unsigned char guest4a[], guest4a_end[];
extern const unsigned char guest4b[], guest4b_end[];

int run_protected_mode1(struct vm *vm, struct vcpu *vcpu)
{
	struct kvm_sregs sregs;
	struct kvm_regs regs;

	if (ioctl(vcpu->vcpu_fd, KVM_GET_SREGS, &sregs) < 0)
	{
		perror("KVM_GET_SREGS");
		exit(1);
	}

	setup_protected_mode(&sregs);

	if (ioctl(vcpu->vcpu_fd, KVM_SET_SREGS, &sregs) < 0)
	{
		perror("KVM_SET_SREGS");
		exit(1);
	}

	memset(&regs, 0, sizeof(regs));
	/* Clear all FLAGS bits, except bit 1 which is always set. */
	regs.rflags = 2;
	regs.rip = 0;
	regs.rsp = 2 << 20;

	if (ioctl(vcpu->vcpu_fd, KVM_SET_REGS, &regs) < 0)
	{
		perror("KVM_SET_REGS");
		exit(1);
	}

	memcpy(vm->mem, guest4a, guest4a_end - guest4a);
	printf("VMFD: %d Loaded Program with size: %ld\n", vm->vm_fd, guest4a_end - guest4a);
	return 0;
}

int run_protected_mode2(struct vm *vm, struct vcpu *vcpu)
{
	struct kvm_sregs sregs;
	struct kvm_regs regs;

	if (ioctl(vcpu->vcpu_fd, KVM_GET_SREGS, &sregs) < 0)
	{
		perror("KVM_GET_SREGS");
		exit(1);
	}

	setup_protected_mode(&sregs);

	if (ioctl(vcpu->vcpu_fd, KVM_SET_SREGS, &sregs) < 0)
	{
		perror("KVM_SET_SREGS");
		exit(1);
	}

	memset(&regs, 0, sizeof(regs));
	/* Clear all FLAGS bits, except bit 1 which is always set. */
	regs.rflags = 2;
	regs.rip = 0;
	regs.rsp = 2 << 20;

	if (ioctl(vcpu->vcpu_fd, KVM_SET_REGS, &regs) < 0)
	{
		perror("KVM_SET_REGS");
		exit(1);
	}

	memcpy(vm->mem, guest4b, guest4b_end - guest4b);
	printf("VMFD: %d Loaded Program with size: %ld\n", vm->vm_fd, guest4b_end - guest4b);
	return 0;
}

void read_sched_file(char *filename)
{
	int fd = open(filename, O_RDONLY);
	if (fd < 0)
	{
		fprintf(stderr, "Error in opening file");
		exit(EXIT_FAILURE);
	}
	int rc = read(fd, sched_order, 100);
	if (rc < 0)
	{
		fprintf(stderr, "Error in opening file");
		exit(EXIT_FAILURE);
	}
}

int main(int argc, char **argv)
{
	struct vm vm1, vm2;
	struct vcpu vcpu1, vcpu2;

	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s filename\n",
				argv[0]);
		return 1;
	}
	read_sched_file(argv[1]);

	vm_init(&vm1, 0x200000);
	vm_init(&vm2, 0x200000);
	vcpu_init(&vm1, &vcpu1);
	vcpu_init(&vm2, &vcpu2);
	run_protected_mode1(&vm1, &vcpu1);
	run_protected_mode2(&vm2, &vcpu2);
	return run_vm(&vm1, &vm2, &vcpu1, &vcpu2, 4);
}

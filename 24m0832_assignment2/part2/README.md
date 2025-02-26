# CS695 Assignment Part 2

Implemented a series of virtualization assignments where two virtual machines (VMs) communicate in a producer–consumer fashion. The hypervisor uses KVM and a single physical CPU to run both VMs. Each successive problem builds on the previous one, increasing in complexity.

---

## Overview

In all four problems the following ideas are used:
- **Two VMs:**  
  - **Producer VM:** Generates data (numbers or arrays) by executing guest code.
  - **Consumer VM:** Reads and “consumes” the data produced.
- **Hypervisor:**  
  - Uses KVM (via `/dev/kvm`) to create and manage VMs.
  - Runs both VMs on a single CPU by alternately calling `KVM_RUN` on each vCPU.
  - Mediates communication between the VMs by intercepting hypercalls (via I/O exits on specific port numbers) and transferring state.
- **Guest Communication:**  
  - Hypercalls are implemented with `in` and `out` instructions on specified I/O ports.
  - The guest programs trap using these hypercalls and the hypervisor inspects the exit reason and port to know what action to perform.

---

## Problem 1: Basic Producer–Consumer

**Description:**  
- The Producer (guest1a.s) issues an OUT on port `0x10` with a number (then increments it).  
- The Consumer (guest1b.s) issues an IN on port `0x11` to read the number and then an OUT on port `0x12` to output the consumed value.  
- The hypervisor (`emu1.c`) avoids using extra pthreads; instead, the main thread alternates between the VMs on a KVM_EXIT_IO event.

**Key Points:**  
- The hypervisor’s `kvm_run_vm()` alternates execution of the two VMs.
- A shared variable (the produced number) is passed from the producer to the consumer via the hypervisor.
- Output demonstrates that numbers produced by VM1 are consumed by VM2.

---

## Problem 2: Bursty Scheduling

**Description:**  
- The Producer now produces **three** numbers in succession before the Consumer is scheduled to run.  
- The guest programs remain similar to Problem 1, but the hypervisor (`emu2.c`) is modified to alternate in bursts.

**Key Points:**  
- The hypervisor schedules the Producer for three iterations (producing three numbers) then schedules the Consumer to consume them.
- The code ensures that the output order matches the expected bursty production/consumption pattern.

---

## Problem 3: Mass Production (Array Transfer)

**Description:**  
- The Producer (guest3a.c) produces an array of **five** items (without trapping on each item).  
- After production, the Producer traps (via a hypercall) and passes the base address of the array to the hypervisor.  
- The hypervisor (`emu3.c`) copies the buffer contents to the Consumer’s memory, and the Consumer (guest3b.c) then consumes and outputs the values.

**Key Points:**  
- Both VMs now run in protected mode.
- The Producer writes an array of five values, and the Consumer reads that array.
- The hypervisor is responsible for copying the produced array from the Producer’s memory to the Consumer’s memory.
- This solution demonstrates bulk data transfer between VMs.

---

## Problem 4: Non-Deterministic Producer–Consumer with a Cyclic Buffer

**Description:**  
- The Producer and Consumer now produce and consume a **random number** (0–10) of items on each scheduling event.
- A **cyclic (ring) buffer** of fixed size (20 elements) is maintained by the hypervisor.
- Each VM maintains a local copy of the buffer state that includes:
  - `prod_p`: The index where the next produced element will be stored.
  - `cons_p`: The index of the next element to be consumed.
  - `count`: The current number of elements in the buffer.
- A scheduling file (e.g., `sched2.txt`) containing a sequence of `1`s and `2`s is used by the hypervisor (`emu4.c`) to decide which VM’s turn it is.
- The Producer (guest4a.c) reads the current state (via an in hypercall on port `0xF4`), produces a random number of items (limited by the available space), updates the cyclic pointers using modulo arithmetic (e.g., `prod_p = (prod_p + 1) % 20`), and signals completion using an out hypercall on port `0xF2`.
- The Consumer (guest4b.c) similarly reads the state, consumes a random number of items (without underflowing), updates its pointer (`cons_p = (cons_p + 1) % 20`), and signals completion using an out hypercall on port `0xF3`.
- The hypervisor synchronizes state between the two VMs and prints the produced/consumed values along with the current state of the cyclic buffer.

**Key Points:**  
- The buffer state is now a cyclic (ring) buffer with three fields: `prod_p`, `cons_p`, and `count`.
- The hypervisor’s scheduling loop (in `emu4.c`) copies the state into the guest memory (at guest physical address `0x400`), runs the corresponding VM until it issues the expected hypercall, and then reads back the updated state.
- The produced numbers are kept small (using `rdtsc() % 100`), and the random production/consumption counts are in the range [0, 10].
- This solution has been debugged to correctly handle wrap-around and to compute the number of items produced or consumed based on the cyclic pointers and the count.

---

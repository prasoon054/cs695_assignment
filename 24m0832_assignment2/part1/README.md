# CS695 Assignment Part 1

This part of the assignment demonstrates how a guest operating system can communicate with a hypervisor using hardware-assisted hypercalls. Since a Guest OS cannot perform I/O operations directly, it issues specific I/O instructions (using `in`/`out`) to exit to the hypervisor. The hypervisor inspects the exit reason, extracts data from the guest’s registers and memory, and then handles the request accordingly.

The provided boilerplate code already implements a simple hypercall, `HC_print8bit()`, which writes a single byte to the serial port at I/O port `0xE9`.

This solution extends that mechanism to implement the following hypercalls:

## Hypercall Implementations

1. **`void HC_print32bit(uint32_t val)`**  
   - **Guest Side:**  
     The guest calls `HC_print32bit(val)` which uses an `outl` instruction to write the 32-bit value to I/O port `0xEA`.
   - **Hypervisor Side:**  
     In the hypervisor (in `simple-kvm.c`), when an exit occurs due to an I/O out instruction on port `0xEA`, the hypervisor retrieves the 32-bit value from the guest’s I/O buffer and prints it to the terminal. A newline character is added after printing.

2. **`uint32_t HC_numExits()`**  
   - **Guest Side:**  
     The guest calls `HC_numExits()`, which uses an `outl`/`inl` sequence to invoke the hypercall.
   - **Hypervisor Side:**  
     The hypervisor maintains a global counter for the number of times the guest VM has exited (for example, for any I/O exit). When a hypercall is issued on port `0xEB`, the hypervisor returns the current exit count. The guest can then print the value by calling `HC_print32bit()`.

3. **`void HC_printStr(char *str)`**  
   - **Guest Side:**  
     Instead of issuing multiple I/O exits for each character, the guest passes the address of the string to the hypervisor by issuing a single out instruction (to port `0xEC`).
   - **Hypervisor Side:**  
     The hypervisor fetches the string from the guest’s memory (using the pointer provided) and prints it exactly as it is (preserving any newline characters). In addition, the hypervisor uses `HC_numExits()` before and after to ensure that the exit count increases by exactly one for this hypercall.

4. **`char* HC_numExitsByType()`**  
   - **Guest Side:**  
     The guest invokes this hypercall to obtain a pointer to a string that describes the number of I/O exits of different types.
   - **Hypervisor Side:**  
     When a hypercall is issued on port `0xED`, the hypervisor formats a string in the following format:
     ```
     IO in: x
     IO out: y
     ```
     where `x` and `y` are the actual counts of exits caused by I/O in and I/O out operations, respectively. The hypervisor writes this string into guest memory and returns the guest virtual address of the string.

5. **`uint32_t HC_gvaToHva(uint32_t gva)`**  
   - **Guest Side:**  
     The guest calls this hypercall to translate a Guest Virtual Address (GVA) into a Host Virtual Address (HVA). Two examples are demonstrated: one valid and one invalid GVA.
   - **Hypervisor Side:**  
     On receiving an out instruction on port `0xEE`, the hypervisor uses the KVM_TRANSLATE mechanism (via an `ioctl` on the vmfd) to translate the GVA into an HVA. Since only 2 MB of memory is mapped, the HVA is truncated to 32 bits. If the provided GVA is invalid (i.e. it does not fall within the guest’s mapped memory), the hypervisor prints `"Invalid GVA"` and returns 0.

## How the System Works

- **Guest Code (`guest.c`):**  
  The guest code first prints a welcome message using `HC_print8bit()`. It then calls the new hypercalls in sequence:
  - `HC_print32bit()` is called to print two test values.
  - `HC_numExits()` is invoked, and its result is printed using `HC_print32bit()`.
  - `HC_printStr()` is called to print a test string.
  - `HC_numExits()` is called again to show that the exit count increased by one.
  - `HC_numExitsByType()` is called to obtain a formatted string containing the I/O exit counts; this string is printed using `HC_printStr()`.
  - Finally, `HC_gvaToHva()` is called twice—once with a valid GVA and once with an invalid GVA—and the results are printed with `HC_print32bit()`.

- **Hypervisor Code (`simple-kvm.c`):**  
  The hypervisor is built on top of the Linux KVM API. It creates a VM and a vCPU, sets up guest memory, and enters a loop that calls `KVM_RUN()`. When the guest issues an I/O instruction (triggering a hypercall), the hypervisor examines the exit reason and the I/O port:
  - For port `0xEA`, the hypervisor prints a 32-bit value (with a newline).
  - For port `0xEB`, it returns the global exit count.
  - For port `0xEC`, it fetches and prints a string from guest memory.
  - For port `0xED`, it formats and returns the string showing the number of I/O in and I/O out exits.
  - For port `0xEE`, it translates a GVA to an HVA (or prints an error for an invalid GVA) and returns the result to the guest by updating the guest’s register.
  - Global counters (`global_exit_count`, `io_in_count`, `io_out_count`) are maintained to track exit statistics.

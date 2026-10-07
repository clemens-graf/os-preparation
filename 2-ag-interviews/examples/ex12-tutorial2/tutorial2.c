#include <stdio.h>
#include <nonstd.h>

int global;

int main()
{
    int stack = 1;
    global = 2;

    // get the ppns (physical page numbers) of two virtual addresses
    size_t physical_global = get_physical_address((size_t)&global);
    size_t physical_stack = get_physical_address((size_t)&stack);

    printf("global: %lu, stack: %lu\n", physical_global, physical_stack);

    int *address = (int*)0x424242424242;

    // map the virtual page of an arbitrary virtual adress to the given physical page
    map_address((size_t)address, physical_global);

    *address = 1;

    printf("at address: %d\n", *address);

    // get the vpn (virtual page number) of the address
    // by ignoring the page offset (last 12 bits) and shifting
    size_t vpn = (size_t)address >> 12;
    // the offset of the global variable on its page
    // is the last 12 bits of the address (same for physical and virtual!)
    size_t global_var_offset = (size_t)&global & 0xfff;

    // the same physical memory is now mapped at `&global` and `global_var_in_new_mapping`
    int *global_var_in_new_mapping = (int*)((vpn << 12) + global_var_offset);

    printf("at global: %d\n", global);

    *global_var_in_new_mapping = 3;

    printf("at global: %d\n", global);

    // the kernel crashes because we mapped the same physical page twice:
    // when trying to free all physical pages of the process after exiting,
    // the double free is detected and causes a kernel panic
}

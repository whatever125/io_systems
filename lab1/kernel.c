#include "kernel.h"
#include "common.h"

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef uint32_t size_t;

extern char __bss[], __bss_end[], __stack_top[];

struct sbiret sbi_call(long arg0, long arg1, long arg2, long arg3, long arg4,
                       long arg5, long fid, long eid) {
    register long a0 __asm__("a0") = arg0;
    register long a1 __asm__("a1") = arg1;
    register long a2 __asm__("a2") = arg2;
    register long a3 __asm__("a3") = arg3;
    register long a4 __asm__("a4") = arg4;
    register long a5 __asm__("a5") = arg5;
    register long a6 __asm__("a6") = fid;
    register long a7 __asm__("a7") = eid;

    __asm__ __volatile__("ecall"
                         : "=r"(a0), "=r"(a1)
                         : "r"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5),
                           "r"(a6), "r"(a7)
                         : "memory");
    return (struct sbiret){.error = a0, .value = a1};
}

void putchar(char ch) {
    sbi_call(ch, 0, 0, 0, 0, 0, 0, 1 /* Console Putchar */);
}

int getchar(void) {
    return (int) sbi_call(0, 0, 0, 0, 0, 0, 0, 2 /* Console Getchar */).error;
}

void *memset(void *buf, char c, size_t n) {
    uint8_t *p = (uint8_t *) buf;
    while (n--)
        *p++ = c;
    return buf;
}

void kernel_main(void) {
    printf("\n\n▓█████▄  ███▄    █  ▄▄▄▄   \n▒██▀ ██▌ ██ ▀█   █ ▓█████▄ \n░██   █▌▓██  ▀█ ██▒▒██▒ ▄██\n░▓█▄   ▌▓██▒  ▐▌██▒▒██░█▀  \n░▒████▓ ▒██░   ▓██░░▓█  ▀█▓\n ▒▒▓  ▒ ░ ▒░   ▒ ▒ ░▒▓███▀▒\n ░ ▒  ▒ ░ ░░   ░ ▒░▒░▒   ░ \n ░ ░  ░    ░   ░ ░  ░    ░ \n   ░             ░  ░      \n ░                       ░ \n\n\n");
    printf("Welcome to DnB!\n\n");
    printf("Choose action:\n");
    printf("h. Get help\n");
    printf("1. Get SBI implementation version\n");
    printf("2. Hart get status\n");
    printf("3. Hart stop\n");
    printf("4. System Shutdown\n");

    int input;
    for (;;) {
        printf("$ ");
        while ((input = getchar()) < 0) {};
        putchar(input);
        putchar('\n');

        if (input == 'h') {
            printf("\nChoose action:\n");
            printf("h. Get help\n");
            printf("1. Get SBI implementation version\n");
            printf("2. Hart get status\n");
            printf("3. Hart stop\n");
            printf("4. System Shutdown");
        } else if (input == '1') {
            struct sbiret result = sbi_call(0, 0, 0, 0, 0, 0, 2, 0x10 /* Get SBI implementation version */);
            int major = (result.value >> 16) & 0xFFFF;
            int minor = result.value & 0xFFFF;
            printf("SBI implementation version: %d.%d", major, minor);
        } else if (input == '2') {
            printf("Choose hart ID:\n");
            printf("  > ");
            while ((input = getchar()) < 0) {};
            putchar(input);
            input -= 48;
            struct sbiret result = sbi_call(input, 0, 0, 0, 0, 0, 2, 0x48534D /* Hart get status */);
            if (result.error < 0) {
                printf("\nERROR: The given hartid is not valid");
            } else {
                printf("\nHart %d status: ", input);
                switch (result.value) {
                    case 0: { printf("STARTED"); break; }
                    case 1: { printf("STOPPED"); break; }
                    case 2: { printf("START_PENDING"); break; }
                    case 3: { printf("STOP_PENDING"); break; }
                    case 4: { printf("SUSPENDED"); break; }
                    case 5: { printf("SUSPEND_PENDING"); break; }
                    case 6: { printf("RESUME_PENDING"); break; }
                }
            }
        } else if (input == '3') {
            printf("Trying to stop execution of the current hart...\n");
            struct sbiret result = sbi_call(input, 0, 0, 0, 0, 0, 1, 0x48534D /* Hart stop */);
            if (result.error < 0) {
                printf("\nERROR: Failed to stop execution of the current hart");
            }
        } else if (input == '4') {
            printf("Putting all the harts to shutdown state...\n");
            sbi_call(input, 0, 0, 0, 0, 0, 0, 0x08 /* System Shutdown */);
        } else {
            printf("ERROR: Invalid input");
        }
        printf("\n\n");
    }
}

__attribute__((section(".text.boot")))
__attribute__((naked))
void boot(void) {
    __asm__ __volatile__(
        "mv sp, %[stack_top]\n" // Set the stack pointer
        "j kernel_main\n"       // Jump to the kernel main function
        :
        : [stack_top] "r" (__stack_top) // Pass the stack top address as %[stack_top]
    );
}

# hello.s
# A program that prints "Hello, world!" to the screen.

.section .data
    msg:
        .ascii "Hello, world!\n"
    len = . - msg

.section .text
.global _start

_start:
    # write(1, msg, len)
    # This block tells the computer to write our message to the screen.
    movq $1, %rax        # System call number 1 is 'write'.
    movq $1, %rdi        # File descriptor 1 is 'stdout' (your screen).
    leaq msg(%rip), %rsi # Load the memory address of our message.
    movq $len, %rdx      # Set the length of our message.
    syscall              # Execute the 'write' system call.

    # exit(0)
    # This block tells the program to exit cleanly.
    movq $60, %rax       # System call number 60 is 'exit'.
    xorq %rdi, %rdi      # Set the exit code to 0 (which means success).
    syscall              # Execute the 'exit' system call.
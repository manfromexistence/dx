.section .data
    msg: .ascii "Hello, world!\n"
    len = . - msg

    dir_name: .asciz "assembly_modules"
    file_prefix: .asciz "assembly_modules/file_"
    file_suffix: .asciz ".txt"

    time_msg: .ascii "Time taken: "
    time_msg_len = . - time_msg
    seconds_label: .ascii " seconds.\n"
    seconds_label_len = . - seconds_label
    dot: .ascii "."
    
    SYS_WRITE:      .quad 1
    SYS_OPEN:       .quad 2
    SYS_CLOSE:      .quad 3
    SYS_EXIT:       .quad 60
    SYS_MKDIR:      .quad 83
    SYS_GETTIME:    .quad 228

    FILE_COUNT:     .quad 1000
    DIR_MODE:       .quad 0755
    FILE_MODE:      .quad 0644
    O_WRONLY_CREAT: .quad 0101

.section .bss
    .lcomm start_time, 16
    .lcomm end_time, 16
    .lcomm file_path, 256
    .lcomm num_buffer, 20

.section .text
.global _start

_start:
    movq $SYS_GETTIME, %rax
    movq $1, %rdi
    leaq start_time(%rip), %rsi
    syscall

    movq $SYS_MKDIR, %rax
    leaq dir_name(%rip), %rdi
    movq $DIR_MODE, %rsi
    syscall

    movq $FILE_COUNT, %rcx

file_loop:
    leaq file_prefix(%rip), %rsi
    leaq file_path(%rip), %rdi
    call _strcpy

    pushq %rcx
    movq %rcx, %rax
    leaq num_buffer(%rip), %rdi
    call _itoa
    popq %rcx

    leaq num_buffer(%rip), %rsi
    leaq file_path(%rip), %rdi
    call _strcat

    leaq file_suffix(%rip), %rsi
    leaq file_path(%rip), %rdi
    call _strcat

    movq $SYS_OPEN, %rax
    leaq file_path(%rip), %rdi
    movq $O_WRONLY_CREAT, %rsi
    movq $FILE_MODE, %rdx
    syscall
    
    movq %rax, %r12

    movq $SYS_WRITE, %rax
    movq %r12, %rdi
    leaq msg(%rip), %rsi
    movq $len, %rdx
    syscall

    movq $SYS_CLOSE, %rax
    movq %r12, %rdi
    syscall

    decq %rcx
    jnz file_loop

    movq $SYS_GETTIME, %rax
    movq $1, %rdi
    leaq end_time(%rip), %rsi
    syscall

    movq end_time+8(%rip), %rax
    subq start_time+8(%rip), %rax

    movq end_time(%rip), %rdx
    subq start_time(%rip), %rdx

    cmpq $0, %rax
    jge no_borrow
    addq $1000000000, %rax
    decq %rdx
no_borrow:
    movq %rdx, %r12
    movq %rax, %r13

    movq $SYS_WRITE, %rax
    movq $1, %rdi
    leaq time_msg(%rip), %rsi
    movq $time_msg_len, %rdx
    syscall

    movq %r12, %rax
    leaq num_buffer(%rip), %rdi
    call _itoa
    call _print_string

    movq $SYS_WRITE, %rax
    movq $1, %rdi
    leaq dot(%rip), %rsi
    movq $1, %rdx
    syscall
    
    movq %r13, %rax
    leaq num_buffer(%rip), %rdi
    call _itoa
    call _print_string

    movq $SYS_WRITE, %rax
    movq $1, %rdi
    leaq seconds_label(%rip), %rsi
    movq $seconds_label_len, %rdx
    syscall

    movq $SYS_EXIT, %rax
    xorq %rdi, %rdi
    syscall

_strcpy:
    movsb
    cmpb $0, -1(%rdi)
    jne _strcpy
    ret

_strcat:
    movq %rdi, %rax
find_end:
    cmpb $0, (%rax)
    je found_end
    incq %rax
    jmp find_end
found_end:
    movq %rax, %rdi
    call _strcpy
    ret

_itoa:
    movq %rdi, %r8
    movq $10, %r9
    movq $0, %rcx
    
    cmpq $0, %rax
    jne conversion_loop
    movb $'0', (%r8)
    incq %r8
    movb $0, (%r8)
    ret

conversion_loop:
    xorq %rdx, %rdx
    divq %r9
    addb $'0', %dl
    pushq %rdx
    incq %rcx
    cmpq $0, %rax
    jne conversion_loop

reverse_loop:
    popq %rax
    movb %al, (%r8)
    incq %r8
    decq %rcx
    jnz reverse_loop

    movb $0, (%r8)
    ret

_print_string:
    pushq %rdi
    movq %rdi, %rsi
    call _strlen
    movq %rax, %rdx
    popq %rsi
    movq $SYS_WRITE, %rax
    movq $1, %rdi
    syscall
    ret

_strlen:
    movq %rsi, %rax
strlen_loop:
    cmpb $0, (%rax)
    je strlen_end
    incq %rax
    jmp strlen_loop
strlen_end:
    subq %rsi, %rax
    ret

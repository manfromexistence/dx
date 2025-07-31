.section .note.GNU-stack,"",@progbits

.section .data
folder: .string "modules/"
file_prefix: .string "modules/file"
file_suffix: .string ".txt"
content: .string "hello world\n"
content_len: .quad . - content
fmt_debug: .string "Writing %d files at file %d\n"
fmt_time: .string "Time taken: %.2f ms\n"
fmt_error: .string "Error: %s\n"
fmt_open_error: .string "Error opening %s: %s\n" # New format string for open errors
mkdir_error: .string "Error creating directory"
stat_error: .string "Error checking directory"
        .equ num_files, 1000
        .equ batch_size, 32
const_1e6: .double 1000000.0
const_1000: .double 1000.0

        .section .bss
        .lcomm filename, 256
        .lcomm iovecs, 32 * 16
        .lcomm fds, 32 * 4
        .lcomm ts_start, 16
        .lcomm ts_end, 16
        .lcomm st, 144

        .section .text
        .globl main
        .extern printf
        .extern strerror

main:
        pushq %rbp
        movq %rsp, %rbp
        subq $32, %rsp

        # Get start time (clock_gettime, CLOCK_MONOTONIC)
        movq $228, %rax
        movq $1, %rdi
        leaq ts_start(%rip), %rsi
        syscall
        cmpq $0, %rax
        jne error_exit

        # Check if folder exists (stat)
        movq $4, %rax
        leaq folder(%rip), %rdi
        leaq st(%rip), %rsi
        syscall
        cmpq $0, %rax
        je folder_exists

        # Handle ENOENT (folder doesn't exist)
        cmpq $-2, %rax
        jne stat_error_exit

        # Create folder (mkdir)
        movq $83, %rax
        leaq folder(%rip), %rdi
        movq $0755, %rsi
        syscall
        cmpq $0, %rax
        jne mkdir_error_exit

folder_exists:
        xorq %r12, %r12
        xorq %r13, %r13

file_loop:
        cmpq $num_files, %r12
        jge end_loop

        # Build the filename string
        leaq filename(%rip), %rdi
        leaq file_prefix(%rip), %rsi
        call strcpy
        movq %r12, %rax
        leaq filename(%rip), %rsi
        addq $12, %rsi # FIX: Correct offset for "modules/file" (12 chars)
        call itoa
        leaq file_suffix(%rip), %rsi
        call strcat

        # Open the file
        movq $2, %rax
        leaq filename(%rip), %rdi
        movq $577, %rsi # O_CREAT | O_WRONLY | O_TRUNC
        movq $0644, %rdx
        syscall
        cmpq $0, %rax
        jl open_error
        
        # Store the file descriptor
        leaq fds(%rip), %rbx
        movl %eax, (%rbx,%r13,4)
        incq %r13

        # Prepare the iovec structure for this file
        movq %r13, %rax
        decq %rax
        shlq $4, %rax
        leaq iovecs(%rip), %rbx
        addq %rax, %rbx
        leaq content(%rip), %rcx
        movq %rcx, (%rbx)
        movq content_len(%rip), %rcx
        movq %rcx, 8(%rbx)

        # If batch is full or we are at the last file, write the batch
        cmpq $batch_size, %r13
        je write_batch
        cmpq $(num_files - 1), %r12
        je write_batch

        # Continue to next file
        incq %r12
        jmp file_loop

write_batch:
        leaq fmt_debug(%rip), %rdi
        movq %r13, %rsi
        movq %r12, %rdx
        xorq %rax, %rax
        call printf

        movq %r13, %r14 # Save batch size
        xorq %r15, %r15 # Loop counter for batch
write_loop:
        cmpq %r14, %r15
        jge end_write
        movq $20, %rax # syscall: writev
        leaq fds(%rip), %rbx
        movl (%rbx,%r15,4), %edi
        leaq iovecs(%rip), %rsi
        movq %r15, %rax
        shlq $4, %rax
        addq %rax, %rsi
        movq $1, %rdx
        syscall
        cmpq $0, %rax
        jl write_error # Jump to error handler on failure
        incq %r15
        jmp write_loop

end_write:
        xorq %r15, %r15
close_loop:
        cmpq %r14, %r15
        jge end_close
        movq $3, %rax # syscall: close
        leaq fds(%rip), %rbx
        movl (%rbx,%r15,4), %edi
        syscall
        incq %r15
        jmp close_loop

end_close:
        xorq %r13, %r13 # Reset batch counter
        incq %r12
        jmp file_loop

end_loop:
        # Get end time
        movq $228, %rax
        movq $1, %rdi
        leaq ts_end(%rip), %rsi
        syscall
        cmpq $0, %rax
        jne error_exit

        # FIX: Correctly calculate elapsed time in milliseconds, handling nanosecond borrow
        movq    ts_end+8(%rip), %rax    # rax = end.tv_nsec
        subq    ts_start+8(%rip), %rax  # rax = nsec_diff
        movq    ts_end(%rip), %rcx      # rcx = end.tv_sec
        subq    ts_start(%rip), %rcx    # rcx = sec_diff

        # Handle nanosecond borrow if nsec_diff is negative
        cmpq    $0, %rax
        jge     no_borrow
        addq    $1000000000, %rax       # Add 1 billion nanoseconds
        subq    $1, %rcx                # Subtract 1 second
no_borrow:
        # Convert seconds to milliseconds
        cvtsi2sdq %rcx, %xmm0
        mulsd   const_1000(%rip), %xmm0

        # Convert nanoseconds to milliseconds
        cvtsi2sdq %rax, %xmm1
        divsd   const_1e6(%rip), %xmm1

        # Add them up for total milliseconds
        addsd   %xmm1, %xmm0

        leaq fmt_time(%rip), %rdi
        movq $1, %rax
        call printf

        movq %rbp, %rsp
        popq %rbp
        xorq %rax, %rax
        ret

# --- Error Handlers ---

open_error:
    # FIX: Provide a detailed error message for open failures
    negq %rax                   # rax = -errno -> errno
    movq %rax, %rdi             # rdi = errno for strerror
    call strerror
    movq %rax, %rdx             # rdx = error string for printf
    leaq fmt_open_error(%rip), %rdi # rdi = "Error opening %s: %s\n"
    leaq filename(%rip), %rsi   # rsi = filename
    xorq %rax, %rax
    call printf
    incq %r12
    jmp file_loop

write_error:
    # FIX: Correctly get and print the error string from the OS
    negq %rax                   # rax = -errno -> errno
    movq %rax, %rdi             # rdi = errno for strerror
    call strerror
    movq %rax, %rsi             # rsi = error string for printf
    leaq fmt_error(%rip), %rdi  # rdi = "Error: %s\n"
    xorq %rax, %rax
    call printf
    incq %r15
    jmp write_loop

mkdir_error_exit:
        leaq fmt_error(%rip), %rdi
        leaq mkdir_error(%rip), %rsi
        xorq %rax, %rax
        call printf
        movq $1, %rax
        jmp exit

stat_error_exit:
        leaq fmt_error(%rip), %rdi
        leaq stat_error(%rip), %rsi
        xorq %rax, %rax
        call printf
        movq $1, %rax
        jmp exit

error_exit:
    # FIX: Correctly handle generic syscall errors
    negq %rax
    movq %rax, %rdi
    call strerror
    movq %rax, %rsi
    leaq fmt_error(%rip), %rdi
    xorq %rax, %rax
    call printf
    movq $1, %rax

exit:
        movq %rbp, %rsp
        popq %rbp
        ret

# --- String Utilities (unchanged) ---

strcpy:
        pushq %rbx
        movq %rdi, %rbx
strcpy_loop:
        movb (%rsi), %al
        movb %al, (%rbx)
        testb %al, %al
        je strcpy_end
        incq %rsi
        incq %rbx
        jmp strcpy_loop
strcpy_end:
        popq %rbx
        ret

strcat:
        pushq %rbx
        movq %rdi, %rbx
strcat_find_end:
        movb (%rbx), %al
        testb %al, %al
        je strcat_copy
        incq %rbx
        jmp strcat_find_end
strcat_copy:
        movb (%rsi), %al
        movb %al, (%rbx)
        testb %al, %al
        je strcat_end
        incq %rsi
        incq %rbx
        jmp strcat_copy
strcat_end:
        popq %rbx
        ret

itoa:
        pushq %rbx
        pushq %r12
        movq %rsi, %r12
        movq $10, %rbx
        leaq 20(%rsi), %rcx
        movb $0, (%rcx)
        decq %rcx
itoa_loop:
        xorq %rdx, %rdx
        divq %rbx
        addb $'0', %dl
        movb %dl, (%rcx)
        decq %rcx
        testq %rax, %rax
        jnz itoa_loop
        incq %rcx
        movq %r12, %rdi
        movq %rcx, %rsi
        call strcpy
        popq %r12
        popq %rbx
        ret

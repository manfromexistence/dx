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
        .lcomm errno_str, 256
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

        leaq filename(%rip), %rdi
        leaq file_prefix(%rip), %rsi
        call strcpy
        movq %r12, %rax
        leaq filename(%rip), %rsi
        addq $11, %rsi
        call itoa
        leaq file_suffix(%rip), %rsi
        call strcat

        movq $2, %rax
        leaq filename(%rip), %rdi
        movq $577, %rsi
        movq $0644, %rdx
        syscall
        cmpq $0, %rax
        jl open_error
        
        # FIX: Load address of fds into rbx, then use it as a base.
        leaq fds(%rip), %rbx
        movl %eax, (%rbx,%r13,4)
        incq %r13

        movq %r13, %rax
        decq %rax
        shlq $4, %rax
        leaq iovecs(%rip), %rbx
        addq %rax, %rbx
        leaq content(%rip), %rcx
        movq %rcx, (%rbx)
        movq content_len(%rip), %rcx
        movq %rcx, 8(%rbx)

        cmpq $batch_size, %r13
        je write_batch
        cmpq $(num_files - 1), %r12
        je write_batch
        incq %r12
        jmp file_loop

write_batch:
        leaq fmt_debug(%rip), %rdi
        movq %r13, %rsi
        movq %r12, %rdx
        xorq %rax, %rax
        call printf

        movq %r13, %r14
        xorq %r15, %r15
write_loop:
        cmpq %r14, %r15
        jge end_write
        movq $20, %rax
        
        # FIX: Load address of fds into rbx, then use it as a base.
        leaq fds(%rip), %rbx
        movl (%rbx,%r15,4), %edi
        
        leaq iovecs(%rip), %rsi
        movq %r15, %rax
        shlq $4, %rax
        addq %rax, %rsi
        movq $1, %rdx
        syscall
        cmpq $0, %rax
        jl write_error
        incq %r15
        jmp write_loop

end_write:
        xorq %r15, %r15
close_loop:
        cmpq %r14, %r15
        jge end_close
        movq $3, %rax
        
        # FIX: Load address of fds into rbx, then use it as a base.
        leaq fds(%rip), %rbx
        movl (%rbx,%r15,4), %edi
        
        syscall
        incq %r15
        jmp close_loop

end_close:
        xorq %r13, %r13
        incq %r12
        jmp file_loop

end_loop:
        movq $228, %rax
        movq $1, %rdi
        leaq ts_end(%rip), %rsi
        syscall
        cmpq $0, %rax
        jne error_exit

        movq ts_end(%rip), %rax
        subq ts_start(%rip), %rax
        cvtsi2sdq %rax, %xmm0
        movq ts_end+8(%rip), %rax
        subq ts_start+8(%rip), %rax
        cvtsi2sdq %rax, %xmm1
        divsd const_1e6(%rip), %xmm1
        addsd %xmm1, %xmm0
        mulsd const_1000(%rip), %xmm0

        leaq fmt_time(%rip), %rdi
        movq $1, %rax
        call printf

        movq %rbp, %rsp
        popq %rbp
        xorq %rax, %rax
        ret

open_error:
        leaq fmt_error(%rip), %rdi
        leaq filename(%rip), %rsi
        xorq %rax, %rax
        call printf
        incq %r12
        jmp file_loop

write_error:
        leaq fmt_error(%rip), %rdi
        leaq errno_str(%rip), %rsi
        call strerror
        movq %rax, %rsi
        leaq fmt_error(%rip), %rdi
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
        leaq fmt_error(%rip), %rdi
        leaq errno_str(%rip), %rsi
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

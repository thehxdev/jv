    .file "x86_64_jv.s"
    .section .text

    .align 8
    .global jv_task_store
    .type   jv_task_store, @function
    # void jv_context_store(jv_context_t *ctx %rdi, void *rsp %rsi, void *rip %rdx)
jv_task_store:
    # save callee saved registers, stack pointer and return addess to context
    movq    %rbx, 0x00(%rdi)
    movq    %rbp, 0x08(%rdi)
    movq    %r12, 0x10(%rdi)
    movq    %r13, 0x18(%rdi)
    movq    %r14, 0x20(%rdi)
    movq    %r15, 0x28(%rdi)
    movq    %rsi, 0x30(%rdi)
    movq    %rdx, 0x38(%rdi)
    retq


    .align 8
    .global jv_task_restore
    .type   jv_task_restore, @function
    # void jv_task_restore(jv_task_t *ctx %rdi)
jv_task_restore:
    # restore the registers
    movq    0x00(%rdi), %rbx
    movq    0x08(%rdi), %rbp
    movq    0x10(%rdi), %r12
    movq    0x18(%rdi), %r13
    movq    0x20(%rdi), %r14
    movq    0x28(%rdi), %r15
    movq    0x30(%rdi), %rsp

    # use r10 as a helper register to setup arguments
    movq    %rdi, %r10

    movq    0x40(%r10), %r11
    andq    $1, %r11
    jz     .skip_args

    # store address of `args` field in jv_task_t to %rax
    leaq    0x48(%r10), %rax

    # then setup routine's arguments for the first run
    movq    0x00(%rax), %rdi
    movq    0x08(%rax), %rsi
    movq    0x10(%rax), %rdx
    movq    0x18(%rax), %rcx
    movq    0x20(%rax), %r8
    movq    0x28(%rax), %r9

.skip_args:
    movq    $8, 0x40(%r10)      # set JV_STATE_RUNNING
    # return 0 (for now) as the return value of jv_await
    xorq    %rax, %rax
    jmp     *0x38(%r10)

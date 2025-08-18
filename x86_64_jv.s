	.file "x86_64_jv.s"
	.section .text

	.extern jv_context_switch

	.global	jv_yield
	.type	jv_yield, @function
jv_yield:
	leaq	8(%rsp), %rdi		# rsp
	movq	(%rsp), %rsi		# return address (rip)
	jmp		jv_context_switch


	.global jv_context_store
	.type	jv_context_store, @function
jv_context_store:
	movq	%rbx, 0x00(%rdi)
	movq	%rbp, 0x08(%rdi)
	movq	%r12, 0x10(%rdi)
	movq	%r13, 0x18(%rdi)
	movq	%r14, 0x20(%rdi)
	movq	%r15, 0x28(%rdi)
	movq	%rsi, 0x30(%rdi)
	movq	%rdx, 0x38(%rdi)
	ret


	.global jv_context_restore
	.type	jv_context_restore, @function
jv_context_restore:
	movq	0x00(%rdi), %rbx
	movq	0x08(%rdi), %rbp
	movq	0x10(%rdi), %r12
	movq	0x18(%rdi), %r13
	movq	0x20(%rdi), %r14
	movq	0x28(%rdi), %r15
	movq	0x30(%rdi), %rsp
	movq	%rdi, %rax

	# If `first_run` field in `jv_context_t` is true,
	# load the function's arguments befor jumping there.
	# Otherwise just do the call.
	cmpq	$1, 0x40(%rax)
	jne		.skip_args
	pushq	%rax

	# load the `args` field location to %rax
	# for easier access
	leaq	0x48(%rax), %rax

	movq	0x00(%rax), %rdi
	movq	0x08(%rax), %rsi
	movq	0x10(%rax), %rdx
	movq	0x18(%rax), %rcx
	movq	0x20(%rax), %r8
	movq	0x28(%rax), %r9

	# set `first_run` to NULL
	movq	$0, 0x40(%rax)
	popq	%rax

.skip_args:
	jmp		*0x38(%rax)

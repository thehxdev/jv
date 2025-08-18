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

	# use %r10 as a helper register
	movq	%rdi, %r10

	# If `first_run` field in `jv_context_t` is true, load
	# the function's arguments befor jumping there.
	# Otherwise just do the call.
	cmpq	$1, 0x40(%r10)
	jne		.skip_args

	# load the `args` field location to %r10 for
	# easier access. but before that save %r10
	# in the stack (must be popped at the end).
	pushq	%r10
	leaq	0x48(%r10), %r10

	movq	0x00(%r10), %rdi
	movq	0x08(%r10), %rsi
	movq	0x10(%r10), %rdx
	movq	0x18(%r10), %rcx
	movq	0x20(%r10), %r8
	movq	0x28(%r10), %r9

	# set `first_run` to 0 and restore %r10
	movq	$0, 0x40(%r10)
	popq	%r10

.skip_args:
	# xorq	%rax, %rax			# return 0 (for now) as the return value of jv_yield
	jmp		*0x38(%r10)

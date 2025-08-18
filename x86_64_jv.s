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
	jmp		*0x38(%rdi)


#  	.global jv_run_
#  	.type	jv_run_, @function
#  	# void jv_run_(void *fn %rdi, void *args[6] %rsi)
# jv_run_:
#  	# if args is NULL, this means funtion accepts no
#  	# parameter so just do the call
#  	test	%rsi, %rsi
#  	je		.do_call
#  
#     # save the function to call and args in temporary registers
#  	movq	%rdi, %r12
#  	movq	%rsi, %r14
#  
#  	movq	0x00(%r14), %rdi
#  	movq	0x08(%r14), %rsi
#  	movq	0x10(%r14), %rdx
#  	movq	0x18(%r14), %rcx
#  	movq	0x20(%r14), %r8
#  	movq	0x28(%r14), %r9
#  
# .do_call:
#  	call	*%r12
#  	ret

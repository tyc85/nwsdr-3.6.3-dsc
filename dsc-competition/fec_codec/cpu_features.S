.text
.global cpu_features
	.type cpu_features,@function
cpu_features:	
	pushq %rbx
	pushq %rcx
	pushq %rdx
	movq $1,%rax
	cpuid
	movq %rdx,%rax
	popq %rdx
	popq %rcx
	popq %rbx
	ret
	

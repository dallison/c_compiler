//
//  cxx_guard.s
//  c_compiler
//
//  Thread-safe Itanium C++ guard operations for x86_64.  Byte zero is the
//  initialized flag and byte one is the pending flag.
//

.text

.global __cxa_guard_acquire
.type __cxa_guard_acquire, @function
__cxa_guard_acquire:
	movb (%rdi), %al
	cmpb $0, %al
	jne .Lguard_done
	push %rdi
	call thrd_current
	mov %eax, %edx
	pop %rdi
.Lguard_retry:
	xor %eax, %eax
	mov $1, %ecx
	// lock cmpxchgb %cl, 1(%rdi)
	.byte 0xf0, 0x0f, 0xb0, 0x4f, 0x01
	je .Lguard_owner
.Lguard_wait:
	cmpl 4(%rdi), %edx
	je .Lguard_recursive
	movb (%rdi), %al
	cmpb $0, %al
	jne .Lguard_done
	movb 1(%rdi), %al
	cmpb $0, %al
	je .Lguard_retry
	// pause
	.byte 0xf3, 0x90
	jmp .Lguard_wait
.Lguard_owner:
	// A previous owner can publish initialization and clear the pending byte
	// between our initialized-byte check and cmpxchg.  If that happened, drop
	// the claim instead of running the initializer a second time.
	movb (%rdi), %al
	cmpb $0, %al
	jne .Lguard_initialized_after_claim
	movl %edx, 4(%rdi)
	mov $1, %eax
	ret
.Lguard_initialized_after_claim:
	movb $0, 1(%rdi)
	xor %eax, %eax
	ret
.Lguard_recursive:
	subq $8, %rsp
	call abort
.Lguard_done:
	xor %eax, %eax
	ret

.global __cxa_guard_release
.type __cxa_guard_release, @function
__cxa_guard_release:
	movb $1, (%rdi)
	movb $0, 1(%rdi)
	movl $0, 4(%rdi)
	ret

.global __cxa_guard_abort
.type __cxa_guard_abort, @function
__cxa_guard_abort:
	movl $0, 4(%rdi)
	movb $0, 1(%rdi)
	ret

.global __davecc_atexit_lock
.type __davecc_atexit_lock, @function
__davecc_atexit_lock:
.Latexit_lock_retry:
	xor %eax, %eax
	mov $1, %ecx
	// lock cmpxchgb %cl, (%rdi)
	.byte 0xf0, 0x0f, 0xb0, 0x0f
	je .Latexit_lock_done
	.byte 0xf3, 0x90
	jmp .Latexit_lock_retry
.Latexit_lock_done:
	ret

.global __davecc_atexit_unlock
.type __davecc_atexit_unlock, @function
__davecc_atexit_unlock:
	movb $0, (%rdi)
	ret

.global __davecc_finalize
.type __davecc_finalize, @function
__davecc_finalize:
	push %rax
	xor %edi, %edi
	call __cxa_finalize
	pop %rax
	ret

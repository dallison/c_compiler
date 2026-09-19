//
//  cxx_guard.s
//  x86 support (i386)
//
//  Thread-safe Itanium C++ guard operations and atexit lock.  cdecl: the
//  pointer argument is at 4(%esp).  Byte 0 of the guard is the initialized
//  flag, byte 1 is the pending flag, and the word at offset 4 is the owner
//  thread id.
//

.text

.global __cxa_guard_acquire
.type __cxa_guard_acquire, @function
__cxa_guard_acquire:
	pushl %ebx
	movl 8(%esp), %ebx
	movb 0(%ebx), %al
	cmpb $0, %al
	jne .Lguard_done
	call thrd_current
	movl %eax, %ecx
.Lguard_retry:
	xorl %eax, %eax
	movb $1, %dl
	// lock cmpxchgb %dl, 1(%ebx)
	.byte 0xf0, 0x0f, 0xb0, 0x53, 0x01
	je .Lguard_owner
.Lguard_wait:
	cmpl 4(%ebx), %ecx
	je .Lguard_recursive
	movb 0(%ebx), %al
	cmpb $0, %al
	jne .Lguard_done
	movb 1(%ebx), %al
	cmpb $0, %al
	je .Lguard_retry
	// pause
	.byte 0xf3, 0x90
	jmp .Lguard_wait
.Lguard_owner:
	movb 0(%ebx), %al
	cmpb $0, %al
	jne .Lguard_initialized_after_claim
	movl %ecx, 4(%ebx)
	movl $1, %eax
	popl %ebx
	ret
.Lguard_initialized_after_claim:
	movb $0, 1(%ebx)
	xorl %eax, %eax
	popl %ebx
	ret
.Lguard_recursive:
	call abort
.Lguard_done:
	xorl %eax, %eax
	popl %ebx
	ret

.global __cxa_guard_release
.type __cxa_guard_release, @function
__cxa_guard_release:
	movl 4(%esp), %eax
	movb $1, 0(%eax)
	movb $0, 1(%eax)
	movl $0, 4(%eax)
	ret

.global __cxa_guard_abort
.type __cxa_guard_abort, @function
__cxa_guard_abort:
	movl 4(%esp), %eax
	movl $0, 4(%eax)
	movb $0, 1(%eax)
	ret

.global __davecc_atexit_lock
.type __davecc_atexit_lock, @function
__davecc_atexit_lock:
	movl 4(%esp), %ecx
.Latexit_lock_retry:
	xorl %eax, %eax
	movb $1, %dl
	// lock cmpxchgb %dl, (%ecx)
	.byte 0xf0, 0x0f, 0xb0, 0x11
	je .Latexit_lock_done
	.byte 0xf3, 0x90
	jmp .Latexit_lock_retry
.Latexit_lock_done:
	ret

.global __davecc_atexit_unlock
.type __davecc_atexit_unlock, @function
__davecc_atexit_unlock:
	movl 4(%esp), %eax
	movb $0, 0(%eax)
	ret

.global __davecc_finalize
.type __davecc_finalize, @function
__davecc_finalize:
	pushl %eax
	movl $0, 0(%esp)
	call __cxa_finalize
	addl $4, %esp
	ret

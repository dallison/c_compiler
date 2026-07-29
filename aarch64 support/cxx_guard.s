	.text

	.global __cxa_guard_acquire
__cxa_guard_acquire:
	ldarb w1, [x0]
	cbnz w1, .Lguard_initialized
	sub sp, sp, #16
	stp x19, x30, [sp, #0]
	mov x19, x0
.Lguard_claim:
	ldarb w1, [x19]
	cbnz w1, .Lguard_initialized_locked
	add x3, x19, #1
	ldaxrb w1, [x3]
	cbnz w1, .Lguard_contended
	mov w1, #1
	stlxrb w2, w1, [x3]
	cbnz w2, .Lguard_claim
	// The initializer may have completed after the initialized-byte check but
	// before this thread claimed the now-cleared pending byte.  Recheck after
	// claiming so only one thread can be told to run the initializer.
	ldarb w1, [x19]
	cbnz w1, .Lguard_initialized_after_claim
	bl thrd_current
	str w0, [x19, #4]
	mov w0, #1
	ldp x19, x30, [sp, #0]
	add sp, sp, #16
	ret

.Lguard_initialized_after_claim:
	add x0, x19, #1
	stlrb wzr, [x0]
	mov w0, #0
	ldp x19, x30, [sp, #0]
	add sp, sp, #16
	ret

.Lguard_initialized_locked:
	mov w0, #0
	ldp x19, x30, [sp, #0]
	add sp, sp, #16
	ret

.Lguard_contended:
	clrex
	bl thrd_current
	ldr w1, [x19, #4]
	cmp w1, w0
	b.eq .Lguard_recursive
	b .Lguard_claim

.Lguard_recursive:
	bl abort

.Lguard_initialized:
	mov w0, #0
	ret

	.global __cxa_guard_release
__cxa_guard_release:
	str wzr, [x0, #4]
	mov w1, #1
	stlrb w1, [x0]
	add x0, x0, #1
	stlrb wzr, [x0]
	ret

	.global __cxa_guard_abort
__cxa_guard_abort:
	str wzr, [x0, #4]
	add x0, x0, #1
	stlrb wzr, [x0]
	ret

	.global __davecc_atexit_lock
__davecc_atexit_lock:
.Latexit_lock_retry:
	ldaxrb w1, [x0]
	cbnz w1, .Latexit_lock_retry
	mov w1, #1
	stlxrb w2, w1, [x0]
	cbnz w2, .Latexit_lock_retry
	ret

	.global __davecc_atexit_unlock
__davecc_atexit_unlock:
	stlrb wzr, [x0]
	ret

	.global __davecc_finalize
__davecc_finalize:
	mov x0, #0
	b __cxa_finalize

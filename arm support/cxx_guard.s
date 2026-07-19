.text

.global thrd_current
.global abort
.global __cxa_finalize

.global __cxa_guard_acquire
.type __cxa_guard_acquire, @function
__cxa_guard_acquire:
	ldrb r1, [r0, #0]
	cmp r1, #0
	bne .Lguard_initialized
	push {r4, lr}
	mov r4, r0
.Lguard_claim:
	ldrb r1, [r4, #0]
	cmp r1, #0
	bne .Lguard_initialized_locked
	add r3, r4, #1
	ldrexb r1, [r3]
	cmp r1, #0
	bne .Lguard_contended
	mov r1, #1
	strexb r2, r1, [r3]
	cmp r2, #0
	bne .Lguard_claim
	dmb ish
	ldrb r1, [r4, #0]
	cmp r1, #0
	bne .Lguard_initialized_after_claim
	bl thrd_current
	str r0, [r4, #4]
	mov r0, #1
	pop {r4, lr}
	bx lr

.Lguard_initialized_after_claim:
	add r0, r4, #1
	mov r1, #0
	dmb ish
	strb r1, [r0, #0]
	mov r0, #0
	pop {r4, lr}
	bx lr

.Lguard_initialized_locked:
	dmb ish
	mov r0, #0
	pop {r4, lr}
	bx lr

.Lguard_contended:
	clrex
	bl thrd_current
	ldr r1, [r4, #4]
	cmp r1, r0
	beq .Lguard_recursive
	b .Lguard_claim

.Lguard_recursive:
	bl abort

.Lguard_initialized:
	dmb ish
	mov r0, #0
	bx lr

.global __cxa_guard_release
.type __cxa_guard_release, @function
__cxa_guard_release:
	mov r1, #0
	str r1, [r0, #4]
	dmb ish
	mov r1, #1
	strb r1, [r0, #0]
	add r0, r0, #1
	mov r1, #0
	strb r1, [r0, #0]
	bx lr

.global __cxa_guard_abort
.type __cxa_guard_abort, @function
__cxa_guard_abort:
	mov r1, #0
	str r1, [r0, #4]
	dmb ish
	add r0, r0, #1
	strb r1, [r0, #0]
	bx lr

.global __davecc_atexit_lock
.type __davecc_atexit_lock, @function
__davecc_atexit_lock:
.Latexit_lock_retry:
	ldrexb r1, [r0]
	cmp r1, #0
	bne .Latexit_lock_retry
	mov r1, #1
	strexb r2, r1, [r0]
	cmp r2, #0
	bne .Latexit_lock_retry
	dmb ish
	bx lr

.global __davecc_atexit_unlock
.type __davecc_atexit_unlock, @function
__davecc_atexit_unlock:
	dmb ish
	mov r1, #0
	strb r1, [r0]
	bx lr

.global __davecc_finalize
.type __davecc_finalize, @function
__davecc_finalize:
	mov r0, #0
	b __cxa_finalize

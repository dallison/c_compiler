.text

.global thrd_current

.global __cxa_guard_acquire
.type __cxa_guard_acquire, @function
__cxa_guard_acquire:
	lr.d.aq t0, (a0)
	andi t1, t0, 255
	bnez t1, .Lguard_initialized
	addi sp, sp, -16
	sd s1, 0(sp)
	sd ra, 8(sp)
	mv s1, a0
.Lguard_claim:
	lr.d.aq t0, (s1)
	andi t1, t0, 255
	bnez t1, .Lguard_initialized_locked
	andi t1, t0, 256
	bnez t1, .Lguard_contended
	ori t1, t0, 256
	sc.d.rl t2, t1, (s1)
	bnez t2, .Lguard_claim
	call thrd_current
	sw a0, 4(s1)
	li a0, 1
	ld s1, 0(sp)
	ld ra, 8(sp)
	addi sp, sp, 16
	ret

.Lguard_initialized_locked:
	li a0, 0
	ld s1, 0(sp)
	ld ra, 8(sp)
	addi sp, sp, 16
	ret

.Lguard_contended:
	call thrd_current
	lw t0, 4(s1)
	beq t0, a0, .Lguard_recursive
	j .Lguard_claim

.Lguard_recursive:
	call abort

.Lguard_initialized:
	li a0, 0
	ret

.global __cxa_guard_release
.type __cxa_guard_release, @function
__cxa_guard_release:
	fence
	li t0, 1
	sd t0, 0(a0)
	fence
	ret

.global __cxa_guard_abort
.type __cxa_guard_abort, @function
__cxa_guard_abort:
	fence
	sd zero, 0(a0)
	fence
	ret

.global __davecc_atexit_lock
.type __davecc_atexit_lock, @function
__davecc_atexit_lock:
	li a0, 20
	j syscall

.global __davecc_atexit_unlock
.type __davecc_atexit_unlock, @function
__davecc_atexit_unlock:
	li a0, 21
	j syscall

.global __davecc_finalize
.type __davecc_finalize, @function
__davecc_finalize:
	li a0, 0
	j __cxa_finalize

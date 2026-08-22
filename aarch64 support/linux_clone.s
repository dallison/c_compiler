.text

.global __davecc_linux_clone
.type __davecc_linux_clone, @function

__davecc_linux_clone:
	stp x19, x20, [sp, #-16]!
	sub x7, x0, #16
	str x2, [x7]
	str x3, [x7, #8]
	mov x19, x2
	mov x20, x3
	mov x0, x1
	mov x1, x7
	mov x2, x5
	mov x3, x4
	mov x4, x5
	mov x8, #220
	svc #0
	cmp x0, #0
	b.eq .Lclone_child
	ldp x19, x20, [sp], #16
	ret

.Lclone_child:
	ldr x0, [sp]
	ldr x20, [sp, #8]
	blr x20
	mov x8, #93
	svc #0

	.global foreign_preinit
	.global foreign_fini

	.section ".preinit_array", "aw", @preinit_array, 8
	.p2align 3
	.8byte foreign_preinit

	.section ".fini_array.00050", "aw", @fini_array, 8
	.p2align 3
	.8byte foreign_fini

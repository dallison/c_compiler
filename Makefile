all:
	make -C Linker
	make -C c_compiler/ELF
	make -C c_compiler/Loader
	make -C c_compiler
	make -C 6502_interpreter
	make -C risc_v_interpreter
	make -C elfdump
	make -C davecc
	make -C 6502asm
	make -C 6502dasm


clean:
	make -C Linker clean
	make -C c_compiler/ELF clean
	make -C c_compiler/Loader clean
	make -C 6502_interpreter clean
	make -C risc_v_interpreter clean
	make -C c_compiler clean
	make -C elfdump clean
	make -C davecc clean
	make -C 6502asm clean
	make -C 6502dasm clean

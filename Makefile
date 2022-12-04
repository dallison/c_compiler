PREFIX=$(shell pwd)

all: opt

opt:
	make -C c_compiler OPT_DEBUG=-O2 PREFIX=$(PREFIX)
	make -C Linker OPT_DEBUG=-O2 PREFIX=$(PREFIX)
	make -C c_compiler/ELF OPT_DEBUG=-O2 PREFIX=$(PREFIX)
	make -C c_compiler/AR OPT_DEBUG=-O2 PREFIX=$(PREFIX)
	make -C c_compiler/Loader OPT_DEBUG=-O2 PREFIX=$(PREFIX)
	make -C 6502_interpreter OPT_DEBUG=-O2 PREFIX=$(PREFIX)
	make -C risc_v_interpreter OPT_DEBUG=-O2 PREFIX=$(PREFIX)
	make -C archivist OPT_DEBUG=-O2 PREFIX=$(PREFIX)
	make -C elfdump OPT_DEBUG=-O2 PREFIX=$(PREFIX)
	make -C davecc OPT_DEBUG=-O2 PREFIX=$(PREFIX)
	make -C 6502asm OPT_DEBUG=-O2 PREFIX=$(PREFIX)
	make -C 6502dasm OPT_DEBUG=-O2 PREFIX=$(PREFIX)


debug:
	make -C c_compiler OPT_DEBUG=-g PREFIX=$(PREFIX)
	make -C Linker OPT_DEBUG=-g PREFIX=$(PREFIX)
	make -C c_compiler/ELF OPT_DEBUG=-g PREFIX=$(PREFIX)
	make -C c_compiler/AR OPT_DEBUG=-g PREFIX=$(PREFIX)
	make -C c_compiler/Loader OPT_DEBUG=-g PREFIX=$(PREFIX)
	make -C 6502_interpreter OPT_DEBUG=-g PREFIX=$(PREFIX)
	make -C risc_v_interpreter OPT_DEBUG=-g PREFIX=$(PREFIX)
	make -C archivist OPT_DEBUG=-g PREFIX=$(PREFIX)
	make -C elfdump OPT_DEBUG=-g PREFIX=$(PREFIX)
	make -C davecc OPT_DEBUG=-g PREFIX=$(PREFIX)
	make -C 6502asm OPT_DEBUG=-g PREFIX=$(PREFIX)
	make -C 6502dasm OPT_DEBUG=-g PREFIX=$(PREFIX)


clean:
	make -C c_compiler clean
	make -C Linker clean
	make -C c_compiler/ELF clean
	make -C c_compiler/AR clean
	make -C c_compiler/Loader clean
	make -C 6502_interpreter clean
	make -C risc_v_interpreter clean
	make -C elfdump clean
	make -C archivist clean
	make -C davecc clean
	make -C 6502asm clean
	make -C 6502dasm clean

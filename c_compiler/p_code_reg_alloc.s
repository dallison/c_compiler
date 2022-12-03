	.file   "p_code_reg_alloc.c"
	.text
	.option pic
.PCbegin:
	.local  InitializeRegister
	.type InitializeRegister, @function

InitializeRegister:

	// *** Basic block 0

	.global TargetRegisterInit
	// Leaf procedure, no stack frame generated
	j           TargetRegisterInit
.func_end_InitializeRegister:
	.size InitializeRegister, .func_end_InitializeRegister-InitializeRegister

	.global PCodeRegisterAllocatorInit
	.type PCodeRegisterAllocatorInit, @function

PCodeRegisterAllocatorInit:

	// *** Basic block 0

	.local InitializeRegister
	.global BitSetInit
	addi sp, sp, -64
	// Saved return address (offset 56) and frame pointer (offset 48)
	sd ra, 56(sp)
	sd s0, 48(sp)
	addi s0, sp, 64
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 40(sp)
	sd s2, 32(sp)
	sd s3, 24(sp)
	sd s4, 16(sp)
	sd s5, 8(sp)
	sd s6, 0(sp)
	// End of stack frame
	mv          s1, a0
	sd          a1, 0(s1)
	mv          s2, x0

	// *** Basic block 1

.PCodeRegisterAllocatorInit_label_43:
	addi        t0, s1, 8
	slli        t1, s2, 3
	slli        t2, s2, 4
	add         t1, t1, t2
	add         a0, t0, t1
	mv          a2, x0
	mv          a1, s2
	call        InitializeRegister

	// *** Basic block 2

.PCodeRegisterAllocatorInit_label_57:
	addi        s2, s2, 1
	li          s3, 256		// 0x100
	bge         s2, s3, .PCodeRegisterAllocatorInit_label_43

	// *** Basic block 3

.PCodeRegisterAllocatorInit_label_62:
	mv          s2, x0

	// *** Basic block 4

.PCodeRegisterAllocatorInit_label_66:
	li          s4, 6152		// 0x1808
	add         t0, s1, s4
	slli        t1, s2, 3
	slli        t2, s2, 4
	add         t1, t1, t2
	add         a0, t0, t1
	li          s5, 1		// 0x1 ASCII \x1
	mv          a2, s5
	mv          a1, s2
	call        InitializeRegister

	// *** Basic block 5

.PCodeRegisterAllocatorInit_label_80:
	addi        s2, s2, 1
	bge         s2, s3, .PCodeRegisterAllocatorInit_label_66

	// *** Basic block 6

.PCodeRegisterAllocatorInit_label_85:
	mv          s2, x0

	// *** Basic block 7

.PCodeRegisterAllocatorInit_label_89:
	li          s6, 12296		// 0x3008
	add         t0, s1, s6
	slli        t1, s2, 3
	slli        t2, s2, 4
	add         t1, t1, t2
	add         a0, t0, t1
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	mv          a1, s2
	call        InitializeRegister

	// *** Basic block 8

.PCodeRegisterAllocatorInit_label_103:
	addi        s2, s2, 1
	bge         s2, s3, .PCodeRegisterAllocatorInit_label_89

	// *** Basic block 9

.PCodeRegisterAllocatorInit_label_108:
	addi        t0, s1, 8
	addi        t0, t0, 744
	sb          s5, 4(t0)
	addi        t0, s1, 8
	addi        t0, t0, 720
	sb          s5, 4(t0)
	addi        t0, s1, 8
	addi        t0, t0, 792
	sb          s5, 4(t0)
	addi        t0, s1, 8
	addi        t0, t0, 816
	sb          s5, 4(t0)
	addi        t0, s1, 8
	addi        t0, t0, 624
	sb          s5, 4(t0)
	addi        t0, s1, 8
	addi        t0, t0, 648
	sb          s5, 4(t0)
	addi        t0, s1, 8
	addi        t0, t0, 672
	sb          s5, 4(t0)
	addi        t0, s1, 8
	addi        t0, t0, 696
	sb          s5, 4(t0)
	addi        t0, s1, 8
	sb          s5, 4(t0)
	add         t0, s1, s4
	sb          s5, 4(t0)
	add         t0, s1, s6
	sb          s5, 4(t0)
	li          t0, 18440		// 0x4808
	add         a0, s1, t0
	call        BitSetInit

	// *** Basic block 10

	li          t0, 18456		// 0x4818
	add         a0, s1, t0
	call        BitSetInit

	// *** Basic block 11

	li          t0, 18472		// 0x4828
	add         a0, s1, t0
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	ld s6, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           BitSetInit
.func_end_PCodeRegisterAllocatorInit:
	.size PCodeRegisterAllocatorInit, .func_end_PCodeRegisterAllocatorInit-PCodeRegisterAllocatorInit

	.global NewPCodeRegisterAllocator
	.type NewPCodeRegisterAllocator, @function

NewPCodeRegisterAllocator:

	// *** Basic block 0

	.global malloc
	.global PCodeRegisterAllocatorInit
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	sd s2, 0(sp)
	// End of stack frame
	mv          s1, a0
	li          a0, 18488		// 0x4838
	call        malloc

	// *** Basic block 1

	mv          s2, a0
	mv          a1, s1
	mv          a0, s2
	call        PCodeRegisterAllocatorInit

	// *** Basic block 2

	mv          a0, s2

	// *** Basic block 3

.NewPCodeRegisterAllocator_label_21:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewPCodeRegisterAllocator:
	.size NewPCodeRegisterAllocator, .func_end_NewPCodeRegisterAllocator-NewPCodeRegisterAllocator

	.global PCodeRegisterAllocatorDestruct
	.type PCodeRegisterAllocatorDestruct, @function

PCodeRegisterAllocatorDestruct:

	// *** Basic block 0

	.global BitSetDestruct
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	mv          s1, a0
	li          t0, 18440		// 0x4808
	add         a0, s1, t0
	call        BitSetDestruct

	// *** Basic block 1

	li          t0, 18456		// 0x4818
	add         a0, s1, t0
	call        BitSetDestruct

	// *** Basic block 2

	li          t0, 18472		// 0x4828
	add         a0, s1, t0
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           BitSetDestruct
.func_end_PCodeRegisterAllocatorDestruct:
	.size PCodeRegisterAllocatorDestruct, .func_end_PCodeRegisterAllocatorDestruct-PCodeRegisterAllocatorDestruct

	.global PCodeRegisterAllocatorDelete
	.type PCodeRegisterAllocatorDelete, @function

PCodeRegisterAllocatorDelete:

	// *** Basic block 0

	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	.global PCodeRegisterAllocatorDestruct
	.global free
	mv          s1, a0
	call        PCodeRegisterAllocatorDestruct

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_PCodeRegisterAllocatorDelete:
	.size PCodeRegisterAllocatorDelete, .func_end_PCodeRegisterAllocatorDelete-PCodeRegisterAllocatorDelete

	.local  FindFreeRegister
	.type FindFreeRegister, @function

FindFreeRegister:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	slli        t1, a1, 2
	auipc       t2, 0
	add         t1, t2, t1
	jalr        x0, t1, 12

	// *** Basic block 1

	j           .FindFreeRegister_label_26

	// *** Basic block 2

	j           .FindFreeRegister_label_32

	// *** Basic block 3

	j           .FindFreeRegister_label_37

	// *** Basic block 4

.FindFreeRegister_label_26:
	addi        t1, t0, 8
	li          t2, 256		// 0x100
	j           .FindFreeRegister_label_42

	// *** Basic block 5

.FindFreeRegister_label_32:
	li          t3, 6152		// 0x1808
	add         t1, t0, t3
	li          t2, 256		// 0x100
	j           .FindFreeRegister_label_42

	// *** Basic block 6

.FindFreeRegister_label_37:
	li          t3, 12296		// 0x3008
	add         t1, t0, t3
	li          t2, 256		// 0x100
	j           .FindFreeRegister_label_42

	// *** Basic block 7

.FindFreeRegister_label_42:
	mv          t3, x0
	bge         x0, t2, .FindFreeRegister_label_74

	// *** Basic block 8

.FindFreeRegister_label_47:
	slli        t5, t3, 3
	slli        t6, t3, 4
	add         t5, t5, t6
	add         t6, t1, t5
	lb          a1, 4(t6)
	not         t4, a1
	beqz        t4, .FindFreeRegister_label_62

	// *** Basic block 9

	ld          t6, 8(t6)
	sub         t6, t6, x0
	seqz        t4, t6

	// *** Basic block 10

.FindFreeRegister_label_62:
	beqz        t4, .FindFreeRegister_label_69

	// *** Basic block 11

	add         a0, t1, t5

	// *** Basic block 12

.FindFreeRegister_label_66:
	ret         

	// *** Basic block 13

.FindFreeRegister_label_69:

	// *** Basic block 14

.FindFreeRegister_label_70:
	addi        t3, t3, 1
	bge         t3, t2, .FindFreeRegister_label_47

	// *** Basic block 15

.FindFreeRegister_label_74:
	mv          a0, x0
	ret         
.func_end_FindFreeRegister:
	.size FindFreeRegister, .func_end_FindFreeRegister-FindFreeRegister

	.local  FreeRegister
	.type FreeRegister, @function

FreeRegister:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	sd          x0, 8(a1)
	ret         
.func_end_FreeRegister:
	.size FreeRegister, .func_end_FreeRegister-FreeRegister

	.local  RegisterTypeFromInstruction
	.type RegisterTypeFromInstruction, @function

RegisterTypeFromInstruction:

	// *** Basic block 0

	.global printf
	.global abort
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	lw          s1, 16(a0)
	li          t0, 3		// 0x3 ASCII \x3
	blt         s1, t0, .RegisterTypeFromInstruction_label_261

	// *** Basic block 1

	li          t0, 131		// 0x83 ASCII \x83
	blt         t0, s1, .RegisterTypeFromInstruction_label_261

	// *** Basic block 2

	addi        t0, s1, -3
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 3

	j           .RegisterTypeFromInstruction_label_213

	// *** Basic block 4

	j           .RegisterTypeFromInstruction_label_220

	// *** Basic block 5

	j           .RegisterTypeFromInstruction_label_261

	// *** Basic block 6

	j           .RegisterTypeFromInstruction_label_261

	// *** Basic block 7

	j           .RegisterTypeFromInstruction_label_261

	// *** Basic block 8

	j           .RegisterTypeFromInstruction_label_261

	// *** Basic block 9

	j           .RegisterTypeFromInstruction_label_261

	// *** Basic block 10

	j           .RegisterTypeFromInstruction_label_261

	// *** Basic block 11

	j           .RegisterTypeFromInstruction_label_160

	// *** Basic block 12

	j           .RegisterTypeFromInstruction_label_227

	// *** Basic block 13

	j           .RegisterTypeFromInstruction_label_244

	// *** Basic block 14

	j           .RegisterTypeFromInstruction_label_161

	// *** Basic block 15

	j           .RegisterTypeFromInstruction_label_228

	// *** Basic block 16

	j           .RegisterTypeFromInstruction_label_245

	// *** Basic block 17

	j           .RegisterTypeFromInstruction_label_162

	// *** Basic block 18

	j           .RegisterTypeFromInstruction_label_166

	// *** Basic block 19

	j           .RegisterTypeFromInstruction_label_229

	// *** Basic block 20

	j           .RegisterTypeFromInstruction_label_246

	// *** Basic block 21

	j           .RegisterTypeFromInstruction_label_261

	// *** Basic block 22

	j           .RegisterTypeFromInstruction_label_261

	// *** Basic block 23

	j           .RegisterTypeFromInstruction_label_214

	// *** Basic block 24

	j           .RegisterTypeFromInstruction_label_215

	// *** Basic block 25

	j           .RegisterTypeFromInstruction_label_217

	// *** Basic block 26

	j           .RegisterTypeFromInstruction_label_218

	// *** Basic block 27

	j           .RegisterTypeFromInstruction_label_239

	// *** Basic block 28

	j           .RegisterTypeFromInstruction_label_254

	// *** Basic block 29

	j           .RegisterTypeFromInstruction_label_221

	// *** Basic block 30

	j           .RegisterTypeFromInstruction_label_261

	// *** Basic block 31

	j           .RegisterTypeFromInstruction_label_261

	// *** Basic block 32

	j           .RegisterTypeFromInstruction_label_261

	// *** Basic block 33

	j           .RegisterTypeFromInstruction_label_261

	// *** Basic block 34

	j           .RegisterTypeFromInstruction_label_261

	// *** Basic block 35

	j           .RegisterTypeFromInstruction_label_216

	// *** Basic block 36

	j           .RegisterTypeFromInstruction_label_261

	// *** Basic block 37

	j           .RegisterTypeFromInstruction_label_261

	// *** Basic block 38

	j           .RegisterTypeFromInstruction_label_261

	// *** Basic block 39

	j           .RegisterTypeFromInstruction_label_261

	// *** Basic block 40

	j           .RegisterTypeFromInstruction_label_261

	// *** Basic block 41

	j           .RegisterTypeFromInstruction_label_261

	// *** Basic block 42

	j           .RegisterTypeFromInstruction_label_261

	// *** Basic block 43

	j           .RegisterTypeFromInstruction_label_261

	// *** Basic block 44

	j           .RegisterTypeFromInstruction_label_261

	// *** Basic block 45

	j           .RegisterTypeFromInstruction_label_261

	// *** Basic block 46

	j           .RegisterTypeFromInstruction_label_167

	// *** Basic block 47

	j           .RegisterTypeFromInstruction_label_168

	// *** Basic block 48

	j           .RegisterTypeFromInstruction_label_169

	// *** Basic block 49

	j           .RegisterTypeFromInstruction_label_170

	// *** Basic block 50

	j           .RegisterTypeFromInstruction_label_171

	// *** Basic block 51

	j           .RegisterTypeFromInstruction_label_172

	// *** Basic block 52

	j           .RegisterTypeFromInstruction_label_173

	// *** Basic block 53

	j           .RegisterTypeFromInstruction_label_230

	// *** Basic block 54

	j           .RegisterTypeFromInstruction_label_247

	// *** Basic block 55

	j           .RegisterTypeFromInstruction_label_174

	// *** Basic block 56

	j           .RegisterTypeFromInstruction_label_175

	// *** Basic block 57

	j           .RegisterTypeFromInstruction_label_176

	// *** Basic block 58

	j           .RegisterTypeFromInstruction_label_231

	// *** Basic block 59

	j           .RegisterTypeFromInstruction_label_248

	// *** Basic block 60

	j           .RegisterTypeFromInstruction_label_177

	// *** Basic block 61

	j           .RegisterTypeFromInstruction_label_178

	// *** Basic block 62

	j           .RegisterTypeFromInstruction_label_232

	// *** Basic block 63

	j           .RegisterTypeFromInstruction_label_249

	// *** Basic block 64

	j           .RegisterTypeFromInstruction_label_179

	// *** Basic block 65

	j           .RegisterTypeFromInstruction_label_180

	// *** Basic block 66

	j           .RegisterTypeFromInstruction_label_233

	// *** Basic block 67

	j           .RegisterTypeFromInstruction_label_250

	// *** Basic block 68

	j           .RegisterTypeFromInstruction_label_181

	// *** Basic block 69

	j           .RegisterTypeFromInstruction_label_234

	// *** Basic block 70

	j           .RegisterTypeFromInstruction_label_251

	// *** Basic block 71

	j           .RegisterTypeFromInstruction_label_182

	// *** Basic block 72

	j           .RegisterTypeFromInstruction_label_261

	// *** Basic block 73

	j           .RegisterTypeFromInstruction_label_235

	// *** Basic block 74

	j           .RegisterTypeFromInstruction_label_252

	// *** Basic block 75

	j           .RegisterTypeFromInstruction_label_183

	// *** Basic block 76

	j           .RegisterTypeFromInstruction_label_261

	// *** Basic block 77

	j           .RegisterTypeFromInstruction_label_184

	// *** Basic block 78

	j           .RegisterTypeFromInstruction_label_185

	// *** Basic block 79

	j           .RegisterTypeFromInstruction_label_186

	// *** Basic block 80

	j           .RegisterTypeFromInstruction_label_187

	// *** Basic block 81

	j           .RegisterTypeFromInstruction_label_188

	// *** Basic block 82

	j           .RegisterTypeFromInstruction_label_189

	// *** Basic block 83

	j           .RegisterTypeFromInstruction_label_190

	// *** Basic block 84

	j           .RegisterTypeFromInstruction_label_191

	// *** Basic block 85

	j           .RegisterTypeFromInstruction_label_192

	// *** Basic block 86

	j           .RegisterTypeFromInstruction_label_236

	// *** Basic block 87

	j           .RegisterTypeFromInstruction_label_253

	// *** Basic block 88

	j           .RegisterTypeFromInstruction_label_193

	// *** Basic block 89

	j           .RegisterTypeFromInstruction_label_194

	// *** Basic block 90

	j           .RegisterTypeFromInstruction_label_195

	// *** Basic block 91

	j           .RegisterTypeFromInstruction_label_196

	// *** Basic block 92

	j           .RegisterTypeFromInstruction_label_197

	// *** Basic block 93

	j           .RegisterTypeFromInstruction_label_198

	// *** Basic block 94

	j           .RegisterTypeFromInstruction_label_261

	// *** Basic block 95

	j           .RegisterTypeFromInstruction_label_261

	// *** Basic block 96

	j           .RegisterTypeFromInstruction_label_261

	// *** Basic block 97

	j           .RegisterTypeFromInstruction_label_261

	// *** Basic block 98

	j           .RegisterTypeFromInstruction_label_199

	// *** Basic block 99

	j           .RegisterTypeFromInstruction_label_200

	// *** Basic block 100

	j           .RegisterTypeFromInstruction_label_201

	// *** Basic block 101

	j           .RegisterTypeFromInstruction_label_202

	// *** Basic block 102

	j           .RegisterTypeFromInstruction_label_203

	// *** Basic block 103

	j           .RegisterTypeFromInstruction_label_204

	// *** Basic block 104

	j           .RegisterTypeFromInstruction_label_205

	// *** Basic block 105

	j           .RegisterTypeFromInstruction_label_206

	// *** Basic block 106

	j           .RegisterTypeFromInstruction_label_207

	// *** Basic block 107

	j           .RegisterTypeFromInstruction_label_208

	// *** Basic block 108

	j           .RegisterTypeFromInstruction_label_209

	// *** Basic block 109

	j           .RegisterTypeFromInstruction_label_210

	// *** Basic block 110

	j           .RegisterTypeFromInstruction_label_261

	// *** Basic block 111

	j           .RegisterTypeFromInstruction_label_261

	// *** Basic block 112

	j           .RegisterTypeFromInstruction_label_261

	// *** Basic block 113

	j           .RegisterTypeFromInstruction_label_261

	// *** Basic block 114

	j           .RegisterTypeFromInstruction_label_237

	// *** Basic block 115

	j           .RegisterTypeFromInstruction_label_256

	// *** Basic block 116

	j           .RegisterTypeFromInstruction_label_261

	// *** Basic block 117

	j           .RegisterTypeFromInstruction_label_261

	// *** Basic block 118

	j           .RegisterTypeFromInstruction_label_257

	// *** Basic block 119

	j           .RegisterTypeFromInstruction_label_238

	// *** Basic block 120

	j           .RegisterTypeFromInstruction_label_211

	// *** Basic block 121

	j           .RegisterTypeFromInstruction_label_212

	// *** Basic block 122

	j           .RegisterTypeFromInstruction_label_261

	// *** Basic block 123

	j           .RegisterTypeFromInstruction_label_261

	// *** Basic block 124

	j           .RegisterTypeFromInstruction_label_261

	// *** Basic block 125

	j           .RegisterTypeFromInstruction_label_261

	// *** Basic block 126

	j           .RegisterTypeFromInstruction_label_163

	// *** Basic block 127

	j           .RegisterTypeFromInstruction_label_164

	// *** Basic block 128

	j           .RegisterTypeFromInstruction_label_165

	// *** Basic block 129

	j           .RegisterTypeFromInstruction_label_219

	// *** Basic block 130

	j           .RegisterTypeFromInstruction_label_240

	// *** Basic block 131

	j           .RegisterTypeFromInstruction_label_255

	// *** Basic block 132

.RegisterTypeFromInstruction_label_160:

	// *** Basic block 133

.RegisterTypeFromInstruction_label_161:

	// *** Basic block 134

.RegisterTypeFromInstruction_label_162:

	// *** Basic block 135

.RegisterTypeFromInstruction_label_163:

	// *** Basic block 136

.RegisterTypeFromInstruction_label_164:

	// *** Basic block 137

.RegisterTypeFromInstruction_label_165:

	// *** Basic block 138

.RegisterTypeFromInstruction_label_166:

	// *** Basic block 139

.RegisterTypeFromInstruction_label_167:

	// *** Basic block 140

.RegisterTypeFromInstruction_label_168:

	// *** Basic block 141

.RegisterTypeFromInstruction_label_169:

	// *** Basic block 142

.RegisterTypeFromInstruction_label_170:

	// *** Basic block 143

.RegisterTypeFromInstruction_label_171:

	// *** Basic block 144

.RegisterTypeFromInstruction_label_172:

	// *** Basic block 145

.RegisterTypeFromInstruction_label_173:

	// *** Basic block 146

.RegisterTypeFromInstruction_label_174:

	// *** Basic block 147

.RegisterTypeFromInstruction_label_175:

	// *** Basic block 148

.RegisterTypeFromInstruction_label_176:

	// *** Basic block 149

.RegisterTypeFromInstruction_label_177:

	// *** Basic block 150

.RegisterTypeFromInstruction_label_178:

	// *** Basic block 151

.RegisterTypeFromInstruction_label_179:

	// *** Basic block 152

.RegisterTypeFromInstruction_label_180:

	// *** Basic block 153

.RegisterTypeFromInstruction_label_181:

	// *** Basic block 154

.RegisterTypeFromInstruction_label_182:

	// *** Basic block 155

.RegisterTypeFromInstruction_label_183:

	// *** Basic block 156

.RegisterTypeFromInstruction_label_184:

	// *** Basic block 157

.RegisterTypeFromInstruction_label_185:

	// *** Basic block 158

.RegisterTypeFromInstruction_label_186:

	// *** Basic block 159

.RegisterTypeFromInstruction_label_187:

	// *** Basic block 160

.RegisterTypeFromInstruction_label_188:

	// *** Basic block 161

.RegisterTypeFromInstruction_label_189:

	// *** Basic block 162

.RegisterTypeFromInstruction_label_190:

	// *** Basic block 163

.RegisterTypeFromInstruction_label_191:

	// *** Basic block 164

.RegisterTypeFromInstruction_label_192:

	// *** Basic block 165

.RegisterTypeFromInstruction_label_193:

	// *** Basic block 166

.RegisterTypeFromInstruction_label_194:

	// *** Basic block 167

.RegisterTypeFromInstruction_label_195:

	// *** Basic block 168

.RegisterTypeFromInstruction_label_196:

	// *** Basic block 169

.RegisterTypeFromInstruction_label_197:

	// *** Basic block 170

.RegisterTypeFromInstruction_label_198:

	// *** Basic block 171

.RegisterTypeFromInstruction_label_199:

	// *** Basic block 172

.RegisterTypeFromInstruction_label_200:

	// *** Basic block 173

.RegisterTypeFromInstruction_label_201:

	// *** Basic block 174

.RegisterTypeFromInstruction_label_202:

	// *** Basic block 175

.RegisterTypeFromInstruction_label_203:

	// *** Basic block 176

.RegisterTypeFromInstruction_label_204:

	// *** Basic block 177

.RegisterTypeFromInstruction_label_205:

	// *** Basic block 178

.RegisterTypeFromInstruction_label_206:

	// *** Basic block 179

.RegisterTypeFromInstruction_label_207:

	// *** Basic block 180

.RegisterTypeFromInstruction_label_208:

	// *** Basic block 181

.RegisterTypeFromInstruction_label_209:

	// *** Basic block 182

.RegisterTypeFromInstruction_label_210:

	// *** Basic block 183

.RegisterTypeFromInstruction_label_211:

	// *** Basic block 184

.RegisterTypeFromInstruction_label_212:

	// *** Basic block 185

.RegisterTypeFromInstruction_label_213:

	// *** Basic block 186

.RegisterTypeFromInstruction_label_214:

	// *** Basic block 187

.RegisterTypeFromInstruction_label_215:

	// *** Basic block 188

.RegisterTypeFromInstruction_label_216:

	// *** Basic block 189

.RegisterTypeFromInstruction_label_217:

	// *** Basic block 190

.RegisterTypeFromInstruction_label_218:

	// *** Basic block 191

.RegisterTypeFromInstruction_label_219:

	// *** Basic block 192

.RegisterTypeFromInstruction_label_220:

	// *** Basic block 193

.RegisterTypeFromInstruction_label_221:
	mv          a0, x0

	// *** Basic block 194

.RegisterTypeFromInstruction_label_224:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 195

.RegisterTypeFromInstruction_label_227:

	// *** Basic block 196

.RegisterTypeFromInstruction_label_228:

	// *** Basic block 197

.RegisterTypeFromInstruction_label_229:

	// *** Basic block 198

.RegisterTypeFromInstruction_label_230:

	// *** Basic block 199

.RegisterTypeFromInstruction_label_231:

	// *** Basic block 200

.RegisterTypeFromInstruction_label_232:

	// *** Basic block 201

.RegisterTypeFromInstruction_label_233:

	// *** Basic block 202

.RegisterTypeFromInstruction_label_234:

	// *** Basic block 203

.RegisterTypeFromInstruction_label_235:

	// *** Basic block 204

.RegisterTypeFromInstruction_label_236:

	// *** Basic block 205

.RegisterTypeFromInstruction_label_237:

	// *** Basic block 206

.RegisterTypeFromInstruction_label_238:

	// *** Basic block 207

.RegisterTypeFromInstruction_label_239:

	// *** Basic block 208

.RegisterTypeFromInstruction_label_240:
	li          a0, 1		// 0x1 ASCII \x1
	j           .RegisterTypeFromInstruction_label_224

	// *** Basic block 209

.RegisterTypeFromInstruction_label_244:

	// *** Basic block 210

.RegisterTypeFromInstruction_label_245:

	// *** Basic block 211

.RegisterTypeFromInstruction_label_246:

	// *** Basic block 212

.RegisterTypeFromInstruction_label_247:

	// *** Basic block 213

.RegisterTypeFromInstruction_label_248:

	// *** Basic block 214

.RegisterTypeFromInstruction_label_249:

	// *** Basic block 215

.RegisterTypeFromInstruction_label_250:

	// *** Basic block 216

.RegisterTypeFromInstruction_label_251:

	// *** Basic block 217

.RegisterTypeFromInstruction_label_252:

	// *** Basic block 218

.RegisterTypeFromInstruction_label_253:

	// *** Basic block 219

.RegisterTypeFromInstruction_label_254:

	// *** Basic block 220

.RegisterTypeFromInstruction_label_255:

	// *** Basic block 221

.RegisterTypeFromInstruction_label_256:

	// *** Basic block 222

.RegisterTypeFromInstruction_label_257:
	li          a0, 2		// 0x2 ASCII \x2
	j           .RegisterTypeFromInstruction_label_224

	// *** Basic block 223

.RegisterTypeFromInstruction_label_261:
	lla         a0, .str.1
	lla         a1, .str.2
	lla         a3, .str.3
	li          t0, 203		// 0xcb ASCII \xcb
	mv          a2, t0
	call        printf

	// *** Basic block 224

	call        abort

	// *** Basic block 225

	mv          a0, x0
	j           .RegisterTypeFromInstruction_label_224
.func_end_RegisterTypeFromInstruction:
	.size RegisterTypeFromInstruction, .func_end_RegisterTypeFromInstruction-RegisterTypeFromInstruction

	.local  FreeRegisters
	.type FreeRegisters, @function

FreeRegisters:

	// *** Basic block 0

	.global printf
	.global abort
	.local FreeRegister
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 24(sp)
	sd s2, 16(sp)
	sd s3, 8(sp)
	sd s4, 0(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, x0
	addi        t0, a1, 40

	// *** Basic block 1

.FreeRegisters_label_29:
	slli        t1, s2, 3
	add         t0, t0, t1
	ld          t1, 0(t0)
	beq         t1, x0, .FreeRegisters_label_86

	// *** Basic block 2

	ld          s3, 32(t1)
	sub         t2, s3, x0
	snez        t0, t2
	beq         s3, x0, .FreeRegisters_label_48

	// *** Basic block 3

	lb          t2, 4(s3)
	not         t0, t2

	// *** Basic block 4

.FreeRegisters_label_48:
	beqz        t0, .FreeRegisters_label_85

	// *** Basic block 5

	lw          s4, 88(t1)
	addi        t0, s4, -1
	sw          t0, 88(t1)
	blt         s4, x0, .FreeRegisters_label_59

	// *** Basic block 6

	j           .FreeRegisters_label_76

	// *** Basic block 7

.FreeRegisters_label_59:
	lla         a0, .str.4
	lla         a1, .str.5
	lla         a3, .str.6
	li          t0, 216		// 0xd8 ASCII \xd8
	mv          a2, t0
	call        printf

	// *** Basic block 8

	call        abort

	// *** Basic block 9

.FreeRegisters_label_76:
	bnez        s4, .FreeRegisters_label_84

	// *** Basic block 10

	mv          a1, s3
	mv          a0, s1
	call        FreeRegister

	// *** Basic block 11

.FreeRegisters_label_84:

	// *** Basic block 12

.FreeRegisters_label_85:

	// *** Basic block 13

.FreeRegisters_label_86:

	// *** Basic block 14

.FreeRegisters_label_87:
	addi        s2, s2, 1
	li          t0, 3		// 0x3 ASCII \x3
	bge         s2, t0, .FreeRegisters_label_29

	// *** Basic block 15

.FreeRegisters_label_92:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_FreeRegisters:
	.size FreeRegisters, .func_end_FreeRegisters-FreeRegisters

	.local  AllocateRegisterWithType
	.type AllocateRegisterWithType, @function

AllocateRegisterWithType:

	// *** Basic block 0

	.local FindFreeRegister
	.global printf
	.global abort
	.global BitSetInsert
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 24(sp)
	sd s2, 16(sp)
	sd s3, 8(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	call        FindFreeRegister

	// *** Basic block 1

	mv          s3, a0
	beq         s3, x0, .AllocateRegisterWithType_label_33

	// *** Basic block 2

	j           .AllocateRegisterWithType_label_50

	// *** Basic block 3

.AllocateRegisterWithType_label_33:
	lla         a0, .str.7
	lla         a1, .str.8
	lla         a3, .str.9
	li          t0, 230		// 0xe6 ASCII \xe6
	mv          a2, t0
	call        printf

	// *** Basic block 4

	call        abort

	// *** Basic block 5

.AllocateRegisterWithType_label_50:
	slli        t0, s2, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 6

	j           .AllocateRegisterWithType_label_59

	// *** Basic block 7

	j           .AllocateRegisterWithType_label_67

	// *** Basic block 8

	j           .AllocateRegisterWithType_label_75

	// *** Basic block 9

.AllocateRegisterWithType_label_59:
	li          t0, 18440		// 0x4808
	add         a0, s1, t0
	lw          a1, 0(s3)
	call        BitSetInsert

	// *** Basic block 10

	j           .AllocateRegisterWithType_label_83

	// *** Basic block 11

.AllocateRegisterWithType_label_67:
	li          t0, 18456		// 0x4818
	add         a0, s1, t0
	lw          a1, 0(s3)
	call        BitSetInsert

	// *** Basic block 12

	j           .AllocateRegisterWithType_label_83

	// *** Basic block 13

.AllocateRegisterWithType_label_75:
	li          t0, 18472		// 0x4828
	add         a0, s1, t0
	lw          a1, 0(s3)
	call        BitSetInsert

	// *** Basic block 14

	j           .AllocateRegisterWithType_label_83

	// *** Basic block 15

.AllocateRegisterWithType_label_83:
	mv          a0, s3

	// *** Basic block 16

.AllocateRegisterWithType_label_86:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_AllocateRegisterWithType:
	.size AllocateRegisterWithType, .func_end_AllocateRegisterWithType-AllocateRegisterWithType

	.local  UsesFixedRegister
	.type UsesFixedRegister, @function

UsesFixedRegister:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	lw          t0, 16(a0)
	li          t1, 23		// 0x17 ASCII \x17
	beq         t0, t1, .UsesFixedRegister_label_62

	// *** Basic block 1

	li          t1, 24		// 0x18 ASCII \x18
	beq         t0, t1, .UsesFixedRegister_label_61

	// *** Basic block 2

	li          t1, 25		// 0x19 ASCII \x19
	beq         t0, t1, .UsesFixedRegister_label_63

	// *** Basic block 3

	li          t1, 35		// 0x23 ASCII '#'
	beq         t0, t1, .UsesFixedRegister_label_60

	// *** Basic block 4

	li          t1, 129		// 0x81 ASCII \x81
	beq         t0, t1, .UsesFixedRegister_label_57

	// *** Basic block 5

	li          t1, 130		// 0x82 ASCII \x82
	beq         t0, t1, .UsesFixedRegister_label_58

	// *** Basic block 6

	li          t1, 131		// 0x83 ASCII \x83
	beq         t0, t1, .UsesFixedRegister_label_59

	// *** Basic block 7

.UsesFixedRegister_label_53:
	mv          a0, x0
	ret         

	// *** Basic block 8

.UsesFixedRegister_label_57:

	// *** Basic block 9

.UsesFixedRegister_label_58:

	// *** Basic block 10

.UsesFixedRegister_label_59:

	// *** Basic block 11

.UsesFixedRegister_label_60:

	// *** Basic block 12

.UsesFixedRegister_label_61:

	// *** Basic block 13

.UsesFixedRegister_label_62:

	// *** Basic block 14

.UsesFixedRegister_label_63:
	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 15

.UsesFixedRegister_label_66:
	ret         
.func_end_UsesFixedRegister:
	.size UsesFixedRegister, .func_end_UsesFixedRegister-UsesFixedRegister

	.local  AllocateForRmov
	.type AllocateForRmov, @function

AllocateForRmov:

	// *** Basic block 0

	.local UsesFixedRegister
	.local FreeRegisters
	addi sp, sp, -64
	// Saved return address (offset 56) and frame pointer (offset 48)
	sd ra, 56(sp)
	sd s0, 48(sp)
	addi s0, sp, 64
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 40(sp)
	sd s2, 32(sp)
	sd s3, 24(sp)
	sd s4, 16(sp)
	sd s5, 8(sp)
	// End of stack frame
	mv          s1, a1
	mv          s2, a0
	addi        t0, s1, 40
	ld          t1, 40(s1)
	ld          s3, 32(t1)
	ld          s4, 8(t0)
	addi        t0, s4, 64
	ld          t0, 8(t0)
	addi        t1, t0, -1
	seqz        s5, t1
	li          t1, 1		// 0x1 ASCII \x1
	bne         t0, t1, .AllocateForRmov_label_40

	// *** Basic block 1

	mv          a0, s4
	call        UsesFixedRegister

	// *** Basic block 2

	not         s5, a0

	// *** Basic block 3

.AllocateForRmov_label_40:
	beqz        s5, .AllocateForRmov_label_62

	// *** Basic block 4

	mv          a1, s1
	mv          a0, s2
	call        FreeRegisters

	// *** Basic block 5

	sd          s3, 32(s4)
	lw          t0, 88(s4)
	addi        t0, t0, 1
	sw          t0, 88(s4)
	sd          s4, 8(s3)
	ld          t0, 32(s4)
	sd          t0, 32(s1)

	// *** Basic block 6

.AllocateForRmov_label_59:
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 7

.AllocateForRmov_label_62:
	ld          t0, 40(s1)
	lw          t1, 88(t0)
	addi        t1, t1, 1
	sw          t1, 88(t0)
	mv          a1, s1
	mv          a0, s2
	call        FreeRegisters

	// *** Basic block 8

	addi        t0, s1, 64
	ld          t0, 8(t0)
	sw          t0, 88(s1)
	sd          s3, 32(s1)
	sd          s1, 8(s3)
	j           .AllocateForRmov_label_59
.func_end_AllocateForRmov:
	.size AllocateForRmov, .func_end_AllocateForRmov-AllocateForRmov

	.local  NeedsRegister
	.type NeedsRegister, @function

NeedsRegister:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	lw          t0, 16(a0)
	li          t1, 36		// 0x24 ASCII '$'
	blt         t0, t1, .NeedsRegister_label_111

	// *** Basic block 1

	beq         t0, t1, .NeedsRegister_label_192

	// *** Basic block 2

	li          t1, 37		// 0x25 ASCII '%'
	beq         t0, t1, .NeedsRegister_label_193

	// *** Basic block 3

	li          t1, 38		// 0x26 ASCII '&'
	beq         t0, t1, .NeedsRegister_label_194

	// *** Basic block 4

	li          t1, 39		// 0x27 ASCII '''
	beq         t0, t1, .NeedsRegister_label_195

	// *** Basic block 5

	li          t1, 40		// 0x28 ASCII '('
	beq         t0, t1, .NeedsRegister_label_196

	// *** Basic block 6

	li          t1, 41		// 0x29 ASCII ')'
	beq         t0, t1, .NeedsRegister_label_197

	// *** Basic block 7

	li          t1, 42		// 0x2a ASCII '*'
	beq         t0, t1, .NeedsRegister_label_198

	// *** Basic block 8

	li          t1, 43		// 0x2b ASCII '+'
	beq         t0, t1, .NeedsRegister_label_199

	// *** Basic block 9

	li          t1, 44		// 0x2c ASCII ','
	beq         t0, t1, .NeedsRegister_label_200

	// *** Basic block 10

	li          t1, 45		// 0x2d ASCII '-'
	beq         t0, t1, .NeedsRegister_label_201

	// *** Basic block 11

	li          t1, 110		// 0x6e ASCII 'n'
	beq         t0, t1, .NeedsRegister_label_188

	// *** Basic block 12

	li          t1, 111		// 0x6f ASCII 'o'
	beq         t0, t1, .NeedsRegister_label_187

	// *** Basic block 13

	li          t1, 112		// 0x70 ASCII 'p'
	beq         t0, t1, .NeedsRegister_label_189

	// *** Basic block 14

	li          t1, 113		// 0x71 ASCII 'q'
	beq         t0, t1, .NeedsRegister_label_190

	// *** Basic block 15

	j           .NeedsRegister_label_213

	// *** Basic block 16

.NeedsRegister_label_111:
	beqz        t0, .NeedsRegister_label_203

	// *** Basic block 17

	li          t1, 1		// 0x1 ASCII \x1
	beq         t0, t1, .NeedsRegister_label_204

	// *** Basic block 18

	li          t1, 2		// 0x2 ASCII \x2
	beq         t0, t1, .NeedsRegister_label_186

	// *** Basic block 19

	li          t1, 3		// 0x3 ASCII \x3
	beq         t0, t1, .NeedsRegister_label_205

	// *** Basic block 20

	li          t1, 5		// 0x5 ASCII \x5
	beq         t0, t1, .NeedsRegister_label_180

	// *** Basic block 21

	li          t1, 6		// 0x6 ASCII \x6
	beq         t0, t1, .NeedsRegister_label_181

	// *** Basic block 22

	li          t1, 7		// 0x7 ASCII \x7
	beq         t0, t1, .NeedsRegister_label_182

	// *** Basic block 23

	li          t1, 8		// 0x8 ASCII \x8
	beq         t0, t1, .NeedsRegister_label_183

	// *** Basic block 24

	li          t1, 9		// 0x9 ASCII \x9
	beq         t0, t1, .NeedsRegister_label_184

	// *** Basic block 25

	li          t1, 10		// 0xa ASCII \xa
	beq         t0, t1, .NeedsRegister_label_185

	// *** Basic block 26

	li          t1, 21		// 0x15 ASCII \x15
	beq         t0, t1, .NeedsRegister_label_202

	// *** Basic block 27

	li          t1, 22		// 0x16 ASCII \x16
	beq         t0, t1, .NeedsRegister_label_191

	// *** Basic block 28

	li          t1, 30		// 0x1e ASCII \x1e
	beq         t0, t1, .NeedsRegister_label_206

	// *** Basic block 29

	li          t1, 31		// 0x1f ASCII \x1f
	beq         t0, t1, .NeedsRegister_label_207

	// *** Basic block 30

	j           .NeedsRegister_label_213

	// *** Basic block 31

.NeedsRegister_label_180:

	// *** Basic block 32

.NeedsRegister_label_181:

	// *** Basic block 33

.NeedsRegister_label_182:

	// *** Basic block 34

.NeedsRegister_label_183:

	// *** Basic block 35

.NeedsRegister_label_184:

	// *** Basic block 36

.NeedsRegister_label_185:

	// *** Basic block 37

.NeedsRegister_label_186:

	// *** Basic block 38

.NeedsRegister_label_187:

	// *** Basic block 39

.NeedsRegister_label_188:

	// *** Basic block 40

.NeedsRegister_label_189:

	// *** Basic block 41

.NeedsRegister_label_190:

	// *** Basic block 42

.NeedsRegister_label_191:

	// *** Basic block 43

.NeedsRegister_label_192:

	// *** Basic block 44

.NeedsRegister_label_193:

	// *** Basic block 45

.NeedsRegister_label_194:

	// *** Basic block 46

.NeedsRegister_label_195:

	// *** Basic block 47

.NeedsRegister_label_196:

	// *** Basic block 48

.NeedsRegister_label_197:

	// *** Basic block 49

.NeedsRegister_label_198:

	// *** Basic block 50

.NeedsRegister_label_199:

	// *** Basic block 51

.NeedsRegister_label_200:

	// *** Basic block 52

.NeedsRegister_label_201:

	// *** Basic block 53

.NeedsRegister_label_202:

	// *** Basic block 54

.NeedsRegister_label_203:

	// *** Basic block 55

.NeedsRegister_label_204:

	// *** Basic block 56

.NeedsRegister_label_205:

	// *** Basic block 57

.NeedsRegister_label_206:

	// *** Basic block 58

.NeedsRegister_label_207:
	mv          a0, x0

	// *** Basic block 59

.NeedsRegister_label_210:
	ret         

	// *** Basic block 60

.NeedsRegister_label_213:
	li          a0, 1		// 0x1 ASCII \x1
	ret         
.func_end_NeedsRegister:
	.size NeedsRegister, .func_end_NeedsRegister-NeedsRegister

	.local  AllocateRegister
	.type AllocateRegister, @function

AllocateRegister:

	// *** Basic block 0

	.local AllocateForRmov
	.local FreeRegisters
	.local NeedsRegister
	.local AllocateRegisterWithType
	.local RegisterTypeFromInstruction
	.local FreeRegister
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 24(sp)
	sd s2, 16(sp)
	sd s3, 8(sp)
	sd s4, 0(sp)
	// End of stack frame
	mv          s1, a1
	mv          s2, a0
	lw          s3, 16(s1)
	addi        t1, s3, -18
	seqz        t0, t1
	li          t1, 18		// 0x12 ASCII \x12
	beq         s3, t1, .AllocateRegister_label_66

	// *** Basic block 1

	addi        t1, s3, -19
	seqz        t0, t1

	// *** Basic block 2

.AllocateRegister_label_66:
	bnez        t0, .AllocateRegister_label_71

	// *** Basic block 3

	addi        t1, s3, -20
	seqz        t0, t1

	// *** Basic block 4

.AllocateRegister_label_71:
	beqz        t0, .AllocateRegister_label_82

	// *** Basic block 5

	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           AllocateForRmov

	// *** Basic block 6

.AllocateRegister_label_79:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 7

.AllocateRegister_label_82:
	mv          a1, s1
	mv          a0, s2
	call        FreeRegisters

	// *** Basic block 8

	mv          a0, s1
	call        NeedsRegister

	// *** Basic block 9

	not         t0, a0
	beqz        t0, .AllocateRegister_label_94

	// *** Basic block 10

	j           .AllocateRegister_label_79

	// *** Basic block 11

.AllocateRegister_label_94:
	li          s4, 118		// 0x76 ASCII 'v'
	blt         s3, s4, .AllocateRegister_label_144

	// *** Basic block 12

	beq         s3, s4, .AllocateRegister_label_253

	// *** Basic block 13

	li          t0, 120		// 0x78 ASCII 'x'
	beq         s3, t0, .AllocateRegister_label_262

	// *** Basic block 14

	li          t0, 121		// 0x79 ASCII 'y'
	beq         s3, t0, .AllocateRegister_label_263

	// *** Basic block 15

	li          t0, 129		// 0x81 ASCII \x81
	beq         s3, t0, .AllocateRegister_label_222

	// *** Basic block 16

	li          t0, 130		// 0x82 ASCII \x82
	beq         s3, t0, .AllocateRegister_label_225

	// *** Basic block 17

	li          t0, 131		// 0x83 ASCII \x83
	beq         s3, t0, .AllocateRegister_label_229

	// *** Basic block 18

	li          t0, 132		// 0x84 ASCII \x84
	beq         s3, t0, .AllocateRegister_label_233

	// *** Basic block 19

	li          t0, 133		// 0x85 ASCII \x85
	beq         s3, t0, .AllocateRegister_label_236

	// *** Basic block 20

	li          t0, 134		// 0x86 ASCII \x86
	beq         s3, t0, .AllocateRegister_label_240

	// *** Basic block 21

	j           .AllocateRegister_label_271

	// *** Basic block 22

.AllocateRegister_label_144:
	li          t0, 23		// 0x17 ASCII \x17
	beq         s3, t0, .AllocateRegister_label_191

	// *** Basic block 23

	li          t0, 24		// 0x18 ASCII \x18
	beq         s3, t0, .AllocateRegister_label_196

	// *** Basic block 24

	li          t0, 25		// 0x19 ASCII \x19
	beq         s3, t0, .AllocateRegister_label_204

	// *** Basic block 25

	li          t0, 26		// 0x1a ASCII \x1a
	beq         s3, t0, .AllocateRegister_label_211

	// *** Basic block 26

	li          t0, 27		// 0x1b ASCII \x1b
	beq         s3, t0, .AllocateRegister_label_214

	// *** Basic block 27

	li          t0, 28		// 0x1c ASCII \x1c
	beq         s3, t0, .AllocateRegister_label_218

	// *** Basic block 28

	li          t0, 29		// 0x1d ASCII \x1d
	beq         s3, t0, .AllocateRegister_label_208

	// *** Basic block 29

	li          t0, 35		// 0x23 ASCII '#'
	beq         s3, t0, .AllocateRegister_label_200

	// *** Basic block 30

	li          t0, 115		// 0x73 ASCII 's'
	beq         s3, t0, .AllocateRegister_label_244

	// *** Basic block 31

	j           .AllocateRegister_label_271

	// *** Basic block 32

.AllocateRegister_label_191:
	addi        t0, s2, 8
	addi        s4, t0, 744
	j           .AllocateRegister_label_283

	// *** Basic block 33

.AllocateRegister_label_196:
	addi        t0, s2, 8
	addi        s4, t0, 720
	j           .AllocateRegister_label_283

	// *** Basic block 34

.AllocateRegister_label_200:
	addi        t0, s2, 8
	addi        s4, t0, 792
	j           .AllocateRegister_label_283

	// *** Basic block 35

.AllocateRegister_label_204:
	addi        t0, s2, 8
	addi        s4, t0, 816
	j           .AllocateRegister_label_283

	// *** Basic block 36

.AllocateRegister_label_208:
	addi        s4, s2, 8
	j           .AllocateRegister_label_283

	// *** Basic block 37

.AllocateRegister_label_211:
	addi        s4, s2, 8
	j           .AllocateRegister_label_283

	// *** Basic block 38

.AllocateRegister_label_214:
	li          t0, 6152		// 0x1808
	add         s4, s2, t0
	j           .AllocateRegister_label_283

	// *** Basic block 39

.AllocateRegister_label_218:
	li          t0, 12296		// 0x3008
	add         s4, s2, t0
	j           .AllocateRegister_label_283

	// *** Basic block 40

.AllocateRegister_label_222:
	addi        s4, s2, 8
	j           .AllocateRegister_label_283

	// *** Basic block 41

.AllocateRegister_label_225:
	li          t0, 6152		// 0x1808
	add         s4, s2, t0
	j           .AllocateRegister_label_283

	// *** Basic block 42

.AllocateRegister_label_229:
	li          t0, 12296		// 0x3008
	add         s4, s2, t0
	j           .AllocateRegister_label_283

	// *** Basic block 43

.AllocateRegister_label_233:
	addi        s4, s2, 8
	j           .AllocateRegister_label_283

	// *** Basic block 44

.AllocateRegister_label_236:
	li          t0, 6152		// 0x1808
	add         s4, s2, t0
	j           .AllocateRegister_label_283

	// *** Basic block 45

.AllocateRegister_label_240:
	li          t0, 12296		// 0x3008
	add         s4, s2, t0
	j           .AllocateRegister_label_283

	// *** Basic block 46

.AllocateRegister_label_244:
	li          t0, 2		// 0x2 ASCII \x2
	mv          a1, t0
	mv          a0, s2
	call        AllocateRegisterWithType

	// *** Basic block 47

	mv          s4, a0
	j           .AllocateRegister_label_283

	// *** Basic block 48

.AllocateRegister_label_253:
	li          t0, 1		// 0x1 ASCII \x1
	mv          a1, t0
	mv          a0, s2
	call        AllocateRegisterWithType

	// *** Basic block 49

	mv          s4, a0
	j           .AllocateRegister_label_283

	// *** Basic block 50

.AllocateRegister_label_262:

	// *** Basic block 51

.AllocateRegister_label_263:
	mv          a1, x0
	mv          a0, s2
	call        AllocateRegisterWithType

	// *** Basic block 52

	mv          s4, a0
	j           .AllocateRegister_label_283

	// *** Basic block 53

.AllocateRegister_label_271:
	mv          a0, s1
	call        RegisterTypeFromInstruction

	// *** Basic block 54

	mv          s3, a0
	mv          a1, s3
	mv          a0, s2
	call        AllocateRegisterWithType

	// *** Basic block 55

	mv          s4, a0

	// *** Basic block 56

.AllocateRegister_label_283:
	addi        t0, s1, 64
	ld          t0, 8(t0)
	sw          t0, 88(s1)
	sd          s4, 32(s1)
	sd          s1, 8(s4)
	lw          t0, 88(s1)
	bnez        t0, .AllocateRegister_label_302

	// *** Basic block 57

	mv          a1, s4
	mv          a0, s2
	call        FreeRegister

	// *** Basic block 58

.AllocateRegister_label_302:
	j           .AllocateRegister_label_79
.func_end_AllocateRegister:
	.size AllocateRegister, .func_end_AllocateRegister-AllocateRegister

	.global PCodeRegisterName
	.type PCodeRegisterName, @function

PCodeRegisterName:

	// *** Basic block 0

	.global snprintf
	addi sp, sp, -48
	// Saved return address (offset 40) and frame pointer (offset 32)
	sd ra, 40(sp)
	sd s0, 32(sp)
	addi s0, sp, 48
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 24(sp)
	sd s2, 16(sp)
	sd s3, 8(sp)
	sd s4, 0(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	mv          s3, a2
	lw          t0, 16(s1)
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 1

	j           .PCodeRegisterName_label_35

	// *** Basic block 2

	j           .PCodeRegisterName_label_109

	// *** Basic block 3

	j           .PCodeRegisterName_label_121

	// *** Basic block 4

.PCodeRegisterName_label_35:
	lw          s4, 0(s1)
	li          t0, 30		// 0x1e ASCII \x1e
	bne         s4, t0, .PCodeRegisterName_label_51

	// *** Basic block 5

	lla         a2, .str.10
	mv          a1, s3
	mv          a0, s2
	call        snprintf

	// *** Basic block 6

	j           .PCodeRegisterName_label_133

	// *** Basic block 7

.PCodeRegisterName_label_51:
	li          t0, 31		// 0x1f ASCII \x1f
	bne         s4, t0, .PCodeRegisterName_label_66

	// *** Basic block 8

	lla         a2, .str.11
	mv          a1, s3
	mv          a0, s2
	call        snprintf

	// *** Basic block 9

	j           .PCodeRegisterName_label_133

	// *** Basic block 10

.PCodeRegisterName_label_66:
	li          t0, 33		// 0x21 ASCII '!'
	bne         s4, t0, .PCodeRegisterName_label_81

	// *** Basic block 11

	lla         a2, .str.12
	mv          a1, s3
	mv          a0, s2
	call        snprintf

	// *** Basic block 12

	j           .PCodeRegisterName_label_133

	// *** Basic block 13

.PCodeRegisterName_label_81:
	li          t0, 34		// 0x22 ASCII '"'
	bne         s4, t0, .PCodeRegisterName_label_96

	// *** Basic block 14

	lla         a2, .str.13
	mv          a1, s3
	mv          a0, s2
	call        snprintf

	// *** Basic block 15

	j           .PCodeRegisterName_label_133

	// *** Basic block 16

.PCodeRegisterName_label_96:
	lla         a2, .str.14
	mv          a3, s4
	mv          a1, s3
	mv          a0, s2
	call        snprintf

	// *** Basic block 17

	j           .PCodeRegisterName_label_133

	// *** Basic block 18

.PCodeRegisterName_label_109:
	lla         a2, .str.15
	lw          a3, 0(s1)
	mv          a1, s3
	mv          a0, s2
	call        snprintf

	// *** Basic block 19

	j           .PCodeRegisterName_label_133

	// *** Basic block 20

.PCodeRegisterName_label_121:
	lla         a2, .str.16
	lw          a3, 0(s1)
	mv          a1, s3
	mv          a0, s2
	call        snprintf

	// *** Basic block 21

	j           .PCodeRegisterName_label_133

	// *** Basic block 22

.PCodeRegisterName_label_133:
	mv          a0, s2

	// *** Basic block 23

.PCodeRegisterName_label_136:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_PCodeRegisterName:
	.size PCodeRegisterName, .func_end_PCodeRegisterName-PCodeRegisterName

	.global PCodeAllocateRegisters
	.type PCodeAllocateRegisters, @function

PCodeAllocateRegisters:

	// *** Basic block 0

	.global TargetFirstInstruction
	.local AllocateRegister
	.global TargetNext
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	sd s2, 0(sp)
	// End of stack frame
	mv          s1, a0
	ld          a0, 0(s1)
	call        TargetFirstInstruction

	// *** Basic block 1

	mv          s2, a0
	beq         s2, x0, .PCodeAllocateRegisters_label_32

	// *** Basic block 2

.PCodeAllocateRegisters_label_18:
	mv          a1, s2
	mv          a0, s1
	call        AllocateRegister

	// *** Basic block 3

	mv          a0, s2
	call        TargetNext

	// *** Basic block 4

	mv          s2, a0
	bne         s2, x0, .PCodeAllocateRegisters_label_18

	// *** Basic block 5

.PCodeAllocateRegisters_label_32:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_PCodeAllocateRegisters:
	.size PCodeAllocateRegisters, .func_end_PCodeAllocateRegisters-PCodeAllocateRegisters

.PCend:
	.data
	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.1, @object
	.size .str.1, 30

.str.2:
	.asciz "(null)"
	.type .str.2, @object
	.size .str.2, 1

.str.3:
	.asciz "false"
	.type .str.3, @object
	.size .str.3, 6

.str.4:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.4, @object
	.size .str.4, 30

.str.5:
	.asciz "(null)"
	.type .str.5, @object
	.size .str.5, 1

.str.6:
	.asciz "op->uses >= 0"
	.type .str.6, @object
	.size .str.6, 14

.str.7:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.7, @object
	.size .str.7, 30

.str.8:
	.asciz "(null)"
	.type .str.8, @object
	.size .str.8, 1

.str.9:
	.asciz "reg != NULL"
	.type .str.9, @object
	.size .str.9, 12

.str.10:
	.asciz "sp"
	.type .str.10, @object
	.size .str.10, 3

.str.11:
	.asciz "fp"
	.type .str.11, @object
	.size .str.11, 3

.str.12:
	.asciz "ap"
	.type .str.12, @object
	.size .str.12, 3

.str.13:
	.asciz "tp"
	.type .str.13, @object
	.size .str.13, 3

.str.14:
	.asciz "r%d"
	.type .str.14, @object
	.size .str.14, 4

.str.15:
	.asciz "f%d"
	.type .str.15, @object
	.size .str.15, 4

.str.16:
	.asciz "d%d"
	.type .str.16, @object
	.size .str.16, 4


	.file   "p_code_codegen.c"
	.text
	.option pic
.PCbegin:
	.global PCodeOpcodeName
	.type PCodeOpcodeName, @function

PCodeOpcodeName:

	// *** Basic block 0

	.global TargetOpcodeName
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	li          t1, 21		// 0x15 ASCII \x15
	blt         t0, t1, .PCodeOpcodeName_label_239

	// *** Basic block 1

	li          t1, 134		// 0x86 ASCII \x86
	blt         t1, t0, .PCodeOpcodeName_label_239

	// *** Basic block 2

	addi        t1, t0, -21
	slli        t1, t1, 2
	auipc       t2, 0
	add         t1, t2, t1
	jalr        x0, t1, 12

	// *** Basic block 3

	j           .PCodeOpcodeName_label_749

	// *** Basic block 4

	j           .PCodeOpcodeName_label_239

	// *** Basic block 5

	j           .PCodeOpcodeName_label_239

	// *** Basic block 6

	j           .PCodeOpcodeName_label_239

	// *** Basic block 7

	j           .PCodeOpcodeName_label_239

	// *** Basic block 8

	j           .PCodeOpcodeName_label_239

	// *** Basic block 9

	j           .PCodeOpcodeName_label_239

	// *** Basic block 10

	j           .PCodeOpcodeName_label_239

	// *** Basic block 11

	j           .PCodeOpcodeName_label_239

	// *** Basic block 12

	j           .PCodeOpcodeName_label_239

	// *** Basic block 13

	j           .PCodeOpcodeName_label_239

	// *** Basic block 14

	j           .PCodeOpcodeName_label_239

	// *** Basic block 15

	j           .PCodeOpcodeName_label_239

	// *** Basic block 16

	j           .PCodeOpcodeName_label_239

	// *** Basic block 17

	j           .PCodeOpcodeName_label_249

	// *** Basic block 18

	j           .PCodeOpcodeName_label_254

	// *** Basic block 19

	j           .PCodeOpcodeName_label_259

	// *** Basic block 20

	j           .PCodeOpcodeName_label_264

	// *** Basic block 21

	j           .PCodeOpcodeName_label_269

	// *** Basic block 22

	j           .PCodeOpcodeName_label_274

	// *** Basic block 23

	j           .PCodeOpcodeName_label_279

	// *** Basic block 24

	j           .PCodeOpcodeName_label_284

	// *** Basic block 25

	j           .PCodeOpcodeName_label_289

	// *** Basic block 26

	j           .PCodeOpcodeName_label_294

	// *** Basic block 27

	j           .PCodeOpcodeName_label_299

	// *** Basic block 28

	j           .PCodeOpcodeName_label_304

	// *** Basic block 29

	j           .PCodeOpcodeName_label_309

	// *** Basic block 30

	j           .PCodeOpcodeName_label_339

	// *** Basic block 31

	j           .PCodeOpcodeName_label_314

	// *** Basic block 32

	j           .PCodeOpcodeName_label_319

	// *** Basic block 33

	j           .PCodeOpcodeName_label_324

	// *** Basic block 34

	j           .PCodeOpcodeName_label_344

	// *** Basic block 35

	j           .PCodeOpcodeName_label_329

	// *** Basic block 36

	j           .PCodeOpcodeName_label_334

	// *** Basic block 37

	j           .PCodeOpcodeName_label_349

	// *** Basic block 38

	j           .PCodeOpcodeName_label_354

	// *** Basic block 39

	j           .PCodeOpcodeName_label_359

	// *** Basic block 40

	j           .PCodeOpcodeName_label_364

	// *** Basic block 41

	j           .PCodeOpcodeName_label_369

	// *** Basic block 42

	j           .PCodeOpcodeName_label_374

	// *** Basic block 43

	j           .PCodeOpcodeName_label_379

	// *** Basic block 44

	j           .PCodeOpcodeName_label_384

	// *** Basic block 45

	j           .PCodeOpcodeName_label_389

	// *** Basic block 46

	j           .PCodeOpcodeName_label_394

	// *** Basic block 47

	j           .PCodeOpcodeName_label_399

	// *** Basic block 48

	j           .PCodeOpcodeName_label_404

	// *** Basic block 49

	j           .PCodeOpcodeName_label_409

	// *** Basic block 50

	j           .PCodeOpcodeName_label_414

	// *** Basic block 51

	j           .PCodeOpcodeName_label_419

	// *** Basic block 52

	j           .PCodeOpcodeName_label_424

	// *** Basic block 53

	j           .PCodeOpcodeName_label_429

	// *** Basic block 54

	j           .PCodeOpcodeName_label_434

	// *** Basic block 55

	j           .PCodeOpcodeName_label_439

	// *** Basic block 56

	j           .PCodeOpcodeName_label_444

	// *** Basic block 57

	j           .PCodeOpcodeName_label_449

	// *** Basic block 58

	j           .PCodeOpcodeName_label_454

	// *** Basic block 59

	j           .PCodeOpcodeName_label_459

	// *** Basic block 60

	j           .PCodeOpcodeName_label_464

	// *** Basic block 61

	j           .PCodeOpcodeName_label_469

	// *** Basic block 62

	j           .PCodeOpcodeName_label_474

	// *** Basic block 63

	j           .PCodeOpcodeName_label_479

	// *** Basic block 64

	j           .PCodeOpcodeName_label_484

	// *** Basic block 65

	j           .PCodeOpcodeName_label_489

	// *** Basic block 66

	j           .PCodeOpcodeName_label_494

	// *** Basic block 67

	j           .PCodeOpcodeName_label_499

	// *** Basic block 68

	j           .PCodeOpcodeName_label_504

	// *** Basic block 69

	j           .PCodeOpcodeName_label_509

	// *** Basic block 70

	j           .PCodeOpcodeName_label_514

	// *** Basic block 71

	j           .PCodeOpcodeName_label_519

	// *** Basic block 72

	j           .PCodeOpcodeName_label_524

	// *** Basic block 73

	j           .PCodeOpcodeName_label_529

	// *** Basic block 74

	j           .PCodeOpcodeName_label_534

	// *** Basic block 75

	j           .PCodeOpcodeName_label_539

	// *** Basic block 76

	j           .PCodeOpcodeName_label_544

	// *** Basic block 77

	j           .PCodeOpcodeName_label_549

	// *** Basic block 78

	j           .PCodeOpcodeName_label_554

	// *** Basic block 79

	j           .PCodeOpcodeName_label_559

	// *** Basic block 80

	j           .PCodeOpcodeName_label_564

	// *** Basic block 81

	j           .PCodeOpcodeName_label_569

	// *** Basic block 82

	j           .PCodeOpcodeName_label_574

	// *** Basic block 83

	j           .PCodeOpcodeName_label_579

	// *** Basic block 84

	j           .PCodeOpcodeName_label_584

	// *** Basic block 85

	j           .PCodeOpcodeName_label_589

	// *** Basic block 86

	j           .PCodeOpcodeName_label_594

	// *** Basic block 87

	j           .PCodeOpcodeName_label_599

	// *** Basic block 88

	j           .PCodeOpcodeName_label_604

	// *** Basic block 89

	j           .PCodeOpcodeName_label_609

	// *** Basic block 90

	j           .PCodeOpcodeName_label_614

	// *** Basic block 91

	j           .PCodeOpcodeName_label_619

	// *** Basic block 92

	j           .PCodeOpcodeName_label_624

	// *** Basic block 93

	j           .PCodeOpcodeName_label_629

	// *** Basic block 94

	j           .PCodeOpcodeName_label_634

	// *** Basic block 95

	j           .PCodeOpcodeName_label_639

	// *** Basic block 96

	j           .PCodeOpcodeName_label_644

	// *** Basic block 97

	j           .PCodeOpcodeName_label_649

	// *** Basic block 98

	j           .PCodeOpcodeName_label_654

	// *** Basic block 99

	j           .PCodeOpcodeName_label_659

	// *** Basic block 100

	j           .PCodeOpcodeName_label_664

	// *** Basic block 101

	j           .PCodeOpcodeName_label_669

	// *** Basic block 102

	j           .PCodeOpcodeName_label_674

	// *** Basic block 103

	j           .PCodeOpcodeName_label_679

	// *** Basic block 104

	j           .PCodeOpcodeName_label_684

	// *** Basic block 105

	j           .PCodeOpcodeName_label_689

	// *** Basic block 106

	j           .PCodeOpcodeName_label_694

	// *** Basic block 107

	j           .PCodeOpcodeName_label_699

	// *** Basic block 108

	j           .PCodeOpcodeName_label_704

	// *** Basic block 109

	j           .PCodeOpcodeName_label_709

	// *** Basic block 110

	j           .PCodeOpcodeName_label_714

	// *** Basic block 111

	j           .PCodeOpcodeName_label_719

	// *** Basic block 112

	j           .PCodeOpcodeName_label_724

	// *** Basic block 113

	j           .PCodeOpcodeName_label_729

	// *** Basic block 114

	j           .PCodeOpcodeName_label_734

	// *** Basic block 115

	j           .PCodeOpcodeName_label_739

	// *** Basic block 116

	j           .PCodeOpcodeName_label_744

	// *** Basic block 117

.PCodeOpcodeName_label_239:
	mv          a0, t0
	j           TargetOpcodeName

	// *** Basic block 120

.PCodeOpcodeName_label_249:
	lla         a0, .str.1
	ret         

	// *** Basic block 121

.PCodeOpcodeName_label_254:
	lla         a0, .str.2
	ret         

	// *** Basic block 122

.PCodeOpcodeName_label_259:
	lla         a0, .str.3
	ret         

	// *** Basic block 123

.PCodeOpcodeName_label_264:
	lla         a0, .str.4
	ret         

	// *** Basic block 124

.PCodeOpcodeName_label_269:
	lla         a0, .str.5
	ret         

	// *** Basic block 125

.PCodeOpcodeName_label_274:
	lla         a0, .str.6
	ret         

	// *** Basic block 126

.PCodeOpcodeName_label_279:
	lla         a0, .str.7
	ret         

	// *** Basic block 127

.PCodeOpcodeName_label_284:
	lla         a0, .str.8
	ret         

	// *** Basic block 128

.PCodeOpcodeName_label_289:
	lla         a0, .str.9
	ret         

	// *** Basic block 129

.PCodeOpcodeName_label_294:
	lla         a0, .str.10
	ret         

	// *** Basic block 130

.PCodeOpcodeName_label_299:
	lla         a0, .str.11
	ret         

	// *** Basic block 131

.PCodeOpcodeName_label_304:
	lla         a0, .str.12
	ret         

	// *** Basic block 132

.PCodeOpcodeName_label_309:
	lla         a0, .str.13
	ret         

	// *** Basic block 133

.PCodeOpcodeName_label_314:
	lla         a0, .str.14
	ret         

	// *** Basic block 134

.PCodeOpcodeName_label_319:
	lla         a0, .str.15
	ret         

	// *** Basic block 135

.PCodeOpcodeName_label_324:
	lla         a0, .str.16
	ret         

	// *** Basic block 136

.PCodeOpcodeName_label_329:
	lla         a0, .str.17
	ret         

	// *** Basic block 137

.PCodeOpcodeName_label_334:
	lla         a0, .str.18
	ret         

	// *** Basic block 138

.PCodeOpcodeName_label_339:
	lla         a0, .str.19
	ret         

	// *** Basic block 139

.PCodeOpcodeName_label_344:
	lla         a0, .str.20
	ret         

	// *** Basic block 140

.PCodeOpcodeName_label_349:
	lla         a0, .str.21
	ret         

	// *** Basic block 141

.PCodeOpcodeName_label_354:
	lla         a0, .str.22
	ret         

	// *** Basic block 142

.PCodeOpcodeName_label_359:
	lla         a0, .str.23
	ret         

	// *** Basic block 143

.PCodeOpcodeName_label_364:
	lla         a0, .str.24
	ret         

	// *** Basic block 144

.PCodeOpcodeName_label_369:
	lla         a0, .str.25
	ret         

	// *** Basic block 145

.PCodeOpcodeName_label_374:
	lla         a0, .str.26
	ret         

	// *** Basic block 146

.PCodeOpcodeName_label_379:
	lla         a0, .str.27
	ret         

	// *** Basic block 147

.PCodeOpcodeName_label_384:
	lla         a0, .str.28
	ret         

	// *** Basic block 148

.PCodeOpcodeName_label_389:
	lla         a0, .str.29
	ret         

	// *** Basic block 149

.PCodeOpcodeName_label_394:
	lla         a0, .str.30
	ret         

	// *** Basic block 150

.PCodeOpcodeName_label_399:
	lla         a0, .str.31
	ret         

	// *** Basic block 151

.PCodeOpcodeName_label_404:
	lla         a0, .str.32
	ret         

	// *** Basic block 152

.PCodeOpcodeName_label_409:
	lla         a0, .str.33
	ret         

	// *** Basic block 153

.PCodeOpcodeName_label_414:
	lla         a0, .str.34
	ret         

	// *** Basic block 154

.PCodeOpcodeName_label_419:
	lla         a0, .str.35
	ret         

	// *** Basic block 155

.PCodeOpcodeName_label_424:
	lla         a0, .str.36
	ret         

	// *** Basic block 156

.PCodeOpcodeName_label_429:
	lla         a0, .str.37
	ret         

	// *** Basic block 157

.PCodeOpcodeName_label_434:
	lla         a0, .str.38
	ret         

	// *** Basic block 158

.PCodeOpcodeName_label_439:
	lla         a0, .str.39
	ret         

	// *** Basic block 159

.PCodeOpcodeName_label_444:
	lla         a0, .str.40
	ret         

	// *** Basic block 160

.PCodeOpcodeName_label_449:
	lla         a0, .str.41
	ret         

	// *** Basic block 161

.PCodeOpcodeName_label_454:
	lla         a0, .str.42
	ret         

	// *** Basic block 162

.PCodeOpcodeName_label_459:
	lla         a0, .str.43
	ret         

	// *** Basic block 163

.PCodeOpcodeName_label_464:
	lla         a0, .str.44
	ret         

	// *** Basic block 164

.PCodeOpcodeName_label_469:
	lla         a0, .str.45
	ret         

	// *** Basic block 165

.PCodeOpcodeName_label_474:
	lla         a0, .str.46
	ret         

	// *** Basic block 166

.PCodeOpcodeName_label_479:
	lla         a0, .str.47
	ret         

	// *** Basic block 167

.PCodeOpcodeName_label_484:
	lla         a0, .str.48
	ret         

	// *** Basic block 168

.PCodeOpcodeName_label_489:
	lla         a0, .str.49
	ret         

	// *** Basic block 169

.PCodeOpcodeName_label_494:
	lla         a0, .str.50
	ret         

	// *** Basic block 170

.PCodeOpcodeName_label_499:
	lla         a0, .str.51
	ret         

	// *** Basic block 171

.PCodeOpcodeName_label_504:
	lla         a0, .str.52
	ret         

	// *** Basic block 172

.PCodeOpcodeName_label_509:
	lla         a0, .str.53
	ret         

	// *** Basic block 173

.PCodeOpcodeName_label_514:
	lla         a0, .str.54
	ret         

	// *** Basic block 174

.PCodeOpcodeName_label_519:
	lla         a0, .str.55
	ret         

	// *** Basic block 175

.PCodeOpcodeName_label_524:
	lla         a0, .str.56
	ret         

	// *** Basic block 176

.PCodeOpcodeName_label_529:
	lla         a0, .str.57
	ret         

	// *** Basic block 177

.PCodeOpcodeName_label_534:
	lla         a0, .str.58
	ret         

	// *** Basic block 178

.PCodeOpcodeName_label_539:
	lla         a0, .str.59
	ret         

	// *** Basic block 179

.PCodeOpcodeName_label_544:
	lla         a0, .str.60
	ret         

	// *** Basic block 180

.PCodeOpcodeName_label_549:
	lla         a0, .str.61
	ret         

	// *** Basic block 181

.PCodeOpcodeName_label_554:
	lla         a0, .str.62
	ret         

	// *** Basic block 182

.PCodeOpcodeName_label_559:
	lla         a0, .str.63
	ret         

	// *** Basic block 183

.PCodeOpcodeName_label_564:
	lla         a0, .str.64
	ret         

	// *** Basic block 184

.PCodeOpcodeName_label_569:
	lla         a0, .str.65
	ret         

	// *** Basic block 185

.PCodeOpcodeName_label_574:
	lla         a0, .str.66
	ret         

	// *** Basic block 186

.PCodeOpcodeName_label_579:
	lla         a0, .str.67
	ret         

	// *** Basic block 187

.PCodeOpcodeName_label_584:
	lla         a0, .str.68
	ret         

	// *** Basic block 188

.PCodeOpcodeName_label_589:
	lla         a0, .str.69
	ret         

	// *** Basic block 189

.PCodeOpcodeName_label_594:
	lla         a0, .str.70
	ret         

	// *** Basic block 190

.PCodeOpcodeName_label_599:
	lla         a0, .str.71
	ret         

	// *** Basic block 191

.PCodeOpcodeName_label_604:
	lla         a0, .str.72
	ret         

	// *** Basic block 192

.PCodeOpcodeName_label_609:
	lla         a0, .str.73
	ret         

	// *** Basic block 193

.PCodeOpcodeName_label_614:
	lla         a0, .str.74
	ret         

	// *** Basic block 194

.PCodeOpcodeName_label_619:
	lla         a0, .str.75
	ret         

	// *** Basic block 195

.PCodeOpcodeName_label_624:
	lla         a0, .str.76
	ret         

	// *** Basic block 196

.PCodeOpcodeName_label_629:
	lla         a0, .str.77
	ret         

	// *** Basic block 197

.PCodeOpcodeName_label_634:
	lla         a0, .str.78
	ret         

	// *** Basic block 198

.PCodeOpcodeName_label_639:
	lla         a0, .str.79
	ret         

	// *** Basic block 199

.PCodeOpcodeName_label_644:
	lla         a0, .str.80
	ret         

	// *** Basic block 200

.PCodeOpcodeName_label_649:
	lla         a0, .str.81
	ret         

	// *** Basic block 201

.PCodeOpcodeName_label_654:
	lla         a0, .str.82
	ret         

	// *** Basic block 202

.PCodeOpcodeName_label_659:
	lla         a0, .str.83
	ret         

	// *** Basic block 203

.PCodeOpcodeName_label_664:
	lla         a0, .str.84
	ret         

	// *** Basic block 204

.PCodeOpcodeName_label_669:
	lla         a0, .str.85
	ret         

	// *** Basic block 205

.PCodeOpcodeName_label_674:
	lla         a0, .str.86
	ret         

	// *** Basic block 206

.PCodeOpcodeName_label_679:
	lla         a0, .str.87
	ret         

	// *** Basic block 207

.PCodeOpcodeName_label_684:
	lla         a0, .str.88
	ret         

	// *** Basic block 208

.PCodeOpcodeName_label_689:
	lla         a0, .str.89
	ret         

	// *** Basic block 209

.PCodeOpcodeName_label_694:
	lla         a0, .str.90
	ret         

	// *** Basic block 210

.PCodeOpcodeName_label_699:
	lla         a0, .str.91
	ret         

	// *** Basic block 211

.PCodeOpcodeName_label_704:
	lla         a0, .str.92
	ret         

	// *** Basic block 212

.PCodeOpcodeName_label_709:
	lla         a0, .str.93
	ret         

	// *** Basic block 213

.PCodeOpcodeName_label_714:
	lla         a0, .str.94
	ret         

	// *** Basic block 214

.PCodeOpcodeName_label_719:
	lla         a0, .str.95
	ret         

	// *** Basic block 215

.PCodeOpcodeName_label_724:
	lla         a0, .str.96
	ret         

	// *** Basic block 216

.PCodeOpcodeName_label_729:
	lla         a0, .str.97
	ret         

	// *** Basic block 217

.PCodeOpcodeName_label_734:
	lla         a0, .str.98
	ret         

	// *** Basic block 218

.PCodeOpcodeName_label_739:
	lla         a0, .str.99
	ret         

	// *** Basic block 219

.PCodeOpcodeName_label_744:
	lla         a0, .str.100
	ret         

	// *** Basic block 220

.PCodeOpcodeName_label_749:
	lla         a0, .str.101
	ret         
.func_end_PCodeOpcodeName:
	.size PCodeOpcodeName, .func_end_PCodeOpcodeName-PCodeOpcodeName

	.local  ArgumentPointer
	.type ArgumentPointer, @function

ArgumentPointer:

	// *** Basic block 0

	.global TargetEmit
	.global TargetNewInstruction
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
	ld          t0, 184(s1)
	bne         t0, x0, .ArgumentPointer_label_28

	// *** Basic block 1

	li          t0, 35		// 0x23 ASCII '#'
	mv          a0, t0
	call        TargetNewInstruction

	// *** Basic block 2

	mv          a1, a0
	mv          a0, s1
	call        TargetEmit

	// *** Basic block 3

	sd          a0, 184(s1)

	// *** Basic block 4

.ArgumentPointer_label_28:
	ld          a0, 184(s1)

	// *** Basic block 5

.ArgumentPointer_label_32:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ArgumentPointer:
	.size ArgumentPointer, .func_end_ArgumentPointer-ArgumentPointer

	.global PCodeGeneratorInit
	.type PCodeGeneratorInit, @function

PCodeGeneratorInit:

	// *** Basic block 0

	.global TargetGeneratorInit
	.global PCodeRegisterAllocatorInit
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
	call        TargetGeneratorInit

	// *** Basic block 1

	sd          x0, 184(s1)
	addi        a0, s1, 192
	mv          a1, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           PCodeRegisterAllocatorInit
.func_end_PCodeGeneratorInit:
	.size PCodeGeneratorInit, .func_end_PCodeGeneratorInit-PCodeGeneratorInit

	.global NewPCodeGenerator
	.type NewPCodeGenerator, @function

NewPCodeGenerator:

	// *** Basic block 0

	.global malloc
	.global PCodeGeneratorInit
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
	li          a0, 18680		// 0x48f8
	call        malloc

	// *** Basic block 1

	mv          s2, a0
	mv          a1, s1
	mv          a0, s2
	call        PCodeGeneratorInit

	// *** Basic block 2

	mv          a0, s2

	// *** Basic block 3

.NewPCodeGenerator_label_21:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewPCodeGenerator:
	.size NewPCodeGenerator, .func_end_NewPCodeGenerator-NewPCodeGenerator

	.global PCodeGeneratorDestruct
	.type PCodeGeneratorDestruct, @function

PCodeGeneratorDestruct:

	// *** Basic block 0

	.global TargetGeneratorDestruct
	.global PCodeRegisterAllocatorDestruct
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
	call        TargetGeneratorDestruct

	// *** Basic block 1

	addi        a0, s1, 192
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           PCodeRegisterAllocatorDestruct
.func_end_PCodeGeneratorDestruct:
	.size PCodeGeneratorDestruct, .func_end_PCodeGeneratorDestruct-PCodeGeneratorDestruct

	.global PCodeGeneratorDelete
	.type PCodeGeneratorDelete, @function

PCodeGeneratorDelete:

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
	.global PCodeGeneratorDestruct
	.global free
	mv          s1, a0
	call        PCodeGeneratorDestruct

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_PCodeGeneratorDelete:
	.size PCodeGeneratorDelete, .func_end_PCodeGeneratorDelete-PCodeGeneratorDelete

	.local  NewInstruction1
	.type NewInstruction1, @function

NewInstruction1:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.global TargetNewInstruction1
	j           TargetNewInstruction1
.func_end_NewInstruction1:
	.size NewInstruction1, .func_end_NewInstruction1-NewInstruction1

	.local  NewInstruction2
	.type NewInstruction2, @function

NewInstruction2:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.global TargetNewInstruction2
	j           TargetNewInstruction2
.func_end_NewInstruction2:
	.size NewInstruction2, .func_end_NewInstruction2-NewInstruction2

	.local  NewInstruction3
	.type NewInstruction3, @function

NewInstruction3:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.global TargetNewInstruction3
	j           TargetNewInstruction3
.func_end_NewInstruction3:
	.size NewInstruction3, .func_end_NewInstruction3-NewInstruction3

	.local  Emit
	.type Emit, @function

Emit:

	// *** Basic block 0

	.global TargetEmit
	// Leaf procedure, no stack frame generated
	j           TargetEmit
.func_end_Emit:
	.size Emit, .func_end_Emit-Emit

	.local  EmitBefore
	.type EmitBefore, @function

EmitBefore:

	// *** Basic block 0

	.global TargetEmitBefore
	// Leaf procedure, no stack frame generated
	j           TargetEmitBefore
.func_end_EmitBefore:
	.size EmitBefore, .func_end_EmitBefore-EmitBefore

	.local  EmitAfter
	.type EmitAfter, @function

EmitAfter:

	// *** Basic block 0

	.global TargetEmitAfter
	// Leaf procedure, no stack frame generated
	j           TargetEmitAfter
.func_end_EmitAfter:
	.size EmitAfter, .func_end_EmitAfter-EmitAfter

	.local  EmitConstant
	.type EmitConstant, @function

EmitConstant:

	// *** Basic block 0

	.global TargetEmitConstant
	// Leaf procedure, no stack frame generated
	j           TargetEmitConstant
.func_end_EmitConstant:
	.size EmitConstant, .func_end_EmitConstant-EmitConstant

	.local  EmitSymbol
	.type EmitSymbol, @function

EmitSymbol:

	// *** Basic block 0

	.global TargetEmitSymbol
	// Leaf procedure, no stack frame generated
	j           TargetEmitSymbol
.func_end_EmitSymbol:
	.size EmitSymbol, .func_end_EmitSymbol-EmitSymbol

	.local  FramePointer
	.type FramePointer, @function

FramePointer:

	// *** Basic block 0

	.global TargetFramePointer
	// Leaf procedure, no stack frame generated
	j           TargetFramePointer
.func_end_FramePointer:
	.size FramePointer, .func_end_FramePointer-FramePointer

	.local  StackPointer
	.type StackPointer, @function

StackPointer:

	// *** Basic block 0

	.global TargetStackPointer
	// Leaf procedure, no stack frame generated
	j           TargetStackPointer
.func_end_StackPointer:
	.size StackPointer, .func_end_StackPointer-StackPointer

	.local  ThreadPointer
	.type ThreadPointer, @function

ThreadPointer:

	// *** Basic block 0

	.global TargetThreadPointer
	// Leaf procedure, no stack frame generated
	j           TargetThreadPointer
.func_end_ThreadPointer:
	.size ThreadPointer, .func_end_ThreadPointer-ThreadPointer

	.local  GetLoweredNode
	.type GetLoweredNode, @function

GetLoweredNode:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.global TargetGetLoweredNode
	j           TargetGetLoweredNode
.func_end_GetLoweredNode:
	.size GetLoweredNode, .func_end_GetLoweredNode-GetLoweredNode

	.local  SetLoweredNode
	.type SetLoweredNode, @function

SetLoweredNode:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.global TargetSetLoweredNode
	j           TargetSetLoweredNode
.func_end_SetLoweredNode:
	.size SetLoweredNode, .func_end_SetLoweredNode-SetLoweredNode

	.local  GetIntConstant
	.type GetIntConstant, @function

GetIntConstant:

	// *** Basic block 0

	.global TargetGetIntConstant
	// Leaf procedure, no stack frame generated
	j           TargetGetIntConstant
.func_end_GetIntConstant:
	.size GetIntConstant, .func_end_GetIntConstant-GetIntConstant

	.local  GetFloatingPointConstant
	.type GetFloatingPointConstant, @function

GetFloatingPointConstant:

	// *** Basic block 0

	.global TargetGetFloatingPointConstant
	// Leaf procedure, no stack frame generated
	fmv.d       ft0, fa0
	fmv.d       fa0, ft0
	j           TargetGetFloatingPointConstant
.func_end_GetFloatingPointConstant:
	.size GetFloatingPointConstant, .func_end_GetFloatingPointConstant-GetFloatingPointConstant

	.local  GetSymbol
	.type GetSymbol, @function

GetSymbol:

	// *** Basic block 0

	.global TargetGetSymbol
	// Leaf procedure, no stack frame generated
	j           TargetGetSymbol
.func_end_GetSymbol:
	.size GetSymbol, .func_end_GetSymbol-GetSymbol

	.local  NewInstruction
	.type NewInstruction, @function

NewInstruction:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.global TargetNewInstruction
	j           TargetNewInstruction
.func_end_NewInstruction:
	.size NewInstruction, .func_end_NewInstruction-NewInstruction

	.local  LoadStaticVariable
	.type LoadStaticVariable, @function

LoadStaticVariable:

	// *** Basic block 0

	.global compiler
	.local Emit
	.local NewInstruction1
	.local GetLoweredNode
	.local NewInstruction2
	.local GetIntConstant
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
	la          t0, compiler
	ld          t0, 0(t0)
	lb          t0, 1230(t0)
	not         t0, t0
	beqz        t0, .LoadStaticVariable_label_48

	// *** Basic block 1

	mv          a0, s2
	call        GetLoweredNode

	// *** Basic block 2

	mv          a1, a0
	li          t0, 17		// 0x11 ASCII \x11
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 3

	mv          a1, a0
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           Emit

	// *** Basic block 6

.LoadStaticVariable_label_48:
	mv          a0, s2
	call        GetLoweredNode

	// *** Basic block 7

	mv          a1, a0
	li          t0, 126		// 0x7e ASCII '~'
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 8

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 9

	mv          s3, a0
	mv          a3, x0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	mv          a1, x0
	mv          a0, s1
	call        GetIntConstant

	// *** Basic block 10

	mv          a2, a0
	mv          a1, s3
	li          t0, 52		// 0x34 ASCII '4'
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 11

	mv          a1, a0
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           Emit
.func_end_LoadStaticVariable:
	.size LoadStaticVariable, .func_end_LoadStaticVariable-LoadStaticVariable

	.local  LoadVariableValue
	.type LoadVariableValue, @function

LoadVariableValue:

	// *** Basic block 0

	.local load_opcodes
	.global printf
	.global abort
	.local Emit
	.local NewInstruction2
	addi sp, sp, -80
	// Saved return address (offset 72) and frame pointer (offset 64)
	sd ra, 72(sp)
	sd s0, 64(sp)
	addi s0, sp, 80
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 56(sp)
	sd s2, 48(sp)
	sd s3, 40(sp)
	sd s4, 32(sp)
	sd s5, 24(sp)
	sd s6, 16(sp)
	sd s7, 8(sp)
	// End of stack frame
	mv          s1, a1
	mv          s2, a0
	mv          s3, a2
	mv          s4, a3
	li          s5, 46		// 0x2e ASCII '.'
	mv          s6, x0
	la          t0, load_opcodes
	ld          t0, 0(t0)
	beq         t0, x0, .LoadVariableValue_label_66

	// *** Basic block 1

	ld          s7, 80(s1)

	// *** Basic block 2

.LoadVariableValue_label_43:
	slli        t0, s6, 4
	la          t1, load_opcodes
	add         s1, t1, t0
	ld          t0, 0(s1)
	mv          a0, s7
	jalr         x1, t0, 0

	// *** Basic block 3

	beqz        a0, .LoadVariableValue_label_56

	// *** Basic block 4

	lw          s5, 8(s1)
	j           .LoadVariableValue_label_66

	// *** Basic block 5

.LoadVariableValue_label_56:

	// *** Basic block 6

.LoadVariableValue_label_57:
	addi        s6, s6, 1
	slli        t0, s6, 4
	la          t1, load_opcodes
	add         t0, t1, t0
	ld          t0, 0(t0)
	beq         t0, x0, .LoadVariableValue_label_43

	// *** Basic block 7

.LoadVariableValue_label_66:
	beqz        s5, .LoadVariableValue_label_70

	// *** Basic block 8

	j           .LoadVariableValue_label_85

	// *** Basic block 9

.LoadVariableValue_label_70:
	lla         a0, .str.102
	lla         a1, .str.103
	lla         a3, .str.104
	li          t0, 425		// 0x1a9
	mv          a2, t0
	call        printf

	// *** Basic block 10

	call        abort

	// *** Basic block 11

.LoadVariableValue_label_85:
	mv          a2, s4
	mv          a1, s3
	mv          a0, s5
	call        NewInstruction2

	// *** Basic block 12

	mv          a1, a0
	mv          a0, s2
	// Restored registers.
	ld s1, 56(sp)
	ld s2, 48(sp)
	ld s3, 40(sp)
	ld s4, 32(sp)
	ld s5, 24(sp)
	ld s6, 16(sp)
	ld s7, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           Emit
.func_end_LoadVariableValue:
	.size LoadVariableValue, .func_end_LoadVariableValue-LoadVariableValue

	.local  GetTlsVariableAddress
	.type GetTlsVariableAddress, @function

GetTlsVariableAddress:

	// *** Basic block 0

	.global compiler
	.global abort
	.local Emit
	.local NewInstruction1
	.local GetLoweredNode
	.local GetSymbol
	.local GetIntConstant
	.local NewInstruction2
	.local ThreadPointer
	addi sp, sp, -112
	// Saved return address (offset 104) and frame pointer (offset 96)
	sd ra, 104(sp)
	sd s0, 96(sp)
	addi s0, sp, 112
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 88(sp)
	sd s2, 80(sp)
	sd s3, 72(sp)
	sd s4, 64(sp)
	sd s5, 56(sp)
	sd s6, 48(sp)
	sd s7, 40(sp)
	sd s8, 32(sp)
	sd s9, 24(sp)
	sd s10, 16(sp)
	sd s11, 8(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	la          t0, compiler
	ld          t0, 0(t0)
	lw          s3, 1104(t0)
	li          t0, 1		// 0x1 ASCII \x1
	blt         s3, t0, .GetTlsVariableAddress_label_58

	// *** Basic block 1

	li          t0, 4		// 0x4 ASCII \x4
	blt         t0, s3, .GetTlsVariableAddress_label_58

	// *** Basic block 2

	addi        t0, s3, -1
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 3

	j           .GetTlsVariableAddress_label_60

	// *** Basic block 4

	j           .GetTlsVariableAddress_label_61

	// *** Basic block 5

	j           .GetTlsVariableAddress_label_141

	// *** Basic block 6

	j           .GetTlsVariableAddress_label_207

	// *** Basic block 7

.GetTlsVariableAddress_label_58:
	call        abort

	// *** Basic block 8

.GetTlsVariableAddress_label_60:

	// *** Basic block 9

.GetTlsVariableAddress_label_61:
	mv          a0, s2
	call        GetLoweredNode

	// *** Basic block 10

	mv          a1, a0
	li          t0, 126		// 0x7e ASCII '~'
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 11

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 12

	mv          s9, a0
	mv          a1, s9
	li          t0, 38		// 0x26 ASCII '&'
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 13

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 14

	ld          a2, 176(s1)
	mv          a1, x0
	mv          a0, s1
	call        GetSymbol

	// *** Basic block 15

	mv          s10, a0
	mv          a1, s10
	li          t0, 129		// 0x81 ASCII \x81
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 16

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 17

	mv          s11, a0
	li          t0, 8		// 0x8 ASCII \x8
	mv          a3, t0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	mv          a1, x0
	mv          a0, s1
	call        GetIntConstant

	// *** Basic block 18

	mv          a1, a0
	li          t0, 37		// 0x25 ASCII '%'
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 19

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 20

	mv          a0, s11

	// *** Basic block 21

.GetTlsVariableAddress_label_138:
	// Restored registers.
	ld s1, 88(sp)
	ld s2, 80(sp)
	ld s3, 72(sp)
	ld s4, 64(sp)
	ld s5, 56(sp)
	ld s6, 48(sp)
	ld s7, 40(sp)
	ld s8, 32(sp)
	ld s9, 24(sp)
	ld s10, 16(sp)
	ld s11, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 22

.GetTlsVariableAddress_label_141:
	mv          a0, s2
	call        GetLoweredNode

	// *** Basic block 23

	mv          a1, a0
	li          t0, 128		// 0x80 ASCII \x80
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 24

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 25

	mv          s3, a0
	mv          a3, x0
	li          t0, 3		// 0x3 ASCII \x3
	mv          a2, t0
	mv          a1, x0
	mv          a0, s1
	call        GetIntConstant

	// *** Basic block 26

	mv          s4, a0
	mv          a2, s4
	mv          a1, s3
	li          t0, 52		// 0x34 ASCII '4'
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 27

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 28

	mv          s5, a0
	mv          a0, s1
	call        ThreadPointer

	// *** Basic block 29

	mv          s6, a0
	mv          a2, s5
	mv          a1, s6
	li          t0, 64		// 0x40 ASCII '@'
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 30

	mv          a1, a0
	mv          a0, s1
	// Restored registers.
	ld s1, 88(sp)
	ld s2, 80(sp)
	ld s3, 72(sp)
	ld s4, 64(sp)
	ld s5, 56(sp)
	ld s6, 48(sp)
	ld s7, 40(sp)
	ld s8, 32(sp)
	ld s9, 24(sp)
	ld s10, 16(sp)
	ld s11, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           Emit

	// *** Basic block 32

.GetTlsVariableAddress_label_207:
	mv          a0, s1
	call        ThreadPointer

	// *** Basic block 33

	mv          s7, a0
	mv          a0, s2
	call        GetLoweredNode

	// *** Basic block 34

	mv          a1, a0
	li          t0, 17		// 0x11 ASCII \x11
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 35

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 36

	mv          s8, a0
	mv          a2, s8
	mv          a1, s7
	li          t0, 61		// 0x3d ASCII '='
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 37

	mv          a1, a0
	mv          a0, s1
	// Restored registers.
	ld s1, 88(sp)
	ld s2, 80(sp)
	ld s3, 72(sp)
	ld s4, 64(sp)
	ld s5, 56(sp)
	ld s6, 48(sp)
	ld s7, 40(sp)
	ld s8, 32(sp)
	ld s9, 24(sp)
	ld s10, 16(sp)
	ld s11, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           Emit
.func_end_GetTlsVariableAddress:
	.size GetTlsVariableAddress, .func_end_GetTlsVariableAddress-GetTlsVariableAddress

	.local  GetTlsAddressAndOffset
	.type GetTlsAddressAndOffset, @function

GetTlsAddressAndOffset:

	// *** Basic block 0

	.global compiler
	.global abort
	.local Emit
	.local NewInstruction1
	.local GetLoweredNode
	.local GetSymbol
	.local GetIntConstant
	.local NewInstruction2
	.local ThreadPointer
	addi sp, sp, -80
	// Saved return address (offset 72) and frame pointer (offset 64)
	sd ra, 72(sp)
	sd s0, 64(sp)
	addi s0, sp, 80
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 56(sp)
	sd s2, 48(sp)
	sd s3, 40(sp)
	sd s4, 32(sp)
	sd s5, 24(sp)
	sd s6, 16(sp)
	sd s7, 8(sp)
	sd s8, 0(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	mv          s3, a2
	mv          s4, a3
	la          t0, compiler
	ld          t0, 0(t0)
	lw          s5, 1104(t0)
	li          t0, 1		// 0x1 ASCII \x1
	blt         s5, t0, .GetTlsAddressAndOffset_label_63

	// *** Basic block 1

	li          t0, 4		// 0x4 ASCII \x4
	blt         t0, s5, .GetTlsAddressAndOffset_label_63

	// *** Basic block 2

	addi        t0, s5, -1
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 3

	j           .GetTlsAddressAndOffset_label_65

	// *** Basic block 4

	j           .GetTlsAddressAndOffset_label_66

	// *** Basic block 5

	j           .GetTlsAddressAndOffset_label_150

	// *** Basic block 6

	j           .GetTlsAddressAndOffset_label_210

	// *** Basic block 7

.GetTlsAddressAndOffset_label_63:
	call        abort

	// *** Basic block 8

.GetTlsAddressAndOffset_label_65:

	// *** Basic block 9

.GetTlsAddressAndOffset_label_66:
	mv          a0, s2
	call        GetLoweredNode

	// *** Basic block 10

	mv          a1, a0
	li          t0, 126		// 0x7e ASCII '~'
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 11

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 12

	mv          s7, a0
	mv          a1, s7
	li          t0, 38		// 0x26 ASCII '&'
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 13

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 14

	ld          a2, 176(s1)
	mv          a1, x0
	mv          a0, s1
	call        GetSymbol

	// *** Basic block 15

	mv          s8, a0
	mv          a1, s8
	li          t0, 129		// 0x81 ASCII \x81
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 16

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 17

	sd          a0, 0(s3)
	li          t0, 8		// 0x8 ASCII \x8
	mv          a3, t0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	mv          a1, x0
	mv          a0, s1
	call        GetIntConstant

	// *** Basic block 18

	mv          a1, a0
	li          t0, 37		// 0x25 ASCII '%'
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 19

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 20

	mv          a3, x0
	li          t0, 3		// 0x3 ASCII \x3
	mv          a2, t0
	mv          a1, x0
	mv          a0, s1
	call        GetIntConstant

	// *** Basic block 21

	sd          a0, 0(s4)
	j           .GetTlsAddressAndOffset_label_256

	// *** Basic block 22

.GetTlsAddressAndOffset_label_150:
	mv          a0, s2
	call        GetLoweredNode

	// *** Basic block 23

	mv          a1, a0
	li          t0, 128		// 0x80 ASCII \x80
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 24

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 25

	mv          s5, a0
	mv          a3, x0
	li          t0, 3		// 0x3 ASCII \x3
	mv          a2, t0
	mv          a1, x0
	mv          a0, s1
	call        GetIntConstant

	// *** Basic block 26

	sd          a0, 0(s4)
	ld          a2, 0(s4)
	mv          a1, s5
	li          t0, 52		// 0x34 ASCII '4'
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 27

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 28

	mv          s5, a0
	mv          a0, s1
	call        ThreadPointer

	// *** Basic block 29

	mv          a2, s5
	mv          a1, a0
	li          t0, 61		// 0x3d ASCII '='
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 30

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 31

	sd          a0, 0(s3)
	j           .GetTlsAddressAndOffset_label_256

	// *** Basic block 32

.GetTlsAddressAndOffset_label_210:
	mv          a0, s2
	call        GetLoweredNode

	// *** Basic block 33

	mv          a1, a0
	li          t0, 17		// 0x11 ASCII \x11
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 34

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 35

	mv          s6, a0
	mv          a0, s1
	call        ThreadPointer

	// *** Basic block 36

	mv          a2, s6
	mv          a1, a0
	li          t0, 61		// 0x3d ASCII '='
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 37

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 38

	sd          a0, 0(s3)
	mv          a3, x0
	li          t0, 3		// 0x3 ASCII \x3
	mv          a2, t0
	mv          a1, x0
	mv          a0, s1
	call        GetIntConstant

	// *** Basic block 39

	sd          a0, 0(s4)
	j           .GetTlsAddressAndOffset_label_256

	// *** Basic block 40

.GetTlsAddressAndOffset_label_256:
	// Restored registers.
	ld s1, 56(sp)
	ld s2, 48(sp)
	ld s3, 40(sp)
	ld s4, 32(sp)
	ld s5, 24(sp)
	ld s6, 16(sp)
	ld s7, 8(sp)
	ld s8, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_GetTlsAddressAndOffset:
	.size GetTlsAddressAndOffset, .func_end_GetTlsAddressAndOffset-GetTlsAddressAndOffset

	.local  Materialize
	.type Materialize, @function

Materialize:

	// *** Basic block 0

	.global IRIsConst
	.local Emit
	.local NewInstruction1
	.local GetLoweredNode
	.global printf
	.global abort
	.global IRIsAutoVariable
	.local FramePointer
	.local GetIntConstant
	.local NewInstruction2
	.global IRIsArgument
	.local ArgumentPointer
	.global IRIsThreadVariable
	.local GetTlsVariableAddress
	.global IRIsStaticVariable
	.local LoadStaticVariable
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
	mv          s1, a1
	mv          s2, a0
	mv          a0, s1
	call        IRIsConst

	// *** Basic block 1

	beqz        a0, .Materialize_label_166

	// *** Basic block 2

	lw          s3, 20(s1)
	li          t0, 2		// 0x2 ASCII \x2
	blt         s3, t0, .Materialize_label_148

	// *** Basic block 3

	li          t0, 8		// 0x8 ASCII \x8
	blt         t0, s3, .Materialize_label_148

	// *** Basic block 4

	addi        t0, s3, -2
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 5

	j           .Materialize_label_67

	// *** Basic block 6

	j           .Materialize_label_68

	// *** Basic block 7

	j           .Materialize_label_69

	// *** Basic block 8

	j           .Materialize_label_90

	// *** Basic block 9

	j           .Materialize_label_110

	// *** Basic block 10

	j           .Materialize_label_129

	// *** Basic block 11

	j           .Materialize_label_91

	// *** Basic block 12

.Materialize_label_67:

	// *** Basic block 13

.Materialize_label_68:

	// *** Basic block 14

.Materialize_label_69:
	mv          a0, s1
	call        GetLoweredNode

	// *** Basic block 15

	mv          a1, a0
	li          t0, 14		// 0xe ASCII \xe
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 16

	mv          a1, a0
	mv          a0, s2
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
	j           Emit

	// *** Basic block 19

.Materialize_label_90:

	// *** Basic block 20

.Materialize_label_91:
	mv          a0, s1
	call        GetLoweredNode

	// *** Basic block 21

	mv          a1, a0
	li          t0, 17		// 0x11 ASCII \x11
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 22

	mv          a1, a0
	mv          a0, s2
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
	j           Emit

	// *** Basic block 24

.Materialize_label_110:
	mv          a0, s1
	call        GetLoweredNode

	// *** Basic block 25

	mv          a1, a0
	li          t0, 15		// 0xf ASCII \xf
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 26

	mv          a1, a0
	mv          a0, s2
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
	j           Emit

	// *** Basic block 28

.Materialize_label_129:
	mv          a0, s1
	call        GetLoweredNode

	// *** Basic block 29

	mv          a1, a0
	li          t0, 16		// 0x10 ASCII \x10
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 30

	mv          a1, a0
	mv          a0, s2
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
	j           Emit

	// *** Basic block 32

.Materialize_label_148:
	lla         a0, .str.105
	lla         a1, .str.106
	lla         a3, .str.107
	li          t0, 545		// 0x221
	mv          a2, t0
	call        printf

	// *** Basic block 33

	call        abort

	// *** Basic block 34

.Materialize_label_165:

	// *** Basic block 35

.Materialize_label_166:
	mv          a0, s1
	call        IRIsAutoVariable

	// *** Basic block 36

	beqz        a0, .Materialize_label_211

	// *** Basic block 37

	mv          a0, s2
	call        FramePointer

	// *** Basic block 38

	mv          s3, a0
	addi        t0, s1, 96
	lw          s4, 8(t0)
	lw          t0, 128(s2)
	sub         a3, s4, t0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	mv          a1, s1
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 39

	mv          s4, a0
	mv          a2, s4
	mv          a1, s3
	li          t0, 64		// 0x40 ASCII '@'
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 40

	mv          a1, a0
	mv          a0, s2
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
	j           Emit

	// *** Basic block 42

.Materialize_label_211:
	mv          a0, s1
	call        IRIsArgument

	// *** Basic block 43

	beqz        a0, .Materialize_label_254

	// *** Basic block 44

	mv          a0, s2
	call        ArgumentPointer

	// *** Basic block 45

	mv          s5, a0
	addi        t0, s1, 96
	lw          s6, 8(t0)
	mv          a3, s6
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	mv          a1, s1
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 46

	mv          s6, a0
	mv          a2, s6
	mv          a1, s5
	li          t0, 64		// 0x40 ASCII '@'
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 47

	mv          a1, a0
	mv          a0, s2
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
	j           Emit

	// *** Basic block 49

.Materialize_label_254:
	mv          a0, s1
	call        IRIsThreadVariable

	// *** Basic block 50

	beqz        a0, .Materialize_label_268

	// *** Basic block 51

	mv          a1, s1
	mv          a0, s2
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
	j           GetTlsVariableAddress

	// *** Basic block 53

.Materialize_label_268:
	mv          a0, s1
	call        IRIsStaticVariable

	// *** Basic block 54

	beqz        a0, .Materialize_label_282

	// *** Basic block 55

	mv          a1, s1
	mv          a0, s2
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
	j           LoadStaticVariable

	// *** Basic block 57

.Materialize_label_282:

	// *** Basic block 58

.Materialize_label_283:

	// *** Basic block 59

.Materialize_label_284:

	// *** Basic block 60

.Materialize_label_285:
	mv          a0, s1
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
	j           GetLoweredNode
.func_end_Materialize:
	.size Materialize, .func_end_Materialize-Materialize

	.local  IR2PCode
	.type IR2PCode, @function

IR2PCode:

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
	sd s2, 0(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	li          t0, 1		// 0x1 ASCII \x1
	blt         s1, t0, .IR2PCode_label_545

	// *** Basic block 1

	li          t0, 115		// 0x73 ASCII 's'
	blt         t0, s1, .IR2PCode_label_545

	// *** Basic block 2

	addi        t0, s1, -1
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 3

	j           .IR2PCode_label_541

	// *** Basic block 4

	j           .IR2PCode_label_545

	// *** Basic block 5

	j           .IR2PCode_label_545

	// *** Basic block 6

	j           .IR2PCode_label_545

	// *** Basic block 7

	j           .IR2PCode_label_545

	// *** Basic block 8

	j           .IR2PCode_label_545

	// *** Basic block 9

	j           .IR2PCode_label_545

	// *** Basic block 10

	j           .IR2PCode_label_545

	// *** Basic block 11

	j           .IR2PCode_label_509

	// *** Basic block 12

	j           .IR2PCode_label_513

	// *** Basic block 13

	j           .IR2PCode_label_517

	// *** Basic block 14

	j           .IR2PCode_label_521

	// *** Basic block 15

	j           .IR2PCode_label_525

	// *** Basic block 16

	j           .IR2PCode_label_529

	// *** Basic block 17

	j           .IR2PCode_label_533

	// *** Basic block 18

	j           .IR2PCode_label_537

	// *** Basic block 19

	j           .IR2PCode_label_545

	// *** Basic block 20

	j           .IR2PCode_label_545

	// *** Basic block 21

	j           .IR2PCode_label_545

	// *** Basic block 22

	j           .IR2PCode_label_545

	// *** Basic block 23

	j           .IR2PCode_label_545

	// *** Basic block 24

	j           .IR2PCode_label_545

	// *** Basic block 25

	j           .IR2PCode_label_545

	// *** Basic block 26

	j           .IR2PCode_label_545

	// *** Basic block 27

	j           .IR2PCode_label_545

	// *** Basic block 28

	j           .IR2PCode_label_545

	// *** Basic block 29

	j           .IR2PCode_label_545

	// *** Basic block 30

	j           .IR2PCode_label_545

	// *** Basic block 31

	j           .IR2PCode_label_545

	// *** Basic block 32

	j           .IR2PCode_label_545

	// *** Basic block 33

	j           .IR2PCode_label_545

	// *** Basic block 34

	j           .IR2PCode_label_545

	// *** Basic block 35

	j           .IR2PCode_label_545

	// *** Basic block 36

	j           .IR2PCode_label_545

	// *** Basic block 37

	j           .IR2PCode_label_545

	// *** Basic block 38

	j           .IR2PCode_label_545

	// *** Basic block 39

	j           .IR2PCode_label_209

	// *** Basic block 40

	j           .IR2PCode_label_215

	// *** Basic block 41

	j           .IR2PCode_label_219

	// *** Basic block 42

	j           .IR2PCode_label_223

	// *** Basic block 43

	j           .IR2PCode_label_227

	// *** Basic block 44

	j           .IR2PCode_label_231

	// *** Basic block 45

	j           .IR2PCode_label_235

	// *** Basic block 46

	j           .IR2PCode_label_239

	// *** Basic block 47

	j           .IR2PCode_label_243

	// *** Basic block 48

	j           .IR2PCode_label_247

	// *** Basic block 49

	j           .IR2PCode_label_251

	// *** Basic block 50

	j           .IR2PCode_label_255

	// *** Basic block 51

	j           .IR2PCode_label_266

	// *** Basic block 52

	j           .IR2PCode_label_270

	// *** Basic block 53

	j           .IR2PCode_label_274

	// *** Basic block 54

	j           .IR2PCode_label_285

	// *** Basic block 55

	j           .IR2PCode_label_289

	// *** Basic block 56

	j           .IR2PCode_label_293

	// *** Basic block 57

	j           .IR2PCode_label_297

	// *** Basic block 58

	j           .IR2PCode_label_301

	// *** Basic block 59

	j           .IR2PCode_label_305

	// *** Basic block 60

	j           .IR2PCode_label_309

	// *** Basic block 61

	j           .IR2PCode_label_313

	// *** Basic block 62

	j           .IR2PCode_label_317

	// *** Basic block 63

	j           .IR2PCode_label_321

	// *** Basic block 64

	j           .IR2PCode_label_325

	// *** Basic block 65

	j           .IR2PCode_label_329

	// *** Basic block 66

	j           .IR2PCode_label_333

	// *** Basic block 67

	j           .IR2PCode_label_337

	// *** Basic block 68

	j           .IR2PCode_label_341

	// *** Basic block 69

	j           .IR2PCode_label_352

	// *** Basic block 70

	j           .IR2PCode_label_363

	// *** Basic block 71

	j           .IR2PCode_label_374

	// *** Basic block 72

	j           .IR2PCode_label_385

	// *** Basic block 73

	j           .IR2PCode_label_389

	// *** Basic block 74

	j           .IR2PCode_label_393

	// *** Basic block 75

	j           .IR2PCode_label_397

	// *** Basic block 76

	j           .IR2PCode_label_401

	// *** Basic block 77

	j           .IR2PCode_label_405

	// *** Basic block 78

	j           .IR2PCode_label_409

	// *** Basic block 79

	j           .IR2PCode_label_413

	// *** Basic block 80

	j           .IR2PCode_label_417

	// *** Basic block 81

	j           .IR2PCode_label_421

	// *** Basic block 82

	j           .IR2PCode_label_425

	// *** Basic block 83

	j           .IR2PCode_label_429

	// *** Basic block 84

	j           .IR2PCode_label_433

	// *** Basic block 85

	j           .IR2PCode_label_437

	// *** Basic block 86

	j           .IR2PCode_label_441

	// *** Basic block 87

	j           .IR2PCode_label_445

	// *** Basic block 88

	j           .IR2PCode_label_449

	// *** Basic block 89

	j           .IR2PCode_label_453

	// *** Basic block 90

	j           .IR2PCode_label_545

	// *** Basic block 91

	j           .IR2PCode_label_545

	// *** Basic block 92

	j           .IR2PCode_label_545

	// *** Basic block 93

	j           .IR2PCode_label_545

	// *** Basic block 94

	j           .IR2PCode_label_545

	// *** Basic block 95

	j           .IR2PCode_label_545

	// *** Basic block 96

	j           .IR2PCode_label_545

	// *** Basic block 97

	j           .IR2PCode_label_545

	// *** Basic block 98

	j           .IR2PCode_label_545

	// *** Basic block 99

	j           .IR2PCode_label_545

	// *** Basic block 100

	j           .IR2PCode_label_545

	// *** Basic block 101

	j           .IR2PCode_label_545

	// *** Basic block 102

	j           .IR2PCode_label_545

	// *** Basic block 103

	j           .IR2PCode_label_545

	// *** Basic block 104

	j           .IR2PCode_label_545

	// *** Basic block 105

	j           .IR2PCode_label_545

	// *** Basic block 106

	j           .IR2PCode_label_545

	// *** Basic block 107

	j           .IR2PCode_label_545

	// *** Basic block 108

	j           .IR2PCode_label_545

	// *** Basic block 109

	j           .IR2PCode_label_545

	// *** Basic block 110

	j           .IR2PCode_label_545

	// *** Basic block 111

	j           .IR2PCode_label_545

	// *** Basic block 112

	j           .IR2PCode_label_457

	// *** Basic block 113

	j           .IR2PCode_label_468

	// *** Basic block 114

	j           .IR2PCode_label_479

	// *** Basic block 115

	j           .IR2PCode_label_483

	// *** Basic block 116

	j           .IR2PCode_label_487

	// *** Basic block 117

	j           .IR2PCode_label_498

	// *** Basic block 118

.IR2PCode_label_209:
	li          a0, 61		// 0x3d ASCII '='

	// *** Basic block 119

.IR2PCode_label_212:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 120

.IR2PCode_label_215:
	li          a0, 62		// 0x3e ASCII '>'
	j           .IR2PCode_label_212

	// *** Basic block 121

.IR2PCode_label_219:
	li          a0, 63		// 0x3f ASCII '?'
	j           .IR2PCode_label_212

	// *** Basic block 122

.IR2PCode_label_223:
	li          a0, 61		// 0x3d ASCII '='
	j           .IR2PCode_label_212

	// *** Basic block 123

.IR2PCode_label_227:
	li          a0, 65		// 0x41 ASCII 'A'
	j           .IR2PCode_label_212

	// *** Basic block 124

.IR2PCode_label_231:
	li          a0, 66		// 0x42 ASCII 'B'
	j           .IR2PCode_label_212

	// *** Basic block 125

.IR2PCode_label_235:
	li          a0, 67		// 0x43 ASCII 'C'
	j           .IR2PCode_label_212

	// *** Basic block 126

.IR2PCode_label_239:
	li          a0, 65		// 0x41 ASCII 'A'
	j           .IR2PCode_label_212

	// *** Basic block 127

.IR2PCode_label_243:
	li          a0, 68		// 0x44 ASCII 'D'
	j           .IR2PCode_label_212

	// *** Basic block 128

.IR2PCode_label_247:
	li          a0, 69		// 0x45 ASCII 'E'
	j           .IR2PCode_label_212

	// *** Basic block 129

.IR2PCode_label_251:
	li          a0, 70		// 0x46 ASCII 'F'
	j           .IR2PCode_label_212

	// *** Basic block 130

.IR2PCode_label_255:
	beqz        s2, .IR2PCode_label_261

	// *** Basic block 131

	li          a0, 72		// 0x48 ASCII 'H'
	j           .IR2PCode_label_263

	// *** Basic block 132

.IR2PCode_label_261:
	li          a0, 71		// 0x47 ASCII 'G'

	// *** Basic block 133

.IR2PCode_label_263:
	j           .IR2PCode_label_212

	// *** Basic block 134

.IR2PCode_label_266:
	li          a0, 73		// 0x49 ASCII 'I'
	j           .IR2PCode_label_212

	// *** Basic block 135

.IR2PCode_label_270:
	li          a0, 74		// 0x4a ASCII 'J'
	j           .IR2PCode_label_212

	// *** Basic block 136

.IR2PCode_label_274:
	beqz        s2, .IR2PCode_label_280

	// *** Basic block 137

	li          a0, 76		// 0x4c ASCII 'L'
	j           .IR2PCode_label_282

	// *** Basic block 138

.IR2PCode_label_280:
	li          a0, 75		// 0x4b ASCII 'K'

	// *** Basic block 139

.IR2PCode_label_282:
	j           .IR2PCode_label_212

	// *** Basic block 140

.IR2PCode_label_285:
	li          a0, 77		// 0x4d ASCII 'M'
	j           .IR2PCode_label_212

	// *** Basic block 141

.IR2PCode_label_289:
	li          a0, 78		// 0x4e ASCII 'N'
	j           .IR2PCode_label_212

	// *** Basic block 142

.IR2PCode_label_293:
	li          a0, 79		// 0x4f ASCII 'O'
	j           .IR2PCode_label_212

	// *** Basic block 143

.IR2PCode_label_297:
	li          a0, 80		// 0x50 ASCII 'P'
	j           .IR2PCode_label_212

	// *** Basic block 144

.IR2PCode_label_301:
	li          a0, 81		// 0x51 ASCII 'Q'
	j           .IR2PCode_label_212

	// *** Basic block 145

.IR2PCode_label_305:
	li          a0, 82		// 0x52 ASCII 'R'
	j           .IR2PCode_label_212

	// *** Basic block 146

.IR2PCode_label_309:
	li          a0, 83		// 0x53 ASCII 'S'
	j           .IR2PCode_label_212

	// *** Basic block 147

.IR2PCode_label_313:
	li          a0, 83		// 0x53 ASCII 'S'
	j           .IR2PCode_label_212

	// *** Basic block 148

.IR2PCode_label_317:
	li          a0, 84		// 0x54 ASCII 'T'
	j           .IR2PCode_label_212

	// *** Basic block 149

.IR2PCode_label_321:
	li          a0, 85		// 0x55 ASCII 'U'
	j           .IR2PCode_label_212

	// *** Basic block 150

.IR2PCode_label_325:
	li          a0, 86		// 0x56 ASCII 'V'
	j           .IR2PCode_label_212

	// *** Basic block 151

.IR2PCode_label_329:
	li          a0, 87		// 0x57 ASCII 'W'
	j           .IR2PCode_label_212

	// *** Basic block 152

.IR2PCode_label_333:
	li          a0, 88		// 0x58 ASCII 'X'
	j           .IR2PCode_label_212

	// *** Basic block 153

.IR2PCode_label_337:
	li          a0, 89		// 0x59 ASCII 'Y'
	j           .IR2PCode_label_212

	// *** Basic block 154

.IR2PCode_label_341:
	beqz        s2, .IR2PCode_label_347

	// *** Basic block 155

	li          a0, 94		// 0x5e ASCII '^'
	j           .IR2PCode_label_349

	// *** Basic block 156

.IR2PCode_label_347:
	li          a0, 90		// 0x5a ASCII 'Z'

	// *** Basic block 157

.IR2PCode_label_349:
	j           .IR2PCode_label_212

	// *** Basic block 158

.IR2PCode_label_352:
	beqz        s2, .IR2PCode_label_358

	// *** Basic block 159

	li          a0, 95		// 0x5f ASCII '_'
	j           .IR2PCode_label_360

	// *** Basic block 160

.IR2PCode_label_358:
	li          a0, 91		// 0x5b ASCII '['

	// *** Basic block 161

.IR2PCode_label_360:
	j           .IR2PCode_label_212

	// *** Basic block 162

.IR2PCode_label_363:
	beqz        s2, .IR2PCode_label_369

	// *** Basic block 163

	li          a0, 96		// 0x60 ASCII '`'
	j           .IR2PCode_label_371

	// *** Basic block 164

.IR2PCode_label_369:
	li          a0, 92		// 0x5c ASCII '\'

	// *** Basic block 165

.IR2PCode_label_371:
	j           .IR2PCode_label_212

	// *** Basic block 166

.IR2PCode_label_374:
	beqz        s2, .IR2PCode_label_380

	// *** Basic block 167

	li          a0, 97		// 0x61 ASCII 'a'
	j           .IR2PCode_label_382

	// *** Basic block 168

.IR2PCode_label_380:
	li          a0, 93		// 0x5d ASCII ']'

	// *** Basic block 169

.IR2PCode_label_382:
	j           .IR2PCode_label_212

	// *** Basic block 170

.IR2PCode_label_385:
	li          a0, 98		// 0x62 ASCII 'b'
	j           .IR2PCode_label_212

	// *** Basic block 171

.IR2PCode_label_389:
	li          a0, 99		// 0x63 ASCII 'c'
	j           .IR2PCode_label_212

	// *** Basic block 172

.IR2PCode_label_393:
	li          a0, 100		// 0x64 ASCII 'd'
	j           .IR2PCode_label_212

	// *** Basic block 173

.IR2PCode_label_397:
	li          a0, 101		// 0x65 ASCII 'e'
	j           .IR2PCode_label_212

	// *** Basic block 174

.IR2PCode_label_401:
	li          a0, 102		// 0x66 ASCII 'f'
	j           .IR2PCode_label_212

	// *** Basic block 175

.IR2PCode_label_405:
	li          a0, 103		// 0x67 ASCII 'g'
	j           .IR2PCode_label_212

	// *** Basic block 176

.IR2PCode_label_409:
	li          a0, 104		// 0x68 ASCII 'h'
	j           .IR2PCode_label_212

	// *** Basic block 177

.IR2PCode_label_413:
	li          a0, 105		// 0x69 ASCII 'i'
	j           .IR2PCode_label_212

	// *** Basic block 178

.IR2PCode_label_417:
	li          a0, 106		// 0x6a ASCII 'j'
	j           .IR2PCode_label_212

	// *** Basic block 179

.IR2PCode_label_421:
	li          a0, 107		// 0x6b ASCII 'k'
	j           .IR2PCode_label_212

	// *** Basic block 180

.IR2PCode_label_425:
	li          a0, 108		// 0x6c ASCII 'l'
	j           .IR2PCode_label_212

	// *** Basic block 181

.IR2PCode_label_429:
	li          a0, 109		// 0x6d ASCII 'm'
	j           .IR2PCode_label_212

	// *** Basic block 182

.IR2PCode_label_433:
	li          a0, 88		// 0x58 ASCII 'X'
	j           .IR2PCode_label_212

	// *** Basic block 183

.IR2PCode_label_437:
	li          a0, 89		// 0x59 ASCII 'Y'
	j           .IR2PCode_label_212

	// *** Basic block 184

.IR2PCode_label_441:
	li          a0, 90		// 0x5a ASCII 'Z'
	j           .IR2PCode_label_212

	// *** Basic block 185

.IR2PCode_label_445:
	li          a0, 91		// 0x5b ASCII '['
	j           .IR2PCode_label_212

	// *** Basic block 186

.IR2PCode_label_449:
	li          a0, 92		// 0x5c ASCII '\'
	j           .IR2PCode_label_212

	// *** Basic block 187

.IR2PCode_label_453:
	li          a0, 93		// 0x5d ASCII ']'
	j           .IR2PCode_label_212

	// *** Basic block 188

.IR2PCode_label_457:
	beqz        s2, .IR2PCode_label_463

	// *** Basic block 189

	li          a0, 116		// 0x74 ASCII 't'
	j           .IR2PCode_label_465

	// *** Basic block 190

.IR2PCode_label_463:
	li          a0, 114		// 0x72 ASCII 'r'

	// *** Basic block 191

.IR2PCode_label_465:
	j           .IR2PCode_label_212

	// *** Basic block 192

.IR2PCode_label_468:
	beqz        s2, .IR2PCode_label_474

	// *** Basic block 193

	li          a0, 117		// 0x75 ASCII 'u'
	j           .IR2PCode_label_476

	// *** Basic block 194

.IR2PCode_label_474:
	li          a0, 115		// 0x73 ASCII 's'

	// *** Basic block 195

.IR2PCode_label_476:
	j           .IR2PCode_label_212

	// *** Basic block 196

.IR2PCode_label_479:
	li          a0, 118		// 0x76 ASCII 'v'
	j           .IR2PCode_label_212

	// *** Basic block 197

.IR2PCode_label_483:
	li          a0, 119		// 0x77 ASCII 'w'
	j           .IR2PCode_label_212

	// *** Basic block 198

.IR2PCode_label_487:
	beqz        s2, .IR2PCode_label_493

	// *** Basic block 199

	li          a0, 122		// 0x7a ASCII 'z'
	j           .IR2PCode_label_495

	// *** Basic block 200

.IR2PCode_label_493:
	li          a0, 120		// 0x78 ASCII 'x'

	// *** Basic block 201

.IR2PCode_label_495:
	j           .IR2PCode_label_212

	// *** Basic block 202

.IR2PCode_label_498:
	beqz        s2, .IR2PCode_label_504

	// *** Basic block 203

	li          a0, 123		// 0x7b ASCII '{'
	j           .IR2PCode_label_506

	// *** Basic block 204

.IR2PCode_label_504:
	li          a0, 121		// 0x79 ASCII 'y'

	// *** Basic block 205

.IR2PCode_label_506:
	j           .IR2PCode_label_212

	// *** Basic block 206

.IR2PCode_label_509:
	li          a0, 11		// 0xb ASCII \xb
	j           .IR2PCode_label_212

	// *** Basic block 207

.IR2PCode_label_513:
	li          a0, 12		// 0xc ASCII \xc
	j           .IR2PCode_label_212

	// *** Basic block 208

.IR2PCode_label_517:
	li          a0, 13		// 0xd ASCII \xd
	j           .IR2PCode_label_212

	// *** Basic block 209

.IR2PCode_label_521:
	li          a0, 11		// 0xb ASCII \xb
	j           .IR2PCode_label_212

	// *** Basic block 210

.IR2PCode_label_525:
	li          a0, 18		// 0x12 ASCII \x12
	j           .IR2PCode_label_212

	// *** Basic block 211

.IR2PCode_label_529:
	li          a0, 19		// 0x13 ASCII \x13
	j           .IR2PCode_label_212

	// *** Basic block 212

.IR2PCode_label_533:
	li          a0, 20		// 0x14 ASCII \x14
	j           .IR2PCode_label_212

	// *** Basic block 213

.IR2PCode_label_537:
	li          a0, 18		// 0x12 ASCII \x12
	j           .IR2PCode_label_212

	// *** Basic block 214

.IR2PCode_label_541:
	li          a0, 4		// 0x4 ASCII \x4
	j           .IR2PCode_label_212

	// *** Basic block 215

.IR2PCode_label_545:
	lla         a0, .str.108
	lla         a1, .str.109
	lla         a3, .str.110
	li          t0, 726		// 0x2d6
	mv          a2, t0
	call        printf

	// *** Basic block 216

	call        abort

	// *** Basic block 217

	mv          a0, x0
	j           .IR2PCode_label_212
.func_end_IR2PCode:
	.size IR2PCode, .func_end_IR2PCode-IR2PCode

	.local  ApplyFixups
	.type ApplyFixups, @function

ApplyFixups:

	// *** Basic block 0

	.global TargetApplyFixups
	// Leaf procedure, no stack frame generated
	j           TargetApplyFixups
.func_end_ApplyFixups:
	.size ApplyFixups, .func_end_ApplyFixups-ApplyFixups

	.local  ReduceExpressionStrength
	.type ReduceExpressionStrength, @function

ReduceExpressionStrength:

	// *** Basic block 0

	.global printf
	.global abort
	.global IRIsConst
	.local NewInstruction
	.local GetIntConstant
	.local Materialize
	.local GetLoweredNode
	sd          a0, -0(s0)	// Spilled @686
	addi sp, sp, -112
	// Saved return address (offset 104) and frame pointer (offset 96)
	sd ra, 104(sp)
	sd s0, 96(sp)
	addi s0, sp, 112
	// Local vars at offset -16(s0)
	// Spilled register region: 8 bytes at -24(s0) to -16(s0)
	// Saved integer registers.
	sd s1, 80(sp)
	sd s2, 72(sp)
	sd s3, 64(sp)
	sd s4, 56(sp)
	sd s5, 48(sp)
	sd s6, 40(sp)
	sd s7, 32(sp)
	sd s8, 24(sp)
	sd s9, 16(sp)
	sd s10, 8(sp)
	sd s11, 0(sp)
	// End of stack frame
	mv          t0, a2
	mv          s1, a1
	sd          s1, -24(s0)	// Spilled @47
	mv          s2, a0
	mv          s3, x0
	li          t1, 61		// 0x3d ASCII '='
	blt         t0, t1, .ReduceExpressionStrength_label_82

	// *** Basic block 1

	li          t1, 72		// 0x48 ASCII 'H'
	blt         t1, t0, .ReduceExpressionStrength_label_82

	// *** Basic block 2

	addi        t1, t0, -61
	slli        t1, t1, 2
	auipc       t2, 0
	add         t1, t2, t1
	jalr        x0, t1, 12

	// *** Basic block 3

	j           .ReduceExpressionStrength_label_84

	// *** Basic block 4

	j           .ReduceExpressionStrength_label_82

	// *** Basic block 5

	j           .ReduceExpressionStrength_label_82

	// *** Basic block 6

	j           .ReduceExpressionStrength_label_82

	// *** Basic block 7

	j           .ReduceExpressionStrength_label_243

	// *** Basic block 8

	j           .ReduceExpressionStrength_label_82

	// *** Basic block 9

	j           .ReduceExpressionStrength_label_82

	// *** Basic block 10

	j           .ReduceExpressionStrength_label_362

	// *** Basic block 11

	j           .ReduceExpressionStrength_label_82

	// *** Basic block 12

	j           .ReduceExpressionStrength_label_82

	// *** Basic block 13

	j           .ReduceExpressionStrength_label_491

	// *** Basic block 14

	j           .ReduceExpressionStrength_label_588

	// *** Basic block 15

.ReduceExpressionStrength_label_82:
	j           .ReduceExpressionStrength_label_685

	// *** Basic block 16

.ReduceExpressionStrength_label_84:
	addi        t0, s1, 24
	ld          t0, 8(t0)
	li          s4, 2		// 0x2 ASCII \x2
	bne         t0, s4, .ReduceExpressionStrength_label_94

	// *** Basic block 17

	j           .ReduceExpressionStrength_label_110

	// *** Basic block 18

.ReduceExpressionStrength_label_94:
	lla         a0, .str.111
	lla         a1, .str.112
	lla         a3, .str.113
	li          t0, 745		// 0x2e9
	mv          a2, t0
	call        printf

	// *** Basic block 19

	call        abort

	// *** Basic block 20

.ReduceExpressionStrength_label_110:
	ld          t0, 24(s1)
	ld          s5, 0(t0)
	ld          s6, 8(t0)
	mv          a0, s5
	call        IRIsConst

	// *** Basic block 21

	mv          s7, a0
	beqz        a0, .ReduceExpressionStrength_label_127

	// *** Basic block 22

	mv          a0, s6
	call        IRIsConst

	// *** Basic block 23

	mv          s7, a0

	// *** Basic block 24

.ReduceExpressionStrength_label_127:
	beqz        s7, .ReduceExpressionStrength_label_153

	// *** Basic block 25

	ld          s8, 136(s5)
	ld          s9, 136(s6)
	li          t0, 17		// 0x11 ASCII \x11
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 26

	mv          s3, a0
	add         a3, s8, s9
	mv          a2, s4
	mv          a1, x0
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 27

	sd          a0, 40(s3)
	j           .ReduceExpressionStrength_label_685

	// *** Basic block 28

.ReduceExpressionStrength_label_153:
	mv          a0, s6
	call        IRIsConst

	// *** Basic block 29

	beqz        a0, .ReduceExpressionStrength_label_197

	// *** Basic block 30

	ld          s4, 136(s6)
	bnez        s4, .ReduceExpressionStrength_label_176

	// *** Basic block 31

	li          t0, 11		// 0xb ASCII \xb
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 32

	mv          s3, a0
	mv          a1, s5
	mv          a0, s2
	call        Materialize

	// *** Basic block 33

	sd          a0, 40(s3)
	j           .ReduceExpressionStrength_label_195

	// *** Basic block 34

.ReduceExpressionStrength_label_176:
	li          t0, 64		// 0x40 ASCII '@'
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 35

	mv          s3, a0
	mv          a1, s5
	mv          a0, s2
	call        Materialize

	// *** Basic block 36

	sd          a0, 40(s3)
	addi        s4, s3, 40
	mv          a0, s6
	call        GetLoweredNode

	// *** Basic block 37

	sd          a0, 8(s4)

	// *** Basic block 38

.ReduceExpressionStrength_label_195:
	j           .ReduceExpressionStrength_label_241

	// *** Basic block 39

.ReduceExpressionStrength_label_197:
	mv          a0, s5
	call        IRIsConst

	// *** Basic block 40

	beqz        a0, .ReduceExpressionStrength_label_240

	// *** Basic block 41

	ld          s4, 136(s5)
	bnez        s4, .ReduceExpressionStrength_label_220

	// *** Basic block 42

	li          t0, 11		// 0xb ASCII \xb
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 43

	mv          s3, a0
	mv          a1, s5
	mv          a0, s2
	call        Materialize

	// *** Basic block 44

	sd          a0, 40(s3)
	j           .ReduceExpressionStrength_label_239

	// *** Basic block 45

.ReduceExpressionStrength_label_220:
	li          t0, 64		// 0x40 ASCII '@'
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 46

	mv          s3, a0
	mv          a1, s6
	mv          a0, s2
	call        Materialize

	// *** Basic block 47

	sd          a0, 40(s3)
	addi        s4, s3, 40
	mv          a0, s5
	call        GetLoweredNode

	// *** Basic block 48

	sd          a0, 8(s4)

	// *** Basic block 49

.ReduceExpressionStrength_label_239:

	// *** Basic block 50

.ReduceExpressionStrength_label_240:

	// *** Basic block 51

.ReduceExpressionStrength_label_241:
	j           .ReduceExpressionStrength_label_685

	// *** Basic block 52

.ReduceExpressionStrength_label_243:
	addi        t0, s1, 24
	ld          t0, 8(t0)
	li          s4, 2		// 0x2 ASCII \x2
	bne         t0, s4, .ReduceExpressionStrength_label_252

	// *** Basic block 53

	j           .ReduceExpressionStrength_label_267

	// *** Basic block 54

.ReduceExpressionStrength_label_252:
	lla         a0, .str.114
	lla         a1, .str.115
	lla         a3, .str.116
	li          t0, 788		// 0x314
	mv          a2, t0
	call        printf

	// *** Basic block 55

	call        abort

	// *** Basic block 56

.ReduceExpressionStrength_label_267:
	ld          t0, 24(s1)
	ld          s5, 0(t0)
	ld          s6, 8(t0)
	mv          a0, s5
	call        IRIsConst

	// *** Basic block 57

	mv          s7, a0
	beqz        a0, .ReduceExpressionStrength_label_284

	// *** Basic block 58

	mv          a0, s6
	call        IRIsConst

	// *** Basic block 59

	mv          s7, a0

	// *** Basic block 60

.ReduceExpressionStrength_label_284:
	beqz        s7, .ReduceExpressionStrength_label_310

	// *** Basic block 61

	ld          s8, 136(s5)
	ld          s9, 136(s6)
	li          t0, 17		// 0x11 ASCII \x11
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 62

	mv          s3, a0
	sub         a3, s8, s9
	mv          a2, s4
	mv          a1, x0
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 63

	sd          a0, 40(s3)
	j           .ReduceExpressionStrength_label_685

	// *** Basic block 64

.ReduceExpressionStrength_label_310:
	mv          a0, s6
	call        IRIsConst

	// *** Basic block 65

	beqz        a0, .ReduceExpressionStrength_label_360

	// *** Basic block 66

	ld          s8, 136(s6)
	bnez        s8, .ReduceExpressionStrength_label_333

	// *** Basic block 67

	li          t0, 11		// 0xb ASCII \xb
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 68

	mv          s3, a0
	mv          a1, s5
	mv          a0, s2
	call        Materialize

	// *** Basic block 69

	sd          a0, 40(s3)
	j           .ReduceExpressionStrength_label_359

	// *** Basic block 70

.ReduceExpressionStrength_label_333:
	li          t0, 64		// 0x40 ASCII '@'
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 71

	mv          s3, a0
	mv          a1, s5
	mv          a0, s2
	call        Materialize

	// *** Basic block 72

	sd          a0, 40(s3)
	addi        s5, s3, 40
	neg         a3, s8
	mv          a2, s4
	mv          a1, x0
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 73

	sd          a0, 8(s5)

	// *** Basic block 74

.ReduceExpressionStrength_label_359:

	// *** Basic block 75

.ReduceExpressionStrength_label_360:
	j           .ReduceExpressionStrength_label_685

	// *** Basic block 76

.ReduceExpressionStrength_label_362:
	addi        t0, s1, 24
	ld          t0, 8(t0)
	li          s4, 2		// 0x2 ASCII \x2
	bne         t0, s4, .ReduceExpressionStrength_label_371

	// *** Basic block 77

	j           .ReduceExpressionStrength_label_386

	// *** Basic block 78

.ReduceExpressionStrength_label_371:
	lla         a0, .str.117
	lla         a1, .str.118
	lla         a3, .str.119
	li          t0, 817		// 0x331
	mv          a2, t0
	call        printf

	// *** Basic block 79

	call        abort

	// *** Basic block 80

.ReduceExpressionStrength_label_386:
	ld          t0, 24(s1)
	ld          s5, 0(t0)
	ld          s6, 8(t0)
	mv          a0, s5
	call        IRIsConst

	// *** Basic block 81

	mv          s7, a0
	bnez        a0, .ReduceExpressionStrength_label_403

	// *** Basic block 82

	mv          a0, s6
	call        IRIsConst

	// *** Basic block 83

	mv          s7, a0

	// *** Basic block 84

.ReduceExpressionStrength_label_403:
	beqz        s7, .ReduceExpressionStrength_label_489

	// *** Basic block 85

	mv          a0, s5
	call        IRIsConst

	// *** Basic block 86

	mv          s8, a0
	beqz        a0, .ReduceExpressionStrength_label_415

	// *** Basic block 87

	mv          a0, s6
	call        IRIsConst

	// *** Basic block 88

	mv          s8, a0

	// *** Basic block 89

.ReduceExpressionStrength_label_415:
	beqz        s8, .ReduceExpressionStrength_label_441

	// *** Basic block 90

	ld          s9, 136(s5)
	ld          s10, 136(s6)
	li          t0, 17		// 0x11 ASCII \x11
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 91

	mv          s3, a0
	mul         a3, s9, s10
	mv          a2, s4
	mv          a1, x0
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 92

	sd          a0, 40(s3)
	j           .ReduceExpressionStrength_label_488

	// *** Basic block 93

.ReduceExpressionStrength_label_441:
	mv          a0, s5
	call        IRIsConst

	// *** Basic block 94

	beqz        a0, .ReduceExpressionStrength_label_450

	// *** Basic block 95

	mv          s9, s5
	mv          s5, s6
	mv          s6, s9

	// *** Basic block 96

.ReduceExpressionStrength_label_450:
	ld          s10, 136(s6)
	bnez        s10, .ReduceExpressionStrength_label_468

	// *** Basic block 97

	mv          a3, x0
	mv          a2, s4
	mv          a1, x0
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 98

	mv          s3, a0
	j           .ReduceExpressionStrength_label_487

	// *** Basic block 99

.ReduceExpressionStrength_label_468:
	li          t0, 1		// 0x1 ASCII \x1
	bne         s10, t0, .ReduceExpressionStrength_label_486

	// *** Basic block 100

	li          t0, 11		// 0xb ASCII \xb
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 101

	mv          s3, a0
	mv          a1, s5
	mv          a0, s2
	call        Materialize

	// *** Basic block 102

	sd          a0, 40(s3)

	// *** Basic block 103

.ReduceExpressionStrength_label_486:

	// *** Basic block 104

.ReduceExpressionStrength_label_487:

	// *** Basic block 105

.ReduceExpressionStrength_label_488:

	// *** Basic block 106

.ReduceExpressionStrength_label_489:
	j           .ReduceExpressionStrength_label_685

	// *** Basic block 107

.ReduceExpressionStrength_label_491:
	addi        t0, s1, 24
	ld          t0, 8(t0)
	li          s4, 2		// 0x2 ASCII \x2
	bne         t0, s4, .ReduceExpressionStrength_label_500

	// *** Basic block 108

	j           .ReduceExpressionStrength_label_515

	// *** Basic block 109

.ReduceExpressionStrength_label_500:
	lla         a0, .str.120
	lla         a1, .str.121
	lla         a3, .str.122
	li          t0, 848		// 0x350
	mv          a2, t0
	call        printf

	// *** Basic block 110

	call        abort

	// *** Basic block 111

.ReduceExpressionStrength_label_515:
	ld          t0, 24(s1)
	ld          s7, 0(t0)
	ld          s8, 8(t0)
	mv          a0, s7
	call        IRIsConst

	// *** Basic block 112

	mv          s10, a0
	beqz        a0, .ReduceExpressionStrength_label_532

	// *** Basic block 113

	mv          a0, s8
	call        IRIsConst

	// *** Basic block 114

	mv          s10, a0

	// *** Basic block 115

.ReduceExpressionStrength_label_532:
	beqz        s10, .ReduceExpressionStrength_label_561

	// *** Basic block 116

	ld          s11, 136(s7)
	ld          a0, 136(s8)
	beqz        a0, .ReduceExpressionStrength_label_560

	// *** Basic block 117

	li          t0, 17		// 0x11 ASCII \x11
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 118

	mv          s3, a0
	div         a3, s11, a0
	mv          a2, s4
	mv          a1, x0
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 119

	sd          a0, 40(s3)
	j           .ReduceExpressionStrength_label_685

	// *** Basic block 120

.ReduceExpressionStrength_label_560:

	// *** Basic block 121

.ReduceExpressionStrength_label_561:
	mv          a0, s8
	call        IRIsConst

	// *** Basic block 122

	beqz        a0, .ReduceExpressionStrength_label_586

	// *** Basic block 123

	ld          s4, 136(s8)
	li          t0, 1		// 0x1 ASCII \x1
	bne         s4, t0, .ReduceExpressionStrength_label_585

	// *** Basic block 124

	li          t0, 11		// 0xb ASCII \xb
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 125

	mv          s3, a0
	mv          a1, s7
	mv          a0, s2
	call        Materialize

	// *** Basic block 126

	sd          a0, 40(s3)

	// *** Basic block 127

.ReduceExpressionStrength_label_585:

	// *** Basic block 128

.ReduceExpressionStrength_label_586:
	j           .ReduceExpressionStrength_label_685

	// *** Basic block 129

.ReduceExpressionStrength_label_588:
	addi        t0, s1, 24
	ld          t0, 8(t0)
	li          s4, 2		// 0x2 ASCII \x2
	bne         t0, s4, .ReduceExpressionStrength_label_597

	// *** Basic block 130

	j           .ReduceExpressionStrength_label_612

	// *** Basic block 131

.ReduceExpressionStrength_label_597:
	lla         a0, .str.123
	lla         a1, .str.124
	lla         a3, .str.125
	li          t0, 874		// 0x36a
	mv          a2, t0
	call        printf

	// *** Basic block 132

	call        abort

	// *** Basic block 133

.ReduceExpressionStrength_label_612:
	ld          t0, 24(s1)
	ld          s7, 0(t0)
	ld          s8, 8(t0)
	mv          a0, s7
	call        IRIsConst

	// *** Basic block 134

	mv          s10, a0
	beqz        a0, .ReduceExpressionStrength_label_629

	// *** Basic block 135

	mv          a0, s8
	call        IRIsConst

	// *** Basic block 136

	mv          s10, a0

	// *** Basic block 137

.ReduceExpressionStrength_label_629:
	beqz        s10, .ReduceExpressionStrength_label_658

	// *** Basic block 138

	ld          s11, 136(s7)
	ld          s1, 136(s8)
	beqz        s1, .ReduceExpressionStrength_label_657

	// *** Basic block 139

	li          t0, 17		// 0x11 ASCII \x11
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 140

	mv          s3, a0
	div         a3, s11, s1
	mv          a2, s4
	mv          a1, x0
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 141

	sd          a0, 40(s3)
	j           .ReduceExpressionStrength_label_685

	// *** Basic block 142

.ReduceExpressionStrength_label_657:

	// *** Basic block 143

.ReduceExpressionStrength_label_658:
	mv          a0, s8
	call        IRIsConst

	// *** Basic block 144

	beqz        a0, .ReduceExpressionStrength_label_683

	// *** Basic block 145

	ld          s1, 136(s8)
	li          t0, 1		// 0x1 ASCII \x1
	bne         s1, t0, .ReduceExpressionStrength_label_682

	// *** Basic block 146

	li          t0, 11		// 0xb ASCII \xb
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 147

	mv          s3, a0
	mv          a1, s7
	mv          a0, s2
	call        Materialize

	// *** Basic block 148

	sd          a0, 40(s3)

	// *** Basic block 149

.ReduceExpressionStrength_label_682:

	// *** Basic block 150

.ReduceExpressionStrength_label_683:
	j           .ReduceExpressionStrength_label_685

	// *** Basic block 151

.ReduceExpressionStrength_label_685:
	mv          a0, s3

	// *** Basic block 152

.ReduceExpressionStrength_label_688:
	// Restored registers.
	ld s1, 80(sp)
	ld s2, 72(sp)
	ld s3, 64(sp)
	ld s4, 56(sp)
	ld s5, 48(sp)
	ld s6, 40(sp)
	ld s7, 32(sp)
	ld s8, 24(sp)
	ld s9, 16(sp)
	ld s10, 8(sp)
	ld s11, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ReduceExpressionStrength:
	.size ReduceExpressionStrength, .func_end_ReduceExpressionStrength-ReduceExpressionStrength

	.local  LowerExpression
	.type LowerExpression, @function

LowerExpression:

	// *** Basic block 0

	.local IR2PCode
	.global TypeIsUnsigned
	.global printf
	.global abort
	.local ReduceExpressionStrength
	.local NewInstruction
	.local Materialize
	.global TargetUpdateOperandUsers
	.local SetLoweredNode
	.local Emit
	addi sp, sp, -80
	// Saved return address (offset 72) and frame pointer (offset 64)
	sd ra, 72(sp)
	sd s0, 64(sp)
	addi s0, sp, 80
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 56(sp)
	sd s2, 48(sp)
	sd s3, 40(sp)
	sd s4, 32(sp)
	sd s5, 24(sp)
	sd s6, 16(sp)
	sd s7, 8(sp)
	// End of stack frame
	mv          s1, a1
	mv          s2, a0
	ld          a0, 96(s1)
	beq         a0, x0, .LowerExpression_label_42

	// *** Basic block 1

.LowerExpression_label_39:
	// Restored registers.
	ld s1, 56(sp)
	ld s2, 48(sp)
	ld s3, 40(sp)
	ld s4, 32(sp)
	ld s5, 24(sp)
	ld s6, 16(sp)
	ld s7, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 2

.LowerExpression_label_42:
	lw          s3, 20(s1)
	ld          a0, 80(s1)
	call        TypeIsUnsigned

	// *** Basic block 3

	mv          a1, a0
	mv          a0, s3
	call        IR2PCode

	// *** Basic block 4

	mv          s3, a0
	addi        t0, s1, 24
	ld          s4, 8(t0)
	li          t0, 2		// 0x2 ASCII \x2
	bge         s4, t0, .LowerExpression_label_66

	// *** Basic block 5

	ld          t0, 24(s1)
	j           .LowerExpression_label_83

	// *** Basic block 6

.LowerExpression_label_66:
	lla         a0, .str.126
	lla         a1, .str.127
	lla         a3, .str.128
	li          t0, 909		// 0x38d
	mv          a2, t0
	call        printf

	// *** Basic block 7

	call        abort

	// *** Basic block 8

.LowerExpression_label_83:
	mv          a2, s3
	mv          a1, s1
	mv          a0, s2
	call        ReduceExpressionStrength

	// *** Basic block 9

	mv          s5, a0
	bne         s5, x0, .LowerExpression_label_123

	// *** Basic block 10

	mv          a0, s3
	call        NewInstruction

	// *** Basic block 11

	mv          s5, a0
	mv          s6, x0
	bge         x0, s4, .LowerExpression_label_122

	// *** Basic block 12

.LowerExpression_label_104:
	slli        t0, s6, 3
	add         t1, t0, t0
	ld          s3, 0(t1)
	addi        t1, s5, 40
	add         s7, t1, t0
	mv          a1, s3
	mv          a0, s2
	call        Materialize

	// *** Basic block 13

	sd          a0, 0(s7)

	// *** Basic block 14

.LowerExpression_label_118:
	addi        s6, s6, 1
	bge         s6, s4, .LowerExpression_label_104

	// *** Basic block 15

.LowerExpression_label_122:

	// *** Basic block 16

.LowerExpression_label_123:
	mv          a0, s5
	call        TargetUpdateOperandUsers

	// *** Basic block 17

	mv          a1, s5
	mv          a0, s1
	call        SetLoweredNode

	// *** Basic block 18

	mv          a1, s5
	mv          a0, s2
	// Restored registers.
	ld s1, 56(sp)
	ld s2, 48(sp)
	ld s3, 40(sp)
	ld s4, 32(sp)
	ld s5, 24(sp)
	ld s6, 16(sp)
	ld s7, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           Emit
.func_end_LowerExpression:
	.size LowerExpression, .func_end_LowerExpression-LowerExpression

	.local  LowerConditionalBranch
	.type LowerConditionalBranch, @function

LowerConditionalBranch:

	// *** Basic block 0

	.global printf
	.global abort
	.local Emit
	.local NewInstruction1
	.local GetLoweredNode
	.global VectorAppend
	.global NewBranchFixup
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
	lw          s3, 20(s1)
	li          t0, 88		// 0x58 ASCII 'X'
	blt         s3, t0, .LowerConditionalBranch_label_63

	// *** Basic block 1

	li          t0, 89		// 0x59 ASCII 'Y'
	blt         t0, s3, .LowerConditionalBranch_label_63

	// *** Basic block 2

	addi        t0, s3, -88
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 3

	j           .LowerConditionalBranch_label_56

	// *** Basic block 4

	j           .LowerConditionalBranch_label_60

	// *** Basic block 5

.LowerConditionalBranch_label_56:
	li          s3, 110		// 0x6e ASCII 'n'
	j           .LowerConditionalBranch_label_80

	// *** Basic block 6

.LowerConditionalBranch_label_60:
	li          s3, 111		// 0x6f ASCII 'o'
	j           .LowerConditionalBranch_label_80

	// *** Basic block 7

.LowerConditionalBranch_label_63:
	lla         a0, .str.129
	lla         a1, .str.130
	lla         a3, .str.131
	li          t0, 935		// 0x3a7
	mv          a2, t0
	call        printf

	// *** Basic block 8

	call        abort

	// *** Basic block 9

.LowerConditionalBranch_label_80:
	addi        t0, s1, 24
	ld          t0, 8(t0)
	li          t1, 2		// 0x2 ASCII \x2
	bne         t0, t1, .LowerConditionalBranch_label_90

	// *** Basic block 10

	j           .LowerConditionalBranch_label_105

	// *** Basic block 11

.LowerConditionalBranch_label_90:
	lla         a0, .str.132
	lla         a1, .str.133
	lla         a3, .str.134
	li          t0, 937		// 0x3a9
	mv          a2, t0
	call        printf

	// *** Basic block 12

	call        abort

	// *** Basic block 13

.LowerConditionalBranch_label_105:
	ld          t0, 24(s1)
	ld          s4, 0(t0)
	ld          s5, 8(t0)
	mv          a0, s4
	call        GetLoweredNode

	// *** Basic block 14

	mv          a1, a0
	mv          a0, s3
	call        NewInstruction1

	// *** Basic block 15

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 16

	mv          s3, a0
	ld          s4, 96(s5)
	bne         s4, x0, .LowerConditionalBranch_label_148

	// *** Basic block 17

	addi        s1, s2, 136
	li          t0, 1		// 0x1 ASCII \x1
	mv          a2, t0
	mv          a1, s5
	mv          a0, s3
	call        NewBranchFixup

	// *** Basic block 18

	mv          a1, a0
	mv          a0, s1
	call        VectorAppend

	// *** Basic block 19

	j           .LowerConditionalBranch_label_152

	// *** Basic block 20

.LowerConditionalBranch_label_148:
	addi        t0, s3, 40
	sd          s4, 8(t0)

	// *** Basic block 21

.LowerConditionalBranch_label_152:
	mv          a0, s3

	// *** Basic block 22

.LowerConditionalBranch_label_155:
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
.func_end_LowerConditionalBranch:
	.size LowerConditionalBranch, .func_end_LowerConditionalBranch-LowerConditionalBranch

	.local  LowerBranch
	.type LowerBranch, @function

LowerBranch:

	// *** Basic block 0

	.global printf
	.global abort
	.local Emit
	.local NewInstruction
	.global VectorAppend
	.global NewBranchFixup
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
	mv          s1, a0
	addi        t0, a1, 24
	ld          t0, 8(t0)
	li          t1, 1		// 0x1 ASCII \x1
	bne         t0, t1, .LowerBranch_label_35

	// *** Basic block 1

	j           .LowerBranch_label_52

	// *** Basic block 2

.LowerBranch_label_35:
	lla         a0, .str.135
	lla         a1, .str.136
	lla         a3, .str.137
	li          t0, 955		// 0x3bb
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

.LowerBranch_label_52:
	ld          t0, 24(a1)
	ld          s2, 0(t0)
	li          t0, 112		// 0x70 ASCII 'p'
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 5

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 6

	mv          s3, a0
	ld          s4, 96(s2)
	bne         s4, x0, .LowerBranch_label_87

	// *** Basic block 7

	addi        s5, s1, 136
	mv          a2, x0
	mv          a1, s2
	mv          a0, s3
	call        NewBranchFixup

	// *** Basic block 8

	mv          a1, a0
	mv          a0, s5
	call        VectorAppend

	// *** Basic block 9

	j           .LowerBranch_label_90

	// *** Basic block 10

.LowerBranch_label_87:
	sd          s4, 40(s3)

	// *** Basic block 11

.LowerBranch_label_90:
	mv          a0, s3

	// *** Basic block 12

.LowerBranch_label_93:
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
.func_end_LowerBranch:
	.size LowerBranch, .func_end_LowerBranch-LowerBranch

	.local  LowerComputedBranch
	.type LowerComputedBranch, @function

LowerComputedBranch:

	// *** Basic block 0

	.global printf
	.global abort
	.local GetLoweredNode
	.local Emit
	.local NewInstruction1
	.local SetLoweredNode
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
	addi        t0, s1, 24
	ld          t0, 8(t0)
	li          t1, 1		// 0x1 ASCII \x1
	bne         t0, t1, .LowerComputedBranch_label_33

	// *** Basic block 1

	j           .LowerComputedBranch_label_50

	// *** Basic block 2

.LowerComputedBranch_label_33:
	lla         a0, .str.138
	lla         a1, .str.139
	lla         a3, .str.140
	li          t0, 973		// 0x3cd
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

.LowerComputedBranch_label_50:
	ld          t0, 24(s1)
	ld          a0, 0(t0)
	call        GetLoweredNode

	// *** Basic block 5

	mv          s3, a0
	mv          a1, s3
	li          t0, 113		// 0x71 ASCII 'q'
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 6

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 7

	mv          s4, a0
	mv          a1, s4
	mv          a0, s1
	call        SetLoweredNode

	// *** Basic block 8

	mv          a0, s4

	// *** Basic block 9

.LowerComputedBranch_label_77:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_LowerComputedBranch:
	.size LowerComputedBranch, .func_end_LowerComputedBranch-LowerComputedBranch

	.local  LowerLabel
	.type LowerLabel, @function

LowerLabel:

	// *** Basic block 0

	.local Emit
	.local NewInstruction
	.local ApplyFixups
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
	li          a0, 22		// 0x16 ASCII \x16
	call        NewInstruction

	// *** Basic block 1

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 2

	mv          s3, a0
	sd          s3, 96(s2)
	mv          a1, s2
	mv          a0, s1
	call        ApplyFixups

	// *** Basic block 3

	mv          a0, s3

	// *** Basic block 4

.LowerLabel_label_34:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_LowerLabel:
	.size LowerLabel, .func_end_LowerLabel-LowerLabel

	.local  LowerNamedLabel
	.type LowerNamedLabel, @function

LowerNamedLabel:

	// *** Basic block 0

	.local Emit
	.global TargetNewNamedLabel
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
	mv          s1, a1
	mv          s2, a0
	mv          t0, s1
	ld          a0, 136(t0)
	call        TargetNewNamedLabel

	// *** Basic block 1

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 2

	mv          s3, a0
	sd          s3, 96(s1)
	mv          a0, s3

	// *** Basic block 3

.LowerNamedLabel_label_31:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_LowerNamedLabel:
	.size LowerNamedLabel, .func_end_LowerNamedLabel-LowerNamedLabel

	.local  GetAddressAndOffset
	.type GetAddressAndOffset, @function

GetAddressAndOffset:

	// *** Basic block 0

	.global IRIsAutoVariable
	.local FramePointer
	.local GetIntConstant
	.global IRIsArgument
	.local ArgumentPointer
	.global IRIsThreadVariable
	.local GetTlsAddressAndOffset
	.global IRIsStaticVariable
	.local LoadStaticVariable
	.local GetLoweredNode
	.global printf
	.global abort
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
	mv          s2, a2
	mv          s3, a0
	mv          s4, a3
	mv          a0, s1
	call        IRIsAutoVariable

	// *** Basic block 1

	beqz        a0, .GetAddressAndOffset_label_66

	// *** Basic block 2

	mv          a0, s3
	call        FramePointer

	// *** Basic block 3

	sd          a0, 0(s2)
	addi        t0, s1, 96
	lw          s5, 8(t0)
	lw          t0, 128(s3)
	sub         a3, s5, t0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	mv          a1, s1
	mv          a0, s3
	call        GetIntConstant

	// *** Basic block 4

	sd          a0, 0(s4)
	j           .GetAddressAndOffset_label_194

	// *** Basic block 5

.GetAddressAndOffset_label_66:
	mv          a0, s1
	call        IRIsArgument

	// *** Basic block 6

	beqz        a0, .GetAddressAndOffset_label_91

	// *** Basic block 7

	mv          a0, s3
	call        ArgumentPointer

	// *** Basic block 8

	sd          a0, 0(s2)
	addi        t0, s1, 96
	lw          s5, 8(t0)
	mv          a3, s5
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	mv          a1, s1
	mv          a0, s3
	call        GetIntConstant

	// *** Basic block 9

	sd          a0, 0(s4)
	j           .GetAddressAndOffset_label_193

	// *** Basic block 10

.GetAddressAndOffset_label_91:
	mv          a0, s1
	call        IRIsThreadVariable

	// *** Basic block 11

	beqz        a0, .GetAddressAndOffset_label_106

	// *** Basic block 12

	mv          a3, s4
	mv          a2, s2
	mv          a1, s1
	mv          a0, s3
	call        GetTlsAddressAndOffset

	// *** Basic block 13

	j           .GetAddressAndOffset_label_192

	// *** Basic block 14

.GetAddressAndOffset_label_106:
	mv          a0, s1
	call        IRIsStaticVariable

	// *** Basic block 15

	beqz        a0, .GetAddressAndOffset_label_129

	// *** Basic block 16

	mv          a1, s1
	mv          a0, s3
	call        LoadStaticVariable

	// *** Basic block 17

	sd          a0, 0(s2)
	mv          a3, x0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	mv          a1, x0
	mv          a0, s3
	call        GetIntConstant

	// *** Basic block 18

	sd          a0, 0(s4)
	j           .GetAddressAndOffset_label_191

	// *** Basic block 19

.GetAddressAndOffset_label_129:
	lw          t0, 20(s1)
	li          t1, 102		// 0x66 ASCII 'f'
	bne         t0, t1, .GetAddressAndOffset_label_154

	// *** Basic block 20

	mv          a0, s3
	call        ArgumentPointer

	// *** Basic block 21

	sd          a0, 0(s2)
	li          t0, 16		// 0x10 ASCII \x10
	mv          a3, t0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	mv          a1, x0
	mv          a0, s3
	call        GetIntConstant

	// *** Basic block 22

	sd          a0, 0(s4)
	j           .GetAddressAndOffset_label_190

	// *** Basic block 23

.GetAddressAndOffset_label_154:
	mv          a0, s1
	call        GetLoweredNode

	// *** Basic block 24

	sd          a0, 0(s2)
	mv          a3, x0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	mv          a1, x0
	mv          a0, s3
	call        GetIntConstant

	// *** Basic block 25

	sd          a0, 0(s4)
	beq         s2, x0, .GetAddressAndOffset_label_174

	// *** Basic block 26

	j           .GetAddressAndOffset_label_189

	// *** Basic block 27

.GetAddressAndOffset_label_174:
	lla         a0, .str.141
	lla         a1, .str.142
	lla         a3, .str.143
	li          t0, 1025		// 0x401
	mv          a2, t0
	call        printf

	// *** Basic block 28

	call        abort

	// *** Basic block 29

.GetAddressAndOffset_label_189:

	// *** Basic block 30

.GetAddressAndOffset_label_190:

	// *** Basic block 31

.GetAddressAndOffset_label_191:

	// *** Basic block 32

.GetAddressAndOffset_label_192:

	// *** Basic block 33

.GetAddressAndOffset_label_193:

	// *** Basic block 34

.GetAddressAndOffset_label_194:
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
.func_end_GetAddressAndOffset:
	.size GetAddressAndOffset, .func_end_GetAddressAndOffset-GetAddressAndOffset

	.local  LowerLoad
	.type LowerLoad, @function

LowerLoad:

	// *** Basic block 0

	.global printf
	.global abort
	.local GetAddressAndOffset
	.local Emit
	.local NewInstruction2
	.local SetLoweredNode
	addi sp, sp, -64
	// Saved return address (offset 56) and frame pointer (offset 48)
	sd ra, 56(sp)
	sd s0, 48(sp)
	addi s0, sp, 64
	// Local vars at offset -32(s0)
	// Saved integer registers.
	sd s1, 24(sp)
	sd s2, 16(sp)
	sd s3, 8(sp)
	// End of stack frame
	mv          s1, a1
	mv          s2, a0
	addi        t0, s1, 24
	ld          t0, 8(t0)
	li          t1, 1		// 0x1 ASCII \x1
	bne         t0, t1, .LowerLoad_label_48

	// *** Basic block 1

	j           .LowerLoad_label_65

	// *** Basic block 2

.LowerLoad_label_48:
	lla         a0, .str.144
	lla         a1, .str.145
	lla         a3, .str.146
	li          t0, 1031		// 0x407
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

.LowerLoad_label_65:
	ld          t0, 24(s1)
	ld          s3, 0(t0)
	addi        a2, s0, -32
	addi        a3, s0, -24
	mv          a1, s3
	mv          a0, s2
	call        GetAddressAndOffset

	// *** Basic block 5

	lw          s3, 20(s1)
	li          t0, 19		// 0x13 ASCII \x13
	blt         s3, t0, .LowerLoad_label_139

	// *** Basic block 6

	li          t0, 28		// 0x1c ASCII \x1c
	blt         t0, s3, .LowerLoad_label_139

	// *** Basic block 7

	addi        t0, s3, -19
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 8

	j           .LowerLoad_label_108

	// *** Basic block 9

	j           .LowerLoad_label_112

	// *** Basic block 10

	j           .LowerLoad_label_115

	// *** Basic block 11

	j           .LowerLoad_label_118

	// *** Basic block 12

	j           .LowerLoad_label_121

	// *** Basic block 13

	j           .LowerLoad_label_124

	// *** Basic block 14

	j           .LowerLoad_label_127

	// *** Basic block 15

	j           .LowerLoad_label_130

	// *** Basic block 16

	j           .LowerLoad_label_133

	// *** Basic block 17

	j           .LowerLoad_label_136

	// *** Basic block 18

.LowerLoad_label_108:
	li          s3, 46		// 0x2e ASCII '.'
	j           .LowerLoad_label_154

	// *** Basic block 19

.LowerLoad_label_112:
	li          s3, 48		// 0x30 ASCII '0'
	j           .LowerLoad_label_154

	// *** Basic block 20

.LowerLoad_label_115:
	li          s3, 52		// 0x34 ASCII '4'
	j           .LowerLoad_label_154

	// *** Basic block 21

.LowerLoad_label_118:
	li          s3, 47		// 0x2f ASCII '/'
	j           .LowerLoad_label_154

	// *** Basic block 22

.LowerLoad_label_121:
	li          s3, 49		// 0x31 ASCII '1'
	j           .LowerLoad_label_154

	// *** Basic block 23

.LowerLoad_label_124:
	li          s3, 50		// 0x32 ASCII '2'
	j           .LowerLoad_label_154

	// *** Basic block 24

.LowerLoad_label_127:
	li          s3, 51		// 0x33 ASCII '3'
	j           .LowerLoad_label_154

	// *** Basic block 25

.LowerLoad_label_130:
	li          s3, 53		// 0x35 ASCII '5'
	j           .LowerLoad_label_154

	// *** Basic block 26

.LowerLoad_label_133:
	li          s3, 54		// 0x36 ASCII '6'
	j           .LowerLoad_label_154

	// *** Basic block 27

.LowerLoad_label_136:
	li          s3, 52		// 0x34 ASCII '4'
	j           .LowerLoad_label_154

	// *** Basic block 28

.LowerLoad_label_139:
	lla         a0, .str.147
	lla         a1, .str.148
	lla         a3, .str.149
	li          t0, 1069		// 0x42d
	mv          a2, t0
	call        printf

	// *** Basic block 29

	call        abort

	// *** Basic block 30

.LowerLoad_label_154:
	ld          a1, -32(s0)
	ld          a2, -24(s0)
	mv          a0, s3
	call        NewInstruction2

	// *** Basic block 31

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 32

	mv          s3, a0
	mv          a1, s3
	mv          a0, s1
	call        SetLoweredNode

	// *** Basic block 33

	mv          a0, s3

	// *** Basic block 34

.LowerLoad_label_176:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_LowerLoad:
	.size LowerLoad, .func_end_LowerLoad-LowerLoad

	.local  LowerStore
	.type LowerStore, @function

LowerStore:

	// *** Basic block 0

	.global printf
	.global abort
	.local GetAddressAndOffset
	.local Materialize
	.local Emit
	.local NewInstruction3
	.local SetLoweredNode
	addi sp, sp, -64
	// Saved return address (offset 56) and frame pointer (offset 48)
	sd ra, 56(sp)
	sd s0, 48(sp)
	addi s0, sp, 64
	// Local vars at offset -32(s0)
	// Saved integer registers.
	sd s1, 24(sp)
	sd s2, 16(sp)
	sd s3, 8(sp)
	sd s4, 0(sp)
	// End of stack frame
	mv          s1, a1
	mv          s2, a0
	addi        t0, s1, 24
	ld          t0, 8(t0)
	li          t1, 2		// 0x2 ASCII \x2
	bne         t0, t1, .LowerStore_label_47

	// *** Basic block 1

	j           .LowerStore_label_64

	// *** Basic block 2

.LowerStore_label_47:
	lla         a0, .str.150
	lla         a1, .str.151
	lla         a3, .str.152
	li          t0, 1080		// 0x438
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

.LowerStore_label_64:
	ld          s3, 24(s1)
	ld          s4, 0(s3)
	addi        a2, s0, -32
	addi        a3, s0, -24
	mv          a1, s4
	mv          a0, s2
	call        GetAddressAndOffset

	// *** Basic block 5

	ld          s4, 8(s3)
	mv          a1, s4
	mv          a0, s2
	call        Materialize

	// *** Basic block 6

	mv          s3, a0
	lw          s4, 20(s1)
	li          t0, 30		// 0x1e ASCII \x1e
	blt         s4, t0, .LowerStore_label_136

	// *** Basic block 7

	li          t0, 36		// 0x24 ASCII '$'
	blt         t0, s4, .LowerStore_label_136

	// *** Basic block 8

	addi        t0, s4, -30
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 9

	j           .LowerStore_label_114

	// *** Basic block 10

	j           .LowerStore_label_118

	// *** Basic block 11

	j           .LowerStore_label_124

	// *** Basic block 12

	j           .LowerStore_label_121

	// *** Basic block 13

	j           .LowerStore_label_127

	// *** Basic block 14

	j           .LowerStore_label_130

	// *** Basic block 15

	j           .LowerStore_label_133

	// *** Basic block 16

.LowerStore_label_114:
	li          s4, 55		// 0x37 ASCII '7'
	j           .LowerStore_label_151

	// *** Basic block 17

.LowerStore_label_118:
	li          s4, 60		// 0x3c ASCII '<'
	j           .LowerStore_label_151

	// *** Basic block 18

.LowerStore_label_121:
	li          s4, 57		// 0x39 ASCII '9'
	j           .LowerStore_label_151

	// *** Basic block 19

.LowerStore_label_124:
	li          s4, 56		// 0x38 ASCII '8'
	j           .LowerStore_label_151

	// *** Basic block 20

.LowerStore_label_127:
	li          s4, 58		// 0x3a ASCII ':'
	j           .LowerStore_label_151

	// *** Basic block 21

.LowerStore_label_130:
	li          s4, 59		// 0x3b ASCII ';'
	j           .LowerStore_label_151

	// *** Basic block 22

.LowerStore_label_133:
	li          s4, 57		// 0x39 ASCII '9'
	j           .LowerStore_label_151

	// *** Basic block 23

.LowerStore_label_136:
	lla         a0, .str.153
	lla         a1, .str.154
	lla         a3, .str.155
	li          t0, 1117		// 0x45d
	mv          a2, t0
	call        printf

	// *** Basic block 24

	call        abort

	// *** Basic block 25

.LowerStore_label_151:
	ld          a2, -32(s0)
	ld          a3, -24(s0)
	mv          a1, s3
	mv          a0, s4
	call        NewInstruction3

	// *** Basic block 26

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 27

	mv          s4, a0
	mv          a1, s4
	mv          a0, s1
	call        SetLoweredNode

	// *** Basic block 28

	mv          a0, s4

	// *** Basic block 29

.LowerStore_label_175:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_LowerStore:
	.size LowerStore, .func_end_LowerStore-LowerStore

	.local  PushArg
	.type PushArg, @function

PushArg:

	// *** Basic block 0

	.local Emit
	.local NewInstruction1
	.local push_map
	.global printf
	.global abort
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
	mv          s2, a2
	mv          s3, a3
	ld          s4, 80(a1)
	bne         s4, x0, .PushArg_label_51

	// *** Basic block 1

	ld          s5, 0(s3)
	mv          a1, s2
	li          t0, 38		// 0x26 ASCII '&'
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 2

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 3

.PushArg_label_51:
	mv          s5, x0
	la          t0, push_map
	ld          t0, 0(t0)
	beq         t0, x0, .PushArg_label_106

	// *** Basic block 4

.PushArg_label_60:
	slli        t0, s5, 3
	slli        t1, s5, 4
	add         t0, t0, t1
	la          t1, push_map
	add         s6, t1, t0
	ld          t0, 0(s6)
	mv          a0, s4
	jalr         x1, t0, 0

	// *** Basic block 5

	beqz        a0, .PushArg_label_94

	// *** Basic block 6

	beq         s3, x0, .PushArg_label_78

	// *** Basic block 7

	ld          t0, 16(s6)
	add         t0, s5, t0
	sd          t0, 0(s3)

	// *** Basic block 8

.PushArg_label_78:
	lw          a0, 8(s6)
	mv          a1, s2
	call        NewInstruction1

	// *** Basic block 9

	mv          a1, a0
	mv          a0, s1
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
	j           Emit

	// *** Basic block 10

.PushArg_label_91:
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
	ret         

	// *** Basic block 11

.PushArg_label_94:

	// *** Basic block 12

.PushArg_label_95:
	addi        s5, s5, 1
	slli        t0, s5, 3
	slli        t1, s5, 4
	add         t0, t0, t1
	la          t1, push_map
	add         t0, t1, t0
	ld          t0, 0(t0)
	beq         t0, x0, .PushArg_label_60

	// *** Basic block 13

.PushArg_label_106:
	lla         a0, .str.156
	lla         a1, .str.157
	lla         a3, .str.158
	li          t0, 1160		// 0x488
	mv          a2, t0
	call        printf

	// *** Basic block 14

	call        abort

	// *** Basic block 15

	j           .PushArg_label_91
.func_end_PushArg:
	.size PushArg, .func_end_PushArg-PushArg

	.local  PushStructArg
	.type PushStructArg, @function

PushStructArg:

	// *** Basic block 0

	.local GetIntConstant
	.local Emit
	.local NewInstruction1
	.local StackPointer
	.local GetAddressAndOffset
	.local NewInstruction2
	.local GetSymbol
	addi sp, sp, -96
	// Saved return address (offset 88) and frame pointer (offset 80)
	sd ra, 88(sp)
	sd s0, 80(sp)
	addi s0, sp, 96
	// Local vars at offset -32(s0)
	// Saved integer registers.
	sd s1, 56(sp)
	sd s2, 48(sp)
	sd s3, 40(sp)
	sd s4, 32(sp)
	sd s5, 24(sp)
	sd s6, 16(sp)
	sd s7, 8(sp)
	// End of stack frame
	mv          s1, a1
	mv          t0, a2
	mv          s2, a0
	ld          t1, 80(s1)
	lw          t2, 20(t1)
	ld          t1, 0(t0)
	add         t1, t1, t2
	sd          t1, 0(t0)
	mv          a3, t2
	li          s3, 2		// 0x2 ASCII \x2
	mv          a2, s3
	mv          a1, x0
	call        GetIntConstant

	// *** Basic block 1

	mv          s4, a0
	mv          a1, s4
	li          t0, 36		// 0x24 ASCII '$'
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 2

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 3

	mv          a0, s2
	call        StackPointer

	// *** Basic block 4

	mv          a1, a0
	li          t0, 11		// 0xb ASCII \xb
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 5

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 6

	mv          s5, a0
	mv          a1, s4
	li          t0, 14		// 0xe ASCII \xe
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 7

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 8

	mv          s6, a0
	mv          a1, s6
	li          s7, 41		// 0x29 ASCII ')'
	mv          a0, s7
	call        NewInstruction1

	// *** Basic block 9

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 10

	addi        a2, s0, -32
	addi        a3, s0, -24
	mv          a1, s1
	mv          a0, s2
	call        GetAddressAndOffset

	// *** Basic block 11

	ld          a1, -32(s0)
	ld          a2, -24(s0)
	li          t0, 64		// 0x40 ASCII '@'
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 12

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 13

	sd          a0, -32(s0)
	ld          a1, -32(s0)
	mv          a0, s7
	call        NewInstruction1

	// *** Basic block 14

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 15

	mv          a1, s5
	li          t0, 38		// 0x26 ASCII '&'
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 16

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 17

	ld          a2, 160(s2)
	mv          a1, x0
	mv          a0, s2
	call        GetSymbol

	// *** Basic block 18

	mv          s7, a0
	mv          a1, s7
	li          t0, 129		// 0x81 ASCII \x81
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 19

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 20

	li          t0, 24		// 0x18 ASCII \x18
	mv          a3, t0
	mv          a2, s3
	mv          a1, x0
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 21

	mv          a1, a0
	li          t0, 37		// 0x25 ASCII '%'
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 22

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 23

	// Restored registers.
	ld s1, 56(sp)
	ld s2, 48(sp)
	ld s3, 40(sp)
	ld s4, 32(sp)
	ld s5, 24(sp)
	ld s6, 16(sp)
	ld s7, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_PushStructArg:
	.size PushStructArg, .func_end_PushStructArg-PushStructArg

	.local  LowerCall
	.type LowerCall, @function

LowerCall:

	// *** Basic block 0

	.global printf
	.global abort
	.global TypeIsStructOrUnion
	.local PushStructArg
	.local Materialize
	.local PushArg
	.local GetLoweredNode
	.global TypeIsFloat
	.global TypeIsDouble
	.local Emit
	.local NewInstruction1
	.local GetIntConstant
	.local SetLoweredNode
	addi sp, sp, -96
	// Saved return address (offset 88) and frame pointer (offset 80)
	sd ra, 88(sp)
	sd s0, 80(sp)
	addi s0, sp, 96
	// Local vars at offset -32(s0)
	// Saved integer registers.
	sd s1, 56(sp)
	sd s2, 48(sp)
	sd s3, 40(sp)
	sd s4, 32(sp)
	sd s5, 24(sp)
	sd s6, 16(sp)
	sd s7, 8(sp)
	// End of stack frame
	mv          s1, a1
	mv          s2, a0
	addi        t0, s1, 24
	ld          s3, 8(t0)
	li          s4, 1		// 0x1 ASCII \x1
	blt         s3, s4, .LowerCall_label_51

	// *** Basic block 1

	ld          t0, 24(s1)
	j           .LowerCall_label_68

	// *** Basic block 2

.LowerCall_label_51:
	lla         a0, .str.159
	lla         a1, .str.160
	lla         a3, .str.161
	li          t0, 1204		// 0x4b4
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

.LowerCall_label_68:
	sd          x0, -32(s0)
	addi        s5, s3, -1
	blt         s5, s4, .LowerCall_label_122

	// *** Basic block 5

.LowerCall_label_79:
	slli        t0, s5, 3
	add         t0, t0, t0
	ld          s3, 0(t0)
	ld          a0, 80(s3)
	call        TypeIsStructOrUnion

	// *** Basic block 6

	beqz        a0, .LowerCall_label_98

	// *** Basic block 7

	addi        a2, s0, -32
	mv          a1, s3
	mv          a0, s2
	call        PushStructArg

	// *** Basic block 8

	j           .LowerCall_label_115

	// *** Basic block 9

.LowerCall_label_98:
	mv          a1, s3
	mv          a0, s2
	call        Materialize

	// *** Basic block 10

	mv          s6, a0
	addi        a3, s0, -32
	mv          a2, s6
	mv          a1, s3
	mv          a0, s2
	call        PushArg

	// *** Basic block 11

.LowerCall_label_115:

	// *** Basic block 12

.LowerCall_label_116:
	addi        s5, s5, -1
	blt         s5, s4, .LowerCall_label_79

	// *** Basic block 13

.LowerCall_label_122:
	ld          t0, 24(s1)
	ld          a0, 0(t0)
	call        GetLoweredNode

	// *** Basic block 14

	mv          s3, a0
	lw          t0, 16(s3)
	li          s4, 2		// 0x2 ASCII \x2
	bne         t0, s4, .LowerCall_label_157

	// *** Basic block 15

	ld          s5, 80(s1)
	mv          a0, s5
	call        TypeIsFloat

	// *** Basic block 16

	beqz        a0, .LowerCall_label_145

	// *** Basic block 17

	li          s7, 130		// 0x82 ASCII \x82
	j           .LowerCall_label_155

	// *** Basic block 18

.LowerCall_label_145:
	mv          a0, s5
	call        TypeIsDouble

	// *** Basic block 19

	beqz        a0, .LowerCall_label_152

	// *** Basic block 20

	li          s7, 131		// 0x83 ASCII \x83
	j           .LowerCall_label_154

	// *** Basic block 21

.LowerCall_label_152:
	li          s7, 129		// 0x81 ASCII \x81

	// *** Basic block 22

.LowerCall_label_154:

	// *** Basic block 23

.LowerCall_label_155:
	j           .LowerCall_label_177

	// *** Basic block 24

.LowerCall_label_157:
	ld          s5, 80(s1)
	mv          a0, s5
	call        TypeIsFloat

	// *** Basic block 25

	beqz        a0, .LowerCall_label_166

	// *** Basic block 26

	li          s7, 133		// 0x85 ASCII \x85
	j           .LowerCall_label_176

	// *** Basic block 27

.LowerCall_label_166:
	mv          a0, s5
	call        TypeIsDouble

	// *** Basic block 28

	beqz        a0, .LowerCall_label_173

	// *** Basic block 29

	li          s7, 134		// 0x86 ASCII \x86
	j           .LowerCall_label_175

	// *** Basic block 30

.LowerCall_label_173:
	li          s7, 132		// 0x84 ASCII \x84

	// *** Basic block 31

.LowerCall_label_175:

	// *** Basic block 32

.LowerCall_label_176:

	// *** Basic block 33

.LowerCall_label_177:
	mv          a1, s3
	mv          a0, s7
	call        NewInstruction1

	// *** Basic block 34

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 35

	mv          s5, a0
	ld          s7, -32(s0)
	bge         x0, s7, .LowerCall_label_214

	// *** Basic block 36

	mv          a3, s7
	mv          a2, s4
	mv          a1, x0
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 37

	mv          a1, a0
	li          t0, 37		// 0x25 ASCII '%'
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 38

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 39

.LowerCall_label_214:
	mv          a1, s5
	mv          a0, s1
	call        SetLoweredNode

	// *** Basic block 40

	mv          a0, s5

	// *** Basic block 41

.LowerCall_label_222:
	// Restored registers.
	ld s1, 56(sp)
	ld s2, 48(sp)
	ld s3, 40(sp)
	ld s4, 32(sp)
	ld s5, 24(sp)
	ld s6, 16(sp)
	ld s7, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_LowerCall:
	.size LowerCall, .func_end_LowerCall-LowerCall

	.local  LowerResult
	.type LowerResult, @function

LowerResult:

	// *** Basic block 0

	.global printf
	.global abort
	.local Materialize
	.local Emit
	.local NewInstruction
	.local NewInstruction2
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
	addi        t0, s1, 24
	ld          t0, 8(t0)
	li          t1, 1		// 0x1 ASCII \x1
	bne         t0, t1, .LowerResult_label_44

	// *** Basic block 1

	j           .LowerResult_label_61

	// *** Basic block 2

.LowerResult_label_44:
	lla         a0, .str.162
	lla         a1, .str.163
	lla         a3, .str.164
	li          t0, 1245		// 0x4dd
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

.LowerResult_label_61:
	lw          s3, 20(s1)
	li          t0, 106		// 0x6a ASCII 'j'
	blt         s3, t0, .LowerResult_label_98

	// *** Basic block 5

	li          t0, 109		// 0x6d ASCII 'm'
	blt         t0, s3, .LowerResult_label_98

	// *** Basic block 6

	addi        t0, s3, -106
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 7

	j           .LowerResult_label_83

	// *** Basic block 8

	j           .LowerResult_label_90

	// *** Basic block 9

	j           .LowerResult_label_94

	// *** Basic block 10

	j           .LowerResult_label_84

	// *** Basic block 11

.LowerResult_label_83:

	// *** Basic block 12

.LowerResult_label_84:
	li          s3, 26		// 0x1a ASCII \x1a
	li          s4, 18		// 0x12 ASCII \x12
	j           .LowerResult_label_113

	// *** Basic block 13

.LowerResult_label_90:
	li          s3, 27		// 0x1b ASCII \x1b
	li          s4, 19		// 0x13 ASCII \x13
	j           .LowerResult_label_113

	// *** Basic block 14

.LowerResult_label_94:
	li          s3, 28		// 0x1c ASCII \x1c
	li          s4, 20		// 0x14 ASCII \x14
	j           .LowerResult_label_113

	// *** Basic block 15

.LowerResult_label_98:
	lla         a0, .str.165
	lla         a1, .str.166
	lla         a3, .str.167
	li          t0, 1262		// 0x4ee
	mv          a2, t0
	call        printf

	// *** Basic block 16

	call        abort

	// *** Basic block 17

.LowerResult_label_113:
	ld          t0, 24(s1)
	ld          a1, 0(t0)
	mv          a0, s2
	call        Materialize

	// *** Basic block 18

	mv          s5, a0
	mv          a0, s3
	call        NewInstruction

	// *** Basic block 19

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 20

	mv          s3, a0
	mv          a2, s5
	mv          a1, s3
	mv          a0, s4
	call        NewInstruction2

	// *** Basic block 21

	mv          a1, a0
	mv          a0, s2
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           Emit
.func_end_LowerResult:
	.size LowerResult, .func_end_LowerResult-LowerResult

	.local  LowerLiteralReference
	.type LowerLiteralReference, @function

LowerLiteralReference:

	// *** Basic block 0

	.local Emit
	.global TargetNewLiteral
	.local NewInstruction1
	.local SetLoweredNode
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
	ld          t0, 24(s1)
	ld          t1, 0(t0)
	ld          a0, 136(t1)
	call        TargetNewLiteral

	// *** Basic block 1

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 2

	mv          s3, a0
	mv          a1, s3
	li          t0, 127		// 0x7f ASCII \x7f
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 3

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 4

	mv          s4, a0
	mv          a1, s4
	mv          a0, s1
	call        SetLoweredNode

	// *** Basic block 5

	mv          a0, s4

	// *** Basic block 6

.LowerLiteralReference_label_53:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_LowerLiteralReference:
	.size LowerLiteralReference, .func_end_LowerLiteralReference-LowerLiteralReference

	.local  LowerAddressOf
	.type LowerAddressOf, @function

LowerAddressOf:

	// *** Basic block 0

	.local SetLoweredNode
	.local Materialize
	addi sp, sp, -32
	// Saved return address (offset 24) and frame pointer (offset 16)
	sd ra, 24(sp)
	sd s0, 16(sp)
	addi s0, sp, 32
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 8(sp)
	// End of stack frame
	mv          s1, a1
	ld          t0, 24(s1)
	ld          a1, 0(t0)
	call        Materialize

	// *** Basic block 1

	mv          a1, a0
	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           SetLoweredNode
.func_end_LowerAddressOf:
	.size LowerAddressOf, .func_end_LowerAddressOf-LowerAddressOf

	.local  LowerMemcpy
	.type LowerMemcpy, @function

LowerMemcpy:

	// *** Basic block 0

	.global printf
	.global abort
	.local Emit
	.local Materialize
	.local NewInstruction1
	.local GetAddressAndOffset
	.local NewInstruction2
	.local PushArg
	.local GetSymbol
	.local GetIntConstant
	.local SetLoweredNode
	addi sp, sp, -96
	// Saved return address (offset 88) and frame pointer (offset 80)
	sd ra, 88(sp)
	sd s0, 80(sp)
	addi s0, sp, 96
	// Local vars at offset -48(s0)
	// Saved integer registers.
	sd s1, 40(sp)
	sd s2, 32(sp)
	sd s3, 24(sp)
	sd s4, 16(sp)
	sd s5, 8(sp)
	sd s6, 0(sp)
	// End of stack frame
	mv          s1, a1
	mv          s2, a0
	addi        t0, s1, 24
	ld          t0, 8(t0)
	li          t1, 3		// 0x3 ASCII \x3
	bne         t0, t1, .LowerMemcpy_label_47

	// *** Basic block 1

	j           .LowerMemcpy_label_64

	// *** Basic block 2

.LowerMemcpy_label_47:
	lla         a0, .str.168
	lla         a1, .str.169
	lla         a3, .str.170
	li          t0, 1294		// 0x50e
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

.LowerMemcpy_label_64:
	ld          s3, 24(s1)
	ld          a1, 16(s3)
	mv          a0, s2
	call        Materialize

	// *** Basic block 5

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 6

	mv          s4, a0
	mv          a1, s4
	li          t0, 41		// 0x29 ASCII ')'
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 7

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 8

	ld          s5, 8(s3)
	addi        a2, s0, -48
	addi        a3, s0, -40
	mv          a1, s5
	mv          a0, s2
	call        GetAddressAndOffset

	// *** Basic block 9

	ld          a1, -48(s0)
	ld          a2, -40(s0)
	li          s6, 64		// 0x40 ASCII '@'
	mv          a0, s6
	call        NewInstruction2

	// *** Basic block 10

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 11

	sd          a0, -48(s0)
	ld          a2, -48(s0)
	sd          a2, 96(s5)
	mv          a3, x0
	mv          a1, s5
	mv          a0, s2
	call        PushArg

	// *** Basic block 12

	ld          s5, 0(s3)
	addi        a2, s0, -32
	addi        a3, s0, -24
	mv          a1, s5
	mv          a0, s2
	call        GetAddressAndOffset

	// *** Basic block 13

	ld          a1, -32(s0)
	ld          a2, -24(s0)
	mv          a0, s6
	call        NewInstruction2

	// *** Basic block 14

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 15

	sd          a0, -32(s0)
	ld          a2, -32(s0)
	sd          a2, 96(s5)
	mv          a3, x0
	mv          a1, s5
	mv          a0, s2
	call        PushArg

	// *** Basic block 16

	ld          a2, 160(s2)
	mv          a1, x0
	mv          a0, s2
	call        GetSymbol

	// *** Basic block 17

	mv          s3, a0
	mv          a1, s3
	li          t0, 129		// 0x81 ASCII \x81
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 18

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 19

	mv          s5, a0
	li          t0, 24		// 0x18 ASCII \x18
	mv          a3, t0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	mv          a1, x0
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 20

	mv          a1, a0
	li          t0, 37		// 0x25 ASCII '%'
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 21

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 22

	mv          a1, s5
	mv          a0, s1
	call        SetLoweredNode

	// *** Basic block 23

	mv          a0, s5

	// *** Basic block 24

.LowerMemcpy_label_220:
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
	ret         
.func_end_LowerMemcpy:
	.size LowerMemcpy, .func_end_LowerMemcpy-LowerMemcpy

	.local  LowerMemzero
	.type LowerMemzero, @function

LowerMemzero:

	// *** Basic block 0

	.global printf
	.global abort
	.local Emit
	.local NewInstruction1
	.local GetIntConstant
	.local GetAddressAndOffset
	.local NewInstruction2
	.local PushArg
	.local GetSymbol
	.local SetLoweredNode
	addi sp, sp, -96
	// Saved return address (offset 88) and frame pointer (offset 80)
	sd ra, 88(sp)
	sd s0, 80(sp)
	addi s0, sp, 96
	// Local vars at offset -32(s0)
	// Saved integer registers.
	sd s1, 56(sp)
	sd s2, 48(sp)
	sd s3, 40(sp)
	sd s4, 32(sp)
	sd s5, 24(sp)
	sd s6, 16(sp)
	sd s7, 8(sp)
	sd s8, 0(sp)
	// End of stack frame
	mv          s1, a1
	mv          s2, a0
	addi        t0, s1, 24
	ld          t0, 8(t0)
	li          t1, 1		// 0x1 ASCII \x1
	bne         t0, t1, .LowerMemzero_label_50

	// *** Basic block 1

	j           .LowerMemzero_label_67

	// *** Basic block 2

.LowerMemzero_label_50:
	lla         a0, .str.171
	lla         a1, .str.172
	lla         a3, .str.173
	li          t0, 1337		// 0x539
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

.LowerMemzero_label_67:
	ld          t0, 24(s1)
	ld          s3, 0(t0)
	mv          s4, s3
	ld          t0, 136(s4)
	ld          t0, 40(t0)
	lw          a3, 20(t0)
	li          s5, 2		// 0x2 ASCII \x2
	mv          a2, s5
	mv          a1, x0
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 5

	mv          a1, a0
	li          s6, 14		// 0xe ASCII \xe
	mv          a0, s6
	call        NewInstruction1

	// *** Basic block 6

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 7

	mv          s7, a0
	mv          a1, s7
	li          t0, 41		// 0x29 ASCII ')'
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 8

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 9

	mv          a3, x0
	mv          a2, s5
	mv          a1, x0
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 10

	mv          a1, a0
	mv          a0, s6
	call        NewInstruction1

	// *** Basic block 11

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 12

	mv          s6, a0
	mv          a1, s6
	li          t0, 38		// 0x26 ASCII '&'
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 13

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 14

	addi        a2, s0, -32
	addi        a3, s0, -24
	mv          a1, s3
	mv          a0, s2
	call        GetAddressAndOffset

	// *** Basic block 15

	ld          a1, -32(s0)
	ld          a2, -24(s0)
	li          t0, 64		// 0x40 ASCII '@'
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 16

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 17

	sd          a0, -32(s0)
	ld          a2, -32(s0)
	sd          a2, 96(s3)
	mv          a3, x0
	mv          a1, s3
	mv          a0, s2
	call        PushArg

	// *** Basic block 18

	ld          a2, 168(s2)
	mv          a1, x0
	mv          a0, s2
	call        GetSymbol

	// *** Basic block 19

	mv          s3, a0
	mv          a1, s3
	li          t0, 129		// 0x81 ASCII \x81
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 20

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 21

	mv          s8, a0
	li          t0, 20		// 0x14 ASCII \x14
	mv          a3, t0
	mv          a2, s5
	mv          a1, x0
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 22

	mv          a1, a0
	li          t0, 37		// 0x25 ASCII '%'
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 23

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 24

	mv          a1, s8
	mv          a0, s1
	call        SetLoweredNode

	// *** Basic block 25

	mv          a0, s8

	// *** Basic block 26

.LowerMemzero_label_235:
	// Restored registers.
	ld s1, 56(sp)
	ld s2, 48(sp)
	ld s3, 40(sp)
	ld s4, 32(sp)
	ld s5, 24(sp)
	ld s6, 16(sp)
	ld s7, 8(sp)
	ld s8, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_LowerMemzero:
	.size LowerMemzero, .func_end_LowerMemzero-LowerMemzero

	.local  LowerZeroExtend
	.type LowerZeroExtend, @function

LowerZeroExtend:

	// *** Basic block 0

	.local Materialize
	.local Emit
	.local NewInstruction2
	.local SetLoweredNode
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
	ld          s3, 24(s2)
	ld          a1, 0(s3)
	call        Materialize

	// *** Basic block 1

	mv          s4, a0
	ld          a1, 8(s3)
	mv          a0, s1
	call        Materialize

	// *** Basic block 2

	mv          a2, a0
	mv          a1, s4
	li          t0, 81		// 0x51 ASCII 'Q'
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 3

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 4

	mv          s4, a0
	mv          a1, s4
	mv          a0, s2
	call        SetLoweredNode

	// *** Basic block 5

	mv          a0, s4

	// *** Basic block 6

.LowerZeroExtend_label_55:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_LowerZeroExtend:
	.size LowerZeroExtend, .func_end_LowerZeroExtend-LowerZeroExtend

	.local  LowerSignExtend
	.type LowerSignExtend, @function

LowerSignExtend:

	// *** Basic block 0

	.local Materialize
	.global PCodeIsSignedLoad
	.local SetLoweredNode
	.local Emit
	.local NewInstruction1
	.local GetIntConstant
	.local NewInstruction2
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
	mv          s2, a1
	ld          s3, 24(s2)
	ld          a1, 0(s3)
	call        Materialize

	// *** Basic block 1

	mv          s4, a0
	lw          a0, 16(s4)
	call        PCodeIsSignedLoad

	// *** Basic block 2

	beqz        a0, .LowerSignExtend_label_51

	// *** Basic block 3

	mv          a1, s4
	mv          a0, s2
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
	j           SetLoweredNode

	// *** Basic block 5

.LowerSignExtend_label_48:
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
	ret         

	// *** Basic block 6

.LowerSignExtend_label_51:
	ld          s5, 8(s3)
	ld          s3, 136(s5)
	mv          a3, s3
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	mv          a1, x0
	mv          a0, s1
	call        GetIntConstant

	// *** Basic block 7

	mv          a1, a0
	li          t0, 14		// 0xe ASCII \xe
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 8

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 9

	mv          s3, a0
	mv          a2, s3
	mv          a1, s4
	li          t0, 79		// 0x4f ASCII 'O'
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 10

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 11

	mv          s5, a0
	mv          a2, s3
	mv          a1, s5
	li          t0, 78		// 0x4e ASCII 'N'
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 12

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 13

	mv          s6, a0
	mv          a1, s6
	mv          a0, s2
	call        SetLoweredNode

	// *** Basic block 14

	mv          a0, s6
	j           .LowerSignExtend_label_48
.func_end_LowerSignExtend:
	.size LowerSignExtend, .func_end_LowerSignExtend-LowerSignExtend

	.local  LowerAlign
	.type LowerAlign, @function

LowerAlign:

	// *** Basic block 0

	.local Materialize
	.local GetIntConstant
	.local Emit
	.local NewInstruction2
	.local SetLoweredNode
	addi sp, sp, -80
	// Saved return address (offset 72) and frame pointer (offset 64)
	sd ra, 72(sp)
	sd s0, 64(sp)
	addi s0, sp, 80
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 56(sp)
	sd s2, 48(sp)
	sd s3, 40(sp)
	sd s4, 32(sp)
	sd s5, 24(sp)
	sd s6, 16(sp)
	sd s7, 8(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	ld          s3, 24(s2)
	ld          a1, 0(s3)
	call        Materialize

	// *** Basic block 1

	mv          s4, a0
	ld          s5, 8(s3)
	ld          t0, 136(s5)
	addi        s3, t0, -1
	mv          a3, s3
	li          s5, 2		// 0x2 ASCII \x2
	mv          a2, s5
	mv          a1, x0
	mv          a0, s1
	call        GetIntConstant

	// *** Basic block 2

	mv          s6, a0
	not         a3, s3
	mv          a2, s5
	mv          a1, x0
	mv          a0, s1
	call        GetIntConstant

	// *** Basic block 3

	mv          s3, a0
	mv          a2, s6
	mv          a1, s4
	li          t0, 61		// 0x3d ASCII '='
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 4

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 5

	mv          s5, a0
	mv          a2, s3
	mv          a1, s5
	li          t0, 81		// 0x51 ASCII 'Q'
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 6

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 7

	mv          s7, a0
	mv          a1, s7
	mv          a0, s2
	call        SetLoweredNode

	// *** Basic block 8

	mv          a0, s7

	// *** Basic block 9

.LowerAlign_label_102:
	// Restored registers.
	ld s1, 56(sp)
	ld s2, 48(sp)
	ld s3, 40(sp)
	ld s4, 32(sp)
	ld s5, 24(sp)
	ld s6, 16(sp)
	ld s7, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_LowerAlign:
	.size LowerAlign, .func_end_LowerAlign-LowerAlign

	.local  LowerAsm
	.type LowerAsm, @function

LowerAsm:

	// *** Basic block 0

	.local Emit
	.global TargetNewLiteral
	.local NewInstruction1
	.local SetLoweredNode
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
	ld          t0, 24(s1)
	ld          t1, 0(t0)
	ld          a0, 136(t1)
	call        TargetNewLiteral

	// *** Basic block 1

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 2

	mv          s3, a0
	mv          a1, s3
	li          t0, 30		// 0x1e ASCII \x1e
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 3

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 4

	mv          s4, a0
	mv          a1, s4
	mv          a0, s1
	call        SetLoweredNode

	// *** Basic block 5

	mv          a0, s4

	// *** Basic block 6

.LowerAsm_label_53:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_LowerAsm:
	.size LowerAsm, .func_end_LowerAsm-LowerAsm

	.local  LowerLocation
	.type LowerLocation, @function

LowerLocation:

	// *** Basic block 0

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
	.local SetLoweredNode
	.local Emit
	.global TargetNewLocation
	mv          s1, a1
	mv          s2, a0
	mv          t0, s1
	mv          a0, t0
	call        TargetNewLocation

	// *** Basic block 1

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 2

	mv          a1, a0
	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           SetLoweredNode
.func_end_LowerLocation:
	.size LowerLocation, .func_end_LowerLocation-LowerLocation

	.local  LowerBuiltinVaStart
	.type LowerBuiltinVaStart, @function

LowerBuiltinVaStart:

	// *** Basic block 0

	.local GetAddressAndOffset
	.local Emit
	.local NewInstruction2
	.local GetIntConstant
	.local NewInstruction3
	.local SetLoweredNode
	addi sp, sp, -96
	// Saved return address (offset 88) and frame pointer (offset 80)
	sd ra, 88(sp)
	sd s0, 80(sp)
	addi s0, sp, 96
	// Local vars at offset -48(s0)
	// Saved integer registers.
	sd s1, 40(sp)
	sd s2, 32(sp)
	sd s3, 24(sp)
	sd s4, 16(sp)
	sd s5, 8(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	ld          s3, 24(s2)
	ld          a1, 8(s3)
	addi        a2, s0, -48
	addi        a3, s0, -40
	call        GetAddressAndOffset

	// *** Basic block 1

	ld          a1, -48(s0)
	ld          a2, -40(s0)
	li          s4, 64		// 0x40 ASCII '@'
	mv          a0, s4
	call        NewInstruction2

	// *** Basic block 2

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 3

	mv          s5, a0
	li          t0, 8		// 0x8 ASCII \x8
	mv          a3, t0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	mv          a1, x0
	mv          a0, s1
	call        GetIntConstant

	// *** Basic block 4

	mv          a2, a0
	mv          a1, s5
	mv          a0, s4
	call        NewInstruction2

	// *** Basic block 5

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 6

	mv          s4, a0
	ld          a1, 0(s3)
	addi        a2, s0, -32
	addi        a3, s0, -24
	mv          a0, s1
	call        GetAddressAndOffset

	// *** Basic block 7

	ld          a2, -32(s0)
	ld          a3, -24(s0)
	mv          a1, s4
	li          t0, 57		// 0x39 ASCII '9'
	mv          a0, t0
	call        NewInstruction3

	// *** Basic block 8

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 9

	mv          s3, a0
	mv          a1, s3
	mv          a0, s2
	call        SetLoweredNode

	// *** Basic block 10


	// *** Basic block 11

.LowerBuiltinVaStart_label_116:
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
.func_end_LowerBuiltinVaStart:
	.size LowerBuiltinVaStart, .func_end_LowerBuiltinVaStart-LowerBuiltinVaStart

	.local  LowerBuiltinVaArg
	.type LowerBuiltinVaArg, @function

LowerBuiltinVaArg:

	// *** Basic block 0

	.local GetAddressAndOffset
	.local Emit
	.local NewInstruction2
	.global TypeIsUnsigned
	.local GetIntConstant
	.local GetLoweredNode
	.local NewInstruction3
	.local SetLoweredNode
	addi sp, sp, -112
	// Saved return address (offset 104) and frame pointer (offset 96)
	sd ra, 104(sp)
	sd s0, 96(sp)
	addi s0, sp, 112
	// Local vars at offset -32(s0)
	// Saved integer registers.
	sd s1, 72(sp)
	sd s2, 64(sp)
	sd s3, 56(sp)
	sd s4, 48(sp)
	sd s5, 40(sp)
	sd s6, 32(sp)
	sd s7, 24(sp)
	sd s8, 16(sp)
	sd s9, 8(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	ld          s3, 24(s2)
	ld          a1, 0(s3)
	addi        a2, s0, -32
	addi        a3, s0, -24
	call        GetAddressAndOffset

	// *** Basic block 1

	ld          s4, -32(s0)
	ld          s5, -24(s0)
	mv          a2, s5
	mv          a1, s4
	li          t0, 52		// 0x34 ASCII '4'
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 2

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 3

	mv          s6, a0
	li          s7, 52		// 0x34 ASCII '4'
	ld          s8, 80(s2)
	lw          s9, 20(s8)
	li          t0, 1		// 0x1 ASCII \x1
	blt         s9, t0, .LowerBuiltinVaArg_label_131

	// *** Basic block 4

	li          t0, 4		// 0x4 ASCII \x4
	blt         t0, s9, .LowerBuiltinVaArg_label_131

	// *** Basic block 5

	addi        t0, s9, -1
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 6

	j           .LowerBuiltinVaArg_label_92

	// *** Basic block 7

	j           .LowerBuiltinVaArg_label_105

	// *** Basic block 8

	j           .LowerBuiltinVaArg_label_131

	// *** Basic block 9

	j           .LowerBuiltinVaArg_label_118

	// *** Basic block 10

.LowerBuiltinVaArg_label_92:
	mv          a0, s8
	call        TypeIsUnsigned

	// *** Basic block 11

	beqz        a0, .LowerBuiltinVaArg_label_101

	// *** Basic block 12

	li          s7, 50		// 0x32 ASCII '2'
	j           .LowerBuiltinVaArg_label_103

	// *** Basic block 13

.LowerBuiltinVaArg_label_101:
	li          s7, 48		// 0x30 ASCII '0'

	// *** Basic block 14

.LowerBuiltinVaArg_label_103:
	j           .LowerBuiltinVaArg_label_131

	// *** Basic block 15

.LowerBuiltinVaArg_label_105:
	mv          a0, s8
	call        TypeIsUnsigned

	// *** Basic block 16

	beqz        a0, .LowerBuiltinVaArg_label_114

	// *** Basic block 17

	li          s7, 51		// 0x33 ASCII '3'
	j           .LowerBuiltinVaArg_label_116

	// *** Basic block 18

.LowerBuiltinVaArg_label_114:
	li          s7, 47		// 0x2f ASCII '/'

	// *** Basic block 19

.LowerBuiltinVaArg_label_116:
	j           .LowerBuiltinVaArg_label_131

	// *** Basic block 20

.LowerBuiltinVaArg_label_118:
	mv          a0, s8
	call        TypeIsUnsigned

	// *** Basic block 21

	beqz        a0, .LowerBuiltinVaArg_label_127

	// *** Basic block 22

	li          s7, 49		// 0x31 ASCII '1'
	j           .LowerBuiltinVaArg_label_129

	// *** Basic block 23

.LowerBuiltinVaArg_label_127:
	li          s7, 46		// 0x2e ASCII '.'

	// *** Basic block 24

.LowerBuiltinVaArg_label_129:
	j           .LowerBuiltinVaArg_label_131

	// *** Basic block 25

.LowerBuiltinVaArg_label_131:
	mv          a3, x0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	mv          a1, x0
	mv          a0, s1
	call        GetIntConstant

	// *** Basic block 26

	mv          a2, a0
	mv          a1, s6
	mv          a0, s7
	call        NewInstruction2

	// *** Basic block 27

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 28

	mv          s7, a0
	ld          a0, 8(s3)
	call        GetLoweredNode

	// *** Basic block 29

	mv          s3, a0
	mv          a2, s3
	mv          a1, s6
	li          t0, 64		// 0x40 ASCII '@'
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 30

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 31

	mv          s8, a0
	mv          a3, s5
	mv          a2, s4
	mv          a1, s8
	li          t0, 57		// 0x39 ASCII '9'
	mv          a0, t0
	call        NewInstruction3

	// *** Basic block 32

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 33

	mv          a1, s7
	mv          a0, s2
	call        SetLoweredNode

	// *** Basic block 34


	// *** Basic block 35

.LowerBuiltinVaArg_label_199:
	// Restored registers.
	ld s1, 72(sp)
	ld s2, 64(sp)
	ld s3, 56(sp)
	ld s4, 48(sp)
	ld s5, 40(sp)
	ld s6, 32(sp)
	ld s7, 24(sp)
	ld s8, 16(sp)
	ld s9, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_LowerBuiltinVaArg:
	.size LowerBuiltinVaArg, .func_end_LowerBuiltinVaArg-LowerBuiltinVaArg

	.local  LowerBuiltinVaEnd
	.type LowerBuiltinVaEnd, @function

LowerBuiltinVaEnd:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          a0, x0

	// *** Basic block 1

.LowerBuiltinVaEnd_label_6:
	ret         
.func_end_LowerBuiltinVaEnd:
	.size LowerBuiltinVaEnd, .func_end_LowerBuiltinVaEnd-LowerBuiltinVaEnd

	.local  LowerBuiltinVaCopy
	.type LowerBuiltinVaCopy, @function

LowerBuiltinVaCopy:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          a0, x0

	// *** Basic block 1

.LowerBuiltinVaCopy_label_6:
	ret         
.func_end_LowerBuiltinVaCopy:
	.size LowerBuiltinVaCopy, .func_end_LowerBuiltinVaCopy-LowerBuiltinVaCopy

	.local  LowerIRNode
	.type LowerIRNode, @function

LowerIRNode:

	// *** Basic block 0

	.local Emit
	.local NewInstruction
	.local SetLoweredNode
	.local LowerLiteralReference
	.local LowerAddressOf
	.local GetIntConstant
	.local GetFloatingPointConstant
	.local NewInstruction1
	.local ArgumentPointer
	.local NewInstruction2
	.local StackPointer
	.local FramePointer
	.local LowerLoad
	.local LowerStore
	.local LowerExpression
	.local LowerConditionalBranch
	.local LowerBranch
	.local LowerComputedBranch
	.local LowerLabel
	.local LowerNamedLabel
	.local LowerCall
	.local Materialize
	.local LowerResult
	.local LowerMemzero
	.local LowerMemcpy
	.local LowerZeroExtend
	.local LowerSignExtend
	.local LowerAlign
	.local LowerAsm
	.local LowerLocation
	.local LowerBuiltinVaStart
	.local LowerBuiltinVaArg
	.local LowerBuiltinVaEnd
	.local LowerBuiltinVaCopy
	.global printf
	.global abort
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
	ld          a0, 96(s1)
	beq         a0, x0, .LowerIRNode_label_77

	// *** Basic block 1

.LowerIRNode_label_74:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 2

.LowerIRNode_label_77:
	lw          s3, 20(s1)
	blt         s3, x0, .LowerIRNode_label_899

	// *** Basic block 3

	li          t0, 132		// 0x84 ASCII \x84
	blt         t0, s3, .LowerIRNode_label_899

	// *** Basic block 4

	slli        t0, s3, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 5

	j           .LowerIRNode_label_232

	// *** Basic block 6

	j           .LowerIRNode_label_671

	// *** Basic block 7

	j           .LowerIRNode_label_298

	// *** Basic block 8

	j           .LowerIRNode_label_313

	// *** Basic block 9

	j           .LowerIRNode_label_280

	// *** Basic block 10

	j           .LowerIRNode_label_330

	// *** Basic block 11

	j           .LowerIRNode_label_346

	// *** Basic block 12

	j           .LowerIRNode_label_363

	// *** Basic block 13

	j           .LowerIRNode_label_329

	// *** Basic block 14

	j           .LowerIRNode_label_663

	// *** Basic block 15

	j           .LowerIRNode_label_664

	// *** Basic block 16

	j           .LowerIRNode_label_665

	// *** Basic block 17

	j           .LowerIRNode_label_666

	// *** Basic block 18

	j           .LowerIRNode_label_667

	// *** Basic block 19

	j           .LowerIRNode_label_668

	// *** Basic block 20

	j           .LowerIRNode_label_669

	// *** Basic block 21

	j           .LowerIRNode_label_670

	// *** Basic block 22

	j           .LowerIRNode_label_712

	// *** Basic block 23

	j           .LowerIRNode_label_722

	// *** Basic block 24

	j           .LowerIRNode_label_571

	// *** Basic block 25

	j           .LowerIRNode_label_572

	// *** Basic block 26

	j           .LowerIRNode_label_573

	// *** Basic block 27

	j           .LowerIRNode_label_574

	// *** Basic block 28

	j           .LowerIRNode_label_575

	// *** Basic block 29

	j           .LowerIRNode_label_576

	// *** Basic block 30

	j           .LowerIRNode_label_577

	// *** Basic block 31

	j           .LowerIRNode_label_578

	// *** Basic block 32

	j           .LowerIRNode_label_579

	// *** Basic block 33

	j           .LowerIRNode_label_580

	// *** Basic block 34

	j           .LowerIRNode_label_742

	// *** Basic block 35

	j           .LowerIRNode_label_590

	// *** Basic block 36

	j           .LowerIRNode_label_591

	// *** Basic block 37

	j           .LowerIRNode_label_592

	// *** Basic block 38

	j           .LowerIRNode_label_593

	// *** Basic block 39

	j           .LowerIRNode_label_594

	// *** Basic block 40

	j           .LowerIRNode_label_595

	// *** Basic block 41

	j           .LowerIRNode_label_596

	// *** Basic block 42

	j           .LowerIRNode_label_606

	// *** Basic block 43

	j           .LowerIRNode_label_607

	// *** Basic block 44

	j           .LowerIRNode_label_608

	// *** Basic block 45

	j           .LowerIRNode_label_609

	// *** Basic block 46

	j           .LowerIRNode_label_610

	// *** Basic block 47

	j           .LowerIRNode_label_611

	// *** Basic block 48

	j           .LowerIRNode_label_612

	// *** Basic block 49

	j           .LowerIRNode_label_613

	// *** Basic block 50

	j           .LowerIRNode_label_614

	// *** Basic block 51

	j           .LowerIRNode_label_615

	// *** Basic block 52

	j           .LowerIRNode_label_616

	// *** Basic block 53

	j           .LowerIRNode_label_617

	// *** Basic block 54

	j           .LowerIRNode_label_618

	// *** Basic block 55

	j           .LowerIRNode_label_619

	// *** Basic block 56

	j           .LowerIRNode_label_620

	// *** Basic block 57

	j           .LowerIRNode_label_621

	// *** Basic block 58

	j           .LowerIRNode_label_622

	// *** Basic block 59

	j           .LowerIRNode_label_623

	// *** Basic block 60

	j           .LowerIRNode_label_624

	// *** Basic block 61

	j           .LowerIRNode_label_625

	// *** Basic block 62

	j           .LowerIRNode_label_626

	// *** Basic block 63

	j           .LowerIRNode_label_627

	// *** Basic block 64

	j           .LowerIRNode_label_628

	// *** Basic block 65

	j           .LowerIRNode_label_629

	// *** Basic block 66

	j           .LowerIRNode_label_630

	// *** Basic block 67

	j           .LowerIRNode_label_631

	// *** Basic block 68

	j           .LowerIRNode_label_632

	// *** Basic block 69

	j           .LowerIRNode_label_633

	// *** Basic block 70

	j           .LowerIRNode_label_634

	// *** Basic block 71

	j           .LowerIRNode_label_635

	// *** Basic block 72

	j           .LowerIRNode_label_636

	// *** Basic block 73

	j           .LowerIRNode_label_637

	// *** Basic block 74

	j           .LowerIRNode_label_638

	// *** Basic block 75

	j           .LowerIRNode_label_639

	// *** Basic block 76

	j           .LowerIRNode_label_640

	// *** Basic block 77

	j           .LowerIRNode_label_641

	// *** Basic block 78

	j           .LowerIRNode_label_642

	// *** Basic block 79

	j           .LowerIRNode_label_643

	// *** Basic block 80

	j           .LowerIRNode_label_644

	// *** Basic block 81

	j           .LowerIRNode_label_645

	// *** Basic block 82

	j           .LowerIRNode_label_646

	// *** Basic block 83

	j           .LowerIRNode_label_647

	// *** Basic block 84

	j           .LowerIRNode_label_648

	// *** Basic block 85

	j           .LowerIRNode_label_649

	// *** Basic block 86

	j           .LowerIRNode_label_650

	// *** Basic block 87

	j           .LowerIRNode_label_651

	// *** Basic block 88

	j           .LowerIRNode_label_652

	// *** Basic block 89

	j           .LowerIRNode_label_653

	// *** Basic block 90

	j           .LowerIRNode_label_654

	// *** Basic block 91

	j           .LowerIRNode_label_655

	// *** Basic block 92

	j           .LowerIRNode_label_656

	// *** Basic block 93

	j           .LowerIRNode_label_681

	// *** Basic block 94

	j           .LowerIRNode_label_682

	// *** Basic block 95

	j           .LowerIRNode_label_692

	// *** Basic block 96

	j           .LowerIRNode_label_702

	// *** Basic block 97

	j           .LowerIRNode_label_732

	// *** Basic block 98

	j           .LowerIRNode_label_557

	// *** Basic block 99

	j           .LowerIRNode_label_379

	// *** Basic block 100

	j           .LowerIRNode_label_489

	// *** Basic block 101

	j           .LowerIRNode_label_224

	// *** Basic block 102

	j           .LowerIRNode_label_228

	// *** Basic block 103

	j           .LowerIRNode_label_225

	// *** Basic block 104

	j           .LowerIRNode_label_227

	// *** Basic block 105

	j           .LowerIRNode_label_226

	// *** Basic block 106

	j           .LowerIRNode_label_237

	// *** Basic block 107

	j           .LowerIRNode_label_240

	// *** Basic block 108

	j           .LowerIRNode_label_270

	// *** Basic block 109

	j           .LowerIRNode_label_260

	// *** Basic block 110

	j           .LowerIRNode_label_849

	// *** Basic block 111

	j           .LowerIRNode_label_759

	// *** Basic block 112

	j           .LowerIRNode_label_760

	// *** Basic block 113

	j           .LowerIRNode_label_761

	// *** Basic block 114

	j           .LowerIRNode_label_762

	// *** Basic block 115

	j           .LowerIRNode_label_657

	// *** Basic block 116

	j           .LowerIRNode_label_658

	// *** Basic block 117

	j           .LowerIRNode_label_659

	// *** Basic block 118

	j           .LowerIRNode_label_660

	// *** Basic block 119

	j           .LowerIRNode_label_661

	// *** Basic block 120

	j           .LowerIRNode_label_662

	// *** Basic block 121

	j           .LowerIRNode_label_809

	// *** Basic block 122

	j           .LowerIRNode_label_819

	// *** Basic block 123

	j           .LowerIRNode_label_829

	// *** Basic block 124

	j           .LowerIRNode_label_772

	// *** Basic block 125

	j           .LowerIRNode_label_782

	// *** Basic block 126

	j           .LowerIRNode_label_792

	// *** Basic block 127

	j           .LowerIRNode_label_238

	// *** Basic block 128

	j           .LowerIRNode_label_839

	// *** Basic block 129

	j           .LowerIRNode_label_899

	// *** Basic block 130

	j           .LowerIRNode_label_899

	// *** Basic block 131

	j           .LowerIRNode_label_899

	// *** Basic block 132

	j           .LowerIRNode_label_899

	// *** Basic block 133

	j           .LowerIRNode_label_859

	// *** Basic block 134

	j           .LowerIRNode_label_869

	// *** Basic block 135

	j           .LowerIRNode_label_879

	// *** Basic block 136

	j           .LowerIRNode_label_889

	// *** Basic block 137

	j           .LowerIRNode_label_233

	// *** Basic block 138

.LowerIRNode_label_224:

	// *** Basic block 139

.LowerIRNode_label_225:

	// *** Basic block 140

.LowerIRNode_label_226:

	// *** Basic block 141

.LowerIRNode_label_227:

	// *** Basic block 142

.LowerIRNode_label_228:
	mv          a0, x0
	j           .LowerIRNode_label_74

	// *** Basic block 143

.LowerIRNode_label_232:

	// *** Basic block 144

.LowerIRNode_label_233:
	mv          a0, x0
	j           .LowerIRNode_label_74

	// *** Basic block 145

.LowerIRNode_label_237:

	// *** Basic block 146

.LowerIRNode_label_238:
	j           .LowerIRNode_label_899

	// *** Basic block 147

.LowerIRNode_label_240:
	li          t0, 29		// 0x1d ASCII \x1d
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 148

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 149

	mv          s4, a0
	mv          a1, s4
	mv          a0, s1
	call        SetLoweredNode

	// *** Basic block 150

	mv          a0, s4
	j           .LowerIRNode_label_74

	// *** Basic block 151

.LowerIRNode_label_260:
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
	j           LowerLiteralReference

	// *** Basic block 153

.LowerIRNode_label_270:
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
	j           LowerAddressOf

	// *** Basic block 155

.LowerIRNode_label_280:
	ld          a3, 136(s1)
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
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
	j           GetIntConstant

	// *** Basic block 157

.LowerIRNode_label_298:
	ld          a3, 136(s1)
	mv          a2, x0
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
	j           GetIntConstant

	// *** Basic block 159

.LowerIRNode_label_313:
	ld          a3, 136(s1)
	li          t0, 1		// 0x1 ASCII \x1
	mv          a2, t0
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
	j           GetIntConstant

	// *** Basic block 161

.LowerIRNode_label_329:

	// *** Basic block 162

.LowerIRNode_label_330:
	ld          a3, 136(s1)
	li          t0, 3		// 0x3 ASCII \x3
	mv          a2, t0
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
	j           GetIntConstant

	// *** Basic block 164

.LowerIRNode_label_346:
	fld         fa0, 136(s1)
	li          t0, 4		// 0x4 ASCII \x4
	mv          a2, t0
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
	j           GetFloatingPointConstant

	// *** Basic block 166

.LowerIRNode_label_363:
	fld         fa0, 136(s1)
	li          t0, 5		// 0x5 ASCII \x5
	mv          a2, t0
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
	j           GetFloatingPointConstant

	// *** Basic block 168

.LowerIRNode_label_379:
	mv          a0, s2
	call        ArgumentPointer

	// *** Basic block 169

	mv          a1, a0
	li          s3, 41		// 0x29 ASCII ')'
	mv          a0, s3
	call        NewInstruction1

	// *** Basic block 170

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 171

	mv          a0, s2
	call        ArgumentPointer

	// *** Basic block 172

	mv          a0, s2
	call        StackPointer

	// *** Basic block 173

	mv          a2, a0
	mv          a1, a0
	li          s4, 18		// 0x12 ASCII \x12
	mv          a0, s4
	call        NewInstruction2

	// *** Basic block 174

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 175

	mv          a0, x0
	call        NewInstruction

	// *** Basic block 176

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 177

	mv          a0, s2
	call        FramePointer

	// *** Basic block 178

	mv          a1, a0
	mv          a0, s3
	call        NewInstruction1

	// *** Basic block 179

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 180

	mv          a0, s2
	call        FramePointer

	// *** Basic block 181

	mv          a0, s2
	call        StackPointer

	// *** Basic block 182

	mv          a2, a0
	mv          a1, a0
	mv          a0, s4
	call        NewInstruction2

	// *** Basic block 183

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 184

	mv          s3, a0
	lw          s4, 128(s2)
	bge         x0, s4, .LowerIRNode_label_485

	// *** Basic block 185

	mv          a3, s4
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	mv          a1, x0
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 186

	mv          a1, a0
	li          t0, 36		// 0x24 ASCII '$'
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 187

	mv          a1, a0
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           Emit

	// *** Basic block 189

.LowerIRNode_label_485:
	mv          a0, s3
	j           .LowerIRNode_label_74

	// *** Basic block 190

.LowerIRNode_label_489:
	lw          s4, 128(s2)
	bge         x0, s4, .LowerIRNode_label_515

	// *** Basic block 191

	mv          a3, s4
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	mv          a1, x0
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 192

	mv          a1, a0
	li          t0, 37		// 0x25 ASCII '%'
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 193

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 194

.LowerIRNode_label_515:
	mv          a0, s2
	call        FramePointer

	// *** Basic block 195

	mv          a1, a0
	li          s4, 45		// 0x2d ASCII '-'
	mv          a0, s4
	call        NewInstruction1

	// *** Basic block 196

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 197

	li          t0, 1		// 0x1 ASCII \x1
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 198

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 199

	mv          a0, s2
	call        ArgumentPointer

	// *** Basic block 200

	mv          a1, a0
	mv          a0, s4
	call        NewInstruction1

	// *** Basic block 201

	mv          a1, a0
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           Emit

	// *** Basic block 203

.LowerIRNode_label_557:
	li          t0, 21		// 0x15 ASCII \x15
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 204

	mv          a1, a0
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           Emit

	// *** Basic block 206

.LowerIRNode_label_571:

	// *** Basic block 207

.LowerIRNode_label_572:

	// *** Basic block 208

.LowerIRNode_label_573:

	// *** Basic block 209

.LowerIRNode_label_574:

	// *** Basic block 210

.LowerIRNode_label_575:

	// *** Basic block 211

.LowerIRNode_label_576:

	// *** Basic block 212

.LowerIRNode_label_577:

	// *** Basic block 213

.LowerIRNode_label_578:

	// *** Basic block 214

.LowerIRNode_label_579:

	// *** Basic block 215

.LowerIRNode_label_580:
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
	j           LowerLoad

	// *** Basic block 217

.LowerIRNode_label_590:

	// *** Basic block 218

.LowerIRNode_label_591:

	// *** Basic block 219

.LowerIRNode_label_592:

	// *** Basic block 220

.LowerIRNode_label_593:

	// *** Basic block 221

.LowerIRNode_label_594:

	// *** Basic block 222

.LowerIRNode_label_595:

	// *** Basic block 223

.LowerIRNode_label_596:
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
	j           LowerStore

	// *** Basic block 225

.LowerIRNode_label_606:

	// *** Basic block 226

.LowerIRNode_label_607:

	// *** Basic block 227

.LowerIRNode_label_608:

	// *** Basic block 228

.LowerIRNode_label_609:

	// *** Basic block 229

.LowerIRNode_label_610:

	// *** Basic block 230

.LowerIRNode_label_611:

	// *** Basic block 231

.LowerIRNode_label_612:

	// *** Basic block 232

.LowerIRNode_label_613:

	// *** Basic block 233

.LowerIRNode_label_614:

	// *** Basic block 234

.LowerIRNode_label_615:

	// *** Basic block 235

.LowerIRNode_label_616:

	// *** Basic block 236

.LowerIRNode_label_617:

	// *** Basic block 237

.LowerIRNode_label_618:

	// *** Basic block 238

.LowerIRNode_label_619:

	// *** Basic block 239

.LowerIRNode_label_620:

	// *** Basic block 240

.LowerIRNode_label_621:

	// *** Basic block 241

.LowerIRNode_label_622:

	// *** Basic block 242

.LowerIRNode_label_623:

	// *** Basic block 243

.LowerIRNode_label_624:

	// *** Basic block 244

.LowerIRNode_label_625:

	// *** Basic block 245

.LowerIRNode_label_626:

	// *** Basic block 246

.LowerIRNode_label_627:

	// *** Basic block 247

.LowerIRNode_label_628:

	// *** Basic block 248

.LowerIRNode_label_629:

	// *** Basic block 249

.LowerIRNode_label_630:

	// *** Basic block 250

.LowerIRNode_label_631:

	// *** Basic block 251

.LowerIRNode_label_632:

	// *** Basic block 252

.LowerIRNode_label_633:

	// *** Basic block 253

.LowerIRNode_label_634:

	// *** Basic block 254

.LowerIRNode_label_635:

	// *** Basic block 255

.LowerIRNode_label_636:

	// *** Basic block 256

.LowerIRNode_label_637:

	// *** Basic block 257

.LowerIRNode_label_638:

	// *** Basic block 258

.LowerIRNode_label_639:

	// *** Basic block 259

.LowerIRNode_label_640:

	// *** Basic block 260

.LowerIRNode_label_641:

	// *** Basic block 261

.LowerIRNode_label_642:

	// *** Basic block 262

.LowerIRNode_label_643:

	// *** Basic block 263

.LowerIRNode_label_644:

	// *** Basic block 264

.LowerIRNode_label_645:

	// *** Basic block 265

.LowerIRNode_label_646:

	// *** Basic block 266

.LowerIRNode_label_647:

	// *** Basic block 267

.LowerIRNode_label_648:

	// *** Basic block 268

.LowerIRNode_label_649:

	// *** Basic block 269

.LowerIRNode_label_650:

	// *** Basic block 270

.LowerIRNode_label_651:

	// *** Basic block 271

.LowerIRNode_label_652:

	// *** Basic block 272

.LowerIRNode_label_653:

	// *** Basic block 273

.LowerIRNode_label_654:

	// *** Basic block 274

.LowerIRNode_label_655:

	// *** Basic block 275

.LowerIRNode_label_656:

	// *** Basic block 276

.LowerIRNode_label_657:

	// *** Basic block 277

.LowerIRNode_label_658:

	// *** Basic block 278

.LowerIRNode_label_659:

	// *** Basic block 279

.LowerIRNode_label_660:

	// *** Basic block 280

.LowerIRNode_label_661:

	// *** Basic block 281

.LowerIRNode_label_662:

	// *** Basic block 282

.LowerIRNode_label_663:

	// *** Basic block 283

.LowerIRNode_label_664:

	// *** Basic block 284

.LowerIRNode_label_665:

	// *** Basic block 285

.LowerIRNode_label_666:

	// *** Basic block 286

.LowerIRNode_label_667:

	// *** Basic block 287

.LowerIRNode_label_668:

	// *** Basic block 288

.LowerIRNode_label_669:

	// *** Basic block 289

.LowerIRNode_label_670:

	// *** Basic block 290

.LowerIRNode_label_671:
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
	j           LowerExpression

	// *** Basic block 292

.LowerIRNode_label_681:

	// *** Basic block 293

.LowerIRNode_label_682:
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
	j           LowerConditionalBranch

	// *** Basic block 295

.LowerIRNode_label_692:
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
	j           LowerBranch

	// *** Basic block 297

.LowerIRNode_label_702:
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
	j           LowerComputedBranch

	// *** Basic block 299

.LowerIRNode_label_712:
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
	j           LowerLabel

	// *** Basic block 301

.LowerIRNode_label_722:
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
	j           LowerNamedLabel

	// *** Basic block 303

.LowerIRNode_label_732:
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
	j           LowerCall

	// *** Basic block 305

.LowerIRNode_label_742:
	ld          t0, 24(s1)
	ld          a1, 0(t0)
	mv          a0, s2
	call        Materialize

	// *** Basic block 306

	mv          a1, a0
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           SetLoweredNode

	// *** Basic block 308

.LowerIRNode_label_759:

	// *** Basic block 309

.LowerIRNode_label_760:

	// *** Basic block 310

.LowerIRNode_label_761:

	// *** Basic block 311

.LowerIRNode_label_762:
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
	j           LowerResult

	// *** Basic block 313

.LowerIRNode_label_772:
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
	j           LowerMemzero

	// *** Basic block 315

.LowerIRNode_label_782:
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
	j           LowerMemcpy

	// *** Basic block 317

.LowerIRNode_label_792:
	ld          t0, 24(s1)
	ld          a1, 0(t0)
	mv          a0, s2
	call        Materialize

	// *** Basic block 318

	mv          a1, a0
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           SetLoweredNode

	// *** Basic block 320

.LowerIRNode_label_809:
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
	j           LowerZeroExtend

	// *** Basic block 322

.LowerIRNode_label_819:
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
	j           LowerSignExtend

	// *** Basic block 324

.LowerIRNode_label_829:
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
	j           LowerAlign

	// *** Basic block 326

.LowerIRNode_label_839:
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
	j           LowerAsm

	// *** Basic block 328

.LowerIRNode_label_849:
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
	j           LowerLocation

	// *** Basic block 330

.LowerIRNode_label_859:
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
	j           LowerBuiltinVaStart

	// *** Basic block 332

.LowerIRNode_label_869:
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
	j           LowerBuiltinVaArg

	// *** Basic block 334

.LowerIRNode_label_879:
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
	j           LowerBuiltinVaEnd

	// *** Basic block 336

.LowerIRNode_label_889:
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
	j           LowerBuiltinVaCopy

	// *** Basic block 338

.LowerIRNode_label_899:
	lla         a0, .str.174
	lla         a1, .str.175
	lla         a3, .str.176
	li          t0, 1774		// 0x6ee
	mv          a2, t0
	call        printf

	// *** Basic block 339

	call        abort

	// *** Basic block 340

	mv          a0, x0
	j           .LowerIRNode_label_74
.func_end_LowerIRNode:
	.size LowerIRNode, .func_end_LowerIRNode-LowerIRNode

	.local  CalculateArgumentSize
	.type CalculateArgumentSize, @function

CalculateArgumentSize:

	// *** Basic block 0

	.global TypeIsFloatingPoint
	.global TypeIsDouble
	.global TypeIsStructOrUnion
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
	mv          t0, a0
	ld          s1, 40(t0)
	mv          a0, s1
	call        TypeIsFloatingPoint

	// *** Basic block 1

	beqz        a0, .CalculateArgumentSize_label_39

	// *** Basic block 2

	mv          a0, s1
	call        TypeIsDouble

	// *** Basic block 3

	beqz        a0, .CalculateArgumentSize_label_35

	// *** Basic block 4

	li          a0, 8		// 0x8 ASCII \x8

	// *** Basic block 5

.CalculateArgumentSize_label_32:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 6

.CalculateArgumentSize_label_35:
	li          a0, 4		// 0x4 ASCII \x4
	j           .CalculateArgumentSize_label_32

	// *** Basic block 7

.CalculateArgumentSize_label_39:
	lw          s3, 16(s1)
	addi        t0, s3, -1
	seqz        s2, t0
	li          t0, 1		// 0x1 ASCII \x1
	beq         s3, t0, .CalculateArgumentSize_label_52

	// *** Basic block 8

	addi        t0, s3, -2
	seqz        s2, t0

	// *** Basic block 9

.CalculateArgumentSize_label_52:

	// *** Basic block 10

.CalculateArgumentSize_label_54:
	beqz        s2, .CalculateArgumentSize_label_61

	// *** Basic block 11

	j           .CalculateArgumentSize_label_57

	// *** Basic block 12

.CalculateArgumentSize_label_57:
	li          a0, 8		// 0x8 ASCII \x8
	j           .CalculateArgumentSize_label_32

	// *** Basic block 13

.CalculateArgumentSize_label_61:
	mv          a0, s1
	call        TypeIsStructOrUnion

	// *** Basic block 14

	beqz        a0, .CalculateArgumentSize_label_72

	// *** Basic block 15

	ld          t0, 32(s1)
	lw          a0, 76(t0)
	j           .CalculateArgumentSize_label_32

	// *** Basic block 16

.CalculateArgumentSize_label_72:
	lw          a0, 20(s1)
	li          t0, 4		// 0x4 ASCII \x4
	bge         a0, t0, .CalculateArgumentSize_label_81

	// *** Basic block 17

	li          a0, 4		// 0x4 ASCII \x4
	j           .CalculateArgumentSize_label_82

	// *** Basic block 18

.CalculateArgumentSize_label_81:

	// *** Basic block 19

.CalculateArgumentSize_label_82:
	j           .CalculateArgumentSize_label_32
.func_end_CalculateArgumentSize:
	.size CalculateArgumentSize, .func_end_CalculateArgumentSize-CalculateArgumentSize

	.global PCodeLower
	.type PCodeLower, @function

PCodeLower:

	// *** Basic block 0

	.global compiler
	.local CalculateArgumentSize
	.local GetSymbol
	.global GeneratorFirstInstruction
	.local LowerIRNode
	.global IRNext
	.global PCodeOptimize
	.global PCodePrint
	.global PCodeAllocateRegisters
	addi sp, sp, -112
	// Saved return address (offset 104) and frame pointer (offset 96)
	sd ra, 104(sp)
	sd s0, 96(sp)
	addi s0, sp, 112
	// Local vars at offset -16(s0)
	// Saved integer registers.
	sd s1, 88(sp)
	sd s2, 80(sp)
	sd s3, 72(sp)
	sd s4, 64(sp)
	sd s5, 56(sp)
	sd s6, 48(sp)
	sd s7, 40(sp)
	sd s8, 32(sp)
	sd s9, 24(sp)
	sd s10, 16(sp)
	sd s11, 8(sp)
	// End of stack frame
	mv          s1, a1
	mv          s2, a0
	mv          s3, x0
	li          s4, 16		// 0x10 ASCII \x10
	la          t0, compiler
	ld          s5, 0(t0)
	ld          t0, 1096(s5)
	addi        t0, t0, 32
	addi        s6, t0, 8
	mv          s7, x0
	ld          s8, 8(s6)
	bge         x0, s8, .PCodeLower_label_76

	// *** Basic block 1

	ld          s6, 0(s6)

	// *** Basic block 2

.PCodeLower_label_57:
	slli        t0, s7, 3
	add         t0, s6, t0
	ld          s6, 0(t0)
	sw          s4, 120(s6)
	mv          a0, s6
	call        CalculateArgumentSize

	// *** Basic block 4

.PCodeLower_label_72:
	addi        s7, s7, 1
	bge         s7, s8, .PCodeLower_label_57

	// *** Basic block 5

.PCodeLower_label_76:
	mv          s4, x0
	addi        t0, s1, 144
	ld          s6, 8(t0)
	bge         x0, s6, .PCodeLower_label_175

	// *** Basic block 6

	ld          t0, 144(s1)

	// *** Basic block 7

.PCodeLower_label_85:
	slli        t1, s4, 3
	add         t0, t0, t1
	ld          s8, 0(t0)
	ld          s9, 16(s8)
	lw          s10, 20(s9)
	addi        t1, s10, -96
	seqz        t0, t1
	li          t1, 96		// 0x60 ASCII '`'
	beq         s10, t1, .PCodeLower_label_103

	// *** Basic block 8

	addi        t1, s10, -100
	seqz        t0, t1

	// *** Basic block 9

.PCodeLower_label_103:
	beqz        t0, .PCodeLower_label_124

	// *** Basic block 10

	ld          t0, 8(s8)
	ld          t0, 40(t0)
	lw          s11, 20(t0)
	addi        t0, s11, -1
	add         t1, s3, t0
	not         t0, t0
	and         s3, t1, t0
	ld          t0, 16(s8)
	addi        t0, t0, 96
	sw          s3, 8(t0)
	add         s3, s3, s11
	j           .PCodeLower_label_170

	// *** Basic block 11

.PCodeLower_label_124:
	li          t0, 98		// 0x62 ASCII 'b'
	bne         s10, t0, .PCodeLower_label_140

	// *** Basic block 12

	ld          t0, 16(s8)
	addi        t0, t0, 96
	ld          t1, 8(s8)
	lw          t1, 120(t1)
	sw          t1, 8(t0)
	j           .PCodeLower_label_169

	// *** Basic block 13

.PCodeLower_label_140:
	addi        t1, s10, -99
	seqz        t0, t1
	li          t1, 99		// 0x63 ASCII 'c'
	beq         s10, t1, .PCodeLower_label_150

	// *** Basic block 14

	addi        t1, s10, -97
	seqz        t0, t1

	// *** Basic block 15

.PCodeLower_label_150:
	beqz        t0, .PCodeLower_label_168

	// *** Basic block 16

	ld          a2, 136(s9)
	mv          a1, x0
	mv          a0, s2
	call        GetSymbol

	// *** Basic block 17

	mv          s9, a0
	ld          t0, 16(s8)
	sd          s9, 96(t0)

	// *** Basic block 18

.PCodeLower_label_168:

	// *** Basic block 19

.PCodeLower_label_169:

	// *** Basic block 20

.PCodeLower_label_170:

	// *** Basic block 21

.PCodeLower_label_171:
	addi        s4, s4, 1
	bge         s4, s6, .PCodeLower_label_85

	// *** Basic block 22

.PCodeLower_label_175:
	sw          s3, 128(s2)
	mv          a0, s1
	call        GeneratorFirstInstruction

	// *** Basic block 23

	mv          s6, a0
	beq         s6, x0, .PCodeLower_label_199

	// *** Basic block 24

.PCodeLower_label_186:
	mv          a1, s6
	mv          a0, s2
	call        LowerIRNode

	// *** Basic block 25

	mv          a0, s6
	call        IRNext

	// *** Basic block 26

	mv          s6, a0
	bne         s6, x0, .PCodeLower_label_186

	// *** Basic block 27

.PCodeLower_label_199:
	mv          a0, s2
	call        PCodeOptimize

	// *** Basic block 28

	lb          t0, 1232(s5)
	beqz        t0, .PCodeLower_label_209

	// *** Basic block 29

	mv          a0, s2
	call        PCodePrint

	// *** Basic block 30

.PCodeLower_label_209:
	addi        a0, s2, 192
	// Restored registers.
	ld s1, 88(sp)
	ld s2, 80(sp)
	ld s3, 72(sp)
	ld s4, 64(sp)
	ld s5, 56(sp)
	ld s6, 48(sp)
	ld s7, 40(sp)
	ld s8, 32(sp)
	ld s9, 24(sp)
	ld s10, 16(sp)
	ld s11, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           PCodeAllocateRegisters
.func_end_PCodeLower:
	.size PCodeLower, .func_end_PCodeLower-PCodeLower

	.global PCodePrint
	.type PCodePrint, @function

PCodePrint:

	// *** Basic block 0

	.global TargetFirstInstruction
	.global TargetPrintInstruction
	.global PCodeOpcodeName
	.global stdout
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
	call        TargetFirstInstruction

	// *** Basic block 1

	la          t0, stdout
	ld          s1, 0(t0)
	mv          s2, a0
	beq         s2, x0, .PCodePrint_label_40

	// *** Basic block 2

.PCodePrint_label_22:
	mv          a2, s1
	la          t0, PCodeOpcodeName
	mv          a1, t0
	mv          a0, s2
	call        TargetPrintInstruction

	// *** Basic block 3

	mv          a0, s2
	call        TargetNext

	// *** Basic block 4

	mv          s2, a0
	bne         s2, x0, .PCodePrint_label_22

	// *** Basic block 5

.PCodePrint_label_40:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_PCodePrint:
	.size PCodePrint, .func_end_PCodePrint-PCodePrint

	.global PCodeIsExpression
	.type PCodeIsExpression, @function

PCodeIsExpression:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	li          t1, 45		// 0x2d ASCII '-'
	blt         t0, t1, .PCodeIsExpression_label_147

	// *** Basic block 1

	li          t2, 112		// 0x70 ASCII 'p'
	blt         t0, t2, .PCodeIsExpression_label_100

	// *** Basic block 2

	beq         t0, t2, .PCodeIsExpression_label_268

	// *** Basic block 3

	li          t2, 113		// 0x71 ASCII 'q'
	beq         t0, t2, .PCodeIsExpression_label_269

	// *** Basic block 4

	li          t2, 124		// 0x7c ASCII '|'
	beq         t0, t2, .PCodeIsExpression_label_270

	// *** Basic block 5

	li          t2, 125		// 0x7d ASCII '}'
	beq         t0, t2, .PCodeIsExpression_label_271

	// *** Basic block 6

	li          t2, 129		// 0x81 ASCII \x81
	beq         t0, t2, .PCodeIsExpression_label_272

	// *** Basic block 7

	li          t2, 130		// 0x82 ASCII \x82
	beq         t0, t2, .PCodeIsExpression_label_273

	// *** Basic block 8

	li          t2, 131		// 0x83 ASCII \x83
	beq         t0, t2, .PCodeIsExpression_label_274

	// *** Basic block 9

	li          t2, 132		// 0x84 ASCII \x84
	beq         t0, t2, .PCodeIsExpression_label_275

	// *** Basic block 10

	li          t2, 133		// 0x85 ASCII \x85
	beq         t0, t2, .PCodeIsExpression_label_276

	// *** Basic block 11

	li          t2, 134		// 0x86 ASCII \x86
	beq         t0, t2, .PCodeIsExpression_label_277

	// *** Basic block 12

	j           .PCodeIsExpression_label_290

	// *** Basic block 13

.PCodeIsExpression_label_100:
	beq         t0, t1, .PCodeIsExpression_label_259

	// *** Basic block 14

	li          t1, 55		// 0x37 ASCII '7'
	beq         t0, t1, .PCodeIsExpression_label_260

	// *** Basic block 15

	li          t1, 56		// 0x38 ASCII '8'
	beq         t0, t1, .PCodeIsExpression_label_261

	// *** Basic block 16

	li          t1, 57		// 0x39 ASCII '9'
	beq         t0, t1, .PCodeIsExpression_label_262

	// *** Basic block 17

	li          t1, 58		// 0x3a ASCII ':'
	beq         t0, t1, .PCodeIsExpression_label_263

	// *** Basic block 18

	li          t1, 59		// 0x3b ASCII ';'
	beq         t0, t1, .PCodeIsExpression_label_264

	// *** Basic block 19

	li          t1, 60		// 0x3c ASCII '<'
	beq         t0, t1, .PCodeIsExpression_label_265

	// *** Basic block 20

	li          t1, 110		// 0x6e ASCII 'n'
	beq         t0, t1, .PCodeIsExpression_label_266

	// *** Basic block 21

	li          t1, 111		// 0x6f ASCII 'o'
	beq         t0, t1, .PCodeIsExpression_label_267

	// *** Basic block 22

	j           .PCodeIsExpression_label_290

	// *** Basic block 23

.PCodeIsExpression_label_147:
	li          t1, 32		// 0x20 ASCII ' '
	blt         t0, t1, .PCodeIsExpression_label_202

	// *** Basic block 24

	beq         t0, t1, .PCodeIsExpression_label_283

	// *** Basic block 25

	li          t1, 36		// 0x24 ASCII '$'
	beq         t0, t1, .PCodeIsExpression_label_250

	// *** Basic block 26

	li          t1, 37		// 0x25 ASCII '%'
	beq         t0, t1, .PCodeIsExpression_label_251

	// *** Basic block 27

	li          t1, 38		// 0x26 ASCII '&'
	beq         t0, t1, .PCodeIsExpression_label_252

	// *** Basic block 28

	li          t1, 39		// 0x27 ASCII '''
	beq         t0, t1, .PCodeIsExpression_label_253

	// *** Basic block 29

	li          t1, 40		// 0x28 ASCII '('
	beq         t0, t1, .PCodeIsExpression_label_254

	// *** Basic block 30

	li          t1, 41		// 0x29 ASCII ')'
	beq         t0, t1, .PCodeIsExpression_label_255

	// *** Basic block 31

	li          t1, 42		// 0x2a ASCII '*'
	beq         t0, t1, .PCodeIsExpression_label_256

	// *** Basic block 32

	li          t1, 43		// 0x2b ASCII '+'
	beq         t0, t1, .PCodeIsExpression_label_257

	// *** Basic block 33

	li          t1, 44		// 0x2c ASCII ','
	beq         t0, t1, .PCodeIsExpression_label_258

	// *** Basic block 34

	j           .PCodeIsExpression_label_290

	// *** Basic block 35

.PCodeIsExpression_label_202:
	beqz        t0, .PCodeIsExpression_label_246

	// *** Basic block 36

	li          t1, 1		// 0x1 ASCII \x1
	beq         t0, t1, .PCodeIsExpression_label_247

	// *** Basic block 37

	li          t1, 18		// 0x12 ASCII \x12
	beq         t0, t1, .PCodeIsExpression_label_279

	// *** Basic block 38

	li          t1, 19		// 0x13 ASCII \x13
	beq         t0, t1, .PCodeIsExpression_label_280

	// *** Basic block 39

	li          t1, 20		// 0x14 ASCII \x14
	beq         t0, t1, .PCodeIsExpression_label_281

	// *** Basic block 40

	li          t1, 21		// 0x15 ASCII \x15
	beq         t0, t1, .PCodeIsExpression_label_278

	// *** Basic block 41

	li          t1, 22		// 0x16 ASCII \x16
	beq         t0, t1, .PCodeIsExpression_label_248

	// *** Basic block 42

	li          t1, 30		// 0x1e ASCII \x1e
	beq         t0, t1, .PCodeIsExpression_label_249

	// *** Basic block 43

	li          t1, 31		// 0x1f ASCII \x1f
	beq         t0, t1, .PCodeIsExpression_label_282

	// *** Basic block 44

	j           .PCodeIsExpression_label_290

	// *** Basic block 45

.PCodeIsExpression_label_246:

	// *** Basic block 46

.PCodeIsExpression_label_247:

	// *** Basic block 47

.PCodeIsExpression_label_248:

	// *** Basic block 48

.PCodeIsExpression_label_249:

	// *** Basic block 49

.PCodeIsExpression_label_250:

	// *** Basic block 50

.PCodeIsExpression_label_251:

	// *** Basic block 51

.PCodeIsExpression_label_252:

	// *** Basic block 52

.PCodeIsExpression_label_253:

	// *** Basic block 53

.PCodeIsExpression_label_254:

	// *** Basic block 54

.PCodeIsExpression_label_255:

	// *** Basic block 55

.PCodeIsExpression_label_256:

	// *** Basic block 56

.PCodeIsExpression_label_257:

	// *** Basic block 57

.PCodeIsExpression_label_258:

	// *** Basic block 58

.PCodeIsExpression_label_259:

	// *** Basic block 59

.PCodeIsExpression_label_260:

	// *** Basic block 60

.PCodeIsExpression_label_261:

	// *** Basic block 61

.PCodeIsExpression_label_262:

	// *** Basic block 62

.PCodeIsExpression_label_263:

	// *** Basic block 63

.PCodeIsExpression_label_264:

	// *** Basic block 64

.PCodeIsExpression_label_265:

	// *** Basic block 65

.PCodeIsExpression_label_266:

	// *** Basic block 66

.PCodeIsExpression_label_267:

	// *** Basic block 67

.PCodeIsExpression_label_268:

	// *** Basic block 68

.PCodeIsExpression_label_269:

	// *** Basic block 69

.PCodeIsExpression_label_270:

	// *** Basic block 70

.PCodeIsExpression_label_271:

	// *** Basic block 71

.PCodeIsExpression_label_272:

	// *** Basic block 72

.PCodeIsExpression_label_273:

	// *** Basic block 73

.PCodeIsExpression_label_274:

	// *** Basic block 74

.PCodeIsExpression_label_275:

	// *** Basic block 75

.PCodeIsExpression_label_276:

	// *** Basic block 76

.PCodeIsExpression_label_277:

	// *** Basic block 77

.PCodeIsExpression_label_278:

	// *** Basic block 78

.PCodeIsExpression_label_279:

	// *** Basic block 79

.PCodeIsExpression_label_280:

	// *** Basic block 80

.PCodeIsExpression_label_281:

	// *** Basic block 81

.PCodeIsExpression_label_282:

	// *** Basic block 82

.PCodeIsExpression_label_283:
	mv          a0, x0

	// *** Basic block 83

.PCodeIsExpression_label_287:
	ret         

	// *** Basic block 84

.PCodeIsExpression_label_290:
	li          a0, 1		// 0x1 ASCII \x1
	ret         
.func_end_PCodeIsExpression:
	.size PCodeIsExpression, .func_end_PCodeIsExpression-PCodeIsExpression

	.global PCodeIsSignedLoad
	.type PCodeIsSignedLoad, @function

PCodeIsSignedLoad:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	li          t1, 46		// 0x2e ASCII '.'
	blt         t0, t1, .PCodeIsSignedLoad_label_36

	// *** Basic block 1

	li          t1, 48		// 0x30 ASCII '0'
	blt         t1, t0, .PCodeIsSignedLoad_label_36

	// *** Basic block 2

	addi        t1, t0, -46
	slli        t1, t1, 2
	auipc       t2, 0
	add         t1, t2, t1
	jalr        x0, t1, 12

	// *** Basic block 3

	j           .PCodeIsSignedLoad_label_30

	// *** Basic block 4

	j           .PCodeIsSignedLoad_label_29

	// *** Basic block 5

	j           .PCodeIsSignedLoad_label_28

	// *** Basic block 6

.PCodeIsSignedLoad_label_28:

	// *** Basic block 7

.PCodeIsSignedLoad_label_29:

	// *** Basic block 8

.PCodeIsSignedLoad_label_30:
	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 9

.PCodeIsSignedLoad_label_33:
	ret         

	// *** Basic block 10

.PCodeIsSignedLoad_label_36:
	mv          a0, x0
	ret         
.func_end_PCodeIsSignedLoad:
	.size PCodeIsSignedLoad, .func_end_PCodeIsSignedLoad-PCodeIsSignedLoad

.PCend:
	.data
load_opcodes:
	.type   load_opcodes,@object
	.local  load_opcodes
	.size   load_opcodes,224
	.p2align  3
	.global TypeIsInt
	.long    TypeIsInt
	.word   46
	.space  4
	.global TypeIsShort
	.long    TypeIsShort
	.word   47
	.space  4
	.global TypeIsChar
	.long    TypeIsChar
	.word   48
	.space  4
	.global TypeIsLong
	.long    TypeIsLong
	.word   52
	.space  4
	.global TypeIsLongLong
	.long    TypeIsLongLong
	.word   52
	.space  4
	.global TypeIsUnsignedInt
	.long    TypeIsUnsignedInt
	.word   49
	.space  4
	.global TypeIsUnsignedShort
	.long    TypeIsUnsignedShort
	.word   51
	.space  4
	.global TypeIsUnsignedChar
	.long    TypeIsUnsignedChar
	.word   50
	.space  4
	.global TypeIsFloat
	.long    TypeIsFloat
	.word   53
	.space  4
	.global TypeIsDouble
	.long    TypeIsDouble
	.word   54
	.space  4
	.global TypeIsBool
	.long    TypeIsBool
	.word   48
	.space  4
	.global TypeIsPointerOrArray
	.long    TypeIsPointerOrArray
	.word   52
	.space  4
	.global TypeIsFunction
	.long    TypeIsFunction
	.word   52
	.space  4
	.word   0
	.space  4
	.word   0
	.space  4

push_map:
	.type   push_map,@object
	.local  push_map
	.size   push_map,288
	.p2align  3
	.global TypeIsInt
	.long    TypeIsInt
	.word   38
	.space  4
	.long   4
	.global TypeIsShort
	.long    TypeIsShort
	.word   38
	.space  4
	.long   4
	.global TypeIsChar
	.long    TypeIsChar
	.word   38
	.space  4
	.long   4
	.global TypeIsLong
	.long    TypeIsLong
	.word   41
	.space  4
	.long   8
	.global TypeIsLongLong
	.long    TypeIsLongLong
	.word   41
	.space  4
	.long   8
	.global TypeIsFloat
	.long    TypeIsFloat
	.word   39
	.space  4
	.long   4
	.global TypeIsDouble
	.long    TypeIsDouble
	.word   40
	.space  4
	.long   8
	.global TypeIsLongDouble
	.long    TypeIsLongDouble
	.word   40
	.space  4
	.long   8
	.global TypeIsPointerOrArray
	.long    TypeIsPointerOrArray
	.word   41
	.space  4
	.long   8
	.global TypeIsFunction
	.long    TypeIsFunction
	.word   41
	.space  4
	.long   8
	.global TypeIsStructOrUnion
	.long    TypeIsStructOrUnion
	.word   41
	.space  4
	.long   8
	.word   0
	.space  4
	.word   38
	.space  4
	.long   0

	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "ap"
	.type .str.1, @object
	.size .str.1, 3

.str.2:
	.asciz "decsp"
	.type .str.2, @object
	.size .str.2, 6

.str.3:
	.asciz "incsp"
	.type .str.3, @object
	.size .str.3, 6

.str.4:
	.asciz "push"
	.type .str.4, @object
	.size .str.4, 5

.str.5:
	.asciz "pushf"
	.type .str.5, @object
	.size .str.5, 6

.str.6:
	.asciz "pushd"
	.type .str.6, @object
	.size .str.6, 6

.str.7:
	.asciz "pushx"
	.type .str.7, @object
	.size .str.7, 6

.str.8:
	.asciz "pop"
	.type .str.8, @object
	.size .str.8, 4

.str.9:
	.asciz "popf"
	.type .str.9, @object
	.size .str.9, 5

.str.10:
	.asciz "popd"
	.type .str.10, @object
	.size .str.10, 5

.str.11:
	.asciz "popx"
	.type .str.11, @object
	.size .str.11, 5

.str.12:
	.asciz "ldw"
	.type .str.12, @object
	.size .str.12, 4

.str.13:
	.asciz "ldh"
	.type .str.13, @object
	.size .str.13, 4

.str.14:
	.asciz "lduw"
	.type .str.14, @object
	.size .str.14, 5

.str.15:
	.asciz "ldub"
	.type .str.15, @object
	.size .str.15, 5

.str.16:
	.asciz "lduh"
	.type .str.16, @object
	.size .str.16, 5

.str.17:
	.asciz "ldf"
	.type .str.17, @object
	.size .str.17, 4

.str.18:
	.asciz "ldd"
	.type .str.18, @object
	.size .str.18, 4

.str.19:
	.asciz "ldb"
	.type .str.19, @object
	.size .str.19, 4

.str.20:
	.asciz "ldx"
	.type .str.20, @object
	.size .str.20, 4

.str.21:
	.asciz "stw"
	.type .str.21, @object
	.size .str.21, 4

.str.22:
	.asciz "sth"
	.type .str.22, @object
	.size .str.22, 4

.str.23:
	.asciz "stx"
	.type .str.23, @object
	.size .str.23, 4

.str.24:
	.asciz "stf"
	.type .str.24, @object
	.size .str.24, 4

.str.25:
	.asciz "std"
	.type .str.25, @object
	.size .str.25, 4

.str.26:
	.asciz "stb"
	.type .str.26, @object
	.size .str.26, 4

.str.27:
	.asciz "add"
	.type .str.27, @object
	.size .str.27, 4

.str.28:
	.asciz "addf"
	.type .str.28, @object
	.size .str.28, 5

.str.29:
	.asciz "addd"
	.type .str.29, @object
	.size .str.29, 5

.str.30:
	.asciz "addc"
	.type .str.30, @object
	.size .str.30, 5

.str.31:
	.asciz "sub"
	.type .str.31, @object
	.size .str.31, 4

.str.32:
	.asciz "subf"
	.type .str.32, @object
	.size .str.32, 5

.str.33:
	.asciz "subd"
	.type .str.33, @object
	.size .str.33, 5

.str.34:
	.asciz "mul"
	.type .str.34, @object
	.size .str.34, 4

.str.35:
	.asciz "mulf"
	.type .str.35, @object
	.size .str.35, 5

.str.36:
	.asciz "muld"
	.type .str.36, @object
	.size .str.36, 5

.str.37:
	.asciz "div"
	.type .str.37, @object
	.size .str.37, 4

.str.38:
	.asciz "divu"
	.type .str.38, @object
	.size .str.38, 5

.str.39:
	.asciz "divf"
	.type .str.39, @object
	.size .str.39, 5

.str.40:
	.asciz "divd"
	.type .str.40, @object
	.size .str.40, 5

.str.41:
	.asciz "mod"
	.type .str.41, @object
	.size .str.41, 4

.str.42:
	.asciz "modu"
	.type .str.42, @object
	.size .str.42, 5

.str.43:
	.asciz "lsr"
	.type .str.43, @object
	.size .str.43, 4

.str.44:
	.asciz "asr"
	.type .str.44, @object
	.size .str.44, 4

.str.45:
	.asciz "lsl"
	.type .str.45, @object
	.size .str.45, 4

.str.46:
	.asciz "or"
	.type .str.46, @object
	.size .str.46, 3

.str.47:
	.asciz "and"
	.type .str.47, @object
	.size .str.47, 4

.str.48:
	.asciz "xor"
	.type .str.48, @object
	.size .str.48, 4

.str.49:
	.asciz "not"
	.type .str.49, @object
	.size .str.49, 4

.str.50:
	.asciz "inv"
	.type .str.50, @object
	.size .str.50, 4

.str.51:
	.asciz "neg"
	.type .str.51, @object
	.size .str.51, 4

.str.52:
	.asciz "negf"
	.type .str.52, @object
	.size .str.52, 5

.str.53:
	.asciz "negd"
	.type .str.53, @object
	.size .str.53, 5

.str.54:
	.asciz "cmpeq"
	.type .str.54, @object
	.size .str.54, 6

.str.55:
	.asciz "cmpne"
	.type .str.55, @object
	.size .str.55, 6

.str.56:
	.asciz "cmplt"
	.type .str.56, @object
	.size .str.56, 6

.str.57:
	.asciz "cmple"
	.type .str.57, @object
	.size .str.57, 6

.str.58:
	.asciz "cmpgt"
	.type .str.58, @object
	.size .str.58, 6

.str.59:
	.asciz "cmpge"
	.type .str.59, @object
	.size .str.59, 6

.str.60:
	.asciz "cmpltu"
	.type .str.60, @object
	.size .str.60, 7

.str.61:
	.asciz "cmpleu"
	.type .str.61, @object
	.size .str.61, 7

.str.62:
	.asciz "cmpgtu"
	.type .str.62, @object
	.size .str.62, 7

.str.63:
	.asciz "cmpgeu"
	.type .str.63, @object
	.size .str.63, 7

.str.64:
	.asciz "cmpeqf"
	.type .str.64, @object
	.size .str.64, 7

.str.65:
	.asciz "cmpnef"
	.type .str.65, @object
	.size .str.65, 7

.str.66:
	.asciz "cmpltf"
	.type .str.66, @object
	.size .str.66, 7

.str.67:
	.asciz "cmplef"
	.type .str.67, @object
	.size .str.67, 7

.str.68:
	.asciz "cmpgtf"
	.type .str.68, @object
	.size .str.68, 7

.str.69:
	.asciz "cmpgef"
	.type .str.69, @object
	.size .str.69, 7

.str.70:
	.asciz "cmpeqd"
	.type .str.70, @object
	.size .str.70, 7

.str.71:
	.asciz "cmpned"
	.type .str.71, @object
	.size .str.71, 7

.str.72:
	.asciz "cmpltd"
	.type .str.72, @object
	.size .str.72, 7

.str.73:
	.asciz "cmpled"
	.type .str.73, @object
	.size .str.73, 7

.str.74:
	.asciz "cmpgtd"
	.type .str.74, @object
	.size .str.74, 7

.str.75:
	.asciz "cmpged"
	.type .str.75, @object
	.size .str.75, 7

.str.76:
	.asciz "bnz"
	.type .str.76, @object
	.size .str.76, 4

.str.77:
	.asciz "bz"
	.type .str.77, @object
	.size .str.77, 3

.str.78:
	.asciz "bra"
	.type .str.78, @object
	.size .str.78, 4

.str.79:
	.asciz "cbra"
	.type .str.79, @object
	.size .str.79, 5

.str.80:
	.asciz "i2f"
	.type .str.80, @object
	.size .str.80, 4

.str.81:
	.asciz "i2d"
	.type .str.81, @object
	.size .str.81, 4

.str.82:
	.asciz "ui2f"
	.type .str.82, @object
	.size .str.82, 5

.str.83:
	.asciz "ui2d"
	.type .str.83, @object
	.size .str.83, 5

.str.84:
	.asciz "f2d"
	.type .str.84, @object
	.size .str.84, 4

.str.85:
	.asciz "d2f"
	.type .str.85, @object
	.size .str.85, 4

.str.86:
	.asciz "f2i"
	.type .str.86, @object
	.size .str.86, 4

.str.87:
	.asciz "d2i"
	.type .str.87, @object
	.size .str.87, 4

.str.88:
	.asciz "f2ui"
	.type .str.88, @object
	.size .str.88, 5

.str.89:
	.asciz "d2ui"
	.type .str.89, @object
	.size .str.89, 5

.str.90:
	.asciz "jmp"
	.type .str.90, @object
	.size .str.90, 4

.str.91:
	.asciz "cjmp"
	.type .str.91, @object
	.size .str.91, 5

.str.92:
	.asciz "adr"
	.type .str.92, @object
	.size .str.92, 4

.str.93:
	.asciz "adrs"
	.type .str.93, @object
	.size .str.93, 5

.str.94:
	.asciz "adrtls"
	.type .str.94, @object
	.size .str.94, 7

.str.95:
	.asciz "call"
	.type .str.95, @object
	.size .str.95, 5

.str.96:
	.asciz "callf"
	.type .str.96, @object
	.size .str.96, 6

.str.97:
	.asciz "calld"
	.type .str.97, @object
	.size .str.97, 6

.str.98:
	.asciz "rcall"
	.type .str.98, @object
	.size .str.98, 6

.str.99:
	.asciz "rcallf"
	.type .str.99, @object
	.size .str.99, 7

.str.100:
	.asciz "rcalld"
	.type .str.100, @object
	.size .str.100, 7

.str.101:
	.asciz "ret"
	.type .str.101, @object
	.size .str.101, 4

.str.102:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.102, @object
	.size .str.102, 30

.str.103:
	.asciz "(null)"
	.type .str.103, @object
	.size .str.103, 1

.str.104:
	.asciz "opcode != 0"
	.type .str.104, @object
	.size .str.104, 12

.str.105:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.105, @object
	.size .str.105, 30

.str.106:
	.asciz "(null)"
	.type .str.106, @object
	.size .str.106, 1

.str.107:
	.asciz "false"
	.type .str.107, @object
	.size .str.107, 6

.str.108:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.108, @object
	.size .str.108, 30

.str.109:
	.asciz "(null)"
	.type .str.109, @object
	.size .str.109, 1

.str.110:
	.asciz "false"
	.type .str.110, @object
	.size .str.110, 6

.str.111:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.111, @object
	.size .str.111, 30

.str.112:
	.asciz "(null)"
	.type .str.112, @object
	.size .str.112, 1

.str.113:
	.asciz "node->inputs.length == 2"
	.type .str.113, @object
	.size .str.113, 25

.str.114:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.114, @object
	.size .str.114, 30

.str.115:
	.asciz "(null)"
	.type .str.115, @object
	.size .str.115, 1

.str.116:
	.asciz "node->inputs.length == 2"
	.type .str.116, @object
	.size .str.116, 25

.str.117:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.117, @object
	.size .str.117, 30

.str.118:
	.asciz "(null)"
	.type .str.118, @object
	.size .str.118, 1

.str.119:
	.asciz "node->inputs.length == 2"
	.type .str.119, @object
	.size .str.119, 25

.str.120:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.120, @object
	.size .str.120, 30

.str.121:
	.asciz "(null)"
	.type .str.121, @object
	.size .str.121, 1

.str.122:
	.asciz "node->inputs.length == 2"
	.type .str.122, @object
	.size .str.122, 25

.str.123:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.123, @object
	.size .str.123, 30

.str.124:
	.asciz "(null)"
	.type .str.124, @object
	.size .str.124, 1

.str.125:
	.asciz "node->inputs.length == 2"
	.type .str.125, @object
	.size .str.125, 25

.str.126:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.126, @object
	.size .str.126, 30

.str.127:
	.asciz "(null)"
	.type .str.127, @object
	.size .str.127, 1

.str.128:
	.asciz "node->inputs.length <= 2"
	.type .str.128, @object
	.size .str.128, 25

.str.129:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.129, @object
	.size .str.129, 30

.str.130:
	.asciz "(null)"
	.type .str.130, @object
	.size .str.130, 1

.str.131:
	.asciz "false"
	.type .str.131, @object
	.size .str.131, 6

.str.132:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.132, @object
	.size .str.132, 30

.str.133:
	.asciz "(null)"
	.type .str.133, @object
	.size .str.133, 1

.str.134:
	.asciz "node->inputs.length == 2"
	.type .str.134, @object
	.size .str.134, 25

.str.135:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.135, @object
	.size .str.135, 30

.str.136:
	.asciz "(null)"
	.type .str.136, @object
	.size .str.136, 1

.str.137:
	.asciz "node->inputs.length == 1"
	.type .str.137, @object
	.size .str.137, 25

.str.138:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.138, @object
	.size .str.138, 30

.str.139:
	.asciz "(null)"
	.type .str.139, @object
	.size .str.139, 1

.str.140:
	.asciz "node->inputs.length == 1"
	.type .str.140, @object
	.size .str.140, 25

.str.141:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.141, @object
	.size .str.141, 30

.str.142:
	.asciz "(null)"
	.type .str.142, @object
	.size .str.142, 1

.str.143:
	.asciz "addr != NULL"
	.type .str.143, @object
	.size .str.143, 13

.str.144:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.144, @object
	.size .str.144, 30

.str.145:
	.asciz "(null)"
	.type .str.145, @object
	.size .str.145, 1

.str.146:
	.asciz "node->inputs.length == 1"
	.type .str.146, @object
	.size .str.146, 25

.str.147:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.147, @object
	.size .str.147, 30

.str.148:
	.asciz "(null)"
	.type .str.148, @object
	.size .str.148, 1

.str.149:
	.asciz "false"
	.type .str.149, @object
	.size .str.149, 6

.str.150:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.150, @object
	.size .str.150, 30

.str.151:
	.asciz "(null)"
	.type .str.151, @object
	.size .str.151, 1

.str.152:
	.asciz "node->inputs.length == 2"
	.type .str.152, @object
	.size .str.152, 25

.str.153:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.153, @object
	.size .str.153, 30

.str.154:
	.asciz "(null)"
	.type .str.154, @object
	.size .str.154, 1

.str.155:
	.asciz "false"
	.type .str.155, @object
	.size .str.155, 6

.str.156:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.156, @object
	.size .str.156, 30

.str.157:
	.asciz "(null)"
	.type .str.157, @object
	.size .str.157, 1

.str.158:
	.asciz "false"
	.type .str.158, @object
	.size .str.158, 6

.str.159:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.159, @object
	.size .str.159, 30

.str.160:
	.asciz "(null)"
	.type .str.160, @object
	.size .str.160, 1

.str.161:
	.asciz "node->inputs.length >= 1"
	.type .str.161, @object
	.size .str.161, 25

.str.162:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.162, @object
	.size .str.162, 30

.str.163:
	.asciz "(null)"
	.type .str.163, @object
	.size .str.163, 1

.str.164:
	.asciz "node->inputs.length == 1"
	.type .str.164, @object
	.size .str.164, 25

.str.165:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.165, @object
	.size .str.165, 30

.str.166:
	.asciz "(null)"
	.type .str.166, @object
	.size .str.166, 1

.str.167:
	.asciz "false"
	.type .str.167, @object
	.size .str.167, 6

.str.168:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.168, @object
	.size .str.168, 30

.str.169:
	.asciz "(null)"
	.type .str.169, @object
	.size .str.169, 1

.str.170:
	.asciz "node->inputs.length == 3"
	.type .str.170, @object
	.size .str.170, 25

.str.171:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.171, @object
	.size .str.171, 30

.str.172:
	.asciz "(null)"
	.type .str.172, @object
	.size .str.172, 1

.str.173:
	.asciz "node->inputs.length == 1"
	.type .str.173, @object
	.size .str.173, 25

.str.174:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.174, @object
	.size .str.174, 30

.str.175:
	.asciz "(null)"
	.type .str.175, @object
	.size .str.175, 1

.str.176:
	.asciz "false"
	.type .str.176, @object
	.size .str.176, 6


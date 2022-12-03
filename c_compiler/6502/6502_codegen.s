	.file   "6502/6502_codegen.c"
	.text
	.option pic
.PCbegin:
	.global W65C02OpcodeName
	.type W65C02OpcodeName, @function

W65C02OpcodeName:

	// *** Basic block 0

	.global TargetOpcodeName
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	li          t1, 21		// 0x15 ASCII \x15
	blt         t0, t1, .W65C02OpcodeName_label_357

	// *** Basic block 1

	li          t1, 194		// 0xc2 ASCII \xc2
	blt         t1, t0, .W65C02OpcodeName_label_357

	// *** Basic block 2

	addi        t1, t0, -21
	slli        t1, t1, 2
	auipc       t2, 0
	add         t1, t2, t1
	jalr        x0, t1, 12

	// *** Basic block 3

	j           .W65C02OpcodeName_label_1157

	// *** Basic block 4

	j           .W65C02OpcodeName_label_357

	// *** Basic block 5

	j           .W65C02OpcodeName_label_357

	// *** Basic block 6

	j           .W65C02OpcodeName_label_357

	// *** Basic block 7

	j           .W65C02OpcodeName_label_357

	// *** Basic block 8

	j           .W65C02OpcodeName_label_357

	// *** Basic block 9

	j           .W65C02OpcodeName_label_357

	// *** Basic block 10

	j           .W65C02OpcodeName_label_357

	// *** Basic block 11

	j           .W65C02OpcodeName_label_357

	// *** Basic block 12

	j           .W65C02OpcodeName_label_357

	// *** Basic block 13

	j           .W65C02OpcodeName_label_357

	// *** Basic block 14

	j           .W65C02OpcodeName_label_357

	// *** Basic block 15

	j           .W65C02OpcodeName_label_357

	// *** Basic block 16

	j           .W65C02OpcodeName_label_357

	// *** Basic block 17

	j           .W65C02OpcodeName_label_367

	// *** Basic block 18

	j           .W65C02OpcodeName_label_372

	// *** Basic block 19

	j           .W65C02OpcodeName_label_357

	// *** Basic block 20

	j           .W65C02OpcodeName_label_377

	// *** Basic block 21

	j           .W65C02OpcodeName_label_382

	// *** Basic block 22

	j           .W65C02OpcodeName_label_387

	// *** Basic block 23

	j           .W65C02OpcodeName_label_392

	// *** Basic block 24

	j           .W65C02OpcodeName_label_397

	// *** Basic block 25

	j           .W65C02OpcodeName_label_402

	// *** Basic block 26

	j           .W65C02OpcodeName_label_407

	// *** Basic block 27

	j           .W65C02OpcodeName_label_412

	// *** Basic block 28

	j           .W65C02OpcodeName_label_417

	// *** Basic block 29

	j           .W65C02OpcodeName_label_422

	// *** Basic block 30

	j           .W65C02OpcodeName_label_427

	// *** Basic block 31

	j           .W65C02OpcodeName_label_432

	// *** Basic block 32

	j           .W65C02OpcodeName_label_437

	// *** Basic block 33

	j           .W65C02OpcodeName_label_442

	// *** Basic block 34

	j           .W65C02OpcodeName_label_447

	// *** Basic block 35

	j           .W65C02OpcodeName_label_452

	// *** Basic block 36

	j           .W65C02OpcodeName_label_457

	// *** Basic block 37

	j           .W65C02OpcodeName_label_462

	// *** Basic block 38

	j           .W65C02OpcodeName_label_467

	// *** Basic block 39

	j           .W65C02OpcodeName_label_472

	// *** Basic block 40

	j           .W65C02OpcodeName_label_477

	// *** Basic block 41

	j           .W65C02OpcodeName_label_482

	// *** Basic block 42

	j           .W65C02OpcodeName_label_487

	// *** Basic block 43

	j           .W65C02OpcodeName_label_492

	// *** Basic block 44

	j           .W65C02OpcodeName_label_497

	// *** Basic block 45

	j           .W65C02OpcodeName_label_502

	// *** Basic block 46

	j           .W65C02OpcodeName_label_507

	// *** Basic block 47

	j           .W65C02OpcodeName_label_512

	// *** Basic block 48

	j           .W65C02OpcodeName_label_517

	// *** Basic block 49

	j           .W65C02OpcodeName_label_522

	// *** Basic block 50

	j           .W65C02OpcodeName_label_527

	// *** Basic block 51

	j           .W65C02OpcodeName_label_557

	// *** Basic block 52

	j           .W65C02OpcodeName_label_532

	// *** Basic block 53

	j           .W65C02OpcodeName_label_537

	// *** Basic block 54

	j           .W65C02OpcodeName_label_542

	// *** Basic block 55

	j           .W65C02OpcodeName_label_562

	// *** Basic block 56

	j           .W65C02OpcodeName_label_547

	// *** Basic block 57

	j           .W65C02OpcodeName_label_552

	// *** Basic block 58

	j           .W65C02OpcodeName_label_567

	// *** Basic block 59

	j           .W65C02OpcodeName_label_572

	// *** Basic block 60

	j           .W65C02OpcodeName_label_577

	// *** Basic block 61

	j           .W65C02OpcodeName_label_582

	// *** Basic block 62

	j           .W65C02OpcodeName_label_587

	// *** Basic block 63

	j           .W65C02OpcodeName_label_592

	// *** Basic block 64

	j           .W65C02OpcodeName_label_597

	// *** Basic block 65

	j           .W65C02OpcodeName_label_602

	// *** Basic block 66

	j           .W65C02OpcodeName_label_607

	// *** Basic block 67

	j           .W65C02OpcodeName_label_612

	// *** Basic block 68

	j           .W65C02OpcodeName_label_617

	// *** Basic block 69

	j           .W65C02OpcodeName_label_622

	// *** Basic block 70

	j           .W65C02OpcodeName_label_627

	// *** Basic block 71

	j           .W65C02OpcodeName_label_632

	// *** Basic block 72

	j           .W65C02OpcodeName_label_637

	// *** Basic block 73

	j           .W65C02OpcodeName_label_642

	// *** Basic block 74

	j           .W65C02OpcodeName_label_647

	// *** Basic block 75

	j           .W65C02OpcodeName_label_652

	// *** Basic block 76

	j           .W65C02OpcodeName_label_657

	// *** Basic block 77

	j           .W65C02OpcodeName_label_662

	// *** Basic block 78

	j           .W65C02OpcodeName_label_357

	// *** Basic block 79

	j           .W65C02OpcodeName_label_667

	// *** Basic block 80

	j           .W65C02OpcodeName_label_672

	// *** Basic block 81

	j           .W65C02OpcodeName_label_677

	// *** Basic block 82

	j           .W65C02OpcodeName_label_682

	// *** Basic block 83

	j           .W65C02OpcodeName_label_687

	// *** Basic block 84

	j           .W65C02OpcodeName_label_692

	// *** Basic block 85

	j           .W65C02OpcodeName_label_697

	// *** Basic block 86

	j           .W65C02OpcodeName_label_702

	// *** Basic block 87

	j           .W65C02OpcodeName_label_707

	// *** Basic block 88

	j           .W65C02OpcodeName_label_712

	// *** Basic block 89

	j           .W65C02OpcodeName_label_717

	// *** Basic block 90

	j           .W65C02OpcodeName_label_722

	// *** Basic block 91

	j           .W65C02OpcodeName_label_727

	// *** Basic block 92

	j           .W65C02OpcodeName_label_732

	// *** Basic block 93

	j           .W65C02OpcodeName_label_737

	// *** Basic block 94

	j           .W65C02OpcodeName_label_742

	// *** Basic block 95

	j           .W65C02OpcodeName_label_747

	// *** Basic block 96

	j           .W65C02OpcodeName_label_752

	// *** Basic block 97

	j           .W65C02OpcodeName_label_757

	// *** Basic block 98

	j           .W65C02OpcodeName_label_762

	// *** Basic block 99

	j           .W65C02OpcodeName_label_767

	// *** Basic block 100

	j           .W65C02OpcodeName_label_772

	// *** Basic block 101

	j           .W65C02OpcodeName_label_777

	// *** Basic block 102

	j           .W65C02OpcodeName_label_782

	// *** Basic block 103

	j           .W65C02OpcodeName_label_787

	// *** Basic block 104

	j           .W65C02OpcodeName_label_792

	// *** Basic block 105

	j           .W65C02OpcodeName_label_797

	// *** Basic block 106

	j           .W65C02OpcodeName_label_802

	// *** Basic block 107

	j           .W65C02OpcodeName_label_807

	// *** Basic block 108

	j           .W65C02OpcodeName_label_812

	// *** Basic block 109

	j           .W65C02OpcodeName_label_817

	// *** Basic block 110

	j           .W65C02OpcodeName_label_822

	// *** Basic block 111

	j           .W65C02OpcodeName_label_827

	// *** Basic block 112

	j           .W65C02OpcodeName_label_832

	// *** Basic block 113

	j           .W65C02OpcodeName_label_837

	// *** Basic block 114

	j           .W65C02OpcodeName_label_842

	// *** Basic block 115

	j           .W65C02OpcodeName_label_847

	// *** Basic block 116

	j           .W65C02OpcodeName_label_852

	// *** Basic block 117

	j           .W65C02OpcodeName_label_857

	// *** Basic block 118

	j           .W65C02OpcodeName_label_862

	// *** Basic block 119

	j           .W65C02OpcodeName_label_867

	// *** Basic block 120

	j           .W65C02OpcodeName_label_872

	// *** Basic block 121

	j           .W65C02OpcodeName_label_877

	// *** Basic block 122

	j           .W65C02OpcodeName_label_882

	// *** Basic block 123

	j           .W65C02OpcodeName_label_887

	// *** Basic block 124

	j           .W65C02OpcodeName_label_892

	// *** Basic block 125

	j           .W65C02OpcodeName_label_897

	// *** Basic block 126

	j           .W65C02OpcodeName_label_902

	// *** Basic block 127

	j           .W65C02OpcodeName_label_907

	// *** Basic block 128

	j           .W65C02OpcodeName_label_912

	// *** Basic block 129

	j           .W65C02OpcodeName_label_917

	// *** Basic block 130

	j           .W65C02OpcodeName_label_922

	// *** Basic block 131

	j           .W65C02OpcodeName_label_927

	// *** Basic block 132

	j           .W65C02OpcodeName_label_932

	// *** Basic block 133

	j           .W65C02OpcodeName_label_937

	// *** Basic block 134

	j           .W65C02OpcodeName_label_942

	// *** Basic block 135

	j           .W65C02OpcodeName_label_947

	// *** Basic block 136

	j           .W65C02OpcodeName_label_952

	// *** Basic block 137

	j           .W65C02OpcodeName_label_957

	// *** Basic block 138

	j           .W65C02OpcodeName_label_962

	// *** Basic block 139

	j           .W65C02OpcodeName_label_967

	// *** Basic block 140

	j           .W65C02OpcodeName_label_972

	// *** Basic block 141

	j           .W65C02OpcodeName_label_977

	// *** Basic block 142

	j           .W65C02OpcodeName_label_982

	// *** Basic block 143

	j           .W65C02OpcodeName_label_987

	// *** Basic block 144

	j           .W65C02OpcodeName_label_992

	// *** Basic block 145

	j           .W65C02OpcodeName_label_997

	// *** Basic block 146

	j           .W65C02OpcodeName_label_1002

	// *** Basic block 147

	j           .W65C02OpcodeName_label_1007

	// *** Basic block 148

	j           .W65C02OpcodeName_label_1012

	// *** Basic block 149

	j           .W65C02OpcodeName_label_1017

	// *** Basic block 150

	j           .W65C02OpcodeName_label_1022

	// *** Basic block 151

	j           .W65C02OpcodeName_label_1027

	// *** Basic block 152

	j           .W65C02OpcodeName_label_1032

	// *** Basic block 153

	j           .W65C02OpcodeName_label_1037

	// *** Basic block 154

	j           .W65C02OpcodeName_label_1042

	// *** Basic block 155

	j           .W65C02OpcodeName_label_1047

	// *** Basic block 156

	j           .W65C02OpcodeName_label_1052

	// *** Basic block 157

	j           .W65C02OpcodeName_label_1057

	// *** Basic block 158

	j           .W65C02OpcodeName_label_1062

	// *** Basic block 159

	j           .W65C02OpcodeName_label_1067

	// *** Basic block 160

	j           .W65C02OpcodeName_label_1072

	// *** Basic block 161

	j           .W65C02OpcodeName_label_1077

	// *** Basic block 162

	j           .W65C02OpcodeName_label_1082

	// *** Basic block 163

	j           .W65C02OpcodeName_label_1087

	// *** Basic block 164

	j           .W65C02OpcodeName_label_1092

	// *** Basic block 165

	j           .W65C02OpcodeName_label_1097

	// *** Basic block 166

	j           .W65C02OpcodeName_label_1102

	// *** Basic block 167

	j           .W65C02OpcodeName_label_1107

	// *** Basic block 168

	j           .W65C02OpcodeName_label_1112

	// *** Basic block 169

	j           .W65C02OpcodeName_label_1117

	// *** Basic block 170

	j           .W65C02OpcodeName_label_1122

	// *** Basic block 171

	j           .W65C02OpcodeName_label_1127

	// *** Basic block 172

	j           .W65C02OpcodeName_label_1132

	// *** Basic block 173

	j           .W65C02OpcodeName_label_1137

	// *** Basic block 174

	j           .W65C02OpcodeName_label_1142

	// *** Basic block 175

	j           .W65C02OpcodeName_label_1147

	// *** Basic block 176

	j           .W65C02OpcodeName_label_1152

	// *** Basic block 177

.W65C02OpcodeName_label_357:
	mv          a0, t0
	j           TargetOpcodeName

	// *** Basic block 180

.W65C02OpcodeName_label_367:
	lla         a0, .str.1
	ret         

	// *** Basic block 181

.W65C02OpcodeName_label_372:
	lla         a0, .str.2
	ret         

	// *** Basic block 182

.W65C02OpcodeName_label_377:
	lla         a0, .str.3
	ret         

	// *** Basic block 183

.W65C02OpcodeName_label_382:
	lla         a0, .str.4
	ret         

	// *** Basic block 184

.W65C02OpcodeName_label_387:
	lla         a0, .str.5
	ret         

	// *** Basic block 185

.W65C02OpcodeName_label_392:
	lla         a0, .str.6
	ret         

	// *** Basic block 186

.W65C02OpcodeName_label_397:
	lla         a0, .str.7
	ret         

	// *** Basic block 187

.W65C02OpcodeName_label_402:
	lla         a0, .str.8
	ret         

	// *** Basic block 188

.W65C02OpcodeName_label_407:
	lla         a0, .str.9
	ret         

	// *** Basic block 189

.W65C02OpcodeName_label_412:
	lla         a0, .str.10
	ret         

	// *** Basic block 190

.W65C02OpcodeName_label_417:
	lla         a0, .str.11
	ret         

	// *** Basic block 191

.W65C02OpcodeName_label_422:
	lla         a0, .str.12
	ret         

	// *** Basic block 192

.W65C02OpcodeName_label_427:
	lla         a0, .str.13
	ret         

	// *** Basic block 193

.W65C02OpcodeName_label_432:
	lla         a0, .str.14
	ret         

	// *** Basic block 194

.W65C02OpcodeName_label_437:
	lla         a0, .str.15
	ret         

	// *** Basic block 195

.W65C02OpcodeName_label_442:
	lla         a0, .str.16
	ret         

	// *** Basic block 196

.W65C02OpcodeName_label_447:
	lla         a0, .str.17
	ret         

	// *** Basic block 197

.W65C02OpcodeName_label_452:
	lla         a0, .str.18
	ret         

	// *** Basic block 198

.W65C02OpcodeName_label_457:
	lla         a0, .str.19
	ret         

	// *** Basic block 199

.W65C02OpcodeName_label_462:
	lla         a0, .str.20
	ret         

	// *** Basic block 200

.W65C02OpcodeName_label_467:
	lla         a0, .str.21
	ret         

	// *** Basic block 201

.W65C02OpcodeName_label_472:
	lla         a0, .str.22
	ret         

	// *** Basic block 202

.W65C02OpcodeName_label_477:
	lla         a0, .str.23
	ret         

	// *** Basic block 203

.W65C02OpcodeName_label_482:
	lla         a0, .str.24
	ret         

	// *** Basic block 204

.W65C02OpcodeName_label_487:
	lla         a0, .str.25
	ret         

	// *** Basic block 205

.W65C02OpcodeName_label_492:
	lla         a0, .str.26
	ret         

	// *** Basic block 206

.W65C02OpcodeName_label_497:
	lla         a0, .str.27
	ret         

	// *** Basic block 207

.W65C02OpcodeName_label_502:
	lla         a0, .str.28
	ret         

	// *** Basic block 208

.W65C02OpcodeName_label_507:
	lla         a0, .str.29
	ret         

	// *** Basic block 209

.W65C02OpcodeName_label_512:
	lla         a0, .str.30
	ret         

	// *** Basic block 210

.W65C02OpcodeName_label_517:
	lla         a0, .str.31
	ret         

	// *** Basic block 211

.W65C02OpcodeName_label_522:
	lla         a0, .str.32
	ret         

	// *** Basic block 212

.W65C02OpcodeName_label_527:
	lla         a0, .str.33
	ret         

	// *** Basic block 213

.W65C02OpcodeName_label_532:
	lla         a0, .str.34
	ret         

	// *** Basic block 214

.W65C02OpcodeName_label_537:
	lla         a0, .str.35
	ret         

	// *** Basic block 215

.W65C02OpcodeName_label_542:
	lla         a0, .str.36
	ret         

	// *** Basic block 216

.W65C02OpcodeName_label_547:
	lla         a0, .str.37
	ret         

	// *** Basic block 217

.W65C02OpcodeName_label_552:
	lla         a0, .str.38
	ret         

	// *** Basic block 218

.W65C02OpcodeName_label_557:
	lla         a0, .str.39
	ret         

	// *** Basic block 219

.W65C02OpcodeName_label_562:
	lla         a0, .str.40
	ret         

	// *** Basic block 220

.W65C02OpcodeName_label_567:
	lla         a0, .str.41
	ret         

	// *** Basic block 221

.W65C02OpcodeName_label_572:
	lla         a0, .str.42
	ret         

	// *** Basic block 222

.W65C02OpcodeName_label_577:
	lla         a0, .str.43
	ret         

	// *** Basic block 223

.W65C02OpcodeName_label_582:
	lla         a0, .str.44
	ret         

	// *** Basic block 224

.W65C02OpcodeName_label_587:
	lla         a0, .str.45
	ret         

	// *** Basic block 225

.W65C02OpcodeName_label_592:
	lla         a0, .str.46
	ret         

	// *** Basic block 226

.W65C02OpcodeName_label_597:
	lla         a0, .str.47
	ret         

	// *** Basic block 227

.W65C02OpcodeName_label_602:
	lla         a0, .str.48
	ret         

	// *** Basic block 228

.W65C02OpcodeName_label_607:
	lla         a0, .str.49
	ret         

	// *** Basic block 229

.W65C02OpcodeName_label_612:
	lla         a0, .str.50
	ret         

	// *** Basic block 230

.W65C02OpcodeName_label_617:
	lla         a0, .str.51
	ret         

	// *** Basic block 231

.W65C02OpcodeName_label_622:
	lla         a0, .str.52
	ret         

	// *** Basic block 232

.W65C02OpcodeName_label_627:
	lla         a0, .str.53
	ret         

	// *** Basic block 233

.W65C02OpcodeName_label_632:
	lla         a0, .str.54
	ret         

	// *** Basic block 234

.W65C02OpcodeName_label_637:
	lla         a0, .str.55
	ret         

	// *** Basic block 235

.W65C02OpcodeName_label_642:
	lla         a0, .str.56
	ret         

	// *** Basic block 236

.W65C02OpcodeName_label_647:
	lla         a0, .str.57
	ret         

	// *** Basic block 237

.W65C02OpcodeName_label_652:
	lla         a0, .str.58
	ret         

	// *** Basic block 238

.W65C02OpcodeName_label_657:
	lla         a0, .str.59
	ret         

	// *** Basic block 239

.W65C02OpcodeName_label_662:
	lla         a0, .str.60
	ret         

	// *** Basic block 240

.W65C02OpcodeName_label_667:
	lla         a0, .str.61
	ret         

	// *** Basic block 241

.W65C02OpcodeName_label_672:
	lla         a0, .str.62
	ret         

	// *** Basic block 242

.W65C02OpcodeName_label_677:
	lla         a0, .str.63
	ret         

	// *** Basic block 243

.W65C02OpcodeName_label_682:
	lla         a0, .str.64
	ret         

	// *** Basic block 244

.W65C02OpcodeName_label_687:
	lla         a0, .str.65
	ret         

	// *** Basic block 245

.W65C02OpcodeName_label_692:
	lla         a0, .str.66
	ret         

	// *** Basic block 246

.W65C02OpcodeName_label_697:
	lla         a0, .str.67
	ret         

	// *** Basic block 247

.W65C02OpcodeName_label_702:
	lla         a0, .str.68
	ret         

	// *** Basic block 248

.W65C02OpcodeName_label_707:
	lla         a0, .str.69
	ret         

	// *** Basic block 249

.W65C02OpcodeName_label_712:
	lla         a0, .str.70
	ret         

	// *** Basic block 250

.W65C02OpcodeName_label_717:
	lla         a0, .str.71
	ret         

	// *** Basic block 251

.W65C02OpcodeName_label_722:
	lla         a0, .str.72
	ret         

	// *** Basic block 252

.W65C02OpcodeName_label_727:
	lla         a0, .str.73
	ret         

	// *** Basic block 253

.W65C02OpcodeName_label_732:
	lla         a0, .str.74
	ret         

	// *** Basic block 254

.W65C02OpcodeName_label_737:
	lla         a0, .str.75
	ret         

	// *** Basic block 255

.W65C02OpcodeName_label_742:
	lla         a0, .str.76
	ret         

	// *** Basic block 256

.W65C02OpcodeName_label_747:
	lla         a0, .str.77
	ret         

	// *** Basic block 257

.W65C02OpcodeName_label_752:
	lla         a0, .str.78
	ret         

	// *** Basic block 258

.W65C02OpcodeName_label_757:
	lla         a0, .str.79
	ret         

	// *** Basic block 259

.W65C02OpcodeName_label_762:
	lla         a0, .str.80
	ret         

	// *** Basic block 260

.W65C02OpcodeName_label_767:
	lla         a0, .str.81
	ret         

	// *** Basic block 261

.W65C02OpcodeName_label_772:
	lla         a0, .str.82
	ret         

	// *** Basic block 262

.W65C02OpcodeName_label_777:
	lla         a0, .str.83
	ret         

	// *** Basic block 263

.W65C02OpcodeName_label_782:
	lla         a0, .str.84
	ret         

	// *** Basic block 264

.W65C02OpcodeName_label_787:
	lla         a0, .str.85
	ret         

	// *** Basic block 265

.W65C02OpcodeName_label_792:
	lla         a0, .str.86
	ret         

	// *** Basic block 266

.W65C02OpcodeName_label_797:
	lla         a0, .str.87
	ret         

	// *** Basic block 267

.W65C02OpcodeName_label_802:
	lla         a0, .str.88
	ret         

	// *** Basic block 268

.W65C02OpcodeName_label_807:
	lla         a0, .str.89
	ret         

	// *** Basic block 269

.W65C02OpcodeName_label_812:
	lla         a0, .str.90
	ret         

	// *** Basic block 270

.W65C02OpcodeName_label_817:
	lla         a0, .str.91
	ret         

	// *** Basic block 271

.W65C02OpcodeName_label_822:
	lla         a0, .str.92
	ret         

	// *** Basic block 272

.W65C02OpcodeName_label_827:
	lla         a0, .str.93
	ret         

	// *** Basic block 273

.W65C02OpcodeName_label_832:
	lla         a0, .str.94
	ret         

	// *** Basic block 274

.W65C02OpcodeName_label_837:
	lla         a0, .str.95
	ret         

	// *** Basic block 275

.W65C02OpcodeName_label_842:
	lla         a0, .str.96
	ret         

	// *** Basic block 276

.W65C02OpcodeName_label_847:
	lla         a0, .str.97
	ret         

	// *** Basic block 277

.W65C02OpcodeName_label_852:
	lla         a0, .str.98
	ret         

	// *** Basic block 278

.W65C02OpcodeName_label_857:
	lla         a0, .str.99
	ret         

	// *** Basic block 279

.W65C02OpcodeName_label_862:
	lla         a0, .str.100
	ret         

	// *** Basic block 280

.W65C02OpcodeName_label_867:
	lla         a0, .str.101
	ret         

	// *** Basic block 281

.W65C02OpcodeName_label_872:
	lla         a0, .str.102
	ret         

	// *** Basic block 282

.W65C02OpcodeName_label_877:
	lla         a0, .str.103
	ret         

	// *** Basic block 283

.W65C02OpcodeName_label_882:
	lla         a0, .str.104
	ret         

	// *** Basic block 284

.W65C02OpcodeName_label_887:
	lla         a0, .str.105
	ret         

	// *** Basic block 285

.W65C02OpcodeName_label_892:
	lla         a0, .str.106
	ret         

	// *** Basic block 286

.W65C02OpcodeName_label_897:
	lla         a0, .str.107
	ret         

	// *** Basic block 287

.W65C02OpcodeName_label_902:
	lla         a0, .str.108
	ret         

	// *** Basic block 288

.W65C02OpcodeName_label_907:
	lla         a0, .str.109
	ret         

	// *** Basic block 289

.W65C02OpcodeName_label_912:
	lla         a0, .str.110
	ret         

	// *** Basic block 290

.W65C02OpcodeName_label_917:
	lla         a0, .str.111
	ret         

	// *** Basic block 291

.W65C02OpcodeName_label_922:
	lla         a0, .str.112
	ret         

	// *** Basic block 292

.W65C02OpcodeName_label_927:
	lla         a0, .str.113
	ret         

	// *** Basic block 293

.W65C02OpcodeName_label_932:
	lla         a0, .str.114
	ret         

	// *** Basic block 294

.W65C02OpcodeName_label_937:
	lla         a0, .str.115
	ret         

	// *** Basic block 295

.W65C02OpcodeName_label_942:
	lla         a0, .str.116
	ret         

	// *** Basic block 296

.W65C02OpcodeName_label_947:
	lla         a0, .str.117
	ret         

	// *** Basic block 297

.W65C02OpcodeName_label_952:
	lla         a0, .str.118
	ret         

	// *** Basic block 298

.W65C02OpcodeName_label_957:
	lla         a0, .str.119
	ret         

	// *** Basic block 299

.W65C02OpcodeName_label_962:
	lla         a0, .str.120
	ret         

	// *** Basic block 300

.W65C02OpcodeName_label_967:
	lla         a0, .str.121
	ret         

	// *** Basic block 301

.W65C02OpcodeName_label_972:
	lla         a0, .str.122
	ret         

	// *** Basic block 302

.W65C02OpcodeName_label_977:
	lla         a0, .str.123
	ret         

	// *** Basic block 303

.W65C02OpcodeName_label_982:
	lla         a0, .str.124
	ret         

	// *** Basic block 304

.W65C02OpcodeName_label_987:
	lla         a0, .str.125
	ret         

	// *** Basic block 305

.W65C02OpcodeName_label_992:
	lla         a0, .str.126
	ret         

	// *** Basic block 306

.W65C02OpcodeName_label_997:
	lla         a0, .str.127
	ret         

	// *** Basic block 307

.W65C02OpcodeName_label_1002:
	lla         a0, .str.128
	ret         

	// *** Basic block 308

.W65C02OpcodeName_label_1007:
	lla         a0, .str.129
	ret         

	// *** Basic block 309

.W65C02OpcodeName_label_1012:
	lla         a0, .str.130
	ret         

	// *** Basic block 310

.W65C02OpcodeName_label_1017:
	lla         a0, .str.131
	ret         

	// *** Basic block 311

.W65C02OpcodeName_label_1022:
	lla         a0, .str.132
	ret         

	// *** Basic block 312

.W65C02OpcodeName_label_1027:
	lla         a0, .str.133
	ret         

	// *** Basic block 313

.W65C02OpcodeName_label_1032:
	lla         a0, .str.134
	ret         

	// *** Basic block 314

.W65C02OpcodeName_label_1037:
	lla         a0, .str.135
	ret         

	// *** Basic block 315

.W65C02OpcodeName_label_1042:
	lla         a0, .str.136
	ret         

	// *** Basic block 316

.W65C02OpcodeName_label_1047:
	lla         a0, .str.137
	ret         

	// *** Basic block 317

.W65C02OpcodeName_label_1052:
	lla         a0, .str.138
	ret         

	// *** Basic block 318

.W65C02OpcodeName_label_1057:
	lla         a0, .str.139
	ret         

	// *** Basic block 319

.W65C02OpcodeName_label_1062:
	lla         a0, .str.140
	ret         

	// *** Basic block 320

.W65C02OpcodeName_label_1067:
	lla         a0, .str.141
	ret         

	// *** Basic block 321

.W65C02OpcodeName_label_1072:
	lla         a0, .str.142
	ret         

	// *** Basic block 322

.W65C02OpcodeName_label_1077:
	lla         a0, .str.143
	ret         

	// *** Basic block 323

.W65C02OpcodeName_label_1082:
	lla         a0, .str.144
	ret         

	// *** Basic block 324

.W65C02OpcodeName_label_1087:
	lla         a0, .str.145
	ret         

	// *** Basic block 325

.W65C02OpcodeName_label_1092:
	lla         a0, .str.146
	ret         

	// *** Basic block 326

.W65C02OpcodeName_label_1097:
	lla         a0, .str.147
	ret         

	// *** Basic block 327

.W65C02OpcodeName_label_1102:
	lla         a0, .str.148
	ret         

	// *** Basic block 328

.W65C02OpcodeName_label_1107:
	lla         a0, .str.149
	ret         

	// *** Basic block 329

.W65C02OpcodeName_label_1112:
	lla         a0, .str.150
	ret         

	// *** Basic block 330

.W65C02OpcodeName_label_1117:
	lla         a0, .str.151
	ret         

	// *** Basic block 331

.W65C02OpcodeName_label_1122:
	lla         a0, .str.152
	ret         

	// *** Basic block 332

.W65C02OpcodeName_label_1127:
	lla         a0, .str.153
	ret         

	// *** Basic block 333

.W65C02OpcodeName_label_1132:
	lla         a0, .str.154
	ret         

	// *** Basic block 334

.W65C02OpcodeName_label_1137:
	lla         a0, .str.155
	ret         

	// *** Basic block 335

.W65C02OpcodeName_label_1142:
	lla         a0, .str.156
	ret         

	// *** Basic block 336

.W65C02OpcodeName_label_1147:
	lla         a0, .str.157
	ret         

	// *** Basic block 337

.W65C02OpcodeName_label_1152:
	lla         a0, .str.158
	ret         

	// *** Basic block 338

.W65C02OpcodeName_label_1157:
	lla         a0, .str.159
	ret         
.func_end_W65C02OpcodeName:
	.size W65C02OpcodeName, .func_end_W65C02OpcodeName-W65C02OpcodeName

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

	li          t0, 49		// 0x31 ASCII '1'
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

	.global W65C02GeneratorInit
	.type W65C02GeneratorInit, @function

W65C02GeneratorInit:

	// *** Basic block 0

	.global TargetGeneratorInit
	.global W65C02RegisterAllocatorInit
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
	j           W65C02RegisterAllocatorInit
.func_end_W65C02GeneratorInit:
	.size W65C02GeneratorInit, .func_end_W65C02GeneratorInit-W65C02GeneratorInit

	.global New6502Generator
	.type New6502Generator, @function

New6502Generator:

	// *** Basic block 0

	.global malloc
	.global W65C02GeneratorInit
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
	li          a0, 1184		// 0x4a0
	call        malloc

	// *** Basic block 1

	mv          s2, a0
	mv          a1, s1
	mv          a0, s2
	call        W65C02GeneratorInit

	// *** Basic block 2

	mv          a0, s2

	// *** Basic block 3

.New6502Generator_label_21:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_New6502Generator:
	.size New6502Generator, .func_end_New6502Generator-New6502Generator

	.global W65C02GeneratorDestruct
	.type W65C02GeneratorDestruct, @function

W65C02GeneratorDestruct:

	// *** Basic block 0

	.global TargetGeneratorDestruct
	.global W65C02RegisterAllocatorDestruct
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
	j           W65C02RegisterAllocatorDestruct
.func_end_W65C02GeneratorDestruct:
	.size W65C02GeneratorDestruct, .func_end_W65C02GeneratorDestruct-W65C02GeneratorDestruct

	.global W65C02GeneratorDelete
	.type W65C02GeneratorDelete, @function

W65C02GeneratorDelete:

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
	.global W65C02GeneratorDestruct
	.global free
	mv          s1, a0
	call        W65C02GeneratorDestruct

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_W65C02GeneratorDelete:
	.size W65C02GeneratorDelete, .func_end_W65C02GeneratorDelete-W65C02GeneratorDelete

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
	li          s5, 67		// 0x43 ASCII 'C'
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
	lla         a0, .str.160
	lla         a1, .str.161
	lla         a3, .str.162
	li          t0, 528		// 0x210
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
	blt         s3, t0, .GetTlsVariableAddress_label_59

	// *** Basic block 1

	li          t0, 4		// 0x4 ASCII \x4
	blt         t0, s3, .GetTlsVariableAddress_label_59

	// *** Basic block 2

	addi        t0, s3, -1
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 3

	j           .GetTlsVariableAddress_label_61

	// *** Basic block 4

	j           .GetTlsVariableAddress_label_62

	// *** Basic block 5

	j           .GetTlsVariableAddress_label_142

	// *** Basic block 6

	j           .GetTlsVariableAddress_label_208

	// *** Basic block 7

.GetTlsVariableAddress_label_59:
	call        abort

	// *** Basic block 8

.GetTlsVariableAddress_label_61:

	// *** Basic block 9

.GetTlsVariableAddress_label_62:
	mv          a0, s2
	call        GetLoweredNode

	// *** Basic block 10

	mv          a1, a0
	li          t0, 182		// 0xb6 ASCII \xb6
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 11

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 12

	mv          s9, a0
	mv          a1, s9
	li          t0, 52		// 0x34 ASCII '4'
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
	li          t0, 185		// 0xb9 ASCII \xb9
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
	li          t0, 6		// 0x6 ASCII \x6
	mv          a2, t0
	mv          a1, x0
	mv          a0, s1
	call        GetIntConstant

	// *** Basic block 18

	mv          a1, a0
	li          t0, 51		// 0x33 ASCII '3'
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 19

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 20

	mv          a0, s11

	// *** Basic block 21

.GetTlsVariableAddress_label_139:
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

.GetTlsVariableAddress_label_142:
	mv          a0, s2
	call        GetLoweredNode

	// *** Basic block 23

	mv          a1, a0
	li          t0, 184		// 0xb8 ASCII \xb8
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
	li          t0, 66		// 0x42 ASCII 'B'
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
	li          t0, 87		// 0x57 ASCII 'W'
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

.GetTlsVariableAddress_label_208:
	mv          a0, s1
	call        ThreadPointer

	// *** Basic block 33

	mv          s7, a0
	mv          a0, s2
	call        GetLoweredNode

	// *** Basic block 34

	mv          a1, a0
	li          t0, 35		// 0x23 ASCII '#'
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
	li          t0, 84		// 0x54 ASCII 'T'
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
	addi sp, sp, -96
	// Saved return address (offset 88) and frame pointer (offset 80)
	sd ra, 88(sp)
	sd s0, 80(sp)
	addi s0, sp, 96
	// Local vars at offset -16(s0)
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
	li          t0, 182		// 0xb6 ASCII \xb6
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 11

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 12

	mv          s7, a0
	mv          a1, s7
	li          t0, 52		// 0x34 ASCII '4'
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
	li          t0, 185		// 0xb9 ASCII \xb9
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
	li          s9, 6		// 0x6 ASCII \x6
	mv          a2, s9
	mv          a1, x0
	mv          a0, s1
	call        GetIntConstant

	// *** Basic block 18

	mv          a1, a0
	li          t0, 51		// 0x33 ASCII '3'
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 19

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 20

	mv          a3, x0
	mv          a2, s9
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
	li          t0, 184		// 0xb8 ASCII \xb8
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 24

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 25

	mv          s5, a0
	mv          a3, x0
	li          t0, 6		// 0x6 ASCII \x6
	mv          a2, t0
	mv          a1, x0
	mv          a0, s1
	call        GetIntConstant

	// *** Basic block 26

	sd          a0, 0(s4)
	ld          a2, 0(s4)
	mv          a1, s5
	li          t0, 66		// 0x42 ASCII 'B'
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
	li          t0, 84		// 0x54 ASCII 'T'
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
	li          t0, 35		// 0x23 ASCII '#'
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
	li          t0, 84		// 0x54 ASCII 'T'
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 37

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 38

	sd          a0, 0(s3)
	mv          a3, x0
	li          t0, 6		// 0x6 ASCII \x6
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
.func_end_GetTlsAddressAndOffset:
	.size GetTlsAddressAndOffset, .func_end_GetTlsAddressAndOffset-GetTlsAddressAndOffset

	.local  ComparisonOpcode
	.type ComparisonOpcode, @function

ComparisonOpcode:

	// *** Basic block 0

	.global TypeIsUnsigned
	.global TypeIsLong
	.global TypeIsLongLong
	.global TypeIsShort
	.global TypeIsChar
	.global TypeIsBool
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
	lw          s1, 20(a0)
	ld          t0, 24(a0)
	ld          t1, 0(t0)
	ld          s2, 80(t1)
	sub         t0, s2, x0
	snez        s3, t0
	beq         s2, x0, .ComparisonOpcode_label_91

	// *** Basic block 1

	mv          a0, s2
	call        TypeIsUnsigned

	// *** Basic block 3

.ComparisonOpcode_label_91:
	beq         s2, x0, .ComparisonOpcode_label_107

	// *** Basic block 4

	mv          a0, s2
	call        TypeIsLong

	// *** Basic block 5

	mv          s4, a0
	bnez        a0, .ComparisonOpcode_label_106

	// *** Basic block 6

	mv          a0, s2
	call        TypeIsLongLong

	// *** Basic block 8

.ComparisonOpcode_label_106:

	// *** Basic block 9

.ComparisonOpcode_label_107:
	mv          s5, s3
	beq         s2, x0, .ComparisonOpcode_label_116

	// *** Basic block 10

	mv          a0, s2
	call        TypeIsShort

	// *** Basic block 12

.ComparisonOpcode_label_116:
	beq         s2, x0, .ComparisonOpcode_label_132

	// *** Basic block 13

	mv          a0, s2
	call        TypeIsChar

	// *** Basic block 14

	mv          s6, a0
	bnez        a0, .ComparisonOpcode_label_131

	// *** Basic block 15

	mv          a0, s2
	call        TypeIsBool

	// *** Basic block 17

.ComparisonOpcode_label_131:

	// *** Basic block 18

.ComparisonOpcode_label_132:
	li          t0, 64		// 0x40 ASCII '@'
	blt         s1, t0, .ComparisonOpcode_label_501

	// *** Basic block 19

	li          t0, 87		// 0x57 ASCII 'W'
	blt         t0, s1, .ComparisonOpcode_label_501

	// *** Basic block 20

	addi        t0, s1, -64
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 21

	j           .ComparisonOpcode_label_173

	// *** Basic block 22

	j           .ComparisonOpcode_label_194

	// *** Basic block 23

	j           .ComparisonOpcode_label_213

	// *** Basic block 24

	j           .ComparisonOpcode_label_260

	// *** Basic block 25

	j           .ComparisonOpcode_label_307

	// *** Basic block 26

	j           .ComparisonOpcode_label_354

	// *** Basic block 27

	j           .ComparisonOpcode_label_401

	// *** Basic block 28

	j           .ComparisonOpcode_label_405

	// *** Basic block 29

	j           .ComparisonOpcode_label_409

	// *** Basic block 30

	j           .ComparisonOpcode_label_413

	// *** Basic block 31

	j           .ComparisonOpcode_label_417

	// *** Basic block 32

	j           .ComparisonOpcode_label_421

	// *** Basic block 33

	j           .ComparisonOpcode_label_425

	// *** Basic block 34

	j           .ComparisonOpcode_label_429

	// *** Basic block 35

	j           .ComparisonOpcode_label_433

	// *** Basic block 36

	j           .ComparisonOpcode_label_437

	// *** Basic block 37

	j           .ComparisonOpcode_label_441

	// *** Basic block 38

	j           .ComparisonOpcode_label_445

	// *** Basic block 39

	j           .ComparisonOpcode_label_449

	// *** Basic block 40

	j           .ComparisonOpcode_label_453

	// *** Basic block 41

	j           .ComparisonOpcode_label_457

	// *** Basic block 42

	j           .ComparisonOpcode_label_468

	// *** Basic block 43

	j           .ComparisonOpcode_label_479

	// *** Basic block 44

	j           .ComparisonOpcode_label_490

	// *** Basic block 45

.ComparisonOpcode_label_173:
	beqz        s4, .ComparisonOpcode_label_180

	// *** Basic block 46

	li          a0, 144		// 0x90 ASCII \x90

	// *** Basic block 47

.ComparisonOpcode_label_177:
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

	// *** Basic block 48

.ComparisonOpcode_label_180:
	beqz        s5, .ComparisonOpcode_label_185

	// *** Basic block 49

	li          a0, 124		// 0x7c ASCII '|'
	j           .ComparisonOpcode_label_177

	// *** Basic block 50

.ComparisonOpcode_label_185:
	beqz        s6, .ComparisonOpcode_label_190

	// *** Basic block 51

	li          a0, 114		// 0x72 ASCII 'r'
	j           .ComparisonOpcode_label_177

	// *** Basic block 52

.ComparisonOpcode_label_190:
	li          a0, 134		// 0x86 ASCII \x86
	j           .ComparisonOpcode_label_177

	// *** Basic block 53

.ComparisonOpcode_label_194:
	beqz        s4, .ComparisonOpcode_label_199

	// *** Basic block 54

	li          a0, 145		// 0x91 ASCII \x91
	j           .ComparisonOpcode_label_177

	// *** Basic block 55

.ComparisonOpcode_label_199:
	beqz        s5, .ComparisonOpcode_label_204

	// *** Basic block 56

	li          a0, 125		// 0x7d ASCII '}'
	j           .ComparisonOpcode_label_177

	// *** Basic block 57

.ComparisonOpcode_label_204:
	beqz        s6, .ComparisonOpcode_label_209

	// *** Basic block 58

	li          a0, 115		// 0x73 ASCII 's'
	j           .ComparisonOpcode_label_177

	// *** Basic block 59

.ComparisonOpcode_label_209:
	li          a0, 135		// 0x87 ASCII \x87
	j           .ComparisonOpcode_label_177

	// *** Basic block 60

.ComparisonOpcode_label_213:
	beqz        s4, .ComparisonOpcode_label_225

	// *** Basic block 61

	beqz        s3, .ComparisonOpcode_label_220

	// *** Basic block 62

	li          a0, 150		// 0x96 ASCII \x96
	j           .ComparisonOpcode_label_222

	// *** Basic block 63

.ComparisonOpcode_label_220:
	li          a0, 146		// 0x92 ASCII \x92

	// *** Basic block 64

.ComparisonOpcode_label_222:
	j           .ComparisonOpcode_label_177

	// *** Basic block 65

.ComparisonOpcode_label_225:
	beqz        s5, .ComparisonOpcode_label_237

	// *** Basic block 66

	beqz        s3, .ComparisonOpcode_label_232

	// *** Basic block 67

	li          a0, 130		// 0x82 ASCII \x82
	j           .ComparisonOpcode_label_234

	// *** Basic block 68

.ComparisonOpcode_label_232:
	li          a0, 126		// 0x7e ASCII '~'

	// *** Basic block 69

.ComparisonOpcode_label_234:
	j           .ComparisonOpcode_label_177

	// *** Basic block 70

.ComparisonOpcode_label_237:
	beqz        s6, .ComparisonOpcode_label_249

	// *** Basic block 71

	beqz        s3, .ComparisonOpcode_label_244

	// *** Basic block 72

	li          a0, 120		// 0x78 ASCII 'x'
	j           .ComparisonOpcode_label_246

	// *** Basic block 73

.ComparisonOpcode_label_244:
	li          a0, 116		// 0x74 ASCII 't'

	// *** Basic block 74

.ComparisonOpcode_label_246:
	j           .ComparisonOpcode_label_177

	// *** Basic block 75

.ComparisonOpcode_label_249:
	beqz        s3, .ComparisonOpcode_label_255

	// *** Basic block 76

	li          a0, 140		// 0x8c ASCII \x8c
	j           .ComparisonOpcode_label_257

	// *** Basic block 77

.ComparisonOpcode_label_255:
	li          a0, 136		// 0x88 ASCII \x88

	// *** Basic block 78

.ComparisonOpcode_label_257:
	j           .ComparisonOpcode_label_177

	// *** Basic block 79

.ComparisonOpcode_label_260:
	beqz        s4, .ComparisonOpcode_label_272

	// *** Basic block 80

	beqz        s3, .ComparisonOpcode_label_267

	// *** Basic block 81

	li          a0, 151		// 0x97 ASCII \x97
	j           .ComparisonOpcode_label_269

	// *** Basic block 82

.ComparisonOpcode_label_267:
	li          a0, 147		// 0x93 ASCII \x93

	// *** Basic block 83

.ComparisonOpcode_label_269:
	j           .ComparisonOpcode_label_177

	// *** Basic block 84

.ComparisonOpcode_label_272:
	beqz        s5, .ComparisonOpcode_label_284

	// *** Basic block 85

	beqz        s3, .ComparisonOpcode_label_279

	// *** Basic block 86

	li          a0, 131		// 0x83 ASCII \x83
	j           .ComparisonOpcode_label_281

	// *** Basic block 87

.ComparisonOpcode_label_279:
	li          a0, 127		// 0x7f ASCII \x7f

	// *** Basic block 88

.ComparisonOpcode_label_281:
	j           .ComparisonOpcode_label_177

	// *** Basic block 89

.ComparisonOpcode_label_284:
	beqz        s6, .ComparisonOpcode_label_296

	// *** Basic block 90

	beqz        s3, .ComparisonOpcode_label_291

	// *** Basic block 91

	li          a0, 121		// 0x79 ASCII 'y'
	j           .ComparisonOpcode_label_293

	// *** Basic block 92

.ComparisonOpcode_label_291:
	li          a0, 117		// 0x75 ASCII 'u'

	// *** Basic block 93

.ComparisonOpcode_label_293:
	j           .ComparisonOpcode_label_177

	// *** Basic block 94

.ComparisonOpcode_label_296:
	beqz        s3, .ComparisonOpcode_label_302

	// *** Basic block 95

	li          a0, 141		// 0x8d ASCII \x8d
	j           .ComparisonOpcode_label_304

	// *** Basic block 96

.ComparisonOpcode_label_302:
	li          a0, 137		// 0x89 ASCII \x89

	// *** Basic block 97

.ComparisonOpcode_label_304:
	j           .ComparisonOpcode_label_177

	// *** Basic block 98

.ComparisonOpcode_label_307:
	beqz        s4, .ComparisonOpcode_label_319

	// *** Basic block 99

	beqz        s3, .ComparisonOpcode_label_314

	// *** Basic block 100

	li          a0, 152		// 0x98 ASCII \x98
	j           .ComparisonOpcode_label_316

	// *** Basic block 101

.ComparisonOpcode_label_314:
	li          a0, 148		// 0x94 ASCII \x94

	// *** Basic block 102

.ComparisonOpcode_label_316:
	j           .ComparisonOpcode_label_177

	// *** Basic block 103

.ComparisonOpcode_label_319:
	beqz        s5, .ComparisonOpcode_label_331

	// *** Basic block 104

	beqz        s3, .ComparisonOpcode_label_326

	// *** Basic block 105

	li          a0, 132		// 0x84 ASCII \x84
	j           .ComparisonOpcode_label_328

	// *** Basic block 106

.ComparisonOpcode_label_326:
	li          a0, 128		// 0x80 ASCII \x80

	// *** Basic block 107

.ComparisonOpcode_label_328:
	j           .ComparisonOpcode_label_177

	// *** Basic block 108

.ComparisonOpcode_label_331:
	beqz        s6, .ComparisonOpcode_label_343

	// *** Basic block 109

	beqz        s3, .ComparisonOpcode_label_338

	// *** Basic block 110

	li          a0, 122		// 0x7a ASCII 'z'
	j           .ComparisonOpcode_label_340

	// *** Basic block 111

.ComparisonOpcode_label_338:
	li          a0, 118		// 0x76 ASCII 'v'

	// *** Basic block 112

.ComparisonOpcode_label_340:
	j           .ComparisonOpcode_label_177

	// *** Basic block 113

.ComparisonOpcode_label_343:
	beqz        s3, .ComparisonOpcode_label_349

	// *** Basic block 114

	li          a0, 142		// 0x8e ASCII \x8e
	j           .ComparisonOpcode_label_351

	// *** Basic block 115

.ComparisonOpcode_label_349:
	li          a0, 138		// 0x8a ASCII \x8a

	// *** Basic block 116

.ComparisonOpcode_label_351:
	j           .ComparisonOpcode_label_177

	// *** Basic block 117

.ComparisonOpcode_label_354:
	beqz        s4, .ComparisonOpcode_label_366

	// *** Basic block 118

	beqz        s3, .ComparisonOpcode_label_361

	// *** Basic block 119

	li          a0, 153		// 0x99 ASCII \x99
	j           .ComparisonOpcode_label_363

	// *** Basic block 120

.ComparisonOpcode_label_361:
	li          a0, 149		// 0x95 ASCII \x95

	// *** Basic block 121

.ComparisonOpcode_label_363:
	j           .ComparisonOpcode_label_177

	// *** Basic block 122

.ComparisonOpcode_label_366:
	beqz        s5, .ComparisonOpcode_label_378

	// *** Basic block 123

	beqz        s3, .ComparisonOpcode_label_373

	// *** Basic block 124

	li          a0, 133		// 0x85 ASCII \x85
	j           .ComparisonOpcode_label_375

	// *** Basic block 125

.ComparisonOpcode_label_373:
	li          a0, 129		// 0x81 ASCII \x81

	// *** Basic block 126

.ComparisonOpcode_label_375:
	j           .ComparisonOpcode_label_177

	// *** Basic block 127

.ComparisonOpcode_label_378:
	beqz        s6, .ComparisonOpcode_label_390

	// *** Basic block 128

	beqz        s3, .ComparisonOpcode_label_385

	// *** Basic block 129

	li          a0, 123		// 0x7b ASCII '{'
	j           .ComparisonOpcode_label_387

	// *** Basic block 130

.ComparisonOpcode_label_385:
	li          a0, 119		// 0x77 ASCII 'w'

	// *** Basic block 131

.ComparisonOpcode_label_387:
	j           .ComparisonOpcode_label_177

	// *** Basic block 132

.ComparisonOpcode_label_390:
	beqz        s3, .ComparisonOpcode_label_396

	// *** Basic block 133

	li          a0, 143		// 0x8f ASCII \x8f
	j           .ComparisonOpcode_label_398

	// *** Basic block 134

.ComparisonOpcode_label_396:
	li          a0, 139		// 0x8b ASCII \x8b

	// *** Basic block 135

.ComparisonOpcode_label_398:
	j           .ComparisonOpcode_label_177

	// *** Basic block 136

.ComparisonOpcode_label_401:
	li          a0, 154		// 0x9a ASCII \x9a
	j           .ComparisonOpcode_label_177

	// *** Basic block 137

.ComparisonOpcode_label_405:
	li          a0, 155		// 0x9b ASCII \x9b
	j           .ComparisonOpcode_label_177

	// *** Basic block 138

.ComparisonOpcode_label_409:
	li          a0, 156		// 0x9c ASCII \x9c
	j           .ComparisonOpcode_label_177

	// *** Basic block 139

.ComparisonOpcode_label_413:
	li          a0, 157		// 0x9d ASCII \x9d
	j           .ComparisonOpcode_label_177

	// *** Basic block 140

.ComparisonOpcode_label_417:
	li          a0, 158		// 0x9e ASCII \x9e
	j           .ComparisonOpcode_label_177

	// *** Basic block 141

.ComparisonOpcode_label_421:
	li          a0, 159		// 0x9f ASCII \x9f
	j           .ComparisonOpcode_label_177

	// *** Basic block 142

.ComparisonOpcode_label_425:
	li          a0, 160		// 0xa0 ASCII \xa0
	j           .ComparisonOpcode_label_177

	// *** Basic block 143

.ComparisonOpcode_label_429:
	li          a0, 161		// 0xa1 ASCII \xa1
	j           .ComparisonOpcode_label_177

	// *** Basic block 144

.ComparisonOpcode_label_433:
	li          a0, 162		// 0xa2 ASCII \xa2
	j           .ComparisonOpcode_label_177

	// *** Basic block 145

.ComparisonOpcode_label_437:
	li          a0, 163		// 0xa3 ASCII \xa3
	j           .ComparisonOpcode_label_177

	// *** Basic block 146

.ComparisonOpcode_label_441:
	li          a0, 164		// 0xa4 ASCII \xa4
	j           .ComparisonOpcode_label_177

	// *** Basic block 147

.ComparisonOpcode_label_445:
	li          a0, 165		// 0xa5 ASCII \xa5
	j           .ComparisonOpcode_label_177

	// *** Basic block 148

.ComparisonOpcode_label_449:
	li          a0, 124		// 0x7c ASCII '|'
	j           .ComparisonOpcode_label_177

	// *** Basic block 149

.ComparisonOpcode_label_453:
	li          a0, 125		// 0x7d ASCII '}'
	j           .ComparisonOpcode_label_177

	// *** Basic block 150

.ComparisonOpcode_label_457:
	beqz        s3, .ComparisonOpcode_label_463

	// *** Basic block 151

	li          a0, 130		// 0x82 ASCII \x82
	j           .ComparisonOpcode_label_465

	// *** Basic block 152

.ComparisonOpcode_label_463:
	li          a0, 126		// 0x7e ASCII '~'

	// *** Basic block 153

.ComparisonOpcode_label_465:
	j           .ComparisonOpcode_label_177

	// *** Basic block 154

.ComparisonOpcode_label_468:
	beqz        s3, .ComparisonOpcode_label_474

	// *** Basic block 155

	li          a0, 131		// 0x83 ASCII \x83
	j           .ComparisonOpcode_label_476

	// *** Basic block 156

.ComparisonOpcode_label_474:
	li          a0, 127		// 0x7f ASCII \x7f

	// *** Basic block 157

.ComparisonOpcode_label_476:
	j           .ComparisonOpcode_label_177

	// *** Basic block 158

.ComparisonOpcode_label_479:
	beqz        s3, .ComparisonOpcode_label_485

	// *** Basic block 159

	li          a0, 132		// 0x84 ASCII \x84
	j           .ComparisonOpcode_label_487

	// *** Basic block 160

.ComparisonOpcode_label_485:
	li          a0, 128		// 0x80 ASCII \x80

	// *** Basic block 161

.ComparisonOpcode_label_487:
	j           .ComparisonOpcode_label_177

	// *** Basic block 162

.ComparisonOpcode_label_490:
	beqz        s3, .ComparisonOpcode_label_496

	// *** Basic block 163

	li          a0, 133		// 0x85 ASCII \x85
	j           .ComparisonOpcode_label_498

	// *** Basic block 164

.ComparisonOpcode_label_496:
	li          a0, 129		// 0x81 ASCII \x81

	// *** Basic block 165

.ComparisonOpcode_label_498:
	j           .ComparisonOpcode_label_177

	// *** Basic block 166

.ComparisonOpcode_label_501:
	lla         a0, .str.163
	lla         a1, .str.164
	lla         a3, .str.165
	li          t0, 745		// 0x2e9
	mv          a2, t0
	call        printf

	// *** Basic block 167

	call        abort

	// *** Basic block 168

	mv          a0, x0
	j           .ComparisonOpcode_label_177
.func_end_ComparisonOpcode:
	.size ComparisonOpcode, .func_end_ComparisonOpcode-ComparisonOpcode

	.local  IR2Opcode
	.type IR2Opcode, @function

IR2Opcode:

	// *** Basic block 0

	.global TypeIsUnsigned
	.global TypeIsLong
	.global TypeIsLongLong
	.global TypeIsShort
	.local ComparisonOpcode
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
	lw          s2, 20(s1)
	ld          s3, 80(s1)
	sub         t0, s3, x0
	snez        s4, t0
	beq         s3, x0, .IR2Opcode_label_82

	// *** Basic block 1

	mv          a0, s3
	call        TypeIsUnsigned

	// *** Basic block 3

.IR2Opcode_label_82:
	beq         s3, x0, .IR2Opcode_label_98

	// *** Basic block 4

	mv          a0, s3
	call        TypeIsLong

	// *** Basic block 5

	mv          s5, a0
	bnez        a0, .IR2Opcode_label_97

	// *** Basic block 6

	mv          a0, s3
	call        TypeIsLongLong

	// *** Basic block 8

.IR2Opcode_label_97:

	// *** Basic block 9

.IR2Opcode_label_98:
	mv          s6, s4
	beq         s3, x0, .IR2Opcode_label_107

	// *** Basic block 10

	mv          a0, s3
	call        TypeIsShort

	// *** Basic block 12

.IR2Opcode_label_107:
	li          t0, 1		// 0x1 ASCII \x1
	blt         s2, t0, .IR2Opcode_label_501

	// *** Basic block 13

	li          t0, 115		// 0x73 ASCII 's'
	blt         t0, s2, .IR2Opcode_label_501

	// *** Basic block 14

	addi        t0, s2, -1
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 15

	j           .IR2Opcode_label_497

	// *** Basic block 16

	j           .IR2Opcode_label_501

	// *** Basic block 17

	j           .IR2Opcode_label_501

	// *** Basic block 18

	j           .IR2Opcode_label_501

	// *** Basic block 19

	j           .IR2Opcode_label_501

	// *** Basic block 20

	j           .IR2Opcode_label_501

	// *** Basic block 21

	j           .IR2Opcode_label_501

	// *** Basic block 22

	j           .IR2Opcode_label_501

	// *** Basic block 23

	j           .IR2Opcode_label_445

	// *** Basic block 24

	j           .IR2Opcode_label_459

	// *** Basic block 25

	j           .IR2Opcode_label_463

	// *** Basic block 26

	j           .IR2Opcode_label_467

	// *** Basic block 27

	j           .IR2Opcode_label_471

	// *** Basic block 28

	j           .IR2Opcode_label_485

	// *** Basic block 29

	j           .IR2Opcode_label_489

	// *** Basic block 30

	j           .IR2Opcode_label_493

	// *** Basic block 31

	j           .IR2Opcode_label_501

	// *** Basic block 32

	j           .IR2Opcode_label_501

	// *** Basic block 33

	j           .IR2Opcode_label_501

	// *** Basic block 34

	j           .IR2Opcode_label_501

	// *** Basic block 35

	j           .IR2Opcode_label_501

	// *** Basic block 36

	j           .IR2Opcode_label_501

	// *** Basic block 37

	j           .IR2Opcode_label_501

	// *** Basic block 38

	j           .IR2Opcode_label_501

	// *** Basic block 39

	j           .IR2Opcode_label_501

	// *** Basic block 40

	j           .IR2Opcode_label_501

	// *** Basic block 41

	j           .IR2Opcode_label_501

	// *** Basic block 42

	j           .IR2Opcode_label_501

	// *** Basic block 43

	j           .IR2Opcode_label_501

	// *** Basic block 44

	j           .IR2Opcode_label_501

	// *** Basic block 45

	j           .IR2Opcode_label_501

	// *** Basic block 46

	j           .IR2Opcode_label_501

	// *** Basic block 47

	j           .IR2Opcode_label_501

	// *** Basic block 48

	j           .IR2Opcode_label_501

	// *** Basic block 49

	j           .IR2Opcode_label_501

	// *** Basic block 50

	j           .IR2Opcode_label_501

	// *** Basic block 51

	j           .IR2Opcode_label_238

	// *** Basic block 52

	j           .IR2Opcode_label_244

	// *** Basic block 53

	j           .IR2Opcode_label_248

	// *** Basic block 54

	j           .IR2Opcode_label_252

	// *** Basic block 55

	j           .IR2Opcode_label_256

	// *** Basic block 56

	j           .IR2Opcode_label_260

	// *** Basic block 57

	j           .IR2Opcode_label_264

	// *** Basic block 58

	j           .IR2Opcode_label_268

	// *** Basic block 59

	j           .IR2Opcode_label_272

	// *** Basic block 60

	j           .IR2Opcode_label_276

	// *** Basic block 61

	j           .IR2Opcode_label_280

	// *** Basic block 62

	j           .IR2Opcode_label_284

	// *** Basic block 63

	j           .IR2Opcode_label_295

	// *** Basic block 64

	j           .IR2Opcode_label_299

	// *** Basic block 65

	j           .IR2Opcode_label_303

	// *** Basic block 66

	j           .IR2Opcode_label_314

	// *** Basic block 67

	j           .IR2Opcode_label_318

	// *** Basic block 68

	j           .IR2Opcode_label_322

	// *** Basic block 69

	j           .IR2Opcode_label_326

	// *** Basic block 70

	j           .IR2Opcode_label_330

	// *** Basic block 71

	j           .IR2Opcode_label_334

	// *** Basic block 72

	j           .IR2Opcode_label_338

	// *** Basic block 73

	j           .IR2Opcode_label_342

	// *** Basic block 74

	j           .IR2Opcode_label_346

	// *** Basic block 75

	j           .IR2Opcode_label_350

	// *** Basic block 76

	j           .IR2Opcode_label_354

	// *** Basic block 77

	j           .IR2Opcode_label_358

	// *** Basic block 78

	j           .IR2Opcode_label_362

	// *** Basic block 79

	j           .IR2Opcode_label_363

	// *** Basic block 80

	j           .IR2Opcode_label_364

	// *** Basic block 81

	j           .IR2Opcode_label_365

	// *** Basic block 82

	j           .IR2Opcode_label_366

	// *** Basic block 83

	j           .IR2Opcode_label_367

	// *** Basic block 84

	j           .IR2Opcode_label_368

	// *** Basic block 85

	j           .IR2Opcode_label_369

	// *** Basic block 86

	j           .IR2Opcode_label_370

	// *** Basic block 87

	j           .IR2Opcode_label_371

	// *** Basic block 88

	j           .IR2Opcode_label_372

	// *** Basic block 89

	j           .IR2Opcode_label_373

	// *** Basic block 90

	j           .IR2Opcode_label_374

	// *** Basic block 91

	j           .IR2Opcode_label_375

	// *** Basic block 92

	j           .IR2Opcode_label_376

	// *** Basic block 93

	j           .IR2Opcode_label_377

	// *** Basic block 94

	j           .IR2Opcode_label_378

	// *** Basic block 95

	j           .IR2Opcode_label_379

	// *** Basic block 96

	j           .IR2Opcode_label_380

	// *** Basic block 97

	j           .IR2Opcode_label_381

	// *** Basic block 98

	j           .IR2Opcode_label_382

	// *** Basic block 99

	j           .IR2Opcode_label_383

	// *** Basic block 100

	j           .IR2Opcode_label_384

	// *** Basic block 101

	j           .IR2Opcode_label_385

	// *** Basic block 102

	j           .IR2Opcode_label_501

	// *** Basic block 103

	j           .IR2Opcode_label_501

	// *** Basic block 104

	j           .IR2Opcode_label_501

	// *** Basic block 105

	j           .IR2Opcode_label_501

	// *** Basic block 106

	j           .IR2Opcode_label_501

	// *** Basic block 107

	j           .IR2Opcode_label_501

	// *** Basic block 108

	j           .IR2Opcode_label_501

	// *** Basic block 109

	j           .IR2Opcode_label_501

	// *** Basic block 110

	j           .IR2Opcode_label_501

	// *** Basic block 111

	j           .IR2Opcode_label_501

	// *** Basic block 112

	j           .IR2Opcode_label_501

	// *** Basic block 113

	j           .IR2Opcode_label_501

	// *** Basic block 114

	j           .IR2Opcode_label_501

	// *** Basic block 115

	j           .IR2Opcode_label_501

	// *** Basic block 116

	j           .IR2Opcode_label_501

	// *** Basic block 117

	j           .IR2Opcode_label_501

	// *** Basic block 118

	j           .IR2Opcode_label_501

	// *** Basic block 119

	j           .IR2Opcode_label_501

	// *** Basic block 120

	j           .IR2Opcode_label_501

	// *** Basic block 121

	j           .IR2Opcode_label_501

	// *** Basic block 122

	j           .IR2Opcode_label_501

	// *** Basic block 123

	j           .IR2Opcode_label_501

	// *** Basic block 124

	j           .IR2Opcode_label_393

	// *** Basic block 125

	j           .IR2Opcode_label_404

	// *** Basic block 126

	j           .IR2Opcode_label_415

	// *** Basic block 127

	j           .IR2Opcode_label_419

	// *** Basic block 128

	j           .IR2Opcode_label_423

	// *** Basic block 129

	j           .IR2Opcode_label_434

	// *** Basic block 130

.IR2Opcode_label_238:
	li          a0, 84		// 0x54 ASCII 'T'

	// *** Basic block 131

.IR2Opcode_label_241:
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

	// *** Basic block 132

.IR2Opcode_label_244:
	li          a0, 85		// 0x55 ASCII 'U'
	j           .IR2Opcode_label_241

	// *** Basic block 133

.IR2Opcode_label_248:
	li          a0, 86		// 0x56 ASCII 'V'
	j           .IR2Opcode_label_241

	// *** Basic block 134

.IR2Opcode_label_252:
	li          a0, 84		// 0x54 ASCII 'T'
	j           .IR2Opcode_label_241

	// *** Basic block 135

.IR2Opcode_label_256:
	li          a0, 89		// 0x59 ASCII 'Y'
	j           .IR2Opcode_label_241

	// *** Basic block 136

.IR2Opcode_label_260:
	li          a0, 90		// 0x5a ASCII 'Z'
	j           .IR2Opcode_label_241

	// *** Basic block 137

.IR2Opcode_label_264:
	li          a0, 91		// 0x5b ASCII '['
	j           .IR2Opcode_label_241

	// *** Basic block 138

.IR2Opcode_label_268:
	li          a0, 89		// 0x59 ASCII 'Y'
	j           .IR2Opcode_label_241

	// *** Basic block 139

.IR2Opcode_label_272:
	li          a0, 93		// 0x5d ASCII ']'
	j           .IR2Opcode_label_241

	// *** Basic block 140

.IR2Opcode_label_276:
	li          a0, 94		// 0x5e ASCII '^'
	j           .IR2Opcode_label_241

	// *** Basic block 141

.IR2Opcode_label_280:
	li          a0, 95		// 0x5f ASCII '_'
	j           .IR2Opcode_label_241

	// *** Basic block 142

.IR2Opcode_label_284:
	beqz        s4, .IR2Opcode_label_290

	// *** Basic block 143

	li          a0, 98		// 0x62 ASCII 'b'
	j           .IR2Opcode_label_292

	// *** Basic block 144

.IR2Opcode_label_290:
	li          a0, 97		// 0x61 ASCII 'a'

	// *** Basic block 145

.IR2Opcode_label_292:
	j           .IR2Opcode_label_241

	// *** Basic block 146

.IR2Opcode_label_295:
	li          a0, 99		// 0x63 ASCII 'c'
	j           .IR2Opcode_label_241

	// *** Basic block 147

.IR2Opcode_label_299:
	li          a0, 100		// 0x64 ASCII 'd'
	j           .IR2Opcode_label_241

	// *** Basic block 148

.IR2Opcode_label_303:
	beqz        s4, .IR2Opcode_label_309

	// *** Basic block 149

	li          a0, 102		// 0x66 ASCII 'f'
	j           .IR2Opcode_label_311

	// *** Basic block 150

.IR2Opcode_label_309:
	li          a0, 101		// 0x65 ASCII 'e'

	// *** Basic block 151

.IR2Opcode_label_311:
	j           .IR2Opcode_label_241

	// *** Basic block 152

.IR2Opcode_label_314:
	li          a0, 103		// 0x67 ASCII 'g'
	j           .IR2Opcode_label_241

	// *** Basic block 153

.IR2Opcode_label_318:
	li          a0, 104		// 0x68 ASCII 'h'
	j           .IR2Opcode_label_241

	// *** Basic block 154

.IR2Opcode_label_322:
	li          a0, 105		// 0x69 ASCII 'i'
	j           .IR2Opcode_label_241

	// *** Basic block 155

.IR2Opcode_label_326:
	li          a0, 106		// 0x6a ASCII 'j'
	j           .IR2Opcode_label_241

	// *** Basic block 156

.IR2Opcode_label_330:
	li          a0, 107		// 0x6b ASCII 'k'
	j           .IR2Opcode_label_241

	// *** Basic block 157

.IR2Opcode_label_334:
	li          a0, 108		// 0x6c ASCII 'l'
	j           .IR2Opcode_label_241

	// *** Basic block 158

.IR2Opcode_label_338:
	li          a0, 109		// 0x6d ASCII 'm'
	j           .IR2Opcode_label_241

	// *** Basic block 159

.IR2Opcode_label_342:
	li          a0, 109		// 0x6d ASCII 'm'
	j           .IR2Opcode_label_241

	// *** Basic block 160

.IR2Opcode_label_346:
	li          a0, 110		// 0x6e ASCII 'n'
	j           .IR2Opcode_label_241

	// *** Basic block 161

.IR2Opcode_label_350:
	li          a0, 111		// 0x6f ASCII 'o'
	j           .IR2Opcode_label_241

	// *** Basic block 162

.IR2Opcode_label_354:
	li          a0, 112		// 0x70 ASCII 'p'
	j           .IR2Opcode_label_241

	// *** Basic block 163

.IR2Opcode_label_358:
	li          a0, 113		// 0x71 ASCII 'q'
	j           .IR2Opcode_label_241

	// *** Basic block 164

.IR2Opcode_label_362:

	// *** Basic block 165

.IR2Opcode_label_363:

	// *** Basic block 166

.IR2Opcode_label_364:

	// *** Basic block 167

.IR2Opcode_label_365:

	// *** Basic block 168

.IR2Opcode_label_366:

	// *** Basic block 169

.IR2Opcode_label_367:

	// *** Basic block 170

.IR2Opcode_label_368:

	// *** Basic block 171

.IR2Opcode_label_369:

	// *** Basic block 172

.IR2Opcode_label_370:

	// *** Basic block 173

.IR2Opcode_label_371:

	// *** Basic block 174

.IR2Opcode_label_372:

	// *** Basic block 175

.IR2Opcode_label_373:

	// *** Basic block 176

.IR2Opcode_label_374:

	// *** Basic block 177

.IR2Opcode_label_375:

	// *** Basic block 178

.IR2Opcode_label_376:

	// *** Basic block 179

.IR2Opcode_label_377:

	// *** Basic block 180

.IR2Opcode_label_378:

	// *** Basic block 181

.IR2Opcode_label_379:

	// *** Basic block 182

.IR2Opcode_label_380:

	// *** Basic block 183

.IR2Opcode_label_381:

	// *** Basic block 184

.IR2Opcode_label_382:

	// *** Basic block 185

.IR2Opcode_label_383:

	// *** Basic block 186

.IR2Opcode_label_384:

	// *** Basic block 187

.IR2Opcode_label_385:
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
	j           ComparisonOpcode

	// *** Basic block 189

.IR2Opcode_label_393:
	beqz        s4, .IR2Opcode_label_399

	// *** Basic block 190

	li          a0, 172		// 0xac ASCII \xac
	j           .IR2Opcode_label_401

	// *** Basic block 191

.IR2Opcode_label_399:
	li          a0, 170		// 0xaa ASCII \xaa

	// *** Basic block 192

.IR2Opcode_label_401:
	j           .IR2Opcode_label_241

	// *** Basic block 193

.IR2Opcode_label_404:
	beqz        s4, .IR2Opcode_label_410

	// *** Basic block 194

	li          a0, 173		// 0xad ASCII \xad
	j           .IR2Opcode_label_412

	// *** Basic block 195

.IR2Opcode_label_410:
	li          a0, 171		// 0xab ASCII \xab

	// *** Basic block 196

.IR2Opcode_label_412:
	j           .IR2Opcode_label_241

	// *** Basic block 197

.IR2Opcode_label_415:
	li          a0, 174		// 0xae ASCII \xae
	j           .IR2Opcode_label_241

	// *** Basic block 198

.IR2Opcode_label_419:
	li          a0, 175		// 0xaf ASCII \xaf
	j           .IR2Opcode_label_241

	// *** Basic block 199

.IR2Opcode_label_423:
	beqz        s4, .IR2Opcode_label_429

	// *** Basic block 200

	li          a0, 178		// 0xb2 ASCII \xb2
	j           .IR2Opcode_label_431

	// *** Basic block 201

.IR2Opcode_label_429:
	li          a0, 176		// 0xb0 ASCII \xb0

	// *** Basic block 202

.IR2Opcode_label_431:
	j           .IR2Opcode_label_241

	// *** Basic block 203

.IR2Opcode_label_434:
	beqz        s4, .IR2Opcode_label_440

	// *** Basic block 204

	li          a0, 179		// 0xb3 ASCII \xb3
	j           .IR2Opcode_label_442

	// *** Basic block 205

.IR2Opcode_label_440:
	li          a0, 177		// 0xb1 ASCII \xb1

	// *** Basic block 206

.IR2Opcode_label_442:
	j           .IR2Opcode_label_241

	// *** Basic block 207

.IR2Opcode_label_445:
	beqz        s6, .IR2Opcode_label_450

	// *** Basic block 208

	li          a0, 36		// 0x24 ASCII '$'
	j           .IR2Opcode_label_241

	// *** Basic block 209

.IR2Opcode_label_450:
	beqz        s5, .IR2Opcode_label_455

	// *** Basic block 210

	li          a0, 37		// 0x25 ASCII '%'
	j           .IR2Opcode_label_241

	// *** Basic block 211

.IR2Opcode_label_455:
	li          a0, 11		// 0xb ASCII \xb
	j           .IR2Opcode_label_241

	// *** Basic block 212

.IR2Opcode_label_459:
	li          a0, 12		// 0xc ASCII \xc
	j           .IR2Opcode_label_241

	// *** Basic block 213

.IR2Opcode_label_463:
	li          a0, 13		// 0xd ASCII \xd
	j           .IR2Opcode_label_241

	// *** Basic block 214

.IR2Opcode_label_467:
	li          a0, 36		// 0x24 ASCII '$'
	j           .IR2Opcode_label_241

	// *** Basic block 215

.IR2Opcode_label_471:
	beqz        s6, .IR2Opcode_label_476

	// *** Basic block 216

	li          a0, 38		// 0x26 ASCII '&'
	j           .IR2Opcode_label_241

	// *** Basic block 217

.IR2Opcode_label_476:
	beqz        s5, .IR2Opcode_label_481

	// *** Basic block 218

	li          a0, 39		// 0x27 ASCII '''
	j           .IR2Opcode_label_241

	// *** Basic block 219

.IR2Opcode_label_481:
	li          a0, 18		// 0x12 ASCII \x12
	j           .IR2Opcode_label_241

	// *** Basic block 220

.IR2Opcode_label_485:
	li          a0, 19		// 0x13 ASCII \x13
	j           .IR2Opcode_label_241

	// *** Basic block 221

.IR2Opcode_label_489:
	li          a0, 20		// 0x14 ASCII \x14
	j           .IR2Opcode_label_241

	// *** Basic block 222

.IR2Opcode_label_493:
	li          a0, 38		// 0x26 ASCII '&'
	j           .IR2Opcode_label_241

	// *** Basic block 223

.IR2Opcode_label_497:
	li          a0, 4		// 0x4 ASCII \x4
	j           .IR2Opcode_label_241

	// *** Basic block 224

.IR2Opcode_label_501:
	lla         a0, .str.166
	lla         a1, .str.167
	lla         a3, .str.168
	li          t0, 889		// 0x379
	mv          a2, t0
	call        printf

	// *** Basic block 225

	call        abort

	// *** Basic block 226

	mv          a0, x0
	j           .IR2Opcode_label_241
.func_end_IR2Opcode:
	.size IR2Opcode, .func_end_IR2Opcode-IR2Opcode

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
	.local GetLoweredNode
	sd          a0, -0(s0)	// Spilled @669
	addi sp, sp, -128
	// Saved return address (offset 120) and frame pointer (offset 112)
	sd ra, 120(sp)
	sd s0, 112(sp)
	addi s0, sp, 128
	// Local vars at offset -16(s0)
	// Spilled register region: 24 bytes at -40(s0) to -16(s0)
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
	mv          s1, a2
	sd          s1, -24(s0)	// Spilled @46
	mv          s2, a1
	mv          s3, a0
	mv          s4, x0
	li          t0, 84		// 0x54 ASCII 'T'
	beq         s1, t0, .ReduceExpressionStrength_label_85

	// *** Basic block 1

	li          t0, 89		// 0x59 ASCII 'Y'
	beq         s1, t0, .ReduceExpressionStrength_label_236

	// *** Basic block 2

	li          t0, 93		// 0x5d ASCII ']'
	beq         s1, t0, .ReduceExpressionStrength_label_351

	// *** Basic block 3

	li          t0, 97		// 0x61 ASCII 'a'
	beq         s1, t0, .ReduceExpressionStrength_label_478

	// *** Basic block 4

	li          t0, 98		// 0x62 ASCII 'b'
	beq         s1, t0, .ReduceExpressionStrength_label_573

	// *** Basic block 5

.ReduceExpressionStrength_label_83:
	j           .ReduceExpressionStrength_label_668

	// *** Basic block 6

.ReduceExpressionStrength_label_85:
	addi        t0, s2, 24
	ld          t0, 8(t0)
	li          s1, 2		// 0x2 ASCII \x2
	sd          s1, -40(s0)	// Spilled @92
	bne         t0, s1, .ReduceExpressionStrength_label_95

	// *** Basic block 7

	j           .ReduceExpressionStrength_label_111

	// *** Basic block 8

.ReduceExpressionStrength_label_95:
	lla         a0, .str.169
	lla         a1, .str.170
	lla         a3, .str.171
	li          t0, 908		// 0x38c
	mv          a2, t0
	call        printf

	// *** Basic block 9

	call        abort

	// *** Basic block 10

.ReduceExpressionStrength_label_111:
	ld          t0, 24(s2)
	ld          s5, 0(t0)
	ld          s8, 8(t0)
	mv          a0, s5
	call        IRIsConst

	// *** Basic block 11

	mv          s9, a0
	beqz        a0, .ReduceExpressionStrength_label_128

	// *** Basic block 12

	mv          a0, s8
	call        IRIsConst

	// *** Basic block 13

	mv          s9, a0

	// *** Basic block 14

.ReduceExpressionStrength_label_128:
	beqz        s9, .ReduceExpressionStrength_label_154

	// *** Basic block 15

	ld          s11, 136(s5)
	ld          s1, 136(s8)
	li          t0, 14		// 0xe ASCII \xe
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 16

	mv          s4, a0
	add         a3, s11, s1
	ld          a2, -40(s0)	// Spilled @92
	mv          a1, x0
	mv          a0, s3
	call        GetIntConstant

	// *** Basic block 17

	sd          a0, 40(s4)
	j           .ReduceExpressionStrength_label_668

	// *** Basic block 18

.ReduceExpressionStrength_label_154:
	mv          a0, s8
	call        IRIsConst

	// *** Basic block 19

	beqz        a0, .ReduceExpressionStrength_label_194

	// *** Basic block 20

	ld          s1, 136(s8)
	bnez        s1, .ReduceExpressionStrength_label_175

	// *** Basic block 21

	li          t0, 11		// 0xb ASCII \xb
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 22

	mv          s4, a0
	mv          a0, s5
	call        GetLoweredNode

	// *** Basic block 23

	sd          a0, 40(s4)
	j           .ReduceExpressionStrength_label_192

	// *** Basic block 24

.ReduceExpressionStrength_label_175:
	li          t0, 87		// 0x57 ASCII 'W'
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 25

	mv          s4, a0
	mv          a0, s5
	call        GetLoweredNode

	// *** Basic block 26

	sd          a0, 40(s4)
	addi        s1, s4, 40
	mv          a0, s8
	call        GetLoweredNode

	// *** Basic block 27

	sd          a0, 8(s1)

	// *** Basic block 28

.ReduceExpressionStrength_label_192:
	j           .ReduceExpressionStrength_label_234

	// *** Basic block 29

.ReduceExpressionStrength_label_194:
	mv          a0, s5
	call        IRIsConst

	// *** Basic block 30

	beqz        a0, .ReduceExpressionStrength_label_233

	// *** Basic block 31

	ld          s1, 136(s5)
	bnez        s1, .ReduceExpressionStrength_label_215

	// *** Basic block 32

	li          t0, 11		// 0xb ASCII \xb
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 33

	mv          s4, a0
	mv          a0, s5
	call        GetLoweredNode

	// *** Basic block 34

	sd          a0, 40(s4)
	j           .ReduceExpressionStrength_label_232

	// *** Basic block 35

.ReduceExpressionStrength_label_215:
	li          t0, 87		// 0x57 ASCII 'W'
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 36

	mv          s4, a0
	mv          a0, s8
	call        GetLoweredNode

	// *** Basic block 37

	sd          a0, 40(s4)
	addi        s1, s4, 40
	mv          a0, s5
	call        GetLoweredNode

	// *** Basic block 38

	sd          a0, 8(s1)

	// *** Basic block 39

.ReduceExpressionStrength_label_232:

	// *** Basic block 40

.ReduceExpressionStrength_label_233:

	// *** Basic block 41

.ReduceExpressionStrength_label_234:
	j           .ReduceExpressionStrength_label_668

	// *** Basic block 42

.ReduceExpressionStrength_label_236:
	addi        t0, s2, 24
	ld          t0, 8(t0)
	li          s5, 2		// 0x2 ASCII \x2
	bne         t0, s5, .ReduceExpressionStrength_label_245

	// *** Basic block 43

	j           .ReduceExpressionStrength_label_260

	// *** Basic block 44

.ReduceExpressionStrength_label_245:
	lla         a0, .str.172
	lla         a1, .str.173
	lla         a3, .str.174
	li          t0, 951		// 0x3b7
	mv          a2, t0
	call        printf

	// *** Basic block 45

	call        abort

	// *** Basic block 46

.ReduceExpressionStrength_label_260:
	ld          t0, 24(s2)
	ld          s8, 0(t0)
	ld          s9, 8(t0)
	mv          a0, s8
	call        IRIsConst

	// *** Basic block 47

	mv          s11, a0
	beqz        a0, .ReduceExpressionStrength_label_277

	// *** Basic block 48

	mv          a0, s9
	call        IRIsConst

	// *** Basic block 49

	mv          s11, a0

	// *** Basic block 50

.ReduceExpressionStrength_label_277:
	beqz        s11, .ReduceExpressionStrength_label_303

	// *** Basic block 51

	ld          a0, 136(s8)
	ld          a0, 136(s9)
	li          t0, 14		// 0xe ASCII \xe
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 52

	mv          s4, a0
	ld          t0, -32(s0)	// Spilled @281
	sub         a3, t0, a0
	sd          a0, -32(s0)	// Spilled @281
	mv          a2, s5
	mv          a1, x0
	mv          a0, s3
	call        GetIntConstant

	// *** Basic block 53

	sd          a0, 40(s4)
	j           .ReduceExpressionStrength_label_668

	// *** Basic block 54

.ReduceExpressionStrength_label_303:
	mv          a0, s9
	call        IRIsConst

	// *** Basic block 55

	beqz        a0, .ReduceExpressionStrength_label_349

	// *** Basic block 56

	ld          s1, 136(s9)
	bnez        s1, .ReduceExpressionStrength_label_324

	// *** Basic block 57

	li          t0, 11		// 0xb ASCII \xb
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 58

	mv          s4, a0
	mv          a0, s8
	call        GetLoweredNode

	// *** Basic block 59

	sd          a0, 40(s4)
	j           .ReduceExpressionStrength_label_348

	// *** Basic block 60

.ReduceExpressionStrength_label_324:
	li          t0, 87		// 0x57 ASCII 'W'
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 61

	mv          s4, a0
	mv          a0, s8
	call        GetLoweredNode

	// *** Basic block 62

	sd          a0, 40(s4)
	addi        s8, s4, 40
	neg         a3, s1
	mv          a2, s5
	mv          a1, x0
	mv          a0, s3
	call        GetIntConstant

	// *** Basic block 63

	sd          a0, 8(s8)

	// *** Basic block 64

.ReduceExpressionStrength_label_348:

	// *** Basic block 65

.ReduceExpressionStrength_label_349:
	j           .ReduceExpressionStrength_label_668

	// *** Basic block 66

.ReduceExpressionStrength_label_351:
	addi        t0, s2, 24
	ld          t0, 8(t0)
	li          s5, 2		// 0x2 ASCII \x2
	bne         t0, s5, .ReduceExpressionStrength_label_360

	// *** Basic block 67

	j           .ReduceExpressionStrength_label_375

	// *** Basic block 68

.ReduceExpressionStrength_label_360:
	lla         a0, .str.175
	lla         a1, .str.176
	lla         a3, .str.177
	li          t0, 980		// 0x3d4
	mv          a2, t0
	call        printf

	// *** Basic block 69

	call        abort

	// *** Basic block 70

.ReduceExpressionStrength_label_375:
	ld          t0, 24(s2)
	ld          s6, 0(t0)
	ld          s7, 8(t0)
	mv          a0, s6
	call        IRIsConst

	// *** Basic block 71

	mv          s8, a0
	bnez        a0, .ReduceExpressionStrength_label_392

	// *** Basic block 72

	mv          a0, s7
	call        IRIsConst

	// *** Basic block 73

	mv          s8, a0

	// *** Basic block 74

.ReduceExpressionStrength_label_392:
	beqz        s8, .ReduceExpressionStrength_label_476

	// *** Basic block 75

	mv          a0, s6
	call        IRIsConst

	// *** Basic block 76

	mv          s9, a0
	beqz        a0, .ReduceExpressionStrength_label_404

	// *** Basic block 77

	mv          a0, s7
	call        IRIsConst

	// *** Basic block 78

	mv          s9, a0

	// *** Basic block 79

.ReduceExpressionStrength_label_404:
	beqz        s9, .ReduceExpressionStrength_label_430

	// *** Basic block 80

	ld          s10, 136(s6)
	ld          s11, 136(s7)
	li          t0, 14		// 0xe ASCII \xe
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 81

	mv          s4, a0
	mul         a3, s10, s11
	mv          a2, s5
	mv          a1, x0
	mv          a0, s3
	call        GetIntConstant

	// *** Basic block 82

	sd          a0, 40(s4)
	j           .ReduceExpressionStrength_label_475

	// *** Basic block 83

.ReduceExpressionStrength_label_430:
	mv          a0, s6
	call        IRIsConst

	// *** Basic block 84

	beqz        a0, .ReduceExpressionStrength_label_439

	// *** Basic block 85

	mv          s10, s6
	mv          s6, s7
	mv          s7, s10

	// *** Basic block 86

.ReduceExpressionStrength_label_439:
	ld          s11, 136(s7)
	bnez        s11, .ReduceExpressionStrength_label_457

	// *** Basic block 87

	mv          a3, x0
	mv          a2, s5
	mv          a1, x0
	mv          a0, s3
	call        GetIntConstant

	// *** Basic block 88

	mv          s4, a0
	j           .ReduceExpressionStrength_label_474

	// *** Basic block 89

.ReduceExpressionStrength_label_457:
	li          t0, 1		// 0x1 ASCII \x1
	bne         s11, t0, .ReduceExpressionStrength_label_473

	// *** Basic block 90

	li          t0, 11		// 0xb ASCII \xb
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 91

	mv          s4, a0
	mv          a0, s6
	call        GetLoweredNode

	// *** Basic block 92

	sd          a0, 40(s4)

	// *** Basic block 93

.ReduceExpressionStrength_label_473:

	// *** Basic block 94

.ReduceExpressionStrength_label_474:

	// *** Basic block 95

.ReduceExpressionStrength_label_475:

	// *** Basic block 96

.ReduceExpressionStrength_label_476:
	j           .ReduceExpressionStrength_label_668

	// *** Basic block 97

.ReduceExpressionStrength_label_478:
	addi        t0, s2, 24
	ld          t0, 8(t0)
	li          s5, 2		// 0x2 ASCII \x2
	bne         t0, s5, .ReduceExpressionStrength_label_487

	// *** Basic block 98

	j           .ReduceExpressionStrength_label_502

	// *** Basic block 99

.ReduceExpressionStrength_label_487:
	lla         a0, .str.178
	lla         a1, .str.179
	lla         a3, .str.180
	li          t0, 1011		// 0x3f3
	mv          a2, t0
	call        printf

	// *** Basic block 100

	call        abort

	// *** Basic block 101

.ReduceExpressionStrength_label_502:
	ld          t0, 24(s2)
	ld          s6, 0(t0)
	ld          s7, 8(t0)
	mv          a0, s6
	call        IRIsConst

	// *** Basic block 102

	mv          s8, a0
	beqz        a0, .ReduceExpressionStrength_label_519

	// *** Basic block 103

	mv          a0, s7
	call        IRIsConst

	// *** Basic block 104

	mv          s8, a0

	// *** Basic block 105

.ReduceExpressionStrength_label_519:
	beqz        s8, .ReduceExpressionStrength_label_548

	// *** Basic block 106

	ld          s9, 136(s6)
	ld          s10, 136(s7)
	beqz        s10, .ReduceExpressionStrength_label_547

	// *** Basic block 107

	li          t0, 14		// 0xe ASCII \xe
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 108

	mv          s4, a0
	div         a3, s9, s10
	mv          a2, s5
	mv          a1, x0
	mv          a0, s3
	call        GetIntConstant

	// *** Basic block 109

	sd          a0, 40(s4)
	j           .ReduceExpressionStrength_label_668

	// *** Basic block 110

.ReduceExpressionStrength_label_547:

	// *** Basic block 111

.ReduceExpressionStrength_label_548:
	mv          a0, s7
	call        IRIsConst

	// *** Basic block 112

	beqz        a0, .ReduceExpressionStrength_label_571

	// *** Basic block 113

	ld          s5, 136(s7)
	li          t0, 1		// 0x1 ASCII \x1
	bne         s5, t0, .ReduceExpressionStrength_label_570

	// *** Basic block 114

	li          t0, 11		// 0xb ASCII \xb
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 115

	mv          s4, a0
	mv          a0, s6
	call        GetLoweredNode

	// *** Basic block 116

	sd          a0, 40(s4)

	// *** Basic block 117

.ReduceExpressionStrength_label_570:

	// *** Basic block 118

.ReduceExpressionStrength_label_571:
	j           .ReduceExpressionStrength_label_668

	// *** Basic block 119

.ReduceExpressionStrength_label_573:
	addi        t0, s2, 24
	ld          t0, 8(t0)
	li          s1, 2		// 0x2 ASCII \x2
	bne         t0, s1, .ReduceExpressionStrength_label_582

	// *** Basic block 120

	j           .ReduceExpressionStrength_label_597

	// *** Basic block 121

.ReduceExpressionStrength_label_582:
	lla         a0, .str.181
	lla         a1, .str.182
	lla         a3, .str.183
	li          t0, 1037		// 0x40d
	mv          a2, t0
	call        printf

	// *** Basic block 122

	call        abort

	// *** Basic block 123

.ReduceExpressionStrength_label_597:
	ld          t0, 24(s2)
	ld          s5, 0(t0)
	ld          s6, 8(t0)
	mv          a0, s5
	call        IRIsConst

	// *** Basic block 124

	mv          s7, a0
	beqz        a0, .ReduceExpressionStrength_label_614

	// *** Basic block 125

	mv          a0, s6
	call        IRIsConst

	// *** Basic block 126

	mv          s7, a0

	// *** Basic block 127

.ReduceExpressionStrength_label_614:
	beqz        s7, .ReduceExpressionStrength_label_643

	// *** Basic block 128

	ld          s8, 136(s5)
	ld          s9, 136(s6)
	beqz        s9, .ReduceExpressionStrength_label_642

	// *** Basic block 129

	li          t0, 14		// 0xe ASCII \xe
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 130

	mv          s4, a0
	div         a3, s8, s9
	mv          a2, s1
	mv          a1, x0
	mv          a0, s3
	call        GetIntConstant

	// *** Basic block 131

	sd          a0, 40(s4)
	j           .ReduceExpressionStrength_label_668

	// *** Basic block 132

.ReduceExpressionStrength_label_642:

	// *** Basic block 133

.ReduceExpressionStrength_label_643:
	mv          a0, s6
	call        IRIsConst

	// *** Basic block 134

	beqz        a0, .ReduceExpressionStrength_label_666

	// *** Basic block 135

	ld          s1, 136(s6)
	li          t0, 1		// 0x1 ASCII \x1
	bne         s1, t0, .ReduceExpressionStrength_label_665

	// *** Basic block 136

	li          t0, 11		// 0xb ASCII \xb
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 137

	mv          s4, a0
	mv          a0, s5
	call        GetLoweredNode

	// *** Basic block 138

	sd          a0, 40(s4)

	// *** Basic block 139

.ReduceExpressionStrength_label_665:

	// *** Basic block 140

.ReduceExpressionStrength_label_666:
	j           .ReduceExpressionStrength_label_668

	// *** Basic block 141

.ReduceExpressionStrength_label_668:
	mv          a0, s4

	// *** Basic block 142

.ReduceExpressionStrength_label_671:
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

	.local IR2Opcode
	.global printf
	.global abort
	.local ReduceExpressionStrength
	.local NewInstruction
	.local GetLoweredNode
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
	beq         a0, x0, .LowerExpression_label_39

	// *** Basic block 1

.LowerExpression_label_36:
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

.LowerExpression_label_39:
	mv          a0, s1
	call        IR2Opcode

	// *** Basic block 3

	mv          s3, a0
	addi        t0, s1, 24
	ld          s4, 8(t0)
	li          t0, 2		// 0x2 ASCII \x2
	bge         s4, t0, .LowerExpression_label_55

	// *** Basic block 4

	ld          t0, 24(s1)
	j           .LowerExpression_label_72

	// *** Basic block 5

.LowerExpression_label_55:
	lla         a0, .str.184
	lla         a1, .str.185
	lla         a3, .str.186
	li          t0, 1073		// 0x431
	mv          a2, t0
	call        printf

	// *** Basic block 6

	call        abort

	// *** Basic block 7

.LowerExpression_label_72:
	mv          a2, s3
	mv          a1, s1
	mv          a0, s2
	call        ReduceExpressionStrength

	// *** Basic block 8

	mv          s5, a0
	bne         s5, x0, .LowerExpression_label_110

	// *** Basic block 9

	mv          a0, s3
	call        NewInstruction

	// *** Basic block 10

	mv          s5, a0
	mv          s6, x0
	bge         x0, s4, .LowerExpression_label_109

	// *** Basic block 11

.LowerExpression_label_93:
	slli        t0, s6, 3
	add         t1, t0, t0
	ld          s3, 0(t1)
	addi        t1, s5, 40
	add         s7, t1, t0
	mv          a0, s3
	call        GetLoweredNode

	// *** Basic block 12

	sd          a0, 0(s7)

	// *** Basic block 13

.LowerExpression_label_105:
	addi        s6, s6, 1
	bge         s6, s4, .LowerExpression_label_93

	// *** Basic block 14

.LowerExpression_label_109:

	// *** Basic block 15

.LowerExpression_label_110:
	mv          a0, s5
	call        TargetUpdateOperandUsers

	// *** Basic block 16

	mv          a1, s5
	mv          a0, s1
	call        SetLoweredNode

	// *** Basic block 17

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
	li          s3, 166		// 0xa6 ASCII \xa6
	j           .LowerConditionalBranch_label_81

	// *** Basic block 6

.LowerConditionalBranch_label_60:
	li          s3, 167		// 0xa7 ASCII \xa7
	j           .LowerConditionalBranch_label_81

	// *** Basic block 7

.LowerConditionalBranch_label_63:
	lla         a0, .str.187
	lla         a1, .str.188
	lla         a3, .str.189
	li          t0, 1099		// 0x44b
	mv          a2, t0
	call        printf

	// *** Basic block 8

	call        abort

	// *** Basic block 9

	j           .LowerConditionalBranch_label_81

	// *** Basic block 10

.LowerConditionalBranch_label_81:
	addi        t0, s1, 24
	ld          t0, 8(t0)
	li          t1, 2		// 0x2 ASCII \x2
	bne         t0, t1, .LowerConditionalBranch_label_91

	// *** Basic block 11

	j           .LowerConditionalBranch_label_106

	// *** Basic block 12

.LowerConditionalBranch_label_91:
	lla         a0, .str.190
	lla         a1, .str.191
	lla         a3, .str.192
	li          t0, 1102		// 0x44e
	mv          a2, t0
	call        printf

	// *** Basic block 13

	call        abort

	// *** Basic block 14

.LowerConditionalBranch_label_106:
	ld          t0, 24(s1)
	ld          s4, 0(t0)
	ld          s5, 8(t0)
	mv          a0, s4
	call        GetLoweredNode

	// *** Basic block 15

	mv          a1, a0
	mv          a0, s3
	call        NewInstruction1

	// *** Basic block 16

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 17

	mv          s3, a0
	ld          s4, 96(s5)
	bne         s4, x0, .LowerConditionalBranch_label_149

	// *** Basic block 18

	addi        s1, s2, 136
	li          t0, 1		// 0x1 ASCII \x1
	mv          a2, t0
	mv          a1, s5
	mv          a0, s3
	call        NewBranchFixup

	// *** Basic block 19

	mv          a1, a0
	mv          a0, s1
	call        VectorAppend

	// *** Basic block 20

	j           .LowerConditionalBranch_label_153

	// *** Basic block 21

.LowerConditionalBranch_label_149:
	addi        t0, s3, 40
	sd          s4, 8(t0)

	// *** Basic block 22

.LowerConditionalBranch_label_153:
	mv          a0, s3

	// *** Basic block 23

.LowerConditionalBranch_label_156:
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
	bne         t0, t1, .LowerBranch_label_36

	// *** Basic block 1

	j           .LowerBranch_label_53

	// *** Basic block 2

.LowerBranch_label_36:
	lla         a0, .str.193
	lla         a1, .str.194
	lla         a3, .str.195
	li          t0, 1120		// 0x460
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

.LowerBranch_label_53:
	ld          t0, 24(a1)
	ld          s2, 0(t0)
	li          t0, 180		// 0xb4 ASCII \xb4
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 5

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 6

	mv          s3, a0
	ld          s4, 96(s2)
	bne         s4, x0, .LowerBranch_label_88

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

	j           .LowerBranch_label_91

	// *** Basic block 10

.LowerBranch_label_88:
	sd          s4, 40(s3)

	// *** Basic block 11

.LowerBranch_label_91:
	mv          a0, s3

	// *** Basic block 12

.LowerBranch_label_94:
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
	lla         a0, .str.196
	lla         a1, .str.197
	lla         a3, .str.198
	li          t0, 1137		// 0x471
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
	li          t0, 169		// 0xa9 ASCII \xa9
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

	.local  LowerVariable
	.type LowerVariable, @function

LowerVariable:

	// *** Basic block 0

	.local Emit
	.local NewInstruction1
	.local GetIntConstant
	.local GetSymbol
	.local NewInstruction
	.global abort
	.local SetLoweredNode
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
	mv          s3, x0
	mv          s4, x0
	lw          s5, 20(s1)
	li          t0, 96		// 0x60 ASCII '`'
	blt         s5, t0, .LowerVariable_label_137

	// *** Basic block 1

	li          t0, 102		// 0x66 ASCII 'f'
	blt         t0, s5, .LowerVariable_label_137

	// *** Basic block 2

	addi        t0, s5, -96
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 3

	j           .LowerVariable_label_54

	// *** Basic block 4

	j           .LowerVariable_label_112

	// *** Basic block 5

	j           .LowerVariable_label_84

	// *** Basic block 6

	j           .LowerVariable_label_111

	// *** Basic block 7

	j           .LowerVariable_label_55

	// *** Basic block 8

	j           .LowerVariable_label_137

	// *** Basic block 9

	j           .LowerVariable_label_125

	// *** Basic block 10

.LowerVariable_label_54:

	// *** Basic block 11

.LowerVariable_label_55:
	addi        t0, s1, 96
	lw          s3, 8(t0)
	mv          a3, s3
	li          t0, 6		// 0x6 ASCII \x6
	mv          a2, t0
	mv          a1, s1
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 12

	mv          a1, a0
	li          t0, 47		// 0x2f ASCII '/'
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 13

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 14

	mv          s4, a0
	j           .LowerVariable_label_139

	// *** Basic block 15

.LowerVariable_label_84:
	addi        t0, s1, 96
	lw          s3, 8(t0)
	mv          a3, s3
	li          t0, 6		// 0x6 ASCII \x6
	mv          a2, t0
	mv          a1, s1
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 16

	mv          a1, a0
	li          t0, 48		// 0x30 ASCII '0'
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 17

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 18

	mv          s4, a0
	j           .LowerVariable_label_139

	// *** Basic block 19

.LowerVariable_label_111:

	// *** Basic block 20

.LowerVariable_label_112:
	mv          s5, s1
	ld          a2, 136(s5)
	mv          a1, s1
	mv          a0, s2
	call        GetSymbol

	// *** Basic block 21

	mv          s4, a0
	j           .LowerVariable_label_139

	// *** Basic block 22

.LowerVariable_label_125:
	li          t0, 29		// 0x1d ASCII \x1d
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 23

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 24

	mv          s4, a0
	j           .LowerVariable_label_139

	// *** Basic block 25

.LowerVariable_label_137:
	call        abort

	// *** Basic block 26

.LowerVariable_label_139:
	mv          a1, s4
	mv          a0, s1
	call        SetLoweredNode

	// *** Basic block 27

	mv          a0, s4

	// *** Basic block 28

.LowerVariable_label_147:
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
.func_end_LowerVariable:
	.size LowerVariable, .func_end_LowerVariable-LowerVariable

	.local  DeferredLoad
	.type DeferredLoad, @function

DeferredLoad:

	// *** Basic block 0

	.local GetLoweredNode
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	addi        t1, t0, 48
	ld          t1, 8(t1)
	li          t2, 1		// 0x1 ASCII \x1
	bne         t1, t2, .DeferredLoad_label_31

	// *** Basic block 1

	ld          t1, 24(t0)
	ld          a0, 0(t1)
	j           GetLoweredNode

	// *** Basic block 4

.DeferredLoad_label_31:
	mv          a0, x0
	ret         
.func_end_DeferredLoad:
	.size DeferredLoad, .func_end_DeferredLoad-DeferredLoad

	.local  LowerLoad
	.type LowerLoad, @function

LowerLoad:

	// *** Basic block 0

	.global printf
	.global abort
	.local DeferredLoad
	.local SetLoweredNode
	.local GetLoweredNode
	.local Emit
	.local NewInstruction1
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
	bne         t0, t1, .LowerLoad_label_50

	// *** Basic block 1

	j           .LowerLoad_label_67

	// *** Basic block 2

.LowerLoad_label_50:
	lla         a0, .str.199
	lla         a1, .str.200
	lla         a3, .str.201
	li          t0, 1205		// 0x4b5
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

.LowerLoad_label_67:
	ld          t0, 24(s1)
	ld          s3, 0(t0)
	mv          a0, s1
	call        DeferredLoad

	// *** Basic block 5

	mv          s4, a0
	beq         s4, x0, .LowerLoad_label_89

	// *** Basic block 6

	mv          a1, s4
	mv          a0, s1
	call        SetLoweredNode

	// *** Basic block 7

	mv          a0, s4

	// *** Basic block 8

.LowerLoad_label_86:
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

	// *** Basic block 9

.LowerLoad_label_89:
	mv          a0, s3
	call        GetLoweredNode

	// *** Basic block 10

	mv          s3, a0
	lw          s5, 20(s1)
	li          t0, 19		// 0x13 ASCII \x13
	blt         s5, t0, .LowerLoad_label_153

	// *** Basic block 11

	li          t0, 28		// 0x1c ASCII \x1c
	blt         t0, s5, .LowerLoad_label_153

	// *** Basic block 12

	addi        t0, s5, -19
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 13

	j           .LowerLoad_label_122

	// *** Basic block 14

	j           .LowerLoad_label_126

	// *** Basic block 15

	j           .LowerLoad_label_129

	// *** Basic block 16

	j           .LowerLoad_label_132

	// *** Basic block 17

	j           .LowerLoad_label_135

	// *** Basic block 18

	j           .LowerLoad_label_138

	// *** Basic block 19

	j           .LowerLoad_label_141

	// *** Basic block 20

	j           .LowerLoad_label_144

	// *** Basic block 21

	j           .LowerLoad_label_147

	// *** Basic block 22

	j           .LowerLoad_label_150

	// *** Basic block 23

.LowerLoad_label_122:
	li          s5, 67		// 0x43 ASCII 'C'
	j           .LowerLoad_label_169

	// *** Basic block 24

.LowerLoad_label_126:
	li          s5, 69		// 0x45 ASCII 'E'
	j           .LowerLoad_label_169

	// *** Basic block 25

.LowerLoad_label_129:
	li          s5, 73		// 0x49 ASCII 'I'
	j           .LowerLoad_label_169

	// *** Basic block 26

.LowerLoad_label_132:
	li          s5, 68		// 0x44 ASCII 'D'
	j           .LowerLoad_label_169

	// *** Basic block 27

.LowerLoad_label_135:
	li          s5, 70		// 0x46 ASCII 'F'
	j           .LowerLoad_label_169

	// *** Basic block 28

.LowerLoad_label_138:
	li          s5, 71		// 0x47 ASCII 'G'
	j           .LowerLoad_label_169

	// *** Basic block 29

.LowerLoad_label_141:
	li          s5, 72		// 0x48 ASCII 'H'
	j           .LowerLoad_label_169

	// *** Basic block 30

.LowerLoad_label_144:
	li          s5, 74		// 0x4a ASCII 'J'
	j           .LowerLoad_label_169

	// *** Basic block 31

.LowerLoad_label_147:
	li          s5, 75		// 0x4b ASCII 'K'
	j           .LowerLoad_label_169

	// *** Basic block 32

.LowerLoad_label_150:
	li          s5, 66		// 0x42 ASCII 'B'
	j           .LowerLoad_label_169

	// *** Basic block 33

.LowerLoad_label_153:
	lla         a0, .str.202
	lla         a1, .str.203
	lla         a3, .str.204
	li          t0, 1248		// 0x4e0
	mv          a2, t0
	call        printf

	// *** Basic block 34

	call        abort

	// *** Basic block 35

	j           .LowerLoad_label_169

	// *** Basic block 36

.LowerLoad_label_169:
	mv          a1, s3
	mv          a0, s5
	call        NewInstruction1

	// *** Basic block 37

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 38

	mv          s5, a0
	mv          a1, s5
	mv          a0, s1
	call        SetLoweredNode

	// *** Basic block 39

	mv          a0, s5
	j           .LowerLoad_label_86
.func_end_LowerLoad:
	.size LowerLoad, .func_end_LowerLoad-LowerLoad

	.local  LowerStore
	.type LowerStore, @function

LowerStore:

	// *** Basic block 0

	.global printf
	.global abort
	.local GetLoweredNode
	.local Emit
	.local NewInstruction2
	.local SetLoweredNode
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
	li          t1, 2		// 0x2 ASCII \x2
	bne         t0, t1, .LowerStore_label_46

	// *** Basic block 1

	j           .LowerStore_label_63

	// *** Basic block 2

.LowerStore_label_46:
	lla         a0, .str.205
	lla         a1, .str.206
	lla         a3, .str.207
	li          t0, 1260		// 0x4ec
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

.LowerStore_label_63:
	ld          t0, 24(s1)
	ld          s3, 0(t0)
	ld          s4, 8(t0)
	mv          a0, s4
	call        GetLoweredNode

	// *** Basic block 5

	mv          s4, a0
	mv          a0, s3
	call        GetLoweredNode

	// *** Basic block 6

	mv          s3, a0
	lw          s5, 20(s1)
	li          t0, 30		// 0x1e ASCII \x1e
	blt         s5, t0, .LowerStore_label_126

	// *** Basic block 7

	li          t0, 36		// 0x24 ASCII '$'
	blt         t0, s5, .LowerStore_label_126

	// *** Basic block 8

	addi        t0, s5, -30
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 9

	j           .LowerStore_label_104

	// *** Basic block 10

	j           .LowerStore_label_108

	// *** Basic block 11

	j           .LowerStore_label_114

	// *** Basic block 12

	j           .LowerStore_label_111

	// *** Basic block 13

	j           .LowerStore_label_117

	// *** Basic block 14

	j           .LowerStore_label_120

	// *** Basic block 15

	j           .LowerStore_label_123

	// *** Basic block 16

.LowerStore_label_104:
	li          s5, 77		// 0x4d ASCII 'M'
	j           .LowerStore_label_142

	// *** Basic block 17

.LowerStore_label_108:
	li          s5, 82		// 0x52 ASCII 'R'
	j           .LowerStore_label_142

	// *** Basic block 18

.LowerStore_label_111:
	li          s5, 79		// 0x4f ASCII 'O'
	j           .LowerStore_label_142

	// *** Basic block 19

.LowerStore_label_114:
	li          s5, 78		// 0x4e ASCII 'N'
	j           .LowerStore_label_142

	// *** Basic block 20

.LowerStore_label_117:
	li          s5, 80		// 0x50 ASCII 'P'
	j           .LowerStore_label_142

	// *** Basic block 21

.LowerStore_label_120:
	li          s5, 81		// 0x51 ASCII 'Q'
	j           .LowerStore_label_142

	// *** Basic block 22

.LowerStore_label_123:
	li          s5, 79		// 0x4f ASCII 'O'
	j           .LowerStore_label_142

	// *** Basic block 23

.LowerStore_label_126:
	lla         a0, .str.208
	lla         a1, .str.209
	lla         a3, .str.210
	li          t0, 1294		// 0x50e
	mv          a2, t0
	call        printf

	// *** Basic block 24

	call        abort

	// *** Basic block 25

	j           .LowerStore_label_142

	// *** Basic block 26

.LowerStore_label_142:
	mv          a2, s3
	mv          a1, s4
	mv          a0, s5
	call        NewInstruction2

	// *** Basic block 27

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 28

	mv          s5, a0
	mv          a1, s5
	mv          a0, s1
	call        SetLoweredNode

	// *** Basic block 29

	mv          a0, s5

	// *** Basic block 30

.LowerStore_label_164:
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
	li          t0, 52		// 0x34 ASCII '4'
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
	lla         a0, .str.211
	lla         a1, .str.212
	lla         a3, .str.213
	li          t0, 1338		// 0x53a
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
	.local GetLoweredNode
	.local GetSymbol
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
	mv          s1, a1
	mv          t0, a2
	mv          s2, a0
	ld          t1, 80(s1)
	lw          t2, 20(t1)
	ld          t1, 0(t0)
	add         t1, t1, t2
	sd          t1, 0(t0)
	mv          a3, t2
	li          s3, 6		// 0x6 ASCII \x6
	mv          a2, s3
	mv          a1, x0
	call        GetIntConstant

	// *** Basic block 1

	mv          s4, a0
	mv          a1, s4
	li          t0, 50		// 0x32 ASCII '2'
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
	li          t0, 54		// 0x36 ASCII '6'
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 9

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 10

	mv          a0, s1
	call        GetLoweredNode

	// *** Basic block 11

	mv          a1, a0
	li          t0, 87		// 0x57 ASCII 'W'
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 12

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 13

	mv          s7, a0
	mv          a1, s7
	li          s8, 52		// 0x34 ASCII '4'
	mv          a0, s8
	call        NewInstruction1

	// *** Basic block 14

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 15

	mv          a1, s5
	mv          a0, s8
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

	mv          s8, a0
	mv          a1, s8
	li          t0, 185		// 0xb9 ASCII \xb9
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 19

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 20

	mv          a3, s3
	mv          a2, s3
	mv          a1, x0
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 21

	mv          a1, a0
	li          t0, 51		// 0x33 ASCII '3'
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 22

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
	ld s8, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           Emit
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
	.local GetLoweredNode
	.local PushArg
	.global TypeIsFloat
	.global TypeIsDouble
	.global TypeIsLong
	.global TypeIsLongLong
	.global TypeIsInt
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
	blt         s3, s4, .LowerCall_label_58

	// *** Basic block 1

	ld          t0, 24(s1)
	j           .LowerCall_label_75

	// *** Basic block 2

.LowerCall_label_58:
	lla         a0, .str.214
	lla         a1, .str.215
	lla         a3, .str.216
	li          t0, 1378		// 0x562
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

.LowerCall_label_75:
	sd          x0, -32(s0)
	addi        s5, s3, -1
	blt         s5, s4, .LowerCall_label_127

	// *** Basic block 5

.LowerCall_label_86:
	slli        t0, s5, 3
	add         t0, t0, t0
	ld          s3, 0(t0)
	ld          a0, 80(s3)
	call        TypeIsStructOrUnion

	// *** Basic block 6

	beqz        a0, .LowerCall_label_105

	// *** Basic block 7

	addi        a2, s0, -32
	mv          a1, s3
	mv          a0, s2
	call        PushStructArg

	// *** Basic block 8

	j           .LowerCall_label_120

	// *** Basic block 9

.LowerCall_label_105:
	mv          a0, s3
	call        GetLoweredNode

	// *** Basic block 10

	mv          s6, a0
	addi        a3, s0, -32
	mv          a2, s6
	mv          a1, s3
	mv          a0, s2
	call        PushArg

	// *** Basic block 11

.LowerCall_label_120:

	// *** Basic block 12

.LowerCall_label_121:
	addi        s5, s5, -1
	blt         s5, s4, .LowerCall_label_86

	// *** Basic block 13

.LowerCall_label_127:
	ld          t0, 24(s1)
	ld          a0, 0(t0)
	call        GetLoweredNode

	// *** Basic block 14

	mv          s3, a0
	lw          t0, 16(s3)
	li          t1, 2		// 0x2 ASCII \x2
	bne         t0, t1, .LowerCall_label_186

	// *** Basic block 15

	ld          s4, 80(s1)
	mv          a0, s4
	call        TypeIsFloat

	// *** Basic block 16

	beqz        a0, .LowerCall_label_150

	// *** Basic block 17

	li          s5, 188		// 0xbc ASCII \xbc
	j           .LowerCall_label_184

	// *** Basic block 18

.LowerCall_label_150:
	mv          a0, s4
	call        TypeIsDouble

	// *** Basic block 19

	beqz        a0, .LowerCall_label_157

	// *** Basic block 20

	li          s5, 189		// 0xbd ASCII \xbd
	j           .LowerCall_label_183

	// *** Basic block 21

.LowerCall_label_157:
	mv          a0, s4
	call        TypeIsLong

	// *** Basic block 22

	mv          s7, a0
	bnez        a0, .LowerCall_label_168

	// *** Basic block 23

	mv          a0, s4
	call        TypeIsLongLong

	// *** Basic block 24

	mv          s7, a0

	// *** Basic block 25

.LowerCall_label_168:
	beqz        s7, .LowerCall_label_172

	// *** Basic block 26

	li          s5, 187		// 0xbb ASCII \xbb
	j           .LowerCall_label_182

	// *** Basic block 27

.LowerCall_label_172:
	mv          a0, s4
	call        TypeIsInt

	// *** Basic block 28

	beqz        a0, .LowerCall_label_179

	// *** Basic block 29

	li          s5, 186		// 0xba ASCII \xba
	j           .LowerCall_label_181

	// *** Basic block 30

.LowerCall_label_179:
	li          s5, 185		// 0xb9 ASCII \xb9

	// *** Basic block 31

.LowerCall_label_181:

	// *** Basic block 32

.LowerCall_label_182:

	// *** Basic block 33

.LowerCall_label_183:

	// *** Basic block 34

.LowerCall_label_184:
	j           .LowerCall_label_230

	// *** Basic block 35

.LowerCall_label_186:
	ld          s4, 80(s1)
	mv          a0, s4
	call        TypeIsFloat

	// *** Basic block 36

	beqz        a0, .LowerCall_label_195

	// *** Basic block 37

	li          s5, 193		// 0xc1 ASCII \xc1
	j           .LowerCall_label_229

	// *** Basic block 38

.LowerCall_label_195:
	mv          a0, s4
	call        TypeIsDouble

	// *** Basic block 39

	beqz        a0, .LowerCall_label_202

	// *** Basic block 40

	li          s5, 194		// 0xc2 ASCII \xc2
	j           .LowerCall_label_228

	// *** Basic block 41

.LowerCall_label_202:
	mv          a0, s4
	call        TypeIsLong

	// *** Basic block 42

	mv          s7, a0
	bnez        a0, .LowerCall_label_213

	// *** Basic block 43

	mv          a0, s4
	call        TypeIsLongLong

	// *** Basic block 44

	mv          s7, a0

	// *** Basic block 45

.LowerCall_label_213:
	beqz        s7, .LowerCall_label_217

	// *** Basic block 46

	li          s5, 192		// 0xc0 ASCII \xc0
	j           .LowerCall_label_227

	// *** Basic block 47

.LowerCall_label_217:
	mv          a0, s4
	call        TypeIsInt

	// *** Basic block 48

	beqz        a0, .LowerCall_label_224

	// *** Basic block 49

	li          s5, 191		// 0xbf ASCII \xbf
	j           .LowerCall_label_226

	// *** Basic block 50

.LowerCall_label_224:
	li          s5, 190		// 0xbe ASCII \xbe

	// *** Basic block 51

.LowerCall_label_226:

	// *** Basic block 52

.LowerCall_label_227:

	// *** Basic block 53

.LowerCall_label_228:

	// *** Basic block 54

.LowerCall_label_229:

	// *** Basic block 55

.LowerCall_label_230:
	mv          a1, s3
	mv          a0, s5
	call        NewInstruction1

	// *** Basic block 56

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 57

	mv          s4, a0
	ld          s5, -32(s0)
	bge         x0, s5, .LowerCall_label_267

	// *** Basic block 58

	mv          a3, s5
	li          t0, 6		// 0x6 ASCII \x6
	mv          a2, t0
	mv          a1, x0
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 59

	mv          a1, a0
	li          t0, 51		// 0x33 ASCII '3'
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 60

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 61

.LowerCall_label_267:
	mv          a1, s4
	mv          a0, s1
	call        SetLoweredNode

	// *** Basic block 62

	mv          a0, s4

	// *** Basic block 63

.LowerCall_label_275:
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
	.local GetLoweredNode
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
	lla         a0, .str.217
	lla         a1, .str.218
	lla         a3, .str.219
	li          t0, 1427		// 0x593
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
	j           .LowerResult_label_114

	// *** Basic block 13

.LowerResult_label_90:
	li          s3, 27		// 0x1b ASCII \x1b
	li          s4, 19		// 0x13 ASCII \x13
	j           .LowerResult_label_114

	// *** Basic block 14

.LowerResult_label_94:
	li          s3, 28		// 0x1c ASCII \x1c
	li          s4, 20		// 0x14 ASCII \x14
	j           .LowerResult_label_114

	// *** Basic block 15

.LowerResult_label_98:
	lla         a0, .str.220
	lla         a1, .str.221
	lla         a3, .str.222
	li          t0, 1444		// 0x5a4
	mv          a2, t0
	call        printf

	// *** Basic block 16

	call        abort

	// *** Basic block 17

	j           .LowerResult_label_114

	// *** Basic block 18

.LowerResult_label_114:
	ld          t0, 24(s1)
	ld          a0, 0(t0)
	call        GetLoweredNode

	// *** Basic block 19

	mv          s5, a0
	mv          a0, s3
	call        NewInstruction

	// *** Basic block 20

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 21

	mv          s3, a0
	mv          a2, s5
	mv          a1, s3
	mv          a0, s4
	call        NewInstruction2

	// *** Basic block 22

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
	li          t0, 183		// 0xb7 ASCII \xb7
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
	.local GetLoweredNode
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
	ld          a0, 0(t0)
	call        GetLoweredNode

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
	.local GetLoweredNode
	.local NewInstruction1
	.local PushArg
	.local GetSymbol
	.local GetIntConstant
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
	mv          s1, a1
	mv          s2, a0
	addi        t0, s1, 24
	ld          t0, 8(t0)
	li          t1, 3		// 0x3 ASCII \x3
	bne         t0, t1, .LowerMemcpy_label_46

	// *** Basic block 1

	j           .LowerMemcpy_label_63

	// *** Basic block 2

.LowerMemcpy_label_46:
	lla         a0, .str.223
	lla         a1, .str.224
	lla         a3, .str.225
	li          t0, 1479		// 0x5c7
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

.LowerMemcpy_label_63:
	ld          s3, 24(s1)
	ld          a0, 16(s3)
	call        GetLoweredNode

	// *** Basic block 5

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 6

	mv          s4, a0
	mv          a1, s4
	li          t0, 52		// 0x34 ASCII '4'
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 7

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 8

	ld          s5, 8(s3)
	mv          a0, s5
	call        GetLoweredNode

	// *** Basic block 9

	mv          s6, a0
	mv          a1, s6
	li          t0, 87		// 0x57 ASCII 'W'
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 10

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 11

	mv          s6, a0
	sd          s6, 96(s5)
	mv          a3, x0
	mv          a2, s6
	mv          a1, s5
	mv          a0, s2
	call        PushArg

	// *** Basic block 12

	ld          s5, 0(s3)
	mv          a0, s5
	call        GetLoweredNode

	// *** Basic block 13

	mv          s3, a0
	sd          s3, 96(s5)
	mv          a3, x0
	mv          a2, s3
	mv          a1, s5
	mv          a0, s2
	call        PushArg

	// *** Basic block 14

	ld          a2, 160(s2)
	mv          a1, x0
	mv          a0, s2
	call        GetSymbol

	// *** Basic block 15

	mv          s5, a0
	mv          a1, s5
	li          t0, 185		// 0xb9 ASCII \xb9
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 16

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 17

	mv          s7, a0
	li          t0, 6		// 0x6 ASCII \x6
	mv          a3, t0
	mv          a2, t0
	mv          a1, x0
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 18

	mv          a1, a0
	li          t0, 51		// 0x33 ASCII '3'
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 19

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 20

	mv          a1, s7
	mv          a0, s1
	call        SetLoweredNode

	// *** Basic block 21

	mv          a0, s7

	// *** Basic block 22

.LowerMemcpy_label_188:
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
	.local GetLoweredNode
	.local PushArg
	.local GetSymbol
	.local SetLoweredNode
	addi sp, sp, -96
	// Saved return address (offset 88) and frame pointer (offset 80)
	sd ra, 88(sp)
	sd s0, 80(sp)
	addi s0, sp, 96
	// Local vars at offset -16(s0)
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
	mv          s1, a1
	mv          s2, a0
	addi        t0, s1, 24
	ld          t0, 8(t0)
	li          t1, 1		// 0x1 ASCII \x1
	bne         t0, t1, .LowerMemzero_label_48

	// *** Basic block 1

	j           .LowerMemzero_label_65

	// *** Basic block 2

.LowerMemzero_label_48:
	lla         a0, .str.226
	lla         a1, .str.227
	lla         a3, .str.228
	li          t0, 1517		// 0x5ed
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

.LowerMemzero_label_65:
	ld          t0, 24(s1)
	ld          s3, 0(t0)
	mv          s4, s3
	ld          t0, 136(s4)
	ld          t0, 40(t0)
	lw          a3, 20(t0)
	li          s5, 6		// 0x6 ASCII \x6
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
	li          t0, 52		// 0x34 ASCII '4'
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
	li          t0, 54		// 0x36 ASCII '6'
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 13

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 14

	mv          a0, s3
	call        GetLoweredNode

	// *** Basic block 15

	mv          s8, a0
	sd          s8, 96(s3)
	mv          a3, x0
	mv          a2, s8
	mv          a1, s3
	mv          a0, s2
	call        PushArg

	// *** Basic block 16

	ld          a2, 168(s2)
	mv          a1, x0
	mv          a0, s2
	call        GetSymbol

	// *** Basic block 17

	mv          s3, a0
	mv          a1, s3
	li          t0, 185		// 0xb9 ASCII \xb9
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 18

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 19

	mv          s9, a0
	mv          a3, s5
	mv          a2, s5
	mv          a1, x0
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 20

	mv          a1, a0
	li          t0, 51		// 0x33 ASCII '3'
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 21

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 22

	mv          a1, s9
	mv          a0, s1
	call        SetLoweredNode

	// *** Basic block 23

	mv          a0, s9

	// *** Basic block 24

.LowerMemzero_label_212:
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
.func_end_LowerMemzero:
	.size LowerMemzero, .func_end_LowerMemzero-LowerMemzero

	.local  LowerZeroExtend
	.type LowerZeroExtend, @function

LowerZeroExtend:

	// *** Basic block 0

	.local GetLoweredNode
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
	mv          s1, a1
	mv          s2, a0
	ld          s3, 24(s1)
	ld          a0, 0(s3)
	call        GetLoweredNode

	// *** Basic block 1

	mv          s4, a0
	ld          a0, 8(s3)
	call        GetLoweredNode

	// *** Basic block 2

	mv          a2, a0
	mv          a1, s4
	li          t0, 107		// 0x6b ASCII 'k'
	mv          a0, t0
	call        NewInstruction2

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

.LowerZeroExtend_label_51:
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

	.local GetLoweredNode
	.global W65C02IsSignedLoad
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
	mv          s1, a1
	mv          s2, a0
	ld          s3, 24(s1)
	ld          a0, 0(s3)
	call        GetLoweredNode

	// *** Basic block 1

	mv          s4, a0
	lw          a0, 16(s4)
	call        W65C02IsSignedLoad

	// *** Basic block 2

	beqz        a0, .LowerSignExtend_label_49

	// *** Basic block 3

	mv          a1, s4
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
	j           SetLoweredNode

	// *** Basic block 5

.LowerSignExtend_label_46:
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

.LowerSignExtend_label_49:
	ld          s5, 8(s3)
	ld          s3, 136(s5)
	mv          a3, s3
	li          t0, 6		// 0x6 ASCII \x6
	mv          a2, t0
	mv          a1, x0
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 7

	mv          a1, a0
	li          t0, 14		// 0xe ASCII \xe
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 8

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 9

	mv          s3, a0
	mv          a2, s3
	mv          a1, s4
	li          t0, 105		// 0x69 ASCII 'i'
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 10

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 11

	mv          s5, a0
	mv          a2, s3
	mv          a1, s5
	li          t0, 104		// 0x68 ASCII 'h'
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 12

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 13

	mv          s6, a0
	mv          a1, s6
	mv          a0, s1
	call        SetLoweredNode

	// *** Basic block 14

	mv          a0, s6
	j           .LowerSignExtend_label_46
.func_end_LowerSignExtend:
	.size LowerSignExtend, .func_end_LowerSignExtend-LowerSignExtend

	.local  LowerAlign
	.type LowerAlign, @function

LowerAlign:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          a0, x0

	// *** Basic block 1

.LowerAlign_label_6:
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

	.local GetLoweredNode
	.local Emit
	.local NewInstruction2
	.local GetIntConstant
	.local SetLoweredNode
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
	ld          s3, 24(s1)
	ld          a0, 8(s3)
	call        GetLoweredNode

	// *** Basic block 1

	mv          s4, a0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a3, t0
	li          t0, 6		// 0x6 ASCII \x6
	mv          a2, t0
	mv          a1, x0
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 2

	mv          a2, a0
	mv          a1, s4
	li          t0, 87		// 0x57 ASCII 'W'
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 3

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 4

	mv          s5, a0
	ld          a0, 0(s3)
	call        GetLoweredNode

	// *** Basic block 5

	mv          a2, a0
	mv          a1, s5
	li          t0, 76		// 0x4c ASCII 'L'
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 6

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 7

	mv          s3, a0
	mv          a1, s3
	mv          a0, s1
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           SetLoweredNode
.func_end_LowerBuiltinVaStart:
	.size LowerBuiltinVaStart, .func_end_LowerBuiltinVaStart-LowerBuiltinVaStart

	.local  LowerBuiltinVaArg
	.type LowerBuiltinVaArg, @function

LowerBuiltinVaArg:

	// *** Basic block 0

	.local Emit
	.local NewInstruction1
	.local GetLoweredNode
	.global TypeIsUnsigned
	.local NewInstruction2
	.local GetIntConstant
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
	sd s8, 0(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	ld          s3, 24(s2)
	ld          s4, 0(s3)
	mv          a0, s4
	call        GetLoweredNode

	// *** Basic block 1

	mv          a1, a0
	li          t0, 66		// 0x42 ASCII 'B'
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 2

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 3

	mv          s5, a0
	li          s6, 66		// 0x42 ASCII 'B'
	ld          s7, 80(s2)
	lw          s8, 20(s7)
	li          t0, 1		// 0x1 ASCII \x1
	blt         s8, t0, .LowerBuiltinVaArg_label_124

	// *** Basic block 4

	li          t0, 8		// 0x8 ASCII \x8
	blt         t0, s8, .LowerBuiltinVaArg_label_124

	// *** Basic block 5

	addi        t0, s8, -1
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 6

	j           .LowerBuiltinVaArg_label_82

	// *** Basic block 7

	j           .LowerBuiltinVaArg_label_95

	// *** Basic block 8

	j           .LowerBuiltinVaArg_label_124

	// *** Basic block 9

	j           .LowerBuiltinVaArg_label_108

	// *** Basic block 10

	j           .LowerBuiltinVaArg_label_124

	// *** Basic block 11

	j           .LowerBuiltinVaArg_label_124

	// *** Basic block 12

	j           .LowerBuiltinVaArg_label_124

	// *** Basic block 13

	j           .LowerBuiltinVaArg_label_121

	// *** Basic block 14

.LowerBuiltinVaArg_label_82:
	mv          a0, s7
	call        TypeIsUnsigned

	// *** Basic block 15

	beqz        a0, .LowerBuiltinVaArg_label_91

	// *** Basic block 16

	li          s6, 71		// 0x47 ASCII 'G'
	j           .LowerBuiltinVaArg_label_93

	// *** Basic block 17

.LowerBuiltinVaArg_label_91:
	li          s6, 69		// 0x45 ASCII 'E'

	// *** Basic block 18

.LowerBuiltinVaArg_label_93:
	j           .LowerBuiltinVaArg_label_124

	// *** Basic block 19

.LowerBuiltinVaArg_label_95:
	mv          a0, s7
	call        TypeIsUnsigned

	// *** Basic block 20

	beqz        a0, .LowerBuiltinVaArg_label_104

	// *** Basic block 21

	li          s6, 72		// 0x48 ASCII 'H'
	j           .LowerBuiltinVaArg_label_106

	// *** Basic block 22

.LowerBuiltinVaArg_label_104:
	li          s6, 68		// 0x44 ASCII 'D'

	// *** Basic block 23

.LowerBuiltinVaArg_label_106:
	j           .LowerBuiltinVaArg_label_124

	// *** Basic block 24

.LowerBuiltinVaArg_label_108:
	mv          a0, s7
	call        TypeIsUnsigned

	// *** Basic block 25

	beqz        a0, .LowerBuiltinVaArg_label_117

	// *** Basic block 26

	li          s6, 70		// 0x46 ASCII 'F'
	j           .LowerBuiltinVaArg_label_119

	// *** Basic block 27

.LowerBuiltinVaArg_label_117:
	li          s6, 67		// 0x43 ASCII 'C'

	// *** Basic block 28

.LowerBuiltinVaArg_label_119:
	j           .LowerBuiltinVaArg_label_124

	// *** Basic block 29

.LowerBuiltinVaArg_label_121:
	li          s6, 73		// 0x49 ASCII 'I'
	j           .LowerBuiltinVaArg_label_124

	// *** Basic block 30

.LowerBuiltinVaArg_label_124:
	mv          a3, x0
	li          t0, 6		// 0x6 ASCII \x6
	mv          a2, t0
	mv          a1, x0
	mv          a0, s1
	call        GetIntConstant

	// *** Basic block 31

	mv          a2, a0
	mv          a1, s5
	mv          a0, s6
	call        NewInstruction2

	// *** Basic block 32

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 33

	mv          s6, a0
	ld          a0, 8(s3)
	call        GetLoweredNode

	// *** Basic block 34

	mv          s3, a0
	mv          a2, s3
	mv          a1, s5
	li          t0, 87		// 0x57 ASCII 'W'
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 35

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 36

	mv          s7, a0
	mv          a0, s4
	call        GetLoweredNode

	// *** Basic block 37

	mv          a2, a0
	mv          a1, s7
	li          t0, 76		// 0x4c ASCII 'L'
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 38

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 39

	mv          a1, s6
	mv          a0, s2
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
	j           SetLoweredNode
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

	.local  LowerComparison
	.type LowerComparison, @function

LowerComparison:

	// *** Basic block 0

	.global IRIsConditionalBranch
	.local LowerExpression
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
	mv          s3, x0
	mv          s4, x0
	addi        t0, s1, 48
	ld          s5, 8(t0)
	bge         x0, s5, .LowerComparison_label_47

	// *** Basic block 1

	ld          s6, 48(s1)

	// *** Basic block 2

.LowerComparison_label_29:
	slli        t0, s4, 3
	add         t0, s6, t0
	ld          s6, 0(t0)
	mv          a0, s6
	call        IRIsConditionalBranch

	// *** Basic block 3

	not         t0, a0
	beqz        t0, .LowerComparison_label_42

	// *** Basic block 4

	li          s3, 1		// 0x1 ASCII \x1
	j           .LowerComparison_label_47

	// *** Basic block 5

.LowerComparison_label_42:

	// *** Basic block 6

.LowerComparison_label_43:
	addi        s4, s4, 1
	bge         s4, s5, .LowerComparison_label_29

	// *** Basic block 7

.LowerComparison_label_47:
	mv          a1, s1
	mv          a0, s2
	call        LowerExpression

	// *** Basic block 8

	mv          s5, a0
	beqz        s3, .LowerComparison_label_61

	// *** Basic block 9

	lw          t0, 104(s5)
	li          t1, 65536		// 0x10000
	or          t0, t0, t1
	sw          t0, 104(s5)

	// *** Basic block 10

.LowerComparison_label_61:
	mv          a0, s5

	// *** Basic block 11

.LowerComparison_label_64:
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
.func_end_LowerComparison:
	.size LowerComparison, .func_end_LowerComparison-LowerComparison

	.local  LowerIRNode
	.type LowerIRNode, @function

LowerIRNode:

	// *** Basic block 0

	.local Emit
	.local NewInstruction
	.local LowerLiteralReference
	.local LowerAddressOf
	.local GetIntConstant
	.local GetFloatingPointConstant
	.local NewInstruction1
	.local LowerLoad
	.local LowerStore
	.local LowerExpression
	.local LowerComparison
	.local LowerConditionalBranch
	.local LowerBranch
	.local LowerComputedBranch
	.local LowerLabel
	.local LowerNamedLabel
	.local LowerCall
	.local LowerResult
	.local LowerMemzero
	.local LowerMemcpy
	.local SetLoweredNode
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
	// End of stack frame
	mv          s1, a1
	mv          s2, a0
	ld          a0, 96(s1)
	beq         a0, x0, .LowerIRNode_label_71

	// *** Basic block 1

.LowerIRNode_label_68:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 2

.LowerIRNode_label_71:
	lw          s3, 20(s1)
	blt         s3, x0, .LowerIRNode_label_763

	// *** Basic block 3

	li          t0, 132		// 0x84 ASCII \x84
	blt         t0, s3, .LowerIRNode_label_763

	// *** Basic block 4

	slli        t0, s3, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 5

	j           .LowerIRNode_label_238

	// *** Basic block 6

	j           .LowerIRNode_label_524

	// *** Basic block 7

	j           .LowerIRNode_label_284

	// *** Basic block 8

	j           .LowerIRNode_label_299

	// *** Basic block 9

	j           .LowerIRNode_label_266

	// *** Basic block 10

	j           .LowerIRNode_label_331

	// *** Basic block 11

	j           .LowerIRNode_label_347

	// *** Basic block 12

	j           .LowerIRNode_label_364

	// *** Basic block 13

	j           .LowerIRNode_label_315

	// *** Basic block 14

	j           .LowerIRNode_label_516

	// *** Basic block 15

	j           .LowerIRNode_label_517

	// *** Basic block 16

	j           .LowerIRNode_label_518

	// *** Basic block 17

	j           .LowerIRNode_label_519

	// *** Basic block 18

	j           .LowerIRNode_label_520

	// *** Basic block 19

	j           .LowerIRNode_label_521

	// *** Basic block 20

	j           .LowerIRNode_label_522

	// *** Basic block 21

	j           .LowerIRNode_label_523

	// *** Basic block 22

	j           .LowerIRNode_label_598

	// *** Basic block 23

	j           .LowerIRNode_label_608

	// *** Basic block 24

	j           .LowerIRNode_label_448

	// *** Basic block 25

	j           .LowerIRNode_label_449

	// *** Basic block 26

	j           .LowerIRNode_label_450

	// *** Basic block 27

	j           .LowerIRNode_label_451

	// *** Basic block 28

	j           .LowerIRNode_label_452

	// *** Basic block 29

	j           .LowerIRNode_label_453

	// *** Basic block 30

	j           .LowerIRNode_label_454

	// *** Basic block 31

	j           .LowerIRNode_label_455

	// *** Basic block 32

	j           .LowerIRNode_label_456

	// *** Basic block 33

	j           .LowerIRNode_label_457

	// *** Basic block 34

	j           .LowerIRNode_label_763

	// *** Basic block 35

	j           .LowerIRNode_label_467

	// *** Basic block 36

	j           .LowerIRNode_label_468

	// *** Basic block 37

	j           .LowerIRNode_label_469

	// *** Basic block 38

	j           .LowerIRNode_label_470

	// *** Basic block 39

	j           .LowerIRNode_label_471

	// *** Basic block 40

	j           .LowerIRNode_label_472

	// *** Basic block 41

	j           .LowerIRNode_label_473

	// *** Basic block 42

	j           .LowerIRNode_label_483

	// *** Basic block 43

	j           .LowerIRNode_label_484

	// *** Basic block 44

	j           .LowerIRNode_label_485

	// *** Basic block 45

	j           .LowerIRNode_label_486

	// *** Basic block 46

	j           .LowerIRNode_label_487

	// *** Basic block 47

	j           .LowerIRNode_label_488

	// *** Basic block 48

	j           .LowerIRNode_label_489

	// *** Basic block 49

	j           .LowerIRNode_label_490

	// *** Basic block 50

	j           .LowerIRNode_label_491

	// *** Basic block 51

	j           .LowerIRNode_label_492

	// *** Basic block 52

	j           .LowerIRNode_label_493

	// *** Basic block 53

	j           .LowerIRNode_label_494

	// *** Basic block 54

	j           .LowerIRNode_label_495

	// *** Basic block 55

	j           .LowerIRNode_label_496

	// *** Basic block 56

	j           .LowerIRNode_label_497

	// *** Basic block 57

	j           .LowerIRNode_label_498

	// *** Basic block 58

	j           .LowerIRNode_label_499

	// *** Basic block 59

	j           .LowerIRNode_label_500

	// *** Basic block 60

	j           .LowerIRNode_label_501

	// *** Basic block 61

	j           .LowerIRNode_label_502

	// *** Basic block 62

	j           .LowerIRNode_label_503

	// *** Basic block 63

	j           .LowerIRNode_label_504

	// *** Basic block 64

	j           .LowerIRNode_label_505

	// *** Basic block 65

	j           .LowerIRNode_label_506

	// *** Basic block 66

	j           .LowerIRNode_label_507

	// *** Basic block 67

	j           .LowerIRNode_label_508

	// *** Basic block 68

	j           .LowerIRNode_label_509

	// *** Basic block 69

	j           .LowerIRNode_label_534

	// *** Basic block 70

	j           .LowerIRNode_label_535

	// *** Basic block 71

	j           .LowerIRNode_label_536

	// *** Basic block 72

	j           .LowerIRNode_label_537

	// *** Basic block 73

	j           .LowerIRNode_label_538

	// *** Basic block 74

	j           .LowerIRNode_label_539

	// *** Basic block 75

	j           .LowerIRNode_label_540

	// *** Basic block 76

	j           .LowerIRNode_label_541

	// *** Basic block 77

	j           .LowerIRNode_label_542

	// *** Basic block 78

	j           .LowerIRNode_label_543

	// *** Basic block 79

	j           .LowerIRNode_label_544

	// *** Basic block 80

	j           .LowerIRNode_label_545

	// *** Basic block 81

	j           .LowerIRNode_label_546

	// *** Basic block 82

	j           .LowerIRNode_label_547

	// *** Basic block 83

	j           .LowerIRNode_label_548

	// *** Basic block 84

	j           .LowerIRNode_label_549

	// *** Basic block 85

	j           .LowerIRNode_label_550

	// *** Basic block 86

	j           .LowerIRNode_label_551

	// *** Basic block 87

	j           .LowerIRNode_label_552

	// *** Basic block 88

	j           .LowerIRNode_label_553

	// *** Basic block 89

	j           .LowerIRNode_label_554

	// *** Basic block 90

	j           .LowerIRNode_label_555

	// *** Basic block 91

	j           .LowerIRNode_label_556

	// *** Basic block 92

	j           .LowerIRNode_label_557

	// *** Basic block 93

	j           .LowerIRNode_label_567

	// *** Basic block 94

	j           .LowerIRNode_label_568

	// *** Basic block 95

	j           .LowerIRNode_label_578

	// *** Basic block 96

	j           .LowerIRNode_label_588

	// *** Basic block 97

	j           .LowerIRNode_label_618

	// *** Basic block 98

	j           .LowerIRNode_label_434

	// *** Basic block 99

	j           .LowerIRNode_label_380

	// *** Basic block 100

	j           .LowerIRNode_label_407

	// *** Basic block 101

	j           .LowerIRNode_label_218

	// *** Basic block 102

	j           .LowerIRNode_label_222

	// *** Basic block 103

	j           .LowerIRNode_label_219

	// *** Basic block 104

	j           .LowerIRNode_label_221

	// *** Basic block 105

	j           .LowerIRNode_label_220

	// *** Basic block 106

	j           .LowerIRNode_label_243

	// *** Basic block 107

	j           .LowerIRNode_label_224

	// *** Basic block 108

	j           .LowerIRNode_label_256

	// *** Basic block 109

	j           .LowerIRNode_label_246

	// *** Basic block 110

	j           .LowerIRNode_label_713

	// *** Basic block 111

	j           .LowerIRNode_label_628

	// *** Basic block 112

	j           .LowerIRNode_label_629

	// *** Basic block 113

	j           .LowerIRNode_label_630

	// *** Basic block 114

	j           .LowerIRNode_label_631

	// *** Basic block 115

	j           .LowerIRNode_label_510

	// *** Basic block 116

	j           .LowerIRNode_label_511

	// *** Basic block 117

	j           .LowerIRNode_label_512

	// *** Basic block 118

	j           .LowerIRNode_label_513

	// *** Basic block 119

	j           .LowerIRNode_label_514

	// *** Basic block 120

	j           .LowerIRNode_label_515

	// *** Basic block 121

	j           .LowerIRNode_label_673

	// *** Basic block 122

	j           .LowerIRNode_label_683

	// *** Basic block 123

	j           .LowerIRNode_label_693

	// *** Basic block 124

	j           .LowerIRNode_label_641

	// *** Basic block 125

	j           .LowerIRNode_label_651

	// *** Basic block 126

	j           .LowerIRNode_label_661

	// *** Basic block 127

	j           .LowerIRNode_label_244

	// *** Basic block 128

	j           .LowerIRNode_label_703

	// *** Basic block 129

	j           .LowerIRNode_label_763

	// *** Basic block 130

	j           .LowerIRNode_label_763

	// *** Basic block 131

	j           .LowerIRNode_label_763

	// *** Basic block 132

	j           .LowerIRNode_label_763

	// *** Basic block 133

	j           .LowerIRNode_label_723

	// *** Basic block 134

	j           .LowerIRNode_label_733

	// *** Basic block 135

	j           .LowerIRNode_label_743

	// *** Basic block 136

	j           .LowerIRNode_label_753

	// *** Basic block 137

	j           .LowerIRNode_label_239

	// *** Basic block 138

.LowerIRNode_label_218:

	// *** Basic block 139

.LowerIRNode_label_219:

	// *** Basic block 140

.LowerIRNode_label_220:

	// *** Basic block 141

.LowerIRNode_label_221:

	// *** Basic block 142

.LowerIRNode_label_222:
	j           .LowerIRNode_label_763

	// *** Basic block 143

.LowerIRNode_label_224:
	li          t0, 29		// 0x1d ASCII \x1d
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 144

	mv          a1, a0
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           Emit

	// *** Basic block 146

.LowerIRNode_label_238:

	// *** Basic block 147

.LowerIRNode_label_239:
	mv          a0, x0
	j           .LowerIRNode_label_68

	// *** Basic block 148

.LowerIRNode_label_243:

	// *** Basic block 149

.LowerIRNode_label_244:
	j           .LowerIRNode_label_763

	// *** Basic block 150

.LowerIRNode_label_246:
	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           LowerLiteralReference

	// *** Basic block 152

.LowerIRNode_label_256:
	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           LowerAddressOf

	// *** Basic block 154

.LowerIRNode_label_266:
	ld          a3, 136(s1)
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           GetIntConstant

	// *** Basic block 156

.LowerIRNode_label_284:
	ld          a3, 136(s1)
	mv          a2, x0
	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           GetIntConstant

	// *** Basic block 158

.LowerIRNode_label_299:
	ld          a3, 136(s1)
	li          t0, 1		// 0x1 ASCII \x1
	mv          a2, t0
	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           GetIntConstant

	// *** Basic block 160

.LowerIRNode_label_315:
	ld          a3, 136(s1)
	li          t0, 6		// 0x6 ASCII \x6
	mv          a2, t0
	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           GetIntConstant

	// *** Basic block 162

.LowerIRNode_label_331:
	ld          a3, 136(s1)
	li          t0, 3		// 0x3 ASCII \x3
	mv          a2, t0
	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           GetIntConstant

	// *** Basic block 164

.LowerIRNode_label_347:
	fld         fa0, 136(s1)
	li          t0, 4		// 0x4 ASCII \x4
	mv          a2, t0
	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           GetFloatingPointConstant

	// *** Basic block 166

.LowerIRNode_label_364:
	fld         fa0, 136(s1)
	li          t0, 5		// 0x5 ASCII \x5
	mv          a2, t0
	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           GetFloatingPointConstant

	// *** Basic block 168

.LowerIRNode_label_380:
	lw          a3, 128(s2)
	li          t0, 6		// 0x6 ASCII \x6
	mv          a2, t0
	mv          a1, x0
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 169

	mv          a1, a0
	li          t0, 45		// 0x2d ASCII '-'
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 170

	mv          a1, a0
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           Emit

	// *** Basic block 172

.LowerIRNode_label_407:
	lw          a3, 128(s2)
	li          t0, 6		// 0x6 ASCII \x6
	mv          a2, t0
	mv          a1, x0
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 173

	mv          a1, a0
	li          t0, 46		// 0x2e ASCII '.'
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 174

	mv          a1, a0
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           Emit

	// *** Basic block 176

.LowerIRNode_label_434:
	li          t0, 21		// 0x15 ASCII \x15
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 177

	mv          a1, a0
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           Emit

	// *** Basic block 179

.LowerIRNode_label_448:

	// *** Basic block 180

.LowerIRNode_label_449:

	// *** Basic block 181

.LowerIRNode_label_450:

	// *** Basic block 182

.LowerIRNode_label_451:

	// *** Basic block 183

.LowerIRNode_label_452:

	// *** Basic block 184

.LowerIRNode_label_453:

	// *** Basic block 185

.LowerIRNode_label_454:

	// *** Basic block 186

.LowerIRNode_label_455:

	// *** Basic block 187

.LowerIRNode_label_456:

	// *** Basic block 188

.LowerIRNode_label_457:
	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           LowerLoad

	// *** Basic block 190

.LowerIRNode_label_467:

	// *** Basic block 191

.LowerIRNode_label_468:

	// *** Basic block 192

.LowerIRNode_label_469:

	// *** Basic block 193

.LowerIRNode_label_470:

	// *** Basic block 194

.LowerIRNode_label_471:

	// *** Basic block 195

.LowerIRNode_label_472:

	// *** Basic block 196

.LowerIRNode_label_473:
	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           LowerStore

	// *** Basic block 198

.LowerIRNode_label_483:

	// *** Basic block 199

.LowerIRNode_label_484:

	// *** Basic block 200

.LowerIRNode_label_485:

	// *** Basic block 201

.LowerIRNode_label_486:

	// *** Basic block 202

.LowerIRNode_label_487:

	// *** Basic block 203

.LowerIRNode_label_488:

	// *** Basic block 204

.LowerIRNode_label_489:

	// *** Basic block 205

.LowerIRNode_label_490:

	// *** Basic block 206

.LowerIRNode_label_491:

	// *** Basic block 207

.LowerIRNode_label_492:

	// *** Basic block 208

.LowerIRNode_label_493:

	// *** Basic block 209

.LowerIRNode_label_494:

	// *** Basic block 210

.LowerIRNode_label_495:

	// *** Basic block 211

.LowerIRNode_label_496:

	// *** Basic block 212

.LowerIRNode_label_497:

	// *** Basic block 213

.LowerIRNode_label_498:

	// *** Basic block 214

.LowerIRNode_label_499:

	// *** Basic block 215

.LowerIRNode_label_500:

	// *** Basic block 216

.LowerIRNode_label_501:

	// *** Basic block 217

.LowerIRNode_label_502:

	// *** Basic block 218

.LowerIRNode_label_503:

	// *** Basic block 219

.LowerIRNode_label_504:

	// *** Basic block 220

.LowerIRNode_label_505:

	// *** Basic block 221

.LowerIRNode_label_506:

	// *** Basic block 222

.LowerIRNode_label_507:

	// *** Basic block 223

.LowerIRNode_label_508:

	// *** Basic block 224

.LowerIRNode_label_509:

	// *** Basic block 225

.LowerIRNode_label_510:

	// *** Basic block 226

.LowerIRNode_label_511:

	// *** Basic block 227

.LowerIRNode_label_512:

	// *** Basic block 228

.LowerIRNode_label_513:

	// *** Basic block 229

.LowerIRNode_label_514:

	// *** Basic block 230

.LowerIRNode_label_515:

	// *** Basic block 231

.LowerIRNode_label_516:

	// *** Basic block 232

.LowerIRNode_label_517:

	// *** Basic block 233

.LowerIRNode_label_518:

	// *** Basic block 234

.LowerIRNode_label_519:

	// *** Basic block 235

.LowerIRNode_label_520:

	// *** Basic block 236

.LowerIRNode_label_521:

	// *** Basic block 237

.LowerIRNode_label_522:

	// *** Basic block 238

.LowerIRNode_label_523:

	// *** Basic block 239

.LowerIRNode_label_524:
	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           LowerExpression

	// *** Basic block 241

.LowerIRNode_label_534:

	// *** Basic block 242

.LowerIRNode_label_535:

	// *** Basic block 243

.LowerIRNode_label_536:

	// *** Basic block 244

.LowerIRNode_label_537:

	// *** Basic block 245

.LowerIRNode_label_538:

	// *** Basic block 246

.LowerIRNode_label_539:

	// *** Basic block 247

.LowerIRNode_label_540:

	// *** Basic block 248

.LowerIRNode_label_541:

	// *** Basic block 249

.LowerIRNode_label_542:

	// *** Basic block 250

.LowerIRNode_label_543:

	// *** Basic block 251

.LowerIRNode_label_544:

	// *** Basic block 252

.LowerIRNode_label_545:

	// *** Basic block 253

.LowerIRNode_label_546:

	// *** Basic block 254

.LowerIRNode_label_547:

	// *** Basic block 255

.LowerIRNode_label_548:

	// *** Basic block 256

.LowerIRNode_label_549:

	// *** Basic block 257

.LowerIRNode_label_550:

	// *** Basic block 258

.LowerIRNode_label_551:

	// *** Basic block 259

.LowerIRNode_label_552:

	// *** Basic block 260

.LowerIRNode_label_553:

	// *** Basic block 261

.LowerIRNode_label_554:

	// *** Basic block 262

.LowerIRNode_label_555:

	// *** Basic block 263

.LowerIRNode_label_556:

	// *** Basic block 264

.LowerIRNode_label_557:
	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           LowerComparison

	// *** Basic block 266

.LowerIRNode_label_567:

	// *** Basic block 267

.LowerIRNode_label_568:
	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           LowerConditionalBranch

	// *** Basic block 269

.LowerIRNode_label_578:
	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           LowerBranch

	// *** Basic block 271

.LowerIRNode_label_588:
	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           LowerComputedBranch

	// *** Basic block 273

.LowerIRNode_label_598:
	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           LowerLabel

	// *** Basic block 275

.LowerIRNode_label_608:
	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           LowerNamedLabel

	// *** Basic block 277

.LowerIRNode_label_618:
	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           LowerCall

	// *** Basic block 279

.LowerIRNode_label_628:

	// *** Basic block 280

.LowerIRNode_label_629:

	// *** Basic block 281

.LowerIRNode_label_630:

	// *** Basic block 282

.LowerIRNode_label_631:
	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           LowerResult

	// *** Basic block 284

.LowerIRNode_label_641:
	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           LowerMemzero

	// *** Basic block 286

.LowerIRNode_label_651:
	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           LowerMemcpy

	// *** Basic block 288

.LowerIRNode_label_661:
	ld          t0, 24(s1)
	ld          a1, 0(t0)
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           SetLoweredNode

	// *** Basic block 290

.LowerIRNode_label_673:
	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           LowerZeroExtend

	// *** Basic block 292

.LowerIRNode_label_683:
	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           LowerSignExtend

	// *** Basic block 294

.LowerIRNode_label_693:
	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           LowerAlign

	// *** Basic block 296

.LowerIRNode_label_703:
	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           LowerAsm

	// *** Basic block 298

.LowerIRNode_label_713:
	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           LowerLocation

	// *** Basic block 300

.LowerIRNode_label_723:
	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           LowerBuiltinVaStart

	// *** Basic block 302

.LowerIRNode_label_733:
	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           LowerBuiltinVaArg

	// *** Basic block 304

.LowerIRNode_label_743:
	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           LowerBuiltinVaEnd

	// *** Basic block 306

.LowerIRNode_label_753:
	mv          a1, s1
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           LowerBuiltinVaCopy

	// *** Basic block 308

.LowerIRNode_label_763:
	lla         a0, .str.229
	lla         a1, .str.230
	lla         a3, .str.231
	li          t0, 1932		// 0x78c
	mv          a2, t0
	call        printf

	// *** Basic block 309

	call        abort

	// *** Basic block 310

	mv          a0, x0
	j           .LowerIRNode_label_68
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

	.global W65C02Lower
	.type W65C02Lower, @function

W65C02Lower:

	// *** Basic block 0

	.global compiler
	.local CalculateArgumentSize
	.global NewTargetSymbol
	.local GetIntConstant
	.local Emit
	.local GetSymbol
	.global GeneratorFirstInstruction
	.local LowerIRNode
	.global IRNext
	.global W65C02Print
	.global W65C02AllocateRegisters
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
	mv          s1, a1
	mv          s2, a0
	mv          s3, x0
	li          s4, 16		// 0x10 ASCII \x10
	la          t0, compiler
	ld          s5, 0(t0)
	sd          s5, -24(s0)	// Spilled @49
	ld          t0, 1096(s5)
	addi        t0, t0, 32
	addi        s6, t0, 8
	mv          s7, x0
	ld          s8, 8(s6)
	bge         x0, s8, .W65C02Lower_label_81

	// *** Basic block 1

	ld          s6, 0(s6)

	// *** Basic block 2

.W65C02Lower_label_62:
	slli        t0, s7, 3
	add         t0, s6, t0
	ld          s6, 0(t0)
	sw          s4, 120(s6)
	mv          a0, s6
	call        CalculateArgumentSize

	// *** Basic block 4

.W65C02Lower_label_77:
	addi        s7, s7, 1
	bge         s7, s8, .W65C02Lower_label_62

	// *** Basic block 5

.W65C02Lower_label_81:
	mv          s4, x0
	addi        t0, s1, 144
	ld          s6, 8(t0)
	bge         x0, s6, .W65C02Lower_label_232

	// *** Basic block 6

	ld          t0, 144(s1)

	// *** Basic block 7

.W65C02Lower_label_90:
	slli        t1, s4, 3
	add         t0, t0, t1
	ld          s8, 0(t0)
	ld          s9, 16(s8)
	lw          s10, 20(s9)
	addi        t1, s10, -96
	seqz        t0, t1
	li          t1, 96		// 0x60 ASCII '`'
	beq         s10, t1, .W65C02Lower_label_108

	// *** Basic block 8

	addi        t1, s10, -100
	seqz        t0, t1

	// *** Basic block 9

.W65C02Lower_label_108:
	beqz        t0, .W65C02Lower_label_158

	// *** Basic block 10

	ld          t0, 8(s8)
	ld          t0, 40(t0)
	lw          s11, 20(t0)
	addi        t0, s11, -1
	add         t1, s3, t0
	not         t0, t0
	and         s3, t1, t0
	ld          a0, 136(s9)
	call        NewTargetSymbol

	// *** Basic block 11

	mv          s5, a0
	mv          a3, s3
	li          t0, 6		// 0x6 ASCII \x6
	mv          a2, t0
	mv          a1, s9
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 12

	sd          a0, 40(s5)
	li          t0, 47		// 0x2f ASCII '/'
	sw          t0, 16(s5)
	mv          a1, s5
	mv          a0, s2
	call        Emit

	// *** Basic block 13

	mv          s5, a0
	ld          t0, 16(s8)
	sd          s5, 96(t0)
	add         s3, s3, s11
	j           .W65C02Lower_label_227

	// *** Basic block 14

.W65C02Lower_label_158:
	li          t0, 98		// 0x62 ASCII 'b'
	bne         s10, t0, .W65C02Lower_label_197

	// *** Basic block 15

	mv          s11, s9
	ld          a0, 136(s11)
	call        NewTargetSymbol

	// *** Basic block 16

	mv          s5, a0
	mv          a3, s3
	li          t0, 6		// 0x6 ASCII \x6
	mv          a2, t0
	mv          a1, s9
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 17

	sd          a0, 40(s5)
	li          t0, 48		// 0x30 ASCII '0'
	sw          t0, 16(s5)
	mv          a1, s5
	mv          a0, s2
	call        Emit

	// *** Basic block 18

	mv          s5, a0
	ld          t0, 16(s8)
	sd          s5, 96(t0)
	j           .W65C02Lower_label_226

	// *** Basic block 19

.W65C02Lower_label_197:
	addi        t1, s10, -99
	seqz        t0, t1
	li          t1, 99		// 0x63 ASCII 'c'
	beq         s10, t1, .W65C02Lower_label_207

	// *** Basic block 20

	addi        t1, s10, -97
	seqz        t0, t1

	// *** Basic block 21

.W65C02Lower_label_207:
	beqz        t0, .W65C02Lower_label_225

	// *** Basic block 22

	mv          s10, s9
	ld          a2, 136(s10)
	mv          a1, x0
	mv          a0, s2
	call        GetSymbol

	// *** Basic block 23

	mv          s9, a0
	ld          t0, 16(s8)
	sd          s9, 96(t0)

	// *** Basic block 24

.W65C02Lower_label_225:

	// *** Basic block 25

.W65C02Lower_label_226:

	// *** Basic block 26

.W65C02Lower_label_227:

	// *** Basic block 27

.W65C02Lower_label_228:
	addi        s4, s4, 1
	bge         s4, s6, .W65C02Lower_label_90

	// *** Basic block 28

.W65C02Lower_label_232:
	sw          s3, 128(s2)
	mv          a0, s1
	call        GeneratorFirstInstruction

	// *** Basic block 29

	mv          s6, a0
	beq         s6, x0, .W65C02Lower_label_256

	// *** Basic block 30

.W65C02Lower_label_243:
	mv          a1, s6
	mv          a0, s2
	call        LowerIRNode

	// *** Basic block 31

	mv          a0, s6
	call        IRNext

	// *** Basic block 32

	mv          s6, a0
	bne         s6, x0, .W65C02Lower_label_243

	// *** Basic block 33

.W65C02Lower_label_256:
	ld          t0, -24(s0)	// Spilled @49
	lb          t1, 1232(t0)
	beqz        t1, .W65C02Lower_label_263

	// *** Basic block 34

	mv          a0, s2
	call        W65C02Print

	// *** Basic block 35

.W65C02Lower_label_263:
	addi        a0, s2, 192
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
	j           W65C02AllocateRegisters
.func_end_W65C02Lower:
	.size W65C02Lower, .func_end_W65C02Lower-W65C02Lower

	.global W65C02PrintInstruction
	.type W65C02PrintInstruction, @function

W65C02PrintInstruction:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.global TargetPrintInstruction
	.global W65C02OpcodeName
	mv          t0, a1
	mv          a2, t0
	la          a1, W65C02OpcodeName
	j           TargetPrintInstruction
.func_end_W65C02PrintInstruction:
	.size W65C02PrintInstruction, .func_end_W65C02PrintInstruction-W65C02PrintInstruction

	.global W65C02Print
	.type W65C02Print, @function

W65C02Print:

	// *** Basic block 0

	.global TargetFirstInstruction
	.global TargetPrintInstruction
	.global W65C02OpcodeName
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
	beq         s2, x0, .W65C02Print_label_40

	// *** Basic block 2

.W65C02Print_label_22:
	mv          a2, s1
	la          t0, W65C02OpcodeName
	mv          a1, t0
	mv          a0, s2
	call        TargetPrintInstruction

	// *** Basic block 3

	mv          a0, s2
	call        TargetNext

	// *** Basic block 4

	mv          s2, a0
	bne         s2, x0, .W65C02Print_label_22

	// *** Basic block 5

.W65C02Print_label_40:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_W65C02Print:
	.size W65C02Print, .func_end_W65C02Print-W65C02Print

	.global W65C02IsExpression
	.type W65C02IsExpression, @function

W65C02IsExpression:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	li          t1, 64		// 0x40 ASCII '@'
	blt         t0, t1, .W65C02IsExpression_label_174

	// *** Basic block 1

	li          t2, 180		// 0xb4 ASCII \xb4
	blt         t0, t2, .W65C02IsExpression_label_117

	// *** Basic block 2

	beq         t0, t2, .W65C02IsExpression_label_315

	// *** Basic block 3

	li          t2, 181		// 0xb5 ASCII \xb5
	beq         t0, t2, .W65C02IsExpression_label_316

	// *** Basic block 4

	li          t2, 185		// 0xb9 ASCII \xb9
	beq         t0, t2, .W65C02IsExpression_label_317

	// *** Basic block 5

	li          t2, 186		// 0xba ASCII \xba
	beq         t0, t2, .W65C02IsExpression_label_318

	// *** Basic block 6

	li          t2, 187		// 0xbb ASCII \xbb
	beq         t0, t2, .W65C02IsExpression_label_319

	// *** Basic block 7

	li          t2, 188		// 0xbc ASCII \xbc
	beq         t0, t2, .W65C02IsExpression_label_320

	// *** Basic block 8

	li          t2, 189		// 0xbd ASCII \xbd
	beq         t0, t2, .W65C02IsExpression_label_321

	// *** Basic block 9

	li          t2, 190		// 0xbe ASCII \xbe
	beq         t0, t2, .W65C02IsExpression_label_322

	// *** Basic block 10

	li          t2, 191		// 0xbf ASCII \xbf
	beq         t0, t2, .W65C02IsExpression_label_323

	// *** Basic block 11

	li          t2, 192		// 0xc0 ASCII \xc0
	beq         t0, t2, .W65C02IsExpression_label_324

	// *** Basic block 12

	li          t2, 193		// 0xc1 ASCII \xc1
	beq         t0, t2, .W65C02IsExpression_label_325

	// *** Basic block 13

	li          t2, 194		// 0xc2 ASCII \xc2
	beq         t0, t2, .W65C02IsExpression_label_326

	// *** Basic block 14

	j           .W65C02IsExpression_label_339

	// *** Basic block 15

.W65C02IsExpression_label_117:
	beq         t0, t1, .W65C02IsExpression_label_304

	// *** Basic block 16

	li          t1, 65		// 0x41 ASCII 'A'
	beq         t0, t1, .W65C02IsExpression_label_305

	// *** Basic block 17

	li          t1, 77		// 0x4d ASCII 'M'
	beq         t0, t1, .W65C02IsExpression_label_306

	// *** Basic block 18

	li          t1, 78		// 0x4e ASCII 'N'
	beq         t0, t1, .W65C02IsExpression_label_307

	// *** Basic block 19

	li          t1, 79		// 0x4f ASCII 'O'
	beq         t0, t1, .W65C02IsExpression_label_308

	// *** Basic block 20

	li          t1, 80		// 0x50 ASCII 'P'
	beq         t0, t1, .W65C02IsExpression_label_309

	// *** Basic block 21

	li          t1, 81		// 0x51 ASCII 'Q'
	beq         t0, t1, .W65C02IsExpression_label_310

	// *** Basic block 22

	li          t1, 82		// 0x52 ASCII 'R'
	beq         t0, t1, .W65C02IsExpression_label_311

	// *** Basic block 23

	li          t1, 166		// 0xa6 ASCII \xa6
	beq         t0, t1, .W65C02IsExpression_label_312

	// *** Basic block 24

	li          t1, 167		// 0xa7 ASCII \xa7
	beq         t0, t1, .W65C02IsExpression_label_313

	// *** Basic block 25

	li          t1, 169		// 0xa9 ASCII \xa9
	beq         t0, t1, .W65C02IsExpression_label_314

	// *** Basic block 26

	j           .W65C02IsExpression_label_339

	// *** Basic block 27

.W65C02IsExpression_label_174:
	li          t1, 51		// 0x33 ASCII '3'
	blt         t0, t1, .W65C02IsExpression_label_234

	// *** Basic block 28

	beq         t0, t1, .W65C02IsExpression_label_293

	// *** Basic block 29

	li          t1, 53		// 0x35 ASCII '5'
	beq         t0, t1, .W65C02IsExpression_label_294

	// *** Basic block 30

	li          t1, 54		// 0x36 ASCII '6'
	beq         t0, t1, .W65C02IsExpression_label_295

	// *** Basic block 31

	li          t1, 55		// 0x37 ASCII '7'
	beq         t0, t1, .W65C02IsExpression_label_296

	// *** Basic block 32

	li          t1, 56		// 0x38 ASCII '8'
	beq         t0, t1, .W65C02IsExpression_label_297

	// *** Basic block 33

	li          t1, 57		// 0x39 ASCII '9'
	beq         t0, t1, .W65C02IsExpression_label_298

	// *** Basic block 34

	li          t1, 58		// 0x3a ASCII ':'
	beq         t0, t1, .W65C02IsExpression_label_299

	// *** Basic block 35

	li          t1, 60		// 0x3c ASCII '<'
	beq         t0, t1, .W65C02IsExpression_label_300

	// *** Basic block 36

	li          t1, 61		// 0x3d ASCII '='
	beq         t0, t1, .W65C02IsExpression_label_301

	// *** Basic block 37

	li          t1, 62		// 0x3e ASCII '>'
	beq         t0, t1, .W65C02IsExpression_label_302

	// *** Basic block 38

	li          t1, 63		// 0x3f ASCII '?'
	beq         t0, t1, .W65C02IsExpression_label_303

	// *** Basic block 39

	j           .W65C02IsExpression_label_339

	// *** Basic block 40

.W65C02IsExpression_label_234:
	beqz        t0, .W65C02IsExpression_label_288

	// *** Basic block 41

	li          t1, 1		// 0x1 ASCII \x1
	beq         t0, t1, .W65C02IsExpression_label_289

	// *** Basic block 42

	li          t1, 18		// 0x12 ASCII \x12
	beq         t0, t1, .W65C02IsExpression_label_328

	// *** Basic block 43

	li          t1, 19		// 0x13 ASCII \x13
	beq         t0, t1, .W65C02IsExpression_label_329

	// *** Basic block 44

	li          t1, 20		// 0x14 ASCII \x14
	beq         t0, t1, .W65C02IsExpression_label_330

	// *** Basic block 45

	li          t1, 21		// 0x15 ASCII \x15
	beq         t0, t1, .W65C02IsExpression_label_327

	// *** Basic block 46

	li          t1, 22		// 0x16 ASCII \x16
	beq         t0, t1, .W65C02IsExpression_label_290

	// *** Basic block 47

	li          t1, 30		// 0x1e ASCII \x1e
	beq         t0, t1, .W65C02IsExpression_label_291

	// *** Basic block 48

	li          t1, 31		// 0x1f ASCII \x1f
	beq         t0, t1, .W65C02IsExpression_label_331

	// *** Basic block 49

	li          t1, 32		// 0x20 ASCII ' '
	beq         t0, t1, .W65C02IsExpression_label_332

	// *** Basic block 50

	li          t1, 50		// 0x32 ASCII '2'
	beq         t0, t1, .W65C02IsExpression_label_292

	// *** Basic block 51

	j           .W65C02IsExpression_label_339

	// *** Basic block 52

.W65C02IsExpression_label_288:

	// *** Basic block 53

.W65C02IsExpression_label_289:

	// *** Basic block 54

.W65C02IsExpression_label_290:

	// *** Basic block 55

.W65C02IsExpression_label_291:

	// *** Basic block 56

.W65C02IsExpression_label_292:

	// *** Basic block 57

.W65C02IsExpression_label_293:

	// *** Basic block 58

.W65C02IsExpression_label_294:

	// *** Basic block 59

.W65C02IsExpression_label_295:

	// *** Basic block 60

.W65C02IsExpression_label_296:

	// *** Basic block 61

.W65C02IsExpression_label_297:

	// *** Basic block 62

.W65C02IsExpression_label_298:

	// *** Basic block 63

.W65C02IsExpression_label_299:

	// *** Basic block 64

.W65C02IsExpression_label_300:

	// *** Basic block 65

.W65C02IsExpression_label_301:

	// *** Basic block 66

.W65C02IsExpression_label_302:

	// *** Basic block 67

.W65C02IsExpression_label_303:

	// *** Basic block 68

.W65C02IsExpression_label_304:

	// *** Basic block 69

.W65C02IsExpression_label_305:

	// *** Basic block 70

.W65C02IsExpression_label_306:

	// *** Basic block 71

.W65C02IsExpression_label_307:

	// *** Basic block 72

.W65C02IsExpression_label_308:

	// *** Basic block 73

.W65C02IsExpression_label_309:

	// *** Basic block 74

.W65C02IsExpression_label_310:

	// *** Basic block 75

.W65C02IsExpression_label_311:

	// *** Basic block 76

.W65C02IsExpression_label_312:

	// *** Basic block 77

.W65C02IsExpression_label_313:

	// *** Basic block 78

.W65C02IsExpression_label_314:

	// *** Basic block 79

.W65C02IsExpression_label_315:

	// *** Basic block 80

.W65C02IsExpression_label_316:

	// *** Basic block 81

.W65C02IsExpression_label_317:

	// *** Basic block 82

.W65C02IsExpression_label_318:

	// *** Basic block 83

.W65C02IsExpression_label_319:

	// *** Basic block 84

.W65C02IsExpression_label_320:

	// *** Basic block 85

.W65C02IsExpression_label_321:

	// *** Basic block 86

.W65C02IsExpression_label_322:

	// *** Basic block 87

.W65C02IsExpression_label_323:

	// *** Basic block 88

.W65C02IsExpression_label_324:

	// *** Basic block 89

.W65C02IsExpression_label_325:

	// *** Basic block 90

.W65C02IsExpression_label_326:

	// *** Basic block 91

.W65C02IsExpression_label_327:

	// *** Basic block 92

.W65C02IsExpression_label_328:

	// *** Basic block 93

.W65C02IsExpression_label_329:

	// *** Basic block 94

.W65C02IsExpression_label_330:

	// *** Basic block 95

.W65C02IsExpression_label_331:

	// *** Basic block 96

.W65C02IsExpression_label_332:
	mv          a0, x0

	// *** Basic block 97

.W65C02IsExpression_label_336:
	ret         

	// *** Basic block 98

.W65C02IsExpression_label_339:
	li          a0, 1		// 0x1 ASCII \x1
	ret         
.func_end_W65C02IsExpression:
	.size W65C02IsExpression, .func_end_W65C02IsExpression-W65C02IsExpression

	.global W65C02IsSignedLoad
	.type W65C02IsSignedLoad, @function

W65C02IsSignedLoad:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	li          t1, 67		// 0x43 ASCII 'C'
	blt         t0, t1, .W65C02IsSignedLoad_label_36

	// *** Basic block 1

	li          t1, 69		// 0x45 ASCII 'E'
	blt         t1, t0, .W65C02IsSignedLoad_label_36

	// *** Basic block 2

	addi        t1, t0, -67
	slli        t1, t1, 2
	auipc       t2, 0
	add         t1, t2, t1
	jalr        x0, t1, 12

	// *** Basic block 3

	j           .W65C02IsSignedLoad_label_30

	// *** Basic block 4

	j           .W65C02IsSignedLoad_label_29

	// *** Basic block 5

	j           .W65C02IsSignedLoad_label_28

	// *** Basic block 6

.W65C02IsSignedLoad_label_28:

	// *** Basic block 7

.W65C02IsSignedLoad_label_29:

	// *** Basic block 8

.W65C02IsSignedLoad_label_30:
	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 9

.W65C02IsSignedLoad_label_33:
	ret         

	// *** Basic block 10

.W65C02IsSignedLoad_label_36:
	mv          a0, x0
	ret         
.func_end_W65C02IsSignedLoad:
	.size W65C02IsSignedLoad, .func_end_W65C02IsSignedLoad-W65C02IsSignedLoad

.PCend:
	.data
load_opcodes:
	.type   load_opcodes,@object
	.local  load_opcodes
	.size   load_opcodes,224
	.p2align  3
	.global TypeIsInt
	.long    TypeIsInt
	.word   67
	.space  4
	.global TypeIsShort
	.long    TypeIsShort
	.word   68
	.space  4
	.global TypeIsChar
	.long    TypeIsChar
	.word   69
	.space  4
	.global TypeIsLong
	.long    TypeIsLong
	.word   73
	.space  4
	.global TypeIsLongLong
	.long    TypeIsLongLong
	.word   73
	.space  4
	.global TypeIsUnsignedInt
	.long    TypeIsUnsignedInt
	.word   70
	.space  4
	.global TypeIsUnsignedShort
	.long    TypeIsUnsignedShort
	.word   72
	.space  4
	.global TypeIsUnsignedChar
	.long    TypeIsUnsignedChar
	.word   71
	.space  4
	.global TypeIsFloat
	.long    TypeIsFloat
	.word   74
	.space  4
	.global TypeIsDouble
	.long    TypeIsDouble
	.word   75
	.space  4
	.global TypeIsBool
	.long    TypeIsBool
	.word   69
	.space  4
	.global TypeIsPointerOrArray
	.long    TypeIsPointerOrArray
	.word   66
	.space  4
	.global TypeIsFunction
	.long    TypeIsFunction
	.word   66
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
	.word   53
	.space  4
	.long   4
	.global TypeIsShort
	.long    TypeIsShort
	.word   54
	.space  4
	.long   2
	.global TypeIsChar
	.long    TypeIsChar
	.word   55
	.space  4
	.long   1
	.global TypeIsLong
	.long    TypeIsLong
	.word   58
	.space  4
	.long   8
	.global TypeIsLongLong
	.long    TypeIsLongLong
	.word   58
	.space  4
	.long   8
	.global TypeIsFloat
	.long    TypeIsFloat
	.word   56
	.space  4
	.long   4
	.global TypeIsDouble
	.long    TypeIsDouble
	.word   57
	.space  4
	.long   8
	.global TypeIsLongDouble
	.long    TypeIsLongDouble
	.word   57
	.space  4
	.long   8
	.global TypeIsPointerOrArray
	.long    TypeIsPointerOrArray
	.word   52
	.space  4
	.long   2
	.global TypeIsFunction
	.long    TypeIsFunction
	.word   52
	.space  4
	.long   2
	.global TypeIsStructOrUnion
	.long    TypeIsStructOrUnion
	.word   52
	.space  4
	.long   2
	.word   0
	.space  4
	.word   52
	.space  4
	.long   0

	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "movac"
	.type .str.1, @object
	.size .str.1, 6

.str.2:
	.asciz "mova"
	.type .str.2, @object
	.size .str.2, 5

.str.3:
	.asciz "rmova"
	.type .str.3, @object
	.size .str.3, 6

.str.4:
	.asciz "rmovx"
	.type .str.4, @object
	.size .str.4, 6

.str.5:
	.asciz "resulta"
	.type .str.5, @object
	.size .str.5, 8

.str.6:
	.asciz "resulti"
	.type .str.6, @object
	.size .str.6, 8

.str.7:
	.asciz "tmpb"
	.type .str.7, @object
	.size .str.7, 5

.str.8:
	.asciz "tmpa"
	.type .str.8, @object
	.size .str.8, 5

.str.9:
	.asciz "tmpx"
	.type .str.9, @object
	.size .str.9, 5

.str.10:
	.asciz "enter"
	.type .str.10, @object
	.size .str.10, 6

.str.11:
	.asciz "rts"
	.type .str.11, @object
	.size .str.11, 4

.str.12:
	.asciz "localvar"
	.type .str.12, @object
	.size .str.12, 9

.str.13:
	.asciz "argument"
	.type .str.13, @object
	.size .str.13, 9

.str.14:
	.asciz "ap"
	.type .str.14, @object
	.size .str.14, 3

.str.15:
	.asciz "decsp"
	.type .str.15, @object
	.size .str.15, 6

.str.16:
	.asciz "incsp"
	.type .str.16, @object
	.size .str.16, 6

.str.17:
	.asciz "pusha"
	.type .str.17, @object
	.size .str.17, 6

.str.18:
	.asciz "pushi"
	.type .str.18, @object
	.size .str.18, 6

.str.19:
	.asciz "pushh"
	.type .str.19, @object
	.size .str.19, 6

.str.20:
	.asciz "pushb"
	.type .str.20, @object
	.size .str.20, 6

.str.21:
	.asciz "pushf"
	.type .str.21, @object
	.size .str.21, 6

.str.22:
	.asciz "pushd"
	.type .str.22, @object
	.size .str.22, 6

.str.23:
	.asciz "pushx"
	.type .str.23, @object
	.size .str.23, 6

.str.24:
	.asciz "popa"
	.type .str.24, @object
	.size .str.24, 5

.str.25:
	.asciz "popi"
	.type .str.25, @object
	.size .str.25, 5

.str.26:
	.asciz "poph"
	.type .str.26, @object
	.size .str.26, 5

.str.27:
	.asciz "popb"
	.type .str.27, @object
	.size .str.27, 5

.str.28:
	.asciz "popf"
	.type .str.28, @object
	.size .str.28, 5

.str.29:
	.asciz "popd"
	.type .str.29, @object
	.size .str.29, 5

.str.30:
	.asciz "popx"
	.type .str.30, @object
	.size .str.30, 5

.str.31:
	.asciz "lda"
	.type .str.31, @object
	.size .str.31, 4

.str.32:
	.asciz "ldw"
	.type .str.32, @object
	.size .str.32, 4

.str.33:
	.asciz "ldh"
	.type .str.33, @object
	.size .str.33, 4

.str.34:
	.asciz "lduw"
	.type .str.34, @object
	.size .str.34, 5

.str.35:
	.asciz "ldub"
	.type .str.35, @object
	.size .str.35, 5

.str.36:
	.asciz "lduh"
	.type .str.36, @object
	.size .str.36, 5

.str.37:
	.asciz "ldf"
	.type .str.37, @object
	.size .str.37, 4

.str.38:
	.asciz "ldd"
	.type .str.38, @object
	.size .str.38, 4

.str.39:
	.asciz "ldb"
	.type .str.39, @object
	.size .str.39, 4

.str.40:
	.asciz "ldx"
	.type .str.40, @object
	.size .str.40, 4

.str.41:
	.asciz "sta"
	.type .str.41, @object
	.size .str.41, 4

.str.42:
	.asciz "stw"
	.type .str.42, @object
	.size .str.42, 4

.str.43:
	.asciz "sth"
	.type .str.43, @object
	.size .str.43, 4

.str.44:
	.asciz "stx"
	.type .str.44, @object
	.size .str.44, 4

.str.45:
	.asciz "stf"
	.type .str.45, @object
	.size .str.45, 4

.str.46:
	.asciz "std"
	.type .str.46, @object
	.size .str.46, 4

.str.47:
	.asciz "stb"
	.type .str.47, @object
	.size .str.47, 4

.str.48:
	.asciz "adda"
	.type .str.48, @object
	.size .str.48, 5

.str.49:
	.asciz "add"
	.type .str.49, @object
	.size .str.49, 4

.str.50:
	.asciz "addf"
	.type .str.50, @object
	.size .str.50, 5

.str.51:
	.asciz "addd"
	.type .str.51, @object
	.size .str.51, 5

.str.52:
	.asciz "addc"
	.type .str.52, @object
	.size .str.52, 5

.str.53:
	.asciz "addac"
	.type .str.53, @object
	.size .str.53, 6

.str.54:
	.asciz "sub"
	.type .str.54, @object
	.size .str.54, 4

.str.55:
	.asciz "subf"
	.type .str.55, @object
	.size .str.55, 5

.str.56:
	.asciz "subd"
	.type .str.56, @object
	.size .str.56, 5

.str.57:
	.asciz "mula"
	.type .str.57, @object
	.size .str.57, 5

.str.58:
	.asciz "mul"
	.type .str.58, @object
	.size .str.58, 4

.str.59:
	.asciz "mulf"
	.type .str.59, @object
	.size .str.59, 5

.str.60:
	.asciz "muld"
	.type .str.60, @object
	.size .str.60, 5

.str.61:
	.asciz "div"
	.type .str.61, @object
	.size .str.61, 4

.str.62:
	.asciz "divu"
	.type .str.62, @object
	.size .str.62, 5

.str.63:
	.asciz "divf"
	.type .str.63, @object
	.size .str.63, 5

.str.64:
	.asciz "divd"
	.type .str.64, @object
	.size .str.64, 5

.str.65:
	.asciz "mod"
	.type .str.65, @object
	.size .str.65, 4

.str.66:
	.asciz "modu"
	.type .str.66, @object
	.size .str.66, 5

.str.67:
	.asciz "lsr"
	.type .str.67, @object
	.size .str.67, 4

.str.68:
	.asciz "asr"
	.type .str.68, @object
	.size .str.68, 4

.str.69:
	.asciz "lsl"
	.type .str.69, @object
	.size .str.69, 4

.str.70:
	.asciz "or"
	.type .str.70, @object
	.size .str.70, 3

.str.71:
	.asciz "and"
	.type .str.71, @object
	.size .str.71, 4

.str.72:
	.asciz "xor"
	.type .str.72, @object
	.size .str.72, 4

.str.73:
	.asciz "not"
	.type .str.73, @object
	.size .str.73, 4

.str.74:
	.asciz "inv"
	.type .str.74, @object
	.size .str.74, 4

.str.75:
	.asciz "neg"
	.type .str.75, @object
	.size .str.75, 4

.str.76:
	.asciz "negf"
	.type .str.76, @object
	.size .str.76, 5

.str.77:
	.asciz "negd"
	.type .str.77, @object
	.size .str.77, 5

.str.78:
	.asciz "cmpeqb"
	.type .str.78, @object
	.size .str.78, 7

.str.79:
	.asciz "cmpneb"
	.type .str.79, @object
	.size .str.79, 7

.str.80:
	.asciz "cmpltb"
	.type .str.80, @object
	.size .str.80, 7

.str.81:
	.asciz "cmpleb"
	.type .str.81, @object
	.size .str.81, 7

.str.82:
	.asciz "cmpgtb"
	.type .str.82, @object
	.size .str.82, 7

.str.83:
	.asciz "cmpgeb"
	.type .str.83, @object
	.size .str.83, 7

.str.84:
	.asciz "cmpltub"
	.type .str.84, @object
	.size .str.84, 8

.str.85:
	.asciz "cmpleub"
	.type .str.85, @object
	.size .str.85, 8

.str.86:
	.asciz "cmpgtub"
	.type .str.86, @object
	.size .str.86, 8

.str.87:
	.asciz "cmpgeub"
	.type .str.87, @object
	.size .str.87, 8

.str.88:
	.asciz "cmpeqa"
	.type .str.88, @object
	.size .str.88, 7

.str.89:
	.asciz "cmpnea"
	.type .str.89, @object
	.size .str.89, 7

.str.90:
	.asciz "cmplta"
	.type .str.90, @object
	.size .str.90, 7

.str.91:
	.asciz "cmplea"
	.type .str.91, @object
	.size .str.91, 7

.str.92:
	.asciz "cmpgta"
	.type .str.92, @object
	.size .str.92, 7

.str.93:
	.asciz "cmpgea"
	.type .str.93, @object
	.size .str.93, 7

.str.94:
	.asciz "cmpltua"
	.type .str.94, @object
	.size .str.94, 8

.str.95:
	.asciz "cmpleua"
	.type .str.95, @object
	.size .str.95, 8

.str.96:
	.asciz "cmpgtua"
	.type .str.96, @object
	.size .str.96, 8

.str.97:
	.asciz "cmpgeua"
	.type .str.97, @object
	.size .str.97, 8

.str.98:
	.asciz "cmpeqi"
	.type .str.98, @object
	.size .str.98, 7

.str.99:
	.asciz "cmpnei"
	.type .str.99, @object
	.size .str.99, 7

.str.100:
	.asciz "cmplti"
	.type .str.100, @object
	.size .str.100, 7

.str.101:
	.asciz "cmplei"
	.type .str.101, @object
	.size .str.101, 7

.str.102:
	.asciz "cmpgti"
	.type .str.102, @object
	.size .str.102, 7

.str.103:
	.asciz "cmpgei"
	.type .str.103, @object
	.size .str.103, 7

.str.104:
	.asciz "cmpltui"
	.type .str.104, @object
	.size .str.104, 8

.str.105:
	.asciz "cmpleui"
	.type .str.105, @object
	.size .str.105, 8

.str.106:
	.asciz "cmpgtui"
	.type .str.106, @object
	.size .str.106, 8

.str.107:
	.asciz "cmpgeui"
	.type .str.107, @object
	.size .str.107, 8

.str.108:
	.asciz "cmpeqx"
	.type .str.108, @object
	.size .str.108, 7

.str.109:
	.asciz "cmpnex"
	.type .str.109, @object
	.size .str.109, 7

.str.110:
	.asciz "cmpltx"
	.type .str.110, @object
	.size .str.110, 7

.str.111:
	.asciz "cmplex"
	.type .str.111, @object
	.size .str.111, 7

.str.112:
	.asciz "cmpgtx"
	.type .str.112, @object
	.size .str.112, 7

.str.113:
	.asciz "cmpgex"
	.type .str.113, @object
	.size .str.113, 7

.str.114:
	.asciz "cmpltux"
	.type .str.114, @object
	.size .str.114, 8

.str.115:
	.asciz "cmpleux"
	.type .str.115, @object
	.size .str.115, 8

.str.116:
	.asciz "cmpgtux"
	.type .str.116, @object
	.size .str.116, 8

.str.117:
	.asciz "cmpgeux"
	.type .str.117, @object
	.size .str.117, 8

.str.118:
	.asciz "cmpeqf"
	.type .str.118, @object
	.size .str.118, 7

.str.119:
	.asciz "cmpnef"
	.type .str.119, @object
	.size .str.119, 7

.str.120:
	.asciz "cmpltf"
	.type .str.120, @object
	.size .str.120, 7

.str.121:
	.asciz "cmplef"
	.type .str.121, @object
	.size .str.121, 7

.str.122:
	.asciz "cmpgtf"
	.type .str.122, @object
	.size .str.122, 7

.str.123:
	.asciz "cmpgef"
	.type .str.123, @object
	.size .str.123, 7

.str.124:
	.asciz "cmpeqd"
	.type .str.124, @object
	.size .str.124, 7

.str.125:
	.asciz "cmpned"
	.type .str.125, @object
	.size .str.125, 7

.str.126:
	.asciz "cmpltd"
	.type .str.126, @object
	.size .str.126, 7

.str.127:
	.asciz "cmpled"
	.type .str.127, @object
	.size .str.127, 7

.str.128:
	.asciz "cmpgtd"
	.type .str.128, @object
	.size .str.128, 7

.str.129:
	.asciz "cmpged"
	.type .str.129, @object
	.size .str.129, 7

.str.130:
	.asciz "bt"
	.type .str.130, @object
	.size .str.130, 3

.str.131:
	.asciz "bf"
	.type .str.131, @object
	.size .str.131, 3

.str.132:
	.asciz "bra"
	.type .str.132, @object
	.size .str.132, 4

.str.133:
	.asciz "cbra"
	.type .str.133, @object
	.size .str.133, 5

.str.134:
	.asciz "i2f"
	.type .str.134, @object
	.size .str.134, 4

.str.135:
	.asciz "i2d"
	.type .str.135, @object
	.size .str.135, 4

.str.136:
	.asciz "ui2f"
	.type .str.136, @object
	.size .str.136, 5

.str.137:
	.asciz "ui2d"
	.type .str.137, @object
	.size .str.137, 5

.str.138:
	.asciz "f2d"
	.type .str.138, @object
	.size .str.138, 4

.str.139:
	.asciz "d2f"
	.type .str.139, @object
	.size .str.139, 4

.str.140:
	.asciz "f2i"
	.type .str.140, @object
	.size .str.140, 4

.str.141:
	.asciz "d2i"
	.type .str.141, @object
	.size .str.141, 4

.str.142:
	.asciz "f2ui"
	.type .str.142, @object
	.size .str.142, 5

.str.143:
	.asciz "d2ui"
	.type .str.143, @object
	.size .str.143, 5

.str.144:
	.asciz "jmp"
	.type .str.144, @object
	.size .str.144, 4

.str.145:
	.asciz "cjmp"
	.type .str.145, @object
	.size .str.145, 5

.str.146:
	.asciz "adr"
	.type .str.146, @object
	.size .str.146, 4

.str.147:
	.asciz "adrs"
	.type .str.147, @object
	.size .str.147, 5

.str.148:
	.asciz "adrtls"
	.type .str.148, @object
	.size .str.148, 7

.str.149:
	.asciz "calla"
	.type .str.149, @object
	.size .str.149, 6

.str.150:
	.asciz "calli"
	.type .str.150, @object
	.size .str.150, 6

.str.151:
	.asciz "callx"
	.type .str.151, @object
	.size .str.151, 6

.str.152:
	.asciz "callf"
	.type .str.152, @object
	.size .str.152, 6

.str.153:
	.asciz "calld"
	.type .str.153, @object
	.size .str.153, 6

.str.154:
	.asciz "rcalla"
	.type .str.154, @object
	.size .str.154, 7

.str.155:
	.asciz "rcalli"
	.type .str.155, @object
	.size .str.155, 7

.str.156:
	.asciz "rcallx"
	.type .str.156, @object
	.size .str.156, 7

.str.157:
	.asciz "rcallf"
	.type .str.157, @object
	.size .str.157, 7

.str.158:
	.asciz "rcalld"
	.type .str.158, @object
	.size .str.158, 7

.str.159:
	.asciz "ret"
	.type .str.159, @object
	.size .str.159, 4

.str.160:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.160, @object
	.size .str.160, 30

.str.161:
	.asciz "(null)"
	.type .str.161, @object
	.size .str.161, 1

.str.162:
	.asciz "opcode != 0"
	.type .str.162, @object
	.size .str.162, 12

.str.163:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.163, @object
	.size .str.163, 30

.str.164:
	.asciz "(null)"
	.type .str.164, @object
	.size .str.164, 1

.str.165:
	.asciz "false"
	.type .str.165, @object
	.size .str.165, 6

.str.166:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.166, @object
	.size .str.166, 30

.str.167:
	.asciz "(null)"
	.type .str.167, @object
	.size .str.167, 1

.str.168:
	.asciz "false"
	.type .str.168, @object
	.size .str.168, 6

.str.169:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.169, @object
	.size .str.169, 30

.str.170:
	.asciz "(null)"
	.type .str.170, @object
	.size .str.170, 1

.str.171:
	.asciz "node->inputs.length == 2"
	.type .str.171, @object
	.size .str.171, 25

.str.172:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.172, @object
	.size .str.172, 30

.str.173:
	.asciz "(null)"
	.type .str.173, @object
	.size .str.173, 1

.str.174:
	.asciz "node->inputs.length == 2"
	.type .str.174, @object
	.size .str.174, 25

.str.175:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.175, @object
	.size .str.175, 30

.str.176:
	.asciz "(null)"
	.type .str.176, @object
	.size .str.176, 1

.str.177:
	.asciz "node->inputs.length == 2"
	.type .str.177, @object
	.size .str.177, 25

.str.178:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.178, @object
	.size .str.178, 30

.str.179:
	.asciz "(null)"
	.type .str.179, @object
	.size .str.179, 1

.str.180:
	.asciz "node->inputs.length == 2"
	.type .str.180, @object
	.size .str.180, 25

.str.181:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.181, @object
	.size .str.181, 30

.str.182:
	.asciz "(null)"
	.type .str.182, @object
	.size .str.182, 1

.str.183:
	.asciz "node->inputs.length == 2"
	.type .str.183, @object
	.size .str.183, 25

.str.184:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.184, @object
	.size .str.184, 30

.str.185:
	.asciz "(null)"
	.type .str.185, @object
	.size .str.185, 1

.str.186:
	.asciz "node->inputs.length <= 2"
	.type .str.186, @object
	.size .str.186, 25

.str.187:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.187, @object
	.size .str.187, 30

.str.188:
	.asciz "(null)"
	.type .str.188, @object
	.size .str.188, 1

.str.189:
	.asciz "false"
	.type .str.189, @object
	.size .str.189, 6

.str.190:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.190, @object
	.size .str.190, 30

.str.191:
	.asciz "(null)"
	.type .str.191, @object
	.size .str.191, 1

.str.192:
	.asciz "node->inputs.length == 2"
	.type .str.192, @object
	.size .str.192, 25

.str.193:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.193, @object
	.size .str.193, 30

.str.194:
	.asciz "(null)"
	.type .str.194, @object
	.size .str.194, 1

.str.195:
	.asciz "node->inputs.length == 1"
	.type .str.195, @object
	.size .str.195, 25

.str.196:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.196, @object
	.size .str.196, 30

.str.197:
	.asciz "(null)"
	.type .str.197, @object
	.size .str.197, 1

.str.198:
	.asciz "node->inputs.length == 1"
	.type .str.198, @object
	.size .str.198, 25

.str.199:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.199, @object
	.size .str.199, 30

.str.200:
	.asciz "(null)"
	.type .str.200, @object
	.size .str.200, 1

.str.201:
	.asciz "node->inputs.length == 1"
	.type .str.201, @object
	.size .str.201, 25

.str.202:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.202, @object
	.size .str.202, 30

.str.203:
	.asciz "(null)"
	.type .str.203, @object
	.size .str.203, 1

.str.204:
	.asciz "false"
	.type .str.204, @object
	.size .str.204, 6

.str.205:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.205, @object
	.size .str.205, 30

.str.206:
	.asciz "(null)"
	.type .str.206, @object
	.size .str.206, 1

.str.207:
	.asciz "node->inputs.length == 2"
	.type .str.207, @object
	.size .str.207, 25

.str.208:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.208, @object
	.size .str.208, 30

.str.209:
	.asciz "(null)"
	.type .str.209, @object
	.size .str.209, 1

.str.210:
	.asciz "false"
	.type .str.210, @object
	.size .str.210, 6

.str.211:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.211, @object
	.size .str.211, 30

.str.212:
	.asciz "(null)"
	.type .str.212, @object
	.size .str.212, 1

.str.213:
	.asciz "false"
	.type .str.213, @object
	.size .str.213, 6

.str.214:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.214, @object
	.size .str.214, 30

.str.215:
	.asciz "(null)"
	.type .str.215, @object
	.size .str.215, 1

.str.216:
	.asciz "node->inputs.length >= 1"
	.type .str.216, @object
	.size .str.216, 25

.str.217:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.217, @object
	.size .str.217, 30

.str.218:
	.asciz "(null)"
	.type .str.218, @object
	.size .str.218, 1

.str.219:
	.asciz "node->inputs.length == 1"
	.type .str.219, @object
	.size .str.219, 25

.str.220:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.220, @object
	.size .str.220, 30

.str.221:
	.asciz "(null)"
	.type .str.221, @object
	.size .str.221, 1

.str.222:
	.asciz "false"
	.type .str.222, @object
	.size .str.222, 6

.str.223:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.223, @object
	.size .str.223, 30

.str.224:
	.asciz "(null)"
	.type .str.224, @object
	.size .str.224, 1

.str.225:
	.asciz "node->inputs.length == 3"
	.type .str.225, @object
	.size .str.225, 25

.str.226:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.226, @object
	.size .str.226, 30

.str.227:
	.asciz "(null)"
	.type .str.227, @object
	.size .str.227, 1

.str.228:
	.asciz "node->inputs.length == 1"
	.type .str.228, @object
	.size .str.228, 25

.str.229:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.229, @object
	.size .str.229, 30

.str.230:
	.asciz "(null)"
	.type .str.230, @object
	.size .str.230, 1

.str.231:
	.asciz "false"
	.type .str.231, @object
	.size .str.231, 6


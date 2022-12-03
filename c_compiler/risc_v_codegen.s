	.file   "risc_v_codegen.c"
	.text
	.option pic
.PCbegin:
	.local  Trap
	.type Trap, @function

Trap:

	// *** Basic block 0

	ret         
.func_end_Trap:
	.size Trap, .func_end_Trap-Trap

	.local  TrapLower
	.type TrapLower, @function

TrapLower:

	// *** Basic block 0

	.global StringEqual
	.local Trap
	addi sp, sp, -16
	// Saved return address (offset 8) and frame pointer (offset 0)
	sd ra, 8(sp)
	sd s0, 0(sp)
	addi s0, sp, 16
	// Local vars at offset -16(s0)
	// End of stack frame
	lla         a1, .str.1
	call        StringEqual

	// *** Basic block 1

	beqz        a0, .TrapLower_label_18

	// *** Basic block 2

	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           Trap

	// *** Basic block 3

.TrapLower_label_18:
	// Restored registers.
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_TrapLower:
	.size TrapLower, .func_end_TrapLower-TrapLower

	.global RVOpcodeName
	.type RVOpcodeName, @function

RVOpcodeName:

	// *** Basic block 0

	.global TargetOpcodeName
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	li          t1, 11		// 0xb ASCII \xb
	blt         t0, t1, .RVOpcodeName_label_405

	// *** Basic block 1

	li          t1, 211		// 0xd3 ASCII \xd3
	blt         t1, t0, .RVOpcodeName_label_405

	// *** Basic block 2

	addi        t1, t0, -11
	slli        t1, t1, 2
	auipc       t2, 0
	add         t1, t2, t1
	jalr        x0, t1, 12

	// *** Basic block 3

	j           .RVOpcodeName_label_415

	// *** Basic block 4

	j           .RVOpcodeName_label_420

	// *** Basic block 5

	j           .RVOpcodeName_label_425

	// *** Basic block 6

	j           .RVOpcodeName_label_405

	// *** Basic block 7

	j           .RVOpcodeName_label_405

	// *** Basic block 8

	j           .RVOpcodeName_label_405

	// *** Basic block 9

	j           .RVOpcodeName_label_405

	// *** Basic block 10

	j           .RVOpcodeName_label_405

	// *** Basic block 11

	j           .RVOpcodeName_label_405

	// *** Basic block 12

	j           .RVOpcodeName_label_405

	// *** Basic block 13

	j           .RVOpcodeName_label_405

	// *** Basic block 14

	j           .RVOpcodeName_label_405

	// *** Basic block 15

	j           .RVOpcodeName_label_405

	// *** Basic block 16

	j           .RVOpcodeName_label_405

	// *** Basic block 17

	j           .RVOpcodeName_label_405

	// *** Basic block 18

	j           .RVOpcodeName_label_405

	// *** Basic block 19

	j           .RVOpcodeName_label_405

	// *** Basic block 20

	j           .RVOpcodeName_label_405

	// *** Basic block 21

	j           .RVOpcodeName_label_405

	// *** Basic block 22

	j           .RVOpcodeName_label_405

	// *** Basic block 23

	j           .RVOpcodeName_label_405

	// *** Basic block 24

	j           .RVOpcodeName_label_405

	// *** Basic block 25

	j           .RVOpcodeName_label_405

	// *** Basic block 26

	j           .RVOpcodeName_label_405

	// *** Basic block 27

	j           .RVOpcodeName_label_430

	// *** Basic block 28

	j           .RVOpcodeName_label_435

	// *** Basic block 29

	j           .RVOpcodeName_label_440

	// *** Basic block 30

	j           .RVOpcodeName_label_445

	// *** Basic block 31

	j           .RVOpcodeName_label_450

	// *** Basic block 32

	j           .RVOpcodeName_label_455

	// *** Basic block 33

	j           .RVOpcodeName_label_460

	// *** Basic block 34

	j           .RVOpcodeName_label_465

	// *** Basic block 35

	j           .RVOpcodeName_label_470

	// *** Basic block 36

	j           .RVOpcodeName_label_475

	// *** Basic block 37

	j           .RVOpcodeName_label_480

	// *** Basic block 38

	j           .RVOpcodeName_label_485

	// *** Basic block 39

	j           .RVOpcodeName_label_490

	// *** Basic block 40

	j           .RVOpcodeName_label_495

	// *** Basic block 41

	j           .RVOpcodeName_label_500

	// *** Basic block 42

	j           .RVOpcodeName_label_505

	// *** Basic block 43

	j           .RVOpcodeName_label_510

	// *** Basic block 44

	j           .RVOpcodeName_label_515

	// *** Basic block 45

	j           .RVOpcodeName_label_520

	// *** Basic block 46

	j           .RVOpcodeName_label_525

	// *** Basic block 47

	j           .RVOpcodeName_label_530

	// *** Basic block 48

	j           .RVOpcodeName_label_535

	// *** Basic block 49

	j           .RVOpcodeName_label_540

	// *** Basic block 50

	j           .RVOpcodeName_label_545

	// *** Basic block 51

	j           .RVOpcodeName_label_550

	// *** Basic block 52

	j           .RVOpcodeName_label_555

	// *** Basic block 53

	j           .RVOpcodeName_label_560

	// *** Basic block 54

	j           .RVOpcodeName_label_565

	// *** Basic block 55

	j           .RVOpcodeName_label_570

	// *** Basic block 56

	j           .RVOpcodeName_label_575

	// *** Basic block 57

	j           .RVOpcodeName_label_580

	// *** Basic block 58

	j           .RVOpcodeName_label_585

	// *** Basic block 59

	j           .RVOpcodeName_label_590

	// *** Basic block 60

	j           .RVOpcodeName_label_595

	// *** Basic block 61

	j           .RVOpcodeName_label_600

	// *** Basic block 62

	j           .RVOpcodeName_label_605

	// *** Basic block 63

	j           .RVOpcodeName_label_610

	// *** Basic block 64

	j           .RVOpcodeName_label_615

	// *** Basic block 65

	j           .RVOpcodeName_label_620

	// *** Basic block 66

	j           .RVOpcodeName_label_625

	// *** Basic block 67

	j           .RVOpcodeName_label_630

	// *** Basic block 68

	j           .RVOpcodeName_label_635

	// *** Basic block 69

	j           .RVOpcodeName_label_640

	// *** Basic block 70

	j           .RVOpcodeName_label_645

	// *** Basic block 71

	j           .RVOpcodeName_label_650

	// *** Basic block 72

	j           .RVOpcodeName_label_655

	// *** Basic block 73

	j           .RVOpcodeName_label_660

	// *** Basic block 74

	j           .RVOpcodeName_label_665

	// *** Basic block 75

	j           .RVOpcodeName_label_670

	// *** Basic block 76

	j           .RVOpcodeName_label_675

	// *** Basic block 77

	j           .RVOpcodeName_label_680

	// *** Basic block 78

	j           .RVOpcodeName_label_685

	// *** Basic block 79

	j           .RVOpcodeName_label_690

	// *** Basic block 80

	j           .RVOpcodeName_label_695

	// *** Basic block 81

	j           .RVOpcodeName_label_700

	// *** Basic block 82

	j           .RVOpcodeName_label_705

	// *** Basic block 83

	j           .RVOpcodeName_label_710

	// *** Basic block 84

	j           .RVOpcodeName_label_715

	// *** Basic block 85

	j           .RVOpcodeName_label_720

	// *** Basic block 86

	j           .RVOpcodeName_label_725

	// *** Basic block 87

	j           .RVOpcodeName_label_730

	// *** Basic block 88

	j           .RVOpcodeName_label_735

	// *** Basic block 89

	j           .RVOpcodeName_label_740

	// *** Basic block 90

	j           .RVOpcodeName_label_745

	// *** Basic block 91

	j           .RVOpcodeName_label_750

	// *** Basic block 92

	j           .RVOpcodeName_label_755

	// *** Basic block 93

	j           .RVOpcodeName_label_760

	// *** Basic block 94

	j           .RVOpcodeName_label_765

	// *** Basic block 95

	j           .RVOpcodeName_label_770

	// *** Basic block 96

	j           .RVOpcodeName_label_775

	// *** Basic block 97

	j           .RVOpcodeName_label_780

	// *** Basic block 98

	j           .RVOpcodeName_label_785

	// *** Basic block 99

	j           .RVOpcodeName_label_790

	// *** Basic block 100

	j           .RVOpcodeName_label_795

	// *** Basic block 101

	j           .RVOpcodeName_label_800

	// *** Basic block 102

	j           .RVOpcodeName_label_805

	// *** Basic block 103

	j           .RVOpcodeName_label_810

	// *** Basic block 104

	j           .RVOpcodeName_label_815

	// *** Basic block 105

	j           .RVOpcodeName_label_820

	// *** Basic block 106

	j           .RVOpcodeName_label_825

	// *** Basic block 107

	j           .RVOpcodeName_label_830

	// *** Basic block 108

	j           .RVOpcodeName_label_835

	// *** Basic block 109

	j           .RVOpcodeName_label_840

	// *** Basic block 110

	j           .RVOpcodeName_label_845

	// *** Basic block 111

	j           .RVOpcodeName_label_850

	// *** Basic block 112

	j           .RVOpcodeName_label_855

	// *** Basic block 113

	j           .RVOpcodeName_label_860

	// *** Basic block 114

	j           .RVOpcodeName_label_865

	// *** Basic block 115

	j           .RVOpcodeName_label_870

	// *** Basic block 116

	j           .RVOpcodeName_label_875

	// *** Basic block 117

	j           .RVOpcodeName_label_880

	// *** Basic block 118

	j           .RVOpcodeName_label_885

	// *** Basic block 119

	j           .RVOpcodeName_label_890

	// *** Basic block 120

	j           .RVOpcodeName_label_895

	// *** Basic block 121

	j           .RVOpcodeName_label_900

	// *** Basic block 122

	j           .RVOpcodeName_label_905

	// *** Basic block 123

	j           .RVOpcodeName_label_910

	// *** Basic block 124

	j           .RVOpcodeName_label_915

	// *** Basic block 125

	j           .RVOpcodeName_label_920

	// *** Basic block 126

	j           .RVOpcodeName_label_925

	// *** Basic block 127

	j           .RVOpcodeName_label_930

	// *** Basic block 128

	j           .RVOpcodeName_label_935

	// *** Basic block 129

	j           .RVOpcodeName_label_940

	// *** Basic block 130

	j           .RVOpcodeName_label_945

	// *** Basic block 131

	j           .RVOpcodeName_label_950

	// *** Basic block 132

	j           .RVOpcodeName_label_955

	// *** Basic block 133

	j           .RVOpcodeName_label_960

	// *** Basic block 134

	j           .RVOpcodeName_label_965

	// *** Basic block 135

	j           .RVOpcodeName_label_970

	// *** Basic block 136

	j           .RVOpcodeName_label_975

	// *** Basic block 137

	j           .RVOpcodeName_label_980

	// *** Basic block 138

	j           .RVOpcodeName_label_985

	// *** Basic block 139

	j           .RVOpcodeName_label_990

	// *** Basic block 140

	j           .RVOpcodeName_label_995

	// *** Basic block 141

	j           .RVOpcodeName_label_1000

	// *** Basic block 142

	j           .RVOpcodeName_label_1005

	// *** Basic block 143

	j           .RVOpcodeName_label_1010

	// *** Basic block 144

	j           .RVOpcodeName_label_1015

	// *** Basic block 145

	j           .RVOpcodeName_label_1020

	// *** Basic block 146

	j           .RVOpcodeName_label_1025

	// *** Basic block 147

	j           .RVOpcodeName_label_1030

	// *** Basic block 148

	j           .RVOpcodeName_label_1035

	// *** Basic block 149

	j           .RVOpcodeName_label_1040

	// *** Basic block 150

	j           .RVOpcodeName_label_1045

	// *** Basic block 151

	j           .RVOpcodeName_label_1050

	// *** Basic block 152

	j           .RVOpcodeName_label_1055

	// *** Basic block 153

	j           .RVOpcodeName_label_1060

	// *** Basic block 154

	j           .RVOpcodeName_label_1065

	// *** Basic block 155

	j           .RVOpcodeName_label_1070

	// *** Basic block 156

	j           .RVOpcodeName_label_1075

	// *** Basic block 157

	j           .RVOpcodeName_label_1080

	// *** Basic block 158

	j           .RVOpcodeName_label_1085

	// *** Basic block 159

	j           .RVOpcodeName_label_1090

	// *** Basic block 160

	j           .RVOpcodeName_label_1095

	// *** Basic block 161

	j           .RVOpcodeName_label_1100

	// *** Basic block 162

	j           .RVOpcodeName_label_1105

	// *** Basic block 163

	j           .RVOpcodeName_label_1110

	// *** Basic block 164

	j           .RVOpcodeName_label_1115

	// *** Basic block 165

	j           .RVOpcodeName_label_1120

	// *** Basic block 166

	j           .RVOpcodeName_label_1125

	// *** Basic block 167

	j           .RVOpcodeName_label_1130

	// *** Basic block 168

	j           .RVOpcodeName_label_1135

	// *** Basic block 169

	j           .RVOpcodeName_label_1140

	// *** Basic block 170

	j           .RVOpcodeName_label_1145

	// *** Basic block 171

	j           .RVOpcodeName_label_1150

	// *** Basic block 172

	j           .RVOpcodeName_label_1155

	// *** Basic block 173

	j           .RVOpcodeName_label_1160

	// *** Basic block 174

	j           .RVOpcodeName_label_1165

	// *** Basic block 175

	j           .RVOpcodeName_label_1170

	// *** Basic block 176

	j           .RVOpcodeName_label_1175

	// *** Basic block 177

	j           .RVOpcodeName_label_1180

	// *** Basic block 178

	j           .RVOpcodeName_label_1185

	// *** Basic block 179

	j           .RVOpcodeName_label_1190

	// *** Basic block 180

	j           .RVOpcodeName_label_1195

	// *** Basic block 181

	j           .RVOpcodeName_label_1200

	// *** Basic block 182

	j           .RVOpcodeName_label_1205

	// *** Basic block 183

	j           .RVOpcodeName_label_1210

	// *** Basic block 184

	j           .RVOpcodeName_label_1215

	// *** Basic block 185

	j           .RVOpcodeName_label_1220

	// *** Basic block 186

	j           .RVOpcodeName_label_1225

	// *** Basic block 187

	j           .RVOpcodeName_label_1230

	// *** Basic block 188

	j           .RVOpcodeName_label_1235

	// *** Basic block 189

	j           .RVOpcodeName_label_1240

	// *** Basic block 190

	j           .RVOpcodeName_label_1245

	// *** Basic block 191

	j           .RVOpcodeName_label_1250

	// *** Basic block 192

	j           .RVOpcodeName_label_1255

	// *** Basic block 193

	j           .RVOpcodeName_label_1260

	// *** Basic block 194

	j           .RVOpcodeName_label_1265

	// *** Basic block 195

	j           .RVOpcodeName_label_1270

	// *** Basic block 196

	j           .RVOpcodeName_label_1275

	// *** Basic block 197

	j           .RVOpcodeName_label_1280

	// *** Basic block 198

	j           .RVOpcodeName_label_1290

	// *** Basic block 199

	j           .RVOpcodeName_label_1295

	// *** Basic block 200

	j           .RVOpcodeName_label_1300

	// *** Basic block 201

	j           .RVOpcodeName_label_1285

	// *** Basic block 202

	j           .RVOpcodeName_label_1305

	// *** Basic block 203

	j           .RVOpcodeName_label_1310

	// *** Basic block 204

.RVOpcodeName_label_405:
	mv          a0, t0
	j           TargetOpcodeName

	// *** Basic block 207

.RVOpcodeName_label_415:
	lla         a0, .str.2
	ret         

	// *** Basic block 208

.RVOpcodeName_label_420:
	lla         a0, .str.3
	ret         

	// *** Basic block 209

.RVOpcodeName_label_425:
	lla         a0, .str.4
	ret         

	// *** Basic block 210

.RVOpcodeName_label_430:
	lla         a0, .str.5
	ret         

	// *** Basic block 211

.RVOpcodeName_label_435:
	lla         a0, .str.6
	ret         

	// *** Basic block 212

.RVOpcodeName_label_440:
	lla         a0, .str.7
	ret         

	// *** Basic block 213

.RVOpcodeName_label_445:
	lla         a0, .str.8
	ret         

	// *** Basic block 214

.RVOpcodeName_label_450:
	lla         a0, .str.9
	ret         

	// *** Basic block 215

.RVOpcodeName_label_455:
	lla         a0, .str.10
	ret         

	// *** Basic block 216

.RVOpcodeName_label_460:
	lla         a0, .str.11
	ret         

	// *** Basic block 217

.RVOpcodeName_label_465:
	lla         a0, .str.12
	ret         

	// *** Basic block 218

.RVOpcodeName_label_470:
	lla         a0, .str.13
	ret         

	// *** Basic block 219

.RVOpcodeName_label_475:
	lla         a0, .str.14
	ret         

	// *** Basic block 220

.RVOpcodeName_label_480:
	lla         a0, .str.15
	ret         

	// *** Basic block 221

.RVOpcodeName_label_485:
	lla         a0, .str.16
	ret         

	// *** Basic block 222

.RVOpcodeName_label_490:
	lla         a0, .str.17
	ret         

	// *** Basic block 223

.RVOpcodeName_label_495:
	lla         a0, .str.18
	ret         

	// *** Basic block 224

.RVOpcodeName_label_500:
	lla         a0, .str.19
	ret         

	// *** Basic block 225

.RVOpcodeName_label_505:
	lla         a0, .str.20
	ret         

	// *** Basic block 226

.RVOpcodeName_label_510:
	lla         a0, .str.21
	ret         

	// *** Basic block 227

.RVOpcodeName_label_515:
	lla         a0, .str.22
	ret         

	// *** Basic block 228

.RVOpcodeName_label_520:
	lla         a0, .str.23
	ret         

	// *** Basic block 229

.RVOpcodeName_label_525:
	lla         a0, .str.24
	ret         

	// *** Basic block 230

.RVOpcodeName_label_530:
	lla         a0, .str.25
	ret         

	// *** Basic block 231

.RVOpcodeName_label_535:
	lla         a0, .str.26
	ret         

	// *** Basic block 232

.RVOpcodeName_label_540:
	lla         a0, .str.27
	ret         

	// *** Basic block 233

.RVOpcodeName_label_545:
	lla         a0, .str.28
	ret         

	// *** Basic block 234

.RVOpcodeName_label_550:
	lla         a0, .str.29
	ret         

	// *** Basic block 235

.RVOpcodeName_label_555:
	lla         a0, .str.30
	ret         

	// *** Basic block 236

.RVOpcodeName_label_560:
	lla         a0, .str.31
	ret         

	// *** Basic block 237

.RVOpcodeName_label_565:
	lla         a0, .str.32
	ret         

	// *** Basic block 238

.RVOpcodeName_label_570:
	lla         a0, .str.33
	ret         

	// *** Basic block 239

.RVOpcodeName_label_575:
	lla         a0, .str.34
	ret         

	// *** Basic block 240

.RVOpcodeName_label_580:
	lla         a0, .str.35
	ret         

	// *** Basic block 241

.RVOpcodeName_label_585:
	lla         a0, .str.36
	ret         

	// *** Basic block 242

.RVOpcodeName_label_590:
	lla         a0, .str.37
	ret         

	// *** Basic block 243

.RVOpcodeName_label_595:
	lla         a0, .str.38
	ret         

	// *** Basic block 244

.RVOpcodeName_label_600:
	lla         a0, .str.39
	ret         

	// *** Basic block 245

.RVOpcodeName_label_605:
	lla         a0, .str.40
	ret         

	// *** Basic block 246

.RVOpcodeName_label_610:
	lla         a0, .str.41
	ret         

	// *** Basic block 247

.RVOpcodeName_label_615:
	lla         a0, .str.42
	ret         

	// *** Basic block 248

.RVOpcodeName_label_620:
	lla         a0, .str.43
	ret         

	// *** Basic block 249

.RVOpcodeName_label_625:
	lla         a0, .str.44
	ret         

	// *** Basic block 250

.RVOpcodeName_label_630:
	lla         a0, .str.45
	ret         

	// *** Basic block 251

.RVOpcodeName_label_635:
	lla         a0, .str.46
	ret         

	// *** Basic block 252

.RVOpcodeName_label_640:
	lla         a0, .str.47
	ret         

	// *** Basic block 253

.RVOpcodeName_label_645:
	lla         a0, .str.48
	ret         

	// *** Basic block 254

.RVOpcodeName_label_650:
	lla         a0, .str.49
	ret         

	// *** Basic block 255

.RVOpcodeName_label_655:
	lla         a0, .str.50
	ret         

	// *** Basic block 256

.RVOpcodeName_label_660:
	lla         a0, .str.51
	ret         

	// *** Basic block 257

.RVOpcodeName_label_665:
	lla         a0, .str.52
	ret         

	// *** Basic block 258

.RVOpcodeName_label_670:
	lla         a0, .str.53
	ret         

	// *** Basic block 259

.RVOpcodeName_label_675:
	lla         a0, .str.54
	ret         

	// *** Basic block 260

.RVOpcodeName_label_680:
	lla         a0, .str.55
	ret         

	// *** Basic block 261

.RVOpcodeName_label_685:
	lla         a0, .str.56
	ret         

	// *** Basic block 262

.RVOpcodeName_label_690:
	lla         a0, .str.57
	ret         

	// *** Basic block 263

.RVOpcodeName_label_695:
	lla         a0, .str.58
	ret         

	// *** Basic block 264

.RVOpcodeName_label_700:
	lla         a0, .str.59
	ret         

	// *** Basic block 265

.RVOpcodeName_label_705:
	lla         a0, .str.60
	ret         

	// *** Basic block 266

.RVOpcodeName_label_710:
	lla         a0, .str.61
	ret         

	// *** Basic block 267

.RVOpcodeName_label_715:
	lla         a0, .str.62
	ret         

	// *** Basic block 268

.RVOpcodeName_label_720:
	lla         a0, .str.63
	ret         

	// *** Basic block 269

.RVOpcodeName_label_725:
	lla         a0, .str.64
	ret         

	// *** Basic block 270

.RVOpcodeName_label_730:
	lla         a0, .str.65
	ret         

	// *** Basic block 271

.RVOpcodeName_label_735:
	lla         a0, .str.66
	ret         

	// *** Basic block 272

.RVOpcodeName_label_740:
	lla         a0, .str.67
	ret         

	// *** Basic block 273

.RVOpcodeName_label_745:
	lla         a0, .str.68
	ret         

	// *** Basic block 274

.RVOpcodeName_label_750:
	lla         a0, .str.69
	ret         

	// *** Basic block 275

.RVOpcodeName_label_755:
	lla         a0, .str.70
	ret         

	// *** Basic block 276

.RVOpcodeName_label_760:
	lla         a0, .str.71
	ret         

	// *** Basic block 277

.RVOpcodeName_label_765:
	lla         a0, .str.72
	ret         

	// *** Basic block 278

.RVOpcodeName_label_770:
	lla         a0, .str.73
	ret         

	// *** Basic block 279

.RVOpcodeName_label_775:
	lla         a0, .str.74
	ret         

	// *** Basic block 280

.RVOpcodeName_label_780:
	lla         a0, .str.75
	ret         

	// *** Basic block 281

.RVOpcodeName_label_785:
	lla         a0, .str.76
	ret         

	// *** Basic block 282

.RVOpcodeName_label_790:
	lla         a0, .str.77
	ret         

	// *** Basic block 283

.RVOpcodeName_label_795:
	lla         a0, .str.78
	ret         

	// *** Basic block 284

.RVOpcodeName_label_800:
	lla         a0, .str.79
	ret         

	// *** Basic block 285

.RVOpcodeName_label_805:
	lla         a0, .str.80
	ret         

	// *** Basic block 286

.RVOpcodeName_label_810:
	lla         a0, .str.81
	ret         

	// *** Basic block 287

.RVOpcodeName_label_815:
	lla         a0, .str.82
	ret         

	// *** Basic block 288

.RVOpcodeName_label_820:
	lla         a0, .str.83
	ret         

	// *** Basic block 289

.RVOpcodeName_label_825:
	lla         a0, .str.84
	ret         

	// *** Basic block 290

.RVOpcodeName_label_830:
	lla         a0, .str.85
	ret         

	// *** Basic block 291

.RVOpcodeName_label_835:
	lla         a0, .str.86
	ret         

	// *** Basic block 292

.RVOpcodeName_label_840:
	lla         a0, .str.87
	ret         

	// *** Basic block 293

.RVOpcodeName_label_845:
	lla         a0, .str.88
	ret         

	// *** Basic block 294

.RVOpcodeName_label_850:
	lla         a0, .str.89
	ret         

	// *** Basic block 295

.RVOpcodeName_label_855:
	lla         a0, .str.90
	ret         

	// *** Basic block 296

.RVOpcodeName_label_860:
	lla         a0, .str.91
	ret         

	// *** Basic block 297

.RVOpcodeName_label_865:
	lla         a0, .str.92
	ret         

	// *** Basic block 298

.RVOpcodeName_label_870:
	lla         a0, .str.93
	ret         

	// *** Basic block 299

.RVOpcodeName_label_875:
	lla         a0, .str.94
	ret         

	// *** Basic block 300

.RVOpcodeName_label_880:
	lla         a0, .str.95
	ret         

	// *** Basic block 301

.RVOpcodeName_label_885:
	lla         a0, .str.96
	ret         

	// *** Basic block 302

.RVOpcodeName_label_890:
	lla         a0, .str.97
	ret         

	// *** Basic block 303

.RVOpcodeName_label_895:
	lla         a0, .str.98
	ret         

	// *** Basic block 304

.RVOpcodeName_label_900:
	lla         a0, .str.99
	ret         

	// *** Basic block 305

.RVOpcodeName_label_905:
	lla         a0, .str.100
	ret         

	// *** Basic block 306

.RVOpcodeName_label_910:
	lla         a0, .str.101
	ret         

	// *** Basic block 307

.RVOpcodeName_label_915:
	lla         a0, .str.102
	ret         

	// *** Basic block 308

.RVOpcodeName_label_920:
	lla         a0, .str.103
	ret         

	// *** Basic block 309

.RVOpcodeName_label_925:
	lla         a0, .str.104
	ret         

	// *** Basic block 310

.RVOpcodeName_label_930:
	lla         a0, .str.105
	ret         

	// *** Basic block 311

.RVOpcodeName_label_935:
	lla         a0, .str.106
	ret         

	// *** Basic block 312

.RVOpcodeName_label_940:
	lla         a0, .str.107
	ret         

	// *** Basic block 313

.RVOpcodeName_label_945:
	lla         a0, .str.108
	ret         

	// *** Basic block 314

.RVOpcodeName_label_950:
	lla         a0, .str.109
	ret         

	// *** Basic block 315

.RVOpcodeName_label_955:
	lla         a0, .str.110
	ret         

	// *** Basic block 316

.RVOpcodeName_label_960:
	lla         a0, .str.111
	ret         

	// *** Basic block 317

.RVOpcodeName_label_965:
	lla         a0, .str.112
	ret         

	// *** Basic block 318

.RVOpcodeName_label_970:
	lla         a0, .str.113
	ret         

	// *** Basic block 319

.RVOpcodeName_label_975:
	lla         a0, .str.114
	ret         

	// *** Basic block 320

.RVOpcodeName_label_980:
	lla         a0, .str.115
	ret         

	// *** Basic block 321

.RVOpcodeName_label_985:
	lla         a0, .str.116
	ret         

	// *** Basic block 322

.RVOpcodeName_label_990:
	lla         a0, .str.117
	ret         

	// *** Basic block 323

.RVOpcodeName_label_995:
	lla         a0, .str.118
	ret         

	// *** Basic block 324

.RVOpcodeName_label_1000:
	lla         a0, .str.119
	ret         

	// *** Basic block 325

.RVOpcodeName_label_1005:
	lla         a0, .str.120
	ret         

	// *** Basic block 326

.RVOpcodeName_label_1010:
	lla         a0, .str.121
	ret         

	// *** Basic block 327

.RVOpcodeName_label_1015:
	lla         a0, .str.122
	ret         

	// *** Basic block 328

.RVOpcodeName_label_1020:
	lla         a0, .str.123
	ret         

	// *** Basic block 329

.RVOpcodeName_label_1025:
	lla         a0, .str.124
	ret         

	// *** Basic block 330

.RVOpcodeName_label_1030:
	lla         a0, .str.125
	ret         

	// *** Basic block 331

.RVOpcodeName_label_1035:
	lla         a0, .str.126
	ret         

	// *** Basic block 332

.RVOpcodeName_label_1040:
	lla         a0, .str.127
	ret         

	// *** Basic block 333

.RVOpcodeName_label_1045:
	lla         a0, .str.128
	ret         

	// *** Basic block 334

.RVOpcodeName_label_1050:
	lla         a0, .str.129
	ret         

	// *** Basic block 335

.RVOpcodeName_label_1055:
	lla         a0, .str.130
	ret         

	// *** Basic block 336

.RVOpcodeName_label_1060:
	lla         a0, .str.131
	ret         

	// *** Basic block 337

.RVOpcodeName_label_1065:
	lla         a0, .str.132
	ret         

	// *** Basic block 338

.RVOpcodeName_label_1070:
	lla         a0, .str.133
	ret         

	// *** Basic block 339

.RVOpcodeName_label_1075:
	lla         a0, .str.134
	ret         

	// *** Basic block 340

.RVOpcodeName_label_1080:
	lla         a0, .str.135
	ret         

	// *** Basic block 341

.RVOpcodeName_label_1085:
	lla         a0, .str.136
	ret         

	// *** Basic block 342

.RVOpcodeName_label_1090:
	lla         a0, .str.137
	ret         

	// *** Basic block 343

.RVOpcodeName_label_1095:
	lla         a0, .str.138
	ret         

	// *** Basic block 344

.RVOpcodeName_label_1100:
	lla         a0, .str.139
	ret         

	// *** Basic block 345

.RVOpcodeName_label_1105:
	lla         a0, .str.140
	ret         

	// *** Basic block 346

.RVOpcodeName_label_1110:
	lla         a0, .str.141
	ret         

	// *** Basic block 347

.RVOpcodeName_label_1115:
	lla         a0, .str.142
	ret         

	// *** Basic block 348

.RVOpcodeName_label_1120:
	lla         a0, .str.143
	ret         

	// *** Basic block 349

.RVOpcodeName_label_1125:
	lla         a0, .str.144
	ret         

	// *** Basic block 350

.RVOpcodeName_label_1130:
	lla         a0, .str.145
	ret         

	// *** Basic block 351

.RVOpcodeName_label_1135:
	lla         a0, .str.146
	ret         

	// *** Basic block 352

.RVOpcodeName_label_1140:
	lla         a0, .str.147
	ret         

	// *** Basic block 353

.RVOpcodeName_label_1145:
	lla         a0, .str.148
	ret         

	// *** Basic block 354

.RVOpcodeName_label_1150:
	lla         a0, .str.149
	ret         

	// *** Basic block 355

.RVOpcodeName_label_1155:
	lla         a0, .str.150
	ret         

	// *** Basic block 356

.RVOpcodeName_label_1160:
	lla         a0, .str.151
	ret         

	// *** Basic block 357

.RVOpcodeName_label_1165:
	lla         a0, .str.152
	ret         

	// *** Basic block 358

.RVOpcodeName_label_1170:
	lla         a0, .str.153
	ret         

	// *** Basic block 359

.RVOpcodeName_label_1175:
	lla         a0, .str.154
	ret         

	// *** Basic block 360

.RVOpcodeName_label_1180:
	lla         a0, .str.155
	ret         

	// *** Basic block 361

.RVOpcodeName_label_1185:
	lla         a0, .str.156
	ret         

	// *** Basic block 362

.RVOpcodeName_label_1190:
	lla         a0, .str.157
	ret         

	// *** Basic block 363

.RVOpcodeName_label_1195:
	lla         a0, .str.158
	ret         

	// *** Basic block 364

.RVOpcodeName_label_1200:
	lla         a0, .str.159
	ret         

	// *** Basic block 365

.RVOpcodeName_label_1205:
	lla         a0, .str.160
	ret         

	// *** Basic block 366

.RVOpcodeName_label_1210:
	lla         a0, .str.161
	ret         

	// *** Basic block 367

.RVOpcodeName_label_1215:
	lla         a0, .str.162
	ret         

	// *** Basic block 368

.RVOpcodeName_label_1220:
	lla         a0, .str.163
	ret         

	// *** Basic block 369

.RVOpcodeName_label_1225:
	lla         a0, .str.164
	ret         

	// *** Basic block 370

.RVOpcodeName_label_1230:
	lla         a0, .str.165
	ret         

	// *** Basic block 371

.RVOpcodeName_label_1235:
	lla         a0, .str.166
	ret         

	// *** Basic block 372

.RVOpcodeName_label_1240:
	lla         a0, .str.167
	ret         

	// *** Basic block 373

.RVOpcodeName_label_1245:
	lla         a0, .str.168
	ret         

	// *** Basic block 374

.RVOpcodeName_label_1250:
	lla         a0, .str.169
	ret         

	// *** Basic block 375

.RVOpcodeName_label_1255:
	lla         a0, .str.170
	ret         

	// *** Basic block 376

.RVOpcodeName_label_1260:
	lla         a0, .str.171
	ret         

	// *** Basic block 377

.RVOpcodeName_label_1265:
	lla         a0, .str.172
	ret         

	// *** Basic block 378

.RVOpcodeName_label_1270:
	lla         a0, .str.173
	ret         

	// *** Basic block 379

.RVOpcodeName_label_1275:
	lla         a0, .str.174
	ret         

	// *** Basic block 380

.RVOpcodeName_label_1280:
	lla         a0, .str.175
	ret         

	// *** Basic block 381

.RVOpcodeName_label_1285:
	lla         a0, .str.176
	ret         

	// *** Basic block 382

.RVOpcodeName_label_1290:
	lla         a0, .str.177
	ret         

	// *** Basic block 383

.RVOpcodeName_label_1295:
	lla         a0, .str.178
	ret         

	// *** Basic block 384

.RVOpcodeName_label_1300:
	lla         a0, .str.179
	ret         

	// *** Basic block 385

.RVOpcodeName_label_1305:
	lla         a0, .str.180
	ret         

	// *** Basic block 386

.RVOpcodeName_label_1310:
	lla         a0, .str.181
	ret         
.func_end_RVOpcodeName:
	.size RVOpcodeName, .func_end_RVOpcodeName-RVOpcodeName

	.global RVIsExpression
	.type RVIsExpression, @function

RVIsExpression:

	// *** Basic block 0

	.global RVIsFixedRegister
	.global RVIsConst
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
	li          s2, 44		// 0x2c ASCII ','
	blt         s1, s2, .RVIsExpression_label_142

	// *** Basic block 1

	li          t0, 181		// 0xb5 ASCII \xb5
	blt         s1, t0, .RVIsExpression_label_95

	// *** Basic block 2

	beq         s1, t0, .RVIsExpression_label_248

	// *** Basic block 3

	li          t0, 182		// 0xb6 ASCII \xb6
	beq         s1, t0, .RVIsExpression_label_249

	// *** Basic block 4

	li          t0, 183		// 0xb7 ASCII \xb7
	beq         s1, t0, .RVIsExpression_label_250

	// *** Basic block 5

	li          t0, 184		// 0xb8 ASCII \xb8
	beq         s1, t0, .RVIsExpression_label_251

	// *** Basic block 6

	li          t0, 185		// 0xb9 ASCII \xb9
	beq         s1, t0, .RVIsExpression_label_252

	// *** Basic block 7

	li          t0, 186		// 0xba ASCII \xba
	beq         s1, t0, .RVIsExpression_label_253

	// *** Basic block 8

	li          t0, 206		// 0xce ASCII \xce
	beq         s1, t0, .RVIsExpression_label_269

	// *** Basic block 9

	li          t0, 209		// 0xd1 ASCII \xd1
	beq         s1, t0, .RVIsExpression_label_268

	// *** Basic block 10

	li          t0, 210		// 0xd2 ASCII \xd2
	beq         s1, t0, .RVIsExpression_label_271

	// *** Basic block 11

	j           .RVIsExpression_label_278

	// *** Basic block 12

.RVIsExpression_label_95:
	beq         s1, s2, .RVIsExpression_label_245

	// *** Basic block 13

	li          t0, 50		// 0x32 ASCII '2'
	beq         s1, t0, .RVIsExpression_label_260

	// *** Basic block 14

	li          t0, 51		// 0x33 ASCII '3'
	beq         s1, t0, .RVIsExpression_label_262

	// *** Basic block 15

	li          t0, 52		// 0x34 ASCII '4'
	beq         s1, t0, .RVIsExpression_label_261

	// *** Basic block 16

	li          t0, 84		// 0x54 ASCII 'T'
	beq         s1, t0, .RVIsExpression_label_263

	// *** Basic block 17

	li          t0, 108		// 0x6c ASCII 'l'
	beq         s1, t0, .RVIsExpression_label_264

	// *** Basic block 18

	li          t0, 138		// 0x8a ASCII \x8a
	beq         s1, t0, .RVIsExpression_label_265

	// *** Basic block 19

	li          t0, 179		// 0xb3 ASCII \xb3
	beq         s1, t0, .RVIsExpression_label_246

	// *** Basic block 20

	li          t0, 180		// 0xb4 ASCII \xb4
	beq         s1, t0, .RVIsExpression_label_247

	// *** Basic block 21

	j           .RVIsExpression_label_278

	// *** Basic block 22

.RVIsExpression_label_142:
	li          t0, 31		// 0x1f ASCII \x1f
	blt         s1, t0, .RVIsExpression_label_192

	// *** Basic block 23

	beq         s1, t0, .RVIsExpression_label_266

	// *** Basic block 24

	li          t0, 32		// 0x20 ASCII ' '
	beq         s1, t0, .RVIsExpression_label_267

	// *** Basic block 25

	li          t0, 37		// 0x25 ASCII '%'
	beq         s1, t0, .RVIsExpression_label_238

	// *** Basic block 26

	li          t0, 38		// 0x26 ASCII '&'
	beq         s1, t0, .RVIsExpression_label_239

	// *** Basic block 27

	li          t0, 39		// 0x27 ASCII '''
	beq         s1, t0, .RVIsExpression_label_240

	// *** Basic block 28

	li          t0, 40		// 0x28 ASCII '('
	beq         s1, t0, .RVIsExpression_label_241

	// *** Basic block 29

	li          t0, 41		// 0x29 ASCII ')'
	beq         s1, t0, .RVIsExpression_label_242

	// *** Basic block 30

	li          t0, 42		// 0x2a ASCII '*'
	beq         s1, t0, .RVIsExpression_label_243

	// *** Basic block 31

	li          t0, 43		// 0x2b ASCII '+'
	beq         s1, t0, .RVIsExpression_label_244

	// *** Basic block 32

	j           .RVIsExpression_label_278

	// *** Basic block 33

.RVIsExpression_label_192:
	beqz        s1, .RVIsExpression_label_255

	// *** Basic block 34

	li          t0, 1		// 0x1 ASCII \x1
	beq         s1, t0, .RVIsExpression_label_256

	// *** Basic block 35

	li          t0, 2		// 0x2 ASCII \x2
	beq         s1, t0, .RVIsExpression_label_270

	// *** Basic block 36

	li          t0, 18		// 0x12 ASCII \x12
	beq         s1, t0, .RVIsExpression_label_257

	// *** Basic block 37

	li          t0, 19		// 0x13 ASCII \x13
	beq         s1, t0, .RVIsExpression_label_258

	// *** Basic block 38

	li          t0, 20		// 0x14 ASCII \x14
	beq         s1, t0, .RVIsExpression_label_259

	// *** Basic block 39

	li          t0, 21		// 0x15 ASCII \x15
	beq         s1, t0, .RVIsExpression_label_254

	// *** Basic block 40

	li          t0, 22		// 0x16 ASCII \x16
	beq         s1, t0, .RVIsExpression_label_236

	// *** Basic block 41

	li          t0, 30		// 0x1e ASCII \x1e
	beq         s1, t0, .RVIsExpression_label_237

	// *** Basic block 42

	j           .RVIsExpression_label_278

	// *** Basic block 43

.RVIsExpression_label_236:

	// *** Basic block 44

.RVIsExpression_label_237:

	// *** Basic block 45

.RVIsExpression_label_238:

	// *** Basic block 46

.RVIsExpression_label_239:

	// *** Basic block 47

.RVIsExpression_label_240:

	// *** Basic block 48

.RVIsExpression_label_241:

	// *** Basic block 49

.RVIsExpression_label_242:

	// *** Basic block 50

.RVIsExpression_label_243:

	// *** Basic block 51

.RVIsExpression_label_244:

	// *** Basic block 52

.RVIsExpression_label_245:

	// *** Basic block 53

.RVIsExpression_label_246:

	// *** Basic block 54

.RVIsExpression_label_247:

	// *** Basic block 55

.RVIsExpression_label_248:

	// *** Basic block 56

.RVIsExpression_label_249:

	// *** Basic block 57

.RVIsExpression_label_250:

	// *** Basic block 58

.RVIsExpression_label_251:

	// *** Basic block 59

.RVIsExpression_label_252:

	// *** Basic block 60

.RVIsExpression_label_253:

	// *** Basic block 61

.RVIsExpression_label_254:

	// *** Basic block 62

.RVIsExpression_label_255:

	// *** Basic block 63

.RVIsExpression_label_256:

	// *** Basic block 64

.RVIsExpression_label_257:

	// *** Basic block 65

.RVIsExpression_label_258:

	// *** Basic block 66

.RVIsExpression_label_259:

	// *** Basic block 67

.RVIsExpression_label_260:

	// *** Basic block 68

.RVIsExpression_label_261:

	// *** Basic block 69

.RVIsExpression_label_262:

	// *** Basic block 70

.RVIsExpression_label_263:

	// *** Basic block 71

.RVIsExpression_label_264:

	// *** Basic block 72

.RVIsExpression_label_265:

	// *** Basic block 73

.RVIsExpression_label_266:

	// *** Basic block 74

.RVIsExpression_label_267:

	// *** Basic block 75

.RVIsExpression_label_268:

	// *** Basic block 76

.RVIsExpression_label_269:

	// *** Basic block 77

.RVIsExpression_label_270:

	// *** Basic block 78

.RVIsExpression_label_271:
	mv          a0, x0

	// *** Basic block 79

.RVIsExpression_label_275:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 80

.RVIsExpression_label_278:
	mv          a0, s1
	call        RVIsFixedRegister

	// *** Basic block 81

	not         a0, a0
	beqz        a0, .RVIsExpression_label_289

	// *** Basic block 82

	mv          a0, s1
	call        RVIsConst

	// *** Basic block 83

	not         a0, a0

	// *** Basic block 84

.RVIsExpression_label_289:
	j           .RVIsExpression_label_275
.func_end_RVIsExpression:
	.size RVIsExpression, .func_end_RVIsExpression-RVIsExpression

	.global RVGeneratesOutput
	.type RVGeneratesOutput, @function

RVGeneratesOutput:

	// *** Basic block 0

	.global RVIsFixedRegister
	.global RVIsExpression
	.global RVIsSymbol
	.global RVIsConst
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
	call        RVIsFixedRegister

	// *** Basic block 1

	beqz        a0, .RVGeneratesOutput_label_22

	// *** Basic block 2

	mv          a0, x0

	// *** Basic block 3

.RVGeneratesOutput_label_19:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 4

.RVGeneratesOutput_label_22:
	li          t0, 26		// 0x1a ASCII \x1a
	blt         s1, t0, .RVGeneratesOutput_label_55

	// *** Basic block 5

	li          t0, 34		// 0x22 ASCII '"'
	blt         t0, s1, .RVGeneratesOutput_label_55

	// *** Basic block 6

	addi        t0, s1, -26
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 7

	j           .RVGeneratesOutput_label_49

	// *** Basic block 8

	j           .RVGeneratesOutput_label_50

	// *** Basic block 9

	j           .RVGeneratesOutput_label_51

	// *** Basic block 10

	j           .RVGeneratesOutput_label_55

	// *** Basic block 11

	j           .RVGeneratesOutput_label_55

	// *** Basic block 12

	j           .RVGeneratesOutput_label_55

	// *** Basic block 13

	j           .RVGeneratesOutput_label_55

	// *** Basic block 14

	j           .RVGeneratesOutput_label_47

	// *** Basic block 15

	j           .RVGeneratesOutput_label_48

	// *** Basic block 16

.RVGeneratesOutput_label_47:

	// *** Basic block 17

.RVGeneratesOutput_label_48:

	// *** Basic block 18

.RVGeneratesOutput_label_49:

	// *** Basic block 19

.RVGeneratesOutput_label_50:

	// *** Basic block 20

.RVGeneratesOutput_label_51:
	mv          a0, x0
	j           .RVGeneratesOutput_label_19

	// *** Basic block 21

.RVGeneratesOutput_label_55:
	mv          a0, s1
	call        RVIsExpression

	// *** Basic block 22

	beqz        a0, .RVGeneratesOutput_label_67

	// *** Basic block 23

	mv          a0, s1
	call        RVIsSymbol

	// *** Basic block 24

	not         a0, a0

	// *** Basic block 25

.RVGeneratesOutput_label_67:
	beqz        a0, .RVGeneratesOutput_label_73

	// *** Basic block 26

	mv          a0, s1
	call        RVIsConst

	// *** Basic block 27

	not         a0, a0

	// *** Basic block 28

.RVGeneratesOutput_label_73:
	j           .RVGeneratesOutput_label_19
.func_end_RVGeneratesOutput:
	.size RVGeneratesOutput, .func_end_RVGeneratesOutput-RVGeneratesOutput

	.global RVIsLoad
	.type RVIsLoad, @function

RVIsLoad:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	li          t1, 45		// 0x2d ASCII '-'
	beq         t0, t1, .RVIsLoad_label_66

	// *** Basic block 1

	li          t1, 46		// 0x2e ASCII '.'
	beq         t0, t1, .RVIsLoad_label_71

	// *** Basic block 2

	li          t1, 47		// 0x2f ASCII '/'
	beq         t0, t1, .RVIsLoad_label_67

	// *** Basic block 3

	li          t1, 48		// 0x30 ASCII '0'
	beq         t0, t1, .RVIsLoad_label_69

	// *** Basic block 4

	li          t1, 49		// 0x31 ASCII '1'
	beq         t0, t1, .RVIsLoad_label_72

	// *** Basic block 5

	li          t1, 82		// 0x52 ASCII 'R'
	beq         t0, t1, .RVIsLoad_label_70

	// *** Basic block 6

	li          t1, 83		// 0x53 ASCII 'S'
	beq         t0, t1, .RVIsLoad_label_68

	// *** Basic block 7

	li          t1, 107		// 0x6b ASCII 'k'
	beq         t0, t1, .RVIsLoad_label_73

	// *** Basic block 8

	li          t1, 137		// 0x89 ASCII \x89
	beq         t0, t1, .RVIsLoad_label_74

	// *** Basic block 9

.RVIsLoad_label_61:
	mv          a0, x0
	ret         

	// *** Basic block 10

.RVIsLoad_label_66:

	// *** Basic block 11

.RVIsLoad_label_67:

	// *** Basic block 12

.RVIsLoad_label_68:

	// *** Basic block 13

.RVIsLoad_label_69:

	// *** Basic block 14

.RVIsLoad_label_70:

	// *** Basic block 15

.RVIsLoad_label_71:

	// *** Basic block 16

.RVIsLoad_label_72:

	// *** Basic block 17

.RVIsLoad_label_73:

	// *** Basic block 18

.RVIsLoad_label_74:
	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 19

.RVIsLoad_label_77:
	ret         
.func_end_RVIsLoad:
	.size RVIsLoad, .func_end_RVIsLoad-RVIsLoad

	.global RVIsSignedLoad
	.type RVIsSignedLoad, @function

RVIsSignedLoad:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	li          t1, 45		// 0x2d ASCII '-'
	beq         t0, t1, .RVIsSignedLoad_label_36

	// *** Basic block 1

	li          t1, 46		// 0x2e ASCII '.'
	beq         t0, t1, .RVIsSignedLoad_label_38

	// *** Basic block 2

	li          t1, 47		// 0x2f ASCII '/'
	beq         t0, t1, .RVIsSignedLoad_label_37

	// *** Basic block 3

	li          t1, 83		// 0x53 ASCII 'S'
	beq         t0, t1, .RVIsSignedLoad_label_39

	// *** Basic block 4

.RVIsSignedLoad_label_31:
	mv          a0, x0
	ret         

	// *** Basic block 5

.RVIsSignedLoad_label_36:

	// *** Basic block 6

.RVIsSignedLoad_label_37:

	// *** Basic block 7

.RVIsSignedLoad_label_38:

	// *** Basic block 8

.RVIsSignedLoad_label_39:
	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 9

.RVIsSignedLoad_label_42:
	ret         
.func_end_RVIsSignedLoad:
	.size RVIsSignedLoad, .func_end_RVIsSignedLoad-RVIsSignedLoad

	.global RVIsStore
	.type RVIsStore, @function

RVIsStore:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	li          t1, 46		// 0x2e ASCII '.'
	beq         t0, t1, .RVIsStore_label_51

	// *** Basic block 1

	li          t1, 50		// 0x32 ASCII '2'
	beq         t0, t1, .RVIsStore_label_48

	// *** Basic block 2

	li          t1, 52		// 0x34 ASCII '4'
	beq         t0, t1, .RVIsStore_label_49

	// *** Basic block 3

	li          t1, 84		// 0x54 ASCII 'T'
	beq         t0, t1, .RVIsStore_label_50

	// *** Basic block 4

	li          t1, 108		// 0x6c ASCII 'l'
	beq         t0, t1, .RVIsStore_label_52

	// *** Basic block 5

	li          t1, 138		// 0x8a ASCII \x8a
	beq         t0, t1, .RVIsStore_label_53

	// *** Basic block 6

.RVIsStore_label_43:
	mv          a0, x0
	ret         

	// *** Basic block 7

.RVIsStore_label_48:

	// *** Basic block 8

.RVIsStore_label_49:

	// *** Basic block 9

.RVIsStore_label_50:

	// *** Basic block 10

.RVIsStore_label_51:

	// *** Basic block 11

.RVIsStore_label_52:

	// *** Basic block 12

.RVIsStore_label_53:
	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 13

.RVIsStore_label_56:
	ret         
.func_end_RVIsStore:
	.size RVIsStore, .func_end_RVIsStore-RVIsStore

	.global RVIsIntConst
	.type RVIsIntConst, @function

RVIsIntConst:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	li          t1, 5		// 0x5 ASCII \x5
	blt         t0, t1, .RVIsIntConst_label_38

	// *** Basic block 1

	li          t1, 8		// 0x8 ASCII \x8
	blt         t1, t0, .RVIsIntConst_label_38

	// *** Basic block 2

	addi        t1, t0, -5
	slli        t1, t1, 2
	auipc       t2, 0
	add         t1, t2, t1
	jalr        x0, t1, 12

	// *** Basic block 3

	j           .RVIsIntConst_label_30

	// *** Basic block 4

	j           .RVIsIntConst_label_31

	// *** Basic block 5

	j           .RVIsIntConst_label_29

	// *** Basic block 6

	j           .RVIsIntConst_label_32

	// *** Basic block 7

.RVIsIntConst_label_29:

	// *** Basic block 8

.RVIsIntConst_label_30:

	// *** Basic block 9

.RVIsIntConst_label_31:

	// *** Basic block 10

.RVIsIntConst_label_32:
	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 11

.RVIsIntConst_label_35:
	ret         

	// *** Basic block 12

.RVIsIntConst_label_38:
	mv          a0, x0
	ret         
.func_end_RVIsIntConst:
	.size RVIsIntConst, .func_end_RVIsIntConst-RVIsIntConst

	.global RVIsConst
	.type RVIsConst, @function

RVIsConst:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	li          t1, 5		// 0x5 ASCII \x5
	blt         t0, t1, .RVIsConst_label_42

	// *** Basic block 1

	li          t1, 10		// 0xa ASCII \xa
	blt         t1, t0, .RVIsConst_label_42

	// *** Basic block 2

	addi        t1, t0, -5
	slli        t1, t1, 2
	auipc       t2, 0
	add         t1, t2, t1
	jalr        x0, t1, 12

	// *** Basic block 3

	j           .RVIsConst_label_32

	// *** Basic block 4

	j           .RVIsConst_label_33

	// *** Basic block 5

	j           .RVIsConst_label_31

	// *** Basic block 6

	j           .RVIsConst_label_34

	// *** Basic block 7

	j           .RVIsConst_label_35

	// *** Basic block 8

	j           .RVIsConst_label_36

	// *** Basic block 9

.RVIsConst_label_31:

	// *** Basic block 10

.RVIsConst_label_32:

	// *** Basic block 11

.RVIsConst_label_33:

	// *** Basic block 12

.RVIsConst_label_34:

	// *** Basic block 13

.RVIsConst_label_35:

	// *** Basic block 14

.RVIsConst_label_36:
	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 15

.RVIsConst_label_39:
	ret         

	// *** Basic block 16

.RVIsConst_label_42:
	mv          a0, x0
	ret         
.func_end_RVIsConst:
	.size RVIsConst, .func_end_RVIsConst-RVIsConst

	.global RVIsFloatingPoint
	.type RVIsFloatingPoint, @function

RVIsFloatingPoint:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	slti        t1, t0, 107
	li          t2, 107		// 0x6b ASCII 'k'
	blt         t0, t2, .RVIsFloatingPoint_label_30

	// *** Basic block 1

	li          t2, 168		// 0xa8 ASCII \xa8
	slt         t1, t2, t0

	// *** Basic block 2

.RVIsFloatingPoint_label_30:
	beqz        t1, .RVIsFloatingPoint_label_38

	// *** Basic block 3

	mv          a0, x0

	// *** Basic block 4

.RVIsFloatingPoint_label_35:
	ret         

	// *** Basic block 5

.RVIsFloatingPoint_label_38:
	li          t1, 9		// 0x9 ASCII \x9
	beq         t0, t1, .RVIsFloatingPoint_label_127

	// *** Basic block 6

	li          t1, 10		// 0xa ASCII \xa
	beq         t0, t1, .RVIsFloatingPoint_label_128

	// *** Basic block 7

	li          t1, 12		// 0xc ASCII \xc
	beq         t0, t1, .RVIsFloatingPoint_label_129

	// *** Basic block 8

	li          t1, 13		// 0xd ASCII \xd
	beq         t0, t1, .RVIsFloatingPoint_label_130

	// *** Basic block 9

	li          t1, 19		// 0x13 ASCII \x13
	beq         t0, t1, .RVIsFloatingPoint_label_131

	// *** Basic block 10

	li          t1, 20		// 0x14 ASCII \x14
	beq         t0, t1, .RVIsFloatingPoint_label_132

	// *** Basic block 11

	li          t1, 34		// 0x22 ASCII '"'
	beq         t0, t1, .RVIsFloatingPoint_label_126

	// *** Basic block 12

	li          t1, 198		// 0xc6 ASCII \xc6
	beq         t0, t1, .RVIsFloatingPoint_label_118

	// *** Basic block 13

	li          t1, 199		// 0xc7 ASCII \xc7
	beq         t0, t1, .RVIsFloatingPoint_label_119

	// *** Basic block 14

	li          t1, 200		// 0xc8 ASCII \xc8
	beq         t0, t1, .RVIsFloatingPoint_label_120

	// *** Basic block 15

	li          t1, 201		// 0xc9 ASCII \xc9
	beq         t0, t1, .RVIsFloatingPoint_label_121

	// *** Basic block 16

	li          t1, 202		// 0xca ASCII \xca
	beq         t0, t1, .RVIsFloatingPoint_label_122

	// *** Basic block 17

	li          t1, 203		// 0xcb ASCII \xcb
	beq         t0, t1, .RVIsFloatingPoint_label_123

	// *** Basic block 18

	li          t1, 204		// 0xcc ASCII \xcc
	beq         t0, t1, .RVIsFloatingPoint_label_124

	// *** Basic block 19

	li          t1, 205		// 0xcd ASCII \xcd
	beq         t0, t1, .RVIsFloatingPoint_label_125

	// *** Basic block 20

.RVIsFloatingPoint_label_114:
	mv          a0, x0
	ret         

	// *** Basic block 21

.RVIsFloatingPoint_label_118:

	// *** Basic block 22

.RVIsFloatingPoint_label_119:

	// *** Basic block 23

.RVIsFloatingPoint_label_120:

	// *** Basic block 24

.RVIsFloatingPoint_label_121:

	// *** Basic block 25

.RVIsFloatingPoint_label_122:

	// *** Basic block 26

.RVIsFloatingPoint_label_123:

	// *** Basic block 27

.RVIsFloatingPoint_label_124:

	// *** Basic block 28

.RVIsFloatingPoint_label_125:

	// *** Basic block 29

.RVIsFloatingPoint_label_126:

	// *** Basic block 30

.RVIsFloatingPoint_label_127:

	// *** Basic block 31

.RVIsFloatingPoint_label_128:

	// *** Basic block 32

.RVIsFloatingPoint_label_129:

	// *** Basic block 33

.RVIsFloatingPoint_label_130:

	// *** Basic block 34

.RVIsFloatingPoint_label_131:

	// *** Basic block 35

.RVIsFloatingPoint_label_132:
	li          a0, 1		// 0x1 ASCII \x1
	ret         
.func_end_RVIsFloatingPoint:
	.size RVIsFloatingPoint, .func_end_RVIsFloatingPoint-RVIsFloatingPoint

	.global RVIsSymbol
	.type RVIsSymbol, @function

RVIsSymbol:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	addi        t0, a0, -2
	seqz        a0, t0

	// *** Basic block 1

.RVIsSymbol_label_10:
	ret         
.func_end_RVIsSymbol:
	.size RVIsSymbol, .func_end_RVIsSymbol-RVIsSymbol

	.global RVIsCall
	.type RVIsCall, @function

RVIsCall:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	li          t1, 183		// 0xb7 ASCII \xb7
	blt         t0, t1, .RVIsCall_label_38

	// *** Basic block 1

	li          t1, 186		// 0xba ASCII \xba
	blt         t1, t0, .RVIsCall_label_38

	// *** Basic block 2

	addi        t1, t0, -183
	slli        t1, t1, 2
	auipc       t2, 0
	add         t1, t2, t1
	jalr        x0, t1, 12

	// *** Basic block 3

	j           .RVIsCall_label_29

	// *** Basic block 4

	j           .RVIsCall_label_30

	// *** Basic block 5

	j           .RVIsCall_label_31

	// *** Basic block 6

	j           .RVIsCall_label_32

	// *** Basic block 7

.RVIsCall_label_29:

	// *** Basic block 8

.RVIsCall_label_30:

	// *** Basic block 9

.RVIsCall_label_31:

	// *** Basic block 10

.RVIsCall_label_32:
	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 11

.RVIsCall_label_35:
	ret         

	// *** Basic block 12

.RVIsCall_label_38:
	mv          a0, x0
	ret         
.func_end_RVIsCall:
	.size RVIsCall, .func_end_RVIsCall-RVIsCall

	.global RVIsArgRegister
	.type RVIsArgRegister, @function

RVIsArgRegister:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	li          t1, 190		// 0xbe ASCII \xbe
	blt         t0, t1, .RVIsArgRegister_label_62

	// *** Basic block 1

	li          t1, 205		// 0xcd ASCII \xcd
	blt         t1, t0, .RVIsArgRegister_label_62

	// *** Basic block 2

	addi        t1, t0, -190
	slli        t1, t1, 2
	auipc       t2, 0
	add         t1, t2, t1
	jalr        x0, t1, 12

	// *** Basic block 3

	j           .RVIsArgRegister_label_41

	// *** Basic block 4

	j           .RVIsArgRegister_label_42

	// *** Basic block 5

	j           .RVIsArgRegister_label_43

	// *** Basic block 6

	j           .RVIsArgRegister_label_44

	// *** Basic block 7

	j           .RVIsArgRegister_label_45

	// *** Basic block 8

	j           .RVIsArgRegister_label_46

	// *** Basic block 9

	j           .RVIsArgRegister_label_47

	// *** Basic block 10

	j           .RVIsArgRegister_label_48

	// *** Basic block 11

	j           .RVIsArgRegister_label_49

	// *** Basic block 12

	j           .RVIsArgRegister_label_50

	// *** Basic block 13

	j           .RVIsArgRegister_label_51

	// *** Basic block 14

	j           .RVIsArgRegister_label_52

	// *** Basic block 15

	j           .RVIsArgRegister_label_53

	// *** Basic block 16

	j           .RVIsArgRegister_label_54

	// *** Basic block 17

	j           .RVIsArgRegister_label_55

	// *** Basic block 18

	j           .RVIsArgRegister_label_56

	// *** Basic block 19

.RVIsArgRegister_label_41:

	// *** Basic block 20

.RVIsArgRegister_label_42:

	// *** Basic block 21

.RVIsArgRegister_label_43:

	// *** Basic block 22

.RVIsArgRegister_label_44:

	// *** Basic block 23

.RVIsArgRegister_label_45:

	// *** Basic block 24

.RVIsArgRegister_label_46:

	// *** Basic block 25

.RVIsArgRegister_label_47:

	// *** Basic block 26

.RVIsArgRegister_label_48:

	// *** Basic block 27

.RVIsArgRegister_label_49:

	// *** Basic block 28

.RVIsArgRegister_label_50:

	// *** Basic block 29

.RVIsArgRegister_label_51:

	// *** Basic block 30

.RVIsArgRegister_label_52:

	// *** Basic block 31

.RVIsArgRegister_label_53:

	// *** Basic block 32

.RVIsArgRegister_label_54:

	// *** Basic block 33

.RVIsArgRegister_label_55:

	// *** Basic block 34

.RVIsArgRegister_label_56:
	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 35

.RVIsArgRegister_label_59:
	ret         

	// *** Basic block 36

.RVIsArgRegister_label_62:
	mv          a0, x0
	ret         
.func_end_RVIsArgRegister:
	.size RVIsArgRegister, .func_end_RVIsArgRegister-RVIsArgRegister

	.global RVIsVarRegister
	.type RVIsVarRegister, @function

RVIsVarRegister:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	lw          t0, 16(a0)
	li          t1, 33		// 0x21 ASCII '!'
	blt         t0, t1, .RVIsVarRegister_label_37

	// *** Basic block 1

	li          t1, 34		// 0x22 ASCII '"'
	blt         t1, t0, .RVIsVarRegister_label_37

	// *** Basic block 2

	addi        t0, t0, -33
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 3

	j           .RVIsVarRegister_label_30

	// *** Basic block 4

	j           .RVIsVarRegister_label_31

	// *** Basic block 5

.RVIsVarRegister_label_30:

	// *** Basic block 6

.RVIsVarRegister_label_31:
	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 7

.RVIsVarRegister_label_34:
	ret         

	// *** Basic block 8

.RVIsVarRegister_label_37:
	mv          a0, x0
	ret         
.func_end_RVIsVarRegister:
	.size RVIsVarRegister, .func_end_RVIsVarRegister-RVIsVarRegister

	.global RVIsFixedRegister
	.type RVIsFixedRegister, @function

RVIsFixedRegister:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	li          t1, 196		// 0xc4 ASCII \xc4
	blt         t0, t1, .RVIsFixedRegister_label_94

	// *** Basic block 1

	beq         t0, t1, .RVIsFixedRegister_label_157

	// *** Basic block 2

	li          t1, 197		// 0xc5 ASCII \xc5
	beq         t0, t1, .RVIsFixedRegister_label_158

	// *** Basic block 3

	li          t1, 198		// 0xc6 ASCII \xc6
	beq         t0, t1, .RVIsFixedRegister_label_159

	// *** Basic block 4

	li          t1, 199		// 0xc7 ASCII \xc7
	beq         t0, t1, .RVIsFixedRegister_label_160

	// *** Basic block 5

	li          t1, 200		// 0xc8 ASCII \xc8
	beq         t0, t1, .RVIsFixedRegister_label_161

	// *** Basic block 6

	li          t1, 201		// 0xc9 ASCII \xc9
	beq         t0, t1, .RVIsFixedRegister_label_162

	// *** Basic block 7

	li          t1, 202		// 0xca ASCII \xca
	beq         t0, t1, .RVIsFixedRegister_label_163

	// *** Basic block 8

	li          t1, 203		// 0xcb ASCII \xcb
	beq         t0, t1, .RVIsFixedRegister_label_164

	// *** Basic block 9

	li          t1, 204		// 0xcc ASCII \xcc
	beq         t0, t1, .RVIsFixedRegister_label_165

	// *** Basic block 10

	li          t1, 205		// 0xcd ASCII \xcd
	beq         t0, t1, .RVIsFixedRegister_label_166

	// *** Basic block 11

	li          t1, 207		// 0xcf ASCII \xcf
	beq         t0, t1, .RVIsFixedRegister_label_167

	// *** Basic block 12

	li          t1, 208		// 0xd0 ASCII \xd0
	beq         t0, t1, .RVIsFixedRegister_label_168

	// *** Basic block 13

	j           .RVIsFixedRegister_label_179

	// *** Basic block 14

.RVIsFixedRegister_label_94:
	li          t1, 23		// 0x17 ASCII \x17
	beq         t0, t1, .RVIsFixedRegister_label_169

	// *** Basic block 15

	li          t1, 183		// 0xb7 ASCII \xb7
	beq         t0, t1, .RVIsFixedRegister_label_170

	// *** Basic block 16

	li          t1, 184		// 0xb8 ASCII \xb8
	beq         t0, t1, .RVIsFixedRegister_label_171

	// *** Basic block 17

	li          t1, 185		// 0xb9 ASCII \xb9
	beq         t0, t1, .RVIsFixedRegister_label_172

	// *** Basic block 18

	li          t1, 186		// 0xba ASCII \xba
	beq         t0, t1, .RVIsFixedRegister_label_173

	// *** Basic block 19

	li          t1, 190		// 0xbe ASCII \xbe
	beq         t0, t1, .RVIsFixedRegister_label_151

	// *** Basic block 20

	li          t1, 191		// 0xbf ASCII \xbf
	beq         t0, t1, .RVIsFixedRegister_label_152

	// *** Basic block 21

	li          t1, 192		// 0xc0 ASCII \xc0
	beq         t0, t1, .RVIsFixedRegister_label_153

	// *** Basic block 22

	li          t1, 193		// 0xc1 ASCII \xc1
	beq         t0, t1, .RVIsFixedRegister_label_154

	// *** Basic block 23

	li          t1, 194		// 0xc2 ASCII \xc2
	beq         t0, t1, .RVIsFixedRegister_label_155

	// *** Basic block 24

	li          t1, 195		// 0xc3 ASCII \xc3
	beq         t0, t1, .RVIsFixedRegister_label_156

	// *** Basic block 25

	j           .RVIsFixedRegister_label_179

	// *** Basic block 26

.RVIsFixedRegister_label_151:

	// *** Basic block 27

.RVIsFixedRegister_label_152:

	// *** Basic block 28

.RVIsFixedRegister_label_153:

	// *** Basic block 29

.RVIsFixedRegister_label_154:

	// *** Basic block 30

.RVIsFixedRegister_label_155:

	// *** Basic block 31

.RVIsFixedRegister_label_156:

	// *** Basic block 32

.RVIsFixedRegister_label_157:

	// *** Basic block 33

.RVIsFixedRegister_label_158:

	// *** Basic block 34

.RVIsFixedRegister_label_159:

	// *** Basic block 35

.RVIsFixedRegister_label_160:

	// *** Basic block 36

.RVIsFixedRegister_label_161:

	// *** Basic block 37

.RVIsFixedRegister_label_162:

	// *** Basic block 38

.RVIsFixedRegister_label_163:

	// *** Basic block 39

.RVIsFixedRegister_label_164:

	// *** Basic block 40

.RVIsFixedRegister_label_165:

	// *** Basic block 41

.RVIsFixedRegister_label_166:

	// *** Basic block 42

.RVIsFixedRegister_label_167:

	// *** Basic block 43

.RVIsFixedRegister_label_168:

	// *** Basic block 44

.RVIsFixedRegister_label_169:

	// *** Basic block 45

.RVIsFixedRegister_label_170:

	// *** Basic block 46

.RVIsFixedRegister_label_171:

	// *** Basic block 47

.RVIsFixedRegister_label_172:

	// *** Basic block 48

.RVIsFixedRegister_label_173:
	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 49

.RVIsFixedRegister_label_176:
	ret         

	// *** Basic block 50

.RVIsFixedRegister_label_179:
	mv          a0, x0
	ret         
.func_end_RVIsFixedRegister:
	.size RVIsFixedRegister, .func_end_RVIsFixedRegister-RVIsFixedRegister

	.global RVIsResult
	.type RVIsResult, @function

RVIsResult:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	li          t1, 26		// 0x1a ASCII \x1a
	blt         t0, t1, .RVIsResult_label_36

	// *** Basic block 1

	li          t1, 28		// 0x1c ASCII \x1c
	blt         t1, t0, .RVIsResult_label_36

	// *** Basic block 2

	addi        t1, t0, -26
	slli        t1, t1, 2
	auipc       t2, 0
	add         t1, t2, t1
	jalr        x0, t1, 12

	// *** Basic block 3

	j           .RVIsResult_label_28

	// *** Basic block 4

	j           .RVIsResult_label_29

	// *** Basic block 5

	j           .RVIsResult_label_30

	// *** Basic block 6

.RVIsResult_label_28:

	// *** Basic block 7

.RVIsResult_label_29:

	// *** Basic block 8

.RVIsResult_label_30:
	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 9

.RVIsResult_label_33:
	ret         

	// *** Basic block 10

.RVIsResult_label_36:
	mv          a0, x0
	ret         
.func_end_RVIsResult:
	.size RVIsResult, .func_end_RVIsResult-RVIsResult

	.global RVIsBranch
	.type RVIsBranch, @function

RVIsBranch:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	li          t1, 37		// 0x25 ASCII '%'
	beq         t0, t1, .RVIsBranch_label_86

	// *** Basic block 1

	li          t1, 38		// 0x26 ASCII '&'
	beq         t0, t1, .RVIsBranch_label_87

	// *** Basic block 2

	li          t1, 39		// 0x27 ASCII '''
	beq         t0, t1, .RVIsBranch_label_88

	// *** Basic block 3

	li          t1, 40		// 0x28 ASCII '('
	beq         t0, t1, .RVIsBranch_label_89

	// *** Basic block 4

	li          t1, 41		// 0x29 ASCII ')'
	beq         t0, t1, .RVIsBranch_label_90

	// *** Basic block 5

	li          t1, 42		// 0x2a ASCII '*'
	beq         t0, t1, .RVIsBranch_label_91

	// *** Basic block 6

	li          t1, 43		// 0x2b ASCII '+'
	beq         t0, t1, .RVIsBranch_label_92

	// *** Basic block 7

	li          t1, 44		// 0x2c ASCII ','
	beq         t0, t1, .RVIsBranch_label_93

	// *** Basic block 8

	li          t1, 179		// 0xb3 ASCII \xb3
	beq         t0, t1, .RVIsBranch_label_94

	// *** Basic block 9

	li          t1, 180		// 0xb4 ASCII \xb4
	beq         t0, t1, .RVIsBranch_label_95

	// *** Basic block 10

	li          t1, 181		// 0xb5 ASCII \xb5
	beq         t0, t1, .RVIsBranch_label_84

	// *** Basic block 11

	li          t1, 182		// 0xb6 ASCII \xb6
	beq         t0, t1, .RVIsBranch_label_85

	// *** Basic block 12

.RVIsBranch_label_79:
	mv          a0, x0
	ret         

	// *** Basic block 13

.RVIsBranch_label_84:

	// *** Basic block 14

.RVIsBranch_label_85:

	// *** Basic block 15

.RVIsBranch_label_86:

	// *** Basic block 16

.RVIsBranch_label_87:

	// *** Basic block 17

.RVIsBranch_label_88:

	// *** Basic block 18

.RVIsBranch_label_89:

	// *** Basic block 19

.RVIsBranch_label_90:

	// *** Basic block 20

.RVIsBranch_label_91:

	// *** Basic block 21

.RVIsBranch_label_92:

	// *** Basic block 22

.RVIsBranch_label_93:

	// *** Basic block 23

.RVIsBranch_label_94:

	// *** Basic block 24

.RVIsBranch_label_95:
	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 25

.RVIsBranch_label_98:
	ret         
.func_end_RVIsBranch:
	.size RVIsBranch, .func_end_RVIsBranch-RVIsBranch

	.global RVIsConditionalBranch
	.type RVIsConditionalBranch, @function

RVIsConditionalBranch:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	li          t1, 39		// 0x27 ASCII '''
	beq         t0, t1, .RVIsConditionalBranch_label_60

	// *** Basic block 1

	li          t1, 40		// 0x28 ASCII '('
	beq         t0, t1, .RVIsConditionalBranch_label_61

	// *** Basic block 2

	li          t1, 41		// 0x29 ASCII ')'
	beq         t0, t1, .RVIsConditionalBranch_label_62

	// *** Basic block 3

	li          t1, 42		// 0x2a ASCII '*'
	beq         t0, t1, .RVIsConditionalBranch_label_63

	// *** Basic block 4

	li          t1, 43		// 0x2b ASCII '+'
	beq         t0, t1, .RVIsConditionalBranch_label_64

	// *** Basic block 5

	li          t1, 44		// 0x2c ASCII ','
	beq         t0, t1, .RVIsConditionalBranch_label_65

	// *** Basic block 6

	li          t1, 179		// 0xb3 ASCII \xb3
	beq         t0, t1, .RVIsConditionalBranch_label_66

	// *** Basic block 7

	li          t1, 180		// 0xb4 ASCII \xb4
	beq         t0, t1, .RVIsConditionalBranch_label_67

	// *** Basic block 8

.RVIsConditionalBranch_label_55:
	mv          a0, x0
	ret         

	// *** Basic block 9

.RVIsConditionalBranch_label_60:

	// *** Basic block 10

.RVIsConditionalBranch_label_61:

	// *** Basic block 11

.RVIsConditionalBranch_label_62:

	// *** Basic block 12

.RVIsConditionalBranch_label_63:

	// *** Basic block 13

.RVIsConditionalBranch_label_64:

	// *** Basic block 14

.RVIsConditionalBranch_label_65:

	// *** Basic block 15

.RVIsConditionalBranch_label_66:

	// *** Basic block 16

.RVIsConditionalBranch_label_67:
	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 17

.RVIsConditionalBranch_label_70:
	ret         
.func_end_RVIsConditionalBranch:
	.size RVIsConditionalBranch, .func_end_RVIsConditionalBranch-RVIsConditionalBranch

	.global RVIsReturn
	.type RVIsReturn, @function

RVIsReturn:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	addi        t0, a0, -21
	seqz        a0, t0

	// *** Basic block 1

.RVIsReturn_label_10:
	ret         
.func_end_RVIsReturn:
	.size RVIsReturn, .func_end_RVIsReturn-RVIsReturn

	.global RVIntValue
	.type RVIntValue, @function

RVIntValue:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	lw          t1, 16(t0)
	li          t2, 207		// 0xcf ASCII \xcf
	bne         t1, t2, .RVIntValue_label_23

	// *** Basic block 1

	mv          a0, x0

	// *** Basic block 2

.RVIntValue_label_20:
	ret         

	// *** Basic block 3

.RVIntValue_label_23:
	ld          a0, 120(t0)
	ret         
.func_end_RVIntValue:
	.size RVIntValue, .func_end_RVIntValue-RVIntValue

	.global RVIsPossibleImmediate
	.type RVIsPossibleImmediate, @function

RVIsPossibleImmediate:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	bge         t0, x0, .RVIsPossibleImmediate_label_18

	// *** Basic block 1

	slti        t1, t0, -2048
	not         a0, t1

	// *** Basic block 2

.RVIsPossibleImmediate_label_15:
	ret         

	// *** Basic block 3

.RVIsPossibleImmediate_label_18:
	li          t1, 2048		// 0x800
	slt         a0, t0, t1
	ret         
.func_end_RVIsPossibleImmediate:
	.size RVIsPossibleImmediate, .func_end_RVIsPossibleImmediate-RVIsPossibleImmediate

	.global RVGeneratorInit
	.type RVGeneratorInit, @function

RVGeneratorInit:

	// *** Basic block 0

	.global TargetGeneratorInit
	.global memset
	.global VectorInit
	.global RVRegisterAllocatorInit
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
	call        TargetGeneratorInit

	// *** Basic block 1

	sw          x0, 184(s1)
	sw          x0, 188(s1)
	sw          x0, 192(s1)
	sw          x0, 196(s1)
	li          t0, -1		// 0xffffffffffffffff
	sw          t0, 200(s1)
	sd          x0, 408(s1)
	sd          x0, 416(s1)
	sb          x0, 204(s1)
	addi        a0, s1, 256
	li          s2, 64		// 0x40 ASCII '@'
	mv          a2, s2
	mv          a1, x0
	call        memset

	// *** Basic block 2

	addi        a0, s1, 320
	mv          a2, s2
	mv          a1, x0
	call        memset

	// *** Basic block 3

	addi        a0, s1, 384
	call        VectorInit

	// *** Basic block 4

	addi        a0, s1, 208
	call        VectorInit

	// *** Basic block 5

	addi        a0, s1, 232
	call        VectorInit

	// *** Basic block 6

	addi        a0, s1, 424
	call        VectorInit

	// *** Basic block 7

	sd          x0, 448(s1)
	sd          x0, 456(s1)
	addi        a0, s1, 464
	mv          a1, s1
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           RVRegisterAllocatorInit
.func_end_RVGeneratorInit:
	.size RVGeneratorInit, .func_end_RVGeneratorInit-RVGeneratorInit

	.global NewRVGenerator
	.type NewRVGenerator, @function

NewRVGenerator:

	// *** Basic block 0

	.global malloc
	.global RVGeneratorInit
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
	li          a0, 2064		// 0x810
	call        malloc

	// *** Basic block 1

	mv          s2, a0
	mv          a1, s1
	mv          a0, s2
	call        RVGeneratorInit

	// *** Basic block 2

	mv          a0, s2

	// *** Basic block 3

.NewRVGenerator_label_21:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewRVGenerator:
	.size NewRVGenerator, .func_end_NewRVGenerator-NewRVGenerator

	.global RVGeneratorDestruct
	.type RVGeneratorDestruct, @function

RVGeneratorDestruct:

	// *** Basic block 0

	.global TargetGeneratorDestruct
	.global VectorDestructWithContents
	.global RVRegisterAllocatorDestruct
	.global TargetBasicBlockDelete
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
	call        TargetGeneratorDestruct

	// *** Basic block 1

	addi        a0, s1, 384
	mv          a1, x0
	call        VectorDestructWithContents

	// *** Basic block 2

	addi        a0, s1, 208
	mv          a1, x0
	call        VectorDestructWithContents

	// *** Basic block 3

	addi        a0, s1, 232
	mv          a1, x0
	call        VectorDestructWithContents

	// *** Basic block 4

	addi        a0, s1, 464
	call        RVRegisterAllocatorDestruct

	// *** Basic block 5

	mv          s2, x0
	addi        t0, s1, 424
	ld          s3, 8(t0)
	bge         x0, s3, .RVGeneratorDestruct_label_62

	// *** Basic block 6

	ld          s4, 424(s1)

	// *** Basic block 7

.RVGeneratorDestruct_label_49:
	slli        t0, s2, 3
	add         t0, s4, t0
	ld          s1, 0(t0)
	mv          a0, s1
	call        TargetBasicBlockDelete

	// *** Basic block 8

.RVGeneratorDestruct_label_58:
	addi        s2, s2, 1
	bge         s2, s3, .RVGeneratorDestruct_label_49

	// *** Basic block 9

.RVGeneratorDestruct_label_62:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_RVGeneratorDestruct:
	.size RVGeneratorDestruct, .func_end_RVGeneratorDestruct-RVGeneratorDestruct

	.global RVGeneratorDelete
	.type RVGeneratorDelete, @function

RVGeneratorDelete:

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
	.global RVGeneratorDestruct
	.global free
	mv          s1, a0
	call        RVGeneratorDestruct

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_RVGeneratorDelete:
	.size RVGeneratorDelete, .func_end_RVGeneratorDelete-RVGeneratorDelete

	.local  NewSavedArgumentRegister
	.type NewSavedArgumentRegister, @function

NewSavedArgumentRegister:

	// *** Basic block 0

	.global malloc
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
	mv          s3, a2
	li          a0, 16		// 0x10 ASCII \x10
	call        malloc

	// *** Basic block 1

	mv          s4, a0
	sw          s1, 4(s4)
	sw          s2, 0(s4)
	sw          s3, 8(s4)
	mv          a0, s4

	// *** Basic block 2

.NewSavedArgumentRegister_label_29:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewSavedArgumentRegister:
	.size NewSavedArgumentRegister, .func_end_NewSavedArgumentRegister-NewSavedArgumentRegister

	.local  SavedArgumentRegisterDelete
	.type SavedArgumentRegisterDelete, @function

SavedArgumentRegisterDelete:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	.global free
	j           free
.func_end_SavedArgumentRegisterDelete:
	.size SavedArgumentRegisterDelete, .func_end_SavedArgumentRegisterDelete-SavedArgumentRegisterDelete

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

	.local  Zero
	.type Zero, @function

Zero:

	// *** Basic block 0

	.local Emit
	.local NewInstruction
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
	ld          t0, 408(s1)
	bne         t0, x0, .Zero_label_28

	// *** Basic block 1

	li          t0, 207		// 0xcf ASCII \xcf
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 2

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 3

	sd          a0, 408(s1)

	// *** Basic block 4

.Zero_label_28:
	ld          a0, 408(s1)

	// *** Basic block 5

.Zero_label_32:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Zero:
	.size Zero, .func_end_Zero-Zero

	.local  Tmp
	.type Tmp, @function

Tmp:

	// *** Basic block 0

	.local Emit
	.local NewInstruction
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
	ld          t0, 416(s1)
	bne         t0, x0, .Tmp_label_28

	// *** Basic block 1

	li          t0, 208		// 0xd0 ASCII \xd0
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 2

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 3

	sd          a0, 416(s1)

	// *** Basic block 4

.Tmp_label_28:
	ld          a0, 416(s1)

	// *** Basic block 5

.Tmp_label_32:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Tmp:
	.size Tmp, .func_end_Tmp-Tmp

	.local  IntArgumentRegister
	.type IntArgumentRegister, @function

IntArgumentRegister:

	// *** Basic block 0

	.local EmitSymbol
	.local NewInstruction
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
	addi        t0, s1, 256
	slli        s3, s2, 3
	add         t0, t0, s3
	ld          t0, 0(t0)
	bne         t0, x0, .IntArgumentRegister_label_34

	// *** Basic block 1

	addi        t0, s1, 256
	add         s4, t0, s3
	addi        a0, s2, 190
	call        NewInstruction

	// *** Basic block 2

	mv          a1, a0
	mv          a0, s1
	call        EmitSymbol

	// *** Basic block 3

	sd          a0, 0(s4)

	// *** Basic block 4

.IntArgumentRegister_label_34:
	addi        t0, s1, 256
	add         t0, t0, s3
	ld          a0, 0(t0)

	// *** Basic block 5

.IntArgumentRegister_label_39:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_IntArgumentRegister:
	.size IntArgumentRegister, .func_end_IntArgumentRegister-IntArgumentRegister

	.local  FloatingPointArgumentRegister
	.type FloatingPointArgumentRegister, @function

FloatingPointArgumentRegister:

	// *** Basic block 0

	.local EmitSymbol
	.local NewInstruction
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
	addi        t0, s1, 320
	slli        s3, s2, 3
	add         t0, t0, s3
	ld          t0, 0(t0)
	bne         t0, x0, .FloatingPointArgumentRegister_label_34

	// *** Basic block 1

	addi        t0, s1, 320
	add         s4, t0, s3
	addi        a0, s2, 198
	call        NewInstruction

	// *** Basic block 2

	mv          a1, a0
	mv          a0, s1
	call        EmitSymbol

	// *** Basic block 3

	sd          a0, 0(s4)

	// *** Basic block 4

.FloatingPointArgumentRegister_label_34:
	addi        t0, s1, 320
	add         t0, t0, s3
	ld          a0, 0(t0)

	// *** Basic block 5

.FloatingPointArgumentRegister_label_39:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_FloatingPointArgumentRegister:
	.size FloatingPointArgumentRegister, .func_end_FloatingPointArgumentRegister-FloatingPointArgumentRegister

	.local  IntVariableRegister
	.type IntVariableRegister, @function

IntVariableRegister:

	// *** Basic block 0

	.global malloc
	.global TargetInitInstruction
	.global VectorAppend
	.local EmitSymbol
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
	mv          s3, a2
	mv          s4, x0
	addi        t0, s1, 384
	ld          s5, 8(t0)
	bge         x0, s5, .IntVariableRegister_label_61

	// *** Basic block 1

	ld          t0, 384(s1)

	// *** Basic block 2

.IntVariableRegister_label_34:
	slli        t1, s4, 3
	add         t0, t0, t1
	ld          s6, 0(t0)
	lb          t1, 12(s6)
	not         t0, t1
	beqz        t0, .IntVariableRegister_label_49

	// *** Basic block 3

	lw          t1, 8(s6)
	sub         t1, t1, s2
	seqz        t0, t1

	// *** Basic block 4

.IntVariableRegister_label_49:
	beqz        t0, .IntVariableRegister_label_56

	// *** Basic block 5

	ld          a0, 0(s6)

	// *** Basic block 6

.IntVariableRegister_label_53:
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

	// *** Basic block 7

.IntVariableRegister_label_56:

	// *** Basic block 8

.IntVariableRegister_label_57:
	addi        s4, s4, 1
	bge         s4, s5, .IntVariableRegister_label_34

	// *** Basic block 9

.IntVariableRegister_label_61:
	li          t0, 16		// 0x10 ASCII \x10
	mv          a0, t0
	call        malloc

	// *** Basic block 10

	mv          s5, a0
	sw          s2, 8(s5)
	li          t0, 120		// 0x78 ASCII 'x'
	mv          a0, t0
	call        malloc

	// *** Basic block 11

	mv          s6, a0
	li          t0, 33		// 0x21 ASCII '!'
	mv          a1, t0
	mv          a0, s6
	call        TargetInitInstruction

	// *** Basic block 12

	sd          s3, 112(s6)
	sd          s6, 0(s5)
	sb          x0, 12(s5)
	addi        a0, s1, 384
	mv          a1, s5
	call        VectorAppend

	// *** Basic block 13

	ld          a1, 0(s5)
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
	j           EmitSymbol
.func_end_IntVariableRegister:
	.size IntVariableRegister, .func_end_IntVariableRegister-IntVariableRegister

	.local  FloatingPointVariableRegister
	.type FloatingPointVariableRegister, @function

FloatingPointVariableRegister:

	// *** Basic block 0

	.global malloc
	.global TargetInitInstruction
	.global VectorAppend
	.local EmitSymbol
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
	mv          s3, a2
	mv          s4, x0
	addi        t0, s1, 384
	ld          s5, 8(t0)
	bge         x0, s5, .FloatingPointVariableRegister_label_61

	// *** Basic block 1

	ld          t0, 384(s1)

	// *** Basic block 2

.FloatingPointVariableRegister_label_35:
	slli        t1, s4, 3
	add         t0, t0, t1
	ld          s6, 0(t0)
	lb          t0, 12(s6)
	beqz        t0, .FloatingPointVariableRegister_label_49

	// *** Basic block 3

	lw          t1, 8(s6)
	sub         t1, t1, s2
	seqz        t0, t1

	// *** Basic block 4

.FloatingPointVariableRegister_label_49:
	beqz        t0, .FloatingPointVariableRegister_label_56

	// *** Basic block 5

	ld          a0, 0(s6)

	// *** Basic block 6

.FloatingPointVariableRegister_label_53:
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

	// *** Basic block 7

.FloatingPointVariableRegister_label_56:

	// *** Basic block 8

.FloatingPointVariableRegister_label_57:
	addi        s4, s4, 1
	bge         s4, s5, .FloatingPointVariableRegister_label_35

	// *** Basic block 9

.FloatingPointVariableRegister_label_61:
	li          t0, 16		// 0x10 ASCII \x10
	mv          a0, t0
	call        malloc

	// *** Basic block 10

	mv          s5, a0
	sw          s2, 8(s5)
	sw          s2, 8(s5)
	li          t0, 120		// 0x78 ASCII 'x'
	mv          a0, t0
	call        malloc

	// *** Basic block 11

	mv          s6, a0
	li          t0, 34		// 0x22 ASCII '"'
	mv          a1, t0
	mv          a0, s6
	call        TargetInitInstruction

	// *** Basic block 12

	sd          s3, 112(s6)
	sd          s6, 0(s5)
	li          t0, 1		// 0x1 ASCII \x1
	sb          t0, 12(s5)
	addi        a0, s1, 384
	mv          a1, s5
	call        VectorAppend

	// *** Basic block 13

	ld          a1, 0(s5)
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
	j           EmitSymbol
.func_end_FloatingPointVariableRegister:
	.size FloatingPointVariableRegister, .func_end_FloatingPointVariableRegister-FloatingPointVariableRegister

	.local  AddImmediate
	.type AddImmediate, @function

AddImmediate:

	// *** Basic block 0

	.local GetIntConstant
	.local Emit
	.local NewInstruction2
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
	mv          s1, a2
	mv          s2, a0
	mv          s3, a1
	mv          s4, s1
	bge         s1, x0, .AddImmediate_label_28

	// *** Basic block 1

	neg         s4, s1

	// *** Basic block 2

.AddImmediate_label_28:
	mv          a3, s1
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	mv          a1, x0
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 3

	mv          s5, a0
	li          t0, 2047		// 0x7ff
	bge         s4, t0, .AddImmediate_label_66

	// *** Basic block 4

	mv          a2, s5
	mv          a1, s3
	li          t0, 53		// 0x35 ASCII '5'
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 5

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

	// *** Basic block 8

.AddImmediate_label_66:
	mv          a1, s5
	li          t0, 174		// 0xae ASCII \xae
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 9

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 10

	mv          s4, a0
	mv          a2, s4
	mv          a1, s3
	li          t0, 62		// 0x3e ASCII '>'
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 11

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
.func_end_AddImmediate:
	.size AddImmediate, .func_end_AddImmediate-AddImmediate

	.local  SetDestOrMove
	.type SetDestOrMove, @function

SetDestOrMove:

	// *** Basic block 0

	.global printf
	.global RVGeneratesOutput
	.global TargetSetDest
	.local Emit
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
	mv          s1, a2
	mv          s2, a1
	mv          s3, a0
	mv          s4, a3
	lw          t0, 20(s1)
	li          t1, 29		// 0x1d ASCII \x1d
	bne         t0, t1, .SetDestOrMove_label_37

	// *** Basic block 1

	lla         a0, .str.182
	call        printf

	// *** Basic block 2

.SetDestOrMove_label_37:
	lw          s5, 16(s2)
	ld          t0, 24(s2)
	sub         t1, t0, x0
	seqz        s6, t1
	bne         t0, x0, .SetDestOrMove_label_54

	// *** Basic block 3

	mv          a0, s5
	call        RVGeneratesOutput

	// *** Basic block 5

.SetDestOrMove_label_54:
	beqz        s6, .SetDestOrMove_label_67

	// *** Basic block 6

	mv          a1, s1
	mv          a0, s2
	call        TargetSetDest

	// *** Basic block 7

	mv          a0, s2

	// *** Basic block 8

.SetDestOrMove_label_64:
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

	// *** Basic block 9

.SetDestOrMove_label_67:
	mv          a2, s2
	mv          a1, s1
	mv          a0, s4
	call        NewInstruction2

	// *** Basic block 10

	mv          a1, a0
	mv          a0, s3
	call        Emit

	// *** Basic block 11

	mv          a0, s1
	j           .SetDestOrMove_label_64
.func_end_SetDestOrMove:
	.size SetDestOrMove, .func_end_SetDestOrMove-SetDestOrMove

	.local  SetDestOrMoveToArgReg
	.type SetDestOrMoveToArgReg, @function

SetDestOrMoveToArgReg:

	// *** Basic block 0

	.local SetDestOrMove
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
	sd s8, 0(sp)
	// End of stack frame
	mv          s1, a1
	mv          s2, a0
	mv          s3, a2
	mv          s4, a3
	mv          s5, a4
	li          s6, 1		// 0x1 ASCII \x1
	mv          s7, x0
	addi        t0, s1, 48
	ld          s8, 8(t0)
	bge         x0, s8, .SetDestOrMoveToArgReg_label_58

	// *** Basic block 1

	ld          t0, 48(s1)
	ld          t1, 72(s1)

	// *** Basic block 2

.SetDestOrMoveToArgReg_label_40:
	slli        t2, s7, 3
	add         t0, t0, t2
	ld          s1, 0(t0)
	ld          t0, 72(s1)
	beq         t0, t1, .SetDestOrMoveToArgReg_label_53

	// *** Basic block 3

	mv          s6, x0
	j           .SetDestOrMoveToArgReg_label_58

	// *** Basic block 4

.SetDestOrMoveToArgReg_label_53:

	// *** Basic block 5

.SetDestOrMoveToArgReg_label_54:
	addi        s7, s7, 1
	bge         s7, s8, .SetDestOrMoveToArgReg_label_40

	// *** Basic block 6

.SetDestOrMoveToArgReg_label_58:
	beqz        s6, .SetDestOrMoveToArgReg_label_75

	// *** Basic block 7

	mv          a3, s5
	mv          a2, s4
	mv          a1, s3
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
	j           SetDestOrMove

	// *** Basic block 9

.SetDestOrMoveToArgReg_label_72:
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

	// *** Basic block 10

.SetDestOrMoveToArgReg_label_75:
	mv          a2, s3
	mv          a1, s4
	mv          a0, s5
	call        NewInstruction2

	// *** Basic block 11

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 12

	mv          a0, s4
	j           .SetDestOrMoveToArgReg_label_72
.func_end_SetDestOrMoveToArgReg:
	.size SetDestOrMoveToArgReg, .func_end_SetDestOrMoveToArgReg-SetDestOrMoveToArgReg

	.local  AddValue
	.type AddValue, @function

AddValue:

	// *** Basic block 0

	.global TargetIsConst
	.local AddImmediate
	.global TargetIntValue
	.local Emit
	.local NewInstruction2
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
	mv          s1, a2
	mv          s2, a0
	mv          s3, a1
	mv          a0, s1
	call        TargetIsConst

	// *** Basic block 1

	beqz        a0, .AddValue_label_39

	// *** Basic block 2

	mv          a0, s1
	call        TargetIntValue

	// *** Basic block 3

	mv          a2, a0
	mv          a1, s3
	mv          a0, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           AddImmediate

	// *** Basic block 5

.AddValue_label_36:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 6

.AddValue_label_39:
	lw          t0, 16(s1)
	li          t1, 207		// 0xcf ASCII \xcf
	bne         t0, t1, .AddValue_label_51

	// *** Basic block 7

	mv          a0, s3
	j           .AddValue_label_36

	// *** Basic block 8

.AddValue_label_51:
	mv          a2, s1
	mv          a1, s3
	li          t0, 62		// 0x3e ASCII '>'
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 9

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
.func_end_AddValue:
	.size AddValue, .func_end_AddValue-AddValue

	.local  LocalVariableOffset
	.type LocalVariableOffset, @function

LocalVariableOffset:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	lw          t0, 128(a0)
	sub         t0, a1, t0
	addi        a0, t0, -16

	// *** Basic block 1

.LocalVariableOffset_label_18:
	ret         
.func_end_LocalVariableOffset:
	.size LocalVariableOffset, .func_end_LocalVariableOffset-LocalVariableOffset

	.local  PagedOffsetFrom
	.type PagedOffsetFrom, @function

PagedOffsetFrom:

	// *** Basic block 0

	.local AddImmediate
	.global malloc
	.global VectorAppend
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
	mv          s1, a2
	mv          s2, a0
	mv          s3, a1
	mv          s4, a3
	bge         s1, x0, .PagedOffsetFrom_label_32

	// *** Basic block 1

	neg         t0, s1
	andi        t0, t0, -2048
	neg         s5, t0
	j           .PagedOffsetFrom_label_34

	// *** Basic block 2

.PagedOffsetFrom_label_32:
	andi        s5, s1, -2048

	// *** Basic block 3

.PagedOffsetFrom_label_34:
	mv          s6, x0
	mv          s7, x0
	addi        t0, s2, 232
	ld          s8, 8(t0)
	bge         x0, s8, .PagedOffsetFrom_label_63

	// *** Basic block 4

	ld          t0, 232(s2)

	// *** Basic block 5

.PagedOffsetFrom_label_45:
	slli        t1, s7, 3
	add         t0, t0, t1
	ld          s9, 0(t0)
	lw          t0, 8(s9)
	bne         t0, s5, .PagedOffsetFrom_label_58

	// *** Basic block 6

	ld          s6, 0(s9)
	j           .PagedOffsetFrom_label_63

	// *** Basic block 7

.PagedOffsetFrom_label_58:

	// *** Basic block 8

.PagedOffsetFrom_label_59:
	addi        s7, s7, 1
	bge         s7, s8, .PagedOffsetFrom_label_45

	// *** Basic block 9

.PagedOffsetFrom_label_63:
	bne         s6, x0, .PagedOffsetFrom_label_89

	// *** Basic block 10

	mv          a2, s5
	mv          a1, s3
	mv          a0, s2
	call        AddImmediate

	// *** Basic block 11

	mv          s6, a0
	li          t0, 16		// 0x10 ASCII \x10
	mv          a0, t0
	call        malloc

	// *** Basic block 12

	mv          s8, a0
	sd          s6, 0(s8)
	sw          s5, 8(s8)
	addi        a0, s2, 232
	mv          a1, s8
	call        VectorAppend

	// *** Basic block 13

.PagedOffsetFrom_label_89:
	sub         t0, s1, s5
	sw          t0, 0(s4)
	mv          a0, s6

	// *** Basic block 14

.PagedOffsetFrom_label_94:
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
.func_end_PagedOffsetFrom:
	.size PagedOffsetFrom, .func_end_PagedOffsetFrom-PagedOffsetFrom

	.local  OffsetFrom
	.type OffsetFrom, @function

OffsetFrom:

	// *** Basic block 0

	.global RVIsPossibleImmediate
	.local AddImmediate
	.local PagedOffsetFrom
	addi sp, sp, -80
	// Saved return address (offset 72) and frame pointer (offset 64)
	sd ra, 72(sp)
	sd s0, 64(sp)
	addi s0, sp, 80
	// Local vars at offset -32(s0)
	// Saved integer registers.
	sd s1, 40(sp)
	sd s2, 32(sp)
	sd s3, 24(sp)
	sd s4, 16(sp)
	sd s5, 8(sp)
	// End of stack frame
	mv          s1, a2
	mv          s2, a0
	mv          s3, a1
	mv          a0, s1
	call        RVIsPossibleImmediate

	// *** Basic block 1

	mv          s4, a0
	beqz        s4, .OffsetFrom_label_33

	// *** Basic block 2

	mv          a2, s1
	mv          a1, s3
	mv          a0, s2
	call        AddImmediate

	// *** Basic block 3


	// *** Basic block 4

.OffsetFrom_label_30:
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

	// *** Basic block 5

.OffsetFrom_label_33:
	addi        a3, s0, -32
	mv          a2, s1
	mv          a1, s3
	mv          a0, s2
	call        PagedOffsetFrom

	// *** Basic block 6

	mv          s4, a0
	lw          s5, -32(s0)
	bnez        s5, .OffsetFrom_label_54

	// *** Basic block 7

	mv          a0, s4
	j           .OffsetFrom_label_30

	// *** Basic block 8

.OffsetFrom_label_54:
	mv          a2, s5
	mv          a1, s4
	mv          a0, s2
	call        AddImmediate

	// *** Basic block 9

	j           .OffsetFrom_label_30
.func_end_OffsetFrom:
	.size OffsetFrom, .func_end_OffsetFrom-OffsetFrom

	.local  LoadImmediate
	.type LoadImmediate, @function

LoadImmediate:

	// *** Basic block 0

	.global RVIsPossibleImmediate
	.local Emit
	.local NewInstruction2
	.local GetIntConstant
	.local PagedOffsetFrom
	addi sp, sp, -80
	// Saved return address (offset 72) and frame pointer (offset 64)
	sd ra, 72(sp)
	sd s0, 64(sp)
	addi s0, sp, 80
	// Local vars at offset -32(s0)
	// Saved integer registers.
	sd s1, 40(sp)
	sd s2, 32(sp)
	sd s3, 24(sp)
	sd s4, 16(sp)
	sd s5, 8(sp)
	// End of stack frame
	mv          s1, a3
	mv          s2, a0
	mv          s3, a1
	mv          s4, a2
	mv          a0, s1
	call        RVIsPossibleImmediate

	// *** Basic block 1

	beqz        a0, .LoadImmediate_label_53

	// *** Basic block 2

	mv          a3, s1
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	mv          a1, x0
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 3

	mv          a2, a0
	mv          a1, s4
	mv          a0, s3
	call        NewInstruction2

	// *** Basic block 4

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 5


	// *** Basic block 6

.LoadImmediate_label_50:
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

.LoadImmediate_label_53:
	addi        a3, s0, -32
	mv          a2, s1
	mv          a1, s4
	mv          a0, s2
	call        PagedOffsetFrom

	// *** Basic block 8

	mv          s5, a0
	lw          a3, -32(s0)
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	mv          a1, x0
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 9

	mv          a2, a0
	mv          a1, s5
	mv          a0, s3
	call        NewInstruction2

	// *** Basic block 10

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 11

	j           .LoadImmediate_label_50
.func_end_LoadImmediate:
	.size LoadImmediate, .func_end_LoadImmediate-LoadImmediate

	.local  StoreImmediate
	.type StoreImmediate, @function

StoreImmediate:

	// *** Basic block 0

	.global RVIsPossibleImmediate
	.local Emit
	.local NewInstruction3
	.local GetIntConstant
	.local PagedOffsetFrom
	addi sp, sp, -80
	// Saved return address (offset 72) and frame pointer (offset 64)
	sd ra, 72(sp)
	sd s0, 64(sp)
	addi s0, sp, 80
	// Local vars at offset -32(s0)
	// Saved integer registers.
	sd s1, 40(sp)
	sd s2, 32(sp)
	sd s3, 24(sp)
	sd s4, 16(sp)
	sd s5, 8(sp)
	sd s6, 0(sp)
	// End of stack frame
	mv          s1, a4
	mv          s2, a0
	mv          s3, a1
	mv          s4, a2
	mv          s5, a3
	mv          a0, s1
	call        RVIsPossibleImmediate

	// *** Basic block 1

	beqz        a0, .StoreImmediate_label_58

	// *** Basic block 2

	mv          a3, s1
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	mv          a1, x0
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 3

	mv          a3, a0
	mv          a2, s5
	mv          a1, s4
	mv          a0, s3
	call        NewInstruction3

	// *** Basic block 4

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 5


	// *** Basic block 6

.StoreImmediate_label_55:
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

	// *** Basic block 7

.StoreImmediate_label_58:
	addi        a3, s0, -32
	mv          a2, s1
	mv          a1, s5
	mv          a0, s2
	call        PagedOffsetFrom

	// *** Basic block 8

	mv          s6, a0
	lw          a3, -32(s0)
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	mv          a1, x0
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 9

	mv          a3, a0
	mv          a2, s6
	mv          a1, s4
	mv          a0, s3
	call        NewInstruction3

	// *** Basic block 10

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 11

	j           .StoreImmediate_label_55
.func_end_StoreImmediate:
	.size StoreImmediate, .func_end_StoreImmediate-StoreImmediate

	.local  Memcpy
	.type Memcpy, @function

Memcpy:

	// *** Basic block 0

	.local LoadImmediate
	.local StoreImmediate
	.local Emit
	.local NewInstruction1
	.local GetIntConstant
	.local SetDestOrMove
	.local IntArgumentRegister
	.local OffsetFrom
	.local NewInstruction2
	.local GetSymbol
	sd          a0, -0(s0)	// Spilled @147
	addi sp, sp, -128
	// Saved return address (offset 120) and frame pointer (offset 112)
	sd ra, 120(sp)
	sd s0, 112(sp)
	addi s0, sp, 128
	// Local vars at offset -16(s0)
	// Spilled register region: 16 bytes at -32(s0) to -16(s0)
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
	mv          s1, a3
	sd          s1, -32(s0)	// Spilled @29
	mv          s2, a0
	mv          s3, a2
	mv          s4, a4
	mv          s5, a1
	mv          s6, a5
	mv          s7, a6
	li          t0, 40		// 0x28 ASCII '('
	bge         s1, t0, .Memcpy_label_152

	// *** Basic block 1

	andi        s8, s1, 7
	srai        s9, s1, 3
	mv          s10, x0
	mv          s11, x0
	bge         x0, s9, .Memcpy_label_100

	// *** Basic block 2

.Memcpy_label_66:
	mv          a3, s4
	mv          a2, s3
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s2
	call        LoadImmediate

	// *** Basic block 3

	sd          a0, -24(s0)	// Spilled @77
	mv          a4, s6
	mv          a3, s5
	mv          a2, a0
	li          t0, 84		// 0x54 ASCII 'T'
	mv          a1, t0
	mv          a0, s2
	call        StoreImmediate

	// *** Basic block 4

	mv          s10, a0

	// *** Basic block 5

.Memcpy_label_92:
	addi        s11, s11, 1
	addi        s4, s4, 8
	addi        s6, s6, 8
	bge         s11, s9, .Memcpy_label_66

	// *** Basic block 6

.Memcpy_label_100:
	mv          s9, x0
	bge         x0, s8, .Memcpy_label_139

	// *** Basic block 7

.Memcpy_label_105:
	mv          a3, s4
	mv          a2, s3
	li          t0, 45		// 0x2d ASCII '-'
	mv          a1, t0
	mv          a0, s2
	call        LoadImmediate

	// *** Basic block 8

	sd          a0, -32(s0)	// Spilled @116
	mv          a4, s6
	mv          a3, s5
	mv          a2, a0
	li          t0, 50		// 0x32 ASCII '2'
	mv          a1, t0
	mv          a0, s2
	call        StoreImmediate

	// *** Basic block 9

	mv          s10, a0

	// *** Basic block 10

.Memcpy_label_131:
	addi        s9, s9, 1
	addi        s4, s4, 1
	addi        s6, s6, 1
	bge         s9, s8, .Memcpy_label_105

	// *** Basic block 11

.Memcpy_label_139:
	beqz        s7, .Memcpy_label_146

	// *** Basic block 12

	lw          t0, 44(s2)
	addi        t0, t0, -1
	sw          t0, 44(s2)

	// *** Basic block 13

.Memcpy_label_146:
	mv          a0, s10

	// *** Basic block 14

.Memcpy_label_149:
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

	// *** Basic block 15

.Memcpy_label_152:
	mv          a3, s1
	li          s8, 2		// 0x2 ASCII \x2
	mv          a2, s8
	mv          a1, x0
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 16

	mv          a1, a0
	li          t0, 174		// 0xae ASCII \xae
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 17

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 18

	sd          a0, -32(s0)	// Spilled @174
	mv          a1, s8
	mv          a0, s2
	call        IntArgumentRegister

	// *** Basic block 19

	li          s8, 18		// 0x12 ASCII \x12
	mv          a3, s8
	mv          a2, a0
	mv          a1, a0
	mv          a0, s2
	call        SetDestOrMove

	// *** Basic block 20

	sd          a0, -32(s0)	// Spilled @192
	beqz        s4, .Memcpy_label_204

	// *** Basic block 21

	mv          a2, s4
	mv          a1, s3
	mv          a0, s2
	call        OffsetFrom

	// *** Basic block 22

	mv          s3, a0

	// *** Basic block 23

.Memcpy_label_204:
	li          t0, 1		// 0x1 ASCII \x1
	mv          a1, t0
	mv          a0, s2
	call        IntArgumentRegister

	// *** Basic block 24

	mv          a3, s8
	mv          a2, a0
	mv          a1, s3
	mv          a0, s2
	call        SetDestOrMove

	// *** Basic block 25

	mv          s1, a0
	sd          s1, -32(s0)	// Spilled @221
	beqz        s6, .Memcpy_label_233

	// *** Basic block 26

	mv          a2, s6
	mv          a1, s5
	mv          a0, s2
	call        OffsetFrom

	// *** Basic block 27

	mv          s5, a0

	// *** Basic block 28

.Memcpy_label_233:
	mv          a1, x0
	mv          a0, s2
	call        IntArgumentRegister
	sd          a0, -32(s0)	// Spilled @238

	// *** Basic block 29

	mv          a3, s8
	mv          a2, a0
	mv          a1, s5
	mv          a0, s2
	call        SetDestOrMove

	// *** Basic block 30

	mv          s8, a0
	mv          a2, a0
	mv          a1, x0
	li          a0, 209		// 0xd1 ASCII \xd1
	call        NewInstruction2

	// *** Basic block 31

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 32

	mv          a2, s1
	mv          a1, a0
	call        NewInstruction2

	// *** Basic block 33

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 34

	mv          a2, s8
	mv          a1, a0
	call        NewInstruction2

	// *** Basic block 35

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 36

	ld          a2, 160(s2)
	mv          a1, x0
	mv          a0, s2
	call        GetSymbol

	// *** Basic block 37

	mv          s1, a0
	mv          a2, a0
	mv          a1, s1
	li          t0, 183		// 0xb7 ASCII \xb7
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 38

	mv          a1, a0
	mv          a0, s2
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
.func_end_Memcpy:
	.size Memcpy, .func_end_Memcpy-Memcpy

	.local  Memzero
	.type Memzero, @function

Memzero:

	// *** Basic block 0

	.local StoreImmediate
	.local Zero
	.local Emit
	.local NewInstruction1
	.local GetIntConstant
	.local SetDestOrMove
	.local IntArgumentRegister
	.local NewInstruction2
	.local GetSymbol
	sd          a0, -0(s0)	// Spilled @114
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
	mv          s1, a2
	sd          s1, -24(s0)	// Spilled @26
	mv          s2, a0
	mv          s3, a1
	mv          s4, a3
	li          t0, 40		// 0x28 ASCII '('
	bge         s1, t0, .Memzero_label_119

	// *** Basic block 1

	andi        s5, s1, 7
	srai        s6, s1, 3
	mv          s7, x0
	mv          s8, x0
	bge         x0, s6, .Memzero_label_79

	// *** Basic block 2

.Memzero_label_54:
	mv          a0, s2
	call        Zero

	// *** Basic block 3

	mv          a4, s4
	mv          a3, s3
	mv          a2, a0
	li          t0, 84		// 0x54 ASCII 'T'
	mv          a1, t0
	mv          a0, s2
	call        StoreImmediate

	// *** Basic block 4

	mv          s7, a0

	// *** Basic block 5

.Memzero_label_72:
	addi        s8, s8, 1
	addi        s4, s4, 8
	bge         s8, s6, .Memzero_label_54

	// *** Basic block 6

.Memzero_label_79:
	mv          s6, x0
	bge         x0, s5, .Memzero_label_108

	// *** Basic block 7

.Memzero_label_84:
	mv          a0, s2
	call        Zero

	// *** Basic block 8

	mv          a4, s4
	mv          a3, s3
	mv          a2, a0
	li          t0, 50		// 0x32 ASCII '2'
	mv          a1, t0
	mv          a0, s2
	call        StoreImmediate

	// *** Basic block 9

	mv          s7, a0

	// *** Basic block 10

.Memzero_label_101:
	addi        s6, s6, 1
	addi        s4, s4, 1
	bge         s6, s5, .Memzero_label_84

	// *** Basic block 11

.Memzero_label_108:
	lw          t0, 44(s2)
	addi        t0, t0, -1
	sw          t0, 44(s2)
	mv          a0, s7

	// *** Basic block 12

.Memzero_label_116:
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

	// *** Basic block 13

.Memzero_label_119:
	mv          a3, s1
	li          s5, 2		// 0x2 ASCII \x2
	mv          a2, s5
	mv          a1, x0
	mv          a0, s2
	call        GetIntConstant
	sd          a0, -24(s0)	// Spilled @129

	// *** Basic block 14

	mv          a1, a0
	li          t0, 174		// 0xae ASCII \xae
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 15

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 16

	mv          s9, a0
	mv          a1, s5
	mv          a0, s2
	call        IntArgumentRegister

	// *** Basic block 17

	li          s5, 18		// 0x12 ASCII \x12
	mv          a3, s5
	mv          a2, a0
	mv          a1, s9
	mv          a0, s2
	call        SetDestOrMove

	// *** Basic block 18

	mv          s10, a0
	li          t0, 1		// 0x1 ASCII \x1
	mv          a1, t0
	mv          a0, s2
	call        IntArgumentRegister

	// *** Basic block 19

	mv          a0, s2
	call        Zero

	// *** Basic block 20

	mv          a2, a0
	mv          a1, a0
	mv          a0, s5
	call        NewInstruction2

	// *** Basic block 21

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 22

	mv          s11, a0
	mv          a1, x0
	mv          a0, s2
	call        IntArgumentRegister

	// *** Basic block 23

	mv          a3, s5
	mv          a2, a0
	mv          a1, s3
	mv          a0, s2
	call        SetDestOrMove

	// *** Basic block 24

	mv          s5, a0
	mv          a2, s10
	mv          a1, x0
	li          a0, 209		// 0xd1 ASCII \xd1
	call        NewInstruction2

	// *** Basic block 25

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 26

	mv          a2, s11
	mv          a1, a0
	call        NewInstruction2

	// *** Basic block 27

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 28

	mv          a2, s5
	mv          a1, a0
	call        NewInstruction2

	// *** Basic block 29

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 30

	ld          a2, 168(s2)
	mv          a1, x0
	mv          a0, s2
	call        GetSymbol

	// *** Basic block 31

	mv          s1, a0
	mv          a2, a0
	mv          a1, s1
	li          t0, 183		// 0xb7 ASCII \xb7
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 32

	mv          a1, a0
	mv          a0, s2
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
	j           Emit
.func_end_Memzero:
	.size Memzero, .func_end_Memzero-Memzero

	.local  IR2RV
	.type IR2RV, @function

IR2RV:

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
	li          s2, 49		// 0x31 ASCII '1'
	blt         s1, s2, .IR2RV_label_196

	// *** Basic block 1

	li          t0, 59		// 0x3b ASCII ';'
	blt         s1, t0, .IR2RV_label_144

	// *** Basic block 2

	beq         s1, t0, .IR2RV_label_398

	// *** Basic block 3

	li          t0, 60		// 0x3c ASCII '<'
	beq         s1, t0, .IR2RV_label_402

	// *** Basic block 4

	li          t0, 61		// 0x3d ASCII '='
	beq         s1, t0, .IR2RV_label_406

	// *** Basic block 5

	li          t0, 62		// 0x3e ASCII '>'
	beq         s1, t0, .IR2RV_label_410

	// *** Basic block 6

	li          t0, 63		// 0x3f ASCII '?'
	beq         s1, t0, .IR2RV_label_414

	// *** Basic block 7

	li          t0, 110		// 0x6e ASCII 'n'
	beq         s1, t0, .IR2RV_label_418

	// *** Basic block 8

	li          t0, 111		// 0x6f ASCII 'o'
	beq         s1, t0, .IR2RV_label_422

	// *** Basic block 9

	li          t0, 112		// 0x70 ASCII 'p'
	beq         s1, t0, .IR2RV_label_426

	// *** Basic block 10

	li          t0, 113		// 0x71 ASCII 'q'
	beq         s1, t0, .IR2RV_label_430

	// *** Basic block 11

	li          t0, 114		// 0x72 ASCII 'r'
	beq         s1, t0, .IR2RV_label_434

	// *** Basic block 12

	li          t0, 115		// 0x73 ASCII 's'
	beq         s1, t0, .IR2RV_label_438

	// *** Basic block 13

	j           .IR2RV_label_478

	// *** Basic block 14

.IR2RV_label_144:
	beq         s1, s2, .IR2RV_label_358

	// *** Basic block 15

	li          t0, 50		// 0x32 ASCII '2'
	beq         s1, t0, .IR2RV_label_362

	// *** Basic block 16

	li          t0, 51		// 0x33 ASCII '3'
	beq         s1, t0, .IR2RV_label_366

	// *** Basic block 17

	li          t0, 52		// 0x34 ASCII '4'
	beq         s1, t0, .IR2RV_label_370

	// *** Basic block 18

	li          t0, 53		// 0x35 ASCII '5'
	beq         s1, t0, .IR2RV_label_374

	// *** Basic block 19

	li          t0, 54		// 0x36 ASCII '6'
	beq         s1, t0, .IR2RV_label_378

	// *** Basic block 20

	li          t0, 55		// 0x37 ASCII '7'
	beq         s1, t0, .IR2RV_label_382

	// *** Basic block 21

	li          t0, 56		// 0x38 ASCII '8'
	beq         s1, t0, .IR2RV_label_386

	// *** Basic block 22

	li          t0, 57		// 0x39 ASCII '9'
	beq         s1, t0, .IR2RV_label_390

	// *** Basic block 23

	li          t0, 58		// 0x3a ASCII ':'
	beq         s1, t0, .IR2RV_label_394

	// *** Basic block 24

	j           .IR2RV_label_478

	// *** Basic block 25

.IR2RV_label_196:
	li          t0, 38		// 0x26 ASCII '&'
	blt         s1, t0, .IR2RV_label_256

	// *** Basic block 26

	beq         s1, t0, .IR2RV_label_314

	// *** Basic block 27

	li          t0, 39		// 0x27 ASCII '''
	beq         s1, t0, .IR2RV_label_318

	// *** Basic block 28

	li          t0, 40		// 0x28 ASCII '('
	beq         s1, t0, .IR2RV_label_322

	// *** Basic block 29

	li          t0, 41		// 0x29 ASCII ')'
	beq         s1, t0, .IR2RV_label_326

	// *** Basic block 30

	li          t0, 42		// 0x2a ASCII '*'
	beq         s1, t0, .IR2RV_label_330

	// *** Basic block 31

	li          t0, 43		// 0x2b ASCII '+'
	beq         s1, t0, .IR2RV_label_334

	// *** Basic block 32

	li          t0, 44		// 0x2c ASCII ','
	beq         s1, t0, .IR2RV_label_338

	// *** Basic block 33

	li          t0, 45		// 0x2d ASCII '-'
	beq         s1, t0, .IR2RV_label_342

	// *** Basic block 34

	li          t0, 46		// 0x2e ASCII '.'
	beq         s1, t0, .IR2RV_label_346

	// *** Basic block 35

	li          t0, 47		// 0x2f ASCII '/'
	beq         s1, t0, .IR2RV_label_350

	// *** Basic block 36

	li          t0, 48		// 0x30 ASCII '0'
	beq         s1, t0, .IR2RV_label_354

	// *** Basic block 37

	j           .IR2RV_label_478

	// *** Basic block 38

.IR2RV_label_256:
	li          t0, 1		// 0x1 ASCII \x1
	beq         s1, t0, .IR2RV_label_474

	// *** Basic block 39

	li          t0, 9		// 0x9 ASCII \x9
	beq         s1, t0, .IR2RV_label_442

	// *** Basic block 40

	li          t0, 10		// 0xa ASCII \xa
	beq         s1, t0, .IR2RV_label_446

	// *** Basic block 41

	li          t0, 11		// 0xb ASCII \xb
	beq         s1, t0, .IR2RV_label_450

	// *** Basic block 42

	li          t0, 12		// 0xc ASCII \xc
	beq         s1, t0, .IR2RV_label_454

	// *** Basic block 43

	li          t0, 13		// 0xd ASCII \xd
	beq         s1, t0, .IR2RV_label_458

	// *** Basic block 44

	li          t0, 14		// 0xe ASCII \xe
	beq         s1, t0, .IR2RV_label_462

	// *** Basic block 45

	li          t0, 15		// 0xf ASCII \xf
	beq         s1, t0, .IR2RV_label_466

	// *** Basic block 46

	li          t0, 16		// 0x10 ASCII \x10
	beq         s1, t0, .IR2RV_label_470

	// *** Basic block 47

	li          t0, 37		// 0x25 ASCII '%'
	beq         s1, t0, .IR2RV_label_308

	// *** Basic block 48

	j           .IR2RV_label_478

	// *** Basic block 49

.IR2RV_label_308:
	li          a0, 62		// 0x3e ASCII '>'

	// *** Basic block 50

.IR2RV_label_311:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 51

.IR2RV_label_314:
	li          a0, 113		// 0x71 ASCII 'q'
	j           .IR2RV_label_311

	// *** Basic block 52

.IR2RV_label_318:
	li          a0, 143		// 0x8f ASCII \x8f
	j           .IR2RV_label_311

	// *** Basic block 53

.IR2RV_label_322:
	li          a0, 62		// 0x3e ASCII '>'
	j           .IR2RV_label_311

	// *** Basic block 54

.IR2RV_label_326:
	li          a0, 63		// 0x3f ASCII '?'
	j           .IR2RV_label_311

	// *** Basic block 55

.IR2RV_label_330:
	li          a0, 114		// 0x72 ASCII 'r'
	j           .IR2RV_label_311

	// *** Basic block 56

.IR2RV_label_334:
	li          a0, 144		// 0x90 ASCII \x90
	j           .IR2RV_label_311

	// *** Basic block 57

.IR2RV_label_338:
	li          a0, 63		// 0x3f ASCII '?'
	j           .IR2RV_label_311

	// *** Basic block 58

.IR2RV_label_342:
	li          a0, 94		// 0x5e ASCII '^'
	j           .IR2RV_label_311

	// *** Basic block 59

.IR2RV_label_346:
	li          a0, 115		// 0x73 ASCII 's'
	j           .IR2RV_label_311

	// *** Basic block 60

.IR2RV_label_350:
	li          a0, 145		// 0x91 ASCII \x91
	j           .IR2RV_label_311

	// *** Basic block 61

.IR2RV_label_354:
	li          a0, 98		// 0x62 ASCII 'b'
	j           .IR2RV_label_311

	// *** Basic block 62

.IR2RV_label_358:
	li          a0, 116		// 0x74 ASCII 't'
	j           .IR2RV_label_311

	// *** Basic block 63

.IR2RV_label_362:
	li          a0, 146		// 0x92 ASCII \x92
	j           .IR2RV_label_311

	// *** Basic block 64

.IR2RV_label_366:
	li          a0, 100		// 0x64 ASCII 'd'
	j           .IR2RV_label_311

	// *** Basic block 65

.IR2RV_label_370:
	li          a0, 68		// 0x44 ASCII 'D'
	j           .IR2RV_label_311

	// *** Basic block 66

.IR2RV_label_374:
	li          a0, 69		// 0x45 ASCII 'E'
	j           .IR2RV_label_311

	// *** Basic block 67

.IR2RV_label_378:
	li          a0, 64		// 0x40 ASCII '@'
	j           .IR2RV_label_311

	// *** Basic block 68

.IR2RV_label_382:
	li          a0, 70		// 0x46 ASCII 'F'
	j           .IR2RV_label_311

	// *** Basic block 69

.IR2RV_label_386:
	li          a0, 71		// 0x47 ASCII 'G'
	j           .IR2RV_label_311

	// *** Basic block 70

.IR2RV_label_390:
	li          a0, 67		// 0x43 ASCII 'C'
	j           .IR2RV_label_311

	// *** Basic block 71

.IR2RV_label_394:
	li          a0, 170		// 0xaa ASCII \xaa
	j           .IR2RV_label_311

	// *** Basic block 72

.IR2RV_label_398:
	li          a0, 170		// 0xaa ASCII \xaa
	j           .IR2RV_label_311

	// *** Basic block 73

.IR2RV_label_402:
	li          a0, 170		// 0xaa ASCII \xaa
	j           .IR2RV_label_311

	// *** Basic block 74

.IR2RV_label_406:
	li          a0, 171		// 0xab ASCII \xab
	j           .IR2RV_label_311

	// *** Basic block 75

.IR2RV_label_410:
	li          a0, 172		// 0xac ASCII \xac
	j           .IR2RV_label_311

	// *** Basic block 76

.IR2RV_label_414:
	li          a0, 173		// 0xad ASCII \xad
	j           .IR2RV_label_311

	// *** Basic block 77

.IR2RV_label_418:
	li          a0, 135		// 0x87 ASCII \x87
	j           .IR2RV_label_311

	// *** Basic block 78

.IR2RV_label_422:
	li          a0, 166		// 0xa6 ASCII \xa6
	j           .IR2RV_label_311

	// *** Basic block 79

.IR2RV_label_426:
	li          a0, 154		// 0x9a ASCII \x9a
	j           .IR2RV_label_311

	// *** Basic block 80

.IR2RV_label_430:
	li          a0, 153		// 0x99 ASCII \x99
	j           .IR2RV_label_311

	// *** Basic block 81

.IR2RV_label_434:
	li          a0, 133		// 0x85 ASCII \x85
	j           .IR2RV_label_311

	// *** Basic block 82

.IR2RV_label_438:
	li          a0, 163		// 0xa3 ASCII \xa3
	j           .IR2RV_label_311

	// *** Basic block 83

.IR2RV_label_442:
	li          a0, 11		// 0xb ASCII \xb
	j           .IR2RV_label_311

	// *** Basic block 84

.IR2RV_label_446:
	li          a0, 12		// 0xc ASCII \xc
	j           .IR2RV_label_311

	// *** Basic block 85

.IR2RV_label_450:
	li          a0, 13		// 0xd ASCII \xd
	j           .IR2RV_label_311

	// *** Basic block 86

.IR2RV_label_454:
	li          a0, 11		// 0xb ASCII \xb
	j           .IR2RV_label_311

	// *** Basic block 87

.IR2RV_label_458:
	li          a0, 18		// 0x12 ASCII \x12
	j           .IR2RV_label_311

	// *** Basic block 88

.IR2RV_label_462:
	li          a0, 19		// 0x13 ASCII \x13
	j           .IR2RV_label_311

	// *** Basic block 89

.IR2RV_label_466:
	li          a0, 20		// 0x14 ASCII \x14
	j           .IR2RV_label_311

	// *** Basic block 90

.IR2RV_label_470:
	li          a0, 18		// 0x12 ASCII \x12
	j           .IR2RV_label_311

	// *** Basic block 91

.IR2RV_label_474:
	li          a0, 4		// 0x4 ASCII \x4
	j           .IR2RV_label_311

	// *** Basic block 92

.IR2RV_label_478:
	lla         a0, .str.183
	lla         a1, .str.184
	lla         a3, .str.185
	li          t0, 1300		// 0x514
	mv          a2, t0
	call        printf

	// *** Basic block 93

	call        abort

	// *** Basic block 94

	mv          a0, x0
	j           .IR2RV_label_311
.func_end_IR2RV:
	.size IR2RV, .func_end_IR2RV-IR2RV

	.local  UseRegisterForVariable
	.type UseRegisterForVariable, @function

UseRegisterForVariable:

	// *** Basic block 0

	.global OptLevel0
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
	mv          s1, a1
	call        OptLevel0

	// *** Basic block 1

	beqz        a0, .UseRegisterForVariable_label_23

	// *** Basic block 2

	mv          a0, x0

	// *** Basic block 3

.UseRegisterForVariable_label_20:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 4

.UseRegisterForVariable_label_23:
	mv          s2, s1
	ld          t0, 136(s2)
	lb          t0, 56(t0)
	slli        t0, t0, 57
	srai        t0, t0, 63
	beqz        t0, .UseRegisterForVariable_label_36

	// *** Basic block 5

	mv          a0, x0
	j           .UseRegisterForVariable_label_20

	// *** Basic block 6

.UseRegisterForVariable_label_36:
	addi        t0, s2, 48
	ld          t0, 8(t0)
	bnez        t0, .UseRegisterForVariable_label_45

	// *** Basic block 7

	mv          a0, x0
	j           .UseRegisterForVariable_label_20

	// *** Basic block 8

.UseRegisterForVariable_label_45:
	li          a0, 1		// 0x1 ASCII \x1
	j           .UseRegisterForVariable_label_20
.func_end_UseRegisterForVariable:
	.size UseRegisterForVariable, .func_end_UseRegisterForVariable-UseRegisterForVariable

	.local  LoadStaticVariableAddress
	.type LoadStaticVariableAddress, @function

LoadStaticVariableAddress:

	// *** Basic block 0

	.local Emit
	.local NewInstruction1
	.local GetLoweredNode
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
	ld          t1, 136(t0)
	lb          t1, 56(t1)
	slli        t1, t1, 60
	srai        t1, t1, 63
	beqz        t1, .LoadStaticVariableAddress_label_32

	// *** Basic block 1

	li          s3, 188		// 0xbc ASCII \xbc
	j           .LoadStaticVariableAddress_label_34

	// *** Basic block 2

.LoadStaticVariableAddress_label_32:
	li          s3, 187		// 0xbb ASCII \xbb

	// *** Basic block 3

.LoadStaticVariableAddress_label_34:
	mv          a0, s1
	call        GetLoweredNode

	// *** Basic block 4

	mv          a1, a0
	mv          a0, s3
	call        NewInstruction1

	// *** Basic block 5

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
.func_end_LoadStaticVariableAddress:
	.size LoadStaticVariableAddress, .func_end_LoadStaticVariableAddress-LoadStaticVariableAddress

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
	li          s5, 83		// 0x53 ASCII 'S'
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
	lla         a0, .str.186
	lla         a1, .str.187
	lla         a3, .str.188
	li          t0, 1366		// 0x556
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

	.local  Materialize
	.type Materialize, @function

Materialize:

	// *** Basic block 0

	.global IRIsConst
	.global IRIsZero
	.local Zero
	.local Emit
	.local NewInstruction1
	.local GetLoweredNode
	.local GetIntConstant
	.global printf
	.global abort
	.global IRIsAutoVariable
	.global TypeIsFloatingPoint
	.local FloatingPointVariableRegister
	.local IntVariableRegister
	.local FramePointer
	.local OffsetFrom
	.local LocalVariableOffset
	.global IRIsArgument
	.global IRIsStaticVariable
	.local LoadStaticVariableAddress
	addi sp, sp, -128
	// Saved return address (offset 120) and frame pointer (offset 112)
	sd ra, 120(sp)
	sd s0, 112(sp)
	addi s0, sp, 128
	// Local vars at offset -32(s0)
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
	// Saved floating point registers.
	fsd fs0, 8(sp)
	// End of stack frame
	mv          s1, a1
	mv          s2, a0
	mv          a0, s1
	call        IRIsConst

	// *** Basic block 1

	beqz        a0, .Materialize_label_244

	// *** Basic block 2

	lw          s3, 20(s1)
	li          s4, 2		// 0x2 ASCII \x2
	blt         s3, s4, .Materialize_label_228

	// *** Basic block 3

	li          t0, 8		// 0x8 ASCII \x8
	blt         t0, s3, .Materialize_label_228

	// *** Basic block 4

	addi        t0, s3, -2
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 5

	j           .Materialize_label_80

	// *** Basic block 6

	j           .Materialize_label_81

	// *** Basic block 7

	j           .Materialize_label_82

	// *** Basic block 8

	j           .Materialize_label_83

	// *** Basic block 9

	j           .Materialize_label_115

	// *** Basic block 10

	j           .Materialize_label_174

	// *** Basic block 11

	j           .Materialize_label_84

	// *** Basic block 12

.Materialize_label_80:

	// *** Basic block 13

.Materialize_label_81:

	// *** Basic block 14

.Materialize_label_82:

	// *** Basic block 15

.Materialize_label_83:

	// *** Basic block 16

.Materialize_label_84:
	mv          a0, s1
	call        IRIsZero

	// *** Basic block 17

	beqz        a0, .Materialize_label_97

	// *** Basic block 18

	mv          a0, s2
	call        Zero

	// *** Basic block 19


	// *** Basic block 20

.Materialize_label_94:
	// Restored registers.
	fld fs0, 88(sp)
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
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 21

.Materialize_label_97:
	mv          a0, s1
	call        GetLoweredNode

	// *** Basic block 22

	mv          a1, a0
	li          t0, 174		// 0xae ASCII \xae
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 23

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 24

	j           .Materialize_label_94

	// *** Basic block 25

.Materialize_label_115:
	fld         fs0, 136(s1)
	fcvt.s.d    ft0, fs0
	fsw         ft0, -32(s0)
	lw          s3, -32(s0)
	bnez        s3, .Materialize_label_134

	// *** Basic block 26

	mv          a0, s2
	call        Zero

	// *** Basic block 27

	mv          s5, a0
	j           .Materialize_label_159

	// *** Basic block 28

.Materialize_label_134:
	mv          a3, s3
	mv          a2, s4
	mv          a1, x0
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 29

	mv          a1, a0
	li          t0, 174		// 0xae ASCII \xae
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 30

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 31

	mv          s5, a0

	// *** Basic block 32

.Materialize_label_159:
	mv          a1, s5
	li          t0, 132		// 0x84 ASCII \x84
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 33

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 34

	j           .Materialize_label_94

	// *** Basic block 35

.Materialize_label_174:
	fld         ft0, 136(s1)
	fsd         ft0, -24(s0)
	ld          s3, -24(s0)
	bnez        s3, .Materialize_label_190

	// *** Basic block 36

	mv          a0, s2
	call        Zero

	// *** Basic block 37

	mv          s4, a0
	j           .Materialize_label_213

	// *** Basic block 38

.Materialize_label_190:
	mv          a3, s3
	li          t0, 3		// 0x3 ASCII \x3
	mv          a2, t0
	mv          a1, x0
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 39

	mv          a1, a0
	li          t0, 174		// 0xae ASCII \xae
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 40

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 41

	mv          s4, a0

	// *** Basic block 42

.Materialize_label_213:
	mv          a1, s4
	li          t0, 168		// 0xa8 ASCII \xa8
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 43

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 44

	j           .Materialize_label_94

	// *** Basic block 45

.Materialize_label_228:
	lla         a0, .str.189
	lla         a1, .str.190
	lla         a3, .str.191
	li          t0, 1421		// 0x58d
	mv          a2, t0
	call        printf

	// *** Basic block 46

	call        abort

	// *** Basic block 47

.Materialize_label_243:

	// *** Basic block 48

.Materialize_label_244:
	mv          a0, s1
	call        IRIsAutoVariable

	// *** Basic block 49

	beqz        a0, .Materialize_label_352

	// *** Basic block 50

	mv          s3, s1
	ld          s6, 80(s1)
	lw          s8, 16(s6)
	addi        t0, s8, -2
	seqz        s7, t0
	li          t0, 2		// 0x2 ASCII \x2
	beq         s8, t0, .Materialize_label_265

	// *** Basic block 51

	addi        t0, s8, -1
	seqz        s7, t0

	// *** Basic block 52

.Materialize_label_265:
	beqz        s7, .Materialize_label_272

	// *** Basic block 53

	addi        t0, s6, 32
	lb          t0, 16(t0)
	slli        t0, t0, 61
	srai        s7, t0, 63

	// *** Basic block 54

.Materialize_label_272:

	// *** Basic block 55

.Materialize_label_274:
	beqz        s7, .Materialize_label_289

	// *** Basic block 56

	j           .Materialize_label_277

	// *** Basic block 57

.Materialize_label_277:
	ld          t0, 136(s3)
	ld          s7, 112(t0)
	mv          a0, s7
	call        GetLoweredNode

	// *** Basic block 58

	j           .Materialize_label_94

	// *** Basic block 59

.Materialize_label_289:
	addi        t0, s1, 96
	lw          s7, 8(t0)
	li          t0, 3221225472		// 0xc0000000
	and         t0, s7, t0
	li          t1, 2147483648		// 0x80000000
	bne         t0, t1, .Materialize_label_331

	// *** Basic block 60

	li          t0, -2147483649		// 0xffffffff7fffffff
	and         s8, s7, t0
	mv          a0, s6
	call        TypeIsFloatingPoint

	// *** Basic block 61

	beqz        a0, .Materialize_label_319

	// *** Basic block 62

	ld          a2, 136(s3)
	mv          a1, s8
	mv          a0, s2
	call        FloatingPointVariableRegister

	// *** Basic block 63

	j           .Materialize_label_94

	// *** Basic block 64

.Materialize_label_319:
	ld          a2, 136(s3)
	mv          a1, s8
	mv          a0, s2
	call        IntVariableRegister

	// *** Basic block 65

	j           .Materialize_label_94

	// *** Basic block 66

.Materialize_label_331:
	mv          a0, s2
	call        FramePointer

	// *** Basic block 67

	mv          s6, a0
	mv          a1, s7
	mv          a0, s2
	call        LocalVariableOffset

	// *** Basic block 68

	mv          a2, a0
	mv          a1, s6
	mv          a0, s2
	call        OffsetFrom

	// *** Basic block 69

	j           .Materialize_label_94

	// *** Basic block 70

.Materialize_label_352:
	mv          a0, s1
	call        IRIsArgument

	// *** Basic block 71

	beqz        a0, .Materialize_label_419

	// *** Basic block 72

	addi        t0, s1, 96
	lw          s7, 8(t0)
	li          t0, 3221225472		// 0xc0000000
	and         t0, s7, t0
	li          t1, 2147483648		// 0x80000000
	bne         t0, t1, .Materialize_label_401

	// *** Basic block 73

	mv          s8, s1
	li          t0, -2147483649		// 0xffffffff7fffffff
	and         s9, s7, t0
	ld          a0, 80(s1)
	call        TypeIsFloatingPoint

	// *** Basic block 74

	beqz        a0, .Materialize_label_389

	// *** Basic block 75

	ld          a2, 136(s8)
	mv          a1, s9
	mv          a0, s2
	call        FloatingPointVariableRegister

	// *** Basic block 76

	j           .Materialize_label_94

	// *** Basic block 77

.Materialize_label_389:
	ld          a2, 136(s8)
	mv          a1, s9
	mv          a0, s2
	call        IntVariableRegister

	// *** Basic block 78

	j           .Materialize_label_94

	// *** Basic block 79

.Materialize_label_401:
	mv          a0, s2
	call        FramePointer

	// *** Basic block 80

	mv          s9, a0
	mv          s10, s7
	mv          a2, s10
	mv          a1, s9
	mv          a0, s2
	call        OffsetFrom

	// *** Basic block 81

	j           .Materialize_label_94

	// *** Basic block 82

.Materialize_label_419:
	mv          a0, s1
	call        IRIsStaticVariable

	// *** Basic block 83

	beqz        a0, .Materialize_label_432

	// *** Basic block 84

	mv          a1, s1
	mv          a0, s2
	call        LoadStaticVariableAddress

	// *** Basic block 85

	j           .Materialize_label_94

	// *** Basic block 86

.Materialize_label_432:

	// *** Basic block 87

.Materialize_label_433:

	// *** Basic block 88

.Materialize_label_434:
	mv          a0, s1
	call        GetLoweredNode

	// *** Basic block 89

	j           .Materialize_label_94
.func_end_Materialize:
	.size Materialize, .func_end_Materialize-Materialize

	.local  ApplyFixups
	.type ApplyFixups, @function

ApplyFixups:

	// *** Basic block 0

	.global TargetApplyFixups
	// Leaf procedure, no stack frame generated
	j           TargetApplyFixups
.func_end_ApplyFixups:
	.size ApplyFixups, .func_end_ApplyFixups-ApplyFixups

	.local  IsPowerOf2
	.type IsPowerOf2, @function

IsPowerOf2:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	snez        a0, t0
	beqz        t0, .IsPowerOf2_label_14

	// *** Basic block 1

	addi        t1, t0, -1
	and         t1, t0, t1
	seqz        a0, t1

	// *** Basic block 2

.IsPowerOf2_label_14:

	// *** Basic block 3

.IsPowerOf2_label_16:
	ret         
.func_end_IsPowerOf2:
	.size IsPowerOf2, .func_end_IsPowerOf2-IsPowerOf2

	.local  Log2
	.type Log2, @function

Log2:

	// *** Basic block 0

	.local MultiplyDeBruijnBitPosition2
	// Leaf procedure, no stack frame generated
	li          t0, 125613361		// 0x77cb531
	mul         t0, a0, t0
	li          t1, 4294967295		// 0xffffffff
	and         t0, t0, t1
	srli        t0, t0, 27
	slli        t0, t0, 2
	lla         t1, MultiplyDeBruijnBitPosition2
	add         t0, t1, t0
	lw          a0, 0(t0)

	// *** Basic block 1

.Log2_label_23:
	ret         
.func_end_Log2:
	.size Log2, .func_end_Log2-Log2

	.local  PopulationCount
	.type PopulationCount, @function

PopulationCount:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, x0
	beqz        t0, .PopulationCount_label_22

	// *** Basic block 1

.PopulationCount_label_14:
	addi        t1, t1, 1

	// *** Basic block 2

.PopulationCount_label_16:
	addi        t2, t0, -1
	and         t0, t0, t2
	beqz        t0, .PopulationCount_label_14

	// *** Basic block 3

.PopulationCount_label_22:
	mv          a0, t1

	// *** Basic block 4

.PopulationCount_label_25:
	ret         
.func_end_PopulationCount:
	.size PopulationCount, .func_end_PopulationCount-PopulationCount

	.local  MultiplyByConstant
	.type MultiplyByConstant, @function

MultiplyByConstant:

	// *** Basic block 0

	.local PopulationCount
	.local GetLoweredNode
	.local Emit
	.local NewInstruction2
	.local GetIntConstant
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
	mv          s2, a0
	ld          s3, 136(a2)
	mv          a0, s3
	call        PopulationCount

	// *** Basic block 1

	mv          s4, a0
	slli        t0, s4, 1
	addi        s5, t0, -1
	andi        t0, s3, 1
	li          s6, 1		// 0x1 ASCII \x1
	bne         t0, s6, .MultiplyByConstant_label_44

	// *** Basic block 2

	addi        s5, s5, -1

	// *** Basic block 3

.MultiplyByConstant_label_44:
	li          t0, 8		// 0x8 ASCII \x8
	bge         t0, s5, .MultiplyByConstant_label_56

	// *** Basic block 4

	mv          a0, x0

	// *** Basic block 5

.MultiplyByConstant_label_53:
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

	// *** Basic block 6

.MultiplyByConstant_label_56:
	mv          s4, x0
	mv          s5, x0
	mv          a0, s1
	call        GetLoweredNode

	// *** Basic block 7

	mv          s7, a0
	mv          s8, x0
	beqz        s3, .MultiplyByConstant_label_150

	// *** Basic block 8

.MultiplyByConstant_label_70:
	andi        t0, s3, 1
	bne         t0, s6, .MultiplyByConstant_label_145

	// *** Basic block 9

	bnez        s8, .MultiplyByConstant_label_81

	// *** Basic block 10

	mv          s1, s7
	j           .MultiplyByConstant_label_107

	// *** Basic block 11

.MultiplyByConstant_label_81:
	mv          a3, s8
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	mv          a1, x0
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 12

	mv          a2, a0
	mv          a1, s7
	li          t0, 59		// 0x3b ASCII ';'
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 13

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 14

	mv          s1, a0

	// *** Basic block 15

.MultiplyByConstant_label_107:
	bne         s4, x0, .MultiplyByConstant_label_113

	// *** Basic block 16

	mv          s4, s1
	j           .MultiplyByConstant_label_119

	// *** Basic block 17

.MultiplyByConstant_label_113:
	bne         s5, x0, .MultiplyByConstant_label_118

	// *** Basic block 18

	mv          s5, s1

	// *** Basic block 19

.MultiplyByConstant_label_118:

	// *** Basic block 20

.MultiplyByConstant_label_119:
	sub         t1, s4, x0
	snez        t0, t1
	beq         s4, x0, .MultiplyByConstant_label_126

	// *** Basic block 21

	sub         t1, s5, x0
	snez        t0, t1

	// *** Basic block 22

.MultiplyByConstant_label_126:
	beqz        t0, .MultiplyByConstant_label_144

	// *** Basic block 23

	mv          a2, s5
	mv          a1, s4
	li          t0, 62		// 0x3e ASCII '>'
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 24

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 25

	mv          s1, a0
	mv          s4, s1
	mv          s5, x0

	// *** Basic block 26

.MultiplyByConstant_label_144:

	// *** Basic block 27

.MultiplyByConstant_label_145:
	addi        s8, s8, 1
	srai        s3, s3, 1
	bnez        s3, .MultiplyByConstant_label_70

	// *** Basic block 28

.MultiplyByConstant_label_150:
	mv          a0, s4
	j           .MultiplyByConstant_label_53
.func_end_MultiplyByConstant:
	.size MultiplyByConstant, .func_end_MultiplyByConstant-MultiplyByConstant

	.local  LowerExpression
	.type LowerExpression, @function

LowerExpression:

	// *** Basic block 0

	.local IR2RV
	.global printf
	.global abort
	.global IRIsConst
	.global RVIsPossibleImmediate
	.local NewInstruction
	.local Materialize
	.local GetLoweredNode
	.local GetIntConstant
	.local Zero
	.local MultiplyByConstant
	.global TypeIsUnsigned
	.local IsPowerOf2
	.local Log2
	.local NewInstruction2
	.local Emit
	.global TargetUpdateOperandUsers
	.local SetLoweredNode
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
	ld          a0, 96(s1)
	beq         a0, x0, .LowerExpression_label_105

	// *** Basic block 1

.LowerExpression_label_102:
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

	// *** Basic block 2

.LowerExpression_label_105:
	lw          a0, 20(s1)
	call        IR2RV

	// *** Basic block 3

	mv          s3, a0
	addi        t0, s1, 24
	ld          s4, 8(t0)
	li          s5, 2		// 0x2 ASCII \x2
	bge         s4, s5, .LowerExpression_label_122

	// *** Basic block 4

	ld          t0, 24(s1)
	j           .LowerExpression_label_139

	// *** Basic block 5

.LowerExpression_label_122:
	lla         a0, .str.192
	lla         a1, .str.193
	lla         a3, .str.194
	li          t0, 1573		// 0x625
	mv          a2, t0
	call        printf

	// *** Basic block 6

	call        abort

	// *** Basic block 7

.LowerExpression_label_139:
	mv          s6, x0
	mv          s7, x0
	sd          s7, -24(s0)	// Spilled @142
	li          t0, 62		// 0x3e ASCII '>'
	beq         s3, t0, .LowerExpression_label_196

	// *** Basic block 8

	li          t0, 63		// 0x3f ASCII '?'
	beq         s3, t0, .LowerExpression_label_303

	// *** Basic block 9

	li          s8, 64		// 0x40 ASCII '@'
	sd          s8, -24(s0)	// Spilled @157
	beq         s3, s8, .LowerExpression_label_370

	// *** Basic block 10

	li          t0, 68		// 0x44 ASCII 'D'
	beq         s3, t0, .LowerExpression_label_371

	// *** Basic block 11

	li          t0, 69		// 0x45 ASCII 'E'
	beq         s3, t0, .LowerExpression_label_372

	// *** Basic block 12

	li          t0, 70		// 0x46 ASCII 'F'
	beq         s3, t0, .LowerExpression_label_920

	// *** Basic block 13

	li          s9, 71		// 0x47 ASCII 'G'
	beq         s3, s9, .LowerExpression_label_851

	// *** Basic block 14

	li          t0, 94		// 0x5e ASCII '^'
	beq         s3, t0, .LowerExpression_label_474

	// *** Basic block 15

	li          t0, 98		// 0x62 ASCII 'b'
	beq         s3, t0, .LowerExpression_label_625

	// *** Basic block 16

	li          t0, 100		// 0x64 ASCII 'd'
	beq         s3, t0, .LowerExpression_label_732

	// *** Basic block 17

.LowerExpression_label_194:
	j           .LowerExpression_label_989

	// *** Basic block 18

.LowerExpression_label_196:
	bne         s4, s5, .LowerExpression_label_203

	// *** Basic block 19

	j           .LowerExpression_label_218

	// *** Basic block 20

.LowerExpression_label_203:
	lla         a0, .str.195
	lla         a1, .str.196
	lla         a3, .str.197
	li          t0, 1585		// 0x631
	mv          a2, t0
	call        printf

	// *** Basic block 21

	call        abort

	// *** Basic block 22

.LowerExpression_label_218:
	ld          t0, 24(s1)
	ld          s5, 0(t0)
	ld          s7, 8(t0)
	mv          a0, s5
	call        IRIsConst

	// *** Basic block 23

	mv          s8, a0
	beqz        a0, .LowerExpression_label_235

	// *** Basic block 24

	mv          a0, s7
	call        IRIsConst

	// *** Basic block 25

	mv          s8, a0

	// *** Basic block 26

.LowerExpression_label_235:
	bnez        s8, .LowerExpression_label_989

	// *** Basic block 27

.LowerExpression_label_237:
	mv          a0, s7
	call        IRIsConst

	// *** Basic block 28

	beqz        a0, .LowerExpression_label_269

	// *** Basic block 29

	ld          s9, 136(s7)
	mv          a0, s9
	call        RVIsPossibleImmediate

	// *** Basic block 30

	beqz        a0, .LowerExpression_label_267

	// *** Basic block 31

	li          t0, 53		// 0x35 ASCII '5'
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 32

	mv          s6, a0
	mv          a1, s5
	mv          a0, s2
	call        Materialize

	// *** Basic block 33

	sd          a0, 40(s6)
	addi        s9, s6, 40
	mv          a0, s7
	call        GetLoweredNode

	// *** Basic block 34

	sd          a0, 8(s9)

	// *** Basic block 35

.LowerExpression_label_267:
	j           .LowerExpression_label_301

	// *** Basic block 36

.LowerExpression_label_269:
	mv          a0, s5
	call        IRIsConst

	// *** Basic block 37

	beqz        a0, .LowerExpression_label_300

	// *** Basic block 38

	ld          s9, 136(s5)
	mv          a0, s9
	call        RVIsPossibleImmediate

	// *** Basic block 39

	beqz        a0, .LowerExpression_label_299

	// *** Basic block 40

	li          t0, 53		// 0x35 ASCII '5'
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 41

	mv          s6, a0
	mv          a1, s7
	mv          a0, s2
	call        Materialize

	// *** Basic block 42

	sd          a0, 40(s6)
	addi        s7, s6, 40
	mv          a0, s5
	call        GetLoweredNode

	// *** Basic block 43

	sd          a0, 8(s7)

	// *** Basic block 44

.LowerExpression_label_299:

	// *** Basic block 45

.LowerExpression_label_300:

	// *** Basic block 46

.LowerExpression_label_301:
	j           .LowerExpression_label_989

	// *** Basic block 47

.LowerExpression_label_303:
	bne         s4, s5, .LowerExpression_label_309

	// *** Basic block 48

	j           .LowerExpression_label_324

	// *** Basic block 49

.LowerExpression_label_309:
	lla         a0, .str.198
	lla         a1, .str.199
	lla         a3, .str.200
	li          t0, 1615		// 0x64f
	mv          a2, t0
	call        printf

	// *** Basic block 50

	call        abort

	// *** Basic block 51

.LowerExpression_label_324:
	ld          t0, 24(s1)
	ld          s7, 0(t0)
	ld          s8, 8(t0)
	mv          a0, s8
	call        IRIsConst

	// *** Basic block 52

	beqz        a0, .LowerExpression_label_368

	// *** Basic block 53

	ld          s9, 136(s8)
	mv          a0, s9
	call        RVIsPossibleImmediate

	// *** Basic block 54

	beqz        a0, .LowerExpression_label_367

	// *** Basic block 55

	li          t0, 53		// 0x35 ASCII '5'
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 56

	mv          s6, a0
	mv          a1, s7
	mv          a0, s2
	call        Materialize

	// *** Basic block 57

	sd          a0, 40(s6)
	addi        s7, s6, 40
	neg         a3, s9
	mv          a2, s5
	mv          a1, x0
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 58

	sd          a0, 8(s7)

	// *** Basic block 59

.LowerExpression_label_367:

	// *** Basic block 60

.LowerExpression_label_368:
	j           .LowerExpression_label_989

	// *** Basic block 61

.LowerExpression_label_370:

	// *** Basic block 62

.LowerExpression_label_371:

	// *** Basic block 63

.LowerExpression_label_372:
	bne         s4, s5, .LowerExpression_label_378

	// *** Basic block 64

	j           .LowerExpression_label_393

	// *** Basic block 65

.LowerExpression_label_378:
	lla         a0, .str.201
	lla         a1, .str.202
	lla         a3, .str.203
	li          t0, 1632		// 0x660
	mv          a2, t0
	call        printf

	// *** Basic block 66

	call        abort

	// *** Basic block 67

.LowerExpression_label_393:
	ld          t0, 24(s1)
	ld          s7, 0(t0)
	ld          s8, 8(t0)
	mv          a0, s8
	call        IRIsConst

	// *** Basic block 68

	beqz        a0, .LowerExpression_label_472

	// *** Basic block 69

	ld          s9, 136(s8)
	bnez        s9, .LowerExpression_label_422

	// *** Basic block 70

	li          t0, 11		// 0xb ASCII \xb
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 71

	mv          s6, a0
	mv          a1, s7
	mv          a0, s2
	call        Materialize

	// *** Basic block 72

	sd          a0, 40(s6)
	j           .LowerExpression_label_471

	// *** Basic block 73

.LowerExpression_label_422:
	ld          t0, -24(s0)	// Spilled @157
	blt         s3, t0, .LowerExpression_label_451

	// *** Basic block 74

	li          t0, 69		// 0x45 ASCII 'E'
	blt         t0, s3, .LowerExpression_label_451

	// *** Basic block 75

	addi        t0, s3, -64
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 76

	j           .LowerExpression_label_442

	// *** Basic block 77

	j           .LowerExpression_label_451

	// *** Basic block 78

	j           .LowerExpression_label_451

	// *** Basic block 79

	j           .LowerExpression_label_451

	// *** Basic block 80

	j           .LowerExpression_label_445

	// *** Basic block 81

	j           .LowerExpression_label_448

	// *** Basic block 82

.LowerExpression_label_442:
	li          s3, 59		// 0x3b ASCII ';'
	j           .LowerExpression_label_453

	// *** Basic block 83

.LowerExpression_label_445:
	li          s3, 60		// 0x3c ASCII '<'
	j           .LowerExpression_label_453

	// *** Basic block 84

.LowerExpression_label_448:
	li          s3, 61		// 0x3d ASCII '='
	j           .LowerExpression_label_453

	// *** Basic block 85

.LowerExpression_label_451:
	j           .LowerExpression_label_453

	// *** Basic block 86

.LowerExpression_label_453:
	mv          a0, s3
	call        NewInstruction

	// *** Basic block 87

	mv          s6, a0
	mv          a1, s7
	mv          a0, s2
	call        Materialize

	// *** Basic block 88

	sd          a0, 40(s6)
	addi        s7, s6, 40
	mv          a0, s8
	call        GetLoweredNode

	// *** Basic block 89

	sd          a0, 8(s7)

	// *** Basic block 90

.LowerExpression_label_471:

	// *** Basic block 91

.LowerExpression_label_472:
	j           .LowerExpression_label_989

	// *** Basic block 92

.LowerExpression_label_474:
	bne         s4, s5, .LowerExpression_label_480

	// *** Basic block 93

	j           .LowerExpression_label_495

	// *** Basic block 94

.LowerExpression_label_480:
	lla         a0, .str.204
	lla         a1, .str.205
	lla         a3, .str.206
	li          t0, 1667		// 0x683
	mv          a2, t0
	call        printf

	// *** Basic block 95

	call        abort

	// *** Basic block 96

.LowerExpression_label_495:
	ld          t0, 24(s1)
	ld          s9, 0(t0)
	ld          s10, 8(t0)
	mv          a0, s9
	call        IRIsConst

	// *** Basic block 97

	mv          s11, a0
	bnez        a0, .LowerExpression_label_512

	// *** Basic block 98

	mv          a0, s10
	call        IRIsConst

	// *** Basic block 99

	mv          s11, a0

	// *** Basic block 100

.LowerExpression_label_512:
	beqz        s11, .LowerExpression_label_623

	// *** Basic block 101

	mv          a0, s9
	call        IRIsConst

	// *** Basic block 102

	mv          s8, a0
	beqz        a0, .LowerExpression_label_524

	// *** Basic block 103

	mv          a0, s10
	call        IRIsConst

	// *** Basic block 104

	mv          s8, a0

	// *** Basic block 105

.LowerExpression_label_524:
	bnez        s8, .LowerExpression_label_989

	// *** Basic block 106

.LowerExpression_label_526:
	mv          a0, s9
	call        IRIsConst

	// *** Basic block 107

	beqz        a0, .LowerExpression_label_577

	// *** Basic block 108

	ld          s7, 136(s9)
	bnez        s7, .LowerExpression_label_541

	// *** Basic block 109

	mv          a0, s2
	call        Zero

	// *** Basic block 110

	mv          s6, a0
	j           .LowerExpression_label_575

	// *** Basic block 111

.LowerExpression_label_541:
	li          t0, 1		// 0x1 ASCII \x1
	bne         s7, t0, .LowerExpression_label_560

	// *** Basic block 112

	li          t0, 11		// 0xb ASCII \xb
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 113

	mv          s6, a0
	mv          a1, s10
	mv          a0, s2
	call        Materialize

	// *** Basic block 114

	sd          a0, 40(s6)
	j           .LowerExpression_label_574

	// *** Basic block 115

.LowerExpression_label_560:
	mv          a2, s9
	mv          a1, s10
	mv          a0, s2
	call        MultiplyByConstant

	// *** Basic block 116

	mv          s6, a0
	beq         s6, x0, .LowerExpression_label_989

	// *** Basic block 117

.LowerExpression_label_572:
	li          s7, 1		// 0x1 ASCII \x1

	// *** Basic block 118

.LowerExpression_label_574:

	// *** Basic block 119

.LowerExpression_label_575:
	j           .LowerExpression_label_622

	// *** Basic block 120

.LowerExpression_label_577:
	ld          s7, 136(s10)
	bnez        s7, .LowerExpression_label_588

	// *** Basic block 121

	mv          a0, s2
	call        Zero

	// *** Basic block 122

	mv          s6, a0
	j           .LowerExpression_label_621

	// *** Basic block 123

.LowerExpression_label_588:
	li          t0, 1		// 0x1 ASCII \x1
	bne         s7, t0, .LowerExpression_label_606

	// *** Basic block 124

	li          t0, 11		// 0xb ASCII \xb
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 125

	mv          s6, a0
	mv          a1, s9
	mv          a0, s2
	call        Materialize

	// *** Basic block 126

	sd          a0, 40(s6)
	j           .LowerExpression_label_620

	// *** Basic block 127

.LowerExpression_label_606:
	mv          a2, s10
	mv          a1, s9
	mv          a0, s2
	call        MultiplyByConstant

	// *** Basic block 128

	mv          s6, a0
	beq         s6, x0, .LowerExpression_label_989

	// *** Basic block 129

.LowerExpression_label_618:
	li          s7, 1		// 0x1 ASCII \x1

	// *** Basic block 130

.LowerExpression_label_620:

	// *** Basic block 131

.LowerExpression_label_621:

	// *** Basic block 132

.LowerExpression_label_622:

	// *** Basic block 133

.LowerExpression_label_623:
	j           .LowerExpression_label_989

	// *** Basic block 134

.LowerExpression_label_625:
	bne         s4, s5, .LowerExpression_label_631

	// *** Basic block 135

	j           .LowerExpression_label_646

	// *** Basic block 136

.LowerExpression_label_631:
	lla         a0, .str.207
	lla         a1, .str.208
	lla         a3, .str.209
	li          t0, 1714		// 0x6b2
	mv          a2, t0
	call        printf

	// *** Basic block 137

	call        abort

	// *** Basic block 138

.LowerExpression_label_646:
	ld          t0, 24(s1)
	ld          s9, 0(t0)
	ld          s10, 8(t0)
	mv          a0, s10
	call        IRIsConst

	// *** Basic block 139

	beqz        a0, .LowerExpression_label_730

	// *** Basic block 140

	ld          a0, 80(s1)
	call        TypeIsUnsigned

	// *** Basic block 141

	beqz        a0, .LowerExpression_label_666

	// *** Basic block 142

	li          s11, 60		// 0x3c ASCII '<'
	j           .LowerExpression_label_668

	// *** Basic block 143

.LowerExpression_label_666:
	li          s11, 61		// 0x3d ASCII '='

	// *** Basic block 144

.LowerExpression_label_668:
	ld          s8, 136(s10)
	li          t0, 1		// 0x1 ASCII \x1
	bne         s8, t0, .LowerExpression_label_690
	sd          s8, -24(s0)	// Spilled @672

	// *** Basic block 145

	li          t0, 11		// 0xb ASCII \xb
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 146

	mv          s6, a0
	mv          a1, s9
	mv          a0, s2
	call        Materialize

	// *** Basic block 147

	sd          a0, 40(s6)
	j           .LowerExpression_label_729

	// *** Basic block 148

.LowerExpression_label_690:
	mv          a0, s8
	call        IsPowerOf2

	// *** Basic block 149

	mv          s10, a0
	beqz        a0, .LowerExpression_label_698

	// *** Basic block 150

	slti        s10, s8, 64

	// *** Basic block 151

.LowerExpression_label_698:
	beqz        s10, .LowerExpression_label_728

	// *** Basic block 152

	mv          a0, s8
	call        Log2

	// *** Basic block 153

	mv          s8, a0
	mv          a1, s9
	mv          a0, s2
	call        Materialize

	// *** Basic block 154

	mv          a3, s8
	mv          a2, s5
	mv          a1, x0
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 155

	mv          a2, a0
	mv          a1, a0
	mv          a0, s11
	call        NewInstruction2

	// *** Basic block 156

	mv          s6, a0
	li          s7, 1		// 0x1 ASCII \x1

	// *** Basic block 157

.LowerExpression_label_728:

	// *** Basic block 158

.LowerExpression_label_729:

	// *** Basic block 159

.LowerExpression_label_730:
	j           .LowerExpression_label_989

	// *** Basic block 160

.LowerExpression_label_732:
	bne         s4, s5, .LowerExpression_label_738

	// *** Basic block 161

	j           .LowerExpression_label_753

	// *** Basic block 162

.LowerExpression_label_738:
	lla         a0, .str.210
	lla         a1, .str.211
	lla         a3, .str.212
	li          t0, 1742		// 0x6ce
	mv          a2, t0
	call        printf

	// *** Basic block 163

	call        abort

	// *** Basic block 164

.LowerExpression_label_753:
	ld          t0, 24(s1)
	ld          s10, 0(t0)
	ld          s11, 8(t0)
	mv          a0, s11
	call        IRIsConst

	// *** Basic block 165

	beqz        a0, .LowerExpression_label_849

	// *** Basic block 166

	ld          s8, 136(s11)
	addi        s11, s8, -1
	mv          a0, s11
	call        RVIsPossibleImmediate

	// *** Basic block 167

	beqz        a0, .LowerExpression_label_799

	// *** Basic block 168

	mv          a1, s10
	mv          a0, s2
	call        Materialize

	// *** Basic block 169

	mv          a3, s11
	mv          a2, s5
	mv          a1, x0
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 170

	mv          a2, a0
	mv          a1, a0
	li          t0, 58		// 0x3a ASCII ':'
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 171

	mv          s6, a0
	li          s7, 1		// 0x1 ASCII \x1
	j           .LowerExpression_label_848

	// *** Basic block 172

.LowerExpression_label_799:
	li          t0, 4		// 0x4 ASCII \x4
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 173

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 174

	mv          s8, a0
	sd          s8, -24(s0)	// Spilled @809
	mv          a3, s11
	mv          a2, s5
	mv          a1, x0
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 175

	mv          a2, a0
	mv          a1, s8
	li          t0, 18		// 0x12 ASCII \x12
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 176

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 177

	mv          a1, s10
	mv          a0, s2
	call        Materialize

	// *** Basic block 178

	mv          a2, s8
	mv          a1, a0
	mv          a0, s9
	call        NewInstruction2

	// *** Basic block 179

	mv          s6, a0

	// *** Basic block 180

.LowerExpression_label_848:

	// *** Basic block 181

.LowerExpression_label_849:
	j           .LowerExpression_label_989

	// *** Basic block 182

.LowerExpression_label_851:
	bne         s4, s5, .LowerExpression_label_857

	// *** Basic block 183

	j           .LowerExpression_label_872

	// *** Basic block 184

.LowerExpression_label_857:
	lla         a0, .str.213
	lla         a1, .str.214
	lla         a3, .str.215
	li          t0, 1769		// 0x6e9
	mv          a2, t0
	call        printf

	// *** Basic block 185

	call        abort

	// *** Basic block 186

.LowerExpression_label_872:
	ld          t0, 24(s1)
	ld          s7, 0(t0)
	ld          s8, 8(t0)
	mv          a0, s8
	call        IRIsConst

	// *** Basic block 187

	beqz        a0, .LowerExpression_label_918

	// *** Basic block 188

	ld          s9, 136(s8)
	bnez        s9, .LowerExpression_label_893

	// *** Basic block 189

	mv          a0, s2
	call        Zero

	// *** Basic block 190

	mv          s6, a0
	j           .LowerExpression_label_917

	// *** Basic block 191

.LowerExpression_label_893:
	mv          a0, s9
	call        RVIsPossibleImmediate

	// *** Basic block 192

	beqz        a0, .LowerExpression_label_916

	// *** Basic block 193

	li          t0, 58		// 0x3a ASCII ':'
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 194

	mv          s6, a0
	mv          a1, s7
	mv          a0, s2
	call        Materialize

	// *** Basic block 195

	sd          a0, 40(s6)
	addi        s7, s6, 40
	mv          a0, s8
	call        GetLoweredNode

	// *** Basic block 196

	sd          a0, 8(s7)

	// *** Basic block 197

.LowerExpression_label_916:

	// *** Basic block 198

.LowerExpression_label_917:

	// *** Basic block 199

.LowerExpression_label_918:
	j           .LowerExpression_label_989

	// *** Basic block 200

.LowerExpression_label_920:
	bne         s4, s5, .LowerExpression_label_926

	// *** Basic block 201

	j           .LowerExpression_label_941

	// *** Basic block 202

.LowerExpression_label_926:
	lla         a0, .str.216
	lla         a1, .str.217
	lla         a3, .str.218
	li          t0, 1787		// 0x6fb
	mv          a2, t0
	call        printf

	// *** Basic block 203

	call        abort

	// *** Basic block 204

.LowerExpression_label_941:
	ld          t0, 24(s1)
	ld          s7, 0(t0)
	ld          s8, 8(t0)
	mv          a0, s8
	call        IRIsConst

	// *** Basic block 205

	beqz        a0, .LowerExpression_label_987

	// *** Basic block 206

	ld          s9, 136(s8)
	bnez        s9, .LowerExpression_label_962

	// *** Basic block 207

	mv          a0, s7
	call        GetLoweredNode

	// *** Basic block 208

	mv          s6, a0
	j           .LowerExpression_label_986

	// *** Basic block 209

.LowerExpression_label_962:
	mv          a0, s9
	call        RVIsPossibleImmediate

	// *** Basic block 210

	beqz        a0, .LowerExpression_label_985

	// *** Basic block 211

	li          t0, 57		// 0x39 ASCII '9'
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 212

	mv          s6, a0
	mv          a1, s7
	mv          a0, s2
	call        Materialize

	// *** Basic block 213

	sd          a0, 40(s6)
	addi        s7, s6, 40
	mv          a0, s8
	call        GetLoweredNode

	// *** Basic block 214

	sd          a0, 8(s7)

	// *** Basic block 215

.LowerExpression_label_985:

	// *** Basic block 216

.LowerExpression_label_986:

	// *** Basic block 217

.LowerExpression_label_987:
	j           .LowerExpression_label_989

	// *** Basic block 218

.LowerExpression_label_989:
	bne         s6, x0, .LowerExpression_label_1020

	// *** Basic block 219

	mv          a0, s3
	call        NewInstruction

	// *** Basic block 220

	mv          s6, a0
	mv          s5, x0
	bge         x0, s4, .LowerExpression_label_1019

	// *** Basic block 221

.LowerExpression_label_1001:
	slli        t0, s5, 3
	add         t1, t0, t0
	ld          s7, 0(t1)
	addi        t1, s6, 40
	add         s8, t1, t0
	mv          a1, s7
	mv          a0, s2
	call        Materialize

	// *** Basic block 222

	sd          a0, 0(s8)

	// *** Basic block 223

.LowerExpression_label_1015:
	addi        s5, s5, 1
	bge         s5, s4, .LowerExpression_label_1001

	// *** Basic block 224

.LowerExpression_label_1019:

	// *** Basic block 225

.LowerExpression_label_1020:
	ld          t0, -24(s0)	// Spilled @142
	not         t1, t0
	beqz        t1, .LowerExpression_label_1026

	// *** Basic block 226

	mv          a0, s6
	call        TargetUpdateOperandUsers

	// *** Basic block 227

.LowerExpression_label_1026:
	mv          a1, s6
	mv          a0, s1
	call        SetLoweredNode

	// *** Basic block 228

	mv          a1, s6
	mv          a0, s2
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
	j           Emit
.func_end_LowerExpression:
	.size LowerExpression, .func_end_LowerExpression-LowerExpression

	.local  LowerRmov
	.type LowerRmov, @function

LowerRmov:

	// *** Basic block 0

	.local GetLoweredNode
	.local Materialize
	.local IR2RV
	.local SetLoweredNode
	.local SetDestOrMove
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
	ld          a0, 0(s3)
	call        GetLoweredNode

	// *** Basic block 1

	mv          s4, a0
	ld          a1, 8(s3)
	mv          a0, s2
	call        Materialize

	// *** Basic block 2

	mv          s3, a0
	lw          a0, 20(s1)
	call        IR2RV

	// *** Basic block 3

	mv          s5, a0
	mv          a3, s5
	mv          a2, s4
	mv          a1, s3
	mv          a0, s2
	call        SetDestOrMove

	// *** Basic block 4

	mv          a1, a0
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
.func_end_LowerRmov:
	.size LowerRmov, .func_end_LowerRmov-LowerRmov

	.local  SubtractForComparison
	.type SubtractForComparison, @function

SubtractForComparison:

	// *** Basic block 0

	.global IRIsConst
	.local Emit
	.local NewInstruction2
	.local Materialize
	.global RVIsPossibleImmediate
	.local AddImmediate
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
	mv          s1, a3
	mv          s2, a0
	mv          s3, a2
	mv          a0, s1
	call        IRIsConst

	// *** Basic block 1

	not         t0, a0
	beqz        t0, .SubtractForComparison_label_56

	// *** Basic block 2

	mv          a1, s3
	mv          a0, s2
	call        Materialize

	// *** Basic block 3

	mv          a1, s1
	mv          a0, s2
	call        Materialize

	// *** Basic block 4

	mv          a2, a0
	mv          a1, a0
	li          t0, 63		// 0x3f ASCII '?'
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 5

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

	// *** Basic block 7

.SubtractForComparison_label_53:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 8

.SubtractForComparison_label_56:
	ld          s4, 136(s1)
	bnez        s4, .SubtractForComparison_label_66

	// *** Basic block 9

	mv          a0, x0
	j           .SubtractForComparison_label_53

	// *** Basic block 10

.SubtractForComparison_label_66:
	mv          a0, s4
	call        RVIsPossibleImmediate

	// *** Basic block 11

	beqz        a0, .SubtractForComparison_label_87

	// *** Basic block 12

	mv          a1, s3
	mv          a0, s2
	call        Materialize

	// *** Basic block 13

	neg         a2, s4
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
	j           AddImmediate

	// *** Basic block 15

.SubtractForComparison_label_87:
	mv          a1, s3
	mv          a0, s2
	call        Materialize

	// *** Basic block 16

	mv          a1, s1
	mv          a0, s2
	call        Materialize

	// *** Basic block 17

	mv          a2, a0
	mv          a1, a0
	li          t0, 63		// 0x3f ASCII '?'
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 18

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
.func_end_SubtractForComparison:
	.size SubtractForComparison, .func_end_SubtractForComparison-SubtractForComparison

	.local  CompareLessThanInt
	.type CompareLessThanInt, @function

CompareLessThanInt:

	// *** Basic block 0

	.global IRIsConst
	.local Materialize
	.local Emit
	.local NewInstruction2
	.local NewInstruction1
	.global RVIsPossibleImmediate
	.local GetIntConstant
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
	mv          s1, a3
	mv          s2, a0
	mv          s3, a2
	mv          a0, s1
	call        IRIsConst

	// *** Basic block 1

	not         t0, a0
	beqz        t0, .CompareLessThanInt_label_64

	// *** Basic block 2

	mv          a1, s3
	mv          a0, s2
	call        Materialize

	// *** Basic block 3

	mv          s4, a0
	mv          a1, s1
	mv          a0, s2
	call        Materialize

	// *** Basic block 4

	mv          s5, a0
	mv          a2, s5
	mv          a1, s4
	li          t0, 65		// 0x41 ASCII 'A'
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 5

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

	// *** Basic block 8

.CompareLessThanInt_label_64:
	ld          s6, 136(s1)
	bnez        s6, .CompareLessThanInt_label_91

	// *** Basic block 9

	mv          a1, s3
	mv          a0, s2
	call        Materialize

	// *** Basic block 10

	mv          a1, a0
	li          t0, 177		// 0xb1 ASCII \xb1
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 11

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

	// *** Basic block 13

.CompareLessThanInt_label_91:
	mv          a0, s6
	call        RVIsPossibleImmediate

	// *** Basic block 14

	beqz        a0, .CompareLessThanInt_label_128

	// *** Basic block 15

	mv          a1, s3
	mv          a0, s2
	call        Materialize

	// *** Basic block 16

	mv          a3, s6
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	mv          a1, x0
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 17

	mv          a2, a0
	mv          a1, a0
	li          t0, 54		// 0x36 ASCII '6'
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 18

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

	// *** Basic block 20

.CompareLessThanInt_label_128:
	mv          a1, s3
	mv          a0, s2
	call        Materialize

	// *** Basic block 21

	mv          a1, s1
	mv          a0, s2
	call        Materialize

	// *** Basic block 22

	mv          a2, a0
	mv          a1, a0
	li          t0, 65		// 0x41 ASCII 'A'
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 23

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
.func_end_CompareLessThanInt:
	.size CompareLessThanInt, .func_end_CompareLessThanInt-CompareLessThanInt

	.local  LowerComparison
	.type LowerComparison, @function

LowerComparison:

	// *** Basic block 0

	.global printf
	.global abort
	.local SubtractForComparison
	.local Emit
	.local SetLoweredNode
	.local NewInstruction1
	.local Materialize
	.local CompareLessThanInt
	.local NewInstruction2
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
	li          t1, 2		// 0x2 ASCII \x2
	bne         t0, t1, .LowerComparison_label_52

	// *** Basic block 1

	j           .LowerComparison_label_69

	// *** Basic block 2

.LowerComparison_label_52:
	lla         a0, .str.219
	lla         a1, .str.220
	lla         a3, .str.221
	li          t0, 1898		// 0x76a
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

.LowerComparison_label_69:
	ld          t0, 24(s1)
	ld          s3, 0(t0)
	ld          s4, 8(t0)
	lw          s5, 20(s1)
	li          t0, 64		// 0x40 ASCII '@'
	blt         s5, t0, .LowerComparison_label_771

	// *** Basic block 5

	li          t0, 87		// 0x57 ASCII 'W'
	blt         t0, s5, .LowerComparison_label_771

	// *** Basic block 6

	addi        t0, s5, -64
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 7

	j           .LowerComparison_label_117

	// *** Basic block 8

	j           .LowerComparison_label_181

	// *** Basic block 9

	j           .LowerComparison_label_243

	// *** Basic block 10

	j           .LowerComparison_label_263

	// *** Basic block 11

	j           .LowerComparison_label_296

	// *** Basic block 12

	j           .LowerComparison_label_316

	// *** Basic block 13

	j           .LowerComparison_label_349

	// *** Basic block 14

	j           .LowerComparison_label_382

	// *** Basic block 15

	j           .LowerComparison_label_428

	// *** Basic block 16

	j           .LowerComparison_label_461

	// *** Basic block 17

	j           .LowerComparison_label_494

	// *** Basic block 18

	j           .LowerComparison_label_527

	// *** Basic block 19

	j           .LowerComparison_label_560

	// *** Basic block 20

	j           .LowerComparison_label_593

	// *** Basic block 21

	j           .LowerComparison_label_639

	// *** Basic block 22

	j           .LowerComparison_label_672

	// *** Basic block 23

	j           .LowerComparison_label_705

	// *** Basic block 24

	j           .LowerComparison_label_738

	// *** Basic block 25

	j           .LowerComparison_label_118

	// *** Basic block 26

	j           .LowerComparison_label_182

	// *** Basic block 27

	j           .LowerComparison_label_244

	// *** Basic block 28

	j           .LowerComparison_label_264

	// *** Basic block 29

	j           .LowerComparison_label_297

	// *** Basic block 30

	j           .LowerComparison_label_317

	// *** Basic block 31

.LowerComparison_label_117:

	// *** Basic block 32

.LowerComparison_label_118:
	mv          a3, s4
	mv          a2, s3
	mv          a1, s1
	mv          a0, s2
	call        SubtractForComparison

	// *** Basic block 33

	mv          s7, a0
	bne         s7, x0, .LowerComparison_label_160

	// *** Basic block 34

	mv          a1, s3
	mv          a0, s2
	call        Materialize

	// *** Basic block 35

	mv          a1, a0
	li          t0, 175		// 0xaf ASCII \xaf
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 36

	mv          a1, a0
	mv          a0, s1
	call        SetLoweredNode

	// *** Basic block 37

	mv          a1, a0
	mv          a0, s2
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
	j           Emit

	// *** Basic block 39

.LowerComparison_label_157:
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

	// *** Basic block 40

.LowerComparison_label_160:
	mv          a1, s7
	li          t0, 175		// 0xaf ASCII \xaf
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 41

	mv          a1, a0
	mv          a0, s1
	call        SetLoweredNode

	// *** Basic block 42

	mv          a1, a0
	mv          a0, s2
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
	j           Emit

	// *** Basic block 44

.LowerComparison_label_181:

	// *** Basic block 45

.LowerComparison_label_182:
	mv          a3, s4
	mv          a2, s3
	mv          a1, s1
	mv          a0, s2
	call        SubtractForComparison

	// *** Basic block 46

	mv          s8, a0
	bne         s8, x0, .LowerComparison_label_222

	// *** Basic block 47

	mv          a1, s3
	mv          a0, s2
	call        Materialize

	// *** Basic block 48

	mv          a1, a0
	li          t0, 176		// 0xb0 ASCII \xb0
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 49

	mv          a1, a0
	mv          a0, s1
	call        SetLoweredNode

	// *** Basic block 50

	mv          a1, a0
	mv          a0, s2
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
	j           Emit

	// *** Basic block 52

.LowerComparison_label_222:
	mv          a1, s8
	li          t0, 176		// 0xb0 ASCII \xb0
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 53

	mv          a1, a0
	mv          a0, s1
	call        SetLoweredNode

	// *** Basic block 54

	mv          a1, a0
	mv          a0, s2
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
	j           Emit

	// *** Basic block 56

.LowerComparison_label_243:

	// *** Basic block 57

.LowerComparison_label_244:
	mv          a3, s4
	mv          a2, s3
	mv          a1, s1
	mv          a0, s2
	call        CompareLessThanInt

	// *** Basic block 58

	mv          a1, a0
	mv          a0, s1
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
	j           SetLoweredNode

	// *** Basic block 60

.LowerComparison_label_263:

	// *** Basic block 61

.LowerComparison_label_264:
	mv          a3, s3
	mv          a2, s4
	mv          a1, s1
	mv          a0, s2
	call        CompareLessThanInt

	// *** Basic block 62

	mv          s9, a0
	mv          a1, s9
	li          t0, 170		// 0xaa ASCII \xaa
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 63

	mv          a1, a0
	mv          a0, s1
	call        SetLoweredNode

	// *** Basic block 64

	mv          a1, a0
	mv          a0, s2
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
	j           Emit

	// *** Basic block 66

.LowerComparison_label_296:

	// *** Basic block 67

.LowerComparison_label_297:
	mv          a3, s3
	mv          a2, s4
	mv          a1, s1
	mv          a0, s2
	call        CompareLessThanInt

	// *** Basic block 68

	mv          a1, a0
	mv          a0, s1
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
	j           SetLoweredNode

	// *** Basic block 70

.LowerComparison_label_316:

	// *** Basic block 71

.LowerComparison_label_317:
	mv          a3, s4
	mv          a2, s3
	mv          a1, s1
	mv          a0, s2
	call        CompareLessThanInt

	// *** Basic block 72

	mv          s3, a0
	mv          a1, s3
	li          t0, 170		// 0xaa ASCII \xaa
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 73

	mv          a1, a0
	mv          a0, s1
	call        SetLoweredNode

	// *** Basic block 74

	mv          a1, a0
	mv          a0, s2
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
	j           Emit

	// *** Basic block 76

.LowerComparison_label_349:
	mv          a1, s3
	mv          a0, s2
	call        Materialize

	// *** Basic block 77

	mv          a1, s4
	mv          a0, s2
	call        Materialize

	// *** Basic block 78

	mv          a2, a0
	mv          a1, a0
	li          t0, 126		// 0x7e ASCII '~'
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 79

	mv          a1, a0
	mv          a0, s1
	call        SetLoweredNode

	// *** Basic block 80

	mv          a1, a0
	mv          a0, s2
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
	j           Emit

	// *** Basic block 82

.LowerComparison_label_382:
	mv          a1, s3
	mv          a0, s2
	call        Materialize

	// *** Basic block 83

	mv          a1, s4
	mv          a0, s2
	call        Materialize

	// *** Basic block 84

	mv          a2, a0
	mv          a1, a0
	li          t0, 126		// 0x7e ASCII '~'
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 85

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 86

	mv          s5, a0
	mv          a1, s5
	li          t0, 170		// 0xaa ASCII \xaa
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 87

	mv          a1, a0
	mv          a0, s1
	call        SetLoweredNode

	// *** Basic block 88

	mv          a1, a0
	mv          a0, s2
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
	j           Emit

	// *** Basic block 90

.LowerComparison_label_428:
	mv          a1, s3
	mv          a0, s2
	call        Materialize

	// *** Basic block 91

	mv          a1, s4
	mv          a0, s2
	call        Materialize

	// *** Basic block 92

	mv          a2, a0
	mv          a1, a0
	li          t0, 127		// 0x7f ASCII \x7f
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 93

	mv          a1, a0
	mv          a0, s1
	call        SetLoweredNode

	// *** Basic block 94

	mv          a1, a0
	mv          a0, s2
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
	j           Emit

	// *** Basic block 96

.LowerComparison_label_461:
	mv          a1, s3
	mv          a0, s2
	call        Materialize

	// *** Basic block 97

	mv          a1, s4
	mv          a0, s2
	call        Materialize

	// *** Basic block 98

	mv          a2, a0
	mv          a1, a0
	li          t0, 128		// 0x80 ASCII \x80
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 99

	mv          a1, a0
	mv          a0, s1
	call        SetLoweredNode

	// *** Basic block 100

	mv          a1, a0
	mv          a0, s2
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
	j           Emit

	// *** Basic block 102

.LowerComparison_label_494:
	mv          a1, s4
	mv          a0, s2
	call        Materialize

	// *** Basic block 103

	mv          a1, s3
	mv          a0, s2
	call        Materialize

	// *** Basic block 104

	mv          a2, a0
	mv          a1, a0
	li          t0, 127		// 0x7f ASCII \x7f
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 105

	mv          a1, a0
	mv          a0, s1
	call        SetLoweredNode

	// *** Basic block 106

	mv          a1, a0
	mv          a0, s2
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
	j           Emit

	// *** Basic block 108

.LowerComparison_label_527:
	mv          a1, s4
	mv          a0, s2
	call        Materialize

	// *** Basic block 109

	mv          a1, s3
	mv          a0, s2
	call        Materialize

	// *** Basic block 110

	mv          a2, a0
	mv          a1, a0
	li          t0, 128		// 0x80 ASCII \x80
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 111

	mv          a1, a0
	mv          a0, s1
	call        SetLoweredNode

	// *** Basic block 112

	mv          a1, a0
	mv          a0, s2
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
	j           Emit

	// *** Basic block 114

.LowerComparison_label_560:
	mv          a1, s3
	mv          a0, s2
	call        Materialize

	// *** Basic block 115

	mv          a1, s4
	mv          a0, s2
	call        Materialize

	// *** Basic block 116

	mv          a2, a0
	mv          a1, a0
	li          t0, 155		// 0x9b ASCII \x9b
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 117

	mv          a1, a0
	mv          a0, s1
	call        SetLoweredNode

	// *** Basic block 118

	mv          a1, a0
	mv          a0, s2
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
	j           Emit

	// *** Basic block 120

.LowerComparison_label_593:
	mv          a1, s3
	mv          a0, s2
	call        Materialize

	// *** Basic block 121

	mv          a1, s4
	mv          a0, s2
	call        Materialize

	// *** Basic block 122

	mv          a2, a0
	mv          a1, a0
	li          t0, 155		// 0x9b ASCII \x9b
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 123

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 124

	mv          s6, a0
	mv          a1, s6
	li          t0, 170		// 0xaa ASCII \xaa
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 125

	mv          a1, a0
	mv          a0, s1
	call        SetLoweredNode

	// *** Basic block 126

	mv          a1, a0
	mv          a0, s2
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
	j           Emit

	// *** Basic block 128

.LowerComparison_label_639:
	mv          a1, s3
	mv          a0, s2
	call        Materialize

	// *** Basic block 129

	mv          a1, s4
	mv          a0, s2
	call        Materialize

	// *** Basic block 130

	mv          a2, a0
	mv          a1, a0
	li          t0, 156		// 0x9c ASCII \x9c
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 131

	mv          a1, a0
	mv          a0, s1
	call        SetLoweredNode

	// *** Basic block 132

	mv          a1, a0
	mv          a0, s2
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
	j           Emit

	// *** Basic block 134

.LowerComparison_label_672:
	mv          a1, s3
	mv          a0, s2
	call        Materialize

	// *** Basic block 135

	mv          a1, s4
	mv          a0, s2
	call        Materialize

	// *** Basic block 136

	mv          a2, a0
	mv          a1, a0
	li          t0, 157		// 0x9d ASCII \x9d
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 137

	mv          a1, a0
	mv          a0, s1
	call        SetLoweredNode

	// *** Basic block 138

	mv          a1, a0
	mv          a0, s2
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
	j           Emit

	// *** Basic block 140

.LowerComparison_label_705:
	mv          a1, s4
	mv          a0, s2
	call        Materialize

	// *** Basic block 141

	mv          a1, s3
	mv          a0, s2
	call        Materialize

	// *** Basic block 142

	mv          a2, a0
	mv          a1, a0
	li          t0, 156		// 0x9c ASCII \x9c
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 143

	mv          a1, a0
	mv          a0, s1
	call        SetLoweredNode

	// *** Basic block 144

	mv          a1, a0
	mv          a0, s2
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
	j           Emit

	// *** Basic block 146

.LowerComparison_label_738:
	mv          a1, s4
	mv          a0, s2
	call        Materialize

	// *** Basic block 147

	mv          a1, s3
	mv          a0, s2
	call        Materialize

	// *** Basic block 148

	mv          a2, a0
	mv          a1, a0
	li          t0, 157		// 0x9d ASCII \x9d
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 149

	mv          a1, a0
	mv          a0, s1
	call        SetLoweredNode

	// *** Basic block 150

	mv          a1, a0
	mv          a0, s2
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
	j           Emit

	// *** Basic block 152

.LowerComparison_label_771:
	lla         a0, .str.222
	lla         a1, .str.223
	lla         a3, .str.224
	li          t0, 2033		// 0x7f1
	mv          a2, t0
	call        printf

	// *** Basic block 153

	call        abort

	// *** Basic block 154

.LowerComparison_label_786:
	mv          a0, x0
	j           .LowerComparison_label_157
.func_end_LowerComparison:
	.size LowerComparison, .func_end_LowerComparison-LowerComparison

	.local  GetAddressAndOffsetFrom
	.type GetAddressAndOffsetFrom, @function

GetAddressAndOffsetFrom:

	// *** Basic block 0

	.global RVIsPossibleImmediate
	.local GetIntConstant
	.local PagedOffsetFrom
	addi sp, sp, -80
	// Saved return address (offset 72) and frame pointer (offset 64)
	sd ra, 72(sp)
	sd s0, 64(sp)
	addi s0, sp, 80
	// Local vars at offset -32(s0)
	// Saved integer registers.
	sd s1, 40(sp)
	sd s2, 32(sp)
	sd s3, 24(sp)
	sd s4, 16(sp)
	sd s5, 8(sp)
	sd s6, 0(sp)
	// End of stack frame
	mv          s1, a2
	mv          s2, a3
	mv          s3, a1
	mv          s4, a4
	mv          s5, a0
	mv          a0, s1
	call        RVIsPossibleImmediate

	// *** Basic block 1

	beqz        a0, .GetAddressAndOffsetFrom_label_42

	// *** Basic block 2

	sd          s3, 0(s2)
	mv          a3, s1
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	mv          a1, x0
	mv          a0, s5
	call        GetIntConstant

	// *** Basic block 3

	sd          a0, 0(s4)

	// *** Basic block 4

.GetAddressAndOffsetFrom_label_39:
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

	// *** Basic block 5

.GetAddressAndOffsetFrom_label_42:
	addi        a3, s0, -32
	mv          a2, s1
	mv          a1, s3
	mv          a0, s5
	call        PagedOffsetFrom

	// *** Basic block 6

	mv          s6, a0
	sd          s6, 0(s2)
	lw          a3, -32(s0)
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	mv          a1, x0
	mv          a0, s5
	call        GetIntConstant

	// *** Basic block 7

	sd          a0, 0(s4)
	j           .GetAddressAndOffsetFrom_label_39
.func_end_GetAddressAndOffsetFrom:
	.size GetAddressAndOffsetFrom, .func_end_GetAddressAndOffsetFrom-GetAddressAndOffsetFrom

	.local  GetRegAndOffset
	.type GetRegAndOffset, @function

GetRegAndOffset:

	// *** Basic block 0

	.global IRIsAutoVariable
	.local GetLoweredNode
	.local Zero
	.local IntVariableRegister
	.global TypeIsFloatingPoint
	.local FloatingPointVariableRegister
	.local GetAddressAndOffsetFrom
	.local FramePointer
	.local LocalVariableOffset
	.global IRIsArgument
	.global IRIsStaticVariable
	.local LoadStaticVariableAddress
	.global printf
	.global abort
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
	mv          s2, a2
	mv          s3, a3
	mv          s4, a0
	mv          a0, s1
	call        IRIsAutoVariable

	// *** Basic block 1

	beqz        a0, .GetRegAndOffset_label_194

	// *** Basic block 2

	mv          s5, s1
	ld          s6, 136(s5)
	ld          s7, 40(s6)
	lw          s9, 16(s7)
	addi        t0, s9, -2
	seqz        s8, t0
	li          t0, 2		// 0x2 ASCII \x2
	beq         s9, t0, .GetRegAndOffset_label_73

	// *** Basic block 3

	addi        t0, s9, -1
	seqz        s8, t0

	// *** Basic block 4

.GetRegAndOffset_label_73:
	beqz        s8, .GetRegAndOffset_label_80

	// *** Basic block 5

	addi        t0, s7, 32
	lb          t0, 16(t0)
	slli        t0, t0, 61
	srai        s8, t0, 63

	// *** Basic block 6

.GetRegAndOffset_label_80:

	// *** Basic block 7

.GetRegAndOffset_label_82:
	beqz        s8, .GetRegAndOffset_label_102

	// *** Basic block 8

	j           .GetRegAndOffset_label_85

	// *** Basic block 9

.GetRegAndOffset_label_85:
	ld          s5, 112(s6)
	mv          a0, s5
	call        GetLoweredNode

	// *** Basic block 10

	sd          a0, 0(s2)
	mv          a0, s4
	call        Zero

	// *** Basic block 11

	sd          a0, 0(s3)
	mv          a0, x0

	// *** Basic block 12

.GetRegAndOffset_label_99:
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

	// *** Basic block 13

.GetRegAndOffset_label_102:
	lw          t0, 88(s1)
	andi        t0, t0, 32
	beqz        t0, .GetRegAndOffset_label_129

	// *** Basic block 14

	addi        t0, s1, 96
	lw          t0, 8(t0)
	li          t1, -2147483649		// 0xffffffff7fffffff
	and         s5, t0, t1
	mv          a2, s6
	mv          a1, s5
	mv          a0, s4
	call        IntVariableRegister

	// *** Basic block 15

	sd          a0, 0(s2)
	mv          a0, s4
	call        Zero

	// *** Basic block 16

	sd          a0, 0(s3)
	li          a0, 1		// 0x1 ASCII \x1
	j           .GetRegAndOffset_label_99

	// *** Basic block 17

.GetRegAndOffset_label_129:
	addi        t0, s1, 96
	lw          s5, 8(t0)
	li          t0, 3221225472		// 0xc0000000
	and         t0, s5, t0
	li          t1, 2147483648		// 0x80000000
	bne         t0, t1, .GetRegAndOffset_label_172

	// *** Basic block 18

	li          t0, -2147483649		// 0xffffffff7fffffff
	and         s7, s5, t0
	ld          a0, 80(s1)
	call        TypeIsFloatingPoint

	// *** Basic block 19

	beqz        a0, .GetRegAndOffset_label_158

	// *** Basic block 20

	mv          a2, s6
	mv          a1, s7
	mv          a0, s4
	call        FloatingPointVariableRegister

	// *** Basic block 21

	sd          a0, 0(s2)
	j           .GetRegAndOffset_label_167

	// *** Basic block 22

.GetRegAndOffset_label_158:
	mv          a2, s6
	mv          a1, s7
	mv          a0, s4
	call        IntVariableRegister

	// *** Basic block 23

	sd          a0, 0(s2)

	// *** Basic block 24

.GetRegAndOffset_label_167:
	sd          x0, 0(s3)
	mv          a0, x0
	j           .GetRegAndOffset_label_99

	// *** Basic block 25

.GetRegAndOffset_label_172:
	mv          a0, s4
	call        FramePointer

	// *** Basic block 26

	mv          a1, s5
	mv          a0, s4
	call        LocalVariableOffset

	// *** Basic block 27

	mv          a4, s3
	mv          a3, s2
	mv          a2, a0
	mv          a1, a0
	mv          a0, s4
	call        GetAddressAndOffsetFrom

	// *** Basic block 28

	j           .GetRegAndOffset_label_309

	// *** Basic block 29

.GetRegAndOffset_label_194:
	mv          a0, s1
	call        IRIsArgument

	// *** Basic block 30

	beqz        a0, .GetRegAndOffset_label_262

	// *** Basic block 31

	addi        t0, s1, 96
	lw          s6, 8(t0)
	mv          s7, s1
	li          t0, 3221225472		// 0xc0000000
	and         t0, s6, t0
	li          t1, 2147483648		// 0x80000000
	bne         t0, t1, .GetRegAndOffset_label_245

	// *** Basic block 32

	li          t0, -2147483649		// 0xffffffff7fffffff
	and         s8, s6, t0
	ld          a0, 80(s1)
	call        TypeIsFloatingPoint

	// *** Basic block 33

	beqz        a0, .GetRegAndOffset_label_230

	// *** Basic block 34

	ld          a2, 136(s7)
	mv          a1, s8
	mv          a0, s4
	call        FloatingPointVariableRegister

	// *** Basic block 35

	sd          a0, 0(s2)
	j           .GetRegAndOffset_label_240

	// *** Basic block 36

.GetRegAndOffset_label_230:
	ld          a2, 136(s7)
	mv          a1, s8
	mv          a0, s4
	call        IntVariableRegister

	// *** Basic block 37

	sd          a0, 0(s2)

	// *** Basic block 38

.GetRegAndOffset_label_240:
	sd          x0, 0(s3)
	mv          a0, x0
	j           .GetRegAndOffset_label_99

	// *** Basic block 39

.GetRegAndOffset_label_245:
	mv          a0, s4
	call        FramePointer

	// *** Basic block 40

	mv          a4, s3
	mv          a3, s2
	mv          a2, s6
	mv          a1, a0
	mv          a0, s4
	call        GetAddressAndOffsetFrom

	// *** Basic block 41

.GetRegAndOffset_label_260:
	j           .GetRegAndOffset_label_308

	// *** Basic block 42

.GetRegAndOffset_label_262:
	mv          a0, s1
	call        IRIsStaticVariable

	// *** Basic block 43

	beqz        a0, .GetRegAndOffset_label_278

	// *** Basic block 44

	mv          a1, s1
	mv          a0, s4
	call        LoadStaticVariableAddress

	// *** Basic block 45

	sd          a0, 0(s2)
	mv          a0, s4
	call        Zero

	// *** Basic block 46

	sd          a0, 0(s3)
	j           .GetRegAndOffset_label_307

	// *** Basic block 47

.GetRegAndOffset_label_278:
	mv          a0, s1
	call        GetLoweredNode

	// *** Basic block 48

	sd          a0, 0(s2)
	mv          a0, s4
	call        Zero

	// *** Basic block 49

	sd          a0, 0(s3)
	beq         s2, x0, .GetRegAndOffset_label_291

	// *** Basic block 50

	j           .GetRegAndOffset_label_306

	// *** Basic block 51

.GetRegAndOffset_label_291:
	lla         a0, .str.225
	lla         a1, .str.226
	lla         a3, .str.227
	li          t0, 2115		// 0x843
	mv          a2, t0
	call        printf

	// *** Basic block 52

	call        abort

	// *** Basic block 53

.GetRegAndOffset_label_306:

	// *** Basic block 54

.GetRegAndOffset_label_307:

	// *** Basic block 55

.GetRegAndOffset_label_308:

	// *** Basic block 56

.GetRegAndOffset_label_309:
	li          a0, 1		// 0x1 ASCII \x1
	j           .GetRegAndOffset_label_99
.func_end_GetRegAndOffset:
	.size GetRegAndOffset, .func_end_GetRegAndOffset-GetRegAndOffset

	.local  LowerLoad
	.type LowerLoad, @function

LowerLoad:

	// *** Basic block 0

	.global printf
	.global abort
	.local GetRegAndOffset
	.local SetLoweredNode
	.global TargetIsZero
	.global TargetIsConst
	.local Emit
	.local NewInstruction2
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
	bne         t0, t1, .LowerLoad_label_66

	// *** Basic block 1

	j           .LowerLoad_label_83

	// *** Basic block 2

.LowerLoad_label_66:
	lla         a0, .str.228
	lla         a1, .str.229
	lla         a3, .str.230
	li          t0, 2122		// 0x84a
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

.LowerLoad_label_83:
	ld          t0, 24(s1)
	ld          s3, 0(t0)
	addi        a2, s0, -32
	addi        a3, s0, -24
	mv          a1, s3
	mv          a0, s2
	call        GetRegAndOffset

	// *** Basic block 5

	mv          s3, a0
	not         t0, s3
	beqz        t0, .LowerLoad_label_113

	// *** Basic block 6

	ld          a0, -32(s0)
	mv          a1, a0
	mv          a0, s1
	call        SetLoweredNode

	// *** Basic block 7

.LowerLoad_label_110:
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

	// *** Basic block 8

.LowerLoad_label_113:
	lw          s3, 20(s1)
	li          t0, 19		// 0x13 ASCII \x13
	blt         s3, t0, .LowerLoad_label_172

	// *** Basic block 9

	li          t0, 28		// 0x1c ASCII \x1c
	blt         t0, s3, .LowerLoad_label_172

	// *** Basic block 10

	addi        t0, s3, -19
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 11

	j           .LowerLoad_label_141

	// *** Basic block 12

	j           .LowerLoad_label_145

	// *** Basic block 13

	j           .LowerLoad_label_148

	// *** Basic block 14

	j           .LowerLoad_label_151

	// *** Basic block 15

	j           .LowerLoad_label_154

	// *** Basic block 16

	j           .LowerLoad_label_157

	// *** Basic block 17

	j           .LowerLoad_label_160

	// *** Basic block 18

	j           .LowerLoad_label_163

	// *** Basic block 19

	j           .LowerLoad_label_166

	// *** Basic block 20

	j           .LowerLoad_label_169

	// *** Basic block 21

.LowerLoad_label_141:
	li          s3, 47		// 0x2f ASCII '/'
	j           .LowerLoad_label_187

	// *** Basic block 22

.LowerLoad_label_145:
	li          s3, 45		// 0x2d ASCII '-'
	j           .LowerLoad_label_187

	// *** Basic block 23

.LowerLoad_label_148:
	li          s3, 83		// 0x53 ASCII 'S'
	j           .LowerLoad_label_187

	// *** Basic block 24

.LowerLoad_label_151:
	li          s3, 46		// 0x2e ASCII '.'
	j           .LowerLoad_label_187

	// *** Basic block 25

.LowerLoad_label_154:
	li          s3, 82		// 0x52 ASCII 'R'
	j           .LowerLoad_label_187

	// *** Basic block 26

.LowerLoad_label_157:
	li          s3, 48		// 0x30 ASCII '0'
	j           .LowerLoad_label_187

	// *** Basic block 27

.LowerLoad_label_160:
	li          s3, 49		// 0x31 ASCII '1'
	j           .LowerLoad_label_187

	// *** Basic block 28

.LowerLoad_label_163:
	li          s3, 107		// 0x6b ASCII 'k'
	j           .LowerLoad_label_187

	// *** Basic block 29

.LowerLoad_label_166:
	li          s3, 137		// 0x89 ASCII \x89
	j           .LowerLoad_label_187

	// *** Basic block 30

.LowerLoad_label_169:
	li          s3, 83		// 0x53 ASCII 'S'
	j           .LowerLoad_label_187

	// *** Basic block 31

.LowerLoad_label_172:
	lla         a0, .str.231
	lla         a1, .str.232
	lla         a3, .str.233
	li          t0, 2165		// 0x875
	mv          a2, t0
	call        printf

	// *** Basic block 32

	call        abort

	// *** Basic block 33

.LowerLoad_label_187:
	mv          s4, x0
	ld          s6, -32(s0)
	lw          t0, 16(s6)
	addi        t1, t0, -53
	seqz        s5, t1
	li          t1, 53		// 0x35 ASCII '5'
	bne         t0, t1, .LowerLoad_label_203

	// *** Basic block 34

	ld          a0, -24(s0)
	call        TargetIsZero

	// *** Basic block 35

	mv          s5, a0

	// *** Basic block 36

.LowerLoad_label_203:
	beqz        s5, .LowerLoad_label_286

	// *** Basic block 37

	addi        t0, s6, 40
	ld          s7, 40(s6)
	ld          s8, 8(t0)
	beq         s7, x0, .LowerLoad_label_215

	// *** Basic block 38

	j           .LowerLoad_label_230

	// *** Basic block 39

.LowerLoad_label_215:
	lla         a0, .str.234
	lla         a1, .str.235
	lla         a3, .str.236
	li          t0, 2176		// 0x880
	mv          a2, t0
	call        printf

	// *** Basic block 40

	call        abort

	// *** Basic block 41

.LowerLoad_label_230:
	beq         s8, x0, .LowerLoad_label_235

	// *** Basic block 42

	j           .LowerLoad_label_250

	// *** Basic block 43

.LowerLoad_label_235:
	lla         a0, .str.237
	lla         a1, .str.238
	lla         a3, .str.239
	li          t0, 2177		// 0x881
	mv          a2, t0
	call        printf

	// *** Basic block 44

	call        abort

	// *** Basic block 45

.LowerLoad_label_250:
	mv          a0, s8
	call        TargetIsConst

	// *** Basic block 46

	beqz        a0, .LowerLoad_label_257

	// *** Basic block 47

	j           .LowerLoad_label_272

	// *** Basic block 48

.LowerLoad_label_257:
	lla         a0, .str.240
	lla         a1, .str.241
	lla         a3, .str.242
	li          t0, 2178		// 0x882
	mv          a2, t0
	call        printf

	// *** Basic block 49

	call        abort

	// *** Basic block 50

.LowerLoad_label_272:
	mv          a2, s8
	mv          a1, s7
	mv          a0, s3
	call        NewInstruction2

	// *** Basic block 51

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 52

	mv          s4, a0

	// *** Basic block 53

.LowerLoad_label_286:
	bne         s4, x0, .LowerLoad_label_303

	// *** Basic block 54

	ld          a2, -24(s0)
	mv          a1, s6
	mv          a0, s3
	call        NewInstruction2

	// *** Basic block 55

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 56

	mv          s4, a0

	// *** Basic block 57

.LowerLoad_label_303:
	mv          a1, s4
	mv          a0, s1
	call        SetLoweredNode

	// *** Basic block 58

	mv          a0, s4
	j           .LowerLoad_label_110
.func_end_LowerLoad:
	.size LowerLoad, .func_end_LowerLoad-LowerLoad

	.local  LowerStore
	.type LowerStore, @function

LowerStore:

	// *** Basic block 0

	.global printf
	.global abort
	.local GetRegAndOffset
	.local Materialize
	.global TypeIsFloatingPoint
	.local SetDestOrMove
	.local SetLoweredNode
	.local Emit
	.local NewInstruction3
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
	ld          t0, 8(t0)
	li          t1, 2		// 0x2 ASCII \x2
	bne         t0, t1, .LowerStore_label_52

	// *** Basic block 1

	j           .LowerStore_label_69

	// *** Basic block 2

.LowerStore_label_52:
	lla         a0, .str.243
	lla         a1, .str.244
	lla         a3, .str.245
	li          t0, 2190		// 0x88e
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

.LowerStore_label_69:
	ld          s3, 24(s1)
	ld          s4, 0(s3)
	addi        a2, s0, -32
	addi        a3, s0, -24
	mv          a1, s4
	mv          a0, s2
	call        GetRegAndOffset

	// *** Basic block 5

	mv          s5, a0
	ld          s6, 8(s3)
	not         t0, s5
	beqz        t0, .LowerStore_label_133

	// *** Basic block 6

	mv          a1, s6
	mv          a0, s2
	call        Materialize

	// *** Basic block 7

	mv          s3, a0
	ld          a0, 80(s4)
	call        TypeIsFloatingPoint

	// *** Basic block 8

	beqz        a0, .LowerStore_label_108

	// *** Basic block 9

	li          s5, 19		// 0x13 ASCII \x13
	j           .LowerStore_label_110

	// *** Basic block 10

.LowerStore_label_108:
	li          s5, 18		// 0x12 ASCII \x12

	// *** Basic block 11

.LowerStore_label_110:
	ld          a2, -32(s0)
	mv          a3, s5
	mv          a1, s3
	mv          a0, s2
	call        SetDestOrMove

	// *** Basic block 12

	mv          s4, a0
	mv          a1, s4
	mv          a0, s1
	call        SetLoweredNode

	// *** Basic block 13

	mv          a0, s4

	// *** Basic block 14

.LowerStore_label_130:
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

	// *** Basic block 15

.LowerStore_label_133:
	lw          s5, 20(s1)
	li          t0, 30		// 0x1e ASCII \x1e
	blt         s5, t0, .LowerStore_label_180

	// *** Basic block 16

	li          t0, 36		// 0x24 ASCII '$'
	blt         t0, s5, .LowerStore_label_180

	// *** Basic block 17

	addi        t0, s5, -30
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 18

	j           .LowerStore_label_158

	// *** Basic block 19

	j           .LowerStore_label_162

	// *** Basic block 20

	j           .LowerStore_label_168

	// *** Basic block 21

	j           .LowerStore_label_165

	// *** Basic block 22

	j           .LowerStore_label_171

	// *** Basic block 23

	j           .LowerStore_label_174

	// *** Basic block 24

	j           .LowerStore_label_177

	// *** Basic block 25

.LowerStore_label_158:
	li          s5, 52		// 0x34 ASCII '4'
	j           .LowerStore_label_195

	// *** Basic block 26

.LowerStore_label_162:
	li          s5, 50		// 0x32 ASCII '2'
	j           .LowerStore_label_195

	// *** Basic block 27

.LowerStore_label_165:
	li          s5, 84		// 0x54 ASCII 'T'
	j           .LowerStore_label_195

	// *** Basic block 28

.LowerStore_label_168:
	li          s5, 51		// 0x33 ASCII '3'
	j           .LowerStore_label_195

	// *** Basic block 29

.LowerStore_label_171:
	li          s5, 108		// 0x6c ASCII 'l'
	j           .LowerStore_label_195

	// *** Basic block 30

.LowerStore_label_174:
	li          s5, 138		// 0x8a ASCII \x8a
	j           .LowerStore_label_195

	// *** Basic block 31

.LowerStore_label_177:
	li          s5, 84		// 0x54 ASCII 'T'
	j           .LowerStore_label_195

	// *** Basic block 32

.LowerStore_label_180:
	lla         a0, .str.246
	lla         a1, .str.247
	lla         a3, .str.248
	li          t0, 2238		// 0x8be
	mv          a2, t0
	call        printf

	// *** Basic block 33

	call        abort

	// *** Basic block 34

.LowerStore_label_195:
	mv          a1, s6
	mv          a0, s2
	call        Materialize

	// *** Basic block 35

	mv          s6, a0
	mv          s7, x0
	ld          a2, -32(s0)
	ld          a3, -24(s0)
	mv          a1, s6
	mv          a0, s5
	call        NewInstruction3

	// *** Basic block 36

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 37

	mv          s7, a0
	mv          a1, s7
	mv          a0, s1
	call        SetLoweredNode

	// *** Basic block 38

	mv          a0, s7
	j           .LowerStore_label_130
.func_end_LowerStore:
	.size LowerStore, .func_end_LowerStore-LowerStore

	.local  LowerConditionalBranch
	.type LowerConditionalBranch, @function

LowerConditionalBranch:

	// *** Basic block 0

	.global printf
	.global abort
	.local branch_compare_ops
	.global IRIsZero
	.local Emit
	.local NewInstruction1
	.local Materialize
	.global VectorAppend
	.global NewBranchFixup
	.local NewInstruction2
	.global OptLevel1
	.global IRIsConst
	.local NewInstruction
	addi sp, sp, -128
	// Saved return address (offset 120) and frame pointer (offset 112)
	sd ra, 120(sp)
	sd s0, 112(sp)
	addi s0, sp, 128
	// Local vars at offset -16(s0)
	// Spilled register region: 16 bytes at -32(s0) to -16(s0)
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
	sd          s2, -24(s0)	// Spilled @49
	addi        t0, s1, 24
	ld          t0, 8(t0)
	li          s3, 2		// 0x2 ASCII \x2
	bne         t0, s3, .LowerConditionalBranch_label_62

	// *** Basic block 1

	j           .LowerConditionalBranch_label_79

	// *** Basic block 2

.LowerConditionalBranch_label_62:
	lla         a0, .str.249
	lla         a1, .str.250
	lla         a3, .str.251
	li          t0, 2290		// 0x8f2
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

.LowerConditionalBranch_label_79:
	ld          t0, 24(s1)
	ld          s4, 0(t0)
	ld          s5, 8(t0)
	lw          s6, 20(s1)
	addi        t0, s6, -88
	seqz        s7, t0
	mv          s8, x0
	mv          s9, x0
	lw          t0, 20(s4)

	// *** Basic block 5

.LowerConditionalBranch_label_99:
	slli        s1, s9, 4
	la          t2, branch_compare_ops
	add         s10, t2, s1
	lw          t2, 0(s10)
	sub         t3, t2, t0
	seqz        t1, t3
	bne         t2, t0, .LowerConditionalBranch_label_112

	// *** Basic block 6

	lb          t0, 4(s10)
	sub         t0, t0, s7
	seqz        t1, t0

	// *** Basic block 7

.LowerConditionalBranch_label_112:
	beqz        t1, .LowerConditionalBranch_label_117

	// *** Basic block 8

	la          t0, branch_compare_ops
	add         s8, t0, s1
	j           .LowerConditionalBranch_label_123

	// *** Basic block 9

.LowerConditionalBranch_label_117:

	// *** Basic block 10

.LowerConditionalBranch_label_118:
	addi        s9, s9, 1
	li          t0, 24		// 0x18 ASCII \x18
	bge         s9, t0, .LowerConditionalBranch_label_99

	// *** Basic block 11

.LowerConditionalBranch_label_123:
	beq         s8, x0, .LowerConditionalBranch_label_295

	// *** Basic block 12

	ld          t0, 24(s4)
	ld          s1, 0(t0)
	ld          s7, 8(t0)
	mv          a0, s1
	sd          s1, -24(s0)	// Spilled @130
	call        IRIsZero

	// *** Basic block 13

	mv          s10, a0
	bnez        a0, .LowerConditionalBranch_label_145

	// *** Basic block 14

	mv          a0, s7
	call        IRIsZero

	// *** Basic block 15

	mv          s10, a0

	// *** Basic block 16

.LowerConditionalBranch_label_145:
	beqz        s10, .LowerConditionalBranch_label_159

	// *** Basic block 17

	lw          t0, 8(s8)
	addi        t1, t0, -39
	seqz        s10, t1
	li          t1, 39		// 0x27 ASCII '''
	beq         t0, t1, .LowerConditionalBranch_label_158

	// *** Basic block 18

	addi        t0, t0, -40
	seqz        s10, t0

	// *** Basic block 19

.LowerConditionalBranch_label_158:

	// *** Basic block 20

.LowerConditionalBranch_label_159:
	beqz        s10, .LowerConditionalBranch_label_231

	// *** Basic block 21

	lw          t0, 8(s8)
	li          t1, 39		// 0x27 ASCII '''
	bne         t0, t1, .LowerConditionalBranch_label_170

	// *** Basic block 22

	li          s11, 179		// 0xb3 ASCII \xb3
	j           .LowerConditionalBranch_label_172

	// *** Basic block 23

.LowerConditionalBranch_label_170:
	li          s11, 180		// 0xb4 ASCII \xb4

	// *** Basic block 24

.LowerConditionalBranch_label_172:
	mv          a0, s1
	call        IRIsZero

	// *** Basic block 25

	beqz        a0, .LowerConditionalBranch_label_182

	// *** Basic block 26

	sd          s1, -32(s0)	// Spilled @178
	mv          s1, s7
	mv          s7, s1

	// *** Basic block 27

.LowerConditionalBranch_label_182:
	ld          a1, -24(s0)	// Spilled @130
	mv          a0, s2
	call        Materialize

	// *** Basic block 28

	mv          a1, a0
	mv          a0, s11
	call        NewInstruction1

	// *** Basic block 29

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 30

	mv          s11, a0
	ld          s2, 96(s5)
	bne         s2, x0, .LowerConditionalBranch_label_221

	// *** Basic block 31

	ld          t0, -24(s0)	// Spilled @49
	addi        s5, t0, 136
	li          t1, 1		// 0x1 ASCII \x1
	mv          a2, t1
	ld          a1, -24(s0)	// Spilled @85
	sd          s5, -24(s0)	// Spilled @85
	mv          a0, s11
	call        NewBranchFixup

	// *** Basic block 32

	mv          a1, a0
	mv          a0, s5
	call        VectorAppend

	// *** Basic block 33

	j           .LowerConditionalBranch_label_225

	// *** Basic block 34

.LowerConditionalBranch_label_221:
	addi        t0, s11, 40
	sd          s2, 8(t0)

	// *** Basic block 35

.LowerConditionalBranch_label_225:
	mv          a0, s11

	// *** Basic block 36

.LowerConditionalBranch_label_228:
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

	// *** Basic block 37

.LowerConditionalBranch_label_231:
	lb          t0, 12(s8)
	beqz        t0, .LowerConditionalBranch_label_239

	// *** Basic block 38

	ld          s2, -24(s0)	// Spilled @130
	mv          s1, s7
	mv          s7, s2

	// *** Basic block 39

.LowerConditionalBranch_label_239:
	lw          s5, 8(s8)
	ld          a1, -24(s0)	// Spilled @130
	ld          a0, -24(s0)	// Spilled @49
	call        Materialize

	// *** Basic block 40

	mv          a1, s7
	ld          a0, -24(s0)	// Spilled @49
	call        Materialize

	// *** Basic block 41

	mv          a2, a0
	mv          a1, a0
	mv          a0, s5
	call        NewInstruction2

	// *** Basic block 42

	mv          a1, a0
	ld          a0, -24(s0)	// Spilled @49
	call        Emit

	// *** Basic block 43

	mv          s5, a0
	sd          s5, -24(s0)	// Spilled @264
	ld          t0, -24(s0)	// Spilled @85
	ld          s5, 96(t0)
	bne         s5, x0, .LowerConditionalBranch_label_287

	// *** Basic block 44

	ld          t0, -24(s0)	// Spilled @49
	addi        s1, t0, 136
	mv          a2, s3
	ld          a1, -24(s0)	// Spilled @85
	ld          a0, -24(s0)	// Spilled @264
	call        NewBranchFixup

	// *** Basic block 45

	mv          a1, a0
	mv          a0, s1
	call        VectorAppend

	// *** Basic block 46

	j           .LowerConditionalBranch_label_291

	// *** Basic block 47

.LowerConditionalBranch_label_287:
	ld          t0, -24(s0)	// Spilled @264
	addi        t1, t0, 40
	sd          s5, 16(t1)

	// *** Basic block 48

.LowerConditionalBranch_label_291:
	ld          a0, -24(s0)	// Spilled @264
	j           .LowerConditionalBranch_label_228

	// *** Basic block 49

.LowerConditionalBranch_label_295:
	li          t0, 88		// 0x58 ASCII 'X'
	blt         s6, t0, .LowerConditionalBranch_label_317

	// *** Basic block 50

	li          t0, 89		// 0x59 ASCII 'Y'
	blt         t0, s6, .LowerConditionalBranch_label_317

	// *** Basic block 51

	addi        t0, s6, -88
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 52

	j           .LowerConditionalBranch_label_310

	// *** Basic block 53

	j           .LowerConditionalBranch_label_314

	// *** Basic block 54

.LowerConditionalBranch_label_310:
	li          s1, 180		// 0xb4 ASCII \xb4
	j           .LowerConditionalBranch_label_332

	// *** Basic block 55

.LowerConditionalBranch_label_314:
	li          s1, 179		// 0xb3 ASCII \xb3
	j           .LowerConditionalBranch_label_332

	// *** Basic block 56

.LowerConditionalBranch_label_317:
	lla         a0, .str.252
	lla         a1, .str.253
	lla         a3, .str.254
	li          t0, 2375		// 0x947
	mv          a2, t0
	call        printf

	// *** Basic block 57

	call        abort

	// *** Basic block 58

.LowerConditionalBranch_label_332:
	li          s3, 1		// 0x1 ASCII \x1
	call        OptLevel1

	// *** Basic block 59

	mv          s5, a0
	beqz        a0, .LowerConditionalBranch_label_343

	// *** Basic block 60

	mv          a0, s4
	call        IRIsConst

	// *** Basic block 61

	mv          s5, a0

	// *** Basic block 62

.LowerConditionalBranch_label_343:
	beqz        s5, .LowerConditionalBranch_label_415

	// *** Basic block 63

	ld          s6, 136(s4)
	bnez        s6, .LowerConditionalBranch_label_382

	// *** Basic block 64

	li          t0, 180		// 0xb4 ASCII \xb4
	bne         s1, t0, .LowerConditionalBranch_label_368

	// *** Basic block 65

	li          t0, 169		// 0xa9 ASCII \xa9
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 66

	mv          a1, a0
	ld          a0, -24(s0)	// Spilled @49
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

	// *** Basic block 68

.LowerConditionalBranch_label_368:
	li          t0, 181		// 0xb5 ASCII \xb5
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 69

	mv          a1, a0
	ld          a0, -24(s0)	// Spilled @49
	call        Emit

	// *** Basic block 70

	mv          s6, a0
	mv          s3, x0
	j           .LowerConditionalBranch_label_413

	// *** Basic block 71

.LowerConditionalBranch_label_382:
	li          t0, 179		// 0xb3 ASCII \xb3
	bne         s1, t0, .LowerConditionalBranch_label_401

	// *** Basic block 72

	li          t0, 169		// 0xa9 ASCII \xa9
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 73

	mv          a1, a0
	ld          a0, -24(s0)	// Spilled @49
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

	// *** Basic block 75

.LowerConditionalBranch_label_401:
	li          t0, 181		// 0xb5 ASCII \xb5
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 76

	mv          a1, a0
	ld          a0, -24(s0)	// Spilled @49
	call        Emit

	// *** Basic block 77

	mv          s6, a0
	mv          s3, x0

	// *** Basic block 78

.LowerConditionalBranch_label_413:
	j           .LowerConditionalBranch_label_432

	// *** Basic block 79

.LowerConditionalBranch_label_415:
	mv          a1, s4
	ld          a0, -24(s0)	// Spilled @49
	call        Materialize

	// *** Basic block 80

	mv          a1, a0
	mv          a0, s1
	call        NewInstruction1

	// *** Basic block 81

	mv          a1, a0
	ld          a0, -24(s0)	// Spilled @49
	call        Emit

	// *** Basic block 82

	mv          s6, a0

	// *** Basic block 83

.LowerConditionalBranch_label_432:
	ld          t0, -24(s0)	// Spilled @85
	ld          s1, 96(t0)
	bne         s1, x0, .LowerConditionalBranch_label_453

	// *** Basic block 84

	ld          t0, -24(s0)	// Spilled @49
	addi        s4, t0, 136
	mv          a2, s3
	ld          a1, -24(s0)	// Spilled @85
	mv          a0, s6
	call        NewBranchFixup

	// *** Basic block 85

	mv          a1, a0
	mv          a0, s4
	call        VectorAppend

	// *** Basic block 86

	j           .LowerConditionalBranch_label_459

	// *** Basic block 87

.LowerConditionalBranch_label_453:
	addi        t0, s6, 40
	slli        t1, s3, 3
	add         t0, t0, t1
	sd          s1, 0(t0)

	// *** Basic block 88

.LowerConditionalBranch_label_459:
	mv          a0, s6
	j           .LowerConditionalBranch_label_228
.func_end_LowerConditionalBranch:
	.size LowerConditionalBranch, .func_end_LowerConditionalBranch-LowerConditionalBranch

	.local  LowerBranch
	.type LowerBranch, @function

LowerBranch:

	// *** Basic block 0

	.global printf
	.global abort
	.global OptLevel1
	.local Emit
	.local NewInstruction
	.global VectorAppend
	.global NewBranchFixup
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
	li          t1, 1		// 0x1 ASCII \x1
	bne         t0, t1, .LowerBranch_label_42

	// *** Basic block 1

	j           .LowerBranch_label_59

	// *** Basic block 2

.LowerBranch_label_42:
	lla         a0, .str.255
	lla         a1, .str.256
	lla         a3, .str.257
	li          t0, 2418		// 0x972
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

.LowerBranch_label_59:
	ld          t0, 24(s1)
	ld          s3, 0(t0)
	lw          t0, 44(s2)
	seqz        s4, t0
	bnez        t0, .LowerBranch_label_72

	// *** Basic block 5

	call        OptLevel1

	// *** Basic block 6

	mv          s4, a0

	// *** Basic block 7

.LowerBranch_label_72:
	beqz        s4, .LowerBranch_label_77

	// *** Basic block 8

	lb          t0, 204(s2)
	not         s4, t0

	// *** Basic block 9

.LowerBranch_label_77:
	beqz        s4, .LowerBranch_label_82

	// *** Basic block 10

	lw          t0, 128(s2)
	seqz        s4, t0

	// *** Basic block 11

.LowerBranch_label_82:
	mv          t0, s4
	beqz        s4, .LowerBranch_label_91

	// *** Basic block 12

	lw          t1, 88(s1)
	andi        t1, t1, 8
	snez        t0, t1

	// *** Basic block 13

.LowerBranch_label_91:
	beqz        t0, .LowerBranch_label_108

	// *** Basic block 14

	li          t0, 21		// 0x15 ASCII \x15
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 15

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

	// *** Basic block 17

.LowerBranch_label_105:
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

	// *** Basic block 18

.LowerBranch_label_108:
	li          t0, 181		// 0xb5 ASCII \xb5
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 19

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 20

	mv          s5, a0
	ld          s6, 96(s3)
	bne         s6, x0, .LowerBranch_label_140

	// *** Basic block 21

	addi        s7, s2, 136
	mv          a2, x0
	mv          a1, s3
	mv          a0, s5
	call        NewBranchFixup

	// *** Basic block 22

	mv          a1, a0
	mv          a0, s7
	call        VectorAppend

	// *** Basic block 23

	j           .LowerBranch_label_143

	// *** Basic block 24

.LowerBranch_label_140:
	sd          s6, 40(s5)

	// *** Basic block 25

.LowerBranch_label_143:
	mv          a0, s5
	j           .LowerBranch_label_105
.func_end_LowerBranch:
	.size LowerBranch, .func_end_LowerBranch-LowerBranch

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

	.local  LowerResult
	.type LowerResult, @function

LowerResult:

	// *** Basic block 0

	.global printf
	.global abort
	.local Materialize
	.local EmitSymbol
	.local NewInstruction
	.local SetLoweredNode
	.local SetDestOrMove
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
	bne         t0, t1, .LowerResult_label_45

	// *** Basic block 1

	j           .LowerResult_label_62

	// *** Basic block 2

.LowerResult_label_45:
	lla         a0, .str.258
	lla         a1, .str.259
	lla         a3, .str.260
	li          t0, 2459		// 0x99b
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

.LowerResult_label_62:
	lw          s3, 20(s1)
	li          t0, 106		// 0x6a ASCII 'j'
	blt         s3, t0, .LowerResult_label_99

	// *** Basic block 5

	li          t0, 109		// 0x6d ASCII 'm'
	blt         t0, s3, .LowerResult_label_99

	// *** Basic block 6

	addi        t0, s3, -106
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 7

	j           .LowerResult_label_84

	// *** Basic block 8

	j           .LowerResult_label_91

	// *** Basic block 9

	j           .LowerResult_label_95

	// *** Basic block 10

	j           .LowerResult_label_85

	// *** Basic block 11

.LowerResult_label_84:

	// *** Basic block 12

.LowerResult_label_85:
	li          s3, 26		// 0x1a ASCII \x1a
	li          s4, 18		// 0x12 ASCII \x12
	j           .LowerResult_label_114

	// *** Basic block 13

.LowerResult_label_91:
	li          s3, 27		// 0x1b ASCII \x1b
	li          s4, 19		// 0x13 ASCII \x13
	j           .LowerResult_label_114

	// *** Basic block 14

.LowerResult_label_95:
	li          s3, 28		// 0x1c ASCII \x1c
	li          s4, 20		// 0x14 ASCII \x14
	j           .LowerResult_label_114

	// *** Basic block 15

.LowerResult_label_99:
	lla         a0, .str.261
	lla         a1, .str.262
	lla         a3, .str.263
	li          t0, 2476		// 0x9ac
	mv          a2, t0
	call        printf

	// *** Basic block 16

	call        abort

	// *** Basic block 17

.LowerResult_label_114:
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
	call        EmitSymbol

	// *** Basic block 20

	mv          s3, a0
	mv          a3, s4
	mv          a2, s3
	mv          a1, s5
	mv          a0, s2
	call        SetDestOrMove

	// *** Basic block 21

	mv          a1, a0
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
.func_end_LowerResult:
	.size LowerResult, .func_end_LowerResult-LowerResult

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
	li          t0, 188		// 0xbc ASCII \xbc
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

	.local Materialize
	.local SetLoweredNode
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
	mv          s1, a1
	ld          t0, 24(s1)
	ld          a1, 0(t0)
	call        Materialize

	// *** Basic block 1

	mv          s2, a0
	mv          a1, s2
	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           SetLoweredNode
.func_end_LowerAddressOf:
	.size LowerAddressOf, .func_end_LowerAddressOf-LowerAddressOf

	.local  LowerZeroExtend
	.type LowerZeroExtend, @function

LowerZeroExtend:

	// *** Basic block 0

	.local Materialize
	.global RVIsPossibleImmediate
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
	mv          s1, a0
	mv          s2, a1
	ld          s3, 24(s2)
	ld          a1, 0(s3)
	call        Materialize

	// *** Basic block 1

	mv          s4, a0
	ld          s5, 8(s3)
	ld          s3, 136(s5)
	mv          a0, s3
	call        RVIsPossibleImmediate

	// *** Basic block 2

	beqz        a0, .LowerZeroExtend_label_69

	// *** Basic block 3

	mv          a3, s3
	li          t0, 3		// 0x3 ASCII \x3
	mv          a2, t0
	mv          a1, x0
	mv          a0, s1
	call        GetIntConstant

	// *** Basic block 4

	mv          a2, a0
	mv          a1, s4
	li          t0, 58		// 0x3a ASCII ':'
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 5

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 6

	mv          s4, a0
	j           .LowerZeroExtend_label_89

	// *** Basic block 7

.LowerZeroExtend_label_69:
	mv          a1, s5
	mv          a0, s1
	call        Materialize

	// *** Basic block 8

	mv          a2, a0
	mv          a1, s4
	li          t0, 71		// 0x47 ASCII 'G'
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 9

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 10

	mv          s4, a0

	// *** Basic block 11

.LowerZeroExtend_label_89:
	mv          a1, s4
	mv          a0, s2
	call        SetLoweredNode

	// *** Basic block 12

	mv          a0, s4

	// *** Basic block 13

.LowerZeroExtend_label_97:
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
.func_end_LowerZeroExtend:
	.size LowerZeroExtend, .func_end_LowerZeroExtend-LowerZeroExtend

	.local  LowerSignExtend
	.type LowerSignExtend, @function

LowerSignExtend:

	// *** Basic block 0

	.local Materialize
	.global RVIsSignedLoad
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
	call        RVIsSignedLoad

	// *** Basic block 2

	beqz        a0, .LowerSignExtend_label_52

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

.LowerSignExtend_label_49:
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

.LowerSignExtend_label_52:
	ld          s5, 8(s3)
	ld          s3, 136(s5)
	li          t0, 32		// 0x20 ASCII ' '
	bne         s3, t0, .LowerSignExtend_label_84

	// *** Basic block 7

	mv          a1, s4
	li          t0, 189		// 0xbd ASCII \xbd
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 8

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 9

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
	j           SetLoweredNode

	// *** Basic block 11

.LowerSignExtend_label_84:
	mv          a3, s3
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	mv          a1, x0
	mv          a0, s1
	call        GetIntConstant

	// *** Basic block 12

	mv          s3, a0
	mv          a2, s3
	mv          a1, s4
	li          t0, 59		// 0x3b ASCII ';'
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 13

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 14

	mv          s5, a0
	mv          a2, s3
	mv          a1, s5
	li          t0, 61		// 0x3d ASCII '='
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 15

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 16

	mv          s6, a0
	mv          a1, s6
	mv          a0, s2
	call        SetLoweredNode

	// *** Basic block 17

	mv          a0, s6
	j           .LowerSignExtend_label_49
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
	li          t0, 53		// 0x35 ASCII '5'
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
	li          t0, 58		// 0x3a ASCII ':'
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

	.local  PushArg
	.type PushArg, @function

PushArg:

	// *** Basic block 0

	.local Emit
	.local NewInstruction3
	.local StackPointer
	.local GetIntConstant
	.global TypeIsFloatingPoint
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
	mv          s3, a2
	mv          s4, a3
	ld          s5, 80(s1)
	bne         s5, x0, .PushArg_label_64

	// *** Basic block 1

	mv          a0, s2
	call        StackPointer

	// *** Basic block 2

	mv          a3, s4
	li          t0, 3		// 0x3 ASCII \x3
	mv          a2, t0
	mv          a1, s1
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 3

	mv          a3, a0
	mv          a2, a0
	mv          a1, s3
	li          t0, 84		// 0x54 ASCII 'T'
	mv          a0, t0
	call        NewInstruction3

	// *** Basic block 4

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

	// *** Basic block 7

.PushArg_label_64:
	li          s6, 84		// 0x54 ASCII 'T'
	mv          a0, s5
	call        TypeIsFloatingPoint

	// *** Basic block 8

	beqz        a0, .PushArg_label_72

	// *** Basic block 9

	li          s6, 138		// 0x8a ASCII \x8a

	// *** Basic block 10

.PushArg_label_72:
	mv          a0, s2
	call        StackPointer

	// *** Basic block 11

	mv          a3, s4
	li          t0, 3		// 0x3 ASCII \x3
	mv          a2, t0
	mv          a1, s1
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 12

	mv          a3, a0
	mv          a2, a0
	mv          a1, s3
	mv          a0, s6
	call        NewInstruction3

	// *** Basic block 13

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
.func_end_PushArg:
	.size PushArg, .func_end_PushArg-PushArg

	.local  PopArg
	.type PopArg, @function

PopArg:

	// *** Basic block 0

	.local Emit
	.local NewInstruction2
	.local StackPointer
	.local GetIntConstant
	.global TypeIsFloatingPoint
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
	mv          s3, a2
	ld          s4, 80(s1)
	bne         s4, x0, .PopArg_label_60

	// *** Basic block 1

	mv          a0, s2
	call        StackPointer

	// *** Basic block 2

	mv          a3, s3
	li          t0, 3		// 0x3 ASCII \x3
	mv          a2, t0
	mv          a1, s1
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 3

	mv          a2, a0
	mv          a1, a0
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 4

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

	// *** Basic block 7

.PopArg_label_60:
	li          s5, 83		// 0x53 ASCII 'S'
	mv          a0, s4
	call        TypeIsFloatingPoint

	// *** Basic block 8

	beqz        a0, .PopArg_label_68

	// *** Basic block 9

	li          s5, 137		// 0x89 ASCII \x89

	// *** Basic block 10

.PopArg_label_68:
	mv          a0, s2
	call        StackPointer

	// *** Basic block 11

	mv          a3, s3
	li          t0, 3		// 0x3 ASCII \x3
	mv          a2, t0
	mv          a1, s1
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 12

	mv          a2, a0
	mv          a1, a0
	mv          a0, s5
	call        NewInstruction2

	// *** Basic block 13

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
.func_end_PopArg:
	.size PopArg, .func_end_PopArg-PopArg

	.local  LowerMemcpy
	.type LowerMemcpy, @function

LowerMemcpy:

	// *** Basic block 0

	.global printf
	.global abort
	.global IRIsConst
	.local GetRegAndOffset
	.global RVIsIntConst
	.local AddValue
	.local Memcpy
	.local SetLoweredNode
	addi sp, sp, -112
	// Saved return address (offset 104) and frame pointer (offset 96)
	sd ra, 104(sp)
	sd s0, 96(sp)
	addi s0, sp, 112
	// Local vars at offset -48(s0)
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
	bne         t0, t1, .LowerMemcpy_label_45

	// *** Basic block 1

	j           .LowerMemcpy_label_62

	// *** Basic block 2

.LowerMemcpy_label_45:
	lla         a0, .str.264
	lla         a1, .str.265
	lla         a3, .str.266
	li          t0, 2609		// 0xa31
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

.LowerMemcpy_label_62:
	ld          s3, 24(s1)
	ld          s4, 16(s3)
	mv          a0, s4
	call        IRIsConst

	// *** Basic block 5

	beqz        a0, .LowerMemcpy_label_72

	// *** Basic block 6

	j           .LowerMemcpy_label_87

	// *** Basic block 7

.LowerMemcpy_label_72:
	lla         a0, .str.267
	lla         a1, .str.268
	lla         a3, .str.269
	li          t0, 2610		// 0xa32
	mv          a2, t0
	call        printf

	// *** Basic block 8

	call        abort

	// *** Basic block 9

.LowerMemcpy_label_87:
	ld          s5, 8(s3)
	mv          s6, x0
	addi        a2, s0, -48
	addi        a3, s0, -40
	mv          a1, s5
	mv          a0, s2
	call        GetRegAndOffset

	// *** Basic block 10

	ld          s7, -40(s0)
	beq         s7, x0, .LowerMemcpy_label_128

	// *** Basic block 11

	lw          a0, 16(s7)
	call        RVIsIntConst

	// *** Basic block 12

	not         t0, a0
	beqz        t0, .LowerMemcpy_label_124

	// *** Basic block 13

	ld          a1, -48(s0)
	mv          a2, s7
	mv          a0, s2
	call        AddValue

	// *** Basic block 14

	sd          a0, -48(s0)
	j           .LowerMemcpy_label_127

	// *** Basic block 15

.LowerMemcpy_label_124:
	ld          s6, 120(s7)

	// *** Basic block 16

.LowerMemcpy_label_127:

	// *** Basic block 17

.LowerMemcpy_label_128:
	ld          s7, -48(s0)
	sd          s7, 96(s5)
	ld          s5, 0(s3)
	mv          s3, x0
	addi        a2, s0, -32
	addi        a3, s0, -24
	mv          a1, s5
	mv          a0, s2
	call        GetRegAndOffset

	// *** Basic block 18

	ld          s5, -24(s0)
	beq         s5, x0, .LowerMemcpy_label_170

	// *** Basic block 19

	lw          a0, 16(s5)
	call        RVIsIntConst

	// *** Basic block 20

	not         t0, a0
	beqz        t0, .LowerMemcpy_label_166

	// *** Basic block 21

	ld          a1, -32(s0)
	mv          a2, s5
	mv          a0, s2
	call        AddValue

	// *** Basic block 22

	sd          a0, -32(s0)
	j           .LowerMemcpy_label_169

	// *** Basic block 23

.LowerMemcpy_label_166:
	ld          s3, 120(s5)

	// *** Basic block 24

.LowerMemcpy_label_169:

	// *** Basic block 25

.LowerMemcpy_label_170:
	ld          s5, 136(s4)
	ld          a1, -32(s0)
	li          t0, 1		// 0x1 ASCII \x1
	mv          a6, t0
	mv          a5, s3
	mv          a4, s6
	mv          a3, s5
	mv          a2, s7
	mv          a0, s2
	call        Memcpy

	// *** Basic block 26

	mv          s4, a0
	mv          a1, s4
	mv          a0, s1
	call        SetLoweredNode

	// *** Basic block 27

	mv          a0, s4

	// *** Basic block 28

.LowerMemcpy_label_202:
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
	.local GetRegAndOffset
	.global RVIsIntConst
	.local AddValue
	.local Memzero
	.local SetLoweredNode
	addi sp, sp, -80
	// Saved return address (offset 72) and frame pointer (offset 64)
	sd ra, 72(sp)
	sd s0, 64(sp)
	addi s0, sp, 80
	// Local vars at offset -32(s0)
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
	li          t1, 1		// 0x1 ASCII \x1
	bne         t0, t1, .LowerMemzero_label_40

	// *** Basic block 1

	j           .LowerMemzero_label_57

	// *** Basic block 2

.LowerMemzero_label_40:
	lla         a0, .str.270
	lla         a1, .str.271
	lla         a3, .str.272
	li          t0, 2655		// 0xa5f
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

.LowerMemzero_label_57:
	ld          t0, 24(s1)
	ld          s3, 0(t0)
	mv          s4, s3
	mv          s5, x0
	addi        a2, s0, -32
	addi        a3, s0, -24
	mv          a1, s3
	mv          a0, s2
	call        GetRegAndOffset

	// *** Basic block 5

	ld          s6, -24(s0)
	beq         s6, x0, .LowerMemzero_label_102

	// *** Basic block 6

	lw          a0, 16(s6)
	call        RVIsIntConst

	// *** Basic block 7

	not         t0, a0
	beqz        t0, .LowerMemzero_label_98

	// *** Basic block 8

	ld          a1, -32(s0)
	mv          a2, s6
	mv          a0, s2
	call        AddValue

	// *** Basic block 9

	sd          a0, -32(s0)
	j           .LowerMemzero_label_101

	// *** Basic block 10

.LowerMemzero_label_98:
	ld          s5, 120(s6)

	// *** Basic block 11

.LowerMemzero_label_101:

	// *** Basic block 12

.LowerMemzero_label_102:
	ld          a1, -32(s0)
	sd          a1, 96(s3)
	ld          t0, 136(s4)
	ld          t0, 40(t0)
	lw          a2, 20(t0)
	mv          a3, s5
	mv          a0, s2
	call        Memzero

	// *** Basic block 13

	mv          s6, a0
	mv          a1, s6
	mv          a0, s1
	call        SetLoweredNode

	// *** Basic block 14

	mv          a0, s6

	// *** Basic block 15

.LowerMemzero_label_128:
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
.func_end_LowerMemzero:
	.size LowerMemzero, .func_end_LowerMemzero-LowerMemzero

	.local  NewArgLocationRegister
	.type NewArgLocationRegister, @function

NewArgLocationRegister:

	// *** Basic block 0

	.global malloc
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
	li          a0, 24		// 0x18 ASCII \x18
	call        malloc

	// *** Basic block 1

	mv          s2, a0
	sw          x0, 0(s2)
	sd          s1, 8(s2)
	sd          x0, 16(s2)
	mv          a0, s2

	// *** Basic block 2

.NewArgLocationRegister_label_24:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewArgLocationRegister:
	.size NewArgLocationRegister, .func_end_NewArgLocationRegister-NewArgLocationRegister

	.local  NewArgLocationPushed
	.type NewArgLocationPushed, @function

NewArgLocationPushed:

	// *** Basic block 0

	.global malloc
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
	li          a0, 24		// 0x18 ASCII \x18
	call        malloc

	// *** Basic block 1

	mv          s3, a0
	sw          s1, 0(s3)
	sd          s2, 8(s3)
	sd          x0, 16(s3)
	mv          a0, s3

	// *** Basic block 2

.NewArgLocationPushed_label_27:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewArgLocationPushed:
	.size NewArgLocationPushed, .func_end_NewArgLocationPushed-NewArgLocationPushed

	.local  NewArgLocationReferenceInRegister
	.type NewArgLocationReferenceInRegister, @function

NewArgLocationReferenceInRegister:

	// *** Basic block 0

	.global malloc
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
	li          a0, 24		// 0x18 ASCII \x18
	call        malloc

	// *** Basic block 1

	mv          s3, a0
	li          t0, 2		// 0x2 ASCII \x2
	sw          t0, 0(s3)
	sd          s1, 8(s3)
	sd          s2, 16(s3)
	mv          a0, s3

	// *** Basic block 2

.NewArgLocationReferenceInRegister_label_28:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewArgLocationReferenceInRegister:
	.size NewArgLocationReferenceInRegister, .func_end_NewArgLocationReferenceInRegister-NewArgLocationReferenceInRegister

	.local  NewArgLocationReferenceOnStack
	.type NewArgLocationReferenceOnStack, @function

NewArgLocationReferenceOnStack:

	// *** Basic block 0

	.global malloc
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
	li          a0, 24		// 0x18 ASCII \x18
	call        malloc

	// *** Basic block 1

	mv          s3, a0
	li          t0, 3		// 0x3 ASCII \x3
	sw          t0, 0(s3)
	sd          s1, 8(s3)
	sd          s2, 16(s3)
	mv          a0, s3

	// *** Basic block 2

.NewArgLocationReferenceOnStack_label_28:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewArgLocationReferenceOnStack:
	.size NewArgLocationReferenceOnStack, .func_end_NewArgLocationReferenceOnStack-NewArgLocationReferenceOnStack

	.local  BuildArgList
	.type BuildArgList, @function

BuildArgList:

	// *** Basic block 0

	.local Emit
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
	mv          t0, a1
	mv          s1, a0
	mv          s2, x0
	mv          s3, x0
	ld          s4, 8(t0)
	bge         x0, s4, .BuildArgList_label_55

	// *** Basic block 1

	ld          t1, 0(t0)

	// *** Basic block 2

.BuildArgList_label_25:
	slli        t0, s3, 3
	add         t0, t1, t0
	ld          s5, 0(t0)
	lw          t0, 0(s5)
	bnez        t0, .BuildArgList_label_50

	// *** Basic block 3

	ld          a2, 8(s5)
	mv          a1, s2
	li          t0, 209		// 0xd1 ASCII \xd1
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 4

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 5

	mv          s2, a0

	// *** Basic block 6

.BuildArgList_label_50:

	// *** Basic block 7

.BuildArgList_label_51:
	addi        s3, s3, 1
	bge         s3, s4, .BuildArgList_label_25

	// *** Basic block 8

.BuildArgList_label_55:
	mv          a0, s2

	// *** Basic block 9

.BuildArgList_label_58:
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
.func_end_BuildArgList:
	.size BuildArgList, .func_end_BuildArgList-BuildArgList

	.local  LowerCall
	.type LowerCall, @function

LowerCall:

	// *** Basic block 0

	.global printf
	.global abort
	.global VectorInit
	.global TypeIsStructOrUnion
	.local IntArgumentRegister
	.global VectorAppend
	.local NewArgLocationRegister
	.local NewArgLocationPushed
	.local NewArgLocationReferenceInRegister
	.local NewArgLocationReferenceOnStack
	.global TypeIsFloatingPoint
	.local FloatingPointArgumentRegister
	.local AddImmediate
	.local StackPointer
	.global TargetSetDest
	.local Materialize
	.local Memcpy
	.local SetDestOrMoveToArgReg
	.local PushArg
	.local Emit
	.local NewInstruction2
	.local GetIntConstant
	.global TypeIsDouble
	.local GetLoweredNode
	.global RVIsExpression
	.local BuildArgList
	.local Tmp
	.local NewInstruction
	.local NewInstruction1
	.local SetLoweredNode
	.global VectorDestructWithContents
	sd          a0, -8(s0)	// Spilled @862
	addi sp, sp, -160
	// Saved return address (offset 152) and frame pointer (offset 144)
	sd ra, 152(sp)
	sd s0, 144(sp)
	addi s0, sp, 160
	// Local vars at offset -48(s0)
	// Spilled register region: 16 bytes at -64(s0) to -48(s0)
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
	sd          s2, -56(s0)	// Spilled @67
	addi        t0, s1, 24
	ld          s3, 8(t0)
	li          s4, 1		// 0x1 ASCII \x1
	blt         s3, s4, .LowerCall_label_82

	// *** Basic block 1

	ld          t0, 24(s1)
	ld          t1, 24(s1)
	ld          t2, 24(s1)
	j           .LowerCall_label_99

	// *** Basic block 2

.LowerCall_label_82:
	lla         a0, .str.273
	lla         a1, .str.274
	lla         a3, .str.275
	li          t0, 2776		// 0xad8
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

.LowerCall_label_99:
	mv          s5, x0
	sd          s5, -64(s0)	// Spilled @100
	mv          s6, x0
	sd          s6, -56(s0)	// Spilled @102
	mv          s7, x0
	mv          s8, x0
	sd          s8, -56(s0)	// Spilled @106
	addi        a0, s0, -48
	call        VectorInit

	// *** Basic block 5

	li          s9, 1		// 0x1 ASCII \x1
	bge         s4, s3, .LowerCall_label_338

	// *** Basic block 6

.LowerCall_label_119:
	slli        t0, s9, 3
	add         t0, t0, t0
	ld          s10, 0(t0)
	ld          s11, 80(s10)
	mv          a0, s11
	call        TypeIsStructOrUnion

	// *** Basic block 7

	beqz        a0, .LowerCall_label_251

	// *** Basic block 8

	addi        t1, s9, -1
	seqz        t0, t1
	bne         s9, s4, .LowerCall_label_141

	// *** Basic block 9

	lw          t1, 20(s10)
	addi        t1, t1, -102
	seqz        t0, t1

	// *** Basic block 10

.LowerCall_label_141:
	beqz        t0, .LowerCall_label_162

	// *** Basic block 11

	addi        s6, s6, 1
	mv          a0, s2
	call        IntArgumentRegister

	// *** Basic block 12

	mv          s10, a0
	addi        s2, s0, -48
	mv          a0, s10
	call        NewArgLocationRegister

	// *** Basic block 13

	mv          a1, a0
	mv          a0, s2
	call        VectorAppend

	// *** Basic block 14

	j           .LowerCall_label_334

	// *** Basic block 15

.LowerCall_label_162:
	lw          s2, 20(s11)
	li          s6, 8		// 0x8 ASCII \x8
	bge         s2, s6, .LowerCall_label_209

	// *** Basic block 16

	ld          t0, -56(s0)	// Spilled @102
	bge         t0, s6, .LowerCall_label_193

	// *** Basic block 17

	ld          t0, -56(s0)	// Spilled @102
	addi        s6, t0, 1
	ld          a0, -56(s0)	// Spilled @67
	call        IntArgumentRegister

	// *** Basic block 18

	mv          s8, a0
	sd          s8, -56(s0)	// Spilled @181
	addi        a0, s0, -48
	mv          a0, s8
	call        NewArgLocationRegister

	// *** Basic block 19

	mv          a1, a0
	call        VectorAppend

	// *** Basic block 20

	j           .LowerCall_label_207

	// *** Basic block 21

.LowerCall_label_193:
	addi        s8, s0, -48
	ld          a1, -56(s0)	// Spilled @106
	mv          a0, s4
	call        NewArgLocationPushed

	// *** Basic block 22

	mv          a1, a0
	mv          a0, s8
	call        VectorAppend

	// *** Basic block 23

	ld          t0, -56(s0)	// Spilled @106
	addi        s8, t0, 8

	// *** Basic block 24

.LowerCall_label_207:
	j           .LowerCall_label_249

	// *** Basic block 25

.LowerCall_label_209:
	ld          t0, -56(s0)	// Spilled @102
	bge         t0, s6, .LowerCall_label_234

	// *** Basic block 26

	ld          t0, -56(s0)	// Spilled @102
	addi        s6, t0, 1
	ld          a0, -56(s0)	// Spilled @67
	call        IntArgumentRegister

	// *** Basic block 27

	mv          s6, a0
	sd          s6, -56(s0)	// Spilled @220
	addi        s8, s0, -48
	mv          a1, s5
	mv          a0, s6
	call        NewArgLocationReferenceInRegister

	// *** Basic block 28

	mv          a1, a0
	mv          a0, s8
	call        VectorAppend

	// *** Basic block 29

	j           .LowerCall_label_247

	// *** Basic block 30

.LowerCall_label_234:
	addi        s8, s0, -48
	mv          a1, s5
	ld          a0, -56(s0)	// Spilled @106
	call        NewArgLocationReferenceOnStack

	// *** Basic block 31

	mv          a1, a0
	mv          a0, s8
	call        VectorAppend

	// *** Basic block 32

	ld          t0, -56(s0)	// Spilled @106
	addi        s8, t0, 8

	// *** Basic block 33

.LowerCall_label_247:
	add         s5, s5, s2

	// *** Basic block 34

.LowerCall_label_249:
	j           .LowerCall_label_333

	// *** Basic block 35

.LowerCall_label_251:
	mv          a0, s11
	call        TypeIsFloatingPoint

	// *** Basic block 36

	beqz        a0, .LowerCall_label_294

	// *** Basic block 37

	li          t0, 8		// 0x8 ASCII \x8
	bge         s7, t0, .LowerCall_label_278

	// *** Basic block 38

	addi        s7, s7, 1
	ld          a0, -56(s0)	// Spilled @67
	call        FloatingPointArgumentRegister

	// *** Basic block 39

	mv          s2, a0
	sd          s2, -56(s0)	// Spilled @266
	addi        s8, s0, -48
	mv          a0, s2
	call        NewArgLocationRegister

	// *** Basic block 40

	mv          a1, a0
	mv          a0, s8
	call        VectorAppend

	// *** Basic block 41

	j           .LowerCall_label_292

	// *** Basic block 42

.LowerCall_label_278:
	addi        s8, s0, -48
	ld          a1, -56(s0)	// Spilled @106
	mv          a0, s4
	call        NewArgLocationPushed

	// *** Basic block 43

	mv          a1, a0
	mv          a0, s8
	call        VectorAppend

	// *** Basic block 44

	ld          t0, -56(s0)	// Spilled @106
	addi        s8, t0, 8

	// *** Basic block 45

.LowerCall_label_292:
	j           .LowerCall_label_332

	// *** Basic block 46

.LowerCall_label_294:
	li          t0, 8		// 0x8 ASCII \x8
	ld          t1, -56(s0)	// Spilled @102
	bge         t1, t0, .LowerCall_label_317

	// *** Basic block 47

	ld          t0, -56(s0)	// Spilled @102
	addi        s6, t0, 1
	ld          a0, -56(s0)	// Spilled @67
	call        IntArgumentRegister

	// *** Basic block 48

	mv          s8, a0
	addi        s11, s0, -48
	mv          a0, s8
	call        NewArgLocationRegister

	// *** Basic block 49

	mv          a1, a0
	mv          a0, s11
	call        VectorAppend

	// *** Basic block 50

	j           .LowerCall_label_331

	// *** Basic block 51

.LowerCall_label_317:
	addi        s11, s0, -48
	ld          a1, -56(s0)	// Spilled @106
	mv          a0, s4
	call        NewArgLocationPushed

	// *** Basic block 52

	mv          a1, a0
	mv          a0, s11
	call        VectorAppend

	// *** Basic block 53

	ld          t0, -56(s0)	// Spilled @106
	addi        s8, t0, 8

	// *** Basic block 54

.LowerCall_label_331:

	// *** Basic block 55

.LowerCall_label_332:

	// *** Basic block 56

.LowerCall_label_333:

	// *** Basic block 57

.LowerCall_label_334:
	addi        s9, s9, 1
	bge         s9, s3, .LowerCall_label_119

	// *** Basic block 58

.LowerCall_label_338:
	ld          t0, -56(s0)	// Spilled @106
	add         s9, s5, t0
	bge         x0, s9, .LowerCall_label_365

	// *** Basic block 59

	ld          s11, -48(s0)
	ld          s2, -48(s0)
	ld          a0, -56(s0)	// Spilled @67
	call        StackPointer
	sd          a0, -56(s0)	// Spilled @347

	// *** Basic block 60

	neg         a2, s9
	mv          a1, a0
	ld          a0, -56(s0)	// Spilled @67
	call        AddImmediate

	// *** Basic block 61

	sd          a0, -56(s0)	// Spilled @355
	ld          a0, -56(s0)	// Spilled @67
	call        StackPointer

	// *** Basic block 62

	mv          a1, a0
	call        TargetSetDest

	// *** Basic block 63

.LowerCall_label_365:
	li          s2, 1		// 0x1 ASCII \x1
	bge         s4, s3, .LowerCall_label_445

	// *** Basic block 64

.LowerCall_label_372:
	addi        t0, s2, -1
	slli        t0, t0, 3
	add         t0, s11, t0
	ld          s11, 0(t0)
	slli        t0, s2, 3
	add         t0, t1, t0
	ld          a0, 0(t0)
	ld          t0, 80(a0)
	lw          s5, 20(t0)
	lw          t0, 0(s11)
	li          t1, 2		// 0x2 ASCII \x2
	blt         t0, t1, .LowerCall_label_438

	// *** Basic block 65

	li          t1, 3		// 0x3 ASCII \x3
	blt         t1, t0, .LowerCall_label_438

	// *** Basic block 66

	addi        t0, t0, -2
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 67

	j           .LowerCall_label_404

	// *** Basic block 68

	j           .LowerCall_label_405

	// *** Basic block 69

.LowerCall_label_404:

	// *** Basic block 70

.LowerCall_label_405:
	mv          a1, a0
	ld          a0, -56(s0)	// Spilled @67
	call        Materialize

	// *** Basic block 71

	mv          s6, a0
	sd          s6, -64(s0)	// Spilled @411
	ld          a0, -56(s0)	// Spilled @67
	call        StackPointer

	// *** Basic block 72

	sext.w      a3, s5
	ld          t0, 16(s11)
	ld          t1, -56(s0)	// Spilled @106
	add         t0, t0, t1
	sext.w      a5, t0
	mv          a6, x0
	mv          a4, x0
	mv          a2, s6
	mv          a1, a0
	ld          a0, -56(s0)	// Spilled @67
	call        Memcpy

	// *** Basic block 73

	j           .LowerCall_label_440

	// *** Basic block 74

.LowerCall_label_438:
	j           .LowerCall_label_440

	// *** Basic block 75

.LowerCall_label_440:

	// *** Basic block 76

.LowerCall_label_441:
	addi        s2, s2, 1
	bge         s2, s3, .LowerCall_label_372

	// *** Basic block 77

.LowerCall_label_445:
	addi        s2, s3, -1
	blt         s2, s4, .LowerCall_label_668

	// *** Basic block 78

.LowerCall_label_452:
	slli        t0, s2, 3
	add         t0, t2, t0
	ld          s3, 0(t0)
	addi        s2, s2, -1
	slli        t0, s2, 3
	add         t0, s2, t0
	ld          s5, 0(t0)
	lw          t0, 0(s5)
	sd          s5, -56(s0)	// Spilled @461
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 79

	j           .LowerCall_label_585

	// *** Basic block 80

	j           .LowerCall_label_526

	// *** Basic block 81

	j           .LowerCall_label_471

	// *** Basic block 82

	j           .LowerCall_label_500

	// *** Basic block 83

.LowerCall_label_471:
	ld          a0, -56(s0)	// Spilled @67
	call        StackPointer

	// *** Basic block 84

	ld          t0, -56(s0)	// Spilled @461
	ld          t1, 16(t0)
	ld          t2, -56(s0)	// Spilled @106
	add         a2, t1, t2
	mv          a1, a0
	ld          a0, -56(s0)	// Spilled @67
	call        AddImmediate

	// *** Basic block 85

	mv          s5, a0
	ld          t0, -56(s0)	// Spilled @461
	ld          a3, 8(t0)
	li          t1, 18		// 0x12 ASCII \x12
	mv          a4, t1
	mv          a2, s5
	ld          a1, -56(s0)	// Spilled @456
	sd          s3, -56(s0)	// Spilled @456
	ld          a0, -56(s0)	// Spilled @67
	call        SetDestOrMoveToArgReg

	// *** Basic block 86

	j           .LowerCall_label_662

	// *** Basic block 87

.LowerCall_label_500:
	ld          a0, -56(s0)	// Spilled @67
	call        StackPointer

	// *** Basic block 88

	ld          t0, -56(s0)	// Spilled @461
	ld          t1, 16(t0)
	ld          t2, -56(s0)	// Spilled @106
	add         a2, t1, t2
	mv          a1, a0
	ld          a0, -56(s0)	// Spilled @67
	call        AddImmediate

	// *** Basic block 89

	mv          s6, a0
	ld          t0, -56(s0)	// Spilled @461
	ld          a3, 8(t0)
	mv          a2, s6
	ld          a1, -56(s0)	// Spilled @456
	ld          a0, -56(s0)	// Spilled @67
	call        PushArg

	// *** Basic block 90

	j           .LowerCall_label_662

	// *** Basic block 91

.LowerCall_label_526:
	ld          a1, -56(s0)	// Spilled @456
	ld          a0, -56(s0)	// Spilled @67
	call        Materialize

	// *** Basic block 92

	mv          s3, a0
	ld          t0, -56(s0)	// Spilled @456
	ld          s5, 80(t0)
	mv          a0, s5
	call        TypeIsStructOrUnion

	// *** Basic block 93

	beqz        a0, .LowerCall_label_573

	// *** Basic block 94

	lw          s6, 20(s5)
	li          t0, 8		// 0x8 ASCII \x8
	bge         s6, t0, .LowerCall_label_572

	// *** Basic block 95

	mv          a3, x0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	mv          a1, x0
	ld          a0, -56(s0)	// Spilled @67
	call        GetIntConstant

	// *** Basic block 96

	mv          a2, a0
	mv          a1, s3
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 97

	mv          a1, a0
	ld          a0, -56(s0)	// Spilled @67
	call        Emit

	// *** Basic block 98

	mv          s3, a0

	// *** Basic block 99

.LowerCall_label_572:

	// *** Basic block 100

.LowerCall_label_573:
	ld          t0, -56(s0)	// Spilled @461
	ld          a3, 8(t0)
	mv          a2, s3
	ld          a1, -56(s0)	// Spilled @456
	ld          a0, -56(s0)	// Spilled @67
	call        PushArg

	// *** Basic block 101

	j           .LowerCall_label_662

	// *** Basic block 102

.LowerCall_label_585:
	mv          a1, s3
	ld          a0, -56(s0)	// Spilled @67
	call        Materialize

	// *** Basic block 103

	mv          s11, a0
	ld          s3, 80(s3)
	mv          a0, s3
	call        TypeIsStructOrUnion

	// *** Basic block 104

	beqz        a0, .LowerCall_label_632

	// *** Basic block 105

	lw          s5, 20(s3)
	li          t0, 8		// 0x8 ASCII \x8
	bge         s5, t0, .LowerCall_label_631

	// *** Basic block 106

	mv          a3, x0
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	mv          a1, x0
	ld          a0, -56(s0)	// Spilled @67
	call        GetIntConstant

	// *** Basic block 107

	mv          a2, a0
	mv          a1, s11
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 108

	mv          a1, a0
	ld          a0, -56(s0)	// Spilled @67
	call        Emit

	// *** Basic block 109

	mv          s11, a0

	// *** Basic block 110

.LowerCall_label_631:

	// *** Basic block 111

.LowerCall_label_632:
	li          s5, 18		// 0x12 ASCII \x12
	mv          a0, s3
	call        TypeIsFloatingPoint

	// *** Basic block 112

	beqz        a0, .LowerCall_label_648

	// *** Basic block 113

	mv          a0, s3
	call        TypeIsDouble

	// *** Basic block 114

	beqz        a0, .LowerCall_label_645

	// *** Basic block 115

	li          s5, 20		// 0x14 ASCII \x14
	j           .LowerCall_label_647

	// *** Basic block 116

.LowerCall_label_645:
	li          s5, 19		// 0x13 ASCII \x13

	// *** Basic block 117

.LowerCall_label_647:

	// *** Basic block 118

.LowerCall_label_648:
	ld          t0, -56(s0)	// Spilled @461
	ld          a3, 8(t0)
	mv          a4, s5
	mv          a2, s11
	ld          a1, -56(s0)	// Spilled @456
	ld          a0, -56(s0)	// Spilled @67
	call        SetDestOrMoveToArgReg

	// *** Basic block 119

	j           .LowerCall_label_662

	// *** Basic block 120

.LowerCall_label_662:

	// *** Basic block 121

.LowerCall_label_663:
	blt         s2, s4, .LowerCall_label_452

	// *** Basic block 122

.LowerCall_label_668:
	ld          t0, 24(s1)
	ld          a0, 0(t0)
	call        GetLoweredNode
	sd          a0, -56(s0)	// Spilled @672

	// *** Basic block 123

	mv          s2, a0
	lw          t0, 88(s1)
	andi        t0, t0, 4
	snez        a0, t0
	beqz        t0, .LowerCall_label_683

	// *** Basic block 124

	seqz        a0, s9

	// *** Basic block 125

.LowerCall_label_683:
	beqz        a0, .LowerCall_label_688

	// *** Basic block 126

	ld          t0, -56(s0)	// Spilled @67
	lw          t1, 128(t0)
	seqz        a0, t1

	// *** Basic block 127

.LowerCall_label_688:
	beqz        a0, .LowerCall_label_776

	// *** Basic block 128

	lw          a0, 16(s2)
	call        RVIsExpression

	// *** Basic block 129

	beqz        a0, .LowerCall_label_743

	// *** Basic block 130

	addi        a1, s0, -48
	ld          a0, -56(s0)	// Spilled @67
	call        BuildArgList

	// *** Basic block 131

	ld          a0, -56(s0)	// Spilled @67
	call        Tmp
	sd          a0, -56(s0)	// Spilled @703

	// *** Basic block 132

	mv          a2, s2
	mv          a1, a0
	li          t0, 18		// 0x12 ASCII \x12
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 133

	mv          a1, a0
	ld          a0, -56(s0)	// Spilled @67
	call        Emit

	// *** Basic block 134

	mv          a0, s4
	call        NewInstruction

	// *** Basic block 135

	mv          a1, a0
	ld          a0, -56(s0)	// Spilled @67
	call        Emit

	// *** Basic block 136

	ld          a0, -56(s0)	// Spilled @67
	call        Tmp

	// *** Basic block 137

	mv          a1, a0
	li          t0, 182		// 0xb6 ASCII \xb6
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 138

	mv          a1, a0
	ld          a0, -56(s0)	// Spilled @67
	call        Emit

	// *** Basic block 139

	j           .LowerCall_label_770

	// *** Basic block 140

.LowerCall_label_743:
	mv          a0, s4
	call        NewInstruction

	// *** Basic block 141

	mv          a1, a0
	ld          a0, -56(s0)	// Spilled @67
	call        Emit

	// *** Basic block 142

	addi        a1, s0, -48
	ld          a0, -56(s0)	// Spilled @67
	call        BuildArgList

	// *** Basic block 143

	mv          a1, s2
	li          t0, 181		// 0xb5 ASCII \xb5
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 144

	mv          a1, a0
	ld          a0, -56(s0)	// Spilled @67
	call        Emit

	// *** Basic block 145


	// *** Basic block 146

.LowerCall_label_770:
	ld          t0, -56(s0)	// Spilled @67
	lw          t1, 44(t0)
	addi        t1, t1, -1
	ld          t2, -56(s0)	// Spilled @67
	sw          t1, 44(t2)
	j           .LowerCall_label_851

	// *** Basic block 147

.LowerCall_label_776:
	lw          t0, 16(s2)
	li          t1, 2		// 0x2 ASCII \x2
	bne         t0, t1, .LowerCall_label_797

	// *** Basic block 148

	ld          a0, 80(s1)
	call        TypeIsFloatingPoint

	// *** Basic block 149

	beqz        a0, .LowerCall_label_792

	// *** Basic block 150

	li          s4, 185		// 0xb9 ASCII \xb9
	j           .LowerCall_label_794

	// *** Basic block 151

.LowerCall_label_792:
	li          s4, 183		// 0xb7 ASCII \xb7

	// *** Basic block 152

.LowerCall_label_794:
	j           .LowerCall_label_810

	// *** Basic block 153

.LowerCall_label_797:
	ld          a0, 80(s1)
	call        TypeIsFloatingPoint

	// *** Basic block 154

	beqz        a0, .LowerCall_label_807

	// *** Basic block 155

	li          s4, 186		// 0xba ASCII \xba
	j           .LowerCall_label_809

	// *** Basic block 156

.LowerCall_label_807:
	li          s4, 184		// 0xb8 ASCII \xb8

	// *** Basic block 157

.LowerCall_label_809:

	// *** Basic block 158

.LowerCall_label_810:
	addi        a1, s0, -48
	ld          a0, -56(s0)	// Spilled @67
	call        BuildArgList

	// *** Basic block 159

	mv          a2, a0
	mv          a1, s2
	mv          a0, s4
	call        NewInstruction2

	// *** Basic block 160

	mv          a1, a0
	ld          a0, -56(s0)	// Spilled @67
	call        Emit

	// *** Basic block 161

	bge         x0, s9, .LowerCall_label_850

	// *** Basic block 162

	ld          a0, -56(s0)	// Spilled @67
	call        StackPointer

	// *** Basic block 163

	mv          a2, s9
	mv          a1, a0
	ld          a0, -56(s0)	// Spilled @67
	call        AddImmediate

	// *** Basic block 164

	mv          s4, a0
	ld          a0, -56(s0)	// Spilled @67
	call        StackPointer

	// *** Basic block 165

	mv          a1, a0
	mv          a0, s4
	call        TargetSetDest

	// *** Basic block 166

.LowerCall_label_850:

	// *** Basic block 167

.LowerCall_label_851:
	mv          a1, a0
	mv          a0, s1
	call        SetLoweredNode

	// *** Basic block 168

	addi        a0, s0, -48
	mv          a1, x0
	call        VectorDestructWithContents

	// *** Basic block 169


	// *** Basic block 170

.LowerCall_label_864:
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
.func_end_LowerCall:
	.size LowerCall, .func_end_LowerCall-LowerCall

	.local  LowerComputedBranch
	.type LowerComputedBranch, @function

LowerComputedBranch:

	// *** Basic block 0

	.global printf
	.global abort
	.local GetLoweredNode
	.local Emit
	.local NewInstruction2
	.local GetIntConstant
	.local NewInstruction1
	.local NewInstruction3
	.local Zero
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
	li          t1, 1		// 0x1 ASCII \x1
	bne         t0, t1, .LowerComputedBranch_label_46

	// *** Basic block 1

	j           .LowerComputedBranch_label_63

	// *** Basic block 2

.LowerComputedBranch_label_46:
	lla         a0, .str.276
	lla         a1, .str.277
	lla         a3, .str.278
	li          t0, 3049		// 0xbe9
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

.LowerComputedBranch_label_63:
	ld          t0, 24(s1)
	ld          a0, 0(t0)
	call        GetLoweredNode

	// *** Basic block 5

	mv          s3, a0
	li          s4, 2		// 0x2 ASCII \x2
	mv          a3, s4
	mv          a2, s4
	mv          a1, x0
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 6

	mv          a2, a0
	mv          a1, s3
	li          t0, 59		// 0x3b ASCII ';'
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 7

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 8

	mv          s5, a0
	mv          a3, x0
	mv          a2, s4
	mv          a1, x0
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 9

	mv          a1, a0
	li          t0, 36		// 0x24 ASCII '$'
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 10

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 11

	mv          s6, a0
	mv          a2, s5
	mv          a1, s6
	li          t0, 62		// 0x3e ASCII '>'
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 12

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 13

	mv          s7, a0
	mv          a0, s2
	call        Zero

	// *** Basic block 14

	li          t0, 12		// 0xc ASCII \xc
	mv          a3, t0
	mv          a2, s4
	mv          a1, x0
	mv          a0, s2
	call        GetIntConstant

	// *** Basic block 15

	mv          a3, a0
	mv          a2, s7
	mv          a1, a0
	li          t0, 38		// 0x26 ASCII '&'
	mv          a0, t0
	call        NewInstruction3

	// *** Basic block 16

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 17

	mv          s4, a0
	lw          t0, 104(s4)
	li          t1, 131072		// 0x20000
	or          t0, t0, t1
	sw          t0, 104(s4)
	mv          a1, s4
	mv          a0, s1
	call        SetLoweredNode

	// *** Basic block 18

	mv          a0, s4

	// *** Basic block 19

.LowerComputedBranch_label_177:
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
.func_end_LowerComputedBranch:
	.size LowerComputedBranch, .func_end_LowerComputedBranch-LowerComputedBranch

	.local  LowerBuiltinVaStart
	.type LowerBuiltinVaStart, @function

LowerBuiltinVaStart:

	// *** Basic block 0

	.local Emit
	.local NewInstruction
	.local GetRegAndOffset
	.local NewInstruction2
	.local SetLoweredNode
	.local NewInstruction3
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
	mv          s1, a0
	mv          s2, a1
	li          a0, 23		// 0x17 ASCII \x17
	call        NewInstruction

	// *** Basic block 1

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 2

	mv          s3, a0
	ld          t0, 24(s2)
	ld          a1, 0(t0)
	addi        a2, s0, -32
	addi        a3, s0, -24
	mv          a0, s1
	call        GetRegAndOffset

	// *** Basic block 3

	mv          s4, a0
	not         t0, s4
	beqz        t0, .LowerBuiltinVaStart_label_74

	// *** Basic block 4

	ld          s4, -32(s0)
	mv          a2, s3
	mv          a1, s4
	li          t0, 18		// 0x12 ASCII \x12
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 5

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 6

	mv          a1, s4
	mv          a0, s2
	call        SetLoweredNode

	// *** Basic block 7


	// *** Basic block 8

.LowerBuiltinVaStart_label_71:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 9

.LowerBuiltinVaStart_label_74:
	ld          a2, -32(s0)
	ld          a3, -24(s0)
	mv          a1, s3
	li          t0, 84		// 0x54 ASCII 'T'
	mv          a0, t0
	call        NewInstruction3

	// *** Basic block 10

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 11

	mv          a1, a0
	mv          a0, s2
	call        SetLoweredNode

	// *** Basic block 12

	j           .LowerBuiltinVaStart_label_71
.func_end_LowerBuiltinVaStart:
	.size LowerBuiltinVaStart, .func_end_LowerBuiltinVaStart-LowerBuiltinVaStart

	.local  LowerBuiltinVaArg
	.type LowerBuiltinVaArg, @function

LowerBuiltinVaArg:

	// *** Basic block 0

	.local GetRegAndOffset
	.local Emit
	.local NewInstruction2
	.local GetIntConstant
	.local NewInstruction3
	.local SetLoweredNode
	addi sp, sp, -80
	// Saved return address (offset 72) and frame pointer (offset 64)
	sd ra, 72(sp)
	sd s0, 64(sp)
	addi s0, sp, 80
	// Local vars at offset -32(s0)
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
	ld          t0, 24(s2)
	ld          a1, 0(t0)
	addi        a2, s0, -32
	addi        a3, s0, -24
	call        GetRegAndOffset

	// *** Basic block 1

	mv          s3, a0
	not         t0, s3
	beqz        t0, .LowerBuiltinVaArg_label_47

	// *** Basic block 2

	ld          s4, -32(s0)
	j           .LowerBuiltinVaArg_label_62

	// *** Basic block 3

.LowerBuiltinVaArg_label_47:
	ld          a1, -32(s0)
	ld          a2, -24(s0)
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 4

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 5

	mv          s4, a0

	// *** Basic block 6

.LowerBuiltinVaArg_label_62:
	mv          a3, x0
	li          s5, 2		// 0x2 ASCII \x2
	mv          a2, s5
	mv          a1, x0
	mv          a0, s1
	call        GetIntConstant

	// *** Basic block 7

	mv          a2, a0
	mv          a1, s4
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 8

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 9

	mv          s6, a0
	li          t0, 8		// 0x8 ASCII \x8
	mv          a3, t0
	mv          a2, s5
	mv          a1, x0
	mv          a0, s1
	call        GetIntConstant

	// *** Basic block 10

	mv          a2, a0
	mv          a1, s4
	li          t0, 53		// 0x35 ASCII '5'
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 11

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 12

	mv          s5, a0
	beqz        s3, .LowerBuiltinVaArg_label_131

	// *** Basic block 13

	ld          a2, -32(s0)
	ld          a3, -24(s0)
	mv          a1, s5
	li          t0, 84		// 0x54 ASCII 'T'
	mv          a0, t0
	call        NewInstruction3

	// *** Basic block 14

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 15

	j           .LowerBuiltinVaArg_label_145

	// *** Basic block 16

.LowerBuiltinVaArg_label_131:
	mv          a2, s5
	mv          a1, s4
	li          t0, 18		// 0x12 ASCII \x12
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 17

	mv          a1, a0
	mv          a0, s1
	call        Emit

	// *** Basic block 18

.LowerBuiltinVaArg_label_145:
	mv          a1, s6
	mv          a0, s2
	call        SetLoweredNode

	// *** Basic block 19


	// *** Basic block 20

.LowerBuiltinVaArg_label_153:
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

	.local  LowerStackPointerOps
	.type LowerStackPointerOps, @function

LowerStackPointerOps:

	// *** Basic block 0

	.local Materialize
	.global TargetIsConst
	.global RVIntValue
	.local AddImmediate
	.local StackPointer
	.local Emit
	.local NewInstruction2
	.global printf
	.global abort
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
	lw          s3, 20(s1)
	li          t0, 125		// 0x7d ASCII '}'
	blt         s3, t0, .LowerStackPointerOps_label_171

	// *** Basic block 1

	li          t0, 127		// 0x7f ASCII \x7f
	blt         t0, s3, .LowerStackPointerOps_label_171

	// *** Basic block 2

	addi        t0, s3, -125
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 3

	j           .LowerStackPointerOps_label_49

	// *** Basic block 4

	j           .LowerStackPointerOps_label_111

	// *** Basic block 5

	j           .LowerStackPointerOps_label_141

	// *** Basic block 6

.LowerStackPointerOps_label_49:
	ld          t0, 24(s1)
	ld          a1, 0(t0)
	mv          a0, s2
	call        Materialize

	// *** Basic block 7

	mv          s3, a0
	mv          a0, s3
	call        TargetIsConst

	// *** Basic block 8

	beqz        a0, .LowerStackPointerOps_label_82

	// *** Basic block 9

	mv          a0, s3
	call        RVIntValue

	// *** Basic block 10

	mv          s4, a0
	mv          a0, s2
	call        StackPointer

	// *** Basic block 11

	neg         a2, s4
	mv          a1, a0
	mv          a0, s2
	call        AddImmediate

	// *** Basic block 12

	mv          s5, a0
	j           .LowerStackPointerOps_label_100

	// *** Basic block 13

.LowerStackPointerOps_label_82:
	mv          a0, s2
	call        StackPointer

	// *** Basic block 14

	mv          a2, s3
	mv          a1, a0
	li          t0, 63		// 0x3f ASCII '?'
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 15

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 16

	mv          s5, a0

	// *** Basic block 17

.LowerStackPointerOps_label_100:
	mv          a0, s2
	call        StackPointer

	// *** Basic block 18

	sd          a0, 24(s5)
	mv          a0, s5

	// *** Basic block 19

.LowerStackPointerOps_label_108:
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

	// *** Basic block 20

.LowerStackPointerOps_label_111:
	ld          t0, 24(s1)
	ld          a1, 0(t0)
	mv          a0, s2
	call        Materialize

	// *** Basic block 21

	mv          s6, a0
	mv          a0, s2
	call        StackPointer

	// *** Basic block 22

	mv          a2, a0
	mv          a1, s6
	li          t0, 18		// 0x12 ASCII \x12
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 23

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

	// *** Basic block 25

.LowerStackPointerOps_label_141:
	ld          t0, 24(s1)
	ld          a1, 0(t0)
	mv          a0, s2
	call        Materialize

	// *** Basic block 26

	mv          s7, a0
	mv          a0, s2
	call        StackPointer

	// *** Basic block 27

	mv          a2, s7
	mv          a1, a0
	li          t0, 18		// 0x12 ASCII \x12
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 28

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

	// *** Basic block 30

.LowerStackPointerOps_label_171:
	lla         a0, .str.279
	lla         a1, .str.280
	lla         a3, .str.281
	li          t0, 3153		// 0xc51
	mv          a2, t0
	call        printf

	// *** Basic block 31

	call        abort

	// *** Basic block 32

	mv          a0, x0
	j           .LowerStackPointerOps_label_108
.func_end_LowerStackPointerOps:
	.size LowerStackPointerOps, .func_end_LowerStackPointerOps-LowerStackPointerOps

	.local  LowerIRNode
	.type LowerIRNode, @function

LowerIRNode:

	// *** Basic block 0

	.local Emit
	.local NewInstruction1
	.local Materialize
	.local SetLoweredNode
	.local EmitSymbol
	.local NewInstruction
	.local NewInstruction2
	.local IntArgumentRegister
	.local LowerLiteralReference
	.local LowerAddressOf
	.local GetIntConstant
	.local GetFloatingPointConstant
	.local LowerVariables
	.local LowerLoad
	.local LowerStore
	.local LowerExpression
	.local LowerRmov
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
	.local LowerZeroExtend
	.local LowerSignExtend
	.local LowerAlign
	.local LowerAsm
	.local LowerLocation
	.local LowerBuiltinVaStart
	.local LowerBuiltinVaArg
	.local LowerBuiltinVaEnd
	.local LowerBuiltinVaCopy
	.local LowerStackPointerOps
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
	mv          s1, a2
	mv          s2, a0
	mv          s3, a1
	ld          a0, 96(s1)
	beq         a0, x0, .LowerIRNode_label_79

	// *** Basic block 1

.LowerIRNode_label_76:
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

.LowerIRNode_label_79:
	lw          t0, 20(s1)
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 3

	j           .LowerIRNode_label_228

	// *** Basic block 4

	j           .LowerIRNode_label_548

	// *** Basic block 5

	j           .LowerIRNode_label_352

	// *** Basic block 6

	j           .LowerIRNode_label_367

	// *** Basic block 7

	j           .LowerIRNode_label_335

	// *** Basic block 8

	j           .LowerIRNode_label_383

	// *** Basic block 9

	j           .LowerIRNode_label_399

	// *** Basic block 10

	j           .LowerIRNode_label_416

	// *** Basic block 11

	j           .LowerIRNode_label_336

	// *** Basic block 12

	j           .LowerIRNode_label_544

	// *** Basic block 13

	j           .LowerIRNode_label_545

	// *** Basic block 14

	j           .LowerIRNode_label_546

	// *** Basic block 15

	j           .LowerIRNode_label_547

	// *** Basic block 16

	j           .LowerIRNode_label_558

	// *** Basic block 17

	j           .LowerIRNode_label_559

	// *** Basic block 18

	j           .LowerIRNode_label_560

	// *** Basic block 19

	j           .LowerIRNode_label_561

	// *** Basic block 20

	j           .LowerIRNode_label_635

	// *** Basic block 21

	j           .LowerIRNode_label_645

	// *** Basic block 22

	j           .LowerIRNode_label_476

	// *** Basic block 23

	j           .LowerIRNode_label_477

	// *** Basic block 24

	j           .LowerIRNode_label_478

	// *** Basic block 25

	j           .LowerIRNode_label_479

	// *** Basic block 26

	j           .LowerIRNode_label_480

	// *** Basic block 27

	j           .LowerIRNode_label_481

	// *** Basic block 28

	j           .LowerIRNode_label_482

	// *** Basic block 29

	j           .LowerIRNode_label_483

	// *** Basic block 30

	j           .LowerIRNode_label_484

	// *** Basic block 31

	j           .LowerIRNode_label_485

	// *** Basic block 32

	j           .LowerIRNode_label_665

	// *** Basic block 33

	j           .LowerIRNode_label_495

	// *** Basic block 34

	j           .LowerIRNode_label_496

	// *** Basic block 35

	j           .LowerIRNode_label_497

	// *** Basic block 36

	j           .LowerIRNode_label_498

	// *** Basic block 37

	j           .LowerIRNode_label_499

	// *** Basic block 38

	j           .LowerIRNode_label_500

	// *** Basic block 39

	j           .LowerIRNode_label_501

	// *** Basic block 40

	j           .LowerIRNode_label_511

	// *** Basic block 41

	j           .LowerIRNode_label_512

	// *** Basic block 42

	j           .LowerIRNode_label_513

	// *** Basic block 43

	j           .LowerIRNode_label_514

	// *** Basic block 44

	j           .LowerIRNode_label_515

	// *** Basic block 45

	j           .LowerIRNode_label_516

	// *** Basic block 46

	j           .LowerIRNode_label_517

	// *** Basic block 47

	j           .LowerIRNode_label_518

	// *** Basic block 48

	j           .LowerIRNode_label_519

	// *** Basic block 49

	j           .LowerIRNode_label_520

	// *** Basic block 50

	j           .LowerIRNode_label_521

	// *** Basic block 51

	j           .LowerIRNode_label_522

	// *** Basic block 52

	j           .LowerIRNode_label_523

	// *** Basic block 53

	j           .LowerIRNode_label_524

	// *** Basic block 54

	j           .LowerIRNode_label_525

	// *** Basic block 55

	j           .LowerIRNode_label_526

	// *** Basic block 56

	j           .LowerIRNode_label_527

	// *** Basic block 57

	j           .LowerIRNode_label_528

	// *** Basic block 58

	j           .LowerIRNode_label_529

	// *** Basic block 59

	j           .LowerIRNode_label_530

	// *** Basic block 60

	j           .LowerIRNode_label_531

	// *** Basic block 61

	j           .LowerIRNode_label_532

	// *** Basic block 62

	j           .LowerIRNode_label_533

	// *** Basic block 63

	j           .LowerIRNode_label_534

	// *** Basic block 64

	j           .LowerIRNode_label_535

	// *** Basic block 65

	j           .LowerIRNode_label_536

	// *** Basic block 66

	j           .LowerIRNode_label_537

	// *** Basic block 67

	j           .LowerIRNode_label_571

	// *** Basic block 68

	j           .LowerIRNode_label_572

	// *** Basic block 69

	j           .LowerIRNode_label_573

	// *** Basic block 70

	j           .LowerIRNode_label_574

	// *** Basic block 71

	j           .LowerIRNode_label_575

	// *** Basic block 72

	j           .LowerIRNode_label_576

	// *** Basic block 73

	j           .LowerIRNode_label_577

	// *** Basic block 74

	j           .LowerIRNode_label_578

	// *** Basic block 75

	j           .LowerIRNode_label_579

	// *** Basic block 76

	j           .LowerIRNode_label_580

	// *** Basic block 77

	j           .LowerIRNode_label_581

	// *** Basic block 78

	j           .LowerIRNode_label_582

	// *** Basic block 79

	j           .LowerIRNode_label_583

	// *** Basic block 80

	j           .LowerIRNode_label_584

	// *** Basic block 81

	j           .LowerIRNode_label_585

	// *** Basic block 82

	j           .LowerIRNode_label_586

	// *** Basic block 83

	j           .LowerIRNode_label_587

	// *** Basic block 84

	j           .LowerIRNode_label_588

	// *** Basic block 85

	j           .LowerIRNode_label_589

	// *** Basic block 86

	j           .LowerIRNode_label_590

	// *** Basic block 87

	j           .LowerIRNode_label_591

	// *** Basic block 88

	j           .LowerIRNode_label_592

	// *** Basic block 89

	j           .LowerIRNode_label_593

	// *** Basic block 90

	j           .LowerIRNode_label_594

	// *** Basic block 91

	j           .LowerIRNode_label_604

	// *** Basic block 92

	j           .LowerIRNode_label_605

	// *** Basic block 93

	j           .LowerIRNode_label_615

	// *** Basic block 94

	j           .LowerIRNode_label_625

	// *** Basic block 95

	j           .LowerIRNode_label_655

	// *** Basic block 96

	j           .LowerIRNode_label_462

	// *** Basic block 97

	j           .LowerIRNode_label_432

	// *** Basic block 98

	j           .LowerIRNode_label_449

	// *** Basic block 99

	j           .LowerIRNode_label_220

	// *** Basic block 100

	j           .LowerIRNode_label_224

	// *** Basic block 101

	j           .LowerIRNode_label_221

	// *** Basic block 102

	j           .LowerIRNode_label_223

	// *** Basic block 103

	j           .LowerIRNode_label_222

	// *** Basic block 104

	j           .LowerIRNode_label_256

	// *** Basic block 105

	j           .LowerIRNode_label_277

	// *** Basic block 106

	j           .LowerIRNode_label_325

	// *** Basic block 107

	j           .LowerIRNode_label_315

	// *** Basic block 108

	j           .LowerIRNode_label_772

	// *** Basic block 109

	j           .LowerIRNode_label_682

	// *** Basic block 110

	j           .LowerIRNode_label_683

	// *** Basic block 111

	j           .LowerIRNode_label_684

	// *** Basic block 112

	j           .LowerIRNode_label_685

	// *** Basic block 113

	j           .LowerIRNode_label_538

	// *** Basic block 114

	j           .LowerIRNode_label_539

	// *** Basic block 115

	j           .LowerIRNode_label_540

	// *** Basic block 116

	j           .LowerIRNode_label_541

	// *** Basic block 117

	j           .LowerIRNode_label_542

	// *** Basic block 118

	j           .LowerIRNode_label_543

	// *** Basic block 119

	j           .LowerIRNode_label_732

	// *** Basic block 120

	j           .LowerIRNode_label_742

	// *** Basic block 121

	j           .LowerIRNode_label_752

	// *** Basic block 122

	j           .LowerIRNode_label_695

	// *** Basic block 123

	j           .LowerIRNode_label_705

	// *** Basic block 124

	j           .LowerIRNode_label_715

	// *** Basic block 125

	j           .LowerIRNode_label_257

	// *** Basic block 126

	j           .LowerIRNode_label_762

	// *** Basic block 127

	j           .LowerIRNode_label_233

	// *** Basic block 128

	j           .LowerIRNode_label_822

	// *** Basic block 129

	j           .LowerIRNode_label_823

	// *** Basic block 130

	j           .LowerIRNode_label_824

	// *** Basic block 131

	j           .LowerIRNode_label_782

	// *** Basic block 132

	j           .LowerIRNode_label_792

	// *** Basic block 133

	j           .LowerIRNode_label_802

	// *** Basic block 134

	j           .LowerIRNode_label_812

	// *** Basic block 135

	j           .LowerIRNode_label_229

	// *** Basic block 136

.LowerIRNode_label_220:

	// *** Basic block 137

.LowerIRNode_label_221:

	// *** Basic block 138

.LowerIRNode_label_222:

	// *** Basic block 139

.LowerIRNode_label_223:

	// *** Basic block 140

.LowerIRNode_label_224:
	mv          a0, x0
	j           .LowerIRNode_label_76

	// *** Basic block 141

.LowerIRNode_label_228:

	// *** Basic block 142

.LowerIRNode_label_229:
	mv          a0, x0
	j           .LowerIRNode_label_76

	// *** Basic block 143

.LowerIRNode_label_233:
	ld          t0, 24(s1)
	ld          a1, 0(t0)
	mv          a0, s2
	call        Materialize

	// *** Basic block 144

	mv          a1, a0
	li          t0, 206		// 0xce ASCII \xce
	mv          a0, t0
	call        NewInstruction1

	// *** Basic block 145

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

	// *** Basic block 147

.LowerIRNode_label_256:

	// *** Basic block 148

.LowerIRNode_label_257:

	// *** Basic block 149

.LowerIRNode_label_258:
	lla         a0, .str.282
	lla         a1, .str.283
	lla         a3, .str.284
	li          t0, 3416		// 0xd58
	mv          a2, t0
	call        printf

	// *** Basic block 150

	call        abort

	// *** Basic block 151

	mv          a0, x0
	j           .LowerIRNode_label_76

	// *** Basic block 152

.LowerIRNode_label_277:
	li          t0, 29		// 0x1d ASCII \x1d
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 153

	mv          a1, a0
	mv          a0, s2
	call        EmitSymbol

	// *** Basic block 154

	mv          a1, a0
	mv          a0, s1
	call        SetLoweredNode

	// *** Basic block 155

	mv          s4, a0
	mv          a1, x0
	mv          a0, s2
	call        IntArgumentRegister

	// *** Basic block 156

	mv          a2, a0
	mv          a1, s4
	li          t0, 18		// 0x12 ASCII \x12
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 157

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 158

	mv          a0, s4
	j           .LowerIRNode_label_76

	// *** Basic block 159

.LowerIRNode_label_315:
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

	// *** Basic block 161

.LowerIRNode_label_325:
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

	// *** Basic block 163

.LowerIRNode_label_335:

	// *** Basic block 164

.LowerIRNode_label_336:
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

	// *** Basic block 166

.LowerIRNode_label_352:
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

	// *** Basic block 168

.LowerIRNode_label_367:
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

	// *** Basic block 170

.LowerIRNode_label_383:
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

	// *** Basic block 172

.LowerIRNode_label_399:
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

	// *** Basic block 174

.LowerIRNode_label_416:
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

	// *** Basic block 176

.LowerIRNode_label_432:
	mv          a0, x0
	call        NewInstruction

	// *** Basic block 177

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 178

	mv          a1, s3
	mv          a0, s2
	call        LowerVariables

	// *** Basic block 179

	mv          a0, x0
	j           .LowerIRNode_label_76

	// *** Basic block 180

.LowerIRNode_label_449:
	li          t0, 1		// 0x1 ASCII \x1
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 181

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 182

	mv          a0, x0
	j           .LowerIRNode_label_76

	// *** Basic block 183

.LowerIRNode_label_462:
	li          t0, 21		// 0x15 ASCII \x15
	mv          a0, t0
	call        NewInstruction

	// *** Basic block 184

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

	// *** Basic block 186

.LowerIRNode_label_476:

	// *** Basic block 187

.LowerIRNode_label_477:

	// *** Basic block 188

.LowerIRNode_label_478:

	// *** Basic block 189

.LowerIRNode_label_479:

	// *** Basic block 190

.LowerIRNode_label_480:

	// *** Basic block 191

.LowerIRNode_label_481:

	// *** Basic block 192

.LowerIRNode_label_482:

	// *** Basic block 193

.LowerIRNode_label_483:

	// *** Basic block 194

.LowerIRNode_label_484:

	// *** Basic block 195

.LowerIRNode_label_485:
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

	// *** Basic block 197

.LowerIRNode_label_495:

	// *** Basic block 198

.LowerIRNode_label_496:

	// *** Basic block 199

.LowerIRNode_label_497:

	// *** Basic block 200

.LowerIRNode_label_498:

	// *** Basic block 201

.LowerIRNode_label_499:

	// *** Basic block 202

.LowerIRNode_label_500:

	// *** Basic block 203

.LowerIRNode_label_501:
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

	// *** Basic block 205

.LowerIRNode_label_511:

	// *** Basic block 206

.LowerIRNode_label_512:

	// *** Basic block 207

.LowerIRNode_label_513:

	// *** Basic block 208

.LowerIRNode_label_514:

	// *** Basic block 209

.LowerIRNode_label_515:

	// *** Basic block 210

.LowerIRNode_label_516:

	// *** Basic block 211

.LowerIRNode_label_517:

	// *** Basic block 212

.LowerIRNode_label_518:

	// *** Basic block 213

.LowerIRNode_label_519:

	// *** Basic block 214

.LowerIRNode_label_520:

	// *** Basic block 215

.LowerIRNode_label_521:

	// *** Basic block 216

.LowerIRNode_label_522:

	// *** Basic block 217

.LowerIRNode_label_523:

	// *** Basic block 218

.LowerIRNode_label_524:

	// *** Basic block 219

.LowerIRNode_label_525:

	// *** Basic block 220

.LowerIRNode_label_526:

	// *** Basic block 221

.LowerIRNode_label_527:

	// *** Basic block 222

.LowerIRNode_label_528:

	// *** Basic block 223

.LowerIRNode_label_529:

	// *** Basic block 224

.LowerIRNode_label_530:

	// *** Basic block 225

.LowerIRNode_label_531:

	// *** Basic block 226

.LowerIRNode_label_532:

	// *** Basic block 227

.LowerIRNode_label_533:

	// *** Basic block 228

.LowerIRNode_label_534:

	// *** Basic block 229

.LowerIRNode_label_535:

	// *** Basic block 230

.LowerIRNode_label_536:

	// *** Basic block 231

.LowerIRNode_label_537:

	// *** Basic block 232

.LowerIRNode_label_538:

	// *** Basic block 233

.LowerIRNode_label_539:

	// *** Basic block 234

.LowerIRNode_label_540:

	// *** Basic block 235

.LowerIRNode_label_541:

	// *** Basic block 236

.LowerIRNode_label_542:

	// *** Basic block 237

.LowerIRNode_label_543:

	// *** Basic block 238

.LowerIRNode_label_544:

	// *** Basic block 239

.LowerIRNode_label_545:

	// *** Basic block 240

.LowerIRNode_label_546:

	// *** Basic block 241

.LowerIRNode_label_547:

	// *** Basic block 242

.LowerIRNode_label_548:
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

	// *** Basic block 244

.LowerIRNode_label_558:

	// *** Basic block 245

.LowerIRNode_label_559:

	// *** Basic block 246

.LowerIRNode_label_560:

	// *** Basic block 247

.LowerIRNode_label_561:
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
	j           LowerRmov

	// *** Basic block 249

.LowerIRNode_label_571:

	// *** Basic block 250

.LowerIRNode_label_572:

	// *** Basic block 251

.LowerIRNode_label_573:

	// *** Basic block 252

.LowerIRNode_label_574:

	// *** Basic block 253

.LowerIRNode_label_575:

	// *** Basic block 254

.LowerIRNode_label_576:

	// *** Basic block 255

.LowerIRNode_label_577:

	// *** Basic block 256

.LowerIRNode_label_578:

	// *** Basic block 257

.LowerIRNode_label_579:

	// *** Basic block 258

.LowerIRNode_label_580:

	// *** Basic block 259

.LowerIRNode_label_581:

	// *** Basic block 260

.LowerIRNode_label_582:

	// *** Basic block 261

.LowerIRNode_label_583:

	// *** Basic block 262

.LowerIRNode_label_584:

	// *** Basic block 263

.LowerIRNode_label_585:

	// *** Basic block 264

.LowerIRNode_label_586:

	// *** Basic block 265

.LowerIRNode_label_587:

	// *** Basic block 266

.LowerIRNode_label_588:

	// *** Basic block 267

.LowerIRNode_label_589:

	// *** Basic block 268

.LowerIRNode_label_590:

	// *** Basic block 269

.LowerIRNode_label_591:

	// *** Basic block 270

.LowerIRNode_label_592:

	// *** Basic block 271

.LowerIRNode_label_593:

	// *** Basic block 272

.LowerIRNode_label_594:
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
	j           LowerComparison

	// *** Basic block 274

.LowerIRNode_label_604:

	// *** Basic block 275

.LowerIRNode_label_605:
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

	// *** Basic block 277

.LowerIRNode_label_615:
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

	// *** Basic block 279

.LowerIRNode_label_625:
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

	// *** Basic block 281

.LowerIRNode_label_635:
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

	// *** Basic block 283

.LowerIRNode_label_645:
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

	// *** Basic block 285

.LowerIRNode_label_655:
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

	// *** Basic block 287

.LowerIRNode_label_665:
	ld          t0, 24(s1)
	ld          a1, 0(t0)
	mv          a0, s2
	call        Materialize

	// *** Basic block 288

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

	// *** Basic block 290

.LowerIRNode_label_682:

	// *** Basic block 291

.LowerIRNode_label_683:

	// *** Basic block 292

.LowerIRNode_label_684:

	// *** Basic block 293

.LowerIRNode_label_685:
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

	// *** Basic block 295

.LowerIRNode_label_695:
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

	// *** Basic block 297

.LowerIRNode_label_705:
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

	// *** Basic block 299

.LowerIRNode_label_715:
	ld          t0, 24(s1)
	ld          a1, 0(t0)
	mv          a0, s2
	call        Materialize

	// *** Basic block 300

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

	// *** Basic block 302

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
	j           LowerZeroExtend

	// *** Basic block 304

.LowerIRNode_label_742:
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

	// *** Basic block 306

.LowerIRNode_label_752:
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

	// *** Basic block 308

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
	j           LowerAsm

	// *** Basic block 310

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
	j           LowerLocation

	// *** Basic block 312

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
	j           LowerBuiltinVaStart

	// *** Basic block 314

.LowerIRNode_label_792:
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

	// *** Basic block 316

.LowerIRNode_label_802:
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

	// *** Basic block 318

.LowerIRNode_label_812:
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

	// *** Basic block 320

.LowerIRNode_label_822:

	// *** Basic block 321

.LowerIRNode_label_823:

	// *** Basic block 322

.LowerIRNode_label_824:
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
	j           LowerStackPointerOps
.func_end_LowerIRNode:
	.size LowerIRNode, .func_end_LowerIRNode-LowerIRNode

	.local  CalculateArgumentSize
	.type CalculateArgumentSize, @function

CalculateArgumentSize:

	// *** Basic block 0

	.global TypeIsFloatingPoint
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
	ld          s1, 80(t0)
	mv          a0, s1
	call        TypeIsFloatingPoint

	// *** Basic block 1

	beqz        a0, .CalculateArgumentSize_label_29

	// *** Basic block 2

	li          a0, 8		// 0x8 ASCII \x8

	// *** Basic block 3

.CalculateArgumentSize_label_26:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 4

.CalculateArgumentSize_label_29:
	lw          s3, 16(s1)
	addi        t0, s3, -1
	seqz        s2, t0
	li          t0, 1		// 0x1 ASCII \x1
	beq         s3, t0, .CalculateArgumentSize_label_42

	// *** Basic block 5

	addi        t0, s3, -2
	seqz        s2, t0

	// *** Basic block 6

.CalculateArgumentSize_label_42:

	// *** Basic block 7

.CalculateArgumentSize_label_44:
	beqz        s2, .CalculateArgumentSize_label_51

	// *** Basic block 8

	j           .CalculateArgumentSize_label_47

	// *** Basic block 9

.CalculateArgumentSize_label_47:
	li          a0, 8		// 0x8 ASCII \x8
	j           .CalculateArgumentSize_label_26

	// *** Basic block 10

.CalculateArgumentSize_label_51:
	mv          a0, s1
	call        TypeIsStructOrUnion

	// *** Basic block 11

	beqz        a0, .CalculateArgumentSize_label_62

	// *** Basic block 12

	ld          t0, 32(s1)
	lw          a0, 76(t0)
	j           .CalculateArgumentSize_label_26

	// *** Basic block 13

.CalculateArgumentSize_label_62:
	lw          a0, 20(s1)
	li          t0, 4		// 0x4 ASCII \x4
	bge         a0, t0, .CalculateArgumentSize_label_71

	// *** Basic block 14

	li          a0, 4		// 0x4 ASCII \x4
	j           .CalculateArgumentSize_label_72

	// *** Basic block 15

.CalculateArgumentSize_label_71:

	// *** Basic block 16

.CalculateArgumentSize_label_72:
	j           .CalculateArgumentSize_label_26
.func_end_CalculateArgumentSize:
	.size CalculateArgumentSize, .func_end_CalculateArgumentSize-CalculateArgumentSize

	.local  CompareRegisterVar
	.type CompareRegisterVar, @function

CompareRegisterVar:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	ld          t0, 0(a0)
	ld          t1, 0(a1)
	ld          t2, 8(t0)
	ld          t0, 8(t1)
	addi        t1, t2, 64
	lw          t2, 8(t1)
	lw          t1, 4(t1)
	addi        t1, t1, 1
	mul         t3, t2, t1
	addi        t0, t0, 64
	lw          t1, 8(t0)
	lw          t0, 4(t0)
	addi        t0, t0, 1
	mul         t2, t1, t0
	sub         a0, t2, t3

	// *** Basic block 1

.CompareRegisterVar_label_42:
	ret         
.func_end_CompareRegisterVar:
	.size CompareRegisterVar, .func_end_CompareRegisterVar-CompareRegisterVar

	.local  ArgumentLocation
	.type ArgumentLocation, @function

ArgumentLocation:

	// *** Basic block 0

	.global TypeIsStructOrUnion
	.global compiler
	.global TypeIsFloatingPoint
	.global printf
	.global abort
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
	mv          s1, a2
	mv          s2, a0
	mv          s2, a0
	ld          t0, 16(a1)
	ld          t0, 136(t0)
	lw          s3, 112(t0)
	la          t0, compiler
	ld          t0, 0(t0)
	ld          t0, 1096(t0)
	ld          t0, 32(t0)
	ld          t0, 40(t0)
	ld          a0, 24(t0)
	call        TypeIsStructOrUnion

	// *** Basic block 1

	ld          s4, 80(t0)
	mv          s5, a0
	li          s6, 10		// 0xa ASCII \xa
	beqz        s5, .ArgumentLocation_label_71

	// *** Basic block 2

	li          s6, 11		// 0xb ASCII \xb

	// *** Basic block 3

.ArgumentLocation_label_71:
	li          s5, 10		// 0xa ASCII \xa
	mv          s7, x0
	mv          s8, x0
	mv          s9, x0
	ld          s10, 8(s1)
	bge         x0, s10, .ArgumentLocation_label_190

	// *** Basic block 4

	ld          s11, 0(s1)

	// *** Basic block 5

.ArgumentLocation_label_85:
	bne         s9, s3, .ArgumentLocation_label_129

	// *** Basic block 6

	mv          a0, s4
	call        TypeIsFloatingPoint

	// *** Basic block 7

	beqz        a0, .ArgumentLocation_label_109

	// *** Basic block 8

	li          t0, 17		// 0x11 ASCII \x11
	bge         s5, t0, .ArgumentLocation_label_102

	// *** Basic block 9

	sw          x0, 0(s2)
	sd          s5, 8(s2)
	j           .ArgumentLocation_label_107

	// *** Basic block 10

.ArgumentLocation_label_102:
	li          t0, 1		// 0x1 ASCII \x1
	sw          t0, 0(s2)
	sd          s8, 8(s2)

	// *** Basic block 11

.ArgumentLocation_label_107:
	j           .ArgumentLocation_label_125

	// *** Basic block 12

.ArgumentLocation_label_109:
	li          t0, 17		// 0x11 ASCII \x11
	bge         s6, t0, .ArgumentLocation_label_119

	// *** Basic block 13

	sw          x0, 0(s2)
	sd          s6, 8(s2)
	j           .ArgumentLocation_label_124

	// *** Basic block 14

.ArgumentLocation_label_119:
	li          t0, 1		// 0x1 ASCII \x1
	sw          t0, 0(s2)
	sd          s8, 8(s2)

	// *** Basic block 15

.ArgumentLocation_label_124:

	// *** Basic block 16

.ArgumentLocation_label_125:

	// *** Basic block 17

.ArgumentLocation_label_126:
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

	// *** Basic block 18

.ArgumentLocation_label_129:
	slli        t0, s9, 3
	add         t0, s11, t0
	ld          s1, 0(t0)
	ld          s1, 40(s1)
	mv          a0, s1
	call        TypeIsFloatingPoint

	// *** Basic block 19

	beqz        a0, .ArgumentLocation_label_155

	// *** Basic block 20

	li          t0, 27		// 0x1b ASCII \x1b
	bge         s5, t0, .ArgumentLocation_label_148

	// *** Basic block 21

	addi        s5, s5, 1
	j           .ArgumentLocation_label_153

	// *** Basic block 22

.ArgumentLocation_label_148:
	mv          s8, s7
	lw          t0, 20(s1)
	add         s7, s7, t0

	// *** Basic block 23

.ArgumentLocation_label_153:
	j           .ArgumentLocation_label_185

	// *** Basic block 24

.ArgumentLocation_label_155:
	li          t0, 27		// 0x1b ASCII \x1b
	bge         s6, t0, .ArgumentLocation_label_163

	// *** Basic block 25

	addi        s6, s6, 1
	j           .ArgumentLocation_label_184

	// *** Basic block 26

.ArgumentLocation_label_163:
	mv          s8, s7
	mv          a0, s1
	call        TypeIsStructOrUnion

	// *** Basic block 27

	beqz        a0, .ArgumentLocation_label_171

	// *** Basic block 28

	addi        s7, s7, 8
	j           .ArgumentLocation_label_183

	// *** Basic block 29

.ArgumentLocation_label_171:
	lw          s3, 20(s1)
	li          t0, 8		// 0x8 ASCII \x8
	bge         s3, t0, .ArgumentLocation_label_180

	// *** Basic block 30

	addi        s7, s7, 4
	j           .ArgumentLocation_label_182

	// *** Basic block 31

.ArgumentLocation_label_180:
	addi        s7, s7, 8

	// *** Basic block 32

.ArgumentLocation_label_182:

	// *** Basic block 33

.ArgumentLocation_label_183:

	// *** Basic block 34

.ArgumentLocation_label_184:

	// *** Basic block 35

.ArgumentLocation_label_185:

	// *** Basic block 36

.ArgumentLocation_label_186:
	addi        s9, s9, 1
	bge         s9, s10, .ArgumentLocation_label_85

	// *** Basic block 37

.ArgumentLocation_label_190:
	lla         a0, .str.285
	lla         a1, .str.286
	lla         a3, .str.287
	li          t0, 3521		// 0xdc1
	mv          a2, t0
	call        printf

	// *** Basic block 38

	call        abort

	// *** Basic block 39

	sd          x0, 0(s2)
	sd          x0, 8(s2)
	sd          x0, 16(s2)
	sw          x0, 0(s2)
	j           .ArgumentLocation_label_126
.func_end_ArgumentLocation:
	.size ArgumentLocation, .func_end_ArgumentLocation-ArgumentLocation

	.local  AlignOffset
	.type AlignOffset, @function

AlignOffset:

	// *** Basic block 0

	.global TypeRecordAlignment
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	ld          t1, 16(t0)
	ld          a0, 80(t1)
	j           TypeRecordAlignment
.func_end_AlignOffset:
	.size AlignOffset, .func_end_AlignOffset-AlignOffset

	.local  SetDebugRegisterLocation
	.type SetDebugRegisterLocation, @function

SetDebugRegisterLocation:

	// *** Basic block 0

	.global VariableDIESetRegister
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	ld          t1, 8(t0)
	ld          a0, 128(t1)
	j           VariableDIESetRegister
.func_end_SetDebugRegisterLocation:
	.size SetDebugRegisterLocation, .func_end_SetDebugRegisterLocation-SetDebugRegisterLocation

	.local  SetDebugStackLocation
	.type SetDebugStackLocation, @function

SetDebugStackLocation:

	// *** Basic block 0

	.global VariableDIESetStackOffset
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	ld          t1, 8(t0)
	ld          a0, 128(t1)
	j           VariableDIESetStackOffset
.func_end_SetDebugStackLocation:
	.size SetDebugStackLocation, .func_end_SetDebugStackLocation-SetDebugStackLocation

	.local  SetDebugSymbolLocation
	.type SetDebugSymbolLocation, @function

SetDebugSymbolLocation:

	// *** Basic block 0

	.global VariableDIESetStatic
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	ld          t1, 8(t0)
	ld          a0, 128(t1)
	ld          a1, 16(t1)
	j           VariableDIESetStatic
.func_end_SetDebugSymbolLocation:
	.size SetDebugSymbolLocation, .func_end_SetDebugSymbolLocation-SetDebugSymbolLocation

	.local  LoadFpArgumentIntoRegisterVariable
	.type LoadFpArgumentIntoRegisterVariable, @function

LoadFpArgumentIntoRegisterVariable:

	// *** Basic block 0

	.local FloatingPointVariableRegister
	.global TypeIsDouble
	.local Emit
	.local NewInstruction2
	.local FloatingPointArgumentRegister
	.local PopArg
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
	mv          s1, a3
	mv          s2, a0
	mv          s3, a1
	li          t0, 1269346304		// 0x4ba8b000
	add         s4, s0, t0
	lw          t0, 1968(s4)
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 1

	j           .LoadFpArgumentIntoRegisterVariable_label_43

	// *** Basic block 2

	j           .LoadFpArgumentIntoRegisterVariable_label_97

	// *** Basic block 3

	j           .LoadFpArgumentIntoRegisterVariable_label_44

	// *** Basic block 4

	j           .LoadFpArgumentIntoRegisterVariable_label_98

	// *** Basic block 5

.LoadFpArgumentIntoRegisterVariable_label_43:

	// *** Basic block 6

.LoadFpArgumentIntoRegisterVariable_label_44:
	mv          s5, s1
	ld          a2, 136(s5)
	mv          a1, s3
	mv          a0, s2
	call        FloatingPointVariableRegister

	// *** Basic block 7

	mv          s6, a0
	ld          a0, 80(s1)
	call        TypeIsDouble

	// *** Basic block 8

	beqz        a0, .LoadFpArgumentIntoRegisterVariable_label_67

	// *** Basic block 9

	li          s7, 20		// 0x14 ASCII \x14
	j           .LoadFpArgumentIntoRegisterVariable_label_69

	// *** Basic block 10

.LoadFpArgumentIntoRegisterVariable_label_67:
	li          s7, 19		// 0x13 ASCII \x13

	// *** Basic block 11

.LoadFpArgumentIntoRegisterVariable_label_69:
	addi        t0, s4, 1968
	ld          t0, 8(t0)
	addi        a1, t0, -10
	mv          a0, s2
	call        FloatingPointArgumentRegister

	// *** Basic block 12

	mv          a2, a0
	mv          a1, s6
	mv          a0, s7
	call        NewInstruction2

	// *** Basic block 13

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 14

	mv          a0, s6

	// *** Basic block 15

.LoadFpArgumentIntoRegisterVariable_label_94:
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

	// *** Basic block 16

.LoadFpArgumentIntoRegisterVariable_label_97:

	// *** Basic block 17

.LoadFpArgumentIntoRegisterVariable_label_98:
	addi        t0, s4, 1968
	ld          a2, 8(t0)
	mv          a1, s1
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
	j           PopArg
.func_end_LoadFpArgumentIntoRegisterVariable:
	.size LoadFpArgumentIntoRegisterVariable, .func_end_LoadFpArgumentIntoRegisterVariable-LoadFpArgumentIntoRegisterVariable

	.local  LoadIntArgumentIntoRegisterVariable
	.type LoadIntArgumentIntoRegisterVariable, @function

LoadIntArgumentIntoRegisterVariable:

	// *** Basic block 0

	.local IntVariableRegister
	.local Emit
	.local NewInstruction2
	.local IntArgumentRegister
	.local PopArg
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
	mv          s1, a3
	mv          s2, a0
	mv          s3, a1
	lw          t0, 0(s0)
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 1

	j           .LoadIntArgumentIntoRegisterVariable_label_36

	// *** Basic block 2

	j           .LoadIntArgumentIntoRegisterVariable_label_79

	// *** Basic block 3

	j           .LoadIntArgumentIntoRegisterVariable_label_37

	// *** Basic block 4

	j           .LoadIntArgumentIntoRegisterVariable_label_78

	// *** Basic block 5

.LoadIntArgumentIntoRegisterVariable_label_36:

	// *** Basic block 6

.LoadIntArgumentIntoRegisterVariable_label_37:
	mv          s4, s1
	ld          a2, 136(s4)
	mv          a1, s3
	mv          a0, s2
	call        IntVariableRegister

	// *** Basic block 7

	mv          s5, a0
	addi        t0, s0, 0
	ld          t0, 8(t0)
	addi        a1, t0, -10
	mv          a0, s2
	call        IntArgumentRegister

	// *** Basic block 8

	mv          a2, a0
	mv          a1, s5
	li          t0, 18		// 0x12 ASCII \x12
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 9

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 10

	mv          a0, s5

	// *** Basic block 11

.LoadIntArgumentIntoRegisterVariable_label_75:
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

	// *** Basic block 12

.LoadIntArgumentIntoRegisterVariable_label_78:

	// *** Basic block 13

.LoadIntArgumentIntoRegisterVariable_label_79:
	addi        t0, s0, 0
	ld          a2, 8(t0)
	mv          a1, s1
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
	j           PopArg
.func_end_LoadIntArgumentIntoRegisterVariable:
	.size LoadIntArgumentIntoRegisterVariable, .func_end_LoadIntArgumentIntoRegisterVariable-LoadIntArgumentIntoRegisterVariable

	.local  AssignRegisterOrOffset
	.type AssignRegisterOrOffset, @function

AssignRegisterOrOffset:

	// *** Basic block 0

	.local CalculateArgumentSize
	.global printf
	.global abort
	.global TypeIsFloatingPoint
	.local UseRegisterForVariable
	.local SetDebugRegisterLocation
	.local ArgumentLocation
	.local LoadFpArgumentIntoRegisterVariable
	.local AlignOffset
	.local SetDebugStackLocation
	.global TypeIsStructOrUnion
	.local LoadIntArgumentIntoRegisterVariable
	.local IntVariableRegister
	.local Emit
	.local NewInstruction2
	.local IntArgumentRegister
	.local GetSymbol
	.local SetDebugSymbolLocation
	.local NewSavedArgumentRegister
	.global VectorAppend
	addi sp, sp, -320
	// Saved return address (offset 312) and frame pointer (offset 304)
	sd ra, 312(sp)
	sd s0, 304(sp)
	addi s0, sp, 320
	// Local vars at offset -208(s0)
	// Spilled register region: 16 bytes at -224(s0) to -208(s0)
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
	mv          s3, a2
	mv          s4, a3
	ld          s5, 16(s1)
	sd          s5, -216(s0)	// Spilled @66
	ld          s6, 80(s5)
	lw          s7, 16(s6)
	addi        t1, s7, -2
	seqz        t0, t1
	li          s8, 2		// 0x2 ASCII \x2
	sd          s8, -216(s0)	// Spilled @77
	beq         s7, s8, .AssignRegisterOrOffset_label_82

	// *** Basic block 1

	addi        t1, s7, -1
	seqz        t0, t1

	// *** Basic block 2

.AssignRegisterOrOffset_label_82:
	beqz        t0, .AssignRegisterOrOffset_label_89

	// *** Basic block 3

	addi        t1, s6, 32
	lb          t1, 16(t1)
	slli        t1, t1, 61
	srai        t0, t1, 63

	// *** Basic block 4

.AssignRegisterOrOffset_label_89:

	// *** Basic block 5

.AssignRegisterOrOffset_label_91:
	beqz        t0, .AssignRegisterOrOffset_label_97

	// *** Basic block 6

	j           .AssignRegisterOrOffset_label_94

	// *** Basic block 7

.AssignRegisterOrOffset_label_94:
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

	// *** Basic block 8

.AssignRegisterOrOffset_label_97:
	lw          t0, 20(s5)
	addi        t0, t0, -98
	seqz        s7, t0
	beqz        s7, .AssignRegisterOrOffset_label_112

	// *** Basic block 9

	mv          a0, s5
	call        CalculateArgumentSize

	// *** Basic block 10

	j           .AssignRegisterOrOffset_label_115

	// *** Basic block 11

.AssignRegisterOrOffset_label_112:
	lw          s9, 20(s6)

	// *** Basic block 12

.AssignRegisterOrOffset_label_115:
	beqz        s9, .AssignRegisterOrOffset_label_120

	// *** Basic block 13

	j           .AssignRegisterOrOffset_label_135

	// *** Basic block 14

.AssignRegisterOrOffset_label_120:
	lla         a0, .str.288
	lla         a1, .str.289
	lla         a3, .str.290
	li          t0, 3597		// 0xe0d
	mv          a2, t0
	call        printf

	// *** Basic block 15

	call        abort

	// *** Basic block 16

.AssignRegisterOrOffset_label_135:
	mv          a0, s6
	call        TypeIsFloatingPoint

	// *** Basic block 17

	beqz        a0, .AssignRegisterOrOffset_label_227

	// *** Basic block 18

	mv          a1, s5
	mv          a0, s2
	call        UseRegisterForVariable

	// *** Basic block 19

	beqz        a0, .AssignRegisterOrOffset_label_205

	// *** Basic block 20

	lw          t0, 196(s2)
	addi        t0, t0, 1
	sw          t0, 196(s2)
	mv          s10, t0
	ld          s11, 16(s1)
	addi        t0, s11, 96
	li          t1, 2147483648		// 0x80000000
	or          t1, t1, s10
	sw          t1, 8(t0)
	mv          a1, s10
	mv          a0, s1
	call        SetDebugRegisterLocation

	// *** Basic block 21

	beqz        s7, .AssignRegisterOrOffset_label_203

	// *** Basic block 22

	addi        t0, s0, -184
	mv          a2, s3
	mv          a1, s1
	mv          a0, t0
	call        ArgumentLocation

	// *** Basic block 23

	ld          t0, -184(s0)
	sd          t0, -208(s0)
	ld          t0, -176(s0)
	sd          t0, -200(s0)
	ld          t0, -168(s0)
	sd          t0, -192(s0)
	addi        sp, sp, -24
	ld          t0, -208(s0)
	sd          t0, 0(sp)
	ld          t0, -200(s0)
	sd          t0, 8(sp)
	ld          t0, -192(s0)
	sd          t0, 16(sp)
	mv          a3, s11
	addi        a2, sp, 0
	mv          a1, s10
	mv          a0, s2
	call        LoadFpArgumentIntoRegisterVariable

	// *** Basic block 24

	addi        sp, sp, 24

	// *** Basic block 25

.AssignRegisterOrOffset_label_203:
	j           .AssignRegisterOrOffset_label_225

	// *** Basic block 26

.AssignRegisterOrOffset_label_205:
	mv          a1, s4
	mv          a0, s1
	call        AlignOffset

	// *** Basic block 27

	ld          t0, 16(s1)
	addi        t0, t0, 96
	lw          s11, 0(s4)
	sw          s11, 8(t0)
	mv          a1, s11
	mv          a0, s1
	call        SetDebugStackLocation

	// *** Basic block 28

	sext.w      t0, s9
	add         t0, s11, t0
	sw          t0, 0(s4)

	// *** Basic block 29

.AssignRegisterOrOffset_label_225:
	j           .AssignRegisterOrOffset_label_645

	// *** Basic block 30

.AssignRegisterOrOffset_label_227:
	mv          a0, s6
	call        TypeIsStructOrUnion

	// *** Basic block 31

	beqz        a0, .AssignRegisterOrOffset_label_396

	// *** Basic block 32

	beqz        s7, .AssignRegisterOrOffset_label_324

	// *** Basic block 33

	li          t0, 8		// 0x8 ASCII \x8
	bge         s9, t0, .AssignRegisterOrOffset_label_321

	// *** Basic block 34

	mv          a1, s5
	mv          a0, s2
	call        UseRegisterForVariable

	// *** Basic block 35

	beqz        a0, .AssignRegisterOrOffset_label_299

	// *** Basic block 36

	lw          t0, 192(s2)
	addi        t0, t0, 1
	sw          t0, 192(s2)
	mv          s11, t0
	ld          s8, 16(s1)
	addi        t0, s8, 96
	li          t1, 2147483648		// 0x80000000
	or          t1, t1, s11
	sw          t1, 8(t0)
	mv          a1, s11
	mv          a0, s1
	call        SetDebugRegisterLocation

	// *** Basic block 37

	addi        t0, s0, -136
	mv          a2, s3
	mv          a1, s1
	mv          a0, t0
	call        ArgumentLocation

	// *** Basic block 38

	ld          t0, -136(s0)
	sd          t0, -160(s0)
	ld          t0, -128(s0)
	sd          t0, -152(s0)
	ld          t0, -120(s0)
	sd          t0, -144(s0)
	addi        sp, sp, -24
	ld          t0, -160(s0)
	sd          t0, 0(sp)
	ld          t0, -152(s0)
	sd          t0, 8(sp)
	ld          t0, -144(s0)
	sd          t0, 16(sp)
	mv          a3, s8
	addi        a2, sp, 0
	mv          a1, s11
	mv          a0, s2
	call        LoadIntArgumentIntoRegisterVariable

	// *** Basic block 39

	j           .AssignRegisterOrOffset_label_319

	// *** Basic block 40

.AssignRegisterOrOffset_label_299:
	mv          a1, s4
	mv          a0, s1
	call        AlignOffset

	// *** Basic block 41

	ld          t0, 16(s1)
	addi        t0, t0, 96
	lw          s8, 0(s4)
	sw          s8, 8(t0)
	mv          a1, s8
	mv          a0, s1
	call        SetDebugStackLocation

	// *** Basic block 42

	sext.w      t0, s9
	add         t0, s8, t0
	sw          t0, 0(s4)

	// *** Basic block 43

.AssignRegisterOrOffset_label_319:
	j           .AssignRegisterOrOffset_label_322

	// *** Basic block 44

.AssignRegisterOrOffset_label_321:

	// *** Basic block 45

.AssignRegisterOrOffset_label_322:
	j           .AssignRegisterOrOffset_label_394

	// *** Basic block 46

.AssignRegisterOrOffset_label_324:
	lw          t0, 88(s5)
	andi        t0, t0, 32
	beqz        t0, .AssignRegisterOrOffset_label_373

	// *** Basic block 47

	ld          t0, 16(s1)
	addi        t0, t0, 96
	lw          s8, 200(s2)
	li          t1, 2147483648		// 0x80000000
	or          t1, t1, s8
	sw          t1, 8(t0)
	ld          a2, 8(s1)
	mv          a1, s8
	mv          a0, s2
	call        IntVariableRegister
	sd          a0, -216(s0)	// Spilled @346

	// *** Basic block 48

	sd          a0, -216(s0)	// Spilled @347
	mv          a1, x0
	mv          a0, s2
	call        IntArgumentRegister

	// *** Basic block 49

	mv          a2, a0
	mv          a1, a0
	li          t0, 18		// 0x12 ASCII \x12
	mv          a0, t0
	call        NewInstruction2

	// *** Basic block 50

	mv          a1, a0
	mv          a0, s2
	call        Emit

	// *** Basic block 51

	mv          a1, s8
	mv          a0, s1
	call        SetDebugRegisterLocation

	// *** Basic block 52

	j           .AssignRegisterOrOffset_label_393

	// *** Basic block 53

.AssignRegisterOrOffset_label_373:
	mv          a1, s4
	mv          a0, s1
	call        AlignOffset

	// *** Basic block 54

	ld          t0, 16(s1)
	addi        t0, t0, 96
	lw          s8, 0(s4)
	sw          s8, 8(t0)
	mv          a1, s8
	mv          a0, s1
	call        SetDebugStackLocation

	// *** Basic block 55

	sext.w      t0, s9
	add         t0, s8, t0
	sw          t0, 0(s4)

	// *** Basic block 56

.AssignRegisterOrOffset_label_393:

	// *** Basic block 57

.AssignRegisterOrOffset_label_394:
	j           .AssignRegisterOrOffset_label_644

	// *** Basic block 58

.AssignRegisterOrOffset_label_396:
	mv          s8, s6
	lw          a0, 16(s8)
	addi        t0, a0, -2
	seqz        a0, t0
	ld          t0, -216(s0)	// Spilled @77
	beq         a0, t0, .AssignRegisterOrOffset_label_409

	// *** Basic block 59

	addi        t0, a0, -1
	seqz        a0, t0

	// *** Basic block 60

.AssignRegisterOrOffset_label_409:
	beqz        a0, .AssignRegisterOrOffset_label_416

	// *** Basic block 61

	addi        t0, s8, 32
	lb          t0, 16(t0)
	slli        t0, t0, 61
	srai        a0, t0, 63

	// *** Basic block 62

.AssignRegisterOrOffset_label_416:

	// *** Basic block 63

.AssignRegisterOrOffset_label_418:
	ld          t0, -224(s0)	// Spilled @417
	beqz        t0, .AssignRegisterOrOffset_label_423
	sd          a0, -224(s0)	// Spilled @417

	// *** Basic block 64

	j           .AssignRegisterOrOffset_label_421

	// *** Basic block 65

.AssignRegisterOrOffset_label_421:
	j           .AssignRegisterOrOffset_label_643

	// *** Basic block 66

.AssignRegisterOrOffset_label_423:
	mv          s5, s6
	lw          t1, 16(s5)
	addi        t1, t1, -2
	sd          t1, -224(s0)	// Spilled @429
	seqz        t1, t1

	// *** Basic block 67

.AssignRegisterOrOffset_label_432:
	mv          t0, t1
	beqz        t1, .AssignRegisterOrOffset_label_438

	// *** Basic block 68

	j           .AssignRegisterOrOffset_label_436

	// *** Basic block 69

.AssignRegisterOrOffset_label_436:
	not         t0, s7

	// *** Basic block 70

.AssignRegisterOrOffset_label_438:
	beqz        t0, .AssignRegisterOrOffset_label_460

	// *** Basic block 71

	mv          a1, s4
	mv          a0, s1
	call        AlignOffset

	// *** Basic block 72

	ld          t0, 16(s1)
	addi        t0, t0, 96
	sd          t0, -224(s0)	// Spilled @447
	lw          t0, 0(s4)
	ld          t1, -224(s0)	// Spilled @447
	sw          t0, 8(t1)
	mv          a1, t0
	mv          a0, s1
	call        SetDebugStackLocation

	// *** Basic block 73

	sext.w      t1, s9
	add         t0, t0, t1
	sw          t0, 0(s4)
	j           .AssignRegisterOrOffset_label_642

	// *** Basic block 74

.AssignRegisterOrOffset_label_460:
	mv          s5, s6
	lw          t0, 16(s5)
	addi        t0, t0, -3
	seqz        s6, t0

	// *** Basic block 75

.AssignRegisterOrOffset_label_469:
	beqz        s6, .AssignRegisterOrOffset_label_492

	// *** Basic block 76

	j           .AssignRegisterOrOffset_label_472

	// *** Basic block 77

.AssignRegisterOrOffset_label_472:
	ld          t0, -224(s0)	// Spilled @473
	ld          a2, 136(t0)
	sd          s5, -224(s0)	// Spilled @473
	mv          a1, x0
	mv          a0, s2
	call        GetSymbol

	// *** Basic block 78

	mv          s5, a0
	sd          s5, -224(s0)	// Spilled @482
	ld          t0, 16(s1)
	sd          s5, 96(t0)
	mv          a0, s1
	call        SetDebugSymbolLocation

	// *** Basic block 79

	j           .AssignRegisterOrOffset_label_641

	// *** Basic block 80

.AssignRegisterOrOffset_label_492:
	ld          a1, -216(s0)	// Spilled @66
	mv          a0, s2
	call        UseRegisterForVariable

	// *** Basic block 81

	beqz        a0, .AssignRegisterOrOffset_label_556

	// *** Basic block 82

	lw          t0, 192(s2)
	addi        t0, t0, 1
	sw          t0, 192(s2)
	mv          s6, t0
	ld          s5, 16(s1)
	addi        t0, s5, 96
	li          t1, 2147483648		// 0x80000000
	or          t1, t1, s6
	sw          t1, 8(t0)
	mv          a1, s6
	mv          a0, s1
	call        SetDebugRegisterLocation

	// *** Basic block 83

	beqz        s7, .AssignRegisterOrOffset_label_554

	// *** Basic block 84

	addi        t0, s0, -88
	mv          a2, s3
	mv          a1, s1
	mv          a0, t0
	call        ArgumentLocation

	// *** Basic block 85

	ld          t0, -88(s0)
	sd          t0, -112(s0)
	ld          t0, -80(s0)
	sd          t0, -104(s0)
	ld          t0, -72(s0)
	sd          t0, -96(s0)
	addi        sp, sp, -24
	ld          t0, -112(s0)
	sd          t0, 0(sp)
	ld          t0, -104(s0)
	sd          t0, 8(sp)
	ld          t0, -96(s0)
	sd          t0, 16(sp)
	mv          a3, s5
	addi        a2, sp, 0
	mv          a1, s6
	mv          a0, s2
	call        LoadIntArgumentIntoRegisterVariable

	// *** Basic block 87

.AssignRegisterOrOffset_label_554:
	j           .AssignRegisterOrOffset_label_640

	// *** Basic block 88

.AssignRegisterOrOffset_label_556:
	beqz        s7, .AssignRegisterOrOffset_label_619

	// *** Basic block 89

	addi        t0, s0, -40
	mv          a2, s3
	mv          a1, s1
	mv          a0, t0
	call        ArgumentLocation

	// *** Basic block 90

	ld          t0, -40(s0)
	sd          t0, -64(s0)
	ld          t0, -32(s0)
	sd          t0, -56(s0)
	ld          t0, -24(s0)
	sd          t0, -48(s0)
	lw          t0, -64(s0)
	bnez        t0, .AssignRegisterOrOffset_label_617

	// *** Basic block 91

	addi        t0, s2, 208
	ld          t0, 8(t0)
	slli        t0, t0, 3
	li          t1, -24		// 0xffffffffffffffe8
	sub         s5, t1, t0
	addi        t0, s0, -64
	ld          a0, 8(t0)
	mv          a2, s5
	li          t0, 8		// 0x8 ASCII \x8
	mv          a1, t0
	call        NewSavedArgumentRegister
	sd          a0, -224(s0)	// Spilled @595

	// *** Basic block 92

	mv          s7, a0
	ld          t0, 16(s1)
	addi        a0, t0, 96
	sw          s5, 8(a0)
	addi        a0, s2, 208
	mv          a1, s7
	call        VectorAppend

	// *** Basic block 93

	lw          t0, 184(s2)
	addi        t0, t0, 1
	sw          t0, 184(s2)
	lw          a1, 8(a0)
	mv          a0, s1
	call        SetDebugStackLocation

	// *** Basic block 94

.AssignRegisterOrOffset_label_617:
	j           .AssignRegisterOrOffset_label_639

	// *** Basic block 95

.AssignRegisterOrOffset_label_619:
	mv          a1, s4
	mv          a0, s1
	call        AlignOffset

	// *** Basic block 96

	ld          t0, 16(s1)
	addi        t0, t0, 96
	lw          s5, 0(s4)
	sw          s5, 8(t0)
	mv          a1, s5
	mv          a0, s1
	call        SetDebugStackLocation

	// *** Basic block 97

	sext.w      t0, s9
	add         t0, s5, t0
	sw          t0, 0(s4)

	// *** Basic block 98

.AssignRegisterOrOffset_label_639:

	// *** Basic block 99

.AssignRegisterOrOffset_label_640:

	// *** Basic block 100

.AssignRegisterOrOffset_label_641:

	// *** Basic block 101

.AssignRegisterOrOffset_label_642:

	// *** Basic block 102

.AssignRegisterOrOffset_label_643:

	// *** Basic block 103

.AssignRegisterOrOffset_label_644:

	// *** Basic block 104

.AssignRegisterOrOffset_label_645:
	j           .AssignRegisterOrOffset_label_94
.func_end_AssignRegisterOrOffset:
	.size AssignRegisterOrOffset, .func_end_AssignRegisterOrOffset-AssignRegisterOrOffset

	.local  AssignRegisterVars
	.type AssignRegisterVars, @function

AssignRegisterVars:

	// *** Basic block 0

	.local AssignRegisterOrOffset
	addi sp, sp, -80
	// Saved return address (offset 72) and frame pointer (offset 64)
	sd ra, 72(sp)
	sd s0, 64(sp)
	addi s0, sp, 80
	// Local vars at offset -32(s0)
	// Saved integer registers.
	sd s1, 40(sp)
	sd s2, 32(sp)
	sd s3, 24(sp)
	sd s4, 16(sp)
	sd s5, 8(sp)
	// End of stack frame
	mv          t0, a1
	mv          s1, a0
	mv          s2, a2
	sw          x0, -32(s0)
	mv          s3, x0
	ld          s4, 8(t0)
	bge         x0, s4, .AssignRegisterVars_label_52

	// *** Basic block 1

	ld          s5, 0(t0)

	// *** Basic block 2

.AssignRegisterVars_label_32:
	slli        t0, s3, 3
	add         t0, s5, t0
	ld          s5, 0(t0)
	addi        a3, s0, -32
	mv          a2, s2
	mv          a1, s5
	mv          a0, s1
	call        AssignRegisterOrOffset

	// *** Basic block 3

.AssignRegisterVars_label_48:
	addi        s3, s3, 1
	bge         s3, s4, .AssignRegisterVars_label_32

	// *** Basic block 4

.AssignRegisterVars_label_52:
	lw          t1, -32(s0)
	addi        t2, s1, 208
	ld          t2, 8(t2)
	slli        t2, t2, 3
	add         t1, t1, t2
	sw          t1, 128(s1)
	lw          t1, 128(s1)
	addi        t1, t1, 15
	andi        t1, t1, -16
	sw          t1, 128(s1)
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
.func_end_AssignRegisterVars:
	.size AssignRegisterVars, .func_end_AssignRegisterVars-AssignRegisterVars

	.local  LowerVariables
	.type LowerVariables, @function

LowerVariables:

	// *** Basic block 0

	.global VectorAppend
	.local GetSymbol
	.local AssignRegisterVars
	.global compiler
	.global VectorDestruct
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
	mv          s1, a0
	sd          x0, -48(s0)
	sd          x0, -40(s0)
	sd          x0, -32(s0)
	sd          x0, -48(s0)
	mv          s2, x0
	addi        t0, a1, 144
	ld          s3, 8(t0)
	bge         x0, s3, .LowerVariables_label_110

	// *** Basic block 1

	ld          t0, 144(a1)

	// *** Basic block 2

.LowerVariables_label_42:
	slli        t1, s2, 3
	add         t0, t0, t1
	ld          s4, 0(t0)
	ld          s5, 16(s4)
	lw          s6, 20(s5)
	li          t0, 96		// 0x60 ASCII '`'
	blt         s6, t0, .LowerVariables_label_87

	// *** Basic block 3

	li          t0, 100		// 0x64 ASCII 'd'
	blt         t0, s6, .LowerVariables_label_87

	// *** Basic block 4

	addi        t0, s6, -96
	slli        t0, t0, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 5

	j           .LowerVariables_label_72

	// *** Basic block 6

	j           .LowerVariables_label_87

	// *** Basic block 7

	j           .LowerVariables_label_80

	// *** Basic block 8

	j           .LowerVariables_label_87

	// *** Basic block 9

	j           .LowerVariables_label_73

	// *** Basic block 10

.LowerVariables_label_72:

	// *** Basic block 11

.LowerVariables_label_73:
	addi        a0, s0, -48
	mv          a1, s4
	call        VectorAppend

	// *** Basic block 12

	j           .LowerVariables_label_105

	// *** Basic block 13

.LowerVariables_label_80:
	addi        a0, s0, -48
	mv          a1, s4
	call        VectorAppend

	// *** Basic block 14

	j           .LowerVariables_label_105

	// *** Basic block 15

.LowerVariables_label_87:
	ld          a2, 136(s5)
	mv          a1, x0
	mv          a0, s1
	call        GetSymbol

	// *** Basic block 16

	mv          s5, a0
	ld          t0, 16(s4)
	sd          s5, 96(t0)
	j           .LowerVariables_label_105

	// *** Basic block 17

.LowerVariables_label_105:

	// *** Basic block 18

.LowerVariables_label_106:
	addi        s2, s2, 1
	bge         s2, s3, .LowerVariables_label_42

	// *** Basic block 19

.LowerVariables_label_110:
	addi        a1, s0, -48
	la          t0, compiler
	ld          t0, 0(t0)
	ld          t0, 1096(t0)
	addi        t0, t0, 32
	addi        a2, t0, 8
	mv          a0, s1
	call        AssignRegisterVars

	// *** Basic block 20

	addi        a0, s0, -48
	call        VectorDestruct

	// *** Basic block 21

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
.func_end_LowerVariables:
	.size LowerVariables, .func_end_LowerVariables-LowerVariables

	.global RVLower
	.type RVLower, @function

RVLower:

	// *** Basic block 0

	.local TrapLower
	.global TypeIsStructOrUnion
	.global GeneratorFirstInstruction
	.local LowerIRNode
	.global IRNext
	.global compiler
	.global stdout
	.global RVPrint
	.global RVBuildBasicBlocks
	.global RVPrintBasicBlocks
	.global OptLevel2
	.global RVOptimize
	.global fprintf
	.global RVAllocateRegisters
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
	ld          s3, 8(s1)
	ld          a0, 32(s3)
	call        TrapLower

	// *** Basic block 1

	ld          a0, 24(s3)
	call        TypeIsStructOrUnion

	// *** Basic block 2

	beqz        a0, .RVLower_label_52

	// *** Basic block 3

	lw          t0, 192(s2)
	addi        t0, t0, 1
	sw          t0, 192(s2)
	sw          t0, 200(s2)

	// *** Basic block 4

.RVLower_label_52:
	mv          a0, s1
	call        GeneratorFirstInstruction

	// *** Basic block 5

	mv          s3, a0
	beq         s3, x0, .RVLower_label_77

	// *** Basic block 6

.RVLower_label_61:
	mv          a2, s3
	mv          a1, s1
	mv          a0, s2
	call        LowerIRNode

	// *** Basic block 7

	mv          a0, s3
	call        IRNext

	// *** Basic block 8

	mv          s3, a0
	bne         s3, x0, .RVLower_label_61

	// *** Basic block 9

.RVLower_label_77:
	la          t1, compiler
	ld          s4, 0(t1)
	lb          t0, 1232(s4)
	bnez        t0, .RVLower_label_90

	// *** Basic block 10

	ld          t1, 1240(s4)
	la          t2, stdout
	ld          t2, 0(t2)
	sub         t1, t1, t2
	snez        t0, t1

	// *** Basic block 11

.RVLower_label_90:
	beqz        t0, .RVLower_label_98

	// *** Basic block 12

	ld          a1, 1240(s4)
	mv          a0, s2
	call        RVPrint

	// *** Basic block 13

.RVLower_label_98:
	mv          a0, s2
	call        RVBuildBasicBlocks

	// *** Basic block 14

	mv          t1, t0
	bnez        t0, .RVLower_label_111

	// *** Basic block 15

	ld          t2, 1240(s4)
	la          t3, stdout
	ld          t3, 0(t3)
	sub         t2, t2, t3
	snez        t1, t2

	// *** Basic block 16

.RVLower_label_111:
	beqz        t1, .RVLower_label_119

	// *** Basic block 17

	ld          a1, 1240(s4)
	mv          a0, s2
	call        RVPrintBasicBlocks

	// *** Basic block 18

.RVLower_label_119:
	call        OptLevel2

	// *** Basic block 19

	beqz        a0, .RVLower_label_150

	// *** Basic block 20

	mv          a0, s2
	call        RVOptimize

	// *** Basic block 21

	mv          t1, t0
	bnez        t0, .RVLower_label_134

	// *** Basic block 22

	ld          t0, 1240(s4)
	la          s5, stdout
	ld          t2, 0(s5)
	sub         t0, t0, t2
	snez        t1, t0

	// *** Basic block 23

.RVLower_label_134:
	beqz        t1, .RVLower_label_149

	// *** Basic block 24

	ld          s4, 1240(s4)
	lla         a1, .str.291
	mv          a0, s4
	call        fprintf

	// *** Basic block 25

	mv          a1, s4
	mv          a0, s2
	call        RVPrintBasicBlocks

	// *** Basic block 26

.RVLower_label_149:

	// *** Basic block 27

.RVLower_label_150:
	addi        a0, s2, 464
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           RVAllocateRegisters
.func_end_RVLower:
	.size RVLower, .func_end_RVLower-RVLower

	.global RVPrint
	.type RVPrint, @function

RVPrint:

	// *** Basic block 0

	.global TargetFirstInstruction
	.global TargetPrintInstruction
	.global RVOpcodeName
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
	mv          s1, a1
	call        TargetFirstInstruction

	// *** Basic block 1

	mv          s2, a0
	beq         s2, x0, .RVPrint_label_39

	// *** Basic block 2

.RVPrint_label_22:
	mv          a2, s1
	la          t0, RVOpcodeName
	mv          a1, t0
	mv          a0, s2
	call        TargetPrintInstruction

	// *** Basic block 3

	mv          a0, s2
	call        TargetNext

	// *** Basic block 4

	mv          s2, a0
	bne         s2, x0, .RVPrint_label_22

	// *** Basic block 5

.RVPrint_label_39:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_RVPrint:
	.size RVPrint, .func_end_RVPrint-RVPrint

.PCend:
	.data
load_opcodes:
	.type   load_opcodes,@object
	.local  load_opcodes
	.size   load_opcodes,224
	.p2align  3
	.global TypeIsInt
	.long    TypeIsInt
	.word   47
	.space  4
	.global TypeIsShort
	.long    TypeIsShort
	.word   46
	.space  4
	.global TypeIsChar
	.long    TypeIsChar
	.word   45
	.space  4
	.global TypeIsLong
	.long    TypeIsLong
	.word   83
	.space  4
	.global TypeIsLongLong
	.long    TypeIsLongLong
	.word   83
	.space  4
	.global TypeIsUnsignedInt
	.long    TypeIsUnsignedInt
	.word   82
	.space  4
	.global TypeIsUnsignedShort
	.long    TypeIsUnsignedShort
	.word   49
	.space  4
	.global TypeIsUnsignedChar
	.long    TypeIsUnsignedChar
	.word   48
	.space  4
	.global TypeIsFloat
	.long    TypeIsFloat
	.word   107
	.space  4
	.global TypeIsDouble
	.long    TypeIsDouble
	.word   137
	.space  4
	.global TypeIsBool
	.long    TypeIsBool
	.word   45
	.space  4
	.global TypeIsPointerOrArray
	.long    TypeIsPointerOrArray
	.word   83
	.space  4
	.global TypeIsFunction
	.long    TypeIsFunction
	.word   83
	.space  4
	.word   0
	.space  4
	.word   0
	.space  4

MultiplyDeBruijnBitPosition2:
	.type   MultiplyDeBruijnBitPosition2,@object
	.local  MultiplyDeBruijnBitPosition2
	.size   MultiplyDeBruijnBitPosition2,128
	.p2align  2
	.word   0
	.word   1
	.word   28
	.word   2
	.word   29
	.word   14
	.word   24
	.word   3
	.word   30
	.word   22
	.word   20
	.word   15
	.word   25
	.word   17
	.word   4
	.word   8
	.word   31
	.word   27
	.word   13
	.word   23
	.word   21
	.word   19
	.word   16
	.word   7
	.word   26
	.word   12
	.word   18
	.word   6
	.word   11
	.word   5
	.word   10
	.word   9

branch_compare_ops:
	.type   branch_compare_ops,@object
	.local  branch_compare_ops
	.size   branch_compare_ops,384
	.p2align  3
	.word   64
	.byte   1
	.space  3
	.word   39
	.byte   0
	.space  3
	.word   65
	.byte   1
	.space  3
	.word   40
	.byte   0
	.space  3
	.word   66
	.byte   1
	.space  3
	.word   41
	.byte   0
	.space  3
	.word   67
	.byte   0
	.space  3
	.word   42
	.byte   0
	.space  3
	.word   68
	.byte   1
	.space  3
	.word   41
	.byte   1
	.space  3
	.word   69
	.byte   1
	.space  3
	.word   42
	.byte   0
	.space  3
	.word   82
	.byte   1
	.space  3
	.word   39
	.byte   0
	.space  3
	.word   83
	.byte   1
	.space  3
	.word   40
	.byte   0
	.space  3
	.word   84
	.byte   1
	.space  3
	.word   43
	.byte   0
	.space  3
	.word   85
	.byte   0
	.space  3
	.word   44
	.byte   0
	.space  3
	.word   86
	.byte   1
	.space  3
	.word   43
	.byte   1
	.space  3
	.word   87
	.byte   1
	.space  3
	.word   44
	.byte   0
	.space  3
	.word   64
	.byte   0
	.space  3
	.word   40
	.byte   0
	.space  3
	.word   65
	.byte   0
	.space  3
	.word   39
	.byte   0
	.space  3
	.word   66
	.byte   0
	.space  3
	.word   42
	.byte   0
	.space  3
	.word   67
	.byte   1
	.space  3
	.word   41
	.byte   0
	.space  3
	.word   68
	.byte   0
	.space  3
	.word   42
	.byte   1
	.space  3
	.word   69
	.byte   0
	.space  3
	.word   41
	.byte   0
	.space  3
	.word   82
	.byte   0
	.space  3
	.word   40
	.byte   0
	.space  3
	.word   83
	.byte   0
	.space  3
	.word   39
	.byte   0
	.space  3
	.word   84
	.byte   0
	.space  3
	.word   44
	.byte   0
	.space  3
	.word   85
	.byte   1
	.space  3
	.word   43
	.byte   0
	.space  3
	.word   86
	.byte   0
	.space  3
	.word   44
	.byte   1
	.space  3
	.word   87
	.byte   0
	.space  3
	.word   43
	.byte   0
	.space  3

	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "CollectActualArguments"
	.type .str.1, @object
	.size .str.1, 23

.str.2:
	.asciz "mv"
	.type .str.2, @object
	.size .str.2, 3

.str.3:
	.asciz "fmv.s"
	.type .str.3, @object
	.size .str.3, 6

.str.4:
	.asciz "fmv.d"
	.type .str.4, @object
	.size .str.4, 6

.str.5:
	.asciz "lui"
	.type .str.5, @object
	.size .str.5, 4

.str.6:
	.asciz "auipc"
	.type .str.6, @object
	.size .str.6, 6

.str.7:
	.asciz "jal"
	.type .str.7, @object
	.size .str.7, 4

.str.8:
	.asciz "jalr"
	.type .str.8, @object
	.size .str.8, 5

.str.9:
	.asciz "beq"
	.type .str.9, @object
	.size .str.9, 4

.str.10:
	.asciz "bne"
	.type .str.10, @object
	.size .str.10, 4

.str.11:
	.asciz "blt"
	.type .str.11, @object
	.size .str.11, 4

.str.12:
	.asciz "bge"
	.type .str.12, @object
	.size .str.12, 4

.str.13:
	.asciz "bltu"
	.type .str.13, @object
	.size .str.13, 5

.str.14:
	.asciz "bgeu"
	.type .str.14, @object
	.size .str.14, 5

.str.15:
	.asciz "lb"
	.type .str.15, @object
	.size .str.15, 3

.str.16:
	.asciz "lh"
	.type .str.16, @object
	.size .str.16, 3

.str.17:
	.asciz "lw"
	.type .str.17, @object
	.size .str.17, 3

.str.18:
	.asciz "lbu"
	.type .str.18, @object
	.size .str.18, 4

.str.19:
	.asciz "lhu"
	.type .str.19, @object
	.size .str.19, 4

.str.20:
	.asciz "sb"
	.type .str.20, @object
	.size .str.20, 3

.str.21:
	.asciz "sh"
	.type .str.21, @object
	.size .str.21, 3

.str.22:
	.asciz "sw"
	.type .str.22, @object
	.size .str.22, 3

.str.23:
	.asciz "addi"
	.type .str.23, @object
	.size .str.23, 5

.str.24:
	.asciz "slti"
	.type .str.24, @object
	.size .str.24, 5

.str.25:
	.asciz "sltiu"
	.type .str.25, @object
	.size .str.25, 6

.str.26:
	.asciz "xori"
	.type .str.26, @object
	.size .str.26, 5

.str.27:
	.asciz "ori"
	.type .str.27, @object
	.size .str.27, 4

.str.28:
	.asciz "andi"
	.type .str.28, @object
	.size .str.28, 5

.str.29:
	.asciz "slli"
	.type .str.29, @object
	.size .str.29, 5

.str.30:
	.asciz "srli"
	.type .str.30, @object
	.size .str.30, 5

.str.31:
	.asciz "srai"
	.type .str.31, @object
	.size .str.31, 5

.str.32:
	.asciz "add"
	.type .str.32, @object
	.size .str.32, 4

.str.33:
	.asciz "sub"
	.type .str.33, @object
	.size .str.33, 4

.str.34:
	.asciz "sll"
	.type .str.34, @object
	.size .str.34, 4

.str.35:
	.asciz "slt"
	.type .str.35, @object
	.size .str.35, 4

.str.36:
	.asciz "sltu"
	.type .str.36, @object
	.size .str.36, 5

.str.37:
	.asciz "xor"
	.type .str.37, @object
	.size .str.37, 4

.str.38:
	.asciz "srl"
	.type .str.38, @object
	.size .str.38, 4

.str.39:
	.asciz "sra"
	.type .str.39, @object
	.size .str.39, 4

.str.40:
	.asciz "or"
	.type .str.40, @object
	.size .str.40, 3

.str.41:
	.asciz "and"
	.type .str.41, @object
	.size .str.41, 4

.str.42:
	.asciz "fence"
	.type .str.42, @object
	.size .str.42, 6

.str.43:
	.asciz "fence.i"
	.type .str.43, @object
	.size .str.43, 8

.str.44:
	.asciz "ecall"
	.type .str.44, @object
	.size .str.44, 6

.str.45:
	.asciz "ebreak"
	.type .str.45, @object
	.size .str.45, 7

.str.46:
	.asciz "csrrw"
	.type .str.46, @object
	.size .str.46, 6

.str.47:
	.asciz "csrrs"
	.type .str.47, @object
	.size .str.47, 6

.str.48:
	.asciz "csrrc"
	.type .str.48, @object
	.size .str.48, 6

.str.49:
	.asciz "csrrwi"
	.type .str.49, @object
	.size .str.49, 7

.str.50:
	.asciz "csrrsi"
	.type .str.50, @object
	.size .str.50, 7

.str.51:
	.asciz "csrrci"
	.type .str.51, @object
	.size .str.51, 7

.str.52:
	.asciz "lwu"
	.type .str.52, @object
	.size .str.52, 4

.str.53:
	.asciz "ld"
	.type .str.53, @object
	.size .str.53, 3

.str.54:
	.asciz "sd"
	.type .str.54, @object
	.size .str.54, 3

.str.55:
	.asciz "addiw"
	.type .str.55, @object
	.size .str.55, 6

.str.56:
	.asciz "slliw"
	.type .str.56, @object
	.size .str.56, 6

.str.57:
	.asciz "srliw"
	.type .str.57, @object
	.size .str.57, 6

.str.58:
	.asciz "sraiw"
	.type .str.58, @object
	.size .str.58, 6

.str.59:
	.asciz "addw"
	.type .str.59, @object
	.size .str.59, 5

.str.60:
	.asciz "subw"
	.type .str.60, @object
	.size .str.60, 5

.str.61:
	.asciz "sllw"
	.type .str.61, @object
	.size .str.61, 5

.str.62:
	.asciz "srlw"
	.type .str.62, @object
	.size .str.62, 5

.str.63:
	.asciz "sraw"
	.type .str.63, @object
	.size .str.63, 5

.str.64:
	.asciz "mul"
	.type .str.64, @object
	.size .str.64, 4

.str.65:
	.asciz "mulh"
	.type .str.65, @object
	.size .str.65, 5

.str.66:
	.asciz "mulhsu"
	.type .str.66, @object
	.size .str.66, 7

.str.67:
	.asciz "mulhu"
	.type .str.67, @object
	.size .str.67, 6

.str.68:
	.asciz "div"
	.type .str.68, @object
	.size .str.68, 4

.str.69:
	.asciz "divu"
	.type .str.69, @object
	.size .str.69, 5

.str.70:
	.asciz "rem"
	.type .str.70, @object
	.size .str.70, 4

.str.71:
	.asciz "remu"
	.type .str.71, @object
	.size .str.71, 5

.str.72:
	.asciz "mulw"
	.type .str.72, @object
	.size .str.72, 5

.str.73:
	.asciz "divw"
	.type .str.73, @object
	.size .str.73, 5

.str.74:
	.asciz "divuw"
	.type .str.74, @object
	.size .str.74, 6

.str.75:
	.asciz "remw"
	.type .str.75, @object
	.size .str.75, 5

.str.76:
	.asciz "remuw"
	.type .str.76, @object
	.size .str.76, 6

.str.77:
	.asciz "flw"
	.type .str.77, @object
	.size .str.77, 4

.str.78:
	.asciz "fsw"
	.type .str.78, @object
	.size .str.78, 4

.str.79:
	.asciz "fmadd.s"
	.type .str.79, @object
	.size .str.79, 8

.str.80:
	.asciz "fmsub.s"
	.type .str.80, @object
	.size .str.80, 8

.str.81:
	.asciz "fnmsub.s"
	.type .str.81, @object
	.size .str.81, 9

.str.82:
	.asciz "fnmadd.s"
	.type .str.82, @object
	.size .str.82, 9

.str.83:
	.asciz "fadd.s"
	.type .str.83, @object
	.size .str.83, 7

.str.84:
	.asciz "fsub.s"
	.type .str.84, @object
	.size .str.84, 7

.str.85:
	.asciz "fmul.s"
	.type .str.85, @object
	.size .str.85, 7

.str.86:
	.asciz "fdiv.s"
	.type .str.86, @object
	.size .str.86, 7

.str.87:
	.asciz "fsqrt.s"
	.type .str.87, @object
	.size .str.87, 8

.str.88:
	.asciz "fsgnj.s"
	.type .str.88, @object
	.size .str.88, 8

.str.89:
	.asciz "fsgnjn.s"
	.type .str.89, @object
	.size .str.89, 9

.str.90:
	.asciz "fsgnjx.s"
	.type .str.90, @object
	.size .str.90, 9

.str.91:
	.asciz "fmin.s"
	.type .str.91, @object
	.size .str.91, 7

.str.92:
	.asciz "fmax.s"
	.type .str.92, @object
	.size .str.92, 7

.str.93:
	.asciz "fcvt.w.s"
	.type .str.93, @object
	.size .str.93, 9

.str.94:
	.asciz "fcvt.wu.s"
	.type .str.94, @object
	.size .str.94, 10

.str.95:
	.asciz "fmv.x.w"
	.type .str.95, @object
	.size .str.95, 8

.str.96:
	.asciz "feq.s"
	.type .str.96, @object
	.size .str.96, 6

.str.97:
	.asciz "flt.s"
	.type .str.97, @object
	.size .str.97, 6

.str.98:
	.asciz "fle.s"
	.type .str.98, @object
	.size .str.98, 6

.str.99:
	.asciz "fclass.s"
	.type .str.99, @object
	.size .str.99, 9

.str.100:
	.asciz "fcvt.s.w"
	.type .str.100, @object
	.size .str.100, 9

.str.101:
	.asciz "fcvt.s.wu"
	.type .str.101, @object
	.size .str.101, 10

.str.102:
	.asciz "fmv.w.x"
	.type .str.102, @object
	.size .str.102, 8

.str.103:
	.asciz "fcvt.l.s"
	.type .str.103, @object
	.size .str.103, 9

.str.104:
	.asciz "fcvt.lu.s"
	.type .str.104, @object
	.size .str.104, 10

.str.105:
	.asciz "fcvt.s.l"
	.type .str.105, @object
	.size .str.105, 9

.str.106:
	.asciz "fcvt.s.lu"
	.type .str.106, @object
	.size .str.106, 10

.str.107:
	.asciz "fld"
	.type .str.107, @object
	.size .str.107, 4

.str.108:
	.asciz "fsd"
	.type .str.108, @object
	.size .str.108, 4

.str.109:
	.asciz "fmadd.d"
	.type .str.109, @object
	.size .str.109, 8

.str.110:
	.asciz "fmsub.d"
	.type .str.110, @object
	.size .str.110, 8

.str.111:
	.asciz "fnmsub.d"
	.type .str.111, @object
	.size .str.111, 9

.str.112:
	.asciz "fnmadd.d"
	.type .str.112, @object
	.size .str.112, 9

.str.113:
	.asciz "fadd.d"
	.type .str.113, @object
	.size .str.113, 7

.str.114:
	.asciz "fsub.d"
	.type .str.114, @object
	.size .str.114, 7

.str.115:
	.asciz "fmul.d"
	.type .str.115, @object
	.size .str.115, 7

.str.116:
	.asciz "fdiv.d"
	.type .str.116, @object
	.size .str.116, 7

.str.117:
	.asciz "fsqrt.d"
	.type .str.117, @object
	.size .str.117, 8

.str.118:
	.asciz "fsgnj.d"
	.type .str.118, @object
	.size .str.118, 8

.str.119:
	.asciz "fsgnjn.d"
	.type .str.119, @object
	.size .str.119, 9

.str.120:
	.asciz "fsgnjx.d"
	.type .str.120, @object
	.size .str.120, 9

.str.121:
	.asciz "fmin.d"
	.type .str.121, @object
	.size .str.121, 7

.str.122:
	.asciz "fmax.d"
	.type .str.122, @object
	.size .str.122, 7

.str.123:
	.asciz "fcvt.s.d"
	.type .str.123, @object
	.size .str.123, 9

.str.124:
	.asciz "fcvt.d.s"
	.type .str.124, @object
	.size .str.124, 9

.str.125:
	.asciz "feq.d"
	.type .str.125, @object
	.size .str.125, 6

.str.126:
	.asciz "flt.d"
	.type .str.126, @object
	.size .str.126, 6

.str.127:
	.asciz "fle.d"
	.type .str.127, @object
	.size .str.127, 6

.str.128:
	.asciz "fclass.d"
	.type .str.128, @object
	.size .str.128, 9

.str.129:
	.asciz "fcvt.w.d"
	.type .str.129, @object
	.size .str.129, 9

.str.130:
	.asciz "fcvt.wu.d"
	.type .str.130, @object
	.size .str.130, 10

.str.131:
	.asciz "fcvt.d.w"
	.type .str.131, @object
	.size .str.131, 9

.str.132:
	.asciz "fcvt.d.wu"
	.type .str.132, @object
	.size .str.132, 10

.str.133:
	.asciz "fcvt.l.d"
	.type .str.133, @object
	.size .str.133, 9

.str.134:
	.asciz "fcvt.lu.d"
	.type .str.134, @object
	.size .str.134, 10

.str.135:
	.asciz "fmv.x.d"
	.type .str.135, @object
	.size .str.135, 8

.str.136:
	.asciz "fcvt.d.l"
	.type .str.136, @object
	.size .str.136, 9

.str.137:
	.asciz "fcvt.d.lu"
	.type .str.137, @object
	.size .str.137, 10

.str.138:
	.asciz "fmv.d.x"
	.type .str.138, @object
	.size .str.138, 8

.str.139:
	.asciz "nop"
	.type .str.139, @object
	.size .str.139, 4

.str.140:
	.asciz "not"
	.type .str.140, @object
	.size .str.140, 4

.str.141:
	.asciz "neg"
	.type .str.141, @object
	.size .str.141, 4

.str.142:
	.asciz "fneg.s"
	.type .str.142, @object
	.size .str.142, 7

.str.143:
	.asciz "fneg.d"
	.type .str.143, @object
	.size .str.143, 7

.str.144:
	.asciz "li"
	.type .str.144, @object
	.size .str.144, 3

.str.145:
	.asciz "seqz"
	.type .str.145, @object
	.size .str.145, 5

.str.146:
	.asciz "snez"
	.type .str.146, @object
	.size .str.146, 5

.str.147:
	.asciz "sltz"
	.type .str.147, @object
	.size .str.147, 5

.str.148:
	.asciz "sz"
	.type .str.148, @object
	.size .str.148, 3

.str.149:
	.asciz "beqz"
	.type .str.149, @object
	.size .str.149, 5

.str.150:
	.asciz "bnez"
	.type .str.150, @object
	.size .str.150, 5

.str.151:
	.asciz "j"
	.type .str.151, @object
	.size .str.151, 2

.str.152:
	.asciz "jr"
	.type .str.152, @object
	.size .str.152, 3

.str.153:
	.asciz "call"
	.type .str.153, @object
	.size .str.153, 5

.str.154:
	.asciz "rcall"
	.type .str.154, @object
	.size .str.154, 6

.str.155:
	.asciz "callf"
	.type .str.155, @object
	.size .str.155, 6

.str.156:
	.asciz "rcallf"
	.type .str.156, @object
	.size .str.156, 7

.str.157:
	.asciz "la"
	.type .str.157, @object
	.size .str.157, 3

.str.158:
	.asciz "lla"
	.type .str.158, @object
	.size .str.158, 4

.str.159:
	.asciz "sext.w"
	.type .str.159, @object
	.size .str.159, 7

.str.160:
	.asciz "a0"
	.type .str.160, @object
	.size .str.160, 3

.str.161:
	.asciz "a1"
	.type .str.161, @object
	.size .str.161, 3

.str.162:
	.asciz "a2"
	.type .str.162, @object
	.size .str.162, 3

.str.163:
	.asciz "a3"
	.type .str.163, @object
	.size .str.163, 3

.str.164:
	.asciz "a4"
	.type .str.164, @object
	.size .str.164, 3

.str.165:
	.asciz "a5"
	.type .str.165, @object
	.size .str.165, 3

.str.166:
	.asciz "a6"
	.type .str.166, @object
	.size .str.166, 3

.str.167:
	.asciz "a7"
	.type .str.167, @object
	.size .str.167, 3

.str.168:
	.asciz "fa0"
	.type .str.168, @object
	.size .str.168, 4

.str.169:
	.asciz "fa1"
	.type .str.169, @object
	.size .str.169, 4

.str.170:
	.asciz "fa2"
	.type .str.170, @object
	.size .str.170, 4

.str.171:
	.asciz "fa3"
	.type .str.171, @object
	.size .str.171, 4

.str.172:
	.asciz "fa4"
	.type .str.172, @object
	.size .str.172, 4

.str.173:
	.asciz "fa5"
	.type .str.173, @object
	.size .str.173, 4

.str.174:
	.asciz "fa6"
	.type .str.174, @object
	.size .str.174, 4

.str.175:
	.asciz "fa7"
	.type .str.175, @object
	.size .str.175, 4

.str.176:
	.asciz "regarg"
	.type .str.176, @object
	.size .str.176, 7

.str.177:
	.asciz "nrvoval"
	.type .str.177, @object
	.size .str.177, 8

.str.178:
	.asciz "x0"
	.type .str.178, @object
	.size .str.178, 3

.str.179:
	.asciz "t0"
	.type .str.179, @object
	.size .str.179, 3

.str.180:
	.asciz "spill"
	.type .str.180, @object
	.size .str.180, 6

.str.181:
	.asciz "reload"
	.type .str.181, @object
	.size .str.181, 7

.str.182:
	.asciz "(null)"
	.type .str.182, @object
	.size .str.182, 1

.str.183:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.183, @object
	.size .str.183, 30

.str.184:
	.asciz "(null)"
	.type .str.184, @object
	.size .str.184, 1

.str.185:
	.asciz "false"
	.type .str.185, @object
	.size .str.185, 6

.str.186:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.186, @object
	.size .str.186, 30

.str.187:
	.asciz "(null)"
	.type .str.187, @object
	.size .str.187, 1

.str.188:
	.asciz "opcode != 0"
	.type .str.188, @object
	.size .str.188, 12

.str.189:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.189, @object
	.size .str.189, 30

.str.190:
	.asciz "(null)"
	.type .str.190, @object
	.size .str.190, 1

.str.191:
	.asciz "false"
	.type .str.191, @object
	.size .str.191, 6

.str.192:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.192, @object
	.size .str.192, 30

.str.193:
	.asciz "(null)"
	.type .str.193, @object
	.size .str.193, 1

.str.194:
	.asciz "node->inputs.length <= 2"
	.type .str.194, @object
	.size .str.194, 25

.str.195:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.195, @object
	.size .str.195, 30

.str.196:
	.asciz "(null)"
	.type .str.196, @object
	.size .str.196, 1

.str.197:
	.asciz "node->inputs.length == 2"
	.type .str.197, @object
	.size .str.197, 25

.str.198:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.198, @object
	.size .str.198, 30

.str.199:
	.asciz "(null)"
	.type .str.199, @object
	.size .str.199, 1

.str.200:
	.asciz "node->inputs.length == 2"
	.type .str.200, @object
	.size .str.200, 25

.str.201:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.201, @object
	.size .str.201, 30

.str.202:
	.asciz "(null)"
	.type .str.202, @object
	.size .str.202, 1

.str.203:
	.asciz "node->inputs.length == 2"
	.type .str.203, @object
	.size .str.203, 25

.str.204:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.204, @object
	.size .str.204, 30

.str.205:
	.asciz "(null)"
	.type .str.205, @object
	.size .str.205, 1

.str.206:
	.asciz "node->inputs.length == 2"
	.type .str.206, @object
	.size .str.206, 25

.str.207:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.207, @object
	.size .str.207, 30

.str.208:
	.asciz "(null)"
	.type .str.208, @object
	.size .str.208, 1

.str.209:
	.asciz "node->inputs.length == 2"
	.type .str.209, @object
	.size .str.209, 25

.str.210:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.210, @object
	.size .str.210, 30

.str.211:
	.asciz "(null)"
	.type .str.211, @object
	.size .str.211, 1

.str.212:
	.asciz "node->inputs.length == 2"
	.type .str.212, @object
	.size .str.212, 25

.str.213:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.213, @object
	.size .str.213, 30

.str.214:
	.asciz "(null)"
	.type .str.214, @object
	.size .str.214, 1

.str.215:
	.asciz "node->inputs.length == 2"
	.type .str.215, @object
	.size .str.215, 25

.str.216:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.216, @object
	.size .str.216, 30

.str.217:
	.asciz "(null)"
	.type .str.217, @object
	.size .str.217, 1

.str.218:
	.asciz "node->inputs.length == 2"
	.type .str.218, @object
	.size .str.218, 25

.str.219:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.219, @object
	.size .str.219, 30

.str.220:
	.asciz "(null)"
	.type .str.220, @object
	.size .str.220, 1

.str.221:
	.asciz "node->inputs.length == 2"
	.type .str.221, @object
	.size .str.221, 25

.str.222:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.222, @object
	.size .str.222, 30

.str.223:
	.asciz "(null)"
	.type .str.223, @object
	.size .str.223, 1

.str.224:
	.asciz "false"
	.type .str.224, @object
	.size .str.224, 6

.str.225:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.225, @object
	.size .str.225, 30

.str.226:
	.asciz "(null)"
	.type .str.226, @object
	.size .str.226, 1

.str.227:
	.asciz "addr != NULL"
	.type .str.227, @object
	.size .str.227, 13

.str.228:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.228, @object
	.size .str.228, 30

.str.229:
	.asciz "(null)"
	.type .str.229, @object
	.size .str.229, 1

.str.230:
	.asciz "node->inputs.length == 1"
	.type .str.230, @object
	.size .str.230, 25

.str.231:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.231, @object
	.size .str.231, 30

.str.232:
	.asciz "(null)"
	.type .str.232, @object
	.size .str.232, 1

.str.233:
	.asciz "false"
	.type .str.233, @object
	.size .str.233, 6

.str.234:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.234, @object
	.size .str.234, 30

.str.235:
	.asciz "(null)"
	.type .str.235, @object
	.size .str.235, 1

.str.236:
	.asciz "src != NULL"
	.type .str.236, @object
	.size .str.236, 12

.str.237:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.237, @object
	.size .str.237, 30

.str.238:
	.asciz "(null)"
	.type .str.238, @object
	.size .str.238, 1

.str.239:
	.asciz "immed != NULL"
	.type .str.239, @object
	.size .str.239, 14

.str.240:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.240, @object
	.size .str.240, 30

.str.241:
	.asciz "(null)"
	.type .str.241, @object
	.size .str.241, 1

.str.242:
	.asciz "TargetIsConst(immed)"
	.type .str.242, @object
	.size .str.242, 21

.str.243:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.243, @object
	.size .str.243, 30

.str.244:
	.asciz "(null)"
	.type .str.244, @object
	.size .str.244, 1

.str.245:
	.asciz "node->inputs.length == 2"
	.type .str.245, @object
	.size .str.245, 25

.str.246:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.246, @object
	.size .str.246, 30

.str.247:
	.asciz "(null)"
	.type .str.247, @object
	.size .str.247, 1

.str.248:
	.asciz "false"
	.type .str.248, @object
	.size .str.248, 6

.str.249:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.249, @object
	.size .str.249, 30

.str.250:
	.asciz "(null)"
	.type .str.250, @object
	.size .str.250, 1

.str.251:
	.asciz "node->inputs.length == 2"
	.type .str.251, @object
	.size .str.251, 25

.str.252:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.252, @object
	.size .str.252, 30

.str.253:
	.asciz "(null)"
	.type .str.253, @object
	.size .str.253, 1

.str.254:
	.asciz "false"
	.type .str.254, @object
	.size .str.254, 6

.str.255:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.255, @object
	.size .str.255, 30

.str.256:
	.asciz "(null)"
	.type .str.256, @object
	.size .str.256, 1

.str.257:
	.asciz "node->inputs.length == 1"
	.type .str.257, @object
	.size .str.257, 25

.str.258:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.258, @object
	.size .str.258, 30

.str.259:
	.asciz "(null)"
	.type .str.259, @object
	.size .str.259, 1

.str.260:
	.asciz "node->inputs.length == 1"
	.type .str.260, @object
	.size .str.260, 25

.str.261:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.261, @object
	.size .str.261, 30

.str.262:
	.asciz "(null)"
	.type .str.262, @object
	.size .str.262, 1

.str.263:
	.asciz "false"
	.type .str.263, @object
	.size .str.263, 6

.str.264:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.264, @object
	.size .str.264, 30

.str.265:
	.asciz "(null)"
	.type .str.265, @object
	.size .str.265, 1

.str.266:
	.asciz "node->inputs.length == 3"
	.type .str.266, @object
	.size .str.266, 25

.str.267:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.267, @object
	.size .str.267, 30

.str.268:
	.asciz "(null)"
	.type .str.268, @object
	.size .str.268, 1

.str.269:
	.asciz "IRIsConst(node->inputs.value.p[2])"
	.type .str.269, @object
	.size .str.269, 35

.str.270:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.270, @object
	.size .str.270, 30

.str.271:
	.asciz "(null)"
	.type .str.271, @object
	.size .str.271, 1

.str.272:
	.asciz "node->inputs.length == 1"
	.type .str.272, @object
	.size .str.272, 25

.str.273:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.273, @object
	.size .str.273, 30

.str.274:
	.asciz "(null)"
	.type .str.274, @object
	.size .str.274, 1

.str.275:
	.asciz "node->inputs.length >= 1"
	.type .str.275, @object
	.size .str.275, 25

.str.276:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.276, @object
	.size .str.276, 30

.str.277:
	.asciz "(null)"
	.type .str.277, @object
	.size .str.277, 1

.str.278:
	.asciz "node->inputs.length == 1"
	.type .str.278, @object
	.size .str.278, 25

.str.279:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.279, @object
	.size .str.279, 30

.str.280:
	.asciz "(null)"
	.type .str.280, @object
	.size .str.280, 1

.str.281:
	.asciz "false"
	.type .str.281, @object
	.size .str.281, 6

.str.282:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.282, @object
	.size .str.282, 30

.str.283:
	.asciz "(null)"
	.type .str.283, @object
	.size .str.283, 1

.str.284:
	.asciz "false"
	.type .str.284, @object
	.size .str.284, 6

.str.285:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.285, @object
	.size .str.285, 30

.str.286:
	.asciz "(null)"
	.type .str.286, @object
	.size .str.286, 1

.str.287:
	.asciz "false"
	.type .str.287, @object
	.size .str.287, 6

.str.288:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.288, @object
	.size .str.288, 30

.str.289:
	.asciz "(null)"
	.type .str.289, @object
	.size .str.289, 1

.str.290:
	.asciz "size != 0"
	.type .str.290, @object
	.size .str.290, 10

.str.291:
	.asciz "\n After RISC-V optimization\n"
	.type .str.291, @object
	.size .str.291, 29


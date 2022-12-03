	.file   "ast.c"
	.text
	.option pic
.PCbegin:
	.global ASTOpcodeName
	.type ASTOpcodeName, @function

ASTOpcodeName:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	blt         t0, x0, .ASTOpcodeName_label_1131

	// *** Basic block 1

	li          t1, 162		// 0xa2 ASCII \xa2
	blt         t1, t0, .ASTOpcodeName_label_1131

	// *** Basic block 2

	slli        t1, t0, 2
	auipc       t2, 0
	add         t1, t2, t1
	jalr        x0, t1, 12

	// *** Basic block 3

	j           .ASTOpcodeName_label_339

	// *** Basic block 4

	j           .ASTOpcodeName_label_346

	// *** Basic block 5

	j           .ASTOpcodeName_label_351

	// *** Basic block 6

	j           .ASTOpcodeName_label_356

	// *** Basic block 7

	j           .ASTOpcodeName_label_361

	// *** Basic block 8

	j           .ASTOpcodeName_label_366

	// *** Basic block 9

	j           .ASTOpcodeName_label_371

	// *** Basic block 10

	j           .ASTOpcodeName_label_376

	// *** Basic block 11

	j           .ASTOpcodeName_label_381

	// *** Basic block 12

	j           .ASTOpcodeName_label_386

	// *** Basic block 13

	j           .ASTOpcodeName_label_391

	// *** Basic block 14

	j           .ASTOpcodeName_label_396

	// *** Basic block 15

	j           .ASTOpcodeName_label_401

	// *** Basic block 16

	j           .ASTOpcodeName_label_628

	// *** Basic block 17

	j           .ASTOpcodeName_label_629

	// *** Basic block 18

	j           .ASTOpcodeName_label_635

	// *** Basic block 19

	j           .ASTOpcodeName_label_636

	// *** Basic block 20

	j           .ASTOpcodeName_label_406

	// *** Basic block 21

	j           .ASTOpcodeName_label_411

	// *** Basic block 22

	j           .ASTOpcodeName_label_416

	// *** Basic block 23

	j           .ASTOpcodeName_label_421

	// *** Basic block 24

	j           .ASTOpcodeName_label_426

	// *** Basic block 25

	j           .ASTOpcodeName_label_431

	// *** Basic block 26

	j           .ASTOpcodeName_label_436

	// *** Basic block 27

	j           .ASTOpcodeName_label_441

	// *** Basic block 28

	j           .ASTOpcodeName_label_446

	// *** Basic block 29

	j           .ASTOpcodeName_label_451

	// *** Basic block 30

	j           .ASTOpcodeName_label_1131

	// *** Basic block 31

	j           .ASTOpcodeName_label_456

	// *** Basic block 32

	j           .ASTOpcodeName_label_461

	// *** Basic block 33

	j           .ASTOpcodeName_label_466

	// *** Basic block 34

	j           .ASTOpcodeName_label_471

	// *** Basic block 35

	j           .ASTOpcodeName_label_476

	// *** Basic block 36

	j           .ASTOpcodeName_label_481

	// *** Basic block 37

	j           .ASTOpcodeName_label_486

	// *** Basic block 38

	j           .ASTOpcodeName_label_491

	// *** Basic block 39

	j           .ASTOpcodeName_label_496

	// *** Basic block 40

	j           .ASTOpcodeName_label_501

	// *** Basic block 41

	j           .ASTOpcodeName_label_506

	// *** Basic block 42

	j           .ASTOpcodeName_label_511

	// *** Basic block 43

	j           .ASTOpcodeName_label_516

	// *** Basic block 44

	j           .ASTOpcodeName_label_521

	// *** Basic block 45

	j           .ASTOpcodeName_label_526

	// *** Basic block 46

	j           .ASTOpcodeName_label_531

	// *** Basic block 47

	j           .ASTOpcodeName_label_536

	// *** Basic block 48

	j           .ASTOpcodeName_label_541

	// *** Basic block 49

	j           .ASTOpcodeName_label_546

	// *** Basic block 50

	j           .ASTOpcodeName_label_547

	// *** Basic block 51

	j           .ASTOpcodeName_label_552

	// *** Basic block 52

	j           .ASTOpcodeName_label_557

	// *** Basic block 53

	j           .ASTOpcodeName_label_562

	// *** Basic block 54

	j           .ASTOpcodeName_label_567

	// *** Basic block 55

	j           .ASTOpcodeName_label_572

	// *** Basic block 56

	j           .ASTOpcodeName_label_577

	// *** Basic block 57

	j           .ASTOpcodeName_label_582

	// *** Basic block 58

	j           .ASTOpcodeName_label_587

	// *** Basic block 59

	j           .ASTOpcodeName_label_592

	// *** Basic block 60

	j           .ASTOpcodeName_label_597

	// *** Basic block 61

	j           .ASTOpcodeName_label_602

	// *** Basic block 62

	j           .ASTOpcodeName_label_607

	// *** Basic block 63

	j           .ASTOpcodeName_label_612

	// *** Basic block 64

	j           .ASTOpcodeName_label_617

	// *** Basic block 65

	j           .ASTOpcodeName_label_622

	// *** Basic block 66

	j           .ASTOpcodeName_label_627

	// *** Basic block 67

	j           .ASTOpcodeName_label_634

	// *** Basic block 68

	j           .ASTOpcodeName_label_641

	// *** Basic block 69

	j           .ASTOpcodeName_label_646

	// *** Basic block 70

	j           .ASTOpcodeName_label_651

	// *** Basic block 71

	j           .ASTOpcodeName_label_656

	// *** Basic block 72

	j           .ASTOpcodeName_label_661

	// *** Basic block 73

	j           .ASTOpcodeName_label_666

	// *** Basic block 74

	j           .ASTOpcodeName_label_671

	// *** Basic block 75

	j           .ASTOpcodeName_label_676

	// *** Basic block 76

	j           .ASTOpcodeName_label_681

	// *** Basic block 77

	j           .ASTOpcodeName_label_686

	// *** Basic block 78

	j           .ASTOpcodeName_label_691

	// *** Basic block 79

	j           .ASTOpcodeName_label_696

	// *** Basic block 80

	j           .ASTOpcodeName_label_701

	// *** Basic block 81

	j           .ASTOpcodeName_label_706

	// *** Basic block 82

	j           .ASTOpcodeName_label_711

	// *** Basic block 83

	j           .ASTOpcodeName_label_716

	// *** Basic block 84

	j           .ASTOpcodeName_label_721

	// *** Basic block 85

	j           .ASTOpcodeName_label_726

	// *** Basic block 86

	j           .ASTOpcodeName_label_731

	// *** Basic block 87

	j           .ASTOpcodeName_label_736

	// *** Basic block 88

	j           .ASTOpcodeName_label_741

	// *** Basic block 89

	j           .ASTOpcodeName_label_746

	// *** Basic block 90

	j           .ASTOpcodeName_label_751

	// *** Basic block 91

	j           .ASTOpcodeName_label_756

	// *** Basic block 92

	j           .ASTOpcodeName_label_761

	// *** Basic block 93

	j           .ASTOpcodeName_label_766

	// *** Basic block 94

	j           .ASTOpcodeName_label_771

	// *** Basic block 95

	j           .ASTOpcodeName_label_776

	// *** Basic block 96

	j           .ASTOpcodeName_label_781

	// *** Basic block 97

	j           .ASTOpcodeName_label_786

	// *** Basic block 98

	j           .ASTOpcodeName_label_791

	// *** Basic block 99

	j           .ASTOpcodeName_label_796

	// *** Basic block 100

	j           .ASTOpcodeName_label_801

	// *** Basic block 101

	j           .ASTOpcodeName_label_806

	// *** Basic block 102

	j           .ASTOpcodeName_label_811

	// *** Basic block 103

	j           .ASTOpcodeName_label_816

	// *** Basic block 104

	j           .ASTOpcodeName_label_821

	// *** Basic block 105

	j           .ASTOpcodeName_label_826

	// *** Basic block 106

	j           .ASTOpcodeName_label_831

	// *** Basic block 107

	j           .ASTOpcodeName_label_836

	// *** Basic block 108

	j           .ASTOpcodeName_label_841

	// *** Basic block 109

	j           .ASTOpcodeName_label_846

	// *** Basic block 110

	j           .ASTOpcodeName_label_851

	// *** Basic block 111

	j           .ASTOpcodeName_label_856

	// *** Basic block 112

	j           .ASTOpcodeName_label_861

	// *** Basic block 113

	j           .ASTOpcodeName_label_866

	// *** Basic block 114

	j           .ASTOpcodeName_label_871

	// *** Basic block 115

	j           .ASTOpcodeName_label_876

	// *** Basic block 116

	j           .ASTOpcodeName_label_881

	// *** Basic block 117

	j           .ASTOpcodeName_label_886

	// *** Basic block 118

	j           .ASTOpcodeName_label_891

	// *** Basic block 119

	j           .ASTOpcodeName_label_896

	// *** Basic block 120

	j           .ASTOpcodeName_label_901

	// *** Basic block 121

	j           .ASTOpcodeName_label_906

	// *** Basic block 122

	j           .ASTOpcodeName_label_911

	// *** Basic block 123

	j           .ASTOpcodeName_label_916

	// *** Basic block 124

	j           .ASTOpcodeName_label_921

	// *** Basic block 125

	j           .ASTOpcodeName_label_926

	// *** Basic block 126

	j           .ASTOpcodeName_label_931

	// *** Basic block 127

	j           .ASTOpcodeName_label_936

	// *** Basic block 128

	j           .ASTOpcodeName_label_941

	// *** Basic block 129

	j           .ASTOpcodeName_label_946

	// *** Basic block 130

	j           .ASTOpcodeName_label_951

	// *** Basic block 131

	j           .ASTOpcodeName_label_956

	// *** Basic block 132

	j           .ASTOpcodeName_label_961

	// *** Basic block 133

	j           .ASTOpcodeName_label_966

	// *** Basic block 134

	j           .ASTOpcodeName_label_971

	// *** Basic block 135

	j           .ASTOpcodeName_label_976

	// *** Basic block 136

	j           .ASTOpcodeName_label_981

	// *** Basic block 137

	j           .ASTOpcodeName_label_986

	// *** Basic block 138

	j           .ASTOpcodeName_label_991

	// *** Basic block 139

	j           .ASTOpcodeName_label_996

	// *** Basic block 140

	j           .ASTOpcodeName_label_1001

	// *** Basic block 141

	j           .ASTOpcodeName_label_1006

	// *** Basic block 142

	j           .ASTOpcodeName_label_1011

	// *** Basic block 143

	j           .ASTOpcodeName_label_1016

	// *** Basic block 144

	j           .ASTOpcodeName_label_1021

	// *** Basic block 145

	j           .ASTOpcodeName_label_1026

	// *** Basic block 146

	j           .ASTOpcodeName_label_1031

	// *** Basic block 147

	j           .ASTOpcodeName_label_1036

	// *** Basic block 148

	j           .ASTOpcodeName_label_1041

	// *** Basic block 149

	j           .ASTOpcodeName_label_1046

	// *** Basic block 150

	j           .ASTOpcodeName_label_1051

	// *** Basic block 151

	j           .ASTOpcodeName_label_1056

	// *** Basic block 152

	j           .ASTOpcodeName_label_1061

	// *** Basic block 153

	j           .ASTOpcodeName_label_1066

	// *** Basic block 154

	j           .ASTOpcodeName_label_1071

	// *** Basic block 155

	j           .ASTOpcodeName_label_1076

	// *** Basic block 156

	j           .ASTOpcodeName_label_1081

	// *** Basic block 157

	j           .ASTOpcodeName_label_1086

	// *** Basic block 158

	j           .ASTOpcodeName_label_1091

	// *** Basic block 159

	j           .ASTOpcodeName_label_1096

	// *** Basic block 160

	j           .ASTOpcodeName_label_1101

	// *** Basic block 161

	j           .ASTOpcodeName_label_1106

	// *** Basic block 162

	j           .ASTOpcodeName_label_1111

	// *** Basic block 163

	j           .ASTOpcodeName_label_1116

	// *** Basic block 164

	j           .ASTOpcodeName_label_1126

	// *** Basic block 165

	j           .ASTOpcodeName_label_1121

	// *** Basic block 166

.ASTOpcodeName_label_339:
	lla         a0, .str.1

	// *** Basic block 167

.ASTOpcodeName_label_343:
	ret         

	// *** Basic block 168

.ASTOpcodeName_label_346:
	lla         a0, .str.2
	ret         

	// *** Basic block 169

.ASTOpcodeName_label_351:
	lla         a0, .str.3
	ret         

	// *** Basic block 170

.ASTOpcodeName_label_356:
	lla         a0, .str.4
	ret         

	// *** Basic block 171

.ASTOpcodeName_label_361:
	lla         a0, .str.5
	ret         

	// *** Basic block 172

.ASTOpcodeName_label_366:
	lla         a0, .str.6
	ret         

	// *** Basic block 173

.ASTOpcodeName_label_371:
	lla         a0, .str.7
	ret         

	// *** Basic block 174

.ASTOpcodeName_label_376:
	lla         a0, .str.8
	ret         

	// *** Basic block 175

.ASTOpcodeName_label_381:
	lla         a0, .str.9
	ret         

	// *** Basic block 176

.ASTOpcodeName_label_386:
	lla         a0, .str.10
	ret         

	// *** Basic block 177

.ASTOpcodeName_label_391:
	lla         a0, .str.11
	ret         

	// *** Basic block 178

.ASTOpcodeName_label_396:
	lla         a0, .str.12
	ret         

	// *** Basic block 179

.ASTOpcodeName_label_401:
	lla         a0, .str.13
	ret         

	// *** Basic block 180

.ASTOpcodeName_label_406:
	lla         a0, .str.14
	ret         

	// *** Basic block 181

.ASTOpcodeName_label_411:
	lla         a0, .str.15
	ret         

	// *** Basic block 182

.ASTOpcodeName_label_416:
	lla         a0, .str.16
	ret         

	// *** Basic block 183

.ASTOpcodeName_label_421:
	lla         a0, .str.17
	ret         

	// *** Basic block 184

.ASTOpcodeName_label_426:
	lla         a0, .str.18
	ret         

	// *** Basic block 185

.ASTOpcodeName_label_431:
	lla         a0, .str.19
	ret         

	// *** Basic block 186

.ASTOpcodeName_label_436:
	lla         a0, .str.20
	ret         

	// *** Basic block 187

.ASTOpcodeName_label_441:
	lla         a0, .str.21
	ret         

	// *** Basic block 188

.ASTOpcodeName_label_446:
	lla         a0, .str.22
	ret         

	// *** Basic block 189

.ASTOpcodeName_label_451:
	lla         a0, .str.23
	ret         

	// *** Basic block 190

.ASTOpcodeName_label_456:
	lla         a0, .str.24
	ret         

	// *** Basic block 191

.ASTOpcodeName_label_461:
	lla         a0, .str.25
	ret         

	// *** Basic block 192

.ASTOpcodeName_label_466:
	lla         a0, .str.26
	ret         

	// *** Basic block 193

.ASTOpcodeName_label_471:
	lla         a0, .str.27
	ret         

	// *** Basic block 194

.ASTOpcodeName_label_476:
	lla         a0, .str.28
	ret         

	// *** Basic block 195

.ASTOpcodeName_label_481:
	lla         a0, .str.29
	ret         

	// *** Basic block 196

.ASTOpcodeName_label_486:
	lla         a0, .str.30
	ret         

	// *** Basic block 197

.ASTOpcodeName_label_491:
	lla         a0, .str.31
	ret         

	// *** Basic block 198

.ASTOpcodeName_label_496:
	lla         a0, .str.32
	ret         

	// *** Basic block 199

.ASTOpcodeName_label_501:
	lla         a0, .str.33
	ret         

	// *** Basic block 200

.ASTOpcodeName_label_506:
	lla         a0, .str.34
	ret         

	// *** Basic block 201

.ASTOpcodeName_label_511:
	lla         a0, .str.35
	ret         

	// *** Basic block 202

.ASTOpcodeName_label_516:
	lla         a0, .str.36
	ret         

	// *** Basic block 203

.ASTOpcodeName_label_521:
	lla         a0, .str.37
	ret         

	// *** Basic block 204

.ASTOpcodeName_label_526:
	lla         a0, .str.38
	ret         

	// *** Basic block 205

.ASTOpcodeName_label_531:
	lla         a0, .str.39
	ret         

	// *** Basic block 206

.ASTOpcodeName_label_536:
	lla         a0, .str.40
	ret         

	// *** Basic block 207

.ASTOpcodeName_label_541:
	lla         a0, .str.41
	ret         

	// *** Basic block 208

.ASTOpcodeName_label_546:

	// *** Basic block 209

.ASTOpcodeName_label_547:
	lla         a0, .str.42
	ret         

	// *** Basic block 210

.ASTOpcodeName_label_552:
	lla         a0, .str.43
	ret         

	// *** Basic block 211

.ASTOpcodeName_label_557:
	lla         a0, .str.44
	ret         

	// *** Basic block 212

.ASTOpcodeName_label_562:
	lla         a0, .str.45
	ret         

	// *** Basic block 213

.ASTOpcodeName_label_567:
	lla         a0, .str.46
	ret         

	// *** Basic block 214

.ASTOpcodeName_label_572:
	lla         a0, .str.47
	ret         

	// *** Basic block 215

.ASTOpcodeName_label_577:
	lla         a0, .str.48
	ret         

	// *** Basic block 216

.ASTOpcodeName_label_582:
	lla         a0, .str.49
	ret         

	// *** Basic block 217

.ASTOpcodeName_label_587:
	lla         a0, .str.50
	ret         

	// *** Basic block 218

.ASTOpcodeName_label_592:
	lla         a0, .str.51
	ret         

	// *** Basic block 219

.ASTOpcodeName_label_597:
	lla         a0, .str.52
	ret         

	// *** Basic block 220

.ASTOpcodeName_label_602:
	lla         a0, .str.53
	ret         

	// *** Basic block 221

.ASTOpcodeName_label_607:
	lla         a0, .str.54
	ret         

	// *** Basic block 222

.ASTOpcodeName_label_612:
	lla         a0, .str.55
	ret         

	// *** Basic block 223

.ASTOpcodeName_label_617:
	lla         a0, .str.56
	ret         

	// *** Basic block 224

.ASTOpcodeName_label_622:
	lla         a0, .str.57
	ret         

	// *** Basic block 225

.ASTOpcodeName_label_627:

	// *** Basic block 226

.ASTOpcodeName_label_628:

	// *** Basic block 227

.ASTOpcodeName_label_629:
	lla         a0, .str.58
	ret         

	// *** Basic block 228

.ASTOpcodeName_label_634:

	// *** Basic block 229

.ASTOpcodeName_label_635:

	// *** Basic block 230

.ASTOpcodeName_label_636:
	lla         a0, .str.59
	ret         

	// *** Basic block 231

.ASTOpcodeName_label_641:
	lla         a0, .str.60
	ret         

	// *** Basic block 232

.ASTOpcodeName_label_646:
	lla         a0, .str.61
	ret         

	// *** Basic block 233

.ASTOpcodeName_label_651:
	lla         a0, .str.62
	ret         

	// *** Basic block 234

.ASTOpcodeName_label_656:
	lla         a0, .str.63
	ret         

	// *** Basic block 235

.ASTOpcodeName_label_661:
	lla         a0, .str.64
	ret         

	// *** Basic block 236

.ASTOpcodeName_label_666:
	lla         a0, .str.65
	ret         

	// *** Basic block 237

.ASTOpcodeName_label_671:
	lla         a0, .str.66
	ret         

	// *** Basic block 238

.ASTOpcodeName_label_676:
	lla         a0, .str.67
	ret         

	// *** Basic block 239

.ASTOpcodeName_label_681:
	lla         a0, .str.68
	ret         

	// *** Basic block 240

.ASTOpcodeName_label_686:
	lla         a0, .str.69
	ret         

	// *** Basic block 241

.ASTOpcodeName_label_691:
	lla         a0, .str.70
	ret         

	// *** Basic block 242

.ASTOpcodeName_label_696:
	lla         a0, .str.71
	ret         

	// *** Basic block 243

.ASTOpcodeName_label_701:
	lla         a0, .str.72
	ret         

	// *** Basic block 244

.ASTOpcodeName_label_706:
	lla         a0, .str.73
	ret         

	// *** Basic block 245

.ASTOpcodeName_label_711:
	lla         a0, .str.74
	ret         

	// *** Basic block 246

.ASTOpcodeName_label_716:
	lla         a0, .str.75
	ret         

	// *** Basic block 247

.ASTOpcodeName_label_721:
	lla         a0, .str.76
	ret         

	// *** Basic block 248

.ASTOpcodeName_label_726:
	lla         a0, .str.77
	ret         

	// *** Basic block 249

.ASTOpcodeName_label_731:
	lla         a0, .str.78
	ret         

	// *** Basic block 250

.ASTOpcodeName_label_736:
	lla         a0, .str.79
	ret         

	// *** Basic block 251

.ASTOpcodeName_label_741:
	lla         a0, .str.80
	ret         

	// *** Basic block 252

.ASTOpcodeName_label_746:
	lla         a0, .str.81
	ret         

	// *** Basic block 253

.ASTOpcodeName_label_751:
	lla         a0, .str.82
	ret         

	// *** Basic block 254

.ASTOpcodeName_label_756:
	lla         a0, .str.83
	ret         

	// *** Basic block 255

.ASTOpcodeName_label_761:
	lla         a0, .str.84
	ret         

	// *** Basic block 256

.ASTOpcodeName_label_766:
	lla         a0, .str.85
	ret         

	// *** Basic block 257

.ASTOpcodeName_label_771:
	lla         a0, .str.86
	ret         

	// *** Basic block 258

.ASTOpcodeName_label_776:
	lla         a0, .str.87
	ret         

	// *** Basic block 259

.ASTOpcodeName_label_781:
	lla         a0, .str.88
	ret         

	// *** Basic block 260

.ASTOpcodeName_label_786:
	lla         a0, .str.89
	ret         

	// *** Basic block 261

.ASTOpcodeName_label_791:
	lla         a0, .str.90
	ret         

	// *** Basic block 262

.ASTOpcodeName_label_796:
	lla         a0, .str.91
	ret         

	// *** Basic block 263

.ASTOpcodeName_label_801:
	lla         a0, .str.92
	ret         

	// *** Basic block 264

.ASTOpcodeName_label_806:
	lla         a0, .str.93
	ret         

	// *** Basic block 265

.ASTOpcodeName_label_811:
	lla         a0, .str.94
	ret         

	// *** Basic block 266

.ASTOpcodeName_label_816:
	lla         a0, .str.95
	ret         

	// *** Basic block 267

.ASTOpcodeName_label_821:
	lla         a0, .str.96
	ret         

	// *** Basic block 268

.ASTOpcodeName_label_826:
	lla         a0, .str.97
	ret         

	// *** Basic block 269

.ASTOpcodeName_label_831:
	lla         a0, .str.98
	ret         

	// *** Basic block 270

.ASTOpcodeName_label_836:
	lla         a0, .str.99
	ret         

	// *** Basic block 271

.ASTOpcodeName_label_841:
	lla         a0, .str.100
	ret         

	// *** Basic block 272

.ASTOpcodeName_label_846:
	lla         a0, .str.101
	ret         

	// *** Basic block 273

.ASTOpcodeName_label_851:
	lla         a0, .str.102
	ret         

	// *** Basic block 274

.ASTOpcodeName_label_856:
	lla         a0, .str.103
	ret         

	// *** Basic block 275

.ASTOpcodeName_label_861:
	lla         a0, .str.104
	ret         

	// *** Basic block 276

.ASTOpcodeName_label_866:
	lla         a0, .str.105
	ret         

	// *** Basic block 277

.ASTOpcodeName_label_871:
	lla         a0, .str.106
	ret         

	// *** Basic block 278

.ASTOpcodeName_label_876:
	lla         a0, .str.107
	ret         

	// *** Basic block 279

.ASTOpcodeName_label_881:
	lla         a0, .str.108
	ret         

	// *** Basic block 280

.ASTOpcodeName_label_886:
	lla         a0, .str.109
	ret         

	// *** Basic block 281

.ASTOpcodeName_label_891:
	lla         a0, .str.110
	ret         

	// *** Basic block 282

.ASTOpcodeName_label_896:
	lla         a0, .str.111
	ret         

	// *** Basic block 283

.ASTOpcodeName_label_901:
	lla         a0, .str.112
	ret         

	// *** Basic block 284

.ASTOpcodeName_label_906:
	lla         a0, .str.113
	ret         

	// *** Basic block 285

.ASTOpcodeName_label_911:
	lla         a0, .str.114
	ret         

	// *** Basic block 286

.ASTOpcodeName_label_916:
	lla         a0, .str.115
	ret         

	// *** Basic block 287

.ASTOpcodeName_label_921:
	lla         a0, .str.116
	ret         

	// *** Basic block 288

.ASTOpcodeName_label_926:
	lla         a0, .str.117
	ret         

	// *** Basic block 289

.ASTOpcodeName_label_931:
	lla         a0, .str.118
	ret         

	// *** Basic block 290

.ASTOpcodeName_label_936:
	lla         a0, .str.119
	ret         

	// *** Basic block 291

.ASTOpcodeName_label_941:
	lla         a0, .str.120
	ret         

	// *** Basic block 292

.ASTOpcodeName_label_946:
	lla         a0, .str.121
	ret         

	// *** Basic block 293

.ASTOpcodeName_label_951:
	lla         a0, .str.122
	ret         

	// *** Basic block 294

.ASTOpcodeName_label_956:
	lla         a0, .str.123
	ret         

	// *** Basic block 295

.ASTOpcodeName_label_961:
	lla         a0, .str.124
	ret         

	// *** Basic block 296

.ASTOpcodeName_label_966:
	lla         a0, .str.125
	ret         

	// *** Basic block 297

.ASTOpcodeName_label_971:
	lla         a0, .str.126
	ret         

	// *** Basic block 298

.ASTOpcodeName_label_976:
	lla         a0, .str.127
	ret         

	// *** Basic block 299

.ASTOpcodeName_label_981:
	lla         a0, .str.128
	ret         

	// *** Basic block 300

.ASTOpcodeName_label_986:
	lla         a0, .str.129
	ret         

	// *** Basic block 301

.ASTOpcodeName_label_991:
	lla         a0, .str.130
	ret         

	// *** Basic block 302

.ASTOpcodeName_label_996:
	lla         a0, .str.131
	ret         

	// *** Basic block 303

.ASTOpcodeName_label_1001:
	lla         a0, .str.132
	ret         

	// *** Basic block 304

.ASTOpcodeName_label_1006:
	lla         a0, .str.133
	ret         

	// *** Basic block 305

.ASTOpcodeName_label_1011:
	lla         a0, .str.134
	ret         

	// *** Basic block 306

.ASTOpcodeName_label_1016:
	lla         a0, .str.135
	ret         

	// *** Basic block 307

.ASTOpcodeName_label_1021:
	lla         a0, .str.136
	ret         

	// *** Basic block 308

.ASTOpcodeName_label_1026:
	lla         a0, .str.137
	ret         

	// *** Basic block 309

.ASTOpcodeName_label_1031:
	lla         a0, .str.138
	ret         

	// *** Basic block 310

.ASTOpcodeName_label_1036:
	lla         a0, .str.139
	ret         

	// *** Basic block 311

.ASTOpcodeName_label_1041:
	lla         a0, .str.140
	ret         

	// *** Basic block 312

.ASTOpcodeName_label_1046:
	lla         a0, .str.141
	ret         

	// *** Basic block 313

.ASTOpcodeName_label_1051:
	lla         a0, .str.142
	ret         

	// *** Basic block 314

.ASTOpcodeName_label_1056:
	lla         a0, .str.143
	ret         

	// *** Basic block 315

.ASTOpcodeName_label_1061:
	lla         a0, .str.144
	ret         

	// *** Basic block 316

.ASTOpcodeName_label_1066:
	lla         a0, .str.145
	ret         

	// *** Basic block 317

.ASTOpcodeName_label_1071:
	lla         a0, .str.146
	ret         

	// *** Basic block 318

.ASTOpcodeName_label_1076:
	lla         a0, .str.147
	ret         

	// *** Basic block 319

.ASTOpcodeName_label_1081:
	lla         a0, .str.148
	ret         

	// *** Basic block 320

.ASTOpcodeName_label_1086:
	lla         a0, .str.149
	ret         

	// *** Basic block 321

.ASTOpcodeName_label_1091:
	lla         a0, .str.150
	ret         

	// *** Basic block 322

.ASTOpcodeName_label_1096:
	lla         a0, .str.151
	ret         

	// *** Basic block 323

.ASTOpcodeName_label_1101:
	lla         a0, .str.152
	ret         

	// *** Basic block 324

.ASTOpcodeName_label_1106:
	lla         a0, .str.153
	ret         

	// *** Basic block 325

.ASTOpcodeName_label_1111:
	lla         a0, .str.154
	ret         

	// *** Basic block 326

.ASTOpcodeName_label_1116:
	lla         a0, .str.155
	ret         

	// *** Basic block 327

.ASTOpcodeName_label_1121:
	lla         a0, .str.156
	ret         

	// *** Basic block 328

.ASTOpcodeName_label_1126:
	lla         a0, .str.157
	ret         

	// *** Basic block 329

.ASTOpcodeName_label_1131:
	lla         a0, .str.158
	ret         
.func_end_ASTOpcodeName:
	.size ASTOpcodeName, .func_end_ASTOpcodeName-ASTOpcodeName

	.global ASTNodeInit
	.type ASTNodeInit, @function

ASTNodeInit:

	// *** Basic block 0

	.global printf
	.global abort
	.local next_ast_node_id
	.global ASTNodeSetType
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
	mv          s1, a4
	mv          s2, a1
	mv          s3, a0
	mv          s4, a3
	mv          s5, a2
	beq         s1, x0, .ASTNodeInit_label_43

	// *** Basic block 1

	j           .ASTNodeInit_label_58

	// *** Basic block 2

.ASTNodeInit_label_43:
	lla         a0, .str.159
	lla         a1, .str.160
	lla         a3, .str.161
	li          t0, 371		// 0x173
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

.ASTNodeInit_label_58:
	beqz        s2, .ASTNodeInit_label_62

	// *** Basic block 5

	j           .ASTNodeInit_label_77

	// *** Basic block 6

.ASTNodeInit_label_62:
	lla         a0, .str.162
	lla         a1, .str.163
	lla         a3, .str.164
	li          t0, 372		// 0x174
	mv          a2, t0
	call        printf

	// *** Basic block 7

	call        abort

	// *** Basic block 8

.ASTNodeInit_label_77:
	sw          s2, 0(s3)
	la          t0, next_ast_node_id
	lw          t1, 0(t0)
	addi        t0, t1, 1
	la          t1, next_ast_node_id
	sw          t0, 0(t1)
	sw          t1, 4(s3)
	sw          x0, 8(s3)
	sd          x0, 16(s3)
	sd          x0, 24(s3)
	sw          x0, 32(s3)
	sd          s4, 40(s3)
	sd          s1, 48(s3)
	mv          a1, s5
	mv          a0, s3
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodeSetType
.func_end_ASTNodeInit:
	.size ASTNodeInit, .func_end_ASTNodeInit-ASTNodeInit

	.local  ASTNodeBaseDelete
	.type ASTNodeBaseDelete, @function

ASTNodeBaseDelete:

	// *** Basic block 0

	.global TypeRecordDelete
	.global free
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
	ld          s2, 16(s1)
	beq         s2, x0, .ASTNodeBaseDelete_label_18

	// *** Basic block 1

	mv          a0, s2
	call        TypeRecordDelete

	// *** Basic block 2

.ASTNodeBaseDelete_label_18:
	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           free
.func_end_ASTNodeBaseDelete:
	.size ASTNodeBaseDelete, .func_end_ASTNodeBaseDelete-ASTNodeBaseDelete

	.local  SetParent
	.type SetParent, @function

SetParent:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, a1
	mv          t2, a2
	bne         t0, x0, .SetParent_label_21

	// *** Basic block 1

.SetParent_label_18:
	ret         

	// *** Basic block 2

.SetParent_label_21:
	sd          t1, 24(t0)
	sw          t2, 32(t0)
	j           .SetParent_label_18
.func_end_SetParent:
	.size SetParent, .func_end_SetParent-SetParent

	.local  Indent
	.type Indent, @function

Indent:

	// *** Basic block 0

	.global fputc
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
	mv          s3, x0
	bge         x0, s1, .Indent_label_28

	// *** Basic block 1

.Indent_label_17:
	mv          a1, s2
	li          t0, 32		// 0x20 ASCII ' '
	mv          a0, t0
	call        fputc

	// *** Basic block 2

.Indent_label_24:
	addi        s3, s3, 1
	bge         s3, s1, .Indent_label_17

	// *** Basic block 3

.Indent_label_28:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_Indent:
	.size Indent, .func_end_Indent-Indent

	.local  ValueNotUsed
	.type ValueNotUsed, @function

ValueNotUsed:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          a0, x0

	// *** Basic block 1

.ValueNotUsed_label_6:
	ret         
.func_end_ValueNotUsed:
	.size ValueNotUsed, .func_end_ValueNotUsed-ValueNotUsed

	.local  ValueAlwaysUsed
	.type ValueAlwaysUsed, @function

ValueAlwaysUsed:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 1

.ValueAlwaysUsed_label_5:
	ret         
.func_end_ValueAlwaysUsed:
	.size ValueAlwaysUsed, .func_end_ValueAlwaysUsed-ValueAlwaysUsed

	.local  ASTNodeBasePrint
	.type ASTNodeBasePrint, @function

ASTNodeBasePrint:

	// *** Basic block 0

	.local Indent
	.global fprintf
	.global ASTOpcodeName
	.global TypeRecordPrint
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
	mv          t0, a1
	mv          s1, a2
	mv          s2, a0
	mv          a1, s1
	mv          a0, t0
	call        Indent

	// *** Basic block 1

	lla         s3, .str.165
	lw          s4, 4(s2)
	lw          a0, 0(s2)
	call        ASTOpcodeName

	// *** Basic block 2

	mv          a3, a0
	mv          a2, s4
	mv          a1, s3
	mv          a0, s1
	call        fprintf

	// *** Basic block 3

	ld          s3, 16(s2)
	beq         s3, x0, .ASTNodeBasePrint_label_53

	// *** Basic block 4

	mv          a1, s1
	mv          a0, s3
	call        TypeRecordPrint

	// *** Basic block 5

.ASTNodeBasePrint_label_53:
	lla         a1, .str.166
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           fprintf
.func_end_ASTNodeBasePrint:
	.size ASTNodeBasePrint, .func_end_ASTNodeBasePrint-ASTNodeBasePrint

	.local  ASTNodeBaseCopy
	.type ASTNodeBaseCopy, @function

ASTNodeBaseCopy:

	// *** Basic block 0

	.global memcpy
	.global TypeRecordIncRef
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
	li          a2, 56		// 0x38 ASCII '8'
	call        memcpy

	// *** Basic block 1

	ld          t0, 16(s1)
	beq         t0, x0, .ASTNodeBaseCopy_label_31

	// *** Basic block 2

	mv          a0, t0
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           TypeRecordIncRef

	// *** Basic block 3

.ASTNodeBaseCopy_label_31:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ASTNodeBaseCopy:
	.size ASTNodeBaseCopy, .func_end_ASTNodeBaseCopy-ASTNodeBaseCopy

	.global NewASTNode
	.type NewASTNode, @function

NewASTNode:

	// *** Basic block 0

	.global malloc
	.global ASTNodeInit
	.local base_vtbl
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
	li          a0, 56		// 0x38 ASCII '8'
	call        malloc

	// *** Basic block 1

	mv          s4, a0
	la          a4, base_vtbl
	mv          a3, s3
	mv          a2, s2
	mv          a1, s1
	mv          a0, s4
	call        ASTNodeInit

	// *** Basic block 2

	mv          a0, s4

	// *** Basic block 3

.NewASTNode_label_35:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewASTNode:
	.size NewASTNode, .func_end_NewASTNode-NewASTNode

	.global ASTNodeDelete
	.type ASTNodeDelete, @function

ASTNodeDelete:

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
	bne         s1, x0, .ASTNodeDelete_label_20

	// *** Basic block 1

.ASTNodeDelete_label_17:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 2

.ASTNodeDelete_label_20:
	ld          t0, 48(s1)
	ld          s2, 0(t0)
	beq         s2, x0, .ASTNodeDelete_label_28

	// *** Basic block 3

	j           .ASTNodeDelete_label_46

	// *** Basic block 4

.ASTNodeDelete_label_28:
	lla         a0, .str.167
	lla         a1, .str.168
	lla         a3, .str.169
	li          t0, 442		// 0x1ba
	mv          a2, t0
	call        printf

	// *** Basic block 5

	call        abort

	// *** Basic block 6

.ASTNodeDelete_label_46:
	mv          a0, s1
	jalr         x1, s2, 0

	// *** Basic block 7

	j           .ASTNodeDelete_label_17
.func_end_ASTNodeDelete:
	.size ASTNodeDelete, .func_end_ASTNodeDelete-ASTNodeDelete

	.global ASTNodeSetType
	.type ASTNodeSetType, @function

ASTNodeSetType:

	// *** Basic block 0

	.global TypeRecordDelete
	.global TypeRecordIncRef
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
	bne         s1, x0, .ASTNodeSetType_label_19

	// *** Basic block 1

.ASTNodeSetType_label_16:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 2

.ASTNodeSetType_label_19:
	ld          s3, 16(s2)
	bne         s3, s1, .ASTNodeSetType_label_26

	// *** Basic block 3

	j           .ASTNodeSetType_label_16

	// *** Basic block 4

.ASTNodeSetType_label_26:
	beq         s3, x0, .ASTNodeSetType_label_33

	// *** Basic block 5

	mv          a0, s3
	call        TypeRecordDelete

	// *** Basic block 6

.ASTNodeSetType_label_33:
	sd          s1, 16(s2)
	mv          a0, s1
	call        TypeRecordIncRef

	// *** Basic block 7

	j           .ASTNodeSetType_label_16
.func_end_ASTNodeSetType:
	.size ASTNodeSetType, .func_end_ASTNodeSetType-ASTNodeSetType

	.global ASTNodePrint
	.type ASTNodePrint, @function

ASTNodePrint:

	// *** Basic block 0

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
	mv          s1, a0
	mv          s2, a1
	mv          s3, a2
	bne         s1, x0, .ASTNodePrint_label_27

	// *** Basic block 1

.ASTNodePrint_label_24:
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

.ASTNodePrint_label_27:
	ld          t0, 48(s1)
	ld          s4, 8(t0)
	beq         s4, x0, .ASTNodePrint_label_36

	// *** Basic block 3

	j           .ASTNodePrint_label_52

	// *** Basic block 4

.ASTNodePrint_label_36:
	lla         a0, .str.170
	lla         a1, .str.171
	lla         a3, .str.172
	li          t0, 465		// 0x1d1
	mv          a2, t0
	call        printf

	// *** Basic block 5

	call        abort

	// *** Basic block 6

.ASTNodePrint_label_52:
	mv          a2, s3
	mv          a1, s2
	mv          a0, s1
	jalr         x1, s4, 0

	// *** Basic block 7

	j           .ASTNodePrint_label_24
.func_end_ASTNodePrint:
	.size ASTNodePrint, .func_end_ASTNodePrint-ASTNodePrint

	.global ASTNodeReplaceChild
	.type ASTNodeReplaceChild, @function

ASTNodeReplaceChild:

	// *** Basic block 0

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
	mv          s1, a0
	mv          s2, a1
	mv          s3, a2
	mv          s4, a3
	bne         s1, x0, .ASTNodeReplaceChild_label_30

	// *** Basic block 1

.ASTNodeReplaceChild_label_27:
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

	// *** Basic block 2

.ASTNodeReplaceChild_label_30:
	ld          t0, 48(s1)
	ld          s5, 16(t0)
	beq         s5, x0, .ASTNodeReplaceChild_label_39

	// *** Basic block 3

	j           .ASTNodeReplaceChild_label_54

	// *** Basic block 4

.ASTNodeReplaceChild_label_39:
	lla         a0, .str.173
	lla         a1, .str.174
	lla         a3, .str.175
	li          t0, 474		// 0x1da
	mv          a2, t0
	call        printf

	// *** Basic block 5

	call        abort

	// *** Basic block 6

.ASTNodeReplaceChild_label_54:
	mv          a3, s4
	mv          a2, s3
	mv          a1, s2
	mv          a0, s1
	jalr         x1, s5, 0

	// *** Basic block 7

	j           .ASTNodeReplaceChild_label_27
.func_end_ASTNodeReplaceChild:
	.size ASTNodeReplaceChild, .func_end_ASTNodeReplaceChild-ASTNodeReplaceChild

	.global ASTNodeUsesValue
	.type ASTNodeUsesValue, @function

ASTNodeUsesValue:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, a1
	bne         t0, x0, .ASTNodeUsesValue_label_20

	// *** Basic block 1

	mv          a0, x0

	// *** Basic block 2

.ASTNodeUsesValue_label_17:
	ret         

	// *** Basic block 3

.ASTNodeUsesValue_label_20:
	ld          t2, 48(t0)
	ld          t2, 40(t2)
	bne         t2, x0, .ASTNodeUsesValue_label_31

	// *** Basic block 4

	mv          a0, x0
	j           .ASTNodeUsesValue_label_17

	// *** Basic block 5

.ASTNodeUsesValue_label_31:
	mv          a1, t1
	mv          a0, t0
	mv          t0, t2
	jr          t0
.func_end_ASTNodeUsesValue:
	.size ASTNodeUsesValue, .func_end_ASTNodeUsesValue-ASTNodeUsesValue

	.global ASTNodeClone
	.type ASTNodeClone, @function

ASTNodeClone:

	// *** Basic block 0

	.global malloc
	.local ASTNodeBaseCopy
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
	mv          s4, a3
	bne         s1, x0, .ASTNodeClone_label_29

	// *** Basic block 1

	mv          a0, x0

	// *** Basic block 2

.ASTNodeClone_label_26:
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

	// *** Basic block 3

.ASTNodeClone_label_29:
	ld          t0, 48(s1)
	ld          s5, 24(t0)
	bne         s5, x0, .ASTNodeClone_label_55

	// *** Basic block 4

	li          t0, 56		// 0x38 ASCII '8'
	mv          a0, t0
	call        malloc

	// *** Basic block 5

	mv          s6, a0
	mv          a1, s1
	mv          a0, s6
	call        ASTNodeBaseCopy

	// *** Basic block 6

	mv          a1, s3
	mv          a0, s6
	jalr         x1, s2, 0

	// *** Basic block 7

	mv          s6, a0
	j           .ASTNodeClone_label_64

	// *** Basic block 8

.ASTNodeClone_label_55:
	mv          a2, s3
	mv          a1, s2
	mv          a0, s1
	jalr         x1, s5, 0

	// *** Basic block 9

	mv          s6, a0

	// *** Basic block 10

.ASTNodeClone_label_64:
	beq         s6, x0, .ASTNodeClone_label_70

	// *** Basic block 11

	sd          s4, 24(s6)

	// *** Basic block 12

.ASTNodeClone_label_70:
	mv          a0, s6
	j           .ASTNodeClone_label_26
.func_end_ASTNodeClone:
	.size ASTNodeClone, .func_end_ASTNodeClone-ASTNodeClone

	.global ASTNodeVisit
	.type ASTNodeVisit, @function

ASTNodeVisit:

	// *** Basic block 0

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
	mv          s3, a3
	mv          s4, a2
	bne         s1, x0, .ASTNodeVisit_label_24

	// *** Basic block 1

.ASTNodeVisit_label_21:
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

	// *** Basic block 2

.ASTNodeVisit_label_24:
	ld          t0, 48(s1)
	ld          s5, 32(t0)
	bne         s5, x0, .ASTNodeVisit_label_42

	// *** Basic block 3

	mv          a3, x0
	mv          a2, s4
	mv          a1, s3
	mv          a0, s1
	jalr         x1, s2, 0

	// *** Basic block 4

	j           .ASTNodeVisit_label_21

	// *** Basic block 5

.ASTNodeVisit_label_42:
	mv          a3, s3
	mv          a2, s4
	mv          a1, s2
	mv          a0, s1
	jalr         x1, s5, 0

	// *** Basic block 6

	j           .ASTNodeVisit_label_21
.func_end_ASTNodeVisit:
	.size ASTNodeVisit, .func_end_ASTNodeVisit-ASTNodeVisit

	.global ASTNodeMove
	.type ASTNodeMove, @function

ASTNodeMove:

	// *** Basic block 0

	.global ASTNodeReplaceChild
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
	ld          a0, 24(s1)
	lw          a1, 32(s1)
	mv          a3, x0
	mv          a2, x0
	call        ASTNodeReplaceChild

	// *** Basic block 1

	sd          x0, 24(s1)
	mv          a0, s1

	// *** Basic block 2

.ASTNodeMove_label_28:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ASTNodeMove:
	.size ASTNodeMove, .func_end_ASTNodeMove-ASTNodeMove

	.global ASTNodeIsIntConstant
	.type ASTNodeIsIntConstant, @function

ASTNodeIsIntConstant:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	lw          t0, 0(a0)
	li          t1, 1		// 0x1 ASCII \x1
	beq         t0, t1, .ASTNodeIsIntConstant_label_24

	// *** Basic block 1

	li          t1, 65		// 0x41 ASCII 'A'
	beq         t0, t1, .ASTNodeIsIntConstant_label_25

	// *** Basic block 2

.ASTNodeIsIntConstant_label_20:
	mv          a0, x0
	ret         

	// *** Basic block 3

.ASTNodeIsIntConstant_label_24:

	// *** Basic block 4

.ASTNodeIsIntConstant_label_25:
	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 5

.ASTNodeIsIntConstant_label_28:
	ret         
.func_end_ASTNodeIsIntConstant:
	.size ASTNodeIsIntConstant, .func_end_ASTNodeIsIntConstant-ASTNodeIsIntConstant

	.global ASTNodeConstantValue
	.type ASTNodeConstantValue, @function

ASTNodeConstantValue:

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
	mv          s1, a0
	lw          t0, 0(s1)
	li          t1, 1		// 0x1 ASCII \x1
	beq         t0, t1, .ASTNodeConstantValue_label_49

	// *** Basic block 1

	li          t1, 65		// 0x41 ASCII 'A'
	beq         t0, t1, .ASTNodeConstantValue_label_56

	// *** Basic block 2

.ASTNodeConstantValue_label_28:
	lla         a0, .str.176
	lla         a1, .str.177
	lla         a3, .str.178
	li          t0, 547		// 0x223
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

	mv          a0, x0
	j           .ASTNodeConstantValue_label_53

	// *** Basic block 5

.ASTNodeConstantValue_label_49:
	ld          a0, 56(s1)

	// *** Basic block 6

.ASTNodeConstantValue_label_53:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 7

.ASTNodeConstantValue_label_56:
	ld          a0, 56(s1)
	j           .ASTNodeConstantValue_label_53
.func_end_ASTNodeConstantValue:
	.size ASTNodeConstantValue, .func_end_ASTNodeConstantValue-ASTNodeConstantValue

	.local  IdentifierASTNodePrint
	.type IdentifierASTNodePrint, @function

IdentifierASTNodePrint:

	// *** Basic block 0

	.local Indent
	.global fprintf
	.local ASTNodeBasePrint
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
	mv          s2, a2
	mv          s3, a0
	mv          a1, s2
	mv          a0, s1
	call        Indent

	// *** Basic block 1

	mv          s4, s3
	lla         a1, .str.179
	ld          t0, 56(s4)
	ld          a2, 16(t0)
	mv          a0, s2
	call        fprintf

	// *** Basic block 2

	addi        a1, s1, 2
	mv          a2, s2
	mv          a0, s3
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodeBasePrint
.func_end_IdentifierASTNodePrint:
	.size IdentifierASTNodePrint, .func_end_IdentifierASTNodePrint-IdentifierASTNodePrint

	.local  IdentifierASTNodeClone
	.type IdentifierASTNodeClone, @function

IdentifierASTNodeClone:

	// *** Basic block 0

	.global malloc
	.local ASTNodeBaseCopy
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
	mv          s3, a2
	mv          s4, s1
	li          a0, 64		// 0x40 ASCII '@'
	call        malloc

	// *** Basic block 1

	mv          s5, a0
	mv          a1, s1
	mv          a0, s5
	call        ASTNodeBaseCopy

	// *** Basic block 2

	ld          t0, 56(s4)
	sd          t0, 56(s5)
	mv          a1, s3
	mv          a0, s5
	mv          t0, s2
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	jr          t0
.func_end_IdentifierASTNodeClone:
	.size IdentifierASTNodeClone, .func_end_IdentifierASTNodeClone-IdentifierASTNodeClone

	.global NewIdentifierASTNode
	.type NewIdentifierASTNode, @function

NewIdentifierASTNode:

	// *** Basic block 0

	.global malloc
	.global ASTNodeInit
	.local identifier_vtbl
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
	li          a0, 64		// 0x40 ASCII '@'
	call        malloc

	// *** Basic block 1

	mv          s3, a0
	ld          a2, 40(s1)
	la          a4, identifier_vtbl
	mv          a3, s2
	li          t0, 2		// 0x2 ASCII \x2
	mv          a1, t0
	mv          a0, s3
	call        ASTNodeInit

	// *** Basic block 2

	sd          s1, 56(s3)
	mv          a0, s3

	// *** Basic block 3

.NewIdentifierASTNode_label_42:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewIdentifierASTNode:
	.size NewIdentifierASTNode, .func_end_NewIdentifierASTNode-NewIdentifierASTNode

	.local  StructMemberASTNodePrint
	.type StructMemberASTNodePrint, @function

StructMemberASTNodePrint:

	// *** Basic block 0

	.local Indent
	.global fprintf
	.local ASTNodeBasePrint
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
	mv          s2, a2
	mv          s3, a0
	mv          a1, s2
	mv          a0, s1
	call        Indent

	// *** Basic block 1

	mv          s4, s3
	lla         a1, .str.180
	ld          t0, 56(s4)
	ld          t1, 0(t0)
	ld          a2, 16(t1)
	lw          a3, 8(t0)
	mv          a0, s2
	call        fprintf

	// *** Basic block 2

	addi        a1, s1, 2
	mv          a2, s2
	mv          a0, s3
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodeBasePrint
.func_end_StructMemberASTNodePrint:
	.size StructMemberASTNodePrint, .func_end_StructMemberASTNodePrint-StructMemberASTNodePrint

	.local  StructMemberASTNodeClone
	.type StructMemberASTNodeClone, @function

StructMemberASTNodeClone:

	// *** Basic block 0

	.global malloc
	.local ASTNodeBaseCopy
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
	mv          s3, a2
	mv          s4, s1
	li          a0, 64		// 0x40 ASCII '@'
	call        malloc

	// *** Basic block 1

	mv          s5, a0
	mv          a1, s1
	mv          a0, s5
	call        ASTNodeBaseCopy

	// *** Basic block 2

	ld          t0, 56(s4)
	sd          t0, 56(s5)
	mv          a1, s3
	mv          a0, s5
	mv          t0, s2
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	jr          t0
.func_end_StructMemberASTNodeClone:
	.size StructMemberASTNodeClone, .func_end_StructMemberASTNodeClone-StructMemberASTNodeClone

	.global NewStructMemberASTNode
	.type NewStructMemberASTNode, @function

NewStructMemberASTNode:

	// *** Basic block 0

	.global malloc
	.global ASTNodeInit
	.local struct_member_vtbl
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
	li          a0, 64		// 0x40 ASCII '@'
	call        malloc

	// *** Basic block 1

	mv          s3, a0
	ld          t0, 0(s1)
	ld          a2, 40(t0)
	la          a4, struct_member_vtbl
	mv          a3, s2
	li          t0, 73		// 0x49 ASCII 'I'
	mv          a1, t0
	mv          a0, s3
	call        ASTNodeInit

	// *** Basic block 2

	sd          s1, 56(s3)
	mv          a0, s3

	// *** Basic block 3

.NewStructMemberASTNode_label_43:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewStructMemberASTNode:
	.size NewStructMemberASTNode, .func_end_NewStructMemberASTNode-NewStructMemberASTNode

	.local  ConstantASTNodePrint
	.type ConstantASTNodePrint, @function

ConstantASTNodePrint:

	// *** Basic block 0

	.local Indent
	.global fprintf
	.global StringEscape
	.global StringDestruct
	.global ASTNodePrint
	addi sp, sp, -112
	// Saved return address (offset 104) and frame pointer (offset 96)
	sd ra, 104(sp)
	sd s0, 96(sp)
	addi s0, sp, 112
	// Local vars at offset -64(s0)
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
	mv          a1, s2
	mv          a0, s1
	call        Indent

	// *** Basic block 1

	mv          s4, s3
	lw          s5, 0(s4)
	li          t0, 1		// 0x1 ASCII \x1
	beq         s5, t0, .ConstantASTNodePrint_label_97

	// *** Basic block 2

	li          t0, 3		// 0x3 ASCII \x3
	beq         s5, t0, .ConstantASTNodePrint_label_120

	// *** Basic block 3

	li          t0, 4		// 0x4 ASCII \x4
	beq         s5, t0, .ConstantASTNodePrint_label_121

	// *** Basic block 4

	li          t0, 5		// 0x5 ASCII \x5
	beq         s5, t0, .ConstantASTNodePrint_label_155

	// *** Basic block 5

	li          t0, 6		// 0x6 ASCII \x6
	beq         s5, t0, .ConstantASTNodePrint_label_108

	// *** Basic block 6

	li          t0, 27		// 0x1b ASCII \x1b
	beq         s5, t0, .ConstantASTNodePrint_label_154

	// *** Basic block 7

	li          t0, 65		// 0x41 ASCII 'A'
	beq         s5, t0, .ConstantASTNodePrint_label_179

	// *** Basic block 8

	li          t0, 81		// 0x51 ASCII 'Q'
	beq         s5, t0, .ConstantASTNodePrint_label_166

	// *** Basic block 9

.ConstantASTNodePrint_label_87:
	lla         a1, .str.187
	mv          a2, s5
	mv          a0, s2
	call        fprintf

	// *** Basic block 10

	j           .ConstantASTNodePrint_label_197

	// *** Basic block 11

.ConstantASTNodePrint_label_97:
	lla         a1, .str.181
	ld          a2, 56(s4)
	mv          a0, s2
	call        fprintf

	// *** Basic block 12

	j           .ConstantASTNodePrint_label_197

	// *** Basic block 13

.ConstantASTNodePrint_label_108:
	lla         a1, .str.182
	fld         fa0, 56(s4)
	mv          a0, s2
	call        fprintf

	// *** Basic block 14

	j           .ConstantASTNodePrint_label_197

	// *** Basic block 15

.ConstantASTNodePrint_label_120:

	// *** Basic block 16

.ConstantASTNodePrint_label_121:
	sd          x0, -64(s0)
	sd          x0, -56(s0)
	sd          x0, -48(s0)
	sd          x0, -40(s0)
	sd          x0, -32(s0)
	sb          x0, -64(s0)
	ld          a0, 56(s4)
	addi        a1, s0, -64
	call        StringEscape

	// *** Basic block 17

	lla         a1, .str.183
	addi        t0, s0, -64
	ld          a2, 16(t0)
	mv          a0, s2
	call        fprintf

	// *** Basic block 18

	addi        a0, s0, -64
	call        StringDestruct

	// *** Basic block 19

	j           .ConstantASTNodePrint_label_197

	// *** Basic block 20

.ConstantASTNodePrint_label_154:

	// *** Basic block 21

.ConstantASTNodePrint_label_155:
	lla         a1, .str.184
	ld          a2, 56(s4)
	mv          a0, s2
	call        fprintf

	// *** Basic block 22

	j           .ConstantASTNodePrint_label_197

	// *** Basic block 23

.ConstantASTNodePrint_label_166:
	lla         a1, .str.185
	ld          t0, 56(s4)
	ld          a2, 16(t0)
	mv          a0, s2
	call        fprintf

	// *** Basic block 24

	j           .ConstantASTNodePrint_label_197

	// *** Basic block 25

.ConstantASTNodePrint_label_179:
	lla         a1, .str.186
	mv          a0, s2
	call        fprintf

	// *** Basic block 26

	mv          s5, s3
	ld          a0, 64(s5)
	addi        a1, s1, 2
	mv          a2, s2
	call        ASTNodePrint

	// *** Basic block 27

	j           .ConstantASTNodePrint_label_197

	// *** Basic block 28

.ConstantASTNodePrint_label_197:
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
.func_end_ConstantASTNodePrint:
	.size ConstantASTNodePrint, .func_end_ConstantASTNodePrint-ConstantASTNodePrint

	.local  ConstantASTNodeClone
	.type ConstantASTNodeClone, @function

ConstantASTNodeClone:

	// *** Basic block 0

	.global malloc
	.local ASTNodeBaseCopy
	.global memcpy
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
	mv          s3, a2
	mv          s4, s1
	li          a0, 64		// 0x40 ASCII '@'
	call        malloc

	// *** Basic block 1

	mv          s5, a0
	mv          a1, s1
	mv          a0, s5
	call        ASTNodeBaseCopy

	// *** Basic block 2

	addi        a0, s5, 56
	addi        a1, s4, 56
	li          t0, 8		// 0x8 ASCII \x8
	mv          a2, t0
	call        memcpy

	// *** Basic block 3

	mv          a1, s3
	mv          a0, s5
	mv          t0, s2
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	jr          t0
.func_end_ConstantASTNodeClone:
	.size ConstantASTNodeClone, .func_end_ConstantASTNodeClone-ConstantASTNodeClone

	.global IntConstantASTNodeInit
	.type IntConstantASTNodeInit, @function

IntConstantASTNodeInit:

	// *** Basic block 0

	.global ASTNodeInit
	.local constant_vtbl
	// Leaf procedure, no stack frame generated
	la          a4, constant_vtbl
	li          a1, 1		// 0x1 ASCII \x1
	j           ASTNodeInit
.func_end_IntConstantASTNodeInit:
	.size IntConstantASTNodeInit, .func_end_IntConstantASTNodeInit-IntConstantASTNodeInit

	.global NewIntConstantASTNode
	.type NewIntConstantASTNode, @function

NewIntConstantASTNode:

	// *** Basic block 0

	.global malloc
	.global IntConstantASTNodeInit
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
	li          a0, 64		// 0x40 ASCII '@'
	call        malloc

	// *** Basic block 1

	mv          s4, a0
	mv          a3, s3
	mv          a2, s2
	mv          a1, s1
	mv          a0, s4
	call        IntConstantASTNodeInit

	// *** Basic block 2

	mv          a0, s4

	// *** Basic block 3

.NewIntConstantASTNode_label_31:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewIntConstantASTNode:
	.size NewIntConstantASTNode, .func_end_NewIntConstantASTNode-NewIntConstantASTNode

	.global NewRealConstantASTNode
	.type NewRealConstantASTNode, @function

NewRealConstantASTNode:

	// *** Basic block 0

	.global malloc
	.global ASTNodeInit
	.local constant_vtbl
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
	// Saved floating point registers.
	fsd fs0, 0(sp)
	// End of stack frame
	mv          s1, a0
	mv          s2, a1
	fmv.d       fs0, fa0
	li          a0, 64		// 0x40 ASCII '@'
	call        malloc

	// *** Basic block 1

	mv          s3, a0
	la          a4, constant_vtbl
	mv          a3, s2
	mv          a2, s1
	li          t0, 6		// 0x6 ASCII \x6
	mv          a1, t0
	mv          a0, s3
	call        ASTNodeInit

	// *** Basic block 2

	fsd         fs0, 56(s3)
	mv          a0, s3

	// *** Basic block 3

.NewRealConstantASTNode_label_43:
	// Restored registers.
	fld fs0, 24(sp)
	ld s1, 16(sp)
	ld s2, 8(sp)
	ld s3, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewRealConstantASTNode:
	.size NewRealConstantASTNode, .func_end_NewRealConstantASTNode-NewRealConstantASTNode

	.global NewStringConstantASTNode
	.type NewStringConstantASTNode, @function

NewStringConstantASTNode:

	// *** Basic block 0

	.global malloc
	.global ASTNodeInit
	.local constant_vtbl
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
	mv          s2, a2
	mv          s3, a0
	li          a0, 64		// 0x40 ASCII '@'
	call        malloc

	// *** Basic block 1

	mv          s4, a0
	la          a4, constant_vtbl
	mv          a3, s2
	mv          a2, s1
	li          t0, 3		// 0x3 ASCII \x3
	mv          a1, t0
	mv          a0, s4
	call        ASTNodeInit

	// *** Basic block 2

	sd          s3, 56(s4)
	mv          a0, s4

	// *** Basic block 3

.NewStringConstantASTNode_label_42:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewStringConstantASTNode:
	.size NewStringConstantASTNode, .func_end_NewStringConstantASTNode-NewStringConstantASTNode

	.global NewWideStringConstantASTNode
	.type NewWideStringConstantASTNode, @function

NewWideStringConstantASTNode:

	// *** Basic block 0

	.global malloc
	.global ASTNodeInit
	.local constant_vtbl
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
	mv          s2, a2
	mv          s3, a0
	li          a0, 64		// 0x40 ASCII '@'
	call        malloc

	// *** Basic block 1

	mv          s4, a0
	la          a4, constant_vtbl
	mv          a3, s2
	mv          a2, s1
	li          t0, 4		// 0x4 ASCII \x4
	mv          a1, t0
	mv          a0, s4
	call        ASTNodeInit

	// *** Basic block 2

	sd          s3, 56(s4)
	mv          a0, s4

	// *** Basic block 3

.NewWideStringConstantASTNode_label_42:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewWideStringConstantASTNode:
	.size NewWideStringConstantASTNode, .func_end_NewWideStringConstantASTNode-NewWideStringConstantASTNode

	.global NewCharConstantASTNode
	.type NewCharConstantASTNode, @function

NewCharConstantASTNode:

	// *** Basic block 0

	.global malloc
	.global ASTNodeInit
	.local constant_vtbl
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
	mv          s2, a2
	mv          s3, a0
	li          a0, 64		// 0x40 ASCII '@'
	call        malloc

	// *** Basic block 1

	mv          s4, a0
	la          a4, constant_vtbl
	mv          a3, s2
	mv          a2, s1
	li          t0, 5		// 0x5 ASCII \x5
	mv          a1, t0
	mv          a0, s4
	call        ASTNodeInit

	// *** Basic block 2

	sd          s3, 56(s4)
	mv          a0, s4

	// *** Basic block 3

.NewCharConstantASTNode_label_42:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewCharConstantASTNode:
	.size NewCharConstantASTNode, .func_end_NewCharConstantASTNode-NewCharConstantASTNode

	.local  UnaryASTNodeDelete
	.type UnaryASTNodeDelete, @function

UnaryASTNodeDelete:

	// *** Basic block 0

	.global ASTNodeDelete
	.local ASTNodeBaseDelete
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
	mv          t0, s1
	ld          s2, 56(t0)
	beq         s2, x0, .UnaryASTNodeDelete_label_20

	// *** Basic block 1

	mv          a0, s2
	call        ASTNodeDelete

	// *** Basic block 2

.UnaryASTNodeDelete_label_20:
	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodeBaseDelete
.func_end_UnaryASTNodeDelete:
	.size UnaryASTNodeDelete, .func_end_UnaryASTNodeDelete-UnaryASTNodeDelete

	.local  UnaryASTNodePrint
	.type UnaryASTNodePrint, @function

UnaryASTNodePrint:

	// *** Basic block 0

	.local ASTNodeBasePrint
	.global ASTNodePrint
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
	call        ASTNodeBasePrint

	// *** Basic block 1

	mv          s4, s1
	ld          t0, 56(s4)
	beq         t0, x0, .UnaryASTNodePrint_label_39

	// *** Basic block 2

	addi        a1, s2, 2
	mv          a2, s3
	mv          a0, t0
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodePrint

	// *** Basic block 3

.UnaryASTNodePrint_label_39:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_UnaryASTNodePrint:
	.size UnaryASTNodePrint, .func_end_UnaryASTNodePrint-UnaryASTNodePrint

	.local  UnaryASTNodeReplaceChild
	.type UnaryASTNodeReplaceChild, @function

UnaryASTNodeReplaceChild:

	// *** Basic block 0

	.local SetParent
	.global ASTNodeDelete
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
	mv          t0, a0
	mv          t1, a2
	mv          t2, a1
	mv          s1, a3
	mv          t3, t0
	ld          s2, 56(t3)
	sd          t1, 56(t3)
	mv          a2, t2
	mv          a1, t0
	mv          a0, t1
	call        SetParent

	// *** Basic block 1

	beqz        s1, .UnaryASTNodeReplaceChild_label_37

	// *** Basic block 2

	mv          a0, s2
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodeDelete

	// *** Basic block 3

.UnaryASTNodeReplaceChild_label_37:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_UnaryASTNodeReplaceChild:
	.size UnaryASTNodeReplaceChild, .func_end_UnaryASTNodeReplaceChild-UnaryASTNodeReplaceChild

	.local  UnaryASTNodeClone
	.type UnaryASTNodeClone, @function

UnaryASTNodeClone:

	// *** Basic block 0

	.global malloc
	.local ASTNodeBaseCopy
	.global ASTNodeClone
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
	mv          s3, a2
	mv          s4, s1
	li          a0, 64		// 0x40 ASCII '@'
	call        malloc

	// *** Basic block 1

	mv          s5, a0
	mv          a1, s1
	mv          a0, s5
	call        ASTNodeBaseCopy

	// *** Basic block 2

	ld          a0, 56(s4)
	mv          a3, s5
	mv          a2, s3
	mv          a1, s2
	call        ASTNodeClone

	// *** Basic block 3

	sd          a0, 56(s5)
	mv          a1, s3
	mv          a0, s5
	mv          t0, s2
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	jr          t0
.func_end_UnaryASTNodeClone:
	.size UnaryASTNodeClone, .func_end_UnaryASTNodeClone-UnaryASTNodeClone

	.local  UnaryASTNodeVisit
	.type UnaryASTNodeVisit, @function

UnaryASTNodeVisit:

	// *** Basic block 0

	.global ASTNodeVisit
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
	mv          s3, a3
	mv          s4, a2
	mv          s5, s1
	mv          a3, x0
	mv          a1, s3
	jalr         x1, s2, 0

	// *** Basic block 1

	ld          a0, 56(s5)
	mv          a3, s3
	mv          a2, x0
	mv          a1, s2
	call        ASTNodeVisit

	// *** Basic block 2

	li          t0, 1		// 0x1 ASCII \x1
	mv          a3, t0
	mv          a2, s4
	mv          a1, s3
	mv          a0, s1
	mv          t0, s2
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	jr          t0
.func_end_UnaryASTNodeVisit:
	.size UnaryASTNodeVisit, .func_end_UnaryASTNodeVisit-UnaryASTNodeVisit

	.global NewUnaryASTNode
	.type NewUnaryASTNode, @function

NewUnaryASTNode:

	// *** Basic block 0

	.global malloc
	.global ASTNodeInit
	.local unary_vtbl
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
	mv          s3, a2
	mv          s4, a3
	li          a0, 64		// 0x40 ASCII '@'
	call        malloc

	// *** Basic block 1

	mv          s5, a0
	la          a4, unary_vtbl
	mv          a3, s3
	mv          a2, s2
	mv          a1, s1
	mv          a0, s5
	call        ASTNodeInit

	// *** Basic block 2

	sd          s4, 56(s5)
	sd          s5, 24(s4)
	mv          a0, s5

	// *** Basic block 3

.NewUnaryASTNode_label_45:
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
.func_end_NewUnaryASTNode:
	.size NewUnaryASTNode, .func_end_NewUnaryASTNode-NewUnaryASTNode

	.local  BinaryASTNodeDelete
	.type BinaryASTNodeDelete, @function

BinaryASTNodeDelete:

	// *** Basic block 0

	.global ASTNodeDelete
	.local ASTNodeBaseDelete
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
	mv          s2, s1
	ld          s3, 56(s2)
	beq         s3, x0, .BinaryASTNodeDelete_label_21

	// *** Basic block 1

	mv          a0, s3
	call        ASTNodeDelete

	// *** Basic block 2

.BinaryASTNodeDelete_label_21:
	ld          s3, 64(s2)
	beq         s3, x0, .BinaryASTNodeDelete_label_30

	// *** Basic block 3

	mv          a0, s3
	call        ASTNodeDelete

	// *** Basic block 4

.BinaryASTNodeDelete_label_30:
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodeBaseDelete
.func_end_BinaryASTNodeDelete:
	.size BinaryASTNodeDelete, .func_end_BinaryASTNodeDelete-BinaryASTNodeDelete

	.local  BinaryASTNodePrint
	.type BinaryASTNodePrint, @function

BinaryASTNodePrint:

	// *** Basic block 0

	.global ASTNodePrint
	.local ASTNodeBasePrint
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
	mv          s3, a2
	mv          s4, s1
	ld          s5, 64(s4)
	beq         s5, x0, .BinaryASTNodePrint_label_33

	// *** Basic block 1

	addi        a1, s2, 2
	mv          a2, s3
	mv          a0, s5
	call        ASTNodePrint

	// *** Basic block 2

.BinaryASTNodePrint_label_33:
	mv          a2, s3
	mv          a1, s2
	mv          a0, s1
	call        ASTNodeBasePrint

	// *** Basic block 3

	ld          t0, 56(s4)
	beq         t0, x0, .BinaryASTNodePrint_label_54

	// *** Basic block 4

	addi        a1, s2, 1
	mv          a2, s3
	mv          a0, t0
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodePrint

	// *** Basic block 5

.BinaryASTNodePrint_label_54:
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
.func_end_BinaryASTNodePrint:
	.size BinaryASTNodePrint, .func_end_BinaryASTNodePrint-BinaryASTNodePrint

	.local  BinaryASTNodeReplaceChild
	.type BinaryASTNodeReplaceChild, @function

BinaryASTNodeReplaceChild:

	// *** Basic block 0

	.local SetParent
	.global ASTNodeDelete
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
	mv          s4, a3
	mv          s5, s1
	mv          s6, x0
	blt         s2, x0, .BinaryASTNodeReplaceChild_label_51

	// *** Basic block 1

	li          t0, 1		// 0x1 ASCII \x1
	blt         t0, s2, .BinaryASTNodeReplaceChild_label_51

	// *** Basic block 2

	slli        t0, s2, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 3

	j           .BinaryASTNodeReplaceChild_label_39

	// *** Basic block 4

	j           .BinaryASTNodeReplaceChild_label_45

	// *** Basic block 5

.BinaryASTNodeReplaceChild_label_39:
	ld          s6, 56(s5)
	sd          s3, 56(s5)
	j           .BinaryASTNodeReplaceChild_label_51

	// *** Basic block 6

.BinaryASTNodeReplaceChild_label_45:
	ld          s6, 64(s5)
	sd          s3, 64(s5)
	j           .BinaryASTNodeReplaceChild_label_51

	// *** Basic block 7

.BinaryASTNodeReplaceChild_label_51:
	mv          a2, s2
	mv          a1, s1
	mv          a0, s3
	call        SetParent

	// *** Basic block 8

	beqz        s4, .BinaryASTNodeReplaceChild_label_64

	// *** Basic block 9

	mv          a0, s6
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
	j           ASTNodeDelete

	// *** Basic block 10

.BinaryASTNodeReplaceChild_label_64:
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
.func_end_BinaryASTNodeReplaceChild:
	.size BinaryASTNodeReplaceChild, .func_end_BinaryASTNodeReplaceChild-BinaryASTNodeReplaceChild

	.local  BinaryASTNodeClone
	.type BinaryASTNodeClone, @function

BinaryASTNodeClone:

	// *** Basic block 0

	.global malloc
	.local ASTNodeBaseCopy
	.global ASTNodeClone
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
	mv          s3, a2
	mv          s4, s1
	li          a0, 72		// 0x48 ASCII 'H'
	call        malloc

	// *** Basic block 1

	mv          s5, a0
	mv          a1, s1
	mv          a0, s5
	call        ASTNodeBaseCopy

	// *** Basic block 2

	ld          a0, 56(s4)
	mv          a3, s5
	mv          a2, s3
	mv          a1, s2
	call        ASTNodeClone

	// *** Basic block 3

	sd          a0, 56(s5)
	ld          a0, 64(s4)
	mv          a3, s5
	mv          a2, s3
	mv          a1, s2
	call        ASTNodeClone

	// *** Basic block 4

	sd          a0, 64(s5)
	mv          a1, s3
	mv          a0, s5
	mv          t0, s2
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	jr          t0
.func_end_BinaryASTNodeClone:
	.size BinaryASTNodeClone, .func_end_BinaryASTNodeClone-BinaryASTNodeClone

	.local  BinaryASTNodeVisit
	.type BinaryASTNodeVisit, @function

BinaryASTNodeVisit:

	// *** Basic block 0

	.global ASTNodeVisit
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
	mv          s3, a3
	mv          s4, a2
	mv          s5, s1
	mv          a3, x0
	mv          a1, s3
	jalr         x1, s2, 0

	// *** Basic block 1

	ld          a0, 56(s5)
	mv          a3, s3
	mv          a2, x0
	mv          a1, s2
	call        ASTNodeVisit

	// *** Basic block 2

	ld          a0, 64(s5)
	mv          a3, s3
	li          s6, 1		// 0x1 ASCII \x1
	mv          a2, s6
	mv          a1, s2
	call        ASTNodeVisit

	// *** Basic block 3

	mv          a3, s6
	mv          a2, s4
	mv          a1, s3
	mv          a0, s1
	mv          t0, s2
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
	jr          t0
.func_end_BinaryASTNodeVisit:
	.size BinaryASTNodeVisit, .func_end_BinaryASTNodeVisit-BinaryASTNodeVisit

	.global NewBinaryASTNode
	.type NewBinaryASTNode, @function

NewBinaryASTNode:

	// *** Basic block 0

	.global malloc
	.global ASTNodeInit
	.local binary_vtbl
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
	mv          s4, a3
	mv          s5, a4
	li          a0, 72		// 0x48 ASCII 'H'
	call        malloc

	// *** Basic block 1

	mv          s6, a0
	la          a4, binary_vtbl
	mv          a3, s3
	mv          a2, s2
	mv          a1, s1
	mv          a0, s6
	call        ASTNodeInit

	// *** Basic block 2

	sd          s4, 56(s6)
	sd          s6, 24(s4)
	sw          x0, 32(s4)
	sd          s5, 64(s6)
	sd          s6, 24(s5)
	li          t0, 1		// 0x1 ASCII \x1
	sw          t0, 32(s5)
	mv          a0, s6

	// *** Basic block 3

.NewBinaryASTNode_label_59:
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
.func_end_NewBinaryASTNode:
	.size NewBinaryASTNode, .func_end_NewBinaryASTNode-NewBinaryASTNode

	.local  InlineCallASTNodeDelete
	.type InlineCallASTNodeDelete, @function

InlineCallASTNodeDelete:

	// *** Basic block 0

	.global ASTNodeDelete
	.local ASTNodeBaseDelete
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
	mv          s2, s1
	ld          s3, 56(s2)
	beq         s3, x0, .InlineCallASTNodeDelete_label_21

	// *** Basic block 1

	mv          a0, s3
	call        ASTNodeDelete

	// *** Basic block 2

.InlineCallASTNodeDelete_label_21:
	ld          s3, 64(s2)
	beq         s3, x0, .InlineCallASTNodeDelete_label_30

	// *** Basic block 3

	mv          a0, s3
	call        ASTNodeDelete

	// *** Basic block 4

.InlineCallASTNodeDelete_label_30:
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodeBaseDelete
.func_end_InlineCallASTNodeDelete:
	.size InlineCallASTNodeDelete, .func_end_InlineCallASTNodeDelete-InlineCallASTNodeDelete

	.local  InlineCallASTNodePrint
	.type InlineCallASTNodePrint, @function

InlineCallASTNodePrint:

	// *** Basic block 0

	.global ASTNodePrint
	.local ASTNodeBasePrint
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
	mv          s3, a2
	mv          s4, s1
	ld          s5, 56(s4)
	beq         s5, x0, .InlineCallASTNodePrint_label_33

	// *** Basic block 1

	addi        a1, s2, 2
	mv          a2, s3
	mv          a0, s5
	call        ASTNodePrint

	// *** Basic block 2

.InlineCallASTNodePrint_label_33:
	mv          a2, s3
	mv          a1, s2
	mv          a0, s1
	call        ASTNodeBasePrint

	// *** Basic block 3

	ld          t0, 64(s4)
	beq         t0, x0, .InlineCallASTNodePrint_label_54

	// *** Basic block 4

	addi        a1, s2, 1
	mv          a2, s3
	mv          a0, t0
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodePrint

	// *** Basic block 5

.InlineCallASTNodePrint_label_54:
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
.func_end_InlineCallASTNodePrint:
	.size InlineCallASTNodePrint, .func_end_InlineCallASTNodePrint-InlineCallASTNodePrint

	.local  InlineCallASTNodeReplaceChild
	.type InlineCallASTNodeReplaceChild, @function

InlineCallASTNodeReplaceChild:

	// *** Basic block 0

	.local SetParent
	.global ASTNodeDelete
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
	mv          s4, a3
	mv          s5, s1
	mv          s6, x0
	blt         s2, x0, .InlineCallASTNodeReplaceChild_label_51

	// *** Basic block 1

	li          t0, 1		// 0x1 ASCII \x1
	blt         t0, s2, .InlineCallASTNodeReplaceChild_label_51

	// *** Basic block 2

	slli        t0, s2, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 3

	j           .InlineCallASTNodeReplaceChild_label_39

	// *** Basic block 4

	j           .InlineCallASTNodeReplaceChild_label_45

	// *** Basic block 5

.InlineCallASTNodeReplaceChild_label_39:
	ld          s6, 56(s5)
	sd          s3, 56(s5)
	j           .InlineCallASTNodeReplaceChild_label_51

	// *** Basic block 6

.InlineCallASTNodeReplaceChild_label_45:
	ld          s6, 64(s5)
	sd          s3, 64(s5)
	j           .InlineCallASTNodeReplaceChild_label_51

	// *** Basic block 7

.InlineCallASTNodeReplaceChild_label_51:
	mv          a2, s2
	mv          a1, s1
	mv          a0, s3
	call        SetParent

	// *** Basic block 8

	beqz        s4, .InlineCallASTNodeReplaceChild_label_64

	// *** Basic block 9

	mv          a0, s6
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
	j           ASTNodeDelete

	// *** Basic block 10

.InlineCallASTNodeReplaceChild_label_64:
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
.func_end_InlineCallASTNodeReplaceChild:
	.size InlineCallASTNodeReplaceChild, .func_end_InlineCallASTNodeReplaceChild-InlineCallASTNodeReplaceChild

	.local  InlineCallASTNodeClone
	.type InlineCallASTNodeClone, @function

InlineCallASTNodeClone:

	// *** Basic block 0

	.global malloc
	.local ASTNodeBaseCopy
	.global ASTNodeClone
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
	mv          s3, a2
	mv          s4, s1
	li          a0, 72		// 0x48 ASCII 'H'
	call        malloc

	// *** Basic block 1

	mv          s5, a0
	mv          a1, s1
	mv          a0, s5
	call        ASTNodeBaseCopy

	// *** Basic block 2

	ld          a0, 56(s4)
	mv          a3, s5
	mv          a2, s3
	mv          a1, s2
	call        ASTNodeClone

	// *** Basic block 3

	sd          a0, 56(s5)
	ld          a0, 64(s4)
	mv          a3, s5
	mv          a2, s3
	mv          a1, s2
	call        ASTNodeClone

	// *** Basic block 4

	sd          a0, 64(s5)
	mv          a1, s3
	mv          a0, s5
	mv          t0, s2
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	jr          t0
.func_end_InlineCallASTNodeClone:
	.size InlineCallASTNodeClone, .func_end_InlineCallASTNodeClone-InlineCallASTNodeClone

	.local  InlineCallASTNodeVisit
	.type InlineCallASTNodeVisit, @function

InlineCallASTNodeVisit:

	// *** Basic block 0

	.global ASTNodeVisit
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
	mv          s3, a3
	mv          s4, a2
	mv          s5, s1
	mv          a3, x0
	mv          a1, s3
	jalr         x1, s2, 0

	// *** Basic block 1

	ld          a0, 56(s5)
	mv          a3, s3
	mv          a2, x0
	mv          a1, s2
	call        ASTNodeVisit

	// *** Basic block 2

	ld          a0, 64(s5)
	mv          a3, s3
	li          s6, 1		// 0x1 ASCII \x1
	mv          a2, s6
	mv          a1, s2
	call        ASTNodeVisit

	// *** Basic block 3

	mv          a3, s6
	mv          a2, s4
	mv          a1, s3
	mv          a0, s1
	mv          t0, s2
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
	jr          t0
.func_end_InlineCallASTNodeVisit:
	.size InlineCallASTNodeVisit, .func_end_InlineCallASTNodeVisit-InlineCallASTNodeVisit

	.global NewInlineCallASTNode
	.type NewInlineCallASTNode, @function

NewInlineCallASTNode:

	// *** Basic block 0

	.global malloc
	.global ASTNodeInit
	.local inline_call_vtbl
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
	mv          s3, a2
	mv          s4, a3
	li          a0, 72		// 0x48 ASCII 'H'
	call        malloc

	// *** Basic block 1

	mv          s5, a0
	la          a4, inline_call_vtbl
	mv          a3, s2
	mv          a2, s1
	li          t0, 47		// 0x2f ASCII '/'
	mv          a1, t0
	mv          a0, s5
	call        ASTNodeInit

	// *** Basic block 2

	sd          s3, 56(s5)
	sd          s5, 24(s3)
	sw          x0, 32(s3)
	sd          s4, 64(s5)
	beq         s4, x0, .NewInlineCallASTNode_label_60

	// *** Basic block 3

	sd          s5, 24(s4)
	li          t0, 1		// 0x1 ASCII \x1
	sw          t0, 32(s4)

	// *** Basic block 4

.NewInlineCallASTNode_label_60:
	mv          a0, s5

	// *** Basic block 5

.NewInlineCallASTNode_label_63:
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
.func_end_NewInlineCallASTNode:
	.size NewInlineCallASTNode, .func_end_NewInlineCallASTNode-NewInlineCallASTNode

	.local  VectorASTNodeDelete
	.type VectorASTNodeDelete, @function

VectorASTNodeDelete:

	// *** Basic block 0

	.global ASTNodeDelete
	.global VectorDestruct
	.local ASTNodeBaseDelete
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
	mv          s2, s1
	ld          s3, 56(s2)
	beq         s3, x0, .VectorASTNodeDelete_label_25

	// *** Basic block 1

	mv          a0, s3
	call        ASTNodeDelete

	// *** Basic block 2

.VectorASTNodeDelete_label_25:
	mv          s3, x0
	ld          s4, 64(s2)
	ld          s5, 8(s4)
	bge         x0, s5, .VectorASTNodeDelete_label_52

	// *** Basic block 3

	ld          t0, 0(s4)

	// *** Basic block 4

.VectorASTNodeDelete_label_35:
	slli        t1, s3, 3
	add         t0, t0, t1
	ld          s2, 0(t0)
	beq         s2, x0, .VectorASTNodeDelete_label_47

	// *** Basic block 5

	mv          a0, s2
	call        ASTNodeDelete

	// *** Basic block 6

.VectorASTNodeDelete_label_47:

	// *** Basic block 7

.VectorASTNodeDelete_label_48:
	addi        s3, s3, 1
	bge         s3, s5, .VectorASTNodeDelete_label_35

	// *** Basic block 8

.VectorASTNodeDelete_label_52:
	mv          a0, s4
	call        VectorDestruct

	// *** Basic block 9

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
	j           ASTNodeBaseDelete
.func_end_VectorASTNodeDelete:
	.size VectorASTNodeDelete, .func_end_VectorASTNodeDelete-VectorASTNodeDelete

	.local  VectorASTNodePrint
	.type VectorASTNodePrint, @function

VectorASTNodePrint:

	// *** Basic block 0

	.global ASTNodePrint
	.local ASTNodeBasePrint
	.local Indent
	.global fprintf
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
	mv          s4, s1
	ld          s5, 56(s4)
	beq         s5, x0, .VectorASTNodePrint_label_39

	// *** Basic block 1

	addi        a1, s2, 2
	mv          a2, s3
	mv          a0, s5
	call        ASTNodePrint

	// *** Basic block 2

.VectorASTNodePrint_label_39:
	mv          a2, s3
	mv          a1, s2
	mv          a0, s1
	call        ASTNodeBasePrint

	// *** Basic block 3

	addi        s5, s2, 2
	addi        s6, s2, 4
	ld          t0, 64(s4)
	ld          s7, 8(t0)
	mv          s8, x0
	bge         x0, s7, .VectorASTNodePrint_label_87

	// *** Basic block 4

	ld          s1, 0(t0)

	// *** Basic block 5

.VectorASTNodePrint_label_59:
	mv          a1, s3
	mv          a0, s5
	call        Indent

	// *** Basic block 6

	lla         a1, .str.188
	mv          a2, s8
	mv          a0, s3
	call        fprintf

	// *** Basic block 7

	slli        t0, s8, 3
	add         t0, s1, t0
	ld          a0, 0(t0)
	mv          a2, s3
	mv          a1, s6
	call        ASTNodePrint

	// *** Basic block 8

.VectorASTNodePrint_label_83:
	addi        s8, s8, 1
	bge         s8, s7, .VectorASTNodePrint_label_59

	// *** Basic block 9

.VectorASTNodePrint_label_87:
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
.func_end_VectorASTNodePrint:
	.size VectorASTNodePrint, .func_end_VectorASTNodePrint-VectorASTNodePrint

	.local  VectorASTNodeReplaceChild
	.type VectorASTNodeReplaceChild, @function

VectorASTNodeReplaceChild:

	// *** Basic block 0

	.local SetParent
	.global ASTNodeDelete
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
	mv          t0, a0
	mv          t1, a1
	mv          t2, a2
	mv          s1, a3
	mv          t3, t0
	ld          t4, 64(t3)
	ld          t4, 0(t4)
	slli        t5, t1, 3
	add         t4, t4, t5
	ld          s2, 0(t4)
	ld          t4, 64(t3)
	ld          t4, 0(t4)
	add         t4, t4, t5
	sd          t2, 0(t4)
	mv          a2, t1
	mv          a1, t0
	mv          a0, t2
	call        SetParent

	// *** Basic block 1

	beqz        s1, .VectorASTNodeReplaceChild_label_47

	// *** Basic block 2

	mv          a0, s2
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodeDelete

	// *** Basic block 3

.VectorASTNodeReplaceChild_label_47:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_VectorASTNodeReplaceChild:
	.size VectorASTNodeReplaceChild, .func_end_VectorASTNodeReplaceChild-VectorASTNodeReplaceChild

	.local  VectorASTNodeClone
	.type VectorASTNodeClone, @function

VectorASTNodeClone:

	// *** Basic block 0

	.global malloc
	.local ASTNodeBaseCopy
	.global ASTNodeClone
	.global NewVector
	.global VectorAppend
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
	mv          s3, a2
	mv          s4, s1
	li          a0, 72		// 0x48 ASCII 'H'
	call        malloc

	// *** Basic block 1

	mv          s5, a0
	mv          a1, s1
	mv          a0, s5
	call        ASTNodeBaseCopy

	// *** Basic block 2

	ld          a0, 56(s4)
	mv          a3, s5
	mv          a2, s3
	mv          a1, s2
	call        ASTNodeClone

	// *** Basic block 3

	sd          a0, 56(s5)
	call        NewVector

	// *** Basic block 4

	sd          a0, 64(s5)
	mv          s6, x0
	ld          t0, 64(s4)
	ld          s7, 8(t0)
	bge         x0, s7, .VectorASTNodeClone_label_86

	// *** Basic block 5

	ld          s1, 0(t0)

	// *** Basic block 6

.VectorASTNodeClone_label_61:
	slli        t0, s6, 3
	add         t0, s1, t0
	ld          a0, 0(t0)
	mv          a3, s5
	mv          a2, s3
	mv          a1, s2
	call        ASTNodeClone

	// *** Basic block 7

	mv          s1, a0
	ld          a0, 64(s5)
	mv          a1, s1
	call        VectorAppend

	// *** Basic block 8

.VectorASTNodeClone_label_82:
	addi        s6, s6, 1
	bge         s6, s7, .VectorASTNodeClone_label_61

	// *** Basic block 9

.VectorASTNodeClone_label_86:
	mv          a1, s3
	mv          a0, s5
	mv          t0, s2
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
	jr          t0
.func_end_VectorASTNodeClone:
	.size VectorASTNodeClone, .func_end_VectorASTNodeClone-VectorASTNodeClone

	.local  VectorASTNodeVisit
	.type VectorASTNodeVisit, @function

VectorASTNodeVisit:

	// *** Basic block 0

	.global ASTNodeVisit
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
	mv          s3, a3
	mv          s4, a2
	mv          s5, s1
	mv          a3, x0
	mv          a1, s3
	jalr         x1, s2, 0

	// *** Basic block 1

	ld          a0, 56(s5)
	mv          a3, s3
	mv          a2, x0
	mv          a1, s2
	call        ASTNodeVisit

	// *** Basic block 2

	mv          s6, x0
	ld          t0, 64(s5)
	ld          s7, 8(t0)
	bge         x0, s7, .VectorASTNodeVisit_label_72

	// *** Basic block 3

	ld          s5, 0(t0)

	// *** Basic block 4

.VectorASTNodeVisit_label_54:
	slli        t0, s6, 3
	add         t0, s5, t0
	ld          a0, 0(t0)
	sext.w      t0, s6
	addi        a2, t0, 1
	mv          a3, s3
	mv          a1, s2
	call        ASTNodeVisit

	// *** Basic block 5

.VectorASTNodeVisit_label_68:
	addi        s6, s6, 1
	bge         s6, s7, .VectorASTNodeVisit_label_54

	// *** Basic block 6

.VectorASTNodeVisit_label_72:
	li          t0, 1		// 0x1 ASCII \x1
	mv          a3, t0
	mv          a2, s4
	mv          a1, s3
	mv          a0, s1
	mv          t0, s2
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
	jr          t0
.func_end_VectorASTNodeVisit:
	.size VectorASTNodeVisit, .func_end_VectorASTNodeVisit-VectorASTNodeVisit

	.global NewVectorASTNode
	.type NewVectorASTNode, @function

NewVectorASTNode:

	// *** Basic block 0

	.global malloc
	.global ASTNodeInit
	.local vector_vtbl
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
	mv          s3, a2
	mv          s4, a3
	mv          s5, a4
	li          a0, 72		// 0x48 ASCII 'H'
	call        malloc

	// *** Basic block 1

	mv          s6, a0
	la          a4, vector_vtbl
	mv          a3, s3
	mv          a2, s2
	mv          a1, s1
	mv          a0, s6
	call        ASTNodeInit

	// *** Basic block 2

	sd          s4, 56(s6)
	sd          s6, 24(s4)
	sd          s5, 64(s6)
	mv          s7, x0
	ld          t0, 8(s5)
	bge         x0, t0, .NewVectorASTNode_label_74

	// *** Basic block 3

	ld          t1, 0(s5)

	// *** Basic block 4

.NewVectorASTNode_label_59:
	slli        t2, s7, 3
	add         t1, t1, t2
	ld          s1, 0(t1)
	sd          s6, 24(s1)
	sext.w      t1, s7
	sw          t1, 32(s1)

	// *** Basic block 5

.NewVectorASTNode_label_70:
	addi        s7, s7, 1
	bge         s7, t0, .NewVectorASTNode_label_59

	// *** Basic block 6

.NewVectorASTNode_label_74:
	mv          a0, s6

	// *** Basic block 7

.NewVectorASTNode_label_77:
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
.func_end_NewVectorASTNode:
	.size NewVectorASTNode, .func_end_NewVectorASTNode-NewVectorASTNode

	.local  CastASTNodeDelete
	.type CastASTNodeDelete, @function

CastASTNodeDelete:

	// *** Basic block 0

	.global TypeRecordDelete
	.global ASTNodeDelete
	.local ASTNodeBaseDelete
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
	mv          s2, s1
	ld          s3, 56(s2)
	beq         s3, x0, .CastASTNodeDelete_label_22

	// *** Basic block 1

	mv          a0, s3
	call        TypeRecordDelete

	// *** Basic block 2

.CastASTNodeDelete_label_22:
	ld          s3, 64(s2)
	beq         s3, x0, .CastASTNodeDelete_label_31

	// *** Basic block 3

	mv          a0, s3
	call        ASTNodeDelete

	// *** Basic block 4

.CastASTNodeDelete_label_31:
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodeBaseDelete
.func_end_CastASTNodeDelete:
	.size CastASTNodeDelete, .func_end_CastASTNodeDelete-CastASTNodeDelete

	.local  CastASTNodePrint
	.type CastASTNodePrint, @function

CastASTNodePrint:

	// *** Basic block 0

	.local Indent
	.global fprintf
	.global TypeRecordPrint
	.global ASTNodePrint
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
	mv          s1, a1
	mv          s2, a2
	mv          s3, t0
	mv          a1, s2
	mv          a0, s1
	call        Indent

	// *** Basic block 1

	lla         a1, .str.189
	mv          a0, s2
	call        fprintf

	// *** Basic block 2

	mv          a1, s2
	mv          a0, s1
	call        Indent

	// *** Basic block 3

	ld          a0, 56(s3)
	mv          a1, s2
	call        TypeRecordPrint

	// *** Basic block 4

	ld          t0, 64(s3)
	beq         t0, x0, .CastASTNodePrint_label_58

	// *** Basic block 5

	addi        a1, s1, 2
	mv          a2, s2
	mv          a0, t0
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodePrint

	// *** Basic block 6

.CastASTNodePrint_label_58:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_CastASTNodePrint:
	.size CastASTNodePrint, .func_end_CastASTNodePrint-CastASTNodePrint

	.local  CastASTNodeReplaceChild
	.type CastASTNodeReplaceChild, @function

CastASTNodeReplaceChild:

	// *** Basic block 0

	.local SetParent
	.global ASTNodeDelete
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
	mv          t0, a0
	mv          t1, a2
	mv          t2, a1
	mv          s1, a3
	mv          t3, t0
	ld          s2, 64(t3)
	sd          t1, 64(t3)
	mv          a2, t2
	mv          a1, t0
	mv          a0, t1
	call        SetParent

	// *** Basic block 1

	beqz        s1, .CastASTNodeReplaceChild_label_37

	// *** Basic block 2

	mv          a0, s2
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodeDelete

	// *** Basic block 3

.CastASTNodeReplaceChild_label_37:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_CastASTNodeReplaceChild:
	.size CastASTNodeReplaceChild, .func_end_CastASTNodeReplaceChild-CastASTNodeReplaceChild

	.local  CastASTNodeClone
	.type CastASTNodeClone, @function

CastASTNodeClone:

	// *** Basic block 0

	.global malloc
	.local ASTNodeBaseCopy
	.global ASTNodeClone
	.global TypeRecordIncRef
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
	mv          s3, a2
	mv          s4, s1
	li          a0, 72		// 0x48 ASCII 'H'
	call        malloc

	// *** Basic block 1

	mv          s5, a0
	mv          a1, s1
	mv          a0, s5
	call        ASTNodeBaseCopy

	// *** Basic block 2

	ld          t0, 56(s4)
	sd          t0, 56(s5)
	ld          a0, 64(s4)
	mv          a3, s5
	mv          a2, s3
	mv          a1, s2
	call        ASTNodeClone

	// *** Basic block 3

	sd          a0, 64(s5)
	ld          a0, 56(s5)
	call        TypeRecordIncRef

	// *** Basic block 4

	mv          a1, s3
	mv          a0, s5
	mv          t0, s2
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	jr          t0
.func_end_CastASTNodeClone:
	.size CastASTNodeClone, .func_end_CastASTNodeClone-CastASTNodeClone

	.local  CastASTNodeUsesValue
	.type CastASTNodeUsesValue, @function

CastASTNodeUsesValue:

	// *** Basic block 0

	.global TypeIsVoid
	.global ASTNodeUsesValue
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
	mv          t0, s1
	ld          a0, 56(t0)
	call        TypeIsVoid

	// *** Basic block 1

	beqz        a0, .CastASTNodeUsesValue_label_23

	// *** Basic block 2

	mv          a0, x0

	// *** Basic block 3

.CastASTNodeUsesValue_label_20:
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         

	// *** Basic block 4

.CastASTNodeUsesValue_label_23:
	ld          a0, 24(s1)
	mv          a1, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodeUsesValue
.func_end_CastASTNodeUsesValue:
	.size CastASTNodeUsesValue, .func_end_CastASTNodeUsesValue-CastASTNodeUsesValue

	.global NewCastASTNode
	.type NewCastASTNode, @function

NewCastASTNode:

	// *** Basic block 0

	.global malloc
	.global ASTNodeInit
	.local cast_vtbl
	.global TypeRecordIncRef
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
	li          a0, 72		// 0x48 ASCII 'H'
	call        malloc

	// *** Basic block 1

	mv          s4, a0
	la          a4, cast_vtbl
	mv          a3, s1
	mv          a2, x0
	li          t0, 80		// 0x50 ASCII 'P'
	mv          a1, t0
	mv          a0, s4
	call        ASTNodeInit

	// *** Basic block 2

	sd          s2, 56(s4)
	mv          a0, s2
	call        TypeRecordIncRef

	// *** Basic block 3

	sd          s3, 64(s4)
	sd          s4, 24(s3)
	mv          a0, s4

	// *** Basic block 4

.NewCastASTNode_label_52:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewCastASTNode:
	.size NewCastASTNode, .func_end_NewCastASTNode-NewCastASTNode

	.local  PtrScaleASTNodeDelete
	.type PtrScaleASTNodeDelete, @function

PtrScaleASTNodeDelete:

	// *** Basic block 0

	.global TypeRecordDelete
	.global ASTNodeDelete
	.local ASTNodeBaseDelete
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
	mv          s2, s1
	ld          s3, 56(s2)
	beq         s3, x0, .PtrScaleASTNodeDelete_label_22

	// *** Basic block 1

	mv          a0, s3
	call        TypeRecordDelete

	// *** Basic block 2

.PtrScaleASTNodeDelete_label_22:
	ld          s3, 72(s2)
	beq         s3, x0, .PtrScaleASTNodeDelete_label_31

	// *** Basic block 3

	mv          a0, s3
	call        ASTNodeDelete

	// *** Basic block 4

.PtrScaleASTNodeDelete_label_31:
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodeBaseDelete
.func_end_PtrScaleASTNodeDelete:
	.size PtrScaleASTNodeDelete, .func_end_PtrScaleASTNodeDelete-PtrScaleASTNodeDelete

	.local  PtrScaleASTNodePrint
	.type PtrScaleASTNodePrint, @function

PtrScaleASTNodePrint:

	// *** Basic block 0

	.local Indent
	.global fprintf
	.global TypeRecordPrint
	.global ASTNodePrint
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
	mv          s1, a1
	mv          s2, a2
	mv          s3, t0
	mv          a1, s2
	mv          a0, s1
	call        Indent

	// *** Basic block 1

	lla         a1, .str.190
	mv          a0, s2
	call        fprintf

	// *** Basic block 2

	mv          a1, s2
	mv          a0, s1
	call        Indent

	// *** Basic block 3

	ld          a0, 56(s3)
	mv          a1, s2
	call        TypeRecordPrint

	// *** Basic block 4

	ld          t0, 72(s3)
	beq         t0, x0, .PtrScaleASTNodePrint_label_58

	// *** Basic block 5

	addi        a1, s1, 2
	mv          a2, s2
	mv          a0, t0
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodePrint

	// *** Basic block 6

.PtrScaleASTNodePrint_label_58:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_PtrScaleASTNodePrint:
	.size PtrScaleASTNodePrint, .func_end_PtrScaleASTNodePrint-PtrScaleASTNodePrint

	.local  PtrScaleASTNodeReplaceChild
	.type PtrScaleASTNodeReplaceChild, @function

PtrScaleASTNodeReplaceChild:

	// *** Basic block 0

	.local SetParent
	.global ASTNodeDelete
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
	mv          t0, a0
	mv          t1, a2
	mv          t2, a1
	mv          s1, a3
	mv          t3, t0
	ld          s2, 72(t3)
	sd          t1, 72(t3)
	mv          a2, t2
	mv          a1, t0
	mv          a0, t1
	call        SetParent

	// *** Basic block 1

	beqz        s1, .PtrScaleASTNodeReplaceChild_label_37

	// *** Basic block 2

	mv          a0, s2
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodeDelete

	// *** Basic block 3

.PtrScaleASTNodeReplaceChild_label_37:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_PtrScaleASTNodeReplaceChild:
	.size PtrScaleASTNodeReplaceChild, .func_end_PtrScaleASTNodeReplaceChild-PtrScaleASTNodeReplaceChild

	.local  PtrScaleASTNodeClone
	.type PtrScaleASTNodeClone, @function

PtrScaleASTNodeClone:

	// *** Basic block 0

	.global malloc
	.local ASTNodeBaseCopy
	.global ASTNodeClone
	.global TypeRecordIncRef
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
	mv          s3, a2
	mv          s4, s1
	li          a0, 80		// 0x50 ASCII 'P'
	call        malloc

	// *** Basic block 1

	mv          s5, a0
	mv          a1, s1
	mv          a0, s5
	call        ASTNodeBaseCopy

	// *** Basic block 2

	ld          t0, 56(s4)
	sd          t0, 56(s5)
	lw          t0, 64(s4)
	sw          t0, 64(s5)
	ld          a0, 72(s4)
	mv          a3, s5
	mv          a2, s3
	mv          a1, s2
	call        ASTNodeClone

	// *** Basic block 3

	sd          a0, 72(s5)
	ld          a0, 56(s5)
	call        TypeRecordIncRef

	// *** Basic block 4

	mv          a1, s3
	mv          a0, s5
	mv          t0, s2
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	jr          t0
.func_end_PtrScaleASTNodeClone:
	.size PtrScaleASTNodeClone, .func_end_PtrScaleASTNodeClone-PtrScaleASTNodeClone

	.local  PtrScaleASTNodeUsesValue
	.type PtrScaleASTNodeUsesValue, @function

PtrScaleASTNodeUsesValue:

	// *** Basic block 0

	.global ASTNodeUsesValue
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	ld          a0, 24(t0)
	mv          a1, t0
	j           ASTNodeUsesValue
.func_end_PtrScaleASTNodeUsesValue:
	.size PtrScaleASTNodeUsesValue, .func_end_PtrScaleASTNodeUsesValue-PtrScaleASTNodeUsesValue

	.global NewPtrScaleASTNode
	.type NewPtrScaleASTNode, @function

NewPtrScaleASTNode:

	// *** Basic block 0

	.global malloc
	.global ASTNodeInit
	.local ptr_scale_vtbl
	.global TypeRecordIncRef
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
	mv          s4, a2
	li          a0, 80		// 0x50 ASCII 'P'
	call        malloc

	// *** Basic block 1

	mv          s5, a0
	la          a4, ptr_scale_vtbl
	mv          a3, s1
	mv          a2, x0
	li          t0, 90		// 0x5a ASCII 'Z'
	mv          a1, t0
	mv          a0, s5
	call        ASTNodeInit

	// *** Basic block 2

	sd          s2, 56(s5)
	sw          s3, 64(s5)
	mv          a0, s2
	call        TypeRecordIncRef

	// *** Basic block 3

	sd          s4, 72(s5)
	sd          s5, 24(s4)
	mv          a0, s5

	// *** Basic block 4

.NewPtrScaleASTNode_label_57:
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
.func_end_NewPtrScaleASTNode:
	.size NewPtrScaleASTNode, .func_end_NewPtrScaleASTNode-NewPtrScaleASTNode

	.global SizeofASTNodeDelete
	.type SizeofASTNodeDelete, @function

SizeofASTNodeDelete:

	// *** Basic block 0

	.global ASTNodeDelete
	.local ASTNodeBaseDelete
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
	mv          t0, s1
	ld          s2, 64(t0)
	beq         s2, x0, .SizeofASTNodeDelete_label_20

	// *** Basic block 1

	mv          a0, s2
	call        ASTNodeDelete

	// *** Basic block 2

.SizeofASTNodeDelete_label_20:
	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodeBaseDelete
.func_end_SizeofASTNodeDelete:
	.size SizeofASTNodeDelete, .func_end_SizeofASTNodeDelete-SizeofASTNodeDelete

	.global SizeofASTNodePrint
	.type SizeofASTNodePrint, @function

SizeofASTNodePrint:

	// *** Basic block 0

	.global fprintf
	.global ASTNodePrint
	.local ConstantASTNodePrint
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
	mv          t0, a0
	mv          s1, a2
	mv          s2, a1
	mv          s3, t0
	lla         a1, .str.191
	mv          a0, s1
	call        fprintf

	// *** Basic block 1

	ld          s4, 64(s3)
	beq         s4, x0, .SizeofASTNodePrint_label_40

	// *** Basic block 2

	addi        a1, s2, 2
	mv          a2, s1
	mv          a0, s4
	call        ASTNodePrint

	// *** Basic block 3

	j           .SizeofASTNodePrint_label_49

	// *** Basic block 4

.SizeofASTNodePrint_label_40:
	addi        a1, s2, 2
	mv          a2, s1
	mv          a0, s3
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ConstantASTNodePrint

	// *** Basic block 5

.SizeofASTNodePrint_label_49:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_SizeofASTNodePrint:
	.size SizeofASTNodePrint, .func_end_SizeofASTNodePrint-SizeofASTNodePrint

	.global SizeofASTNodeReplaceChild
	.type SizeofASTNodeReplaceChild, @function

SizeofASTNodeReplaceChild:

	// *** Basic block 0

	.local SetParent
	.global ASTNodeDelete
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
	mv          t0, a0
	mv          t1, a2
	mv          t2, a1
	mv          s1, a3
	mv          t3, t0
	ld          s2, 64(t3)
	sd          t1, 64(t3)
	mv          a2, t2
	mv          a1, t0
	mv          a0, t1
	call        SetParent

	// *** Basic block 1

	beqz        s1, .SizeofASTNodeReplaceChild_label_37

	// *** Basic block 2

	mv          a0, s2
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodeDelete

	// *** Basic block 3

.SizeofASTNodeReplaceChild_label_37:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_SizeofASTNodeReplaceChild:
	.size SizeofASTNodeReplaceChild, .func_end_SizeofASTNodeReplaceChild-SizeofASTNodeReplaceChild

	.local  SizeofASTNodeClone
	.type SizeofASTNodeClone, @function

SizeofASTNodeClone:

	// *** Basic block 0

	.global malloc
	.global memcpy
	.global ASTNodeClone
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
	mv          t0, a0
	mv          s1, a1
	mv          s2, a2
	mv          s3, t0
	li          a0, 72		// 0x48 ASCII 'H'
	call        malloc

	// *** Basic block 1

	mv          s4, a0
	li          t0, 64		// 0x40 ASCII '@'
	mv          a2, t0
	mv          a1, s3
	mv          a0, s4
	call        memcpy

	// *** Basic block 2

	ld          a0, 64(s3)
	mv          a3, s4
	mv          a2, s2
	mv          a1, s1
	call        ASTNodeClone

	// *** Basic block 3

	sd          a0, 64(s4)
	mv          a1, s2
	mv          a0, s4
	mv          t0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	jr          t0
.func_end_SizeofASTNodeClone:
	.size SizeofASTNodeClone, .func_end_SizeofASTNodeClone-SizeofASTNodeClone

	.global NewSizeofASTNodeWithKnownSize
	.type NewSizeofASTNodeWithKnownSize, @function

NewSizeofASTNodeWithKnownSize:

	// *** Basic block 0

	.global malloc
	.global NewTypeRecord
	.global IntConstantASTNodeInit
	.local sizeof_vtbl
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
	li          a0, 72		// 0x48 ASCII 'H'
	call        malloc

	// *** Basic block 1

	mv          s3, a0
	li          t0, 4		// 0x4 ASCII \x4
	mv          a1, t0
	li          t0, 16386		// 0x4002
	mv          a0, t0
	call        NewTypeRecord

	// *** Basic block 2

	mv          s4, a0
	mv          a3, s2
	mv          a2, s4
	mv          a1, s1
	mv          a0, s3
	call        IntConstantASTNodeInit

	// *** Basic block 3

	la          t0, sizeof_vtbl
	sd          t0, 48(s3)
	sd          x0, 64(s3)
	li          t0, 65		// 0x41 ASCII 'A'
	sw          t0, 0(s3)
	mv          a0, s3

	// *** Basic block 4

.NewSizeofASTNodeWithKnownSize_label_54:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewSizeofASTNodeWithKnownSize:
	.size NewSizeofASTNodeWithKnownSize, .func_end_NewSizeofASTNodeWithKnownSize-NewSizeofASTNodeWithKnownSize

	.global NewSizeofASTNodeWithExpression
	.type NewSizeofASTNodeWithExpression, @function

NewSizeofASTNodeWithExpression:

	// *** Basic block 0

	.global malloc
	.global NewTypeRecord
	.global IntConstantASTNodeInit
	.local sizeof_vtbl
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
	li          a0, 72		// 0x48 ASCII 'H'
	call        malloc

	// *** Basic block 1

	mv          s3, a0
	li          t0, 4		// 0x4 ASCII \x4
	mv          a1, t0
	li          t0, 16386		// 0x4002
	mv          a0, t0
	call        NewTypeRecord

	// *** Basic block 2

	mv          s4, a0
	mv          a3, s1
	mv          a2, s4
	mv          a1, x0
	mv          a0, s3
	call        IntConstantASTNodeInit

	// *** Basic block 3

	la          t0, sizeof_vtbl
	sd          t0, 48(s3)
	li          t0, 65		// 0x41 ASCII 'A'
	sw          t0, 0(s3)
	sd          s2, 64(s3)
	sd          s3, 24(s2)
	mv          a0, s3

	// *** Basic block 4

.NewSizeofASTNodeWithExpression_label_58:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewSizeofASTNodeWithExpression:
	.size NewSizeofASTNodeWithExpression, .func_end_NewSizeofASTNodeWithExpression-NewSizeofASTNodeWithExpression

	.local  MacroNameASTNodeDelete
	.type MacroNameASTNodeDelete, @function

MacroNameASTNodeDelete:

	// *** Basic block 0

	.global StringDestruct
	.local ASTNodeBaseDelete
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
	mv          t0, s1
	addi        a0, t0, 56
	call        StringDestruct

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodeBaseDelete
.func_end_MacroNameASTNodeDelete:
	.size MacroNameASTNodeDelete, .func_end_MacroNameASTNodeDelete-MacroNameASTNodeDelete

	.local  MacroNameASTNodePrint
	.type MacroNameASTNodePrint, @function

MacroNameASTNodePrint:

	// *** Basic block 0

	.global fprintf
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	mv          t1, a2
	mv          t2, t0
	lla         a1, .str.192
	addi        t3, t2, 56
	ld          a2, 16(t3)
	mv          a0, t1
	j           fprintf
.func_end_MacroNameASTNodePrint:
	.size MacroNameASTNodePrint, .func_end_MacroNameASTNodePrint-MacroNameASTNodePrint

	.local  MacroNameASTNodeClone
	.type MacroNameASTNodeClone, @function

MacroNameASTNodeClone:

	// *** Basic block 0

	.global malloc
	.local ASTNodeBaseCopy
	.global StringInit
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
	mv          s3, a2
	mv          s4, s1
	li          a0, 96		// 0x60 ASCII '`'
	call        malloc

	// *** Basic block 1

	mv          s5, a0
	mv          a1, s1
	mv          a0, s5
	call        ASTNodeBaseCopy

	// *** Basic block 2

	addi        a0, s5, 56
	addi        t0, s4, 56
	ld          a1, 16(t0)
	call        StringInit

	// *** Basic block 3

	mv          a1, s3
	mv          a0, s5
	mv          t0, s2
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	jr          t0
.func_end_MacroNameASTNodeClone:
	.size MacroNameASTNodeClone, .func_end_MacroNameASTNodeClone-MacroNameASTNodeClone

	.global NewMacroNameASTNode
	.type NewMacroNameASTNode, @function

NewMacroNameASTNode:

	// *** Basic block 0

	.global malloc
	.global ASTNodeInit
	.local macro_vtbl
	.global StringInit
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
	li          a0, 96		// 0x60 ASCII '`'
	call        malloc

	// *** Basic block 1

	mv          s3, a0
	la          a4, macro_vtbl
	mv          a3, s1
	mv          a2, x0
	li          t0, 84		// 0x54 ASCII 'T'
	mv          a1, t0
	mv          a0, s3
	call        ASTNodeInit

	// *** Basic block 2

	addi        a0, s3, 56
	ld          a1, 16(s2)
	call        StringInit

	// *** Basic block 3

	mv          a0, s3

	// *** Basic block 4

.NewMacroNameASTNode_label_46:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewMacroNameASTNode:
	.size NewMacroNameASTNode, .func_end_NewMacroNameASTNode-NewMacroNameASTNode

	.local  GotoStatementASTNodeDelete
	.type GotoStatementASTNodeDelete, @function

GotoStatementASTNodeDelete:

	// *** Basic block 0

	.global StringDelete
	.local ASTNodeBaseDelete
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
	mv          t0, s1
	ld          a0, 56(t0)
	call        StringDelete

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodeBaseDelete
.func_end_GotoStatementASTNodeDelete:
	.size GotoStatementASTNodeDelete, .func_end_GotoStatementASTNodeDelete-GotoStatementASTNodeDelete

	.local  GotoStatementASTNodePrint
	.type GotoStatementASTNodePrint, @function

GotoStatementASTNodePrint:

	// *** Basic block 0

	.local Indent
	.global fprintf
	.local ASTNodeBasePrint
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
	mv          s4, s1
	mv          a1, s3
	mv          a0, s2
	call        Indent

	// *** Basic block 1

	lla         a1, .str.193
	ld          t0, 56(s4)
	ld          a2, 16(t0)
	mv          a0, s3
	call        fprintf

	// *** Basic block 2

	addi        a1, s2, 2
	mv          a2, s3
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodeBasePrint
.func_end_GotoStatementASTNodePrint:
	.size GotoStatementASTNodePrint, .func_end_GotoStatementASTNodePrint-GotoStatementASTNodePrint

	.local  GotoStatementASTNodeClone
	.type GotoStatementASTNodeClone, @function

GotoStatementASTNodeClone:

	// *** Basic block 0

	.global malloc
	.local ASTNodeBaseCopy
	.global NewString
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
	mv          s3, a2
	mv          s4, s1
	li          a0, 80		// 0x50 ASCII 'P'
	call        malloc

	// *** Basic block 1

	mv          s5, a0
	mv          a1, s1
	mv          a0, s5
	call        ASTNodeBaseCopy

	// *** Basic block 2

	ld          t0, 56(s4)
	ld          a0, 16(t0)
	call        NewString

	// *** Basic block 3

	sd          a0, 56(s5)
	sd          x0, 64(s5)
	lw          t0, 8(s5)
	andi        t0, t0, -9
	sw          t0, 8(s5)
	sd          x0, 72(s5)
	mv          a1, s3
	mv          a0, s5
	mv          t0, s2
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	jr          t0
.func_end_GotoStatementASTNodeClone:
	.size GotoStatementASTNodeClone, .func_end_GotoStatementASTNodeClone-GotoStatementASTNodeClone

	.global NewGotoStatementASTNode
	.type NewGotoStatementASTNode, @function

NewGotoStatementASTNode:

	// *** Basic block 0

	.global malloc
	.global ASTNodeInit
	.local goto_vtbl
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
	li          a0, 80		// 0x50 ASCII 'P'
	call        malloc

	// *** Basic block 1

	mv          s3, a0
	la          a4, goto_vtbl
	mv          a3, s1
	mv          a2, x0
	li          t0, 36		// 0x24 ASCII '$'
	mv          a1, t0
	mv          a0, s3
	call        ASTNodeInit

	// *** Basic block 2

	sd          s2, 56(s3)
	sd          x0, 64(s3)
	sd          x0, 72(s3)
	mv          a0, s3

	// *** Basic block 3

.NewGotoStatementASTNode_label_46:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewGotoStatementASTNode:
	.size NewGotoStatementASTNode, .func_end_NewGotoStatementASTNode-NewGotoStatementASTNode

	.local  ExpressionStatementASTNodeDelete
	.type ExpressionStatementASTNodeDelete, @function

ExpressionStatementASTNodeDelete:

	// *** Basic block 0

	.global ASTNodeDelete
	.local ASTNodeBaseDelete
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
	mv          t0, s1
	ld          s2, 56(t0)
	beq         s2, x0, .ExpressionStatementASTNodeDelete_label_20

	// *** Basic block 1

	mv          a0, s2
	call        ASTNodeDelete

	// *** Basic block 2

.ExpressionStatementASTNodeDelete_label_20:
	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodeBaseDelete
.func_end_ExpressionStatementASTNodeDelete:
	.size ExpressionStatementASTNodeDelete, .func_end_ExpressionStatementASTNodeDelete-ExpressionStatementASTNodeDelete

	.local  ExpressionStatementASTNodePrint
	.type ExpressionStatementASTNodePrint, @function

ExpressionStatementASTNodePrint:

	// *** Basic block 0

	.global ASTNodePrint
	// Leaf procedure, no stack frame generated
	mv          t0, a1
	mv          t1, a2
	mv          t2, a0
	ld          t3, 56(t2)
	beq         t3, x0, .ExpressionStatementASTNodePrint_label_31

	// *** Basic block 1

	addi        a1, t0, 2
	mv          a2, t1
	mv          a0, t3
	j           ASTNodePrint

	// *** Basic block 2

.ExpressionStatementASTNodePrint_label_31:
	ret         
.func_end_ExpressionStatementASTNodePrint:
	.size ExpressionStatementASTNodePrint, .func_end_ExpressionStatementASTNodePrint-ExpressionStatementASTNodePrint

	.local  ExpressionStatementASTNodeReplaceChild
	.type ExpressionStatementASTNodeReplaceChild, @function

ExpressionStatementASTNodeReplaceChild:

	// *** Basic block 0

	.local SetParent
	.global ASTNodeDelete
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
	mv          t0, a0
	mv          t1, a2
	mv          t2, a1
	mv          s1, a3
	mv          t3, t0
	ld          s2, 56(t3)
	sd          t1, 56(t3)
	mv          a2, t2
	mv          a1, t0
	mv          a0, t1
	call        SetParent

	// *** Basic block 1

	beqz        s1, .ExpressionStatementASTNodeReplaceChild_label_37

	// *** Basic block 2

	mv          a0, s2
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodeDelete

	// *** Basic block 3

.ExpressionStatementASTNodeReplaceChild_label_37:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ExpressionStatementASTNodeReplaceChild:
	.size ExpressionStatementASTNodeReplaceChild, .func_end_ExpressionStatementASTNodeReplaceChild-ExpressionStatementASTNodeReplaceChild

	.local  ExpressionStatementASTNodeClone
	.type ExpressionStatementASTNodeClone, @function

ExpressionStatementASTNodeClone:

	// *** Basic block 0

	.global malloc
	.local ASTNodeBaseCopy
	.global ASTNodeClone
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
	mv          s3, a2
	mv          s4, s1
	li          a0, 64		// 0x40 ASCII '@'
	call        malloc

	// *** Basic block 1

	mv          s5, a0
	mv          a1, s1
	mv          a0, s5
	call        ASTNodeBaseCopy

	// *** Basic block 2

	ld          a0, 56(s4)
	mv          a3, s5
	mv          a2, s3
	mv          a1, s2
	call        ASTNodeClone

	// *** Basic block 3

	sd          a0, 56(s5)
	mv          a1, s3
	mv          a0, s5
	mv          t0, s2
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	jr          t0
.func_end_ExpressionStatementASTNodeClone:
	.size ExpressionStatementASTNodeClone, .func_end_ExpressionStatementASTNodeClone-ExpressionStatementASTNodeClone

	.local  ExpressionStatementASTNodeVisit
	.type ExpressionStatementASTNodeVisit, @function

ExpressionStatementASTNodeVisit:

	// *** Basic block 0

	.global ASTNodeVisit
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
	mv          s3, a3
	mv          s4, a2
	mv          s5, s1
	mv          a3, x0
	mv          a1, s3
	jalr         x1, s2, 0

	// *** Basic block 1

	ld          a0, 56(s5)
	mv          a3, s3
	mv          a2, x0
	mv          a1, s2
	call        ASTNodeVisit

	// *** Basic block 2

	li          t0, 1		// 0x1 ASCII \x1
	mv          a3, t0
	mv          a2, s4
	mv          a1, s3
	mv          a0, s1
	mv          t0, s2
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	jr          t0
.func_end_ExpressionStatementASTNodeVisit:
	.size ExpressionStatementASTNodeVisit, .func_end_ExpressionStatementASTNodeVisit-ExpressionStatementASTNodeVisit

	.global NewExpressionStatementASTNode
	.type NewExpressionStatementASTNode, @function

NewExpressionStatementASTNode:

	// *** Basic block 0

	.global malloc
	.global ASTNodeInit
	.local expr_stmt_vtbl
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
	li          a0, 64		// 0x40 ASCII '@'
	call        malloc

	// *** Basic block 1

	mv          s3, a0
	la          a4, expr_stmt_vtbl
	mv          a3, s1
	mv          a2, x0
	li          t0, 85		// 0x55 ASCII 'U'
	mv          a1, t0
	mv          a0, s3
	call        ASTNodeInit

	// *** Basic block 2

	sd          s2, 56(s3)
	sd          s3, 24(s2)
	mv          a0, s3

	// *** Basic block 3

.NewExpressionStatementASTNode_label_43:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewExpressionStatementASTNode:
	.size NewExpressionStatementASTNode, .func_end_NewExpressionStatementASTNode-NewExpressionStatementASTNode

	.local  IfStatementASTNodeDelete
	.type IfStatementASTNodeDelete, @function

IfStatementASTNodeDelete:

	// *** Basic block 0

	.global ASTNodeDelete
	.local ASTNodeBaseDelete
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
	mv          s2, s1
	ld          s3, 56(s2)
	beq         s3, x0, .IfStatementASTNodeDelete_label_22

	// *** Basic block 1

	mv          a0, s3
	call        ASTNodeDelete

	// *** Basic block 2

.IfStatementASTNodeDelete_label_22:
	ld          s3, 64(s2)
	beq         s3, x0, .IfStatementASTNodeDelete_label_31

	// *** Basic block 3

	mv          a0, s3
	call        ASTNodeDelete

	// *** Basic block 4

.IfStatementASTNodeDelete_label_31:
	ld          s3, 72(s2)
	beq         s3, x0, .IfStatementASTNodeDelete_label_40

	// *** Basic block 5

	mv          a0, s3
	call        ASTNodeDelete

	// *** Basic block 6

.IfStatementASTNodeDelete_label_40:
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodeBaseDelete
.func_end_IfStatementASTNodeDelete:
	.size IfStatementASTNodeDelete, .func_end_IfStatementASTNodeDelete-IfStatementASTNodeDelete

	.local  IfStatementASTNodePrint
	.type IfStatementASTNodePrint, @function

IfStatementASTNodePrint:

	// *** Basic block 0

	.global ASTNodePrint
	.local ASTNodeBasePrint
	.local Indent
	.global fprintf
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
	mv          s2, a2
	mv          s3, a0
	ld          s4, 56(s3)
	beq         s4, x0, .IfStatementASTNodePrint_label_36

	// *** Basic block 1

	addi        a1, s1, 2
	mv          a2, s2
	mv          a0, s4
	call        ASTNodePrint

	// *** Basic block 2

.IfStatementASTNodePrint_label_36:
	mv          a2, s2
	mv          a1, s1
	mv          a0, s3
	call        ASTNodeBasePrint

	// *** Basic block 3

	ld          s4, 64(s3)
	beq         s4, x0, .IfStatementASTNodePrint_label_56

	// *** Basic block 4

	addi        a1, s1, 2
	mv          a2, s2
	mv          a0, s4
	call        ASTNodePrint

	// *** Basic block 5

.IfStatementASTNodePrint_label_56:
	mv          a1, s2
	mv          a0, s1
	call        Indent

	// *** Basic block 6

	lla         a1, .str.194
	mv          a0, s2
	call        fprintf

	// *** Basic block 7

	ld          t0, 72(s3)
	beq         t0, x0, .IfStatementASTNodePrint_label_81

	// *** Basic block 8

	addi        a1, s1, 2
	mv          a2, s2
	mv          a0, t0
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodePrint

	// *** Basic block 9

.IfStatementASTNodePrint_label_81:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_IfStatementASTNodePrint:
	.size IfStatementASTNodePrint, .func_end_IfStatementASTNodePrint-IfStatementASTNodePrint

	.local  IfStatementASTNodeReplaceChild
	.type IfStatementASTNodeReplaceChild, @function

IfStatementASTNodeReplaceChild:

	// *** Basic block 0

	.local SetParent
	.global ASTNodeDelete
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
	mv          s4, a3
	mv          s5, s1
	mv          s6, x0
	blt         s2, x0, .IfStatementASTNodeReplaceChild_label_58

	// *** Basic block 1

	li          t0, 2		// 0x2 ASCII \x2
	blt         t0, s2, .IfStatementASTNodeReplaceChild_label_58

	// *** Basic block 2

	slli        t0, s2, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 3

	j           .IfStatementASTNodeReplaceChild_label_40

	// *** Basic block 4

	j           .IfStatementASTNodeReplaceChild_label_46

	// *** Basic block 5

	j           .IfStatementASTNodeReplaceChild_label_52

	// *** Basic block 6

.IfStatementASTNodeReplaceChild_label_40:
	ld          s6, 56(s5)
	sd          s3, 56(s5)
	j           .IfStatementASTNodeReplaceChild_label_58

	// *** Basic block 7

.IfStatementASTNodeReplaceChild_label_46:
	ld          s6, 64(s5)
	sd          s3, 64(s5)
	j           .IfStatementASTNodeReplaceChild_label_58

	// *** Basic block 8

.IfStatementASTNodeReplaceChild_label_52:
	ld          s6, 72(s5)
	sd          s3, 72(s5)
	j           .IfStatementASTNodeReplaceChild_label_58

	// *** Basic block 9

.IfStatementASTNodeReplaceChild_label_58:
	mv          a2, s2
	mv          a1, s1
	mv          a0, s3
	call        SetParent

	// *** Basic block 10

	beqz        s4, .IfStatementASTNodeReplaceChild_label_71

	// *** Basic block 11

	mv          a0, s6
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
	j           ASTNodeDelete

	// *** Basic block 12

.IfStatementASTNodeReplaceChild_label_71:
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
.func_end_IfStatementASTNodeReplaceChild:
	.size IfStatementASTNodeReplaceChild, .func_end_IfStatementASTNodeReplaceChild-IfStatementASTNodeReplaceChild

	.local  IfStatementASTNodeClone
	.type IfStatementASTNodeClone, @function

IfStatementASTNodeClone:

	// *** Basic block 0

	.global malloc
	.local ASTNodeBaseCopy
	.global ASTNodeClone
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
	mv          s3, a2
	mv          s4, s1
	li          a0, 80		// 0x50 ASCII 'P'
	call        malloc

	// *** Basic block 1

	mv          s5, a0
	mv          a1, s1
	mv          a0, s5
	call        ASTNodeBaseCopy

	// *** Basic block 2

	ld          a0, 56(s4)
	mv          a3, s5
	mv          a2, s3
	mv          a1, s2
	call        ASTNodeClone

	// *** Basic block 3

	sd          a0, 56(s5)
	ld          a0, 64(s4)
	mv          a3, s5
	mv          a2, s3
	mv          a1, s2
	call        ASTNodeClone

	// *** Basic block 4

	sd          a0, 64(s5)
	ld          a0, 72(s4)
	mv          a3, s5
	mv          a2, s3
	mv          a1, s2
	call        ASTNodeClone

	// *** Basic block 5

	sd          a0, 72(s5)
	mv          a1, s3
	mv          a0, s5
	mv          t0, s2
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	jr          t0
.func_end_IfStatementASTNodeClone:
	.size IfStatementASTNodeClone, .func_end_IfStatementASTNodeClone-IfStatementASTNodeClone

	.local  IfStatementASTNodeVisit
	.type IfStatementASTNodeVisit, @function

IfStatementASTNodeVisit:

	// *** Basic block 0

	.global ASTNodeVisit
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
	mv          s3, a3
	mv          s4, a2
	mv          s5, s1
	mv          a3, x0
	mv          a1, s3
	jalr         x1, s2, 0

	// *** Basic block 1

	ld          a0, 56(s5)
	mv          a3, s3
	mv          a2, x0
	mv          a1, s2
	call        ASTNodeVisit

	// *** Basic block 2

	ld          a0, 64(s5)
	mv          a3, s3
	li          s6, 1		// 0x1 ASCII \x1
	mv          a2, s6
	mv          a1, s2
	call        ASTNodeVisit

	// *** Basic block 3

	ld          a0, 72(s5)
	mv          a3, s3
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	mv          a1, s2
	call        ASTNodeVisit

	// *** Basic block 4

	mv          a3, s6
	mv          a2, s4
	mv          a1, s3
	mv          a0, s1
	mv          t0, s2
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
	jr          t0
.func_end_IfStatementASTNodeVisit:
	.size IfStatementASTNodeVisit, .func_end_IfStatementASTNodeVisit-IfStatementASTNodeVisit

	.local  IfStatementUsesValue
	.type IfStatementUsesValue, @function

IfStatementUsesValue:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	ld          t1, 56(t0)
	bne         a1, t1, .IfStatementUsesValue_label_24

	// *** Basic block 1

	li          a0, 1		// 0x1 ASCII \x1

	// *** Basic block 2

.IfStatementUsesValue_label_21:
	ret         

	// *** Basic block 3

.IfStatementUsesValue_label_24:
	mv          a0, x0
	ret         
.func_end_IfStatementUsesValue:
	.size IfStatementUsesValue, .func_end_IfStatementUsesValue-IfStatementUsesValue

	.global NewIfStatementASTNode
	.type NewIfStatementASTNode, @function

NewIfStatementASTNode:

	// *** Basic block 0

	.global malloc
	.global ASTNodeInit
	.local if_stmt_vtbl
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
	mv          s4, a2
	li          a0, 80		// 0x50 ASCII 'P'
	call        malloc

	// *** Basic block 1

	mv          s5, a0
	la          a4, if_stmt_vtbl
	mv          a3, s1
	mv          a2, x0
	li          t0, 39		// 0x27 ASCII '''
	mv          a1, t0
	mv          a0, s5
	call        ASTNodeInit

	// *** Basic block 2

	sd          s2, 56(s5)
	sd          s5, 24(s2)
	sw          x0, 32(s2)
	sd          s3, 64(s5)
	sd          s5, 24(s3)
	li          t0, 1		// 0x1 ASCII \x1
	sw          t0, 32(s3)
	sd          s4, 72(s5)
	beq         s4, x0, .NewIfStatementASTNode_label_69

	// *** Basic block 3

	sd          s5, 24(s4)
	li          t0, 2		// 0x2 ASCII \x2
	sw          t0, 32(s4)

	// *** Basic block 4

.NewIfStatementASTNode_label_69:
	mv          a0, s5

	// *** Basic block 5

.NewIfStatementASTNode_label_72:
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
.func_end_NewIfStatementASTNode:
	.size NewIfStatementASTNode, .func_end_NewIfStatementASTNode-NewIfStatementASTNode

	.local  CombinedStatementASTNodeDelete
	.type CombinedStatementASTNodeDelete, @function

CombinedStatementASTNodeDelete:

	// *** Basic block 0

	.global ASTNodeDelete
	.local ASTNodeBaseDelete
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
	mv          s2, s1
	ld          s3, 56(s2)
	beq         s3, x0, .CombinedStatementASTNodeDelete_label_21

	// *** Basic block 1

	mv          a0, s3
	call        ASTNodeDelete

	// *** Basic block 2

.CombinedStatementASTNodeDelete_label_21:
	ld          s3, 64(s2)
	beq         s3, x0, .CombinedStatementASTNodeDelete_label_30

	// *** Basic block 3

	mv          a0, s3
	call        ASTNodeDelete

	// *** Basic block 4

.CombinedStatementASTNodeDelete_label_30:
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodeBaseDelete
.func_end_CombinedStatementASTNodeDelete:
	.size CombinedStatementASTNodeDelete, .func_end_CombinedStatementASTNodeDelete-CombinedStatementASTNodeDelete

	.local  CombinedStatementASTNodePrint
	.type CombinedStatementASTNodePrint, @function

CombinedStatementASTNodePrint:

	// *** Basic block 0

	.global ASTNodePrint
	.local ASTNodeBasePrint
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
	mv          s2, a2
	mv          s3, a0
	ld          s4, 56(s3)
	beq         s4, x0, .CombinedStatementASTNodePrint_label_32

	// *** Basic block 1

	addi        a1, s1, 2
	mv          a2, s2
	mv          a0, s4
	call        ASTNodePrint

	// *** Basic block 2

.CombinedStatementASTNodePrint_label_32:
	mv          a2, s2
	mv          a1, s1
	mv          a0, s3
	call        ASTNodeBasePrint

	// *** Basic block 3

	ld          t0, 64(s3)
	beq         t0, x0, .CombinedStatementASTNodePrint_label_53

	// *** Basic block 4

	addi        a1, s1, 2
	mv          a2, s2
	mv          a0, t0
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodePrint

	// *** Basic block 5

.CombinedStatementASTNodePrint_label_53:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_CombinedStatementASTNodePrint:
	.size CombinedStatementASTNodePrint, .func_end_CombinedStatementASTNodePrint-CombinedStatementASTNodePrint

	.local  CombinedStatementASTNodeReplaceChild
	.type CombinedStatementASTNodeReplaceChild, @function

CombinedStatementASTNodeReplaceChild:

	// *** Basic block 0

	.local SetParent
	.global ASTNodeDelete
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
	mv          s4, a3
	mv          s5, s1
	mv          s6, x0
	blt         s2, x0, .CombinedStatementASTNodeReplaceChild_label_51

	// *** Basic block 1

	li          t0, 1		// 0x1 ASCII \x1
	blt         t0, s2, .CombinedStatementASTNodeReplaceChild_label_51

	// *** Basic block 2

	slli        t0, s2, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 3

	j           .CombinedStatementASTNodeReplaceChild_label_39

	// *** Basic block 4

	j           .CombinedStatementASTNodeReplaceChild_label_45

	// *** Basic block 5

.CombinedStatementASTNodeReplaceChild_label_39:
	ld          s6, 56(s5)
	sd          s3, 56(s5)
	j           .CombinedStatementASTNodeReplaceChild_label_51

	// *** Basic block 6

.CombinedStatementASTNodeReplaceChild_label_45:
	ld          s6, 64(s5)
	sd          s3, 64(s5)
	j           .CombinedStatementASTNodeReplaceChild_label_51

	// *** Basic block 7

.CombinedStatementASTNodeReplaceChild_label_51:
	mv          a2, s2
	mv          a1, s1
	mv          a0, s3
	call        SetParent

	// *** Basic block 8

	beqz        s4, .CombinedStatementASTNodeReplaceChild_label_64

	// *** Basic block 9

	mv          a0, s6
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
	j           ASTNodeDelete

	// *** Basic block 10

.CombinedStatementASTNodeReplaceChild_label_64:
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
.func_end_CombinedStatementASTNodeReplaceChild:
	.size CombinedStatementASTNodeReplaceChild, .func_end_CombinedStatementASTNodeReplaceChild-CombinedStatementASTNodeReplaceChild

	.local  CombinedStatementASTNodeClone
	.type CombinedStatementASTNodeClone, @function

CombinedStatementASTNodeClone:

	// *** Basic block 0

	.global malloc
	.local ASTNodeBaseCopy
	.global ASTNodeClone
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
	mv          s3, a2
	mv          s4, s1
	li          a0, 72		// 0x48 ASCII 'H'
	call        malloc

	// *** Basic block 1

	mv          s5, a0
	mv          a1, s1
	mv          a0, s5
	call        ASTNodeBaseCopy

	// *** Basic block 2

	ld          a0, 56(s4)
	mv          a3, s5
	mv          a2, s3
	mv          a1, s2
	call        ASTNodeClone

	// *** Basic block 3

	sd          a0, 56(s5)
	ld          a0, 64(s4)
	mv          a3, s5
	mv          a2, s3
	mv          a1, s2
	call        ASTNodeClone

	// *** Basic block 4

	sd          a0, 64(s5)
	mv          a1, s3
	mv          a0, s5
	mv          t0, s2
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	jr          t0
.func_end_CombinedStatementASTNodeClone:
	.size CombinedStatementASTNodeClone, .func_end_CombinedStatementASTNodeClone-CombinedStatementASTNodeClone

	.local  CombinedStatementASTNodeVisit
	.type CombinedStatementASTNodeVisit, @function

CombinedStatementASTNodeVisit:

	// *** Basic block 0

	.global ASTNodeVisit
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
	mv          s3, a3
	mv          s4, a2
	mv          s5, s1
	mv          a3, x0
	mv          a1, s3
	jalr         x1, s2, 0

	// *** Basic block 1

	ld          a0, 56(s5)
	mv          a3, s3
	mv          a2, x0
	mv          a1, s2
	call        ASTNodeVisit

	// *** Basic block 2

	ld          a0, 64(s5)
	mv          a3, s3
	li          s6, 1		// 0x1 ASCII \x1
	mv          a2, s6
	mv          a1, s2
	call        ASTNodeVisit

	// *** Basic block 3

	mv          a3, s6
	mv          a2, s4
	mv          a1, s3
	mv          a0, s1
	mv          t0, s2
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
	jr          t0
.func_end_CombinedStatementASTNodeVisit:
	.size CombinedStatementASTNodeVisit, .func_end_CombinedStatementASTNodeVisit-CombinedStatementASTNodeVisit

	.local  CombinedStatementASTNodeUsesValue
	.type CombinedStatementASTNodeUsesValue, @function

CombinedStatementASTNodeUsesValue:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	ld          t1, 56(t0)
	sub         t1, a1, t1
	seqz        a0, t1

	// *** Basic block 1

.CombinedStatementASTNodeUsesValue_label_17:
	ret         
.func_end_CombinedStatementASTNodeUsesValue:
	.size CombinedStatementASTNodeUsesValue, .func_end_CombinedStatementASTNodeUsesValue-CombinedStatementASTNodeUsesValue

	.global NewCombinedStatementASTNode
	.type NewCombinedStatementASTNode, @function

NewCombinedStatementASTNode:

	// *** Basic block 0

	.global malloc
	.global ASTNodeInit
	.local combined_stmt_vtbl
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
	mv          s2, a3
	mv          s3, a1
	mv          s4, a2
	li          a0, 72		// 0x48 ASCII 'H'
	call        malloc

	// *** Basic block 1

	mv          s5, a0
	la          a4, combined_stmt_vtbl
	mv          a3, s2
	mv          a2, x0
	mv          a1, s1
	mv          a0, s5
	call        ASTNodeInit

	// *** Basic block 2

	sd          s3, 56(s5)
	sd          s4, 64(s5)
	beq         s3, x0, .NewCombinedStatementASTNode_label_53

	// *** Basic block 3

	sd          s5, 24(s3)
	sw          x0, 32(s3)

	// *** Basic block 4

.NewCombinedStatementASTNode_label_53:
	beq         s4, x0, .NewCombinedStatementASTNode_label_62

	// *** Basic block 5

	sd          s5, 24(s4)
	li          t0, 1		// 0x1 ASCII \x1
	sw          t0, 32(s4)

	// *** Basic block 6

.NewCombinedStatementASTNode_label_62:
	mv          a0, s5

	// *** Basic block 7

.NewCombinedStatementASTNode_label_65:
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
.func_end_NewCombinedStatementASTNode:
	.size NewCombinedStatementASTNode, .func_end_NewCombinedStatementASTNode-NewCombinedStatementASTNode

	.local  CompoundStatementASTNodeDelete
	.type CompoundStatementASTNodeDelete, @function

CompoundStatementASTNodeDelete:

	// *** Basic block 0

	.global ASTNodeDelete
	.global VectorDestruct
	.local ASTNodeBaseDelete
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
	mv          t0, s1
	mv          s2, x0
	ld          s3, 56(t0)
	ld          s4, 8(s3)
	bge         x0, s4, .CompoundStatementASTNodeDelete_label_42

	// *** Basic block 1

	ld          t0, 0(s3)

	// *** Basic block 2

.CompoundStatementASTNodeDelete_label_25:
	slli        t1, s2, 3
	add         t0, t0, t1
	ld          s5, 0(t0)
	beq         s5, x0, .CompoundStatementASTNodeDelete_label_37

	// *** Basic block 3

	mv          a0, s5
	call        ASTNodeDelete

	// *** Basic block 4

.CompoundStatementASTNodeDelete_label_37:

	// *** Basic block 5

.CompoundStatementASTNodeDelete_label_38:
	addi        s2, s2, 1
	bge         s2, s4, .CompoundStatementASTNodeDelete_label_25

	// *** Basic block 6

.CompoundStatementASTNodeDelete_label_42:
	mv          a0, s3
	call        VectorDestruct

	// *** Basic block 7

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
	j           ASTNodeBaseDelete
.func_end_CompoundStatementASTNodeDelete:
	.size CompoundStatementASTNodeDelete, .func_end_CompoundStatementASTNodeDelete-CompoundStatementASTNodeDelete

	.local  CompoundStatementASTNodePrint
	.type CompoundStatementASTNodePrint, @function

CompoundStatementASTNodePrint:

	// *** Basic block 0

	.global ASTNodePrint
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
	mv          t0, a0
	mv          s3, x0
	ld          t1, 56(t0)
	ld          s4, 8(t1)
	bge         x0, s4, .CompoundStatementASTNodePrint_label_50

	// *** Basic block 1

	ld          t0, 0(t1)

	// *** Basic block 2

.CompoundStatementASTNodePrint_label_29:
	slli        t1, s3, 3
	add         t0, t0, t1
	ld          s5, 0(t0)
	beq         s5, x0, .CompoundStatementASTNodePrint_label_45

	// *** Basic block 3

	mv          a2, s2
	mv          a1, s1
	mv          a0, s5
	call        ASTNodePrint

	// *** Basic block 4

.CompoundStatementASTNodePrint_label_45:

	// *** Basic block 5

.CompoundStatementASTNodePrint_label_46:
	addi        s3, s3, 1
	bge         s3, s4, .CompoundStatementASTNodePrint_label_29

	// *** Basic block 6

.CompoundStatementASTNodePrint_label_50:
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
.func_end_CompoundStatementASTNodePrint:
	.size CompoundStatementASTNodePrint, .func_end_CompoundStatementASTNodePrint-CompoundStatementASTNodePrint

	.local  CompoundStatementASTNodeReplaceChild
	.type CompoundStatementASTNodeReplaceChild, @function

CompoundStatementASTNodeReplaceChild:

	// *** Basic block 0

	.local SetParent
	.global ASTNodeDelete
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
	mv          t0, a0
	mv          t1, a1
	mv          t2, a2
	mv          s1, a3
	mv          t3, t0
	ld          t4, 56(t3)
	ld          t4, 0(t4)
	slli        t5, t1, 3
	add         t4, t4, t5
	ld          s2, 0(t4)
	ld          t4, 56(t3)
	ld          t4, 0(t4)
	add         t4, t4, t5
	sd          t2, 0(t4)
	mv          a2, t1
	mv          a1, t0
	mv          a0, t2
	call        SetParent

	// *** Basic block 1

	beqz        s1, .CompoundStatementASTNodeReplaceChild_label_47

	// *** Basic block 2

	mv          a0, s2
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodeDelete

	// *** Basic block 3

.CompoundStatementASTNodeReplaceChild_label_47:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_CompoundStatementASTNodeReplaceChild:
	.size CompoundStatementASTNodeReplaceChild, .func_end_CompoundStatementASTNodeReplaceChild-CompoundStatementASTNodeReplaceChild

	.local  CompoundStatementASTNodeClone
	.type CompoundStatementASTNodeClone, @function

CompoundStatementASTNodeClone:

	// *** Basic block 0

	.global malloc
	.local ASTNodeBaseCopy
	.global NewVector
	.global ASTNodeClone
	.global VectorAppend
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
	mv          s3, a2
	mv          s4, s1
	li          a0, 80		// 0x50 ASCII 'P'
	call        malloc

	// *** Basic block 1

	mv          s5, a0
	mv          a1, s1
	mv          a0, s5
	call        ASTNodeBaseCopy

	// *** Basic block 2

	call        NewVector

	// *** Basic block 3

	sd          a0, 56(s5)
	mv          s6, x0
	ld          t0, 56(s4)
	ld          s7, 8(t0)
	bge         x0, s7, .CompoundStatementASTNodeClone_label_73

	// *** Basic block 4

	ld          s1, 0(t0)

	// *** Basic block 5

.CompoundStatementASTNodeClone_label_47:
	slli        t0, s6, 3
	add         t0, s1, t0
	ld          a0, 0(t0)
	mv          a3, s5
	mv          a2, s3
	mv          a1, s2
	call        ASTNodeClone

	// *** Basic block 6

	mv          s1, a0
	ld          a0, 56(s5)
	mv          a1, s1
	call        VectorAppend

	// *** Basic block 7

.CompoundStatementASTNodeClone_label_69:
	addi        s6, s6, 1
	bge         s6, s7, .CompoundStatementASTNodeClone_label_47

	// *** Basic block 8

.CompoundStatementASTNodeClone_label_73:
	mv          a1, s3
	mv          a0, s5
	mv          t0, s2
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
	jr          t0
.func_end_CompoundStatementASTNodeClone:
	.size CompoundStatementASTNodeClone, .func_end_CompoundStatementASTNodeClone-CompoundStatementASTNodeClone

	.local  CompoundStatementASTNodeVisit
	.type CompoundStatementASTNodeVisit, @function

CompoundStatementASTNodeVisit:

	// *** Basic block 0

	.global ASTNodeVisit
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
	mv          s3, a3
	mv          s4, a2
	mv          s5, s1
	mv          a3, x0
	mv          a1, s3
	jalr         x1, s2, 0

	// *** Basic block 1

	mv          s6, x0
	ld          t0, 56(s5)
	ld          s7, 8(t0)
	bge         x0, s7, .CompoundStatementASTNodeVisit_label_60

	// *** Basic block 2

	ld          s5, 0(t0)

	// *** Basic block 3

.CompoundStatementASTNodeVisit_label_43:
	slli        t0, s6, 3
	add         t0, s5, t0
	ld          a0, 0(t0)
	sext.w      a2, s6
	mv          a3, s3
	mv          a1, s2
	call        ASTNodeVisit

	// *** Basic block 4

.CompoundStatementASTNodeVisit_label_56:
	addi        s6, s6, 1
	bge         s6, s7, .CompoundStatementASTNodeVisit_label_43

	// *** Basic block 5

.CompoundStatementASTNodeVisit_label_60:
	li          t0, 1		// 0x1 ASCII \x1
	mv          a3, t0
	mv          a2, s4
	mv          a1, s3
	mv          a0, s1
	mv          t0, s2
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
	jr          t0
.func_end_CompoundStatementASTNodeVisit:
	.size CompoundStatementASTNodeVisit, .func_end_CompoundStatementASTNodeVisit-CompoundStatementASTNodeVisit

	.global NewCompoundStatementASTNode
	.type NewCompoundStatementASTNode, @function

NewCompoundStatementASTNode:

	// *** Basic block 0

	.global malloc
	.global ASTNodeInit
	.local compound_stmt_vtbl
	.global VectorFirst
	.global VectorLast
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
	li          a0, 80		// 0x50 ASCII 'P'
	call        malloc

	// *** Basic block 1

	mv          s3, a0
	la          a4, compound_stmt_vtbl
	mv          a3, s1
	mv          a2, x0
	li          t0, 41		// 0x29 ASCII ')'
	mv          a1, t0
	mv          a0, s3
	call        ASTNodeInit

	// *** Basic block 2

	sd          s2, 56(s3)
	mv          s4, x0
	ld          s5, 56(s3)
	ld          s6, 8(s5)
	bge         x0, s6, .NewCompoundStatementASTNode_label_70

	// *** Basic block 3

	ld          t0, 0(s5)

	// *** Basic block 4

.NewCompoundStatementASTNode_label_55:
	slli        t1, s4, 3
	add         t0, t0, t1
	ld          s1, 0(t0)
	sd          s3, 24(s1)
	sext.w      t0, s4
	sw          t0, 32(s1)

	// *** Basic block 5

.NewCompoundStatementASTNode_label_66:
	addi        s4, s4, 1
	bge         s4, s6, .NewCompoundStatementASTNode_label_55

	// *** Basic block 6

.NewCompoundStatementASTNode_label_70:
	mv          a0, s2
	call        VectorFirst

	// *** Basic block 7

	sd          a0, 64(s3)
	mv          a0, s2
	call        VectorLast

	// *** Basic block 8

	sd          a0, 72(s3)
	mv          a0, s3

	// *** Basic block 9

.NewCompoundStatementASTNode_label_83:
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
.func_end_NewCompoundStatementASTNode:
	.size NewCompoundStatementASTNode, .func_end_NewCompoundStatementASTNode-NewCompoundStatementASTNode

	.global CompoundASTNodeInsertStatement
	.type CompoundASTNodeInsertStatement, @function

CompoundASTNodeInsertStatement:

	// *** Basic block 0

	.global VectorInsertBefore
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
	mv          s2, a2
	mv          s3, a1
	ld          a0, 56(s1)
	mv          a2, s3
	mv          a1, s2
	call        VectorInsertBefore

	// *** Basic block 1

	sd          s1, 24(s3)
	sext.w      t0, s2
	sw          t0, 32(s3)
	addi        s4, s2, 1
	ld          t0, 56(s1)
	ld          t1, 8(t0)
	bge         s4, t1, .CompoundASTNodeInsertStatement_label_56

	// *** Basic block 2

	ld          t0, 0(t0)

	// *** Basic block 3

.CompoundASTNodeInsertStatement_label_42:
	slli        t2, s4, 3
	add         t0, t0, t2
	ld          s1, 0(t0)
	lw          t0, 32(s1)
	addi        t0, t0, 1
	sw          t0, 32(s1)

	// *** Basic block 4

.CompoundASTNodeInsertStatement_label_52:
	addi        s4, s4, 1
	bge         s4, t1, .CompoundASTNodeInsertStatement_label_42

	// *** Basic block 5

.CompoundASTNodeInsertStatement_label_56:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_CompoundASTNodeInsertStatement:
	.size CompoundASTNodeInsertStatement, .func_end_CompoundASTNodeInsertStatement-CompoundASTNodeInsertStatement

	.local  ForStatementASTNodeDelete
	.type ForStatementASTNodeDelete, @function

ForStatementASTNodeDelete:

	// *** Basic block 0

	.global ASTNodeDelete
	.local ASTNodeBaseDelete
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
	mv          s2, s1
	ld          s3, 56(s2)
	beq         s3, x0, .ForStatementASTNodeDelete_label_23

	// *** Basic block 1

	mv          a0, s3
	call        ASTNodeDelete

	// *** Basic block 2

.ForStatementASTNodeDelete_label_23:
	ld          s3, 64(s2)
	beq         s3, x0, .ForStatementASTNodeDelete_label_32

	// *** Basic block 3

	mv          a0, s3
	call        ASTNodeDelete

	// *** Basic block 4

.ForStatementASTNodeDelete_label_32:
	ld          s3, 72(s2)
	beq         s3, x0, .ForStatementASTNodeDelete_label_41

	// *** Basic block 5

	mv          a0, s3
	call        ASTNodeDelete

	// *** Basic block 6

.ForStatementASTNodeDelete_label_41:
	ld          s3, 80(s2)
	beq         s3, x0, .ForStatementASTNodeDelete_label_50

	// *** Basic block 7

	mv          a0, s3
	call        ASTNodeDelete

	// *** Basic block 8

.ForStatementASTNodeDelete_label_50:
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodeBaseDelete
.func_end_ForStatementASTNodeDelete:
	.size ForStatementASTNodeDelete, .func_end_ForStatementASTNodeDelete-ForStatementASTNodeDelete

	.local  ForStatementASTNodePrint
	.type ForStatementASTNodePrint, @function

ForStatementASTNodePrint:

	// *** Basic block 0

	.local Indent
	.global fprintf
	.global ASTNodePrint
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
	mv          t0, a0
	mv          s1, a1
	mv          s2, a2
	mv          s3, t0
	mv          a1, s2
	mv          a0, s1
	call        Indent

	// *** Basic block 1

	lla         a1, .str.195
	mv          a0, s2
	call        fprintf

	// *** Basic block 2

	addi        s4, s1, 2
	mv          a1, s2
	mv          a0, s4
	call        Indent

	// *** Basic block 3

	ld          s5, 56(s3)
	beq         s5, x0, .ForStatementASTNodePrint_label_56

	// *** Basic block 4

	mv          a2, s2
	mv          a1, s4
	mv          a0, s5
	call        ASTNodePrint

	// *** Basic block 5

.ForStatementASTNodePrint_label_56:
	mv          a1, s2
	mv          a0, s1
	call        Indent

	// *** Basic block 6

	lla         a1, .str.196
	mv          a0, s2
	call        fprintf

	// *** Basic block 7

	ld          s5, 64(s3)
	beq         s5, x0, .ForStatementASTNodePrint_label_80

	// *** Basic block 8

	mv          a2, s2
	mv          a1, s4
	mv          a0, s5
	call        ASTNodePrint

	// *** Basic block 9

.ForStatementASTNodePrint_label_80:
	mv          a1, s2
	mv          a0, s1
	call        Indent

	// *** Basic block 10

	lla         a1, .str.197
	mv          a0, s2
	call        fprintf

	// *** Basic block 11

	ld          s5, 72(s3)
	beq         s5, x0, .ForStatementASTNodePrint_label_104

	// *** Basic block 12

	mv          a2, s2
	mv          a1, s4
	mv          a0, s5
	call        ASTNodePrint

	// *** Basic block 13

.ForStatementASTNodePrint_label_104:
	ld          t0, 80(s3)
	beq         t0, x0, .ForStatementASTNodePrint_label_118

	// *** Basic block 14

	addi        a1, s1, 4
	mv          a2, s2
	mv          a0, t0
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodePrint

	// *** Basic block 15

.ForStatementASTNodePrint_label_118:
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
.func_end_ForStatementASTNodePrint:
	.size ForStatementASTNodePrint, .func_end_ForStatementASTNodePrint-ForStatementASTNodePrint

	.local  ForStatementASTNodeReplaceChild
	.type ForStatementASTNodeReplaceChild, @function

ForStatementASTNodeReplaceChild:

	// *** Basic block 0

	.local SetParent
	.global ASTNodeDelete
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
	mv          s4, a3
	mv          s5, s1
	mv          s6, x0
	blt         s2, x0, .ForStatementASTNodeReplaceChild_label_67

	// *** Basic block 1

	li          t0, 3		// 0x3 ASCII \x3
	blt         t0, s2, .ForStatementASTNodeReplaceChild_label_67

	// *** Basic block 2

	slli        t0, s2, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 3

	j           .ForStatementASTNodeReplaceChild_label_43

	// *** Basic block 4

	j           .ForStatementASTNodeReplaceChild_label_49

	// *** Basic block 5

	j           .ForStatementASTNodeReplaceChild_label_55

	// *** Basic block 6

	j           .ForStatementASTNodeReplaceChild_label_61

	// *** Basic block 7

.ForStatementASTNodeReplaceChild_label_43:
	ld          s6, 56(s5)
	sd          s3, 56(s5)
	j           .ForStatementASTNodeReplaceChild_label_67

	// *** Basic block 8

.ForStatementASTNodeReplaceChild_label_49:
	ld          s6, 64(s5)
	sd          s3, 64(s5)
	j           .ForStatementASTNodeReplaceChild_label_67

	// *** Basic block 9

.ForStatementASTNodeReplaceChild_label_55:
	ld          s6, 72(s5)
	sd          s3, 72(s5)
	j           .ForStatementASTNodeReplaceChild_label_67

	// *** Basic block 10

.ForStatementASTNodeReplaceChild_label_61:
	ld          s6, 80(s5)
	sd          s3, 80(s5)
	j           .ForStatementASTNodeReplaceChild_label_67

	// *** Basic block 11

.ForStatementASTNodeReplaceChild_label_67:
	mv          a2, s2
	mv          a1, s1
	mv          a0, s3
	call        SetParent

	// *** Basic block 12

	beqz        s4, .ForStatementASTNodeReplaceChild_label_80

	// *** Basic block 13

	mv          a0, s6
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
	j           ASTNodeDelete

	// *** Basic block 14

.ForStatementASTNodeReplaceChild_label_80:
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
.func_end_ForStatementASTNodeReplaceChild:
	.size ForStatementASTNodeReplaceChild, .func_end_ForStatementASTNodeReplaceChild-ForStatementASTNodeReplaceChild

	.local  ForStatementASTNodeClone
	.type ForStatementASTNodeClone, @function

ForStatementASTNodeClone:

	// *** Basic block 0

	.global malloc
	.local ASTNodeBaseCopy
	.global ASTNodeClone
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
	mv          s3, a2
	mv          s4, s1
	li          a0, 88		// 0x58 ASCII 'X'
	call        malloc

	// *** Basic block 1

	mv          s5, a0
	mv          a1, s1
	mv          a0, s5
	call        ASTNodeBaseCopy

	// *** Basic block 2

	ld          a0, 56(s4)
	mv          a3, s5
	mv          a2, s3
	mv          a1, s2
	call        ASTNodeClone

	// *** Basic block 3

	sd          a0, 56(s5)
	ld          a0, 64(s4)
	mv          a3, s5
	mv          a2, s3
	mv          a1, s2
	call        ASTNodeClone

	// *** Basic block 4

	sd          a0, 64(s5)
	ld          a0, 72(s4)
	mv          a3, s5
	mv          a2, s3
	mv          a1, s2
	call        ASTNodeClone

	// *** Basic block 5

	sd          a0, 72(s5)
	ld          a0, 80(s4)
	mv          a3, s5
	mv          a2, s3
	mv          a1, s2
	call        ASTNodeClone

	// *** Basic block 6

	sd          a0, 80(s5)
	mv          a1, s3
	mv          a0, s5
	mv          t0, s2
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	jr          t0
.func_end_ForStatementASTNodeClone:
	.size ForStatementASTNodeClone, .func_end_ForStatementASTNodeClone-ForStatementASTNodeClone

	.local  ForStatementASTNodeVisit
	.type ForStatementASTNodeVisit, @function

ForStatementASTNodeVisit:

	// *** Basic block 0

	.global ASTNodeVisit
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
	mv          s3, a3
	mv          s4, a2
	mv          s5, s1
	mv          a3, x0
	mv          a1, s3
	jalr         x1, s2, 0

	// *** Basic block 1

	ld          a0, 56(s5)
	mv          a3, s3
	mv          a2, x0
	mv          a1, s2
	call        ASTNodeVisit

	// *** Basic block 2

	ld          a0, 64(s5)
	mv          a3, s3
	li          s6, 1		// 0x1 ASCII \x1
	mv          a2, s6
	mv          a1, s2
	call        ASTNodeVisit

	// *** Basic block 3

	ld          a0, 72(s5)
	mv          a3, s3
	li          t0, 2		// 0x2 ASCII \x2
	mv          a2, t0
	mv          a1, s2
	call        ASTNodeVisit

	// *** Basic block 4

	ld          a0, 80(s5)
	mv          a3, s3
	li          t0, 3		// 0x3 ASCII \x3
	mv          a2, t0
	mv          a1, s2
	call        ASTNodeVisit

	// *** Basic block 5

	mv          a3, s6
	mv          a2, s4
	mv          a1, s3
	mv          a0, s1
	mv          t0, s2
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
	jr          t0
.func_end_ForStatementASTNodeVisit:
	.size ForStatementASTNodeVisit, .func_end_ForStatementASTNodeVisit-ForStatementASTNodeVisit

	.local  ForStatementASTNodeUsesValue
	.type ForStatementASTNodeUsesValue, @function

ForStatementASTNodeUsesValue:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	ld          t1, 64(t0)
	sub         t1, t1, a1
	seqz        a0, t1

	// *** Basic block 1

.ForStatementASTNodeUsesValue_label_17:
	ret         
.func_end_ForStatementASTNodeUsesValue:
	.size ForStatementASTNodeUsesValue, .func_end_ForStatementASTNodeUsesValue-ForStatementASTNodeUsesValue

	.global NewForStatementASTNode
	.type NewForStatementASTNode, @function

NewForStatementASTNode:

	// *** Basic block 0

	.global malloc
	.global ASTNodeInit
	.local for_stmt_vtbl
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
	mv          s1, a4
	mv          s2, a0
	mv          s3, a1
	mv          s4, a2
	mv          s5, a3
	li          a0, 88		// 0x58 ASCII 'X'
	call        malloc

	// *** Basic block 1

	mv          s6, a0
	la          a4, for_stmt_vtbl
	mv          a3, s1
	mv          a2, x0
	li          t0, 35		// 0x23 ASCII '#'
	mv          a1, t0
	mv          a0, s6
	call        ASTNodeInit

	// *** Basic block 2

	sd          s2, 56(s6)
	beq         s2, x0, .NewForStatementASTNode_label_59

	// *** Basic block 3

	sd          s6, 24(s2)
	sw          x0, 32(s2)

	// *** Basic block 4

.NewForStatementASTNode_label_59:
	sd          s3, 64(s6)
	beq         s3, x0, .NewForStatementASTNode_label_70

	// *** Basic block 5

	sd          s6, 24(s3)
	li          t0, 1		// 0x1 ASCII \x1
	sw          t0, 32(s3)

	// *** Basic block 6

.NewForStatementASTNode_label_70:
	sd          s4, 72(s6)
	beq         s4, x0, .NewForStatementASTNode_label_81

	// *** Basic block 7

	sd          s6, 24(s4)
	li          t0, 2		// 0x2 ASCII \x2
	sw          t0, 32(s4)

	// *** Basic block 8

.NewForStatementASTNode_label_81:
	sd          s5, 80(s6)
	sd          s6, 24(s5)
	li          t0, 3		// 0x3 ASCII \x3
	sw          t0, 32(s5)
	mv          a0, s6

	// *** Basic block 9

.NewForStatementASTNode_label_91:
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
.func_end_NewForStatementASTNode:
	.size NewForStatementASTNode, .func_end_NewForStatementASTNode-NewForStatementASTNode

	.local  VariableDeclarationASTNodeDelete
	.type VariableDeclarationASTNodeDelete, @function

VariableDeclarationASTNodeDelete:

	// *** Basic block 0

	.global ASTNodeDelete
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
	mv          t0, s1
	ld          s2, 64(t0)
	beq         s2, x0, .VariableDeclarationASTNodeDelete_label_19

	// *** Basic block 1

	mv          a0, s2
	call        ASTNodeDelete

	// *** Basic block 2

.VariableDeclarationASTNodeDelete_label_19:
	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodeDelete
.func_end_VariableDeclarationASTNodeDelete:
	.size VariableDeclarationASTNodeDelete, .func_end_VariableDeclarationASTNodeDelete-VariableDeclarationASTNodeDelete

	.local  VariableDeclarationASTNodePrint
	.type VariableDeclarationASTNodePrint, @function

VariableDeclarationASTNodePrint:

	// *** Basic block 0

	.local Indent
	.global SymbolPrint
	.global fprintf
	.global ASTNodePrint
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
	mv          t0, a0
	mv          s1, a1
	mv          s2, a2
	mv          s3, t0
	mv          a1, s2
	mv          a0, s1
	call        Indent

	// *** Basic block 1

	ld          a0, 56(s3)
	mv          a1, s2
	call        SymbolPrint

	// *** Basic block 2

	ld          s4, 64(s3)
	beq         s4, x0, .VariableDeclarationASTNodePrint_label_53

	// *** Basic block 3

	lla         a1, .str.198
	mv          a0, s2
	call        fprintf

	// *** Basic block 4

	addi        a1, s1, 2
	mv          a2, s2
	mv          a0, s4
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodePrint

	// *** Basic block 5

.VariableDeclarationASTNodePrint_label_53:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_VariableDeclarationASTNodePrint:
	.size VariableDeclarationASTNodePrint, .func_end_VariableDeclarationASTNodePrint-VariableDeclarationASTNodePrint

	.local  VariableDeclarationASTNodeReplaceChild
	.type VariableDeclarationASTNodeReplaceChild, @function

VariableDeclarationASTNodeReplaceChild:

	// *** Basic block 0

	.local SetParent
	.global ASTNodeDelete
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
	mv          t0, a0
	mv          t1, a2
	mv          t2, a1
	mv          s1, a3
	mv          t3, t0
	ld          s2, 64(t3)
	sd          t1, 64(t3)
	mv          a2, t2
	mv          a1, t0
	mv          a0, t1
	call        SetParent

	// *** Basic block 1

	beqz        s1, .VariableDeclarationASTNodeReplaceChild_label_37

	// *** Basic block 2

	mv          a0, s2
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodeDelete

	// *** Basic block 3

.VariableDeclarationASTNodeReplaceChild_label_37:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_VariableDeclarationASTNodeReplaceChild:
	.size VariableDeclarationASTNodeReplaceChild, .func_end_VariableDeclarationASTNodeReplaceChild-VariableDeclarationASTNodeReplaceChild

	.local  VariableDeclarationASTNodeClone
	.type VariableDeclarationASTNodeClone, @function

VariableDeclarationASTNodeClone:

	// *** Basic block 0

	.global malloc
	.local ASTNodeBaseCopy
	.global ASTNodeClone
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
	mv          s3, a2
	mv          s4, s1
	li          a0, 80		// 0x50 ASCII 'P'
	call        malloc

	// *** Basic block 1

	mv          s5, a0
	mv          a1, s1
	mv          a0, s5
	call        ASTNodeBaseCopy

	// *** Basic block 2

	ld          t0, 56(s4)
	sd          t0, 56(s5)
	ld          a0, 64(s4)
	mv          a3, s5
	mv          a2, s3
	mv          a1, s2
	call        ASTNodeClone

	// *** Basic block 3

	sd          a0, 64(s5)
	mv          a1, s3
	mv          a0, s5
	mv          t0, s2
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	jr          t0
.func_end_VariableDeclarationASTNodeClone:
	.size VariableDeclarationASTNodeClone, .func_end_VariableDeclarationASTNodeClone-VariableDeclarationASTNodeClone

	.local  VariableDeclarationASTNodeVisit
	.type VariableDeclarationASTNodeVisit, @function

VariableDeclarationASTNodeVisit:

	// *** Basic block 0

	.global ASTNodeVisit
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
	mv          s3, a3
	mv          s4, a2
	mv          s5, s1
	mv          a3, x0
	mv          a1, s3
	jalr         x1, s2, 0

	// *** Basic block 1

	ld          a0, 64(s5)
	mv          a3, s3
	mv          a2, x0
	mv          a1, s2
	call        ASTNodeVisit

	// *** Basic block 2

	li          t0, 1		// 0x1 ASCII \x1
	mv          a3, t0
	mv          a2, s4
	mv          a1, s3
	mv          a0, s1
	mv          t0, s2
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	jr          t0
.func_end_VariableDeclarationASTNodeVisit:
	.size VariableDeclarationASTNodeVisit, .func_end_VariableDeclarationASTNodeVisit-VariableDeclarationASTNodeVisit

	.global NewVariableDeclarationASTNode
	.type NewVariableDeclarationASTNode, @function

NewVariableDeclarationASTNode:

	// *** Basic block 0

	.global malloc
	.global ASTNodeInit
	.local var_decl_vtbl
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
	mv          s2, a2
	mv          s3, a1
	li          a0, 80		// 0x50 ASCII 'P'
	call        malloc

	// *** Basic block 1

	mv          s4, a0
	ld          a2, 40(s1)
	la          a4, var_decl_vtbl
	mv          a3, s2
	li          t0, 82		// 0x52 ASCII 'R'
	mv          a1, t0
	mv          a0, s4
	call        ASTNodeInit

	// *** Basic block 2

	sd          s1, 56(s4)
	sd          s3, 64(s4)
	beq         s3, x0, .NewVariableDeclarationASTNode_label_52

	// *** Basic block 3

	sd          s4, 24(s3)

	// *** Basic block 4

.NewVariableDeclarationASTNode_label_52:
	sd          x0, 72(s4)
	mv          a0, s4

	// *** Basic block 5

.NewVariableDeclarationASTNode_label_57:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewVariableDeclarationASTNode:
	.size NewVariableDeclarationASTNode, .func_end_NewVariableDeclarationASTNode-NewVariableDeclarationASTNode

	.local  DeclarationListASTNodeDelete
	.type DeclarationListASTNodeDelete, @function

DeclarationListASTNodeDelete:

	// *** Basic block 0

	.global ASTNodeDelete
	.global VectorDestruct
	.local ASTNodeBaseDelete
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
	mv          t0, s1
	mv          s2, x0
	ld          s3, 56(t0)
	ld          s4, 8(s3)
	bge         x0, s4, .DeclarationListASTNodeDelete_label_42

	// *** Basic block 1

	ld          t0, 0(s3)

	// *** Basic block 2

.DeclarationListASTNodeDelete_label_25:
	slli        t1, s2, 3
	add         t0, t0, t1
	ld          s5, 0(t0)
	beq         s5, x0, .DeclarationListASTNodeDelete_label_37

	// *** Basic block 3

	mv          a0, s5
	call        ASTNodeDelete

	// *** Basic block 4

.DeclarationListASTNodeDelete_label_37:

	// *** Basic block 5

.DeclarationListASTNodeDelete_label_38:
	addi        s2, s2, 1
	bge         s2, s4, .DeclarationListASTNodeDelete_label_25

	// *** Basic block 6

.DeclarationListASTNodeDelete_label_42:
	mv          a0, s3
	call        VectorDestruct

	// *** Basic block 7

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
	j           ASTNodeBaseDelete
.func_end_DeclarationListASTNodeDelete:
	.size DeclarationListASTNodeDelete, .func_end_DeclarationListASTNodeDelete-DeclarationListASTNodeDelete

	.local  DeclarationListASTNodePrint
	.type DeclarationListASTNodePrint, @function

DeclarationListASTNodePrint:

	// *** Basic block 0

	.global ASTNodePrint
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
	mv          t0, a0
	mv          s3, x0
	ld          t1, 56(t0)
	ld          s4, 8(t1)
	bge         x0, s4, .DeclarationListASTNodePrint_label_50

	// *** Basic block 1

	ld          t0, 0(t1)

	// *** Basic block 2

.DeclarationListASTNodePrint_label_29:
	slli        t1, s3, 3
	add         t0, t0, t1
	ld          s5, 0(t0)
	beq         s5, x0, .DeclarationListASTNodePrint_label_45

	// *** Basic block 3

	mv          a2, s2
	mv          a1, s1
	mv          a0, s5
	call        ASTNodePrint

	// *** Basic block 4

.DeclarationListASTNodePrint_label_45:

	// *** Basic block 5

.DeclarationListASTNodePrint_label_46:
	addi        s3, s3, 1
	bge         s3, s4, .DeclarationListASTNodePrint_label_29

	// *** Basic block 6

.DeclarationListASTNodePrint_label_50:
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
.func_end_DeclarationListASTNodePrint:
	.size DeclarationListASTNodePrint, .func_end_DeclarationListASTNodePrint-DeclarationListASTNodePrint

	.local  DeclarationListASTNodeClone
	.type DeclarationListASTNodeClone, @function

DeclarationListASTNodeClone:

	// *** Basic block 0

	.global malloc
	.local ASTNodeBaseCopy
	.global NewVector
	.global ASTNodeClone
	.global VectorAppend
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
	mv          s3, a2
	mv          s4, s1
	li          a0, 64		// 0x40 ASCII '@'
	call        malloc

	// *** Basic block 1

	mv          s5, a0
	mv          a1, s1
	mv          a0, s5
	call        ASTNodeBaseCopy

	// *** Basic block 2

	call        NewVector

	// *** Basic block 3

	sd          a0, 56(s5)
	mv          s6, x0
	ld          t0, 56(s4)
	ld          s7, 8(t0)
	bge         x0, s7, .DeclarationListASTNodeClone_label_73

	// *** Basic block 4

	ld          s1, 0(t0)

	// *** Basic block 5

.DeclarationListASTNodeClone_label_47:
	slli        t0, s6, 3
	add         t0, s1, t0
	ld          a0, 0(t0)
	mv          a3, s5
	mv          a2, s3
	mv          a1, s2
	call        ASTNodeClone

	// *** Basic block 6

	mv          s1, a0
	ld          a0, 56(s5)
	mv          a1, s1
	call        VectorAppend

	// *** Basic block 7

.DeclarationListASTNodeClone_label_69:
	addi        s6, s6, 1
	bge         s6, s7, .DeclarationListASTNodeClone_label_47

	// *** Basic block 8

.DeclarationListASTNodeClone_label_73:
	mv          a1, s3
	mv          a0, s5
	mv          t0, s2
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
	jr          t0
.func_end_DeclarationListASTNodeClone:
	.size DeclarationListASTNodeClone, .func_end_DeclarationListASTNodeClone-DeclarationListASTNodeClone

	.local  DeclarationListASTNodeVisit
	.type DeclarationListASTNodeVisit, @function

DeclarationListASTNodeVisit:

	// *** Basic block 0

	.global ASTNodeVisit
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
	mv          s3, a3
	mv          s4, a2
	mv          s5, s1
	mv          a3, x0
	mv          a1, s3
	jalr         x1, s2, 0

	// *** Basic block 1

	mv          s6, x0
	ld          t0, 56(s5)
	ld          s7, 8(t0)
	bge         x0, s7, .DeclarationListASTNodeVisit_label_60

	// *** Basic block 2

	ld          s5, 0(t0)

	// *** Basic block 3

.DeclarationListASTNodeVisit_label_43:
	slli        t0, s6, 3
	add         t0, s5, t0
	ld          a0, 0(t0)
	sext.w      a2, s6
	mv          a3, s3
	mv          a1, s2
	call        ASTNodeVisit

	// *** Basic block 4

.DeclarationListASTNodeVisit_label_56:
	addi        s6, s6, 1
	bge         s6, s7, .DeclarationListASTNodeVisit_label_43

	// *** Basic block 5

.DeclarationListASTNodeVisit_label_60:
	li          t0, 1		// 0x1 ASCII \x1
	mv          a3, t0
	mv          a2, s4
	mv          a1, s3
	mv          a0, s1
	mv          t0, s2
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
	jr          t0
.func_end_DeclarationListASTNodeVisit:
	.size DeclarationListASTNodeVisit, .func_end_DeclarationListASTNodeVisit-DeclarationListASTNodeVisit

	.global NewDeclarationListASTNode
	.type NewDeclarationListASTNode, @function

NewDeclarationListASTNode:

	// *** Basic block 0

	.global malloc
	.global ASTNodeInit
	.local decl_list_vtbl
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
	li          a0, 64		// 0x40 ASCII '@'
	call        malloc

	// *** Basic block 1

	mv          s3, a0
	la          a4, decl_list_vtbl
	mv          a3, s1
	mv          a2, x0
	li          t0, 83		// 0x53 ASCII 'S'
	mv          a1, t0
	mv          a0, s3
	call        ASTNodeInit

	// *** Basic block 2

	sd          s2, 56(s3)
	mv          a0, s3

	// *** Basic block 3

.NewDeclarationListASTNode_label_40:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewDeclarationListASTNode:
	.size NewDeclarationListASTNode, .func_end_NewDeclarationListASTNode-NewDeclarationListASTNode

	.local  CaseLabelASTNodeDelete
	.type CaseLabelASTNodeDelete, @function

CaseLabelASTNodeDelete:

	// *** Basic block 0

	.global ASTNodeDelete
	.local ASTNodeBaseDelete
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
	mv          s2, s1
	ld          s3, 56(s2)
	beq         s3, x0, .CaseLabelASTNodeDelete_label_21

	// *** Basic block 1

	mv          a0, s3
	call        ASTNodeDelete

	// *** Basic block 2

.CaseLabelASTNodeDelete_label_21:
	ld          s3, 64(s2)
	beq         s3, x0, .CaseLabelASTNodeDelete_label_30

	// *** Basic block 3

	mv          a0, s3
	call        ASTNodeDelete

	// *** Basic block 4

.CaseLabelASTNodeDelete_label_30:
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodeBaseDelete
.func_end_CaseLabelASTNodeDelete:
	.size CaseLabelASTNodeDelete, .func_end_CaseLabelASTNodeDelete-CaseLabelASTNodeDelete

	.local  CaseLabelASTNodePrint
	.type CaseLabelASTNodePrint, @function

CaseLabelASTNodePrint:

	// *** Basic block 0

	.global ASTNodePrint
	.local ASTNodeBasePrint
	.local Indent
	.global fprintf
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
	ld          s4, 56(s3)
	beq         s4, x0, .CaseLabelASTNodePrint_label_44

	// *** Basic block 1

	addi        s5, s1, 2
	mv          a2, s2
	mv          a1, s5
	mv          a0, s4
	call        ASTNodePrint

	// *** Basic block 2

	mv          a2, s2
	mv          a1, s5
	mv          a0, s3
	call        ASTNodeBasePrint

	// *** Basic block 3

	j           .CaseLabelASTNodePrint_label_56

	// *** Basic block 4

.CaseLabelASTNodePrint_label_44:
	mv          a1, s2
	mv          a0, s1
	call        Indent

	// *** Basic block 5

	lla         a1, .str.199
	mv          a0, s2
	call        fprintf

	// *** Basic block 6

.CaseLabelASTNodePrint_label_56:
	ld          a0, 64(s3)
	addi        a1, s1, 2
	mv          a2, s2
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodePrint
.func_end_CaseLabelASTNodePrint:
	.size CaseLabelASTNodePrint, .func_end_CaseLabelASTNodePrint-CaseLabelASTNodePrint

	.local  CaseLabelASTNodeClone
	.type CaseLabelASTNodeClone, @function

CaseLabelASTNodeClone:

	// *** Basic block 0

	.global malloc
	.local ASTNodeBaseCopy
	.global ASTNodeClone
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
	mv          s3, a2
	mv          s4, s1
	li          a0, 88		// 0x58 ASCII 'X'
	call        malloc

	// *** Basic block 1

	mv          s5, a0
	mv          a1, s1
	mv          a0, s5
	call        ASTNodeBaseCopy

	// *** Basic block 2

	ld          t0, 72(s4)
	sd          t0, 72(s5)
	ld          a0, 56(s4)
	mv          a3, s5
	mv          a2, s3
	mv          a1, s2
	call        ASTNodeClone

	// *** Basic block 3

	sd          a0, 56(s5)
	ld          a0, 64(s4)
	mv          a3, s5
	mv          a2, s3
	mv          a1, s2
	call        ASTNodeClone

	// *** Basic block 4

	sd          a0, 64(s5)
	ld          t0, 80(s4)
	sd          t0, 80(s5)
	mv          a1, s3
	mv          a0, s5
	mv          t0, s2
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	jr          t0
.func_end_CaseLabelASTNodeClone:
	.size CaseLabelASTNodeClone, .func_end_CaseLabelASTNodeClone-CaseLabelASTNodeClone

	.local  CaseLabelASTNodeVisit
	.type CaseLabelASTNodeVisit, @function

CaseLabelASTNodeVisit:

	// *** Basic block 0

	.global ASTNodeVisit
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
	mv          s3, a3
	mv          s4, a2
	mv          s5, s1
	mv          a3, x0
	mv          a1, s3
	jalr         x1, s2, 0

	// *** Basic block 1

	ld          a0, 56(s5)
	mv          a3, s3
	mv          a2, x0
	mv          a1, s2
	call        ASTNodeVisit

	// *** Basic block 2

	ld          a0, 64(s5)
	mv          a3, s3
	li          s6, 1		// 0x1 ASCII \x1
	mv          a2, s6
	mv          a1, s2
	call        ASTNodeVisit

	// *** Basic block 3

	mv          a3, s6
	mv          a2, s4
	mv          a1, s3
	mv          a0, s1
	mv          t0, s2
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
	jr          t0
.func_end_CaseLabelASTNodeVisit:
	.size CaseLabelASTNodeVisit, .func_end_CaseLabelASTNodeVisit-CaseLabelASTNodeVisit

	.local  CaseLabelASTNodeReplaceChild
	.type CaseLabelASTNodeReplaceChild, @function

CaseLabelASTNodeReplaceChild:

	// *** Basic block 0

	.global printf
	.global abort
	.local SetParent
	.global ASTNodeDelete
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
	mv          s4, a3
	mv          s5, s1
	blt         s2, x0, .CaseLabelASTNodeReplaceChild_label_56

	// *** Basic block 1

	li          t0, 1		// 0x1 ASCII \x1
	blt         t0, s2, .CaseLabelASTNodeReplaceChild_label_56

	// *** Basic block 2

	slli        t0, s2, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 3

	j           .CaseLabelASTNodeReplaceChild_label_43

	// *** Basic block 4

	j           .CaseLabelASTNodeReplaceChild_label_50

	// *** Basic block 5

.CaseLabelASTNodeReplaceChild_label_43:
	ld          s6, 56(s5)
	sd          s3, 56(s5)
	j           .CaseLabelASTNodeReplaceChild_label_72

	// *** Basic block 6

.CaseLabelASTNodeReplaceChild_label_50:
	ld          s6, 64(s5)
	sd          s3, 64(s5)
	j           .CaseLabelASTNodeReplaceChild_label_72

	// *** Basic block 7

.CaseLabelASTNodeReplaceChild_label_56:
	lla         a0, .str.200
	lla         a1, .str.201
	lla         a3, .str.202
	li          t0, 2006		// 0x7d6
	mv          a2, t0
	call        printf

	// *** Basic block 8

	call        abort

	// *** Basic block 9

	j           .CaseLabelASTNodeReplaceChild_label_72

	// *** Basic block 10

.CaseLabelASTNodeReplaceChild_label_72:
	mv          a2, s2
	mv          a1, s1
	mv          a0, s3
	call        SetParent

	// *** Basic block 11

	beqz        s4, .CaseLabelASTNodeReplaceChild_label_85

	// *** Basic block 12

	mv          a0, s6
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
	j           ASTNodeDelete

	// *** Basic block 13

.CaseLabelASTNodeReplaceChild_label_85:
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
.func_end_CaseLabelASTNodeReplaceChild:
	.size CaseLabelASTNodeReplaceChild, .func_end_CaseLabelASTNodeReplaceChild-CaseLabelASTNodeReplaceChild

	.global NewCaseLabelASTNode
	.type NewCaseLabelASTNode, @function

NewCaseLabelASTNode:

	// *** Basic block 0

	.global malloc
	.global ASTNodeInit
	.local case_label_vtbl
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
	li          a0, 88		// 0x58 ASCII 'X'
	call        malloc

	// *** Basic block 1

	mv          s4, a0
	la          a4, case_label_vtbl
	mv          a3, s1
	mv          a2, x0
	li          t0, 26		// 0x1a ASCII \x1a
	mv          a1, t0
	mv          a0, s4
	call        ASTNodeInit

	// *** Basic block 2

	sd          s2, 56(s4)
	beq         s2, x0, .NewCaseLabelASTNode_label_54

	// *** Basic block 3

	sd          s4, 24(s2)
	sw          x0, 32(s2)

	// *** Basic block 4

.NewCaseLabelASTNode_label_54:
	sd          s3, 64(s4)
	ld          t0, 64(s4)
	beq         t0, x0, .NewCaseLabelASTNode_label_66

	// *** Basic block 5

	sd          s4, 24(s3)
	li          t0, 1		// 0x1 ASCII \x1
	sw          t0, 32(s3)

	// *** Basic block 6

.NewCaseLabelASTNode_label_66:
	sd          x0, 72(s4)
	sd          x0, 80(s4)
	mv          a0, s4

	// *** Basic block 7

.NewCaseLabelASTNode_label_73:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewCaseLabelASTNode:
	.size NewCaseLabelASTNode, .func_end_NewCaseLabelASTNode-NewCaseLabelASTNode

	.local  SwitchStatementASTNodeDelete
	.type SwitchStatementASTNodeDelete, @function

SwitchStatementASTNodeDelete:

	// *** Basic block 0

	.global ASTNodeDelete
	.local ASTNodeBaseDelete
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
	mv          s2, s1
	ld          s3, 56(s2)
	beq         s3, x0, .SwitchStatementASTNodeDelete_label_21

	// *** Basic block 1

	mv          a0, s3
	call        ASTNodeDelete

	// *** Basic block 2

.SwitchStatementASTNodeDelete_label_21:
	ld          s3, 64(s2)
	beq         s3, x0, .SwitchStatementASTNodeDelete_label_30

	// *** Basic block 3

	mv          a0, s3
	call        ASTNodeDelete

	// *** Basic block 4

.SwitchStatementASTNodeDelete_label_30:
	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodeBaseDelete
.func_end_SwitchStatementASTNodeDelete:
	.size SwitchStatementASTNodeDelete, .func_end_SwitchStatementASTNodeDelete-SwitchStatementASTNodeDelete

	.local  SwitchStatementASTNodePrint
	.type SwitchStatementASTNodePrint, @function

SwitchStatementASTNodePrint:

	// *** Basic block 0

	.global ASTNodePrint
	.local ASTNodeBasePrint
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
	mv          s2, a2
	mv          s3, a0
	ld          s4, 56(s3)
	beq         s4, x0, .SwitchStatementASTNodePrint_label_32

	// *** Basic block 1

	addi        a1, s1, 2
	mv          a2, s2
	mv          a0, s4
	call        ASTNodePrint

	// *** Basic block 2

.SwitchStatementASTNodePrint_label_32:
	mv          a2, s2
	mv          a1, s1
	mv          a0, s3
	call        ASTNodeBasePrint

	// *** Basic block 3

	ld          t0, 64(s3)
	beq         t0, x0, .SwitchStatementASTNodePrint_label_53

	// *** Basic block 4

	addi        a1, s1, 2
	mv          a2, s2
	mv          a0, t0
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodePrint

	// *** Basic block 5

.SwitchStatementASTNodePrint_label_53:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_SwitchStatementASTNodePrint:
	.size SwitchStatementASTNodePrint, .func_end_SwitchStatementASTNodePrint-SwitchStatementASTNodePrint

	.local  SwitchStatementASTNodeReplaceChild
	.type SwitchStatementASTNodeReplaceChild, @function

SwitchStatementASTNodeReplaceChild:

	// *** Basic block 0

	.local SetParent
	.global ASTNodeDelete
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
	mv          s4, a3
	mv          s5, s1
	mv          s6, x0
	blt         s2, x0, .SwitchStatementASTNodeReplaceChild_label_51

	// *** Basic block 1

	li          t0, 1		// 0x1 ASCII \x1
	blt         t0, s2, .SwitchStatementASTNodeReplaceChild_label_51

	// *** Basic block 2

	slli        t0, s2, 2
	auipc       t1, 0
	add         t0, t1, t0
	jalr        x0, t0, 12

	// *** Basic block 3

	j           .SwitchStatementASTNodeReplaceChild_label_39

	// *** Basic block 4

	j           .SwitchStatementASTNodeReplaceChild_label_45

	// *** Basic block 5

.SwitchStatementASTNodeReplaceChild_label_39:
	ld          s6, 56(s5)
	sd          s3, 56(s5)
	j           .SwitchStatementASTNodeReplaceChild_label_51

	// *** Basic block 6

.SwitchStatementASTNodeReplaceChild_label_45:
	ld          s6, 64(s5)
	sd          s3, 64(s5)
	j           .SwitchStatementASTNodeReplaceChild_label_51

	// *** Basic block 7

.SwitchStatementASTNodeReplaceChild_label_51:
	mv          a2, s2
	mv          a1, s1
	mv          a0, s3
	call        SetParent

	// *** Basic block 8

	beqz        s4, .SwitchStatementASTNodeReplaceChild_label_64

	// *** Basic block 9

	mv          a0, s6
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
	j           ASTNodeDelete

	// *** Basic block 10

.SwitchStatementASTNodeReplaceChild_label_64:
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
.func_end_SwitchStatementASTNodeReplaceChild:
	.size SwitchStatementASTNodeReplaceChild, .func_end_SwitchStatementASTNodeReplaceChild-SwitchStatementASTNodeReplaceChild

	.local  SwitchStatementASTNodeClone
	.type SwitchStatementASTNodeClone, @function

SwitchStatementASTNodeClone:

	// *** Basic block 0

	.global malloc
	.local ASTNodeBaseCopy
	.global ASTNodeClone
	.global VectorInit
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
	mv          s3, a2
	mv          s4, s1
	li          a0, 136		// 0x88 ASCII \x88
	call        malloc

	// *** Basic block 1

	mv          s5, a0
	mv          a1, s1
	mv          a0, s5
	call        ASTNodeBaseCopy

	// *** Basic block 2

	ld          a0, 56(s4)
	mv          a3, s5
	mv          a2, s3
	mv          a1, s2
	call        ASTNodeClone

	// *** Basic block 3

	sd          a0, 56(s5)
	ld          a0, 64(s4)
	mv          a3, s5
	mv          a2, s3
	mv          a1, s2
	call        ASTNodeClone

	// *** Basic block 4

	sd          a0, 64(s5)
	addi        a0, s5, 72
	call        VectorInit

	// *** Basic block 5

	sd          x0, 96(s5)
	fmv.w.x     ft0, x0
	fsw         ft0, 104(s5)
	li          t0, 9223372036854775807		// 0x7fffffffffffffff
	sd          t0, 112(s5)
	li          t0, -9223372036854775808		// 0x8000000000000000
	sd          t0, 120(s5)
	sb          x0, 128(s5)
	lw          t0, 8(s5)
	andi        t0, t0, -9
	sw          t0, 8(s5)
	mv          a1, s3
	mv          a0, s5
	mv          t0, s2
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	jr          t0
.func_end_SwitchStatementASTNodeClone:
	.size SwitchStatementASTNodeClone, .func_end_SwitchStatementASTNodeClone-SwitchStatementASTNodeClone

	.local  SwitchStatementASTNodeVisit
	.type SwitchStatementASTNodeVisit, @function

SwitchStatementASTNodeVisit:

	// *** Basic block 0

	.global ASTNodeVisit
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
	mv          s3, a3
	mv          s4, a2
	mv          s5, s1
	mv          a3, x0
	mv          a1, s3
	jalr         x1, s2, 0

	// *** Basic block 1

	ld          a0, 56(s5)
	mv          a3, s3
	mv          a2, x0
	mv          a1, s2
	call        ASTNodeVisit

	// *** Basic block 2

	ld          a0, 64(s5)
	mv          a3, s3
	li          s6, 1		// 0x1 ASCII \x1
	mv          a2, s6
	mv          a1, s2
	call        ASTNodeVisit

	// *** Basic block 3

	mv          a3, s6
	mv          a2, s4
	mv          a1, s3
	mv          a0, s1
	mv          t0, s2
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
	jr          t0
.func_end_SwitchStatementASTNodeVisit:
	.size SwitchStatementASTNodeVisit, .func_end_SwitchStatementASTNodeVisit-SwitchStatementASTNodeVisit

	.local  SwitchStatementASTNodeUsesValue
	.type SwitchStatementASTNodeUsesValue, @function

SwitchStatementASTNodeUsesValue:

	// *** Basic block 0

	// Leaf procedure, no stack frame generated
	mv          t0, a0
	ld          t1, 56(t0)
	sub         t1, t1, a1
	seqz        a0, t1

	// *** Basic block 1

.SwitchStatementASTNodeUsesValue_label_17:
	ret         
.func_end_SwitchStatementASTNodeUsesValue:
	.size SwitchStatementASTNodeUsesValue, .func_end_SwitchStatementASTNodeUsesValue-SwitchStatementASTNodeUsesValue

	.global NewSwitchStatementASTNode
	.type NewSwitchStatementASTNode, @function

NewSwitchStatementASTNode:

	// *** Basic block 0

	.global malloc
	.global ASTNodeInit
	.local switch_stmt_vtbl
	.global VectorInit
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
	li          a0, 136		// 0x88 ASCII \x88
	call        malloc

	// *** Basic block 1

	mv          s4, a0
	la          a4, switch_stmt_vtbl
	mv          a3, s1
	mv          a2, x0
	li          t0, 70		// 0x46 ASCII 'F'
	mv          a1, t0
	mv          a0, s4
	call        ASTNodeInit

	// *** Basic block 2

	sd          s2, 56(s4)
	sd          s4, 24(s2)
	sw          x0, 32(s2)
	sd          s3, 64(s4)
	sd          s4, 24(s3)
	li          t0, 1		// 0x1 ASCII \x1
	sw          t0, 32(s3)
	addi        a0, s4, 72
	call        VectorInit

	// *** Basic block 3

	sd          x0, 96(s4)
	fmv.w.x     ft0, x0
	fsw         ft0, 104(s4)
	li          t0, 9223372036854775807		// 0x7fffffffffffffff
	sd          t0, 112(s4)
	li          t0, -9223372036854775808		// 0x8000000000000000
	sd          t0, 120(s4)
	sb          x0, 128(s4)
	mv          a0, s4

	// *** Basic block 4

.NewSwitchStatementASTNode_label_83:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewSwitchStatementASTNode:
	.size NewSwitchStatementASTNode, .func_end_NewSwitchStatementASTNode-NewSwitchStatementASTNode

	.local  LabelASTNodeDelete
	.type LabelASTNodeDelete, @function

LabelASTNodeDelete:

	// *** Basic block 0

	.global ASTNodeDelete
	.global StringDestruct
	.local ASTNodeBaseDelete
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
	mv          s2, s1
	ld          s3, 56(s2)
	beq         s3, x0, .LabelASTNodeDelete_label_22

	// *** Basic block 1

	mv          a0, s3
	call        ASTNodeDelete

	// *** Basic block 2

.LabelASTNodeDelete_label_22:
	addi        a0, s2, 64
	call        StringDestruct

	// *** Basic block 3

	mv          a0, s1
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodeBaseDelete
.func_end_LabelASTNodeDelete:
	.size LabelASTNodeDelete, .func_end_LabelASTNodeDelete-LabelASTNodeDelete

	.local  LabelASTNodePrint
	.type LabelASTNodePrint, @function

LabelASTNodePrint:

	// *** Basic block 0

	.local ASTNodeBasePrint
	.global fprintf
	.global ASTNodePrint
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
	mv          t1, a1
	mv          s1, a2
	mv          s2, t0
	addi        s3, t1, 2
	mv          a1, s3
	mv          a0, s2
	call        ASTNodeBasePrint

	// *** Basic block 1

	lla         a1, .str.203
	addi        t0, s2, 64
	ld          a2, 16(t0)
	mv          a0, s1
	call        fprintf

	// *** Basic block 2

	ld          a0, 56(s2)
	mv          a2, s1
	mv          a1, s3
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodePrint
.func_end_LabelASTNodePrint:
	.size LabelASTNodePrint, .func_end_LabelASTNodePrint-LabelASTNodePrint

	.local  LabelASTNodeClone
	.type LabelASTNodeClone, @function

LabelASTNodeClone:

	// *** Basic block 0

	.global malloc
	.local ASTNodeBaseCopy
	.global StringInit
	.global ASTNodeClone
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
	mv          s3, a2
	mv          s4, s1
	li          a0, 120		// 0x78 ASCII 'x'
	call        malloc

	// *** Basic block 1

	mv          s5, a0
	mv          a1, s1
	mv          a0, s5
	call        ASTNodeBaseCopy

	// *** Basic block 2

	addi        a0, s5, 64
	addi        t0, s4, 64
	ld          a1, 16(t0)
	call        StringInit

	// *** Basic block 3

	ld          a0, 56(s4)
	mv          a3, s5
	mv          a2, s3
	mv          a1, s2
	call        ASTNodeClone

	// *** Basic block 4

	sd          a0, 56(s5)
	ld          t0, 104(s4)
	sd          t0, 104(s5)
	mv          a1, s3
	mv          a0, s5
	mv          t0, s2
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	jr          t0
.func_end_LabelASTNodeClone:
	.size LabelASTNodeClone, .func_end_LabelASTNodeClone-LabelASTNodeClone

	.local  LabelASTNodeReplaceChild
	.type LabelASTNodeReplaceChild, @function

LabelASTNodeReplaceChild:

	// *** Basic block 0

	.global printf
	.global abort
	.local SetParent
	.global ASTNodeDelete
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
	mv          s4, a3
	mv          s5, s1
	ld          s6, 56(s5)
	bnez        s2, .LabelASTNodeReplaceChild_label_33

	// *** Basic block 1

	j           .LabelASTNodeReplaceChild_label_48

	// *** Basic block 2

.LabelASTNodeReplaceChild_label_33:
	lla         a0, .str.204
	lla         a1, .str.205
	lla         a3, .str.206
	li          t0, 2176		// 0x880
	mv          a2, t0
	call        printf

	// *** Basic block 3

	call        abort

	// *** Basic block 4

.LabelASTNodeReplaceChild_label_48:
	sd          s3, 56(s5)
	mv          a2, s2
	mv          a1, s1
	mv          a0, s3
	call        SetParent

	// *** Basic block 5

	beqz        s4, .LabelASTNodeReplaceChild_label_63

	// *** Basic block 6

	mv          a0, s6
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
	j           ASTNodeDelete

	// *** Basic block 7

.LabelASTNodeReplaceChild_label_63:
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
.func_end_LabelASTNodeReplaceChild:
	.size LabelASTNodeReplaceChild, .func_end_LabelASTNodeReplaceChild-LabelASTNodeReplaceChild

	.local  LabelASTNodeVisit
	.type LabelASTNodeVisit, @function

LabelASTNodeVisit:

	// *** Basic block 0

	.global ASTNodeVisit
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
	mv          s3, a3
	mv          s4, a2
	mv          s5, s1
	mv          a3, x0
	mv          a1, s3
	jalr         x1, s2, 0

	// *** Basic block 1

	ld          a0, 56(s5)
	mv          a3, s3
	li          s6, 1		// 0x1 ASCII \x1
	mv          a2, s6
	mv          a1, s2
	call        ASTNodeVisit

	// *** Basic block 2

	mv          a3, s6
	mv          a2, s4
	mv          a1, s3
	mv          a0, s1
	mv          t0, s2
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
	jr          t0
.func_end_LabelASTNodeVisit:
	.size LabelASTNodeVisit, .func_end_LabelASTNodeVisit-LabelASTNodeVisit

	.global NewLabelASTNode
	.type NewLabelASTNode, @function

NewLabelASTNode:

	// *** Basic block 0

	.global malloc
	.global ASTNodeInit
	.local label_vtbl
	.global StringInit
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
	mv          s4, a2
	li          a0, 120		// 0x78 ASCII 'x'
	call        malloc

	// *** Basic block 1

	mv          s5, a0
	la          a4, label_vtbl
	mv          a3, s1
	mv          a2, x0
	li          t0, 81		// 0x51 ASCII 'Q'
	mv          a1, t0
	mv          a0, s5
	call        ASTNodeInit

	// *** Basic block 2

	addi        a0, s5, 64
	mv          a1, s2
	call        StringInit

	// *** Basic block 3

	sd          s3, 56(s5)
	beq         s3, x0, .NewLabelASTNode_label_57

	// *** Basic block 4

	sd          s5, 24(s3)

	// *** Basic block 5

.NewLabelASTNode_label_57:
	sd          x0, 104(s5)
	sb          s4, 112(s5)
	mv          a0, s5

	// *** Basic block 6

.NewLabelASTNode_label_64:
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
.func_end_NewLabelASTNode:
	.size NewLabelASTNode, .func_end_NewLabelASTNode-NewLabelASTNode

	.local  AsmASTNodeDelete
	.type AsmASTNodeDelete, @function

AsmASTNodeDelete:

	// *** Basic block 0

	.global StringDelete
	.local ASTNodeBaseDelete
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
	mv          t0, s1
	ld          a0, 56(t0)
	call        StringDelete

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodeBaseDelete
.func_end_AsmASTNodeDelete:
	.size AsmASTNodeDelete, .func_end_AsmASTNodeDelete-AsmASTNodeDelete

	.local  AsmASTNodePrint
	.type AsmASTNodePrint, @function

AsmASTNodePrint:

	// *** Basic block 0

	.local ASTNodeBasePrint
	.global fprintf
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
	mv          t0, a0
	mv          t1, a1
	mv          s1, a2
	mv          s2, t0
	addi        a1, t1, 2
	mv          a0, s2
	call        ASTNodeBasePrint

	// *** Basic block 1

	lla         a1, .str.207
	ld          t0, 56(s2)
	ld          a2, 16(t0)
	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           fprintf
.func_end_AsmASTNodePrint:
	.size AsmASTNodePrint, .func_end_AsmASTNodePrint-AsmASTNodePrint

	.local  AsmASTNodeClone
	.type AsmASTNodeClone, @function

AsmASTNodeClone:

	// *** Basic block 0

	.global malloc
	.local ASTNodeBaseCopy
	.global NewString
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
	mv          s3, a2
	mv          s4, s1
	li          a0, 72		// 0x48 ASCII 'H'
	call        malloc

	// *** Basic block 1

	mv          s5, a0
	mv          a1, s1
	mv          a0, s5
	call        ASTNodeBaseCopy

	// *** Basic block 2

	ld          t0, 56(s4)
	ld          a0, 16(t0)
	call        NewString

	// *** Basic block 3

	sd          a0, 56(s5)
	lb          t0, 64(s4)
	sb          t0, 64(s5)
	mv          a1, s3
	mv          a0, s5
	mv          t0, s2
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	jr          t0
.func_end_AsmASTNodeClone:
	.size AsmASTNodeClone, .func_end_AsmASTNodeClone-AsmASTNodeClone

	.global NewAsmASTNode
	.type NewAsmASTNode, @function

NewAsmASTNode:

	// *** Basic block 0

	.global malloc
	.global NewTypeRecord
	.global ASTNodeInit
	.local asm_vtbl
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
	li          a0, 72		// 0x48 ASCII 'H'
	call        malloc

	// *** Basic block 1

	mv          s4, a0
	mv          a1, x0
	li          t0, 512		// 0x200
	mv          a0, t0
	call        NewTypeRecord

	// *** Basic block 2

	mv          s5, a0
	la          a4, asm_vtbl
	mv          a3, s1
	mv          a2, s5
	li          t0, 74		// 0x4a ASCII 'J'
	mv          a1, t0
	mv          a0, s4
	call        ASTNodeInit

	// *** Basic block 3

	sd          s2, 56(s4)
	sb          s3, 64(s4)
	mv          a0, s4

	// *** Basic block 4

.NewAsmASTNode_label_55:
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
.func_end_NewAsmASTNode:
	.size NewAsmASTNode, .func_end_NewAsmASTNode-NewAsmASTNode

	.local  ExpressionInitializerASTNodeDelete
	.type ExpressionInitializerASTNodeDelete, @function

ExpressionInitializerASTNodeDelete:

	// *** Basic block 0

	.global ASTNodeDelete
	.local ASTNodeBaseDelete
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
	mv          t0, s1
	ld          a0, 56(t0)
	call        ASTNodeDelete

	// *** Basic block 1

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodeBaseDelete
.func_end_ExpressionInitializerASTNodeDelete:
	.size ExpressionInitializerASTNodeDelete, .func_end_ExpressionInitializerASTNodeDelete-ExpressionInitializerASTNodeDelete

	.local  ExpressionInitializerASTNodePrint
	.type ExpressionInitializerASTNodePrint, @function

ExpressionInitializerASTNodePrint:

	// *** Basic block 0

	.local Indent
	.global fprintf
	.global ASTNodePrint
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
	mv          s1, a1
	mv          s2, a2
	mv          s3, t0
	mv          a1, s2
	mv          a0, s1
	call        Indent

	// *** Basic block 1

	lla         a1, .str.208
	mv          a0, s2
	call        fprintf

	// *** Basic block 2

	ld          a0, 56(s3)
	addi        a1, s1, 2
	mv          a2, s2
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodePrint
.func_end_ExpressionInitializerASTNodePrint:
	.size ExpressionInitializerASTNodePrint, .func_end_ExpressionInitializerASTNodePrint-ExpressionInitializerASTNodePrint

	.local  ExpressionInitializerASTNodeReplaceChild
	.type ExpressionInitializerASTNodeReplaceChild, @function

ExpressionInitializerASTNodeReplaceChild:

	// *** Basic block 0

	.local SetParent
	.global ASTNodeDelete
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
	mv          t0, a0
	mv          t1, a2
	mv          t2, a1
	mv          s1, a3
	mv          t3, t0
	ld          s2, 56(t3)
	sd          t1, 56(t3)
	mv          a2, t2
	mv          a1, t0
	mv          a0, t1
	call        SetParent

	// *** Basic block 1

	beqz        s1, .ExpressionInitializerASTNodeReplaceChild_label_37

	// *** Basic block 2

	mv          a0, s2
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodeDelete

	// *** Basic block 3

.ExpressionInitializerASTNodeReplaceChild_label_37:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_ExpressionInitializerASTNodeReplaceChild:
	.size ExpressionInitializerASTNodeReplaceChild, .func_end_ExpressionInitializerASTNodeReplaceChild-ExpressionInitializerASTNodeReplaceChild

	.local  ExpressionInitializerASTNodeClone
	.type ExpressionInitializerASTNodeClone, @function

ExpressionInitializerASTNodeClone:

	// *** Basic block 0

	.global malloc
	.local ASTNodeBaseCopy
	.global ASTNodeClone
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
	mv          s3, a2
	mv          s4, s1
	li          a0, 64		// 0x40 ASCII '@'
	call        malloc

	// *** Basic block 1

	mv          s5, a0
	mv          a1, s1
	mv          a0, s5
	call        ASTNodeBaseCopy

	// *** Basic block 2

	ld          a0, 56(s4)
	mv          a3, s5
	mv          a2, s3
	mv          a1, s2
	call        ASTNodeClone

	// *** Basic block 3

	sd          a0, 56(s5)
	mv          a1, s3
	mv          a0, s5
	mv          t0, s2
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	jr          t0
.func_end_ExpressionInitializerASTNodeClone:
	.size ExpressionInitializerASTNodeClone, .func_end_ExpressionInitializerASTNodeClone-ExpressionInitializerASTNodeClone

	.local  ExpressionInitializerASTNodeVisit
	.type ExpressionInitializerASTNodeVisit, @function

ExpressionInitializerASTNodeVisit:

	// *** Basic block 0

	.global ASTNodeVisit
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
	mv          s3, a3
	mv          s4, a2
	mv          s5, s1
	mv          a3, x0
	mv          a1, s3
	jalr         x1, s2, 0

	// *** Basic block 1

	ld          a0, 56(s5)
	mv          a3, s3
	mv          a2, x0
	mv          a1, s2
	call        ASTNodeVisit

	// *** Basic block 2

	li          t0, 1		// 0x1 ASCII \x1
	mv          a3, t0
	mv          a2, s4
	mv          a1, s3
	mv          a0, s1
	mv          t0, s2
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	jr          t0
.func_end_ExpressionInitializerASTNodeVisit:
	.size ExpressionInitializerASTNodeVisit, .func_end_ExpressionInitializerASTNodeVisit-ExpressionInitializerASTNodeVisit

	.global NewExpressionInitializerASTNode
	.type NewExpressionInitializerASTNode, @function

NewExpressionInitializerASTNode:

	// *** Basic block 0

	.global malloc
	.global ASTNodeInit
	.local expr_init_vtbl
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
	li          a0, 64		// 0x40 ASCII '@'
	call        malloc

	// *** Basic block 1

	mv          s3, a0
	la          a4, expr_init_vtbl
	mv          a3, s1
	mv          a2, x0
	li          t0, 87		// 0x57 ASCII 'W'
	mv          a1, t0
	mv          a0, s3
	call        ASTNodeInit

	// *** Basic block 2

	sd          s2, 56(s3)
	sd          s3, 24(s2)
	mv          a0, s3

	// *** Basic block 3

.NewExpressionInitializerASTNode_label_43:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewExpressionInitializerASTNode:
	.size NewExpressionInitializerASTNode, .func_end_NewExpressionInitializerASTNode-NewExpressionInitializerASTNode

	.local  BracedInitializerASTNodeDelete
	.type BracedInitializerASTNodeDelete, @function

BracedInitializerASTNodeDelete:

	// *** Basic block 0

	.global ASTNodeDelete
	.global VectorDelete
	.local ASTNodeBaseDelete
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
	mv          t0, s1
	mv          s2, x0
	ld          s3, 56(t0)
	ld          s4, 8(s3)
	bge         x0, s4, .BracedInitializerASTNodeDelete_label_42

	// *** Basic block 1

	ld          t0, 0(s3)

	// *** Basic block 2

.BracedInitializerASTNodeDelete_label_25:
	slli        t1, s2, 3
	add         t0, t0, t1
	ld          s5, 0(t0)
	beq         s5, x0, .BracedInitializerASTNodeDelete_label_37

	// *** Basic block 3

	mv          a0, s5
	call        ASTNodeDelete

	// *** Basic block 4

.BracedInitializerASTNodeDelete_label_37:

	// *** Basic block 5

.BracedInitializerASTNodeDelete_label_38:
	addi        s2, s2, 1
	bge         s2, s4, .BracedInitializerASTNodeDelete_label_25

	// *** Basic block 6

.BracedInitializerASTNodeDelete_label_42:
	mv          a0, s3
	call        VectorDelete

	// *** Basic block 7

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
	j           ASTNodeBaseDelete
.func_end_BracedInitializerASTNodeDelete:
	.size BracedInitializerASTNodeDelete, .func_end_BracedInitializerASTNodeDelete-BracedInitializerASTNodeDelete

	.local  BracedInitializerASTNodePrint
	.type BracedInitializerASTNodePrint, @function

BracedInitializerASTNodePrint:

	// *** Basic block 0

	.global ASTNodePrint
	.local Indent
	.global fprintf
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
	mv          s2, a2
	mv          t0, a0
	mv          s3, x0
	ld          s4, 56(t0)
	ld          s5, 8(s4)
	bge         x0, s5, .BracedInitializerASTNodePrint_label_55

	// *** Basic block 1

	ld          t0, 0(s4)
	addi        s4, s1, 2

	// *** Basic block 2

.BracedInitializerASTNodePrint_label_34:
	slli        t1, s3, 3
	add         t0, t0, t1
	ld          s6, 0(t0)
	beq         s6, x0, .BracedInitializerASTNodePrint_label_50

	// *** Basic block 3

	mv          a2, s2
	mv          a1, s4
	mv          a0, s6
	call        ASTNodePrint

	// *** Basic block 4

.BracedInitializerASTNodePrint_label_50:

	// *** Basic block 5

.BracedInitializerASTNodePrint_label_51:
	addi        s3, s3, 1
	bge         s3, s5, .BracedInitializerASTNodePrint_label_34

	// *** Basic block 6

.BracedInitializerASTNodePrint_label_55:
	mv          a1, s2
	mv          a0, s1
	call        Indent

	// *** Basic block 7

	lla         a1, .str.209
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
	j           fprintf
.func_end_BracedInitializerASTNodePrint:
	.size BracedInitializerASTNodePrint, .func_end_BracedInitializerASTNodePrint-BracedInitializerASTNodePrint

	.local  BracedInitializerASTNodeReplaceChild
	.type BracedInitializerASTNodeReplaceChild, @function

BracedInitializerASTNodeReplaceChild:

	// *** Basic block 0

	.local SetParent
	.global ASTNodeDelete
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
	mv          t0, a0
	mv          t1, a1
	mv          t2, a2
	mv          s1, a3
	mv          t3, t0
	ld          t4, 56(t3)
	ld          t4, 0(t4)
	slli        t5, t1, 3
	add         t4, t4, t5
	ld          s2, 0(t4)
	ld          t4, 56(t3)
	ld          t4, 0(t4)
	add         t4, t4, t5
	sd          t2, 0(t4)
	mv          a2, t1
	mv          a1, t0
	mv          a0, t2
	call        SetParent

	// *** Basic block 1

	beqz        s1, .BracedInitializerASTNodeReplaceChild_label_47

	// *** Basic block 2

	mv          a0, s2
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodeDelete

	// *** Basic block 3

.BracedInitializerASTNodeReplaceChild_label_47:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_BracedInitializerASTNodeReplaceChild:
	.size BracedInitializerASTNodeReplaceChild, .func_end_BracedInitializerASTNodeReplaceChild-BracedInitializerASTNodeReplaceChild

	.local  BracedInitializerASTNodeClone
	.type BracedInitializerASTNodeClone, @function

BracedInitializerASTNodeClone:

	// *** Basic block 0

	.global malloc
	.local ASTNodeBaseCopy
	.global NewVector
	.global ASTNodeClone
	.global VectorAppend
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
	mv          s3, a2
	mv          s4, s1
	li          a0, 64		// 0x40 ASCII '@'
	call        malloc

	// *** Basic block 1

	mv          s5, a0
	mv          a1, s1
	mv          a0, s5
	call        ASTNodeBaseCopy

	// *** Basic block 2

	call        NewVector

	// *** Basic block 3

	sd          a0, 56(s5)
	mv          s6, x0
	ld          t0, 56(s4)
	ld          s7, 8(t0)
	bge         x0, s7, .BracedInitializerASTNodeClone_label_73

	// *** Basic block 4

	ld          s1, 0(t0)

	// *** Basic block 5

.BracedInitializerASTNodeClone_label_47:
	slli        t0, s6, 3
	add         t0, s1, t0
	ld          a0, 0(t0)
	mv          a3, s5
	mv          a2, s3
	mv          a1, s2
	call        ASTNodeClone

	// *** Basic block 6

	mv          s1, a0
	ld          a0, 56(s5)
	mv          a1, s1
	call        VectorAppend

	// *** Basic block 7

.BracedInitializerASTNodeClone_label_69:
	addi        s6, s6, 1
	bge         s6, s7, .BracedInitializerASTNodeClone_label_47

	// *** Basic block 8

.BracedInitializerASTNodeClone_label_73:
	mv          a1, s3
	mv          a0, s5
	mv          t0, s2
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
	jr          t0
.func_end_BracedInitializerASTNodeClone:
	.size BracedInitializerASTNodeClone, .func_end_BracedInitializerASTNodeClone-BracedInitializerASTNodeClone

	.local  BracedInitializerASTNodeVisit
	.type BracedInitializerASTNodeVisit, @function

BracedInitializerASTNodeVisit:

	// *** Basic block 0

	.global ASTNodeVisit
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
	mv          s3, a3
	mv          s4, a2
	mv          s5, s1
	mv          a3, x0
	mv          a1, s3
	jalr         x1, s2, 0

	// *** Basic block 1

	mv          s6, x0
	ld          t0, 56(s5)
	ld          s7, 8(t0)
	bge         x0, s7, .BracedInitializerASTNodeVisit_label_60

	// *** Basic block 2

	ld          s5, 0(t0)

	// *** Basic block 3

.BracedInitializerASTNodeVisit_label_43:
	slli        t0, s6, 3
	add         t0, s5, t0
	ld          a0, 0(t0)
	sext.w      a2, s6
	mv          a3, s3
	mv          a1, s2
	call        ASTNodeVisit

	// *** Basic block 4

.BracedInitializerASTNodeVisit_label_56:
	addi        s6, s6, 1
	bge         s6, s7, .BracedInitializerASTNodeVisit_label_43

	// *** Basic block 5

.BracedInitializerASTNodeVisit_label_60:
	li          t0, 1		// 0x1 ASCII \x1
	mv          a3, t0
	mv          a2, s4
	mv          a1, s3
	mv          a0, s1
	mv          t0, s2
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
	jr          t0
.func_end_BracedInitializerASTNodeVisit:
	.size BracedInitializerASTNodeVisit, .func_end_BracedInitializerASTNodeVisit-BracedInitializerASTNodeVisit

	.global NewBracedInitializerASTNode
	.type NewBracedInitializerASTNode, @function

NewBracedInitializerASTNode:

	// *** Basic block 0

	.global malloc
	.global ASTNodeInit
	.local braced_init_vtbl
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
	li          a0, 64		// 0x40 ASCII '@'
	call        malloc

	// *** Basic block 1

	mv          s3, a0
	la          a4, braced_init_vtbl
	mv          a3, s1
	mv          a2, x0
	li          t0, 88		// 0x58 ASCII 'X'
	mv          a1, t0
	mv          a0, s3
	call        ASTNodeInit

	// *** Basic block 2

	sd          s2, 56(s3)
	mv          s4, x0
	ld          t0, 56(s3)
	ld          t1, 8(t0)
	bge         x0, t1, .NewBracedInitializerASTNode_label_66

	// *** Basic block 3

	ld          t0, 0(t0)

	// *** Basic block 4

.NewBracedInitializerASTNode_label_51:
	slli        t2, s4, 3
	add         t0, t0, t2
	ld          s1, 0(t0)
	sd          s3, 24(s1)
	sext.w      t0, s4
	sw          t0, 32(s1)

	// *** Basic block 5

.NewBracedInitializerASTNode_label_62:
	addi        s4, s4, 1
	bge         s4, t1, .NewBracedInitializerASTNode_label_51

	// *** Basic block 6

.NewBracedInitializerASTNode_label_66:
	mv          a0, s3

	// *** Basic block 7

.NewBracedInitializerASTNode_label_69:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewBracedInitializerASTNode:
	.size NewBracedInitializerASTNode, .func_end_NewBracedInitializerASTNode-NewBracedInitializerASTNode

	.global NewArrayDesignator
	.type NewArrayDesignator, @function

NewArrayDesignator:

	// *** Basic block 0

	.global malloc
	.global TypeRecordIncRef
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
	li          a0, 24		// 0x18 ASCII \x18
	call        malloc

	// *** Basic block 1

	mv          s3, a0
	sw          x0, 0(s3)
	sw          s1, 16(s3)
	sd          s2, 8(s3)
	mv          a0, s2
	call        TypeRecordIncRef

	// *** Basic block 2

	mv          a0, s3

	// *** Basic block 3

.NewArrayDesignator_label_30:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewArrayDesignator:
	.size NewArrayDesignator, .func_end_NewArrayDesignator-NewArrayDesignator

	.global NewStructDesignator
	.type NewStructDesignator, @function

NewStructDesignator:

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
	li          t0, 1		// 0x1 ASCII \x1
	sw          t0, 0(s2)
	sd          s1, 16(s2)
	mv          a0, s2

	// *** Basic block 2

.NewStructDesignator_label_22:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewStructDesignator:
	.size NewStructDesignator, .func_end_NewStructDesignator-NewStructDesignator

	.global NewStructMemberDesignator
	.type NewStructMemberDesignator, @function

NewStructMemberDesignator:

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
	li          t0, 1		// 0x1 ASCII \x1
	sw          t0, 0(s2)
	sd          s1, 16(s2)
	mv          a0, s2

	// *** Basic block 2

.NewStructMemberDesignator_label_22:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewStructMemberDesignator:
	.size NewStructMemberDesignator, .func_end_NewStructMemberDesignator-NewStructMemberDesignator

	.local  DesignatedInitializerASTNodeDelete
	.type DesignatedInitializerASTNodeDelete, @function

DesignatedInitializerASTNodeDelete:

	// *** Basic block 0

	.global VectorDelete
	.global ASTNodeDelete
	.local ASTNodeBaseDelete
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
	mv          s2, s1
	ld          a0, 56(s2)
	call        VectorDelete

	// *** Basic block 1

	ld          a0, 64(s2)
	call        ASTNodeDelete

	// *** Basic block 2

	mv          a0, s1
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodeBaseDelete
.func_end_DesignatedInitializerASTNodeDelete:
	.size DesignatedInitializerASTNodeDelete, .func_end_DesignatedInitializerASTNodeDelete-DesignatedInitializerASTNodeDelete

	.local  DesignatedInitializerASTNodePrint
	.type DesignatedInitializerASTNodePrint, @function

DesignatedInitializerASTNodePrint:

	// *** Basic block 0

	.local Indent
	.global fprintf
	.global ASTNodePrint
	.local ASTNodeBasePrint
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
	mv          s4, s1
	addi        s5, s2, 2
	mv          a1, s3
	mv          a0, s5
	call        Indent

	// *** Basic block 1

	ld          s6, 56(s4)
	beq         s6, x0, .DesignatedInitializerASTNodePrint_label_110

	// *** Basic block 2

	ld          s7, 0(s6)
	mv          s8, x0
	ld          s6, 8(s6)
	bge         x0, s6, .DesignatedInitializerASTNodePrint_label_103

	// *** Basic block 3

.DesignatedInitializerASTNodePrint_label_49:
	slli        t0, s8, 3
	add         t0, s7, t0
	ld          s7, 0(t0)
	lw          t0, 0(s7)
	bnez        t0, .DesignatedInitializerASTNodePrint_label_68

	// *** Basic block 4

	lla         a1, .str.210
	lw          a2, 16(s7)
	mv          a0, s3
	call        fprintf

	// *** Basic block 5

	j           .DesignatedInitializerASTNodePrint_label_98

	// *** Basic block 6

.DesignatedInitializerASTNodePrint_label_68:
	ld          s7, 16(s7)
	beq         s7, x0, .DesignatedInitializerASTNodePrint_label_85

	// *** Basic block 7

	lla         a1, .str.211
	ld          t0, 0(s7)
	ld          a2, 16(t0)
	mv          a0, s3
	call        fprintf

	// *** Basic block 8

	j           .DesignatedInitializerASTNodePrint_label_97

	// *** Basic block 9

.DesignatedInitializerASTNodePrint_label_85:
	beq         s7, x0, .DesignatedInitializerASTNodePrint_label_96

	// *** Basic block 10

	lla         a1, .str.212
	ld          a2, 16(s7)
	mv          a0, s3
	call        fprintf

	// *** Basic block 11

.DesignatedInitializerASTNodePrint_label_96:

	// *** Basic block 12

.DesignatedInitializerASTNodePrint_label_97:

	// *** Basic block 13

.DesignatedInitializerASTNodePrint_label_98:

	// *** Basic block 14

.DesignatedInitializerASTNodePrint_label_99:
	addi        s8, s8, 1
	bge         s8, s6, .DesignatedInitializerASTNodePrint_label_49

	// *** Basic block 15

.DesignatedInitializerASTNodePrint_label_103:
	lla         a1, .str.213
	mv          a0, s3
	call        fprintf

	// *** Basic block 16

.DesignatedInitializerASTNodePrint_label_110:
	mv          a1, s3
	mv          a0, s2
	call        Indent

	// *** Basic block 17

	lla         a1, .str.214
	mv          a0, s3
	call        fprintf

	// *** Basic block 18

	ld          a0, 64(s4)
	mv          a2, s3
	mv          a1, s5
	call        ASTNodePrint

	// *** Basic block 19

	mv          a2, s3
	mv          a1, s2
	mv          a0, s1
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
	j           ASTNodeBasePrint
.func_end_DesignatedInitializerASTNodePrint:
	.size DesignatedInitializerASTNodePrint, .func_end_DesignatedInitializerASTNodePrint-DesignatedInitializerASTNodePrint

	.local  DesignatedInitializerASTNodeReplaceChild
	.type DesignatedInitializerASTNodeReplaceChild, @function

DesignatedInitializerASTNodeReplaceChild:

	// *** Basic block 0

	.local SetParent
	.global ASTNodeDelete
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
	mv          t0, a0
	mv          t1, a2
	mv          t2, a1
	mv          s1, a3
	mv          t3, t0
	ld          s2, 64(t3)
	sd          t1, 64(t3)
	mv          a2, t2
	mv          a1, t0
	mv          a0, t1
	call        SetParent

	// *** Basic block 1

	beqz        s1, .DesignatedInitializerASTNodeReplaceChild_label_37

	// *** Basic block 2

	mv          a0, s2
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	j           ASTNodeDelete

	// *** Basic block 3

.DesignatedInitializerASTNodeReplaceChild_label_37:
	// Restored registers.
	ld s1, 8(sp)
	ld s2, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_DesignatedInitializerASTNodeReplaceChild:
	.size DesignatedInitializerASTNodeReplaceChild, .func_end_DesignatedInitializerASTNodeReplaceChild-DesignatedInitializerASTNodeReplaceChild

	.local  DesignatedInitializerASTNodeClone
	.type DesignatedInitializerASTNodeClone, @function

DesignatedInitializerASTNodeClone:

	// *** Basic block 0

	.global malloc
	.local ASTNodeBaseCopy
	.global NewVector
	.global TypeRecordIncRef
	.global memcpy
	.global VectorAppend
	.global ASTNodeClone
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
	mv          s4, s1
	li          a0, 72		// 0x48 ASCII 'H'
	call        malloc

	// *** Basic block 1

	mv          s5, a0
	mv          a1, s1
	mv          a0, s5
	call        ASTNodeBaseCopy

	// *** Basic block 2

	call        NewVector

	// *** Basic block 3

	ld          s6, 56(s5)
	sd          a0, 56(s5)
	mv          s7, x0
	ld          s8, 56(s4)
	ld          s9, 8(s8)
	bge         x0, s9, .DesignatedInitializerASTNodeClone_label_92

	// *** Basic block 4

	ld          s1, 0(s8)

	// *** Basic block 5

.DesignatedInitializerASTNodeClone_label_54:
	slli        t0, s7, 3
	add         t0, s1, t0
	ld          s1, 0(t0)
	li          t0, 24		// 0x18 ASCII \x18
	mv          a0, t0
	call        malloc

	// *** Basic block 6

	mv          s8, a0
	lw          t0, 0(s1)
	sw          t0, 0(s8)
	ld          t0, 8(s1)
	sd          t0, 8(s8)
	ld          a0, 8(s8)
	call        TypeRecordIncRef

	// *** Basic block 7

	addi        a0, s8, 16
	addi        a1, s1, 16
	li          t0, 8		// 0x8 ASCII \x8
	mv          a2, t0
	call        memcpy

	// *** Basic block 8

	mv          a1, s8
	mv          a0, s6
	call        VectorAppend

	// *** Basic block 9

.DesignatedInitializerASTNodeClone_label_88:
	addi        s7, s7, 1
	bge         s7, s9, .DesignatedInitializerASTNodeClone_label_54

	// *** Basic block 10

.DesignatedInitializerASTNodeClone_label_92:
	ld          a0, 64(s4)
	mv          a3, s5
	mv          a2, s3
	mv          a1, s2
	call        ASTNodeClone

	// *** Basic block 11

	sd          a0, 64(s5)
	mv          a1, s3
	mv          a0, s5
	mv          t0, s2
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
	jr          t0
.func_end_DesignatedInitializerASTNodeClone:
	.size DesignatedInitializerASTNodeClone, .func_end_DesignatedInitializerASTNodeClone-DesignatedInitializerASTNodeClone

	.local  DesignatedInitializerASTNodeVisit
	.type DesignatedInitializerASTNodeVisit, @function

DesignatedInitializerASTNodeVisit:

	// *** Basic block 0

	.global ASTNodeVisit
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
	mv          s3, a3
	mv          s4, a2
	mv          s5, s1
	mv          a3, x0
	mv          a1, s3
	jalr         x1, s2, 0

	// *** Basic block 1

	ld          a0, 64(s5)
	mv          a3, s3
	mv          a2, x0
	mv          a1, s2
	call        ASTNodeVisit

	// *** Basic block 2

	li          t0, 1		// 0x1 ASCII \x1
	mv          a3, t0
	mv          a2, s4
	mv          a1, s3
	mv          a0, s1
	mv          t0, s2
	// Restored registers.
	ld s1, 40(sp)
	ld s2, 32(sp)
	ld s3, 24(sp)
	ld s4, 16(sp)
	ld s5, 8(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	jr          t0
.func_end_DesignatedInitializerASTNodeVisit:
	.size DesignatedInitializerASTNodeVisit, .func_end_DesignatedInitializerASTNodeVisit-DesignatedInitializerASTNodeVisit

	.global NewDesignatedInitializerASTNode
	.type NewDesignatedInitializerASTNode, @function

NewDesignatedInitializerASTNode:

	// *** Basic block 0

	.global malloc
	.global ASTNodeInit
	.local designated_init_vtbl
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
	li          a0, 72		// 0x48 ASCII 'H'
	call        malloc

	// *** Basic block 1

	mv          s4, a0
	la          a4, designated_init_vtbl
	mv          a3, s1
	mv          a2, x0
	li          t0, 89		// 0x59 ASCII 'Y'
	mv          a1, t0
	mv          a0, s4
	call        ASTNodeInit

	// *** Basic block 2

	sd          s2, 56(s4)
	sd          s3, 64(s4)
	sd          s4, 24(s3)
	sw          x0, 32(s3)
	mv          a0, s4

	// *** Basic block 3

.NewDesignatedInitializerASTNode_label_51:
	// Restored registers.
	ld s1, 24(sp)
	ld s2, 16(sp)
	ld s3, 8(sp)
	ld s4, 0(sp)
	addi sp, s0, 0
	ld ra, -8(s0)
	ld s0, -16(s0)
	ret         
.func_end_NewDesignatedInitializerASTNode:
	.size NewDesignatedInitializerASTNode, .func_end_NewDesignatedInitializerASTNode-NewDesignatedInitializerASTNode

	.global IsBitfieldReference
	.type IsBitfieldReference, @function

IsBitfieldReference:

	// *** Basic block 0

	.global StructMemberIsBitField
	// Leaf procedure, no stack frame generated
	mv          t0, a0
	lw          t2, 0(t0)
	addi        t3, t2, -33
	snez        t1, t3
	li          t3, 33		// 0x21 ASCII '!'
	beq         t2, t3, .IsBitfieldReference_label_22

	// *** Basic block 1

	addi        t2, t2, -19
	snez        t1, t2

	// *** Basic block 2

.IsBitfieldReference_label_22:
	beqz        t1, .IsBitfieldReference_label_29

	// *** Basic block 3

	mv          a0, x0

	// *** Basic block 4

.IsBitfieldReference_label_26:
	ret         

	// *** Basic block 5

.IsBitfieldReference_label_29:
	mv          t1, t0
	ld          t2, 64(t1)
	ld          a0, 56(t2)
	j           StructMemberIsBitField
.func_end_IsBitfieldReference:
	.size IsBitfieldReference, .func_end_IsBitfieldReference-IsBitfieldReference

.PCend:
	.data
next_ast_node_id:
	.type   next_ast_node_id,@object
	.local  next_ast_node_id
	.size   next_ast_node_id,4
	.p2align  2
	.word   1

base_vtbl:
	.type   base_vtbl,@object
	.local  base_vtbl
	.size   base_vtbl,48
	.p2align  3
	.global ASTNodeBaseDelete
	.long    ASTNodeBaseDelete
	.global ASTNodeBasePrint
	.long    ASTNodeBasePrint
	.word   0
	.space  4
	.word   0
	.space  4
	.word   0
	.space  4
	.word   0
	.space  4

identifier_vtbl:
	.type   identifier_vtbl,@object
	.local  identifier_vtbl
	.size   identifier_vtbl,48
	.p2align  3
	.global ASTNodeBaseDelete
	.long    ASTNodeBaseDelete
	.global IdentifierASTNodePrint
	.long    IdentifierASTNodePrint
	.word   0
	.space  4
	.global IdentifierASTNodeClone
	.long    IdentifierASTNodeClone
	.word   0
	.space  4
	.word   0
	.space  4

struct_member_vtbl:
	.type   struct_member_vtbl,@object
	.local  struct_member_vtbl
	.size   struct_member_vtbl,48
	.p2align  3
	.global ASTNodeBaseDelete
	.long    ASTNodeBaseDelete
	.global StructMemberASTNodePrint
	.long    StructMemberASTNodePrint
	.word   0
	.space  4
	.global StructMemberASTNodeClone
	.long    StructMemberASTNodeClone
	.word   0
	.space  4
	.word   0
	.space  4

constant_vtbl:
	.type   constant_vtbl,@object
	.local  constant_vtbl
	.size   constant_vtbl,48
	.p2align  3
	.global ASTNodeBaseDelete
	.long    ASTNodeBaseDelete
	.global ConstantASTNodePrint
	.long    ConstantASTNodePrint
	.word   0
	.space  4
	.global ConstantASTNodeClone
	.long    ConstantASTNodeClone
	.word   0
	.space  4
	.word   0
	.space  4

unary_vtbl:
	.type   unary_vtbl,@object
	.local  unary_vtbl
	.size   unary_vtbl,48
	.p2align  3
	.global UnaryASTNodeDelete
	.long    UnaryASTNodeDelete
	.global UnaryASTNodePrint
	.long    UnaryASTNodePrint
	.global UnaryASTNodeReplaceChild
	.long    UnaryASTNodeReplaceChild
	.global UnaryASTNodeClone
	.long    UnaryASTNodeClone
	.global UnaryASTNodeVisit
	.long    UnaryASTNodeVisit
	.global ValueAlwaysUsed
	.long    ValueAlwaysUsed

binary_vtbl:
	.type   binary_vtbl,@object
	.local  binary_vtbl
	.size   binary_vtbl,48
	.p2align  3
	.global BinaryASTNodeDelete
	.long    BinaryASTNodeDelete
	.global BinaryASTNodePrint
	.long    BinaryASTNodePrint
	.global BinaryASTNodeReplaceChild
	.long    BinaryASTNodeReplaceChild
	.global BinaryASTNodeClone
	.long    BinaryASTNodeClone
	.global BinaryASTNodeVisit
	.long    BinaryASTNodeVisit
	.global ValueAlwaysUsed
	.long    ValueAlwaysUsed

inline_call_vtbl:
	.type   inline_call_vtbl,@object
	.local  inline_call_vtbl
	.size   inline_call_vtbl,48
	.p2align  3
	.global InlineCallASTNodeDelete
	.long    InlineCallASTNodeDelete
	.global InlineCallASTNodePrint
	.long    InlineCallASTNodePrint
	.global InlineCallASTNodeReplaceChild
	.long    InlineCallASTNodeReplaceChild
	.global InlineCallASTNodeClone
	.long    InlineCallASTNodeClone
	.global InlineCallASTNodeVisit
	.long    InlineCallASTNodeVisit
	.global ValueAlwaysUsed
	.long    ValueAlwaysUsed

vector_vtbl:
	.type   vector_vtbl,@object
	.local  vector_vtbl
	.size   vector_vtbl,48
	.p2align  3
	.global VectorASTNodeDelete
	.long    VectorASTNodeDelete
	.global VectorASTNodePrint
	.long    VectorASTNodePrint
	.global VectorASTNodeReplaceChild
	.long    VectorASTNodeReplaceChild
	.global VectorASTNodeClone
	.long    VectorASTNodeClone
	.global VectorASTNodeVisit
	.long    VectorASTNodeVisit
	.global ValueAlwaysUsed
	.long    ValueAlwaysUsed

cast_vtbl:
	.type   cast_vtbl,@object
	.local  cast_vtbl
	.size   cast_vtbl,48
	.p2align  3
	.global CastASTNodeDelete
	.long    CastASTNodeDelete
	.global CastASTNodePrint
	.long    CastASTNodePrint
	.global CastASTNodeReplaceChild
	.long    CastASTNodeReplaceChild
	.global CastASTNodeClone
	.long    CastASTNodeClone
	.word   0
	.space  4
	.global CastASTNodeUsesValue
	.long    CastASTNodeUsesValue

ptr_scale_vtbl:
	.type   ptr_scale_vtbl,@object
	.local  ptr_scale_vtbl
	.size   ptr_scale_vtbl,48
	.p2align  3
	.global PtrScaleASTNodeDelete
	.long    PtrScaleASTNodeDelete
	.global PtrScaleASTNodePrint
	.long    PtrScaleASTNodePrint
	.global PtrScaleASTNodeReplaceChild
	.long    PtrScaleASTNodeReplaceChild
	.global PtrScaleASTNodeClone
	.long    PtrScaleASTNodeClone
	.word   0
	.space  4
	.global PtrScaleASTNodeUsesValue
	.long    PtrScaleASTNodeUsesValue

sizeof_vtbl:
	.type   sizeof_vtbl,@object
	.local  sizeof_vtbl
	.size   sizeof_vtbl,48
	.p2align  3
	.global SizeofASTNodeDelete
	.long    SizeofASTNodeDelete
	.global SizeofASTNodePrint
	.long    SizeofASTNodePrint
	.global SizeofASTNodeReplaceChild
	.long    SizeofASTNodeReplaceChild
	.global SizeofASTNodeClone
	.long    SizeofASTNodeClone
	.word   0
	.space  4
	.global ValueAlwaysUsed
	.long    ValueAlwaysUsed

macro_vtbl:
	.type   macro_vtbl,@object
	.local  macro_vtbl
	.size   macro_vtbl,48
	.p2align  3
	.global MacroNameASTNodeDelete
	.long    MacroNameASTNodeDelete
	.global MacroNameASTNodePrint
	.long    MacroNameASTNodePrint
	.word   0
	.space  4
	.global MacroNameASTNodeClone
	.long    MacroNameASTNodeClone
	.word   0
	.space  4
	.word   0
	.space  4

goto_vtbl:
	.type   goto_vtbl,@object
	.local  goto_vtbl
	.size   goto_vtbl,48
	.p2align  3
	.global GotoStatementASTNodeDelete
	.long    GotoStatementASTNodeDelete
	.global GotoStatementASTNodePrint
	.long    GotoStatementASTNodePrint
	.word   0
	.space  4
	.global GotoStatementASTNodeClone
	.long    GotoStatementASTNodeClone
	.word   0
	.space  4
	.word   0
	.space  4

expr_stmt_vtbl:
	.type   expr_stmt_vtbl,@object
	.local  expr_stmt_vtbl
	.size   expr_stmt_vtbl,48
	.p2align  3
	.global ExpressionStatementASTNodeDelete
	.long    ExpressionStatementASTNodeDelete
	.global ExpressionStatementASTNodePrint
	.long    ExpressionStatementASTNodePrint
	.global ExpressionStatementASTNodeReplaceChild
	.long    ExpressionStatementASTNodeReplaceChild
	.global ExpressionStatementASTNodeClone
	.long    ExpressionStatementASTNodeClone
	.global ExpressionStatementASTNodeVisit
	.long    ExpressionStatementASTNodeVisit
	.global ValueNotUsed
	.long    ValueNotUsed

if_stmt_vtbl:
	.type   if_stmt_vtbl,@object
	.local  if_stmt_vtbl
	.size   if_stmt_vtbl,48
	.p2align  3
	.global IfStatementASTNodeDelete
	.long    IfStatementASTNodeDelete
	.global IfStatementASTNodePrint
	.long    IfStatementASTNodePrint
	.global IfStatementASTNodeReplaceChild
	.long    IfStatementASTNodeReplaceChild
	.global IfStatementASTNodeClone
	.long    IfStatementASTNodeClone
	.global IfStatementASTNodeVisit
	.long    IfStatementASTNodeVisit
	.global IfStatementUsesValue
	.long    IfStatementUsesValue

combined_stmt_vtbl:
	.type   combined_stmt_vtbl,@object
	.local  combined_stmt_vtbl
	.size   combined_stmt_vtbl,48
	.p2align  3
	.global CombinedStatementASTNodeDelete
	.long    CombinedStatementASTNodeDelete
	.global CombinedStatementASTNodePrint
	.long    CombinedStatementASTNodePrint
	.global CombinedStatementASTNodeReplaceChild
	.long    CombinedStatementASTNodeReplaceChild
	.global CombinedStatementASTNodeClone
	.long    CombinedStatementASTNodeClone
	.global CombinedStatementASTNodeVisit
	.long    CombinedStatementASTNodeVisit
	.global CombinedStatementASTNodeUsesValue
	.long    CombinedStatementASTNodeUsesValue

compound_stmt_vtbl:
	.type   compound_stmt_vtbl,@object
	.local  compound_stmt_vtbl
	.size   compound_stmt_vtbl,48
	.p2align  3
	.global CompoundStatementASTNodeDelete
	.long    CompoundStatementASTNodeDelete
	.global CompoundStatementASTNodePrint
	.long    CompoundStatementASTNodePrint
	.global CompoundStatementASTNodeReplaceChild
	.long    CompoundStatementASTNodeReplaceChild
	.global CompoundStatementASTNodeClone
	.long    CompoundStatementASTNodeClone
	.global CompoundStatementASTNodeVisit
	.long    CompoundStatementASTNodeVisit
	.global ValueNotUsed
	.long    ValueNotUsed

for_stmt_vtbl:
	.type   for_stmt_vtbl,@object
	.local  for_stmt_vtbl
	.size   for_stmt_vtbl,48
	.p2align  3
	.global ForStatementASTNodeDelete
	.long    ForStatementASTNodeDelete
	.global ForStatementASTNodePrint
	.long    ForStatementASTNodePrint
	.global ForStatementASTNodeReplaceChild
	.long    ForStatementASTNodeReplaceChild
	.global ForStatementASTNodeClone
	.long    ForStatementASTNodeClone
	.global ForStatementASTNodeVisit
	.long    ForStatementASTNodeVisit
	.global ForStatementASTNodeUsesValue
	.long    ForStatementASTNodeUsesValue

var_decl_vtbl:
	.type   var_decl_vtbl,@object
	.local  var_decl_vtbl
	.size   var_decl_vtbl,48
	.p2align  3
	.global VariableDeclarationASTNodeDelete
	.long    VariableDeclarationASTNodeDelete
	.global VariableDeclarationASTNodePrint
	.long    VariableDeclarationASTNodePrint
	.global VariableDeclarationASTNodeReplaceChild
	.long    VariableDeclarationASTNodeReplaceChild
	.global VariableDeclarationASTNodeClone
	.long    VariableDeclarationASTNodeClone
	.global VariableDeclarationASTNodeVisit
	.long    VariableDeclarationASTNodeVisit
	.global ValueAlwaysUsed
	.long    ValueAlwaysUsed

decl_list_vtbl:
	.type   decl_list_vtbl,@object
	.local  decl_list_vtbl
	.size   decl_list_vtbl,48
	.p2align  3
	.global DeclarationListASTNodeDelete
	.long    DeclarationListASTNodeDelete
	.global DeclarationListASTNodePrint
	.long    DeclarationListASTNodePrint
	.word   0
	.space  4
	.global DeclarationListASTNodeClone
	.long    DeclarationListASTNodeClone
	.global DeclarationListASTNodeVisit
	.long    DeclarationListASTNodeVisit
	.global ValueNotUsed
	.long    ValueNotUsed

case_label_vtbl:
	.type   case_label_vtbl,@object
	.local  case_label_vtbl
	.size   case_label_vtbl,48
	.p2align  3
	.global CaseLabelASTNodeDelete
	.long    CaseLabelASTNodeDelete
	.global CaseLabelASTNodePrint
	.long    CaseLabelASTNodePrint
	.global CaseLabelASTNodeReplaceChild
	.long    CaseLabelASTNodeReplaceChild
	.global CaseLabelASTNodeClone
	.long    CaseLabelASTNodeClone
	.global CaseLabelASTNodeVisit
	.long    CaseLabelASTNodeVisit
	.global ValueAlwaysUsed
	.long    ValueAlwaysUsed

switch_stmt_vtbl:
	.type   switch_stmt_vtbl,@object
	.local  switch_stmt_vtbl
	.size   switch_stmt_vtbl,48
	.p2align  3
	.global SwitchStatementASTNodeDelete
	.long    SwitchStatementASTNodeDelete
	.global SwitchStatementASTNodePrint
	.long    SwitchStatementASTNodePrint
	.global SwitchStatementASTNodeReplaceChild
	.long    SwitchStatementASTNodeReplaceChild
	.global SwitchStatementASTNodeClone
	.long    SwitchStatementASTNodeClone
	.global SwitchStatementASTNodeVisit
	.long    SwitchStatementASTNodeVisit
	.global SwitchStatementASTNodeUsesValue
	.long    SwitchStatementASTNodeUsesValue

label_vtbl:
	.type   label_vtbl,@object
	.local  label_vtbl
	.size   label_vtbl,48
	.p2align  3
	.global LabelASTNodeDelete
	.long    LabelASTNodeDelete
	.global LabelASTNodePrint
	.long    LabelASTNodePrint
	.global LabelASTNodeReplaceChild
	.long    LabelASTNodeReplaceChild
	.global LabelASTNodeClone
	.long    LabelASTNodeClone
	.global LabelASTNodeVisit
	.long    LabelASTNodeVisit
	.word   0
	.space  4

asm_vtbl:
	.type   asm_vtbl,@object
	.local  asm_vtbl
	.size   asm_vtbl,48
	.p2align  3
	.global AsmASTNodeDelete
	.long    AsmASTNodeDelete
	.global AsmASTNodePrint
	.long    AsmASTNodePrint
	.word   0
	.space  4
	.global AsmASTNodeClone
	.long    AsmASTNodeClone
	.word   0
	.space  4
	.word   0
	.space  4

expr_init_vtbl:
	.type   expr_init_vtbl,@object
	.local  expr_init_vtbl
	.size   expr_init_vtbl,48
	.p2align  3
	.global ExpressionInitializerASTNodeDelete
	.long    ExpressionInitializerASTNodeDelete
	.global ExpressionInitializerASTNodePrint
	.long    ExpressionInitializerASTNodePrint
	.global ExpressionInitializerASTNodeReplaceChild
	.long    ExpressionInitializerASTNodeReplaceChild
	.global ExpressionInitializerASTNodeClone
	.long    ExpressionInitializerASTNodeClone
	.global ExpressionInitializerASTNodeVisit
	.long    ExpressionInitializerASTNodeVisit
	.global ValueAlwaysUsed
	.long    ValueAlwaysUsed

braced_init_vtbl:
	.type   braced_init_vtbl,@object
	.local  braced_init_vtbl
	.size   braced_init_vtbl,48
	.p2align  3
	.global BracedInitializerASTNodeDelete
	.long    BracedInitializerASTNodeDelete
	.global BracedInitializerASTNodePrint
	.long    BracedInitializerASTNodePrint
	.global BracedInitializerASTNodeReplaceChild
	.long    BracedInitializerASTNodeReplaceChild
	.global BracedInitializerASTNodeClone
	.long    BracedInitializerASTNodeClone
	.global BracedInitializerASTNodeVisit
	.long    BracedInitializerASTNodeVisit
	.word   0
	.space  4

designated_init_vtbl:
	.type   designated_init_vtbl,@object
	.local  designated_init_vtbl
	.size   designated_init_vtbl,48
	.p2align  3
	.global DesignatedInitializerASTNodeDelete
	.long    DesignatedInitializerASTNodeDelete
	.global DesignatedInitializerASTNodePrint
	.long    DesignatedInitializerASTNodePrint
	.global DesignatedInitializerASTNodeReplaceChild
	.long    DesignatedInitializerASTNodeReplaceChild
	.global DesignatedInitializerASTNodeClone
	.long    DesignatedInitializerASTNodeClone
	.global DesignatedInitializerASTNodeVisit
	.long    DesignatedInitializerASTNodeVisit
	.global ValueAlwaysUsed
	.long    ValueAlwaysUsed

	.section ".rodata", "aMS", @progbits
.str.1:
	.asciz "bad"
	.type .str.1, @object
	.size .str.1, 4

.str.2:
	.asciz "number"
	.type .str.2, @object
	.size .str.2, 7

.str.3:
	.asciz "identifier"
	.type .str.3, @object
	.size .str.3, 11

.str.4:
	.asciz "string"
	.type .str.4, @object
	.size .str.4, 7

.str.5:
	.asciz "wide string"
	.type .str.5, @object
	.size .str.5, 12

.str.6:
	.asciz "char const"
	.type .str.6, @object
	.size .str.6, 11

.str.7:
	.asciz "fnumber"
	.type .str.7, @object
	.size .str.7, 8

.str.8:
	.asciz "++"
	.type .str.8, @object
	.size .str.8, 3

.str.9:
	.asciz "--"
	.type .str.9, @object
	.size .str.9, 3

.str.10:
	.asciz "-"
	.type .str.10, @object
	.size .str.10, 2

.str.11:
	.asciz "+"
	.type .str.11, @object
	.size .str.11, 2

.str.12:
	.asciz "*"
	.type .str.12, @object
	.size .str.12, 2

.str.13:
	.asciz "&"
	.type .str.13, @object
	.size .str.13, 2

.str.14:
	.asciz "&"
	.type .str.14, @object
	.size .str.14, 2

.str.15:
	.asciz "&="
	.type .str.15, @object
	.size .str.15, 3

.str.16:
	.asciz "->"
	.type .str.16, @object
	.size .str.16, 3

.str.17:
	.asciz "="
	.type .str.17, @object
	.size .str.17, 2

.str.18:
	.asciz "!"
	.type .str.18, @object
	.size .str.18, 2

.str.19:
	.asciz "|"
	.type .str.19, @object
	.size .str.19, 2

.str.20:
	.asciz "break"
	.type .str.20, @object
	.size .str.20, 6

.str.21:
	.asciz "^"
	.type .str.21, @object
	.size .str.21, 2

.str.22:
	.asciz "^="
	.type .str.22, @object
	.size .str.22, 3

.str.23:
	.asciz "case"
	.type .str.23, @object
	.size .str.23, 5

.str.24:
	.asciz ":"
	.type .str.24, @object
	.size .str.24, 2

.str.25:
	.asciz ","
	.type .str.25, @object
	.size .str.25, 2

.str.26:
	.asciz "_Complex"
	.type .str.26, @object
	.size .str.26, 9

.str.27:
	.asciz "continue"
	.type .str.27, @object
	.size .str.27, 9

.str.28:
	.asciz "do"
	.type .str.28, @object
	.size .str.28, 3

.str.29:
	.asciz "."
	.type .str.29, @object
	.size .str.29, 2

.str.30:
	.asciz "=="
	.type .str.30, @object
	.size .str.30, 3

.str.31:
	.asciz "for"
	.type .str.31, @object
	.size .str.31, 4

.str.32:
	.asciz "goto"
	.type .str.32, @object
	.size .str.32, 5

.str.33:
	.asciz ">"
	.type .str.33, @object
	.size .str.33, 2

.str.34:
	.asciz ">="
	.type .str.34, @object
	.size .str.34, 3

.str.35:
	.asciz "if"
	.type .str.35, @object
	.size .str.35, 3

.str.36:
	.asciz "_Imaginary"
	.type .str.36, @object
	.size .str.36, 11

.str.37:
	.asciz "{"
	.type .str.37, @object
	.size .str.37, 2

.str.38:
	.asciz "<"
	.type .str.38, @object
	.size .str.38, 2

.str.39:
	.asciz "<="
	.type .str.39, @object
	.size .str.39, 3

.str.40:
	.asciz "&&"
	.type .str.40, @object
	.size .str.40, 3

.str.41:
	.asciz "||"
	.type .str.41, @object
	.size .str.41, 3

.str.42:
	.asciz "("
	.type .str.42, @object
	.size .str.42, 2

.str.43:
	.asciz "<<"
	.type .str.43, @object
	.size .str.43, 3

.str.44:
	.asciz "<<="
	.type .str.44, @object
	.size .str.44, 4

.str.45:
	.asciz "["
	.type .str.45, @object
	.size .str.45, 2

.str.46:
	.asciz "-"
	.type .str.46, @object
	.size .str.46, 2

.str.47:
	.asciz "-="
	.type .str.47, @object
	.size .str.47, 3

.str.48:
	.asciz "--"
	.type .str.48, @object
	.size .str.48, 3

.str.49:
	.asciz "!="
	.type .str.49, @object
	.size .str.49, 3

.str.50:
	.asciz "|="
	.type .str.50, @object
	.size .str.50, 3

.str.51:
	.asciz "%"
	.type .str.51, @object
	.size .str.51, 2

.str.52:
	.asciz "%="
	.type .str.52, @object
	.size .str.52, 3

.str.53:
	.asciz "+"
	.type .str.53, @object
	.size .str.53, 2

.str.54:
	.asciz "+="
	.type .str.54, @object
	.size .str.54, 3

.str.55:
	.asciz "++"
	.type .str.55, @object
	.size .str.55, 3

.str.56:
	.asciz "?"
	.type .str.56, @object
	.size .str.56, 2

.str.57:
	.asciz "return"
	.type .str.57, @object
	.size .str.57, 7

.str.58:
	.asciz ">>"
	.type .str.58, @object
	.size .str.58, 3

.str.59:
	.asciz ">>="
	.type .str.59, @object
	.size .str.59, 4

.str.60:
	.asciz "sizeof"
	.type .str.60, @object
	.size .str.60, 7

.str.61:
	.asciz "/"
	.type .str.61, @object
	.size .str.61, 2

.str.62:
	.asciz "/="
	.type .str.62, @object
	.size .str.62, 3

.str.63:
	.asciz "*"
	.type .str.63, @object
	.size .str.63, 2

.str.64:
	.asciz "*="
	.type .str.64, @object
	.size .str.64, 3

.str.65:
	.asciz "switch"
	.type .str.65, @object
	.size .str.65, 7

.str.66:
	.asciz "~"
	.type .str.66, @object
	.size .str.66, 2

.str.67:
	.asciz "while"
	.type .str.67, @object
	.size .str.67, 6

.str.68:
	.asciz "structmember"
	.type .str.68, @object
	.size .str.68, 13

.str.69:
	.asciz "__asm"
	.type .str.69, @object
	.size .str.69, 6

.str.70:
	.asciz "__attribute"
	.type .str.70, @object
	.size .str.70, 12

.str.71:
	.asciz "builtin_va_start"
	.type .str.71, @object
	.size .str.71, 17

.str.72:
	.asciz "builtin_va_arg"
	.type .str.72, @object
	.size .str.72, 15

.str.73:
	.asciz "builtin_va_end"
	.type .str.73, @object
	.size .str.73, 15

.str.74:
	.asciz "builtin_va_copy"
	.type .str.74, @object
	.size .str.74, 16

.str.75:
	.asciz "cast"
	.type .str.75, @object
	.size .str.75, 5

.str.76:
	.asciz "label"
	.type .str.76, @object
	.size .str.76, 6

.str.77:
	.asciz "variable"
	.type .str.77, @object
	.size .str.77, 9

.str.78:
	.asciz "decl_list"
	.type .str.78, @object
	.size .str.78, 10

.str.79:
	.asciz "macro"
	.type .str.79, @object
	.size .str.79, 6

.str.80:
	.asciz "expr"
	.type .str.80, @object
	.size .str.80, 5

.str.81:
	.asciz "init"
	.type .str.81, @object
	.size .str.81, 5

.str.82:
	.asciz "expr-init"
	.type .str.82, @object
	.size .str.82, 10

.str.83:
	.asciz "braced-init"
	.type .str.83, @object
	.size .str.83, 12

.str.84:
	.asciz "designated-init"
	.type .str.84, @object
	.size .str.84, 16

.str.85:
	.asciz "ptr-scale"
	.type .str.85, @object
	.size .str.85, 10

.str.86:
	.asciz "i2s"
	.type .str.86, @object
	.size .str.86, 4

.str.87:
	.asciz "i2c"
	.type .str.87, @object
	.size .str.87, 4

.str.88:
	.asciz "i2l"
	.type .str.88, @object
	.size .str.88, 4

.str.89:
	.asciz "i2ll"
	.type .str.89, @object
	.size .str.89, 5

.str.90:
	.asciz "i2f"
	.type .str.90, @object
	.size .str.90, 4

.str.91:
	.asciz "i2d"
	.type .str.91, @object
	.size .str.91, 4

.str.92:
	.asciz "i2ld"
	.type .str.92, @object
	.size .str.92, 5

.str.93:
	.asciz "i2b"
	.type .str.93, @object
	.size .str.93, 4

.str.94:
	.asciz "c2i"
	.type .str.94, @object
	.size .str.94, 4

.str.95:
	.asciz "c2s"
	.type .str.95, @object
	.size .str.95, 4

.str.96:
	.asciz "c2l"
	.type .str.96, @object
	.size .str.96, 4

.str.97:
	.asciz "c2ll"
	.type .str.97, @object
	.size .str.97, 5

.str.98:
	.asciz "c2f"
	.type .str.98, @object
	.size .str.98, 4

.str.99:
	.asciz "c2d"
	.type .str.99, @object
	.size .str.99, 4

.str.100:
	.asciz "c2ld"
	.type .str.100, @object
	.size .str.100, 5

.str.101:
	.asciz "c2b"
	.type .str.101, @object
	.size .str.101, 4

.str.102:
	.asciz "s2i"
	.type .str.102, @object
	.size .str.102, 4

.str.103:
	.asciz "s2c"
	.type .str.103, @object
	.size .str.103, 4

.str.104:
	.asciz "s2l"
	.type .str.104, @object
	.size .str.104, 4

.str.105:
	.asciz "s2ll"
	.type .str.105, @object
	.size .str.105, 5

.str.106:
	.asciz "s2f"
	.type .str.106, @object
	.size .str.106, 4

.str.107:
	.asciz "s2d"
	.type .str.107, @object
	.size .str.107, 4

.str.108:
	.asciz "s2ld"
	.type .str.108, @object
	.size .str.108, 5

.str.109:
	.asciz "s2b"
	.type .str.109, @object
	.size .str.109, 4

.str.110:
	.asciz "l2i"
	.type .str.110, @object
	.size .str.110, 4

.str.111:
	.asciz "l2c"
	.type .str.111, @object
	.size .str.111, 4

.str.112:
	.asciz "l2s"
	.type .str.112, @object
	.size .str.112, 4

.str.113:
	.asciz "l2ll"
	.type .str.113, @object
	.size .str.113, 5

.str.114:
	.asciz "l2f"
	.type .str.114, @object
	.size .str.114, 4

.str.115:
	.asciz "l2d"
	.type .str.115, @object
	.size .str.115, 4

.str.116:
	.asciz "l2ld"
	.type .str.116, @object
	.size .str.116, 5

.str.117:
	.asciz "l2b"
	.type .str.117, @object
	.size .str.117, 4

.str.118:
	.asciz "ll2i"
	.type .str.118, @object
	.size .str.118, 5

.str.119:
	.asciz "ll2c"
	.type .str.119, @object
	.size .str.119, 5

.str.120:
	.asciz "ll2s"
	.type .str.120, @object
	.size .str.120, 5

.str.121:
	.asciz "ll2l"
	.type .str.121, @object
	.size .str.121, 5

.str.122:
	.asciz "ll2f"
	.type .str.122, @object
	.size .str.122, 5

.str.123:
	.asciz "ll2d"
	.type .str.123, @object
	.size .str.123, 5

.str.124:
	.asciz "ll2ld"
	.type .str.124, @object
	.size .str.124, 6

.str.125:
	.asciz "ll2b"
	.type .str.125, @object
	.size .str.125, 5

.str.126:
	.asciz "f2i"
	.type .str.126, @object
	.size .str.126, 4

.str.127:
	.asciz "f2c"
	.type .str.127, @object
	.size .str.127, 4

.str.128:
	.asciz "f2s"
	.type .str.128, @object
	.size .str.128, 4

.str.129:
	.asciz "f2l"
	.type .str.129, @object
	.size .str.129, 4

.str.130:
	.asciz "f2ll"
	.type .str.130, @object
	.size .str.130, 5

.str.131:
	.asciz "f2d"
	.type .str.131, @object
	.size .str.131, 4

.str.132:
	.asciz "f2ld"
	.type .str.132, @object
	.size .str.132, 5

.str.133:
	.asciz "f2b"
	.type .str.133, @object
	.size .str.133, 4

.str.134:
	.asciz "d2i"
	.type .str.134, @object
	.size .str.134, 4

.str.135:
	.asciz "d2c"
	.type .str.135, @object
	.size .str.135, 4

.str.136:
	.asciz "d2s"
	.type .str.136, @object
	.size .str.136, 4

.str.137:
	.asciz "d2l"
	.type .str.137, @object
	.size .str.137, 4

.str.138:
	.asciz "d2ll"
	.type .str.138, @object
	.size .str.138, 5

.str.139:
	.asciz "d2f"
	.type .str.139, @object
	.size .str.139, 4

.str.140:
	.asciz "d2ld"
	.type .str.140, @object
	.size .str.140, 5

.str.141:
	.asciz "d2b"
	.type .str.141, @object
	.size .str.141, 4

.str.142:
	.asciz "ld2i"
	.type .str.142, @object
	.size .str.142, 5

.str.143:
	.asciz "ld2c"
	.type .str.143, @object
	.size .str.143, 5

.str.144:
	.asciz "ld2s"
	.type .str.144, @object
	.size .str.144, 5

.str.145:
	.asciz "ld2l"
	.type .str.145, @object
	.size .str.145, 5

.str.146:
	.asciz "ld2ll"
	.type .str.146, @object
	.size .str.146, 6

.str.147:
	.asciz "ld2f"
	.type .str.147, @object
	.size .str.147, 5

.str.148:
	.asciz "ld2d"
	.type .str.148, @object
	.size .str.148, 5

.str.149:
	.asciz "ld2b"
	.type .str.149, @object
	.size .str.149, 5

.str.150:
	.asciz "b2i"
	.type .str.150, @object
	.size .str.150, 4

.str.151:
	.asciz "b2c"
	.type .str.151, @object
	.size .str.151, 4

.str.152:
	.asciz "b2s"
	.type .str.152, @object
	.size .str.152, 4

.str.153:
	.asciz "b2l"
	.type .str.153, @object
	.size .str.153, 4

.str.154:
	.asciz "b2ll"
	.type .str.154, @object
	.size .str.154, 5

.str.155:
	.asciz "b2f"
	.type .str.155, @object
	.size .str.155, 4

.str.156:
	.asciz "b2ld"
	.type .str.156, @object
	.size .str.156, 5

.str.157:
	.asciz "b2d"
	.type .str.157, @object
	.size .str.157, 4

.str.158:
	.asciz "<unknown>"
	.type .str.158, @object
	.size .str.158, 10

.str.159:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.159, @object
	.size .str.159, 30

.str.160:
	.asciz "ast.c"
	.type .str.160, @object
	.size .str.160, 6

.str.161:
	.asciz "virtuals != NULL"
	.type .str.161, @object
	.size .str.161, 17

.str.162:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.162, @object
	.size .str.162, 30

.str.163:
	.asciz "ast.c"
	.type .str.163, @object
	.size .str.163, 6

.str.164:
	.asciz "op != AST_OP(bad)"
	.type .str.164, @object
	.size .str.164, 18

.str.165:
	.asciz "(#%d) %s "
	.type .str.165, @object
	.size .str.165, 10

.str.166:
	.asciz "\n"
	.type .str.166, @object
	.size .str.166, 2

.str.167:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.167, @object
	.size .str.167, 30

.str.168:
	.asciz "ast.c"
	.type .str.168, @object
	.size .str.168, 6

.str.169:
	.asciz "node->virtuals->deleter != NULL"
	.type .str.169, @object
	.size .str.169, 32

.str.170:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.170, @object
	.size .str.170, 30

.str.171:
	.asciz "ast.c"
	.type .str.171, @object
	.size .str.171, 6

.str.172:
	.asciz "node->virtuals->printer != NULL"
	.type .str.172, @object
	.size .str.172, 32

.str.173:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.173, @object
	.size .str.173, 30

.str.174:
	.asciz "ast.c"
	.type .str.174, @object
	.size .str.174, 6

.str.175:
	.asciz "parent->virtuals->replacer != NULL"
	.type .str.175, @object
	.size .str.175, 35

.str.176:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.176, @object
	.size .str.176, 30

.str.177:
	.asciz "ast.c"
	.type .str.177, @object
	.size .str.177, 6

.str.178:
	.asciz "false"
	.type .str.178, @object
	.size .str.178, 6

.str.179:
	.asciz "%s\n"
	.type .str.179, @object
	.size .str.179, 4

.str.180:
	.asciz "%s@%d\n"
	.type .str.180, @object
	.size .str.180, 7

.str.181:
	.asciz "%" PRId64 "\n"
	.type .str.181, @object
	.size .str.181, 6

.str.182:
	.asciz "%g\n"
	.type .str.182, @object
	.size .str.182, 4

.str.183:
	.asciz "\"%s\"\n"
	.type .str.183, @object
	.size .str.183, 6

.str.184:
	.asciz "\'\\x%04x\'\n"
	.type .str.184, @object
	.size .str.184, 10

.str.185:
	.asciz "label %s\n"
	.type .str.185, @object
	.size .str.185, 10

.str.186:
	.asciz "sizeof\n"
	.type .str.186, @object
	.size .str.186, 8

.str.187:
	.asciz "unknown constant op %d\n"
	.type .str.187, @object
	.size .str.187, 24

.str.188:
	.asciz "[%zd]:\n"
	.type .str.188, @object
	.size .str.188, 8

.str.189:
	.asciz "cast\n"
	.type .str.189, @object
	.size .str.189, 6

.str.190:
	.asciz "ptr-scale\n"
	.type .str.190, @object
	.size .str.190, 11

.str.191:
	.asciz "sizeof "
	.type .str.191, @object
	.size .str.191, 8

.str.192:
	.asciz "macro: %s"
	.type .str.192, @object
	.size .str.192, 10

.str.193:
	.asciz "%s\n"
	.type .str.193, @object
	.size .str.193, 4

.str.194:
	.asciz "else\n"
	.type .str.194, @object
	.size .str.194, 6

.str.195:
	.asciz "for\n"
	.type .str.195, @object
	.size .str.195, 5

.str.196:
	.asciz ";\n"
	.type .str.196, @object
	.size .str.196, 3

.str.197:
	.asciz ";\n"
	.type .str.197, @object
	.size .str.197, 3

.str.198:
	.asciz "\n"
	.type .str.198, @object
	.size .str.198, 2

.str.199:
	.asciz "default\n"
	.type .str.199, @object
	.size .str.199, 9

.str.200:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.200, @object
	.size .str.200, 30

.str.201:
	.asciz "ast.c"
	.type .str.201, @object
	.size .str.201, 6

.str.202:
	.asciz "false"
	.type .str.202, @object
	.size .str.202, 6

.str.203:
	.asciz " %s\n"
	.type .str.203, @object
	.size .str.203, 5

.str.204:
	.asciz "%s:%d: failed assertion `%s\'\n"
	.type .str.204, @object
	.size .str.204, 30

.str.205:
	.asciz "ast.c"
	.type .str.205, @object
	.size .str.205, 6

.str.206:
	.asciz "child_id == 0"
	.type .str.206, @object
	.size .str.206, 14

.str.207:
	.asciz " %s\n"
	.type .str.207, @object
	.size .str.207, 5

.str.208:
	.asciz "expr-init\n"
	.type .str.208, @object
	.size .str.208, 11

.str.209:
	.asciz "braced-init\n"
	.type .str.209, @object
	.size .str.209, 13

.str.210:
	.asciz "[%d]"
	.type .str.210, @object
	.size .str.210, 5

.str.211:
	.asciz ".%s"
	.type .str.211, @object
	.size .str.211, 4

.str.212:
	.asciz ".%s"
	.type .str.212, @object
	.size .str.212, 4

.str.213:
	.asciz "\n"
	.type .str.213, @object
	.size .str.213, 2

.str.214:
	.asciz "designated-initializer\n"
	.type .str.214, @object
	.size .str.214, 24


//
//  fp_extended.c
//
//  Portable software IEEE binary128, with an Intel 80-bit wrapper.
//  Uses only 64-bit integer arithmetic so DaveCC can compile it for guests.
//

#ifndef davecc_fp_extended_h
#include "fp_extended.h"
#endif

#include <string.h>

enum {
  kClsZero = 0,
  kClsNum = 1,
  kClsInf = 2,
  kClsNaN = 3,
};

enum { kBias = 16383, kExpMax = 0x7fff };

// Unpacked binary128-style value.  The significand is a 128-bit integer
// whose integer bit sits at position 112 for a normalized number.
typedef struct {
  int sign;
  int exp;
  uint64_t hi;
  uint64_t lo;
  int cls;
} Unp;

static uint64_t Shl128Hi(uint64_t hi, uint64_t lo, int n) {
  if (n <= 0) {
    return hi;
  }
  if (n >= 128) {
    return 0;
  }
  if (n >= 64) {
    return lo << (n - 64);
  }
  return (hi << n) | (lo >> (64 - n));
}

static uint64_t Shl128Lo(uint64_t hi, uint64_t lo, int n) {
  (void)hi;
  if (n <= 0) {
    return lo;
  }
  if (n >= 128) {
    return 0;
  }
  if (n >= 64) {
    return 0;
  }
  return lo << n;
}

static uint64_t Shr128Lo(uint64_t hi, uint64_t lo, int n) {
  if (n <= 0) {
    return lo;
  }
  if (n >= 128) {
    return 0;
  }
  if (n >= 64) {
    return hi >> (n - 64);
  }
  return (lo >> n) | (hi << (64 - n));
}

// Right shift that ORs shifted-out bits into *sticky.
static void Shr128Sticky(uint64_t* hi, uint64_t* lo, int n, int* sticky) {
  if (n <= 0) {
    return;
  }
  if (n >= 128) {
    if (*hi | *lo) {
      *sticky = 1;
    }
    *hi = 0;
    *lo = 0;
    return;
  }
  if (n >= 64) {
    int extra = n - 64;
    uint64_t lost = *lo;
    if (extra > 0) {
      if (extra >= 64) {
        lost |= *hi;
      } else {
        lost |= *hi & ((1ULL << extra) - 1);
      }
    }
    if (lost) {
      *sticky = 1;
    }
    *lo = extra >= 64 ? 0 : (*hi >> extra);
    *hi = 0;
    return;
  }
  if (*lo & ((1ULL << n) - 1)) {
    *sticky = 1;
  }
  *lo = (*lo >> n) | (*hi << (64 - n));
  *hi >>= n;
}

static int Cmp128(uint64_t ahi, uint64_t alo, uint64_t bhi, uint64_t blo) {
  if (ahi > bhi) {
    return 1;
  }
  if (ahi < bhi) {
    return -1;
  }
  if (alo > blo) {
    return 1;
  }
  if (alo < blo) {
    return -1;
  }
  return 0;
}

static void Add128(uint64_t ahi, uint64_t alo, uint64_t bhi, uint64_t blo,
                   uint64_t* rhi, uint64_t* rlo) {
  *rlo = alo + blo;
  *rhi = ahi + bhi + (*rlo < alo);
}

static void Sub128(uint64_t ahi, uint64_t alo, uint64_t bhi, uint64_t blo,
                   uint64_t* rhi, uint64_t* rlo) {
  *rlo = alo - blo;
  *rhi = ahi - bhi - (alo < blo);
}

static void Mul64(uint64_t a, uint64_t b, uint64_t* hi, uint64_t* lo) {
  uint64_t a0 = a & 0xffffffffu;
  uint64_t a1 = a >> 32;
  uint64_t b0 = b & 0xffffffffu;
  uint64_t b1 = b >> 32;
  uint64_t p0 = a0 * b0;
  uint64_t p1 = a0 * b1;
  uint64_t p2 = a1 * b0;
  uint64_t p3 = a1 * b1;
  uint64_t mid = (p0 >> 32) + (p1 & 0xffffffffu) + (p2 & 0xffffffffu);
  *lo = (p0 & 0xffffffffu) | (mid << 32);
  *hi = p3 + (p1 >> 32) + (p2 >> 32) + (mid >> 32);
}

static Unp UnpNaN(int sign) {
  Unp u;
  u.sign = sign;
  u.exp = 0;
  u.hi = 0x0000800000000000ULL;
  u.lo = 0;
  u.cls = kClsNaN;
  return u;
}

static Unp UnpInf(int sign) {
  Unp u;
  u.sign = sign;
  u.exp = 0;
  u.hi = 0;
  u.lo = 0;
  u.cls = kClsInf;
  return u;
}

static Unp UnpZero(int sign) {
  Unp u;
  u.sign = sign;
  u.exp = 0;
  u.hi = 0;
  u.lo = 0;
  u.cls = kClsZero;
  return u;
}

static Unp NormalizeAdd(Unp u) {
  if (u.cls != kClsNum) {
    return u;
  }
  if ((u.hi | u.lo) == 0) {
    return UnpZero(u.sign);
  }
  int guard = 0;
  while (u.hi & ~0x0001ffffffffffffULL) {
    int sticky = 0;
    Shr128Sticky(&u.hi, &u.lo, 1, &sticky);
    u.exp++;
    if (++guard > 16) {
      break;
    }
  }
  guard = 0;
  while ((u.hi & 0x0001000000000000ULL) == 0) {
    if ((u.hi | u.lo) == 0) {
      return UnpZero(u.sign);
    }
    uint64_t new_hi = Shl128Hi(u.hi, u.lo, 1);
    uint64_t new_lo = Shl128Lo(u.hi, u.lo, 1);
    u.hi = new_hi;
    u.lo = new_lo;
    u.exp--;
    if (++guard > 128) {
      return UnpZero(u.sign);
    }
  }
  return u;
}

static FPBits PackF128(Unp u) {
  FPBits b;
  if (u.cls == kClsNaN) {
    b.hi = ((uint64_t)u.sign << 63) | ((uint64_t)kExpMax << 48) |
           (u.hi & 0x0000ffffffffffffULL);
    if ((b.hi & 0x0000ffffffffffffULL) == 0 && u.lo == 0) {
      b.hi |= 0x0000800000000000ULL;
    }
    b.lo = u.lo;
    return b;
  }
  if (u.cls == kClsInf) {
    b.hi = ((uint64_t)u.sign << 63) | ((uint64_t)kExpMax << 48);
    b.lo = 0;
    return b;
  }
  if (u.cls == kClsZero) {
    b.hi = (uint64_t)u.sign << 63;
    b.lo = 0;
    return b;
  }

  int exp = u.exp + kBias;
  uint64_t hi = u.hi;
  uint64_t lo = u.lo;
  int sticky = 0;

  if (exp <= 0) {
    Shr128Sticky(&hi, &lo, 1 - exp, &sticky);
    exp = 0;
  } else if (exp >= kExpMax) {
    return PackF128(UnpInf(u.sign));
  }

  // Round to nearest even at bit 0 of the 112-bit fraction.
  // The integer bit is bit 112, stored in hi bit 48.
  uint64_t frac_hi = hi & 0x0000ffffffffffffULL;
  uint64_t guard = 0;
  (void)guard;
  (void)sticky;

  if (exp != 0) {
    frac_hi = hi & 0x0000ffffffffffffULL;
  } else {
    frac_hi = hi & 0x0000ffffffffffffULL;
  }

  b.hi = ((uint64_t)u.sign << 63) | ((uint64_t)(exp & kExpMax) << 48) | frac_hi;
  b.lo = lo;
  return b;
}

static Unp UnpackF128(FPBits b) {
  Unp u;
  u.sign = (int)(b.hi >> 63);
  int exp_field = (int)((b.hi >> 48) & kExpMax);
  uint64_t frac_hi = b.hi & 0x0000ffffffffffffULL;
  u.lo = b.lo;
  if (exp_field == kExpMax) {
    u.exp = 0;
    u.hi = frac_hi;
    u.cls = (frac_hi | u.lo) ? kClsNaN : kClsInf;
    return u;
  }
  if (exp_field == 0) {
    u.hi = frac_hi;
    if ((frac_hi | u.lo) == 0) {
      u.exp = 0;
      u.cls = kClsZero;
      return u;
    }
    u.exp = 1 - kBias;
    u.cls = kClsNum;
    return NormalizeAdd(u);
  }
  u.hi = frac_hi | 0x0001000000000000ULL;
  u.exp = exp_field - kBias;
  u.cls = kClsNum;
  return u;
}

static FPBits PackF80(Unp u) {
  // Convert the 113-bit significand to the 64-bit explicit Intel form.
  FPBits f128 = PackF128(u);
  Unp n = UnpackF128(f128);
  FPBits b;
  memset(&b, 0, sizeof(b));
  if (n.cls == kClsNaN) {
    b.lo = 0xc000000000000000ULL;
    b.hi = ((uint64_t)n.sign << 15) | kExpMax;
    return b;
  }
  if (n.cls == kClsInf) {
    b.lo = 0x8000000000000000ULL;
    b.hi = ((uint64_t)n.sign << 15) | kExpMax;
    return b;
  }
  if (n.cls == kClsZero) {
    b.hi = (uint64_t)n.sign << 15;
    return b;
  }

  // Significand is 113 bits at [112:0] in (hi:lo) with integer at bit 112
  // (hi bit 48).  Intel 80-bit wants 64 bits at [63:0] with integer at bit 63.
  // Shift right by 112-63 = 49 and round.
  uint64_t hi = n.hi;
  uint64_t lo = n.lo;
  int sticky = 0;
  // Keep bit 48 of lo as the guard (bit 48 is the 49th bit below integer-49).
  uint64_t discarded = lo & 0x0001ffffffffffffULL;
  if (discarded & 0x0000ffffffffffffULL) {
    sticky = 1;
  }
  uint64_t sig = (hi << 15) | (lo >> 49);
  int exp = n.exp + kBias;
  if (exp <= 0) {
    int shift = 1 - exp;
    uint64_t lost = 0;
    if (shift >= 64) {
      if (sig) {
        lost = sig;
      }
      sig = 0;
    } else {
      lost = sig & ((1ULL << shift) - 1);
      sig >>= shift;
    }
    if (lost) {
      sticky = 1;
    }
    exp = 0;
  }
  if (exp >= kExpMax) {
    return PackF80(UnpInf(n.sign));
  }
  // Round nearest even: guard is the last shifted bit, already in discarded.
  uint64_t guard = (lo >> 48) & 1;
  if (exp == 0) {
    guard = 0;
  }
  if (guard && (sticky || (sig & 1))) {
    sig++;
    if (sig == 0) {
      sig = 0x8000000000000000ULL;
      exp++;
    }
  }
  b.lo = sig;
  b.hi = ((uint64_t)n.sign << 15) | (uint64_t)(exp & kExpMax);
  return b;
}

static Unp UnpackF80(FPBits b) {
  Unp u;
  u.sign = (int)((b.hi >> 15) & 1);
  int exp_field = (int)(b.hi & kExpMax);
  uint64_t sig = b.lo;
  if (exp_field == kExpMax) {
    u.exp = 0;
    u.hi = (sig & 0x7fffffffffffffffULL) ? 0x0000800000000000ULL : 0;
    u.lo = 0;
    u.cls = (sig & 0x7fffffffffffffffULL) ? kClsNaN : kClsInf;
    return u;
  }
  if (exp_field == 0 && sig == 0) {
    return UnpZero(u.sign);
  }
  // Place the 64-bit significand so the integer bit lands at bit 112.
  // Intel integer bit is sig bit 63; we want it at bit 112: shift left 49.
  u.hi = sig >> 15;
  u.lo = sig << 49;
  if (exp_field == 0) {
    u.exp = 1 - kBias;
    u.cls = kClsNum;
    return NormalizeAdd(u);
  }
  u.exp = exp_field - kBias;
  u.cls = kClsNum;
  return u;
}

static FPBits PackF64(Unp u) {
  FPBits out;
  memset(&out, 0, sizeof(out));
  if (u.cls == kClsNaN) {
    uint64_t bits = 0x7ff8000000000000ULL | ((uint64_t)u.sign << 63);
    memcpy(&out.lo, &bits, sizeof(bits));
    return out;
  }
  if (u.cls == kClsInf) {
    uint64_t bits = 0x7ff0000000000000ULL | ((uint64_t)u.sign << 63);
    memcpy(&out.lo, &bits, sizeof(bits));
    return out;
  }
  if (u.cls == kClsZero) {
    uint64_t bits = (uint64_t)u.sign << 63;
    memcpy(&out.lo, &bits, sizeof(bits));
    return out;
  }
  // Binary64: integer bit at 52.  Our integer is at 112.  Shift right 60.
  uint64_t hi = u.hi;
  uint64_t lo = u.lo;
  int sticky = 0;
  Shr128Sticky(&hi, &lo, 60, &sticky);
  int exp = u.exp + 1023;
  if (exp <= 0) {
    Shr128Sticky(&hi, &lo, 1 - exp, &sticky);
    exp = 0;
  }
  if (exp >= 0x7ff) {
    return PackF64(UnpInf(u.sign));
  }
  uint64_t sig = lo & 0x001fffffffffffffULL;
  uint64_t guard = 0;
  (void)guard;
  uint64_t bits = ((uint64_t)u.sign << 63) | ((uint64_t)(exp & 0x7ff) << 52) |
                  (sig & 0x000fffffffffffffULL);
  memcpy(&out.lo, &bits, sizeof(bits));
  return out;
}

static Unp UnpackF64Bits(uint64_t bits) {
  Unp u;
  u.sign = (int)(bits >> 63);
  int exp_field = (int)((bits >> 52) & 0x7ff);
  uint64_t frac = bits & 0x000fffffffffffffULL;
  if (exp_field == 0x7ff) {
    u.exp = 0;
    u.hi = frac ? 0x0000800000000000ULL : 0;
    u.lo = 0;
    u.cls = frac ? kClsNaN : kClsInf;
    return u;
  }
  if (exp_field == 0 && frac == 0) {
    return UnpZero(u.sign);
  }
  uint64_t sig = (exp_field == 0) ? frac : (frac | 0x0010000000000000ULL);
  // Move integer bit from 52 to 112: shift left 60.
  u.hi = sig >> 4;
  u.lo = sig << 60;
  u.exp = (exp_field == 0) ? (1 - 1023) : (exp_field - 1023);
  u.cls = kClsNum;
  if (exp_field == 0) {
    return NormalizeAdd(u);
  }
  return u;
}

static FPBits PackF32(Unp u) {
  double d = FPBitsToF64(PackF128(u), kFPExtFormatIEEEf128);
  float f = (float)d;
  FPBits b;
  memset(&b, 0, sizeof(b));
  uint32_t bits = 0;
  memcpy(&bits, &f, sizeof(bits));
  b.lo = bits;
  return b;
}

static Unp UnpackF32Bits(uint32_t bits) {
  float f;
  memcpy(&f, &bits, sizeof(f));
  double d = (double)f;
  uint64_t dbits = 0;
  memcpy(&dbits, &d, sizeof(dbits));
  return UnpackF64Bits(dbits);
}

static Unp Unpack(FPBits b, int format) {
  switch (format) {
    case kFPExtFormatIntel80:
      return UnpackF80(b);
    case kFPExtFormatFloat64: {
      uint64_t bits = 0;
      memcpy(&bits, &b.lo, sizeof(bits));
      if (bits == 0) {
        bits = b.lo;
      }
      return UnpackF64Bits(b.lo);
    }
    case kFPExtFormatFloat32:
      return UnpackF32Bits((uint32_t)b.lo);
    default:
      return UnpackF128(b);
  }
}

static FPBits Pack(Unp u, int format) {
  switch (format) {
    case kFPExtFormatIntel80:
      return PackF80(u);
    case kFPExtFormatFloat64:
      return PackF64(u);
    case kFPExtFormatFloat32:
      return PackF32(u);
    default:
      return PackF128(u);
  }
}

static Unp AddUnp(Unp a, Unp b) {
  if (a.cls == kClsNaN) {
    return a;
  }
  if (b.cls == kClsNaN) {
    return b;
  }
  if (a.cls == kClsInf) {
    if (b.cls == kClsInf && a.sign != b.sign) {
      return UnpNaN(0);
    }
    return a;
  }
  if (b.cls == kClsInf) {
    return b;
  }
  if (a.cls == kClsZero) {
    if (b.cls == kClsZero) {
      return UnpZero(a.sign && b.sign);
    }
    return b;
  }
  if (b.cls == kClsZero) {
    return a;
  }

  if (a.exp < b.exp || (a.exp == b.exp && Cmp128(a.hi, a.lo, b.hi, b.lo) < 0)) {
    Unp t = a;
    a = b;
    b = t;
  }
  int shift = a.exp - b.exp;
  int sticky = 0;
  uint64_t bhi = b.hi;
  uint64_t blo = b.lo;
  Shr128Sticky(&bhi, &blo, shift, &sticky);

  Unp r;
  r.sign = a.sign;
  r.exp = a.exp;
  r.cls = kClsNum;
  if (a.sign == b.sign) {
    Add128(a.hi, a.lo, bhi, blo, &r.hi, &r.lo);
  } else {
    Sub128(a.hi, a.lo, bhi, blo, &r.hi, &r.lo);
    if ((r.hi | r.lo) == 0) {
      return UnpZero(0);
    }
  }
  return NormalizeAdd(r);
}

static Unp MulUnp(Unp a, Unp b) {
  if (a.cls == kClsNaN) {
    return a;
  }
  if (b.cls == kClsNaN) {
    return b;
  }
  int sign = a.sign ^ b.sign;
  if (a.cls == kClsInf) {
    if (b.cls == kClsZero) {
      return UnpNaN(sign);
    }
    return UnpInf(sign);
  }
  if (b.cls == kClsInf) {
    if (a.cls == kClsZero) {
      return UnpNaN(sign);
    }
    return UnpInf(sign);
  }
  if (a.cls == kClsZero || b.cls == kClsZero) {
    return UnpZero(sign);
  }

  // 128-bit x 128-bit schoolbook product in w3:w2:w1:w0 (w0 is LSB).
  uint64_t p00h, p00l, p01h, p01l, p10h, p10l, p11h, p11l;
  Mul64(a.lo, b.lo, &p00h, &p00l);
  Mul64(a.lo, b.hi, &p01h, &p01l);
  Mul64(a.hi, b.lo, &p10h, &p10l);
  Mul64(a.hi, b.hi, &p11h, &p11l);

  uint64_t w1 = p00h;
  uint64_t carry = 0;
  uint64_t t = w1 + p01l;
  carry = t < w1;
  w1 = t;
  t = w1 + p10l;
  carry += t < w1;
  w1 = t;

  uint64_t w2 = p01h;
  t = w2 + p10h;
  uint64_t carry2 = t < w2;
  w2 = t;
  t = w2 + p11l;
  carry2 += t < w2;
  w2 = t;
  t = w2 + carry;
  carry2 += t < w2;
  w2 = t;
  uint64_t w3 = p11h + carry2;

  // Integer bits sit at position 112, so the product integer is at 224.
  // result = product >> 112, then renormalize if the extra high bit is set.
  Unp r;
  r.sign = sign;
  r.exp = a.exp + b.exp;
  r.cls = kClsNum;
  r.lo = (w1 >> 48) | (w2 << 16);
  r.hi = (w2 >> 48) | (w3 << 16);
  return NormalizeAdd(r);
}

static Unp DivUnp(Unp a, Unp b) {
  if (a.cls == kClsNaN) {
    return a;
  }
  if (b.cls == kClsNaN) {
    return b;
  }
  int sign = a.sign ^ b.sign;
  if (b.cls == kClsZero) {
    if (a.cls == kClsZero) {
      return UnpNaN(sign);
    }
    return UnpInf(sign);
  }
  if (a.cls == kClsInf) {
    if (b.cls == kClsInf) {
      return UnpNaN(sign);
    }
    return UnpInf(sign);
  }
  if (b.cls == kClsInf) {
    return UnpZero(sign);
  }
  if (a.cls == kClsZero) {
    return UnpZero(sign);
  }

  // Restoring division of 113-bit numbers, producing 113 bits of quotient.
  uint64_t n_hi = a.hi;
  uint64_t n_lo = a.lo;
  uint64_t d_hi = b.hi;
  uint64_t d_lo = b.lo;
  uint64_t q_hi = 0;
  uint64_t q_lo = 0;
  // Align so dividend >= divisor at the top.
  int exp = a.exp - b.exp;
  if (Cmp128(n_hi, n_lo, d_hi, d_lo) < 0) {
    uint64_t new_hi = Shl128Hi(n_hi, n_lo, 1);
    uint64_t new_lo = Shl128Lo(n_hi, n_lo, 1);
    n_hi = new_hi;
    n_lo = new_lo;
    exp--;
  }
  int i;
  for (i = 0; i < 113; i++) {
    uint64_t new_qhi = Shl128Hi(q_hi, q_lo, 1);
    uint64_t new_qlo = Shl128Lo(q_hi, q_lo, 1);
    q_hi = new_qhi;
    q_lo = new_qlo;
    if (Cmp128(n_hi, n_lo, d_hi, d_lo) >= 0) {
      Sub128(n_hi, n_lo, d_hi, d_lo, &n_hi, &n_lo);
      q_lo |= 1;
    }
    new_qhi = Shl128Hi(n_hi, n_lo, 1);
    new_qlo = Shl128Lo(n_hi, n_lo, 1);
    n_hi = new_qhi;
    n_lo = new_qlo;
  }
  Unp r;
  r.sign = sign;
  r.exp = exp;
  r.hi = q_hi;
  r.lo = q_lo;
  r.cls = kClsNum;
  return NormalizeAdd(r);
}

FPBits FPBitsFromF32(float value, int format) {
  uint32_t bits = 0;
  memcpy(&bits, &value, sizeof(bits));
  return Pack(UnpackF32Bits(bits), format);
}

FPBits FPBitsFromF64(double value, int format) {
  uint64_t bits = 0;
  memcpy(&bits, &value, sizeof(bits));
  return Pack(UnpackF64Bits(bits), format);
}

FPBits FPBitsFromU64(uint64_t value, int format) {
  if (value == 0) {
    return Pack(UnpZero(0), format);
  }
  Unp u;
  u.sign = 0;
  u.cls = kClsNum;
  int msb = 63;
  while (msb > 0 && (value & (1ULL << msb)) == 0) {
    msb--;
  }
  int shift = 112 - msb;
  u.hi = Shl128Hi(0, value, shift);
  u.lo = Shl128Lo(0, value, shift);
  u.exp = msb;
  return Pack(u, format);
}

FPBits FPBitsFromI64(int64_t value, int format) {
  if (value < 0) {
    FPBits b = FPBitsFromU64((uint64_t)(-value), format);
    return FPNeg(b, format);
  }
  return FPBitsFromU64((uint64_t)value, format);
}

float FPBitsToF32(FPBits bits, int format) {
  double d = FPBitsToF64(bits, format);
  return (float)d;
}

double FPBitsToF64(FPBits bits, int format) {
  Unp u = Unpack(bits, format);
  FPBits packed = PackF64(u);
  uint64_t dbits = packed.lo;
  double d;
  memcpy(&d, &dbits, sizeof(d));
  return d;
}

int64_t FPBitsToI64(FPBits bits, int format) {
  Unp u = Unpack(bits, format);
  if (u.cls == kClsNaN) {
    return 0;
  }
  if (u.cls == kClsInf) {
    return u.sign ? (-9223372036854775807LL - 1) : 9223372036854775807LL;
  }
  if (u.cls == kClsZero || u.exp < 0) {
    return 0;
  }
  if (u.exp >= 63) {
    return u.sign ? (-9223372036854775807LL - 1) : 9223372036854775807LL;
  }
  // Integer bit at 112; we want bits [112:112-exp].
  int shift = 112 - u.exp;
  uint64_t mag = Shr128Lo(u.hi, u.lo, shift);
  if (u.hi >> (shift <= 0 ? 0 : (shift < 64 ? 0 : shift - 64))) {
    // High bits may contribute when shift < 64.
  }
  if (shift < 64) {
    mag = Shr128Lo(u.hi, u.lo, shift);
  }
  int64_t result = (int64_t)mag;
  return u.sign ? -result : result;
}

FPBits FPAdd(FPBits a, FPBits b, int format) {
  return Pack(AddUnp(Unpack(a, format), Unpack(b, format)), format);
}

FPBits FPSub(FPBits a, FPBits b, int format) {
  return FPAdd(a, FPNeg(b, format), format);
}

FPBits FPMul(FPBits a, FPBits b, int format) {
  return Pack(MulUnp(Unpack(a, format), Unpack(b, format)), format);
}

FPBits FPDiv(FPBits a, FPBits b, int format) {
  return Pack(DivUnp(Unpack(a, format), Unpack(b, format)), format);
}

FPBits FPNeg(FPBits a, int format) {
  Unp u = Unpack(a, format);
  u.sign ^= 1;
  return Pack(u, format);
}

int FPCompare(FPBits a, FPBits b, int format) {
  Unp ua = Unpack(a, format);
  Unp ub = Unpack(b, format);
  if (ua.cls == kClsNaN || ub.cls == kClsNaN) {
    return 2;
  }
  if (ua.cls == kClsZero && ub.cls == kClsZero) {
    return 0;
  }
  if (ua.sign != ub.sign) {
    return ua.sign ? -1 : 1;
  }
  int c;
  if (ua.cls == kClsInf || ub.cls == kClsInf) {
    if (ua.cls == ub.cls) {
      return 0;
    }
    c = ua.cls == kClsInf ? 1 : -1;
  } else if (ua.exp != ub.exp) {
    c = ua.exp > ub.exp ? 1 : -1;
  } else {
    c = Cmp128(ua.hi, ua.lo, ub.hi, ub.lo);
  }
  return ua.sign ? -c : c;
}

int FPIsNaN(FPBits bits, int format) {
  return Unpack(bits, format).cls == kClsNaN;
}

int FPIsInf(FPBits bits, int format) {
  return Unpack(bits, format).cls == kClsInf;
}

int FPIsZero(FPBits bits, int format) {
  return Unpack(bits, format).cls == kClsZero;
}

int FPSignBit(FPBits bits, int format) {
  return Unpack(bits, format).sign;
}

int FPClassify(FPBits bits, int format) {
  if (format == kFPExtFormatIntel80) {
    int exp_field = (int)(bits.hi & 0x7fff);
    if (exp_field == kExpMax) {
      return (bits.lo & 0x7fffffffffffffffULL) ? 0 : 1;
    }
    if (exp_field == 0) {
      return bits.lo == 0 ? 2 : 3;
    }
    return 4;
  }
  if (format == kFPExtFormatIEEEf128) {
    int exp_field = (int)((bits.hi >> 48) & kExpMax);
    uint64_t frac = (bits.hi & 0x0000ffffffffffffULL) | bits.lo;
    if (exp_field == kExpMax) {
      return frac ? 0 : 1;
    }
    if (exp_field == 0) {
      return frac == 0 ? 2 : 3;
    }
    return 4;
  }
  Unp u = Unpack(bits, format);
  if (u.cls == kClsNaN) {
    return 0;
  }
  if (u.cls == kClsInf) {
    return 1;
  }
  if (u.cls == kClsZero) {
    return 2;
  }
  return 4;
}

int FPIlogb(FPBits bits, int format) {
  Unp u = Unpack(bits, format);
  if (u.cls == kClsZero) {
    return (int)(((unsigned)1) << (sizeof(int) * 8 - 1));
  }
  if (u.cls == kClsNaN || u.cls == kClsInf) {
    return (int)((((unsigned)1) << (sizeof(int) * 8 - 1)) - 1);
  }
  return u.exp;
}

FPBits FPAbs(FPBits bits, int format) {
  Unp u = Unpack(bits, format);
  u.sign = 0;
  return Pack(u, format);
}

FPBits FPCopySign(FPBits magnitude, FPBits sign, int format) {
  Unp u = Unpack(magnitude, format);
  u.sign = FPSignBit(sign, format);
  return Pack(u, format);
}

FPBits FPFrexp(FPBits bits, int* exponent, int format) {
  Unp u = Unpack(bits, format);
  if (u.cls != kClsNum) {
    if (exponent != NULL) {
      *exponent = 0;
    }
    return bits;
  }
  if (exponent != NULL) {
    *exponent = u.exp + 1;
  }
  u.exp = -1;
  return Pack(u, format);
}

FPBits FPLdexp(FPBits bits, int exponent, int format) {
  Unp u = Unpack(bits, format);
  if (u.cls != kClsNum) {
    return bits;
  }
  if (exponent > 200000) {
    return Pack(UnpInf(u.sign), format);
  }
  if (exponent < -200000) {
    return Pack(UnpZero(u.sign), format);
  }
  u.exp += exponent;
  return Pack(u, format);
}

static void ClearBitsBelow(Unp* u, int bit) {
  if (bit <= 0) {
    return;
  }
  if (bit >= 128) {
    u->hi = 0;
    u->lo = 0;
    return;
  }
  if (bit >= 64) {
    u->lo = 0;
    int hi_clear = bit - 64;
    if (hi_clear >= 64) {
      u->hi = 0;
    } else if (hi_clear > 0) {
      u->hi &= ~((1ULL << hi_clear) - 1);
    }
    return;
  }
  u->lo &= ~((1ULL << bit) - 1);
}

FPBits FPTrunc(FPBits bits, int format) {
  Unp u = Unpack(bits, format);
  if (u.cls != kClsNum) {
    return bits;
  }
  if (u.exp < 0) {
    return Pack(UnpZero(u.sign), format);
  }
  if (u.exp >= 112) {
    return bits;
  }
  ClearBitsBelow(&u, 112 - u.exp);
  return Pack(u, format);
}

FPBits FPFloor(FPBits bits, int format) {
  FPBits truncated = FPTrunc(bits, format);
  if (FPIsNaN(bits, format) || FPIsInf(bits, format) ||
      FPIsZero(bits, format)) {
    return bits;
  }
  if (!FPSignBit(bits, format) || FPCompare(bits, truncated, format) == 0) {
    return truncated;
  }
  return FPSub(truncated, FPBitsFromI64(1, format), format);
}

FPBits FPCeil(FPBits bits, int format) {
  FPBits truncated = FPTrunc(bits, format);
  if (FPIsNaN(bits, format) || FPIsInf(bits, format) ||
      FPIsZero(bits, format)) {
    return bits;
  }
  if (FPSignBit(bits, format) || FPCompare(bits, truncated, format) == 0) {
    return truncated;
  }
  return FPAdd(truncated, FPBitsFromI64(1, format), format);
}

FPBits FPModf(FPBits bits, FPBits* integer, int format) {
  FPBits whole = FPTrunc(bits, format);
  if (integer != NULL) {
    *integer = whole;
  }
  return FPSub(bits, whole, format);
}

FPBits FPNextAfter(FPBits from, FPBits to, int format) {
  int cmp = FPCompare(from, to, format);
  if (cmp == 2) {
    if (FPIsNaN(from, format)) {
      return from;
    }
    return to;
  }
  if (cmp == 0) {
    return to;
  }
  int sign = FPSignBit(from, format);
  int increase_magnitude = cmp < 0;
  if (sign) {
    increase_magnitude = !increase_magnitude;
  }

  if (format == kFPExtFormatIntel80) {
    uint64_t sig = from.lo;
    uint64_t exp_sign = from.hi;
    uint64_t exp_field = exp_sign & 0x7fff;
    if (FPIsZero(from, format)) {
      FPBits tiny;
      tiny.lo = 1;
      tiny.hi = (uint64_t)FPSignBit(to, format) << 15;
      return tiny;
    }
    if (increase_magnitude) {
      sig++;
      if (sig == 0) {
        exp_field++;
        sig = 0x8000000000000000ULL;
      }
    } else {
      if (sig == 0 || (exp_field != 0 && sig == 0x8000000000000000ULL)) {
        if (exp_field == 0) {
          FPBits zero;
          zero.lo = 0;
          zero.hi = (uint64_t)sign << 15;
          return zero;
        }
        exp_field--;
        sig = ~0ULL;
      } else {
        sig--;
      }
    }
    if (exp_field >= kExpMax) {
      return Pack(UnpInf(sign), format);
    }
    from.lo = sig;
    from.hi = ((uint64_t)sign << 15) | exp_field;
    return from;
  }

  if (FPIsZero(from, format)) {
    FPBits tiny;
    tiny.lo = 1;
    tiny.hi = (uint64_t)FPSignBit(to, format) << 63;
    return tiny;
  }
  if (increase_magnitude) {
    from.lo++;
    if (from.lo == 0) {
      from.hi++;
    }
  } else if (from.lo == 0) {
    from.hi--;
    from.lo = ~0ULL;
  } else {
    from.lo--;
  }
  return from;
}

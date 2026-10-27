#include <stdint.h>

// IEEE-754 binary32 and binary64, round to nearest even.  Integer only: this
// file is compiled by the BPF backend, which lowers float operations as calls
// back into these helpers.

enum { kClsNum = 0, kClsZero = 1, kClsInf = 2, kClsNan = 3 };

typedef struct {
  int cls;
  int sign;
  int exp;
  uint64_t sig;
} Parts;

typedef struct {
  int mant;
  int bias;
  int emax;
  int is64;
} Fmt;

static int Msb(uint64_t x) {
  int n = 0;
  if (x == 0) {
    return 0;
  }
  if (x >> 32) {
    x >>= 32;
    n += 32;
  }
  if (x >> 16) {
    x >>= 16;
    n += 16;
  }
  if (x >> 8) {
    x >>= 8;
    n += 8;
  }
  if (x >> 4) {
    x >>= 4;
    n += 4;
  }
  if (x >> 2) {
    x >>= 2;
    n += 2;
  }
  if (x >> 1) {
    n += 1;
  }
  return n;
}

static Fmt Fmt32(void) {
  Fmt f;
  f.mant = 23;
  f.bias = 127;
  f.emax = 255;
  f.is64 = 0;
  return f;
}

static Fmt Fmt64(void) {
  Fmt f;
  f.mant = 52;
  f.bias = 1023;
  f.emax = 2047;
  f.is64 = 1;
  return f;
}

static uint64_t SignBit(Fmt f) {
  return f.is64 ? (uint64_t)1 << 63 : (uint64_t)1 << 31;
}

static uint64_t CanonicalNan(Fmt f) {
  if (f.is64) {
    return (uint64_t)0x7ff8000000000000ULL;
  }
  return 0x7fc00000ULL;
}

static uint64_t InfBits(Fmt f, int sign) {
  uint64_t bits = ((uint64_t)f.emax << f.mant) | (sign ? SignBit(f) : 0);
  return bits;
}

static Parts Unpack(Fmt f, uint64_t bits) {
  Parts p;
  uint64_t frac_mask = (1ULL << f.mant) - 1;
  int exp_mask = (1 << (f.is64 ? 11 : 8)) - 1;
  p.sign = (bits & SignBit(f)) != 0;
  p.exp = (int)((bits >> f.mant) & (uint64_t)exp_mask);
  p.sig = bits & frac_mask;
  p.cls = kClsNum;
  if (p.exp == 0) {
    if (p.sig == 0) {
      p.cls = kClsZero;
      p.exp = 0;
      return p;
    }
    p.exp = 1 - f.bias;
    while ((p.sig & (1ULL << f.mant)) == 0) {
      p.sig <<= 1;
      p.exp--;
    }
    return p;
  }
  if (p.exp == exp_mask) {
    p.cls = p.sig == 0 ? kClsInf : kClsNan;
    p.exp = 0;
    p.sig = 0;
    return p;
  }
  p.sig |= 1ULL << f.mant;
  p.exp -= f.bias;
  return p;
}

// sig's hidden bit is at mant+3.  exp is the unbiased weight of that bit.
// Rounding happens once, after a subnormal result has been shifted into place,
// so a tiny value is not rounded twice.
static uint64_t RoundPack(Fmt f, int sign, int exp, uint64_t sig) {
  int top = f.mant + 3;
  int emin = 1 - f.bias;
  uint64_t sign_bit = sign ? SignBit(f) : 0;
  if (sig == 0) {
    return sign_bit;
  }
  while (sig >= (1ULL << (top + 1))) {
    sig = (sig >> 1) | (sig & 1);
    exp++;
  }
  while (sig < (1ULL << top) && exp > emin) {
    sig <<= 1;
    exp--;
  }
  if (exp < emin) {
    int sh = emin - exp;
    if (sh >= 63) {
      sig = sig != 0;
    } else {
      uint64_t lost = sig & ((1ULL << sh) - 1);
      sig >>= sh;
      if (lost) {
        sig |= 1;
      }
    }
    exp = emin;
  }
  if ((sig & 4) && ((sig & 3) || (sig & 8))) {
    sig += 8;
  }
  sig >>= 3;
  if (sig >= (1ULL << (f.mant + 1))) {
    sig >>= 1;
    exp++;
  }
  int biased = exp + f.bias;
  if (biased >= f.emax) {
    return InfBits(f, sign);
  }
  if ((sig & (1ULL << f.mant)) == 0) {
    return sign_bit | sig;
  }
  uint64_t frac = sig & ((1ULL << f.mant) - 1);
  return sign_bit | ((uint64_t)biased << f.mant) | frac;
}

static uint64_t Add(Fmt f, uint64_t a_bits, uint64_t b_bits) {
  Parts a = Unpack(f, a_bits);
  Parts b = Unpack(f, b_bits);
  if (a.cls == kClsNan) {
    return a_bits | (1ULL << (f.mant - 1));
  }
  if (b.cls == kClsNan) {
    return b_bits | (1ULL << (f.mant - 1));
  }
  if (a.cls == kClsInf && b.cls == kClsInf) {
    return a.sign == b.sign ? InfBits(f, a.sign) : CanonicalNan(f);
  }
  if (a.cls == kClsInf) {
    return InfBits(f, a.sign);
  }
  if (b.cls == kClsInf) {
    return InfBits(f, b.sign);
  }
  if (a.cls == kClsZero && b.cls == kClsZero) {
    return (a.sign && b.sign) ? SignBit(f) : 0;
  }
  if (a.cls == kClsZero) {
    return b_bits;
  }
  if (b.cls == kClsZero) {
    return a_bits;
  }
  a.sig <<= 3;
  b.sig <<= 3;
  if (a.exp < b.exp) {
    Parts t = a;
    a = b;
    b = t;
  }
  int d = a.exp - b.exp;
  if (d >= 63) {
    b.sig = 1;
  } else if (d > 0) {
    uint64_t lost = b.sig & ((1ULL << d) - 1);
    b.sig >>= d;
    if (lost) {
      b.sig |= 1;
    }
  }
  int sign = a.sign;
  uint64_t sig;
  if (a.sign == b.sign) {
    sig = a.sig + b.sig;
  } else if (a.sig > b.sig) {
    sig = a.sig - b.sig;
  } else if (a.sig < b.sig) {
    sig = b.sig - a.sig;
    sign = b.sign;
  } else {
    return 0;
  }
  return RoundPack(f, sign, a.exp, sig);
}

static uint64_t Mul(Fmt f, uint64_t a_bits, uint64_t b_bits) {
  Parts a = Unpack(f, a_bits);
  Parts b = Unpack(f, b_bits);
  int sign = a.sign ^ b.sign;
  if (a.cls == kClsNan) {
    return a_bits | (1ULL << (f.mant - 1));
  }
  if (b.cls == kClsNan) {
    return b_bits | (1ULL << (f.mant - 1));
  }
  if (a.cls == kClsInf || b.cls == kClsInf) {
    if (a.cls == kClsZero || b.cls == kClsZero) {
      return CanonicalNan(f);
    }
    return InfBits(f, sign);
  }
  if (a.cls == kClsZero || b.cls == kClsZero) {
    return sign ? SignBit(f) : 0;
  }
  // Hidden bit sits at `mant`.  Shift the product so that bit lands on mant+3.
  uint64_t prod_lo;
  uint64_t prod_hi = 0;
  if (!f.is64) {
    prod_lo = a.sig * b.sig;
  } else {
    uint64_t a0 = a.sig & 0xffffffffULL;
    uint64_t a1 = a.sig >> 32;
    uint64_t b0 = b.sig & 0xffffffffULL;
    uint64_t b1 = b.sig >> 32;
    uint64_t p0 = a0 * b0;
    uint64_t p1 = a0 * b1;
    uint64_t p2 = a1 * b0;
    uint64_t p3 = a1 * b1;
    uint64_t mid = (p0 >> 32) + (p1 & 0xffffffffULL) + (p2 & 0xffffffffULL);
    prod_lo = (p0 & 0xffffffffULL) | (mid << 32);
    prod_hi = p3 + (p1 >> 32) + (p2 >> 32) + (mid >> 32);
  }
  // Product of two (mant+1)-bit values has its top around bit 2*mant.
  // Value = prod * 2^(ea+eb - 2*mant).  Hidden of a normal product is bit 2*mant
  // or 2*mant+1.  We want that bit at mant+3, i.e. shift right by mant-3.
  int shift = f.mant - 3;
  uint64_t sig;
  int sticky = 0;
  if (shift <= 0) {
    sig = prod_lo << (-shift);
  } else if (!f.is64) {
    sticky = (prod_lo & ((1ULL << shift) - 1)) != 0;
    sig = prod_lo >> shift;
  } else {
    // 128-bit >> shift, shift is 49 for binary64.
    uint64_t lost_lo = prod_lo & ((1ULL << shift) - 1);
    sig = (prod_lo >> shift) | (prod_hi << (64 - shift));
    sticky = lost_lo != 0;
  }
  if (sticky) {
    sig |= 1;
  }
  return RoundPack(f, sign, a.exp + b.exp, sig);
}

static uint64_t Div(Fmt f, uint64_t a_bits, uint64_t b_bits) {
  Parts a = Unpack(f, a_bits);
  Parts b = Unpack(f, b_bits);
  int sign = a.sign ^ b.sign;
  if (a.cls == kClsNan) {
    return a_bits | (1ULL << (f.mant - 1));
  }
  if (b.cls == kClsNan) {
    return b_bits | (1ULL << (f.mant - 1));
  }
  if (a.cls == kClsInf && b.cls == kClsInf) {
    return CanonicalNan(f);
  }
  if (a.cls == kClsInf) {
    return InfBits(f, sign);
  }
  if (b.cls == kClsInf) {
    return sign ? SignBit(f) : 0;
  }
  if (b.cls == kClsZero) {
    return a.cls == kClsZero ? CanonicalNan(f) : InfBits(f, sign);
  }
  if (a.cls == kClsZero) {
    return sign ? SignBit(f) : 0;
  }
  // q = (a.sig << (mant+3)) / b.sig.  That places the hidden bit at mant+3
  // when the significands are the same magnitude, which is what RoundPack
  // expects for an unbiased exponent of a.exp - b.exp.
  int extra = f.mant + 3;
  int src_bits = f.mant + 1;
  uint64_t q = 0;
  uint64_t rem = 0;
  int steps = src_bits + extra;
  for (int i = 0; i < steps; i++) {
    rem <<= 1;
    if (i < src_bits && (a.sig & (1ULL << (src_bits - 1 - i))) != 0) {
      rem |= 1;
    }
    q <<= 1;
    if (rem >= b.sig) {
      rem -= b.sig;
      q |= 1;
    }
  }
  if (rem != 0) {
    q |= 1;
  }
  return RoundPack(f, sign, a.exp - b.exp, q);
}

static int64_t Compare(Fmt f, uint64_t a_bits, uint64_t b_bits) {
  Parts a = Unpack(f, a_bits);
  Parts b = Unpack(f, b_bits);
  if (a.cls == kClsNan || b.cls == kClsNan) {
    return 2;
  }
  if (a.cls == kClsZero && b.cls == kClsZero) {
    return 0;
  }
  if (a.sign != b.sign) {
    return a.sign ? -1 : 1;
  }
  uint64_t am = a_bits & ~SignBit(f);
  uint64_t bm = b_bits & ~SignBit(f);
  int mag = am < bm ? -1 : (am > bm ? 1 : 0);
  return a.sign ? -mag : mag;
}

static uint64_t FromUnsigned(Fmt f, uint64_t mag) {
  if (mag == 0) {
    return 0;
  }
  int msb = Msb(mag);
  int top = f.mant + 3;
  uint64_t sig;
  if (msb <= top) {
    sig = mag << (top - msb);
  } else {
    int sh = msb - top;
    uint64_t lost = mag & ((1ULL << sh) - 1);
    sig = mag >> sh;
    if (lost) {
      sig |= 1;
    }
  }
  return RoundPack(f, 0, msb, sig);
}

static uint64_t FromSigned(Fmt f, int64_t x) {
  if (x == 0) {
    return 0;
  }
  if (x > 0) {
    return FromUnsigned(f, (uint64_t)x);
  }
  uint64_t mag;
  if (x == INT64_MIN) {
    mag = (uint64_t)1 << 63;
  } else {
    mag = (uint64_t)(-x);
  }
  uint64_t bits = FromUnsigned(f, mag);
  return bits | SignBit(f);
}

static int64_t ToSigned(Fmt f, uint64_t bits) {
  Parts p = Unpack(f, bits);
  if (p.cls == kClsNan || p.cls == kClsZero) {
    return 0;
  }
  if (p.cls == kClsInf || p.exp >= 63) {
    return p.sign ? INT64_MIN : INT64_MAX;
  }
  if (p.exp < 0) {
    return 0;
  }
  uint64_t mag;
  if (p.exp >= f.mant) {
    mag = p.sig << (p.exp - f.mant);
  } else {
    mag = p.sig >> (f.mant - p.exp);
  }
  if (p.sign) {
    if (mag > (uint64_t)INT64_MAX) {
      return INT64_MIN;
    }
    return -(int64_t)mag;
  }
  if (mag > (uint64_t)INT64_MAX) {
    return INT64_MAX;
  }
  return (int64_t)mag;
}

static uint64_t Widen(uint32_t bits) {
  Fmt src = Fmt32();
  Fmt dst = Fmt64();
  Parts p = Unpack(src, bits);
  if (p.cls == kClsNan) {
    return CanonicalNan(dst);
  }
  if (p.cls == kClsInf) {
    return InfBits(dst, p.sign);
  }
  if (p.cls == kClsZero) {
    return p.sign ? SignBit(dst) : 0;
  }
  // Hidden bit moves from the float's mant+3 slot up to the double's.
  return RoundPack(dst, p.sign, p.exp + (dst.mant - src.mant), p.sig << 3);
}

static uint32_t Narrow(uint64_t bits) {
  Fmt src = Fmt64();
  Fmt dst = Fmt32();
  Parts p = Unpack(src, bits);
  if (p.cls == kClsNan) {
    return (uint32_t)CanonicalNan(dst);
  }
  if (p.cls == kClsInf) {
    return (uint32_t)InfBits(dst, p.sign);
  }
  if (p.cls == kClsZero) {
    return p.sign ? (uint32_t)SignBit(dst) : 0;
  }
  int sh = src.mant - (dst.mant + 3);
  uint64_t lost = p.sig & ((1ULL << sh) - 1);
  uint64_t sig = p.sig >> sh;
  if (lost) {
    sig |= 1;
  }
  return (uint32_t)RoundPack(dst, p.sign, p.exp, sig);
}

uint32_t __bpf_addsf3(uint32_t a, uint32_t b) {
  return (uint32_t)Add(Fmt32(), a, b);
}
uint32_t __bpf_subsf3(uint32_t a, uint32_t b) {
  // Flipping the sign of a NaN would not match subtraction.
  if ((a << 1) > 0xff000000u) {
    return a | 0x00400000u;
  }
  if ((b << 1) > 0xff000000u) {
    return b | 0x00400000u;
  }
  return (uint32_t)Add(Fmt32(), a, b ^ 0x80000000u);
}
uint32_t __bpf_mulsf3(uint32_t a, uint32_t b) {
  return (uint32_t)Mul(Fmt32(), a, b);
}
uint32_t __bpf_divsf3(uint32_t a, uint32_t b) {
  return (uint32_t)Div(Fmt32(), a, b);
}
int64_t __bpf_cmpsf(uint32_t a, uint32_t b) {
  return Compare(Fmt32(), a, b);
}
uint32_t __bpf_i64_to_sf(int64_t x) {
  return (uint32_t)FromSigned(Fmt32(), x);
}
uint32_t __bpf_u64_to_sf(uint64_t x) {
  return (uint32_t)FromUnsigned(Fmt32(), x);
}
int64_t __bpf_sf_to_i64(uint32_t x) { return ToSigned(Fmt32(), x); }

uint64_t __bpf_adddf3(uint64_t a, uint64_t b) { return Add(Fmt64(), a, b); }
uint64_t __bpf_subdf3(uint64_t a, uint64_t b) {
  if ((a << 1) > 0xffe0000000000000ULL) {
    return a | (1ULL << 51);
  }
  if ((b << 1) > 0xffe0000000000000ULL) {
    return b | (1ULL << 51);
  }
  return Add(Fmt64(), a, b ^ (1ULL << 63));
}
uint64_t __bpf_muldf3(uint64_t a, uint64_t b) { return Mul(Fmt64(), a, b); }
uint64_t __bpf_divdf3(uint64_t a, uint64_t b) { return Div(Fmt64(), a, b); }
int64_t __bpf_cmpdf(uint64_t a, uint64_t b) { return Compare(Fmt64(), a, b); }
uint64_t __bpf_i64_to_df(int64_t x) { return FromSigned(Fmt64(), x); }
uint64_t __bpf_u64_to_df(uint64_t x) { return FromUnsigned(Fmt64(), x); }
int64_t __bpf_df_to_i64(uint64_t x) { return ToSigned(Fmt64(), x); }
uint64_t __bpf_sf_to_df(uint32_t x) { return Widen(x); }
uint32_t __bpf_df_to_sf(uint64_t x) { return Narrow(x); }

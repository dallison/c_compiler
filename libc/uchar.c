#include <uchar.h>

#include <errno.h>

static mbstate_t internal_c16_input_state;
static mbstate_t internal_c16_output_state;
static mbstate_t internal_c32_input_state;
static mbstate_t internal_c32_output_state;

static void reset_state(mbstate_t* state) {
  state->__value = 0;
  state->__pending = 0;
  state->__count = 0;
  state->__expected = 0;
  state->__has_pending = 0;
}

static size_t decode_c32(char32_t* output, const char* string, size_t count,
                         mbstate_t* state) {
  size_t consumed = 0;
  unsigned int value;
  unsigned int minimum;
  if (count == 0) return (size_t)-2;
  if (state->__count == 0) {
    unsigned char first = (unsigned char)string[consumed++];
    if (first < 0x80) {
      if (output != NULL) *output = (char32_t)first;
      return first == 0 ? 0 : 1;
    }
    if (first >= 0xc2 && first <= 0xdf) {
      state->__value = first & 0x1f;
      state->__expected = 2;
    } else if (first >= 0xe0 && first <= 0xef) {
      state->__value = first & 0x0f;
      state->__expected = 3;
    } else if (first >= 0xf0 && first <= 0xf4) {
      state->__value = first & 0x07;
      state->__expected = 4;
    } else {
      errno = EILSEQ;
      reset_state(state);
      return (size_t)-1;
    }
    state->__count = 1;
  }
  while (state->__count < state->__expected && consumed < count) {
    unsigned char next = (unsigned char)string[consumed];
    if ((next & 0xc0) != 0x80) {
      errno = EILSEQ;
      reset_state(state);
      return (size_t)-1;
    }
    state->__value = (state->__value << 6) | (next & 0x3f);
    state->__count++;
    consumed++;
  }
  if (state->__count < state->__expected) return (size_t)-2;
  value = state->__value;
  minimum = state->__expected == 2 ? 0x80
            : state->__expected == 3 ? 0x800
                                     : 0x10000;
  if (value < minimum || value > 0x10ffff ||
      (value >= 0xd800 && value <= 0xdfff)) {
    errno = EILSEQ;
    reset_state(state);
    return (size_t)-1;
  }
  reset_state(state);
  if (output != NULL) *output = (char32_t)value;
  return consumed;
}

size_t mbrtoc32(char32_t* restrict output, const char* restrict string,
                size_t count, mbstate_t* restrict state) {
  if (state == NULL) state = &internal_c32_input_state;
  if (string == NULL) {
    reset_state(state);
    return 0;
  }
  return decode_c32(output, string, count, state);
}

size_t c32rtomb(char* restrict output, char32_t character,
                mbstate_t* restrict state) {
  unsigned int value = (unsigned int)character;
  if (state == NULL) state = &internal_c32_output_state;
  if (output == NULL) {
    reset_state(state);
    return 1;
  }
  reset_state(state);
  if (value > 0x10ffff || (value >= 0xd800 && value <= 0xdfff)) {
    errno = EILSEQ;
    return (size_t)-1;
  }
  if (value < 0x80) {
    output[0] = (char)value;
    return 1;
  }
  if (value < 0x800) {
    output[0] = (char)(0xc0 | (value >> 6));
    output[1] = (char)(0x80 | (value & 0x3f));
    return 2;
  }
  if (value < 0x10000) {
    output[0] = (char)(0xe0 | (value >> 12));
    output[1] = (char)(0x80 | ((value >> 6) & 0x3f));
    output[2] = (char)(0x80 | (value & 0x3f));
    return 3;
  }
  output[0] = (char)(0xf0 | (value >> 18));
  output[1] = (char)(0x80 | ((value >> 12) & 0x3f));
  output[2] = (char)(0x80 | ((value >> 6) & 0x3f));
  output[3] = (char)(0x80 | (value & 0x3f));
  return 4;
}

size_t mbrtoc16(char16_t* restrict output, const char* restrict string,
                size_t count, mbstate_t* restrict state) {
  char32_t value;
  size_t result;
  if (state == NULL) state = &internal_c16_input_state;
  if (state->__has_pending) {
    if (output != NULL) *output = (char16_t)state->__pending;
    state->__pending = 0;
    state->__has_pending = 0;
    return (size_t)-3;
  }
  if (string == NULL) {
    reset_state(state);
    return 0;
  }
  result = decode_c32(&value, string, count, state);
  if (result == (size_t)-1 || result == (size_t)-2 || value <= 0xffff) {
    if (output != NULL && result != (size_t)-1 && result != (size_t)-2) {
      *output = (char16_t)value;
    }
    return result;
  }
  value -= 0x10000;
  if (output != NULL) *output = (char16_t)(0xd800 + (value >> 10));
  state->__pending = (unsigned short)(0xdc00 + (value & 0x3ff));
  state->__has_pending = 1;
  return result;
}

size_t c16rtomb(char* restrict output, char16_t character,
                mbstate_t* restrict state) {
  unsigned int value = (unsigned int)character;
  if (state == NULL) state = &internal_c16_output_state;
  if (output == NULL) {
    reset_state(state);
    return 1;
  }
  if (state->__has_pending) {
    unsigned int high = state->__pending;
    reset_state(state);
    if (value < 0xdc00 || value > 0xdfff) {
      errno = EILSEQ;
      return (size_t)-1;
    }
    value = 0x10000 + ((high - 0xd800) << 10) + (value - 0xdc00);
    return c32rtomb(output, (char32_t)value, NULL);
  }
  if (value >= 0xd800 && value <= 0xdbff) {
    state->__pending = (unsigned short)value;
    state->__has_pending = 1;
    return 0;
  }
  if (value >= 0xdc00 && value <= 0xdfff) {
    errno = EILSEQ;
    return (size_t)-1;
  }
  return c32rtomb(output, (char32_t)value, NULL);
}

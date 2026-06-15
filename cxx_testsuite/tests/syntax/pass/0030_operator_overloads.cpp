// RUN: -std=c++17
struct MemberNumber {
  int operator+(int value);
  int operator-(int value);
  int operator-(void);
  int operator~(void);
  int operator==(int value);
  int operator<<(int value);
};

int MemberNumber::operator+(int value) {
  return value;
}

int MemberNumber::operator-(int value) {
  return value;
}

int MemberNumber::operator-(void) {
  return 6;
}

int MemberNumber::operator~(void) {
  return 7;
}

int MemberNumber::operator==(int value) {
  return value;
}

int MemberNumber::operator<<(int value) {
  return value;
}

struct FreeNumber {
  int value;
};

int operator+(FreeNumber left, FreeNumber right) {
  return 2;
}

int operator*(FreeNumber left, FreeNumber right) {
  return 3;
}

int operator<(FreeNumber left, FreeNumber right) {
  return 4;
}

int operator|(FreeNumber left, FreeNumber right) {
  return 5;
}

int operator!(FreeNumber value) {
  return 8;
}

int main(void) {
  MemberNumber member_left;
  MemberNumber member_right;
  FreeNumber free_left;
  FreeNumber free_right;
  int member_sum = member_left + 1;
  int member_diff = member_left - 1;
  int member_equal = member_left == 1;
  int member_shift = member_left << 1;
  int member_neg = -member_left;
  int member_ones = ~member_left;
  int free_sum = free_left + free_right;
  int free_product = free_left * free_right;
  int free_less = free_left < free_right;
  int free_or = free_left | free_right;
  int free_not = !free_left;
  return member_sum + member_diff + member_equal + member_shift + member_neg +
         member_ones + free_sum + free_product + free_less + free_or + free_not;
}

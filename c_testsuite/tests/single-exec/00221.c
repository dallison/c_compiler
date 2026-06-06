// Exercises the math.h functions implemented from first principles:
// sqrt, fabs, ldexp, tan, atan, asin, acos, atan2 (plus sin/cos).
// Each result is checked against a reference value within a relative
// tolerance and reported as "<name> ok" so the test is robust to last-bit
// rounding differences between targets.

#include <math.h>
#include <stdio.h>

static int fails = 0;

static void check(const char* name, double got, double want) {
   double d = got - want;
   if (d < 0) d = -d;
   double scale = want;
   if (scale < 0) scale = -scale;
   if (scale < 1.0) scale = 1.0;
   if (d <= 1e-6 * scale) {
      printf("%s ok\n", name);
   } else {
      printf("%s FAIL got %.10f want %.10f\n", name, got, want);
      fails++;
   }
}

int main(void) {
   check("sqrt2", sqrt(2.0), 1.4142135623730951);
   check("sqrt16", sqrt(16.0), 4.0);
   check("sqrt_quarter", sqrt(0.25), 0.5);
   check("sqrt_big", sqrt(1000000.0), 1000.0);

   check("fabs_neg", fabs(-3.5), 3.5);

   check("ldexp_pos", ldexp(1.5, 4), 24.0);
   check("ldexp_neg", ldexp(48.0, -3), 6.0);

   check("tan_half", tan(0.5), 0.5463024898437905);
   check("tan_one", tan(1.0), 1.5574077246549023);
   check("tan_neg", tan(-0.3), -0.30933624960962325);

   check("atan_one", atan(1.0), 0.7853981633974483);
   check("atan_half", atan(0.5), 0.4636476090008061);
   check("atan_three", atan(3.0), 1.2490457723982544);
   check("atan_neg", atan(-2.0), -1.1071487177940904);

   check("asin_half", asin(0.5), 0.5235987755982989);
   check("asin_one", asin(1.0), 1.5707963267948966);
   check("asin_pt8", asin(0.8), 0.9272952180016122);
   check("asin_neg", asin(-0.3), -0.3046926540153975);

   check("acos_half", acos(0.5), 1.0471975511965979);
   check("acos_one", acos(1.0), 0.0);
   check("acos_zero", acos(0.0), 1.5707963267948966);
   check("acos_neg", acos(-0.6), 2.214297435588181);

   check("atan2_q1", atan2(1.0, 1.0), 0.7853981633974483);
   check("atan2_q2", atan2(1.0, -1.0), 2.356194490192345);
   check("atan2_q3", atan2(-1.0, -1.0), -2.356194490192345);
   check("atan2_yaxis", atan2(1.0, 0.0), 1.5707963267948966);

   check("sin_one", sin(1.0), 0.8414709848078965);
   check("cos_one", cos(1.0), 0.5403023058681398);

   if (fails == 0) {
      printf("all ok\n");
   }
   return fails == 0 ? 0 : 1;
}

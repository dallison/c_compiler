static double g(double x){ return x*x + 1.0; }
int main(void){ double s=0; for(int i=0;i<5;i++) s+=g((double)i); return (int)s; }

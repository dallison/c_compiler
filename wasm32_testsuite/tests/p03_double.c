double area(double r){ return 3.14159 * r * r; }
int main(void){ double a = area(4.0); float f = (float)a / 2.0f; return (int)a % 100 + (int)f % 10; }

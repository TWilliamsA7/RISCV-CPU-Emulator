// test.c

int main() {
    int a = 12;
    int b = 14;

    int c = a * b;

    for (int i = 0; i < 3; i++) {
        c /= 2;
    }

    return c + (b % a);
}
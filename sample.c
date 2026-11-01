int main()
{
    int dec = 10;          // decimal
    int hex = 0x1A;        // hexadecimal
    float pi = 3.14;       // float
    char ch = 'x';         // character
    const char *msg = "Hello"; // string

    if (dec != hex && pi > 2.0)
    {
        dec += 5;
    }
    else
    {
        dec -= 3;
    }

    for (int i = 0; i < 3; i++)
    {
        printf("%s %d %c %.2f\n", msg, dec, ch, pi);
    }

    return 0;
}

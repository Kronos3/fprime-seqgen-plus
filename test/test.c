void main(F32 argument, I32 arg2, I32 arg3)
{
    print("%f", argument);
    print("%f", argument);
    
    for (I32 i = 0; i < 4; i++)
    {
        print("%d", i * 1 + 2 * my_function(i / 2));
        while (1)
        {
            F32 j = 0;
            j = j + 1;
            break;
        }

        if (i > 2)
        {
            i = i + 2;
        }

        I32 k;
        if (i < 3)
        {
            k = i + 7;
        }
        else if (i < 4)
        {
            k = 0;
        }
        else
        {
            k = i - 2;
        }
    }
}

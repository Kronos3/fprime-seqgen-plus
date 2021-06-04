void print(char lit, i32 a);
void my_function(f32 a);

struct MyType
{
    f32 field_1;
    i32 field_2;
};

void main(f32 argument, i32 arg2, i32 arg3)
{
    print("%f", argument);
    print("%f", argument);

    MyType**** ** * declare;
    
    for (i32 i = 0; i < 4; i++)
    {
        print("%d", i * 1 + 2 * my_function(i / 2));
        while (1)
        {
            i32 j = 0;
            j = j + 1;
            ++j++;
            break;
        }

        if (i > 2)
        {
            i = i + 2;
        }

        i32 k;
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

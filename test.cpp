int foo()
{
    int arr_raw[100];
    int *arr = arr_raw;
    for (int i = 0; i < 10; i++)
    {
        {
            arr[i] = 0;
        }
    }
}

int bar()
{
    int arr_raw[100];
    int *arr = arr_raw;

    for (int i = 0; i < 10; i++)
    {
        auto task = [=]()
        {
            arr[i] = 0;
        };
        task();
    }
}
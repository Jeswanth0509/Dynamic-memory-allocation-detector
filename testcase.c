#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* =========================================================
   VALID MALLOC + FREE
   ========================================================= */

void valid_malloc_free()
{
    int *a = (int *)malloc(7 * sizeof(int));

    if (a != NULL)
    {
        a[0] = 10;
        free(a);
    }
}

/* =========================================================
   MEMORY LEAK
   ========================================================= */

void memory_leak()
{
    int *b = (int *)malloc(10 * sizeof(int));

    if (b != NULL)
    {
        b[0] = 100;
    }

    /* intentionally leaked */
}

/* =========================================================
   PARTIAL LEAK
   ========================================================= */

void partial_leak()
{
    int *c = (int *)malloc(4 * sizeof(int));

    int *d = (int *)malloc(8 * sizeof(int));

    free(c);

    /* d intentionally leaked */
}

/* =========================================================
   CALLOC TEST
   ========================================================= */

void calloc_test()
{
    int *arr =
        (int *)calloc(5, sizeof(int));

    if (arr != NULL)
    {
        arr[2] = 50;

        free(arr);
    }
}

/* =========================================================
   CALLOC LEAK
   ========================================================= */

void calloc_leak()
{
    int *arr =
        (int *)calloc(20, sizeof(int));

    if (arr != NULL)
    {
        arr[0] = 1;
    }

    /* intentionally leaked */
}

/* =========================================================
   REALLOC TEST
   ========================================================= */

void realloc_test()
{
    int *arr =
        (int *)malloc(5 * sizeof(int));

    if (arr == NULL)
        return;

    arr = (int *)realloc(
        arr,
        20 * sizeof(int));

    if (arr != NULL)
    {
        arr[10] = 500;

        free(arr);
    }
}

/* =========================================================
   REALLOC LEAK
   ========================================================= */

void realloc_leak()
{
    char *buffer =
        (char *)malloc(16);

    if (buffer == NULL)
        return;

    buffer = (char *)realloc(
        buffer,
        64);

    if (buffer != NULL)
    {
        strcpy(buffer, "PIN TOOL TEST");
    }

    /* intentionally leaked */
}

/* =========================================================
   INVALID FREE
   ========================================================= */

void invalid_free_test()
{
    int x = 10;

    /*
       stack memory
       not heap memory
    */

    free(&x);
}

/* =========================================================
   DOUBLE FREE
   ========================================================= */

void double_free_test()
{
    int *e =
        (int *)malloc(6 * sizeof(int));

    free(e);

    /*
       second free
    */

    free(e);
}

/* =========================================================
   MAIN
   ========================================================= */

int main()
{
    printf("===== TEST PROGRAM STARTED =====\n");

    valid_malloc_free();

    memory_leak();

    partial_leak();

    calloc_test();

    calloc_leak();

    realloc_test();

    realloc_leak();

    double_free_test();

    invalid_free_test();

    printf("===== TEST PROGRAM FINISHED =====\n");

    return 0;
}
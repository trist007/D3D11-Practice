/* date = March 25th 2026 7:41 pm */

#ifndef SHARED_H
#define SHARED_H

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

float
rand_float()
{
    return((float)rand() / (float)RAND_MAX);
}

struct Timer
{
    LARGE_INTEGER start;
    LARGE_INTEGER frequency;
};

void
TimerInit(Timer *t)
{
    QueryPerformanceFrequency(&t->frequency);
    QueryPerformanceCounter(&t->start);
}

float
TimerPeek(Timer *t)
{
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return((float)(now.QuadPart - t->start.QuadPart) / (float)t->frequency.QuadPart);
}



#endif //SHARED_H

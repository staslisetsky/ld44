#include <stdio.h>
#include <stdint.h>
#include <malloc.h>
#include <math.h>
#include <emscripten/emscripten.h>

#define UINT_MAX 400000000

extern "C" {
    void * EMSCRIPTEN_KEEPALIVE BactorialInitWorld();
    void EMSCRIPTEN_KEEPALIVE BactorialUpdateWorld(float dt);
    void EMSCRIPTEN_KEEPALIVE BactorialSelect(float minx, float miny, float maxx, float maxy);
    void EMSCRIPTEN_KEEPALIVE BactorialDivide();
}

#pragma pack(push,8)  
#include "bactorial.h"
#pragma pack(pop)

#include "bactorial.cpp"

int
main()
{
    printf("Hello world");
    return 0;
}

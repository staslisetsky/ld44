#include <stdio.h>
#include <stdint.h>
#include <malloc.h>
#include <math.h>
#include <emscripten/emscripten.h>

#define UINT_MAX 400000000

extern "C" {
    void * EMSCRIPTEN_KEEPALIVE BactorialInitWorld(int Count, float Radius, float Distribution);
    void EMSCRIPTEN_KEEPALIVE BactorialUpdateWorld(float dt);
    int EMSCRIPTEN_KEEPALIVE BactorialSelect(float minx, float miny, float maxx, float maxy);
    void EMSCRIPTEN_KEEPALIVE BactorialDivide();
    void EMSCRIPTEN_KEEPALIVE BactorialUnselect();
    void EMSCRIPTEN_KEEPALIVE BactorialSpawnEnemy(float Distance, float Radius, float velocityx, float velocityy);
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

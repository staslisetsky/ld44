#include <stdio.h>
#include <math.h>
#include <emscripten/emscripten.h>

extern "C" {
    void * EMSCRIPTEN_KEEPALIVE initState();
    void EMSCRIPTEN_KEEPALIVE updateWorld(float dt);
}

world_state World = {};

void *
initState() {
    World.ObjectCount = 2;
    World.Objects = Objects;
    return &World;
}

void 
updateWorld(float dt) {
    Objects[0].x = sin(t / 200.0f) * 250.0f + 250.0f;
    Objects[1].x = sin(t / 50.0f) * 30.0f + 200.0f;
    Objects[1].y = cos(t / 50.0f) * 30.0f + 200.0f;

    t += dt;
}

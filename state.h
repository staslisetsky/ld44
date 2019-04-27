struct object {
    int x;
    int y;
    int w;
    int h;
    // v4 Color;
};

struct world_state {
    int ObjectCount;
    object *Objects;
};

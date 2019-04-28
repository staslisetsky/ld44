#define Assert(expr) if(!(expr)) { *(int *)0 = 0;} 

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef s32 b32;
typedef char utf8;

typedef float r32;
typedef double r64;

#define Kilobytes(value) (1024 * value)
#define Megabytes(value) (Kilobytes(1024 * value))
#define Gigabytes(value) (Megabytes(1024 * value))
#define Terabytes(value) (Gigabytes(1024 * value))

#include "cascade_math.h"
#include "cascade_math.cpp"

#define MAX_NODE_COUNT 100000

struct quad_tree_node {
    rect Rect;
    u32 Index;
    v2 P;
    // r32 Charge;
    b32 Subdivided;
    b32 HasObjects;
    r32 CellSize;

    quad_tree_node *Sub[4];
};

struct quad_tree {
    quad_tree_node *Root;
    u32 NodeCount;
};

enum mode_ {
    Mode_Idle,
    Mode_Mitosis,
    Mode_Attack,

    Mode_Count,
};


enum enemy_mode_ {
    EnemyMode_Approach,
    EnemyMode_Orbit,
    EnemyMode_Attack,
};

struct enemy {
    u32 Id;
    enemy_mode_ Mode;

    r32 AttackTimer;
    u32 TargetIndex;

    v2 P;
    v2 Velocity;
    u32 Radius;
    r32 Soul;
};

static u32 GlobalEnemyId = 0;

struct object {
    u32 EnemyId;

    v2 P;
    v2 Velocity;
    v2 Velocity2;
    u32 Radius;
    b32 Selected;

    mode_ Mode;
    r32 MitosisProgress;

    char *Name;
    r32 Soul;
};


struct world {
    u32 ObjectCount;
    v2 *Positions;
    v2 *Velocities;
    r32 *Radii;

    u32 Fattest[10];

    rect BoundingRect;

// internal data:
    u32 SelectedIndex;
    u32 SelectedCount;

    v2 SelectionStart;
    rect SelectionRect;
    b32 SelectionMode;

    object *Objects;
    enemy *Enemies;
    u32 EnemyCount;

    v2 AttractorP;
    b32 AttractorActive;
    quad_tree Tree;
    r32 Time;
};

static world World = {};


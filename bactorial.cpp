#include "include/pcg_basic.h"
#include "include/pcg_basic.c"

#define MAX_OBJECTS 100000
#define MAX_ENEMIES 1000
#define MITOSIS_TIME 1.0f
#define START_R 50

static pcg32_random_t Rng;

r32 
RandomN()
{
    return (r32)(pcg32_random_r(&Rng) % 100) / 100.0f;
}

u32
Random32()
{
    return pcg32_random_r(&Rng);
}

void
BactorialSpawnEnemy(float Distance, float Radius, float velocityx, float velocityy)
{
    enemy New = {};

    v2 RectCenter = World.BoundingRect.Min + (World.BoundingRect.Max - World.BoundingRect.Min) / 2.0f;
    r32 R = Length(World.BoundingRect.Max - World.BoundingRect.Min) / 2.0f;

    r32 Theta = RandomN() * PI * 2.0f;
    v2 Direction = {(r32)sin(Theta), (r32)cos(Theta)};

    New.Radius = Radius;
    New.P = Direction * (Distance + R);
    New.Velocity = {velocityx, velocityy};
    New.Id = GlobalEnemyId++;

    World.Enemies[World.EnemyCount++] = New;
#if WASM
    EM_ASM({
      console.log('Enemy spawned at p: ' + [$0, $1]);
    }, New.P.x, New.P.y);
#endif
}

v2
RandomDirection() {
    r32 Theta = RandomN() * PI * 2.0f;
    return {(r32)sin(Theta), (r32)cos(Theta)};    
}

void
DivideCell(object *Object)
{
    if (Object->Radius > 2.0f) {
        object New = {};
        u32 NewR = sqrt(Object->Radius * Object->Radius / 2.0f); 

        v2 DivideDirection = Object->DirectionChoice;

        Object->Radius = NewR;
        New.Radius = NewR;

        New.P = Object->P + DivideDirection * (r32)NewR;
        New.DirectionChoice = RandomDirection();

        Object->P = Object->P - DivideDirection * (r32)NewR;
        Object->Status = Status_Idle;
        Object->DirectionChoice = RandomDirection();

        Object->Velocity = DivideDirection * -7.0f;
        New.Velocity = DivideDirection * 7.0f;

        New.Soul = RandomN();

        World.Objects[World.ObjectCount++] = New;
    }
}

void
BuildQuadTree(quad_tree *Tree, quad_tree_node *Node, rect QRect, object *Objects, u32 Count, u32 MaxNodeCount, u32 ArrayOffset=0)
{
    if (!Tree->NodeCount) {
        Tree->NodeCount++;
    }

    rect SubRects[4];
    
    r32 SubW = (QRect.Max.x - QRect.Min.x) / 2.0f;
    r32 SubH = (QRect.Max.y - QRect.Min.y) / 2.0f;

    SubRects[0] = RectLeftTopWidthHeight(QRect.Min.x, QRect.Min.y, SubW, SubH);
    SubRects[1] = RectLeftTopWidthHeight(QRect.Min.x + SubW, QRect.Min.y, SubW, SubH);
    SubRects[2] = RectLeftTopWidthHeight(QRect.Min.x + SubW, QRect.Min.y + SubH, SubW, SubH);
    SubRects[3] = RectLeftTopWidthHeight(QRect.Min.x, QRect.Min.y + SubH, SubW, SubH);

    u32 ObjectIndex = 0;
    u32 BinCount = 0;

    u32 TotalCount = 0;

    for (u32 j=0; j<4; ++j) {
        for (u32 i=ObjectIndex; i<Count; ++i) {
            if (InRect(SubRects[j], Objects[i].P)) {
                object Tmp = Objects[BinCount + ObjectIndex];
                Objects[BinCount + ObjectIndex] = Objects[i];
                Objects[i] = Tmp;
                ++BinCount;
                ++TotalCount;
            }
        }

        Assert(Tree->NodeCount + 4 < MaxNodeCount);

        Node->Sub[j] = Tree->Root + Tree->NodeCount++;
        *Node->Sub[j] = {};

        Node->Sub[j]->Rect = SubRects[j];

        if (BinCount > 1) {
            BuildQuadTree(Tree, Node->Sub[j], SubRects[j], Objects + ObjectIndex, BinCount, MaxNodeCount, ArrayOffset + ObjectIndex);
        } else if (BinCount == 1) {
            Node->Sub[j]->HasObjects = true;
            Node->Sub[j]->Index = ArrayOffset + ObjectIndex;
        }

        Node->Sub[j]->CellSize = SubRects[j].Max.x - SubRects[j].Min.x;

        ObjectIndex += BinCount;
        BinCount = 0;
    }

    if (Count > 0) {
        Node->HasObjects = true;
    }

    if (Count > 1) {
        Node->Subdivided = true;
    }

    Node->CellSize = QRect.Max.x - QRect.Min.x;
}

void
BactorialStabilizeColony()
{
    if (World.ObjectCount > 1) {
        for (u32 i=0; i<World.ObjectCount; ++i) {
            object *Me = World.Objects + i;

            if (Me->Status == Status_Attack) {
                continue;
            }

            v2 Velocity = {};
            for (u32 j=0; j<World.ObjectCount; ++j) {
                if (j == i) {
                    continue;
                }

                object *They = World.Objects + j;
                r32 Distance = Length(Me->P - They->P) - (Me->Radius + They->Radius);

                if (Distance == 0.0) {
                    Distance = 0.000001;
                }

                v2 Direction = (Me->P - They->P) / (Distance * Distance);

                if (Distance > 0.0f) {
                    Velocity += Direction * (1.0 / Distance) * 10.0f;

                    if (Distance > 3.0f) {
                        r32 AttractionFactor = (Distance - 3 + 1);
                        AttractionFactor *= AttractionFactor;
                        Velocity -= Direction * AttractionFactor;
                    }
                } else {
                    Velocity += Direction * 10000000.0f * Abs(Distance * Distance * Distance * Distance * Distance);
                }
            }

            r32 Value  = Max_r32(0.01f, Length(Velocity));
            v2 Direction = Velocity / Value;

            Velocity = Direction * Min_r32(Value, 100.0f);
            Me->Velocity2 = Velocity / r32(World.ObjectCount - 1);
        }
    }
}

void
DestroyEnemy(u32 Index)
{
    if (Index < World.EnemyCount - 1) {
        World.Enemies[Index] = World.Enemies[World.EnemyCount - 1];
    }
    --World.EnemyCount;
}

void
DestroyObject(u32 Index)
{
    if (Index < World.ObjectCount - 1) {
        World.Objects[Index] = World.Objects[World.ObjectCount - 1];
    }
    --World.ObjectCount;
}

enemy *
FindEnemy(u32 Id)
{
    for (u32 i=0; i<World.EnemyCount; ++i) {
        if (World.Enemies[i].Id == Id) {
            return World.Enemies + i;
        }
    }

    return 0;
}


u8 
MakeStateVar(b32 Good, b32 Selected, status_ Status)
{
    u8 Value = (Good << 7) |
             (Selected << 6) |
             Status;

    return Value;
}

void 
BactorialUpdateWorld(float dt) 
{
    rect Screen = {{0.0f, 0.0f}, {500.0f, 500.0f}};

    r32 MaxDistance = 0.0f;

    // World.AttractorActive = true;

    World.BoundingRect = {{9999999.0f, 9999999.0f},{-9999999.0f, -9999999.0f}};

    BactorialStabilizeColony();

    World.DataCount = World.ObjectCount + World.EnemyCount;

    r32 LastFrameDeadRadius = World.DeadRadius;
    World.DeadRadius = 0.0f;

    if (World.FloatProgress <= 0.0f) {
        World.FloatDirection = World.NextDirection;
        World.FloatProgress = 1.0f;
        World.FloatTime = (RandomN() * 4.0f + 1);
        World.NextDirection = RandomDirection();
    }

    World.FloatProgress -= dt / World.FloatTime;

    for (s32 i=0; i<World.ObjectCount; ++i) {
        object *Object = World.Objects + i;

        if (LastFrameDeadRadius > 0.0f) {
            Object->Radius += LastFrameDeadRadius / World.ObjectCount;
        }

        b32 Dead = false;
        for (u32 j=0; j<World.EnemyCount; ++j) {
            enemy *Enemy = World.Enemies + j;

            if (Enemy->Status == Status_Dead) {
                continue;
            }

            r32 Distance  = Max_r32(Length(Object->P - Enemy->P) - (Enemy->Radius + Object->Radius), 0.0f);
            if (Distance < 1.0f) {
                Enemy->Status = Status_Dead;
                Object->Status = Status_Dead;
                Dead = true;
                World.DeadRadius += Enemy->Radius;
                break;
            }
        }

        if (Object->Status == Status_Mitosis) {
            Object->MitosisProgress += dt / MITOSIS_TIME;

            if (Object->MitosisProgress >= 1.0f) {
                DivideCell(Object);
            }
        } else if (Object->Status == Status_Attack) {
            enemy *Enemy = FindEnemy(Object->EnemyId);
            if (Enemy) {
                v2 ToEnemy = Enemy->P - Object->P;
                v2 Direction = ToEnemy / Length(ToEnemy);
                Object->Velocity = Direction * 100.0f;
            } else {
                Object->Status = Status_Idle;
            }
        } else if (Object->Status == Status_Idle) {

        }

        v2 Slowdown = World.Objects[i].Velocity * -1.0f * 0.02f;
        World.Objects[i].Velocity += Slowdown;

        v2 DriftVelocity = {};
        r32 RandomRange = (Object->Soul - 0.5f) * 2.0f;
        DriftVelocity.x = sin((World.Time) / 3.0f * RandomRange) * 3.0f * RandomRange;
        DriftVelocity.y = cos((World.Time) / 3.0f * RandomRange) * 3.0f * RandomRange;

        DriftVelocity = DriftVelocity + Lerp(World.NextDirection, World.FloatDirection, Clamp(0.0f, World.FloatProgress, 1.0f)) * 3.0f;

        v2 SummedVelocity = (World.Objects[i].Velocity + World.Objects[i].Velocity2) + DriftVelocity;

        World.Objects[i].P = Object->P + SummedVelocity * dt;

        World.BoundingRect.Min.x = Min_r32(World.BoundingRect.Min.x, Object->P.x - Object->Radius);
        World.BoundingRect.Min.y = Min_r32(World.BoundingRect.Min.y, Object->P.y - Object->Radius);
        World.BoundingRect.Max.x = Max_r32(World.BoundingRect.Max.x, Object->P.x + Object->Radius);
        World.BoundingRect.Max.y = Max_r32(World.BoundingRect.Max.y, Object->P.y + Object->Radius);

        // js export:
        World.Positions[i] = Object->P;
        World.Velocities[i] = SummedVelocity;
        World.Radii[i] = Object->Radius;
        World.States[i] = MakeStateVar(1, Object->Selected, Object->Status);
        World.Seeds[i] = Object->DirectionChoice;

        if (Object->Status == Status_Dead) {
            DestroyObject(i);
            --i;
            continue;
        }
    }

    v2 RectCenter = World.BoundingRect.Min + (World.BoundingRect.Max - World.BoundingRect.Min) / 2.0f;
    r32 ApproachRadius = Length(World.BoundingRect.Max - World.BoundingRect.Min) / 2.0f + 50.0f;

    for (s32 i=0; i<World.EnemyCount; ++i) {
        enemy *Enemy = World.Enemies + i;

        Enemy->TargetIndex = 0;

        object *Object = World.Objects + Enemy->TargetIndex;
        v2 ToTarget = Object->P - Enemy->P;
        v2 Direction = ToTarget / Length(ToTarget);
        Enemy->Velocity = Direction * 1000.0 / Enemy->Radius;

        Enemy->P = Enemy->P  + Enemy->Velocity * dt;

        u32 ExportIndex= i + World.ObjectCount;
        // js export:
        World.Positions[ExportIndex] = Enemy->P;
        World.Velocities[ExportIndex] = Enemy->Velocity;
        World.Radii[ExportIndex] = Enemy->Radius;
        World.States[ExportIndex] = MakeStateVar(0, 0, Enemy->Status);
        World.Seeds[ExportIndex] = {0.0f, 0.0f};

        if (Enemy->Status == Status_Dead) {
            DestroyEnemy(i);
            --i;
            continue;
        }
    }

    World.AABB = World.BoundingRect;
    
    // World.Tree.NodeCount = 0;
    // BuildQuadTree(&World.Tree, World.Tree.Root, Screen, World.Objects, World.ObjectCount, MAX_NODE_COUNT);

    World.Time += dt;
}

void *
BactorialInitWorld(int Count, float Radius, float Distribution)
{
    pcg32_srandom_r(&Rng, 3908476239044, (int)&Rng);

    World.ObjectCount = Count;

    World.Objects = (object *)malloc(sizeof(object) * MAX_OBJECTS);
    World.Positions = (v2 *)malloc(sizeof(v2) * MAX_OBJECTS);
    World.Velocities = (v2 *)malloc(sizeof(v2) * MAX_OBJECTS);
    World.Radii = (r32 *)malloc(sizeof(r32) * MAX_OBJECTS);
    World.States = (u8 *)malloc(sizeof(u8) * MAX_OBJECTS);
    World.Seeds = (v2 *)malloc(sizeof(v2) * MAX_OBJECTS);

    World.Enemies = (enemy *)malloc(sizeof(enemy) * MAX_ENEMIES);

#if 0
    World.Objects[0].Radius = START_R;
#else
    for (u32 i=0; i<World.ObjectCount; ++i) {
        World.Objects[i].P.x = (RandomN()-0.5f) * 2.0f * Radius * Distribution;
        World.Objects[i].P.y = (RandomN()-0.5f) * 2.0f * Radius * Distribution;
        // World.Objects[i].DirectionChoice = {-0.187381238f, -0.982287288f};

        World.Objects[i].DirectionChoice = RandomDirection();
        World.Objects[i].Radius = Radius;
        World.Objects[i].Soul = RandomN();
    }
#endif

    World.NextDirection = RandomDirection();

    World.Tree.Root = (quad_tree_node *)malloc(MAX_NODE_COUNT * sizeof(quad_tree_node));
    World.NextSpawn = RandomN() * 5.0f;

    return &World;
}

void 
BactorialSelectRect(r32 X, r32 Y, r32 DimX, r32 DimY) 
{
}

b32
LocateObjectWithP(quad_tree_node *Node, rect Rect, v2 P, u32 *Result)
{
    b32 Found = false;

    if (Node->Subdivided) {
        for (u32 i=0; i<4; ++i) {
            if (InRect(Node->Sub[i]->Rect, P)) {
                Found = LocateObjectWithP(Node->Sub[i], Node->Sub[i]->Rect, P, Result);
                break;
            }
        }
    } else {
        *Result = Node->Index;
        Found = true;
    }

    return Found;
}   

rect 
GrowRect(rect Rect, u32 Value)
{
    Rect.Min = Rect.Min - V2(Value, Value);
    Rect.Max = Rect.Max + V2(Value, Value);

    return Rect;
}

b32
BactorialSelectAtP(v2 P)
{
    b32 Found = false;
    u32 Index = 0;
    r32 MinDistance = 999999999.0f;
    for (u32 i=0; i<World.ObjectCount; ++i) {
        r32 Len = Length(P - World.Objects[i].P);
        if (Len < MinDistance) {
            Index = i;
            MinDistance = Len;
        }
    }

    r32 DistanceFromSurface = Max_r32(Length(P - World.Objects[Index].P) - World.Objects[Index].Radius, 0.0f);

    if (DistanceFromSurface < 1) {
        Found = true;    
        World.SelectedCount = 1;

        for (u32 i=0; i<World.ObjectCount; ++i) {
            World.Objects[i].Selected = false;
        }

        World.Objects[Index].Selected = true;
        World.SelectedIndex = Index;
    }
    
    return Found;
}

enemy *
BactorialFindEnemy(v2 P)
{
    enemy *Result = 0;

    r32 MinDistanceSq = 999999999.0f;
    for (u32 i=0; i<World.EnemyCount; ++i) {
        r32 Lsq = LengthSq(P - World.Enemies[i].P);
        if (Lsq < MinDistanceSq) {
            MinDistanceSq = Lsq;
            Result = World.Enemies + i;
        }
    }

    if (Result) {
        r32 DistanceFromSurface = Max_r32(Length(P - Result->P) - Result->Radius, 0.0f);
        if (DistanceFromSurface > 1) {
            Result = 0;
        }
    }

    return Result;
}

void 
BactorialCommenceMitosis()
{
    if (World.SelectedCount == 1) {
        if (World.Objects[World.SelectedIndex].Status != Status_Mitosis) {
            World.Objects[World.SelectedIndex].Status = Status_Mitosis;
            World.Objects[World.SelectedIndex].MitosisProgress = 0.0f;
        }
    } if (World.SelectedCount > 1) {
        for (u32 i=0; i<World.ObjectCount; ++i) {
            if (World.Objects[i].Selected) {
                World.Objects[i].Status = Status_Mitosis;
                World.Objects[i].MitosisProgress = 0.0f;
            }
        }
    }
}

void 
BactorialAttack(enemy *Enemy)
{
    if (World.SelectedCount == 1) {
        World.Objects[World.SelectedIndex].Status = Status_Attack;
        World.Objects[World.SelectedIndex].EnemyId = Enemy->Id;
    } if (World.SelectedCount > 1) {
        for (u32 i=0; i<World.ObjectCount; ++i) {
            if (World.Objects[i].Selected) {
                World.Objects[i].Status = Status_Attack;
                World.Objects[i].EnemyId = Enemy->Id;
            }
        }
    }
}

void
BactorialDivide() {
#if WASM
    EM_ASM({
      console.log('DIVIDE!!!');
    });
#endif
    BactorialCommenceMitosis();
}

void
BactorialUnselect() {
    for (u32 i=0; i<World.ObjectCount; ++i) {
        object *Object = World.Objects + i;
        Object->Selected = false;
    }
#if WASM
    EM_ASM({
      console.log('Unselected all objects');
    });
#endif
    World.SelectedCount = 0;
}

void
BactorialAttackEnemy(float x, float y) {
#if WASM
    EM_ASM({
      console.log('Attack');
    });
#endif
    enemy *Enemy = BactorialFindEnemy(V2(x,y));
    if (Enemy) {
        BactorialAttack(Enemy);
    } else {
        BactorialUnselect();
    }
}

int
BactorialSelect(float minx, float miny, float maxx, float maxy)
{

#if WASM
    EM_ASM({
      console.log('Selecting rect: : ' + [$0, $1, $2, $3]);
    }, minx, miny, maxx, maxy);
#endif

    rect Rect = {{minx, miny}, {maxx, maxy}};

    if (Length(Rect.Max - Rect.Min) < 5.0f) {
        v2 ClickP = Rect.Min + (Rect.Max - Rect.Min);
        if (BactorialSelectAtP(ClickP)) {
            u32 a =4;
        } else if (World.SelectedCount) {
            BactorialAttackEnemy(ClickP.x, ClickP.y);
        }
    } else {
        World.SelectedCount = 0;

        for (u32 i=0; i<World.ObjectCount; ++i) {
            object *Object = World.Objects + i;
            Object->Selected = false;
            rect SelectionRect = GrowRect(Rect, Object->Radius);

            if (InRect(SelectionRect, Object->P)) {
                Object->Selected = true;
                ++World.SelectedCount;
            }
        }    
    }
#if WASM
    EM_ASM({
      console.log('Selected: ' + [$0]);
    }, World.SelectedCount);
#endif

    return World.SelectedCount;
}



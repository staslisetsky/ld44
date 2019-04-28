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
SpawnEnemy()
{
    enemy New = {};

    r32 Theta = RandomN() * PI * 2.0f;
    v2 Direction = {(r32)sin(Theta), (r32)cos(Theta)};

    New.Radius = Random32() % 40 + 10;
    New.P = Direction * 200.0f;
    New.Id = GlobalEnemyId++;

    World.Enemies[World.EnemyCount++] = New;
}

void
DivideCell(object *Object)
{
    object New = {};

    r32 Theta = RandomN() * PI * 2.0f;
    v2 Direction = {(r32)sin(Theta), (r32)cos(Theta)};

    u32 NewR = Object->Radius / 2;
    Object->Radius /= 2;
    New.Radius = NewR;

    New.P = Object->P + Direction * (r32)NewR;

    Object->P = Object->P - Direction * (r32)NewR;
    Object->Mode = Mode_Idle;

    r32 Speed = RandomN() * 5.0f;

    Object->Velocity -= Speed * Direction;
    New.Velocity += Speed * Direction;

    New.Soul = RandomN();

    World.Objects[World.ObjectCount++] = New;
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
    for (u32 i=0; i<World.ObjectCount; ++i) {
        object *Me = World.Objects + i;

        if (Me->Mode == Mode_Attack) {
            continue;
        }

        v2 Velocity = {};
        for (u32 j=0; j<World.ObjectCount; ++j) {
            if (j == i) {
                continue;
            }

            object *They = World.Objects + j;
            r32 Distance = Length(Me->P - They->P);

            Distance = Max_r32(0.001f, Distance - (Me->Radius + They->Radius));

            if (Distance > 0.0f) {
                v2 Direction = (Me->P - They->P) / (Distance * Distance);
                Velocity += Direction * (1.0 / Distance) * 10000.0f;
            }
        }

        r32 Value  = Length(Velocity);
        v2 Direction = Velocity / Value;
        Velocity = Direction * Min_r32(Value, 100.0f);

        Me->Velocity2 = Velocity / (World.ObjectCount - 1);
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

void 
BactorialUpdateWorld(float dt) 
{
    rect Screen = {{0.0f, 0.0f}, {500.0f, 500.0f}};

    r32 MaxDistance = 0.0f;

    // World.AttractorActive = true;

    World.BoundingRect = {{9999999.0f, 9999999.0f},{-9999999.0f, -9999999.0f}};

    BactorialStabilizeColony();

    for (s32 i=0; i<World.ObjectCount; ++i) {
        object *Object = World.Objects + i;

        b32 Destroyed = false;
        for (u32 j=0; j<World.EnemyCount; ++j) {
            enemy *Enemy = World.Enemies + j;

            r32 Distance  = Max_r32(Length(Object->P - Enemy->P) - (Enemy->Radius + Object->Radius), 0.0f);
            if (Distance < 1.0f) {
                DestroyEnemy(j);
                DestroyObject(i);
                Destroyed = true;
                break;
            }
        }

        if (Destroyed) {
            --i;
            break;
        }

        if (Object->Mode == Mode_Mitosis) {
            Object->MitosisProgress += dt / MITOSIS_TIME;

            if (Object->MitosisProgress >= 1.0f) {
                DivideCell(Object);
            }
        } else if (Object->Mode == Mode_Attack) {
            enemy *Enemy = FindEnemy(Object->EnemyId);
            if (Enemy) {
                v2 ToEnemy = Enemy->P - Object->P;
                v2 Direction = ToEnemy / Length(ToEnemy);
                Object->Velocity = Direction * 100.0f;
            } else {
                Object->Mode = Mode_Idle;
            }
        } else if (Object->Mode == Mode_Idle) {

        }

        // if (World.AttractorActive) {
        //     World.AttractorP = {(r32)sin(World.Time * 4.0f) * 250.0f + 200.0f, (r32)cos(World.Time * 2.0f) * 250.0f + 200.0f};

        //     r32 Distance = Length(World.AttractorP - World.Objects[i].P);
        //     v2 Direction = (World.AttractorP - World.Objects[i].P) / Distance;

        //     if (Distance > 40.0f) {
        //         World.Objects[i].Velocity += Direction * 15000.0f / Distance;
        //     } else {
        //         World.Objects[i].Velocity -= Direction * 100;
        //     }
            
        // } else {
        //     
        // }

        // r32 Len = Length(World.AttractorP - World.Objects[i].P);
        // if (Len > MaxDistance) {
        //     MaxDistance = Len;
        // }

        // v2 OscillationV = 

        // v2 Velocity = World.Objects[i].Velocity;
        // r32 DeltaX = Max_r32(Abs_r32(Velocity.x) * 0.1, 0.0f);
        // r32 DeltaY = Max_r32(Abs_r32(Velocity.x) * 0.1, 0.0f);
        // Velocity.x += Sign(Velocity.x) * -1.0f * 5.0f;

        v2 Slowdown = World.Objects[i].Velocity * -1.0f * 0.02f;
        World.Objects[i].Velocity += Slowdown;

        v2 SummedVelocity = (World.Objects[i].Velocity + World.Objects[i].Velocity2);

        World.Objects[i].P = World.Objects[i].P + SummedVelocity * dt;
        World.Positions[i] = World.Objects[i].P;
        World.Velocities[i] = SummedVelocity;
        World.Radii[i] = World.Objects[i].Radius;

        World.BoundingRect.Min.x = Min_r32(World.BoundingRect.Min.x, World.Objects[i].P.x - World.Objects[i].Radius);
        World.BoundingRect.Min.y = Min_r32(World.BoundingRect.Min.y, World.Objects[i].P.y - World.Objects[i].Radius);
        World.BoundingRect.Max.x = Max_r32(World.BoundingRect.Max.x, World.Objects[i].P.x + World.Objects[i].Radius);
        World.BoundingRect.Max.y = Max_r32(World.BoundingRect.Max.y, World.Objects[i].P.y + World.Objects[i].Radius);
    }

    v2 RectCenter = World.BoundingRect.Min + (World.BoundingRect.Max - World.BoundingRect.Min) / 2.0f;
    r32 ApproachRadius = Length(World.BoundingRect.Max - World.BoundingRect.Min) / 2.0f + 50.0f;

    for (u32 i=0; i<World.EnemyCount; ++i) {
        enemy *Enemy = World.Enemies + i;

        Enemy->TargetIndex = 0;

        r32 Distance = Length(Enemy->P - RectCenter);
        v2 Direction = (RectCenter - Enemy->P) / Distance;

        if (Enemy->Mode == EnemyMode_Approach) {
            if (Distance > ApproachRadius) {
                Enemy->Velocity = Direction * Distance / 10.0;
            } else {
                Enemy->Mode = EnemyMode_Orbit;
                Enemy->AttackTimer = RandomN() * 20.0f + 0.5f;
            }
        } else if (Enemy->Mode == EnemyMode_Orbit) {
            Enemy->Velocity = Perp((Enemy->P - RectCenter) / Distance) * 100.0f;
            Enemy->Velocity += Direction * (Distance - ApproachRadius);
            Enemy->AttackTimer = Max_r32(Enemy->AttackTimer - dt, 0.0f);

            if (Enemy->AttackTimer == 0.0f) {
                Enemy->Mode = EnemyMode_Attack;
            }
        } else if (Enemy->Mode == EnemyMode_Attack) {
            object *Object = World.Objects + Enemy->TargetIndex;
            v2 ToTarget = Object->P - Enemy->P;
            v2 Direction = ToTarget / Length(ToTarget);
            Enemy->Velocity = Direction * 1000.0 / Enemy->Radius;
        }

        Enemy->P = Enemy->P  + Enemy->Velocity * dt;
    }

    // World.Tree.NodeCount = 0;
    // BuildQuadTree(&World.Tree, World.Tree.Root, Screen, World.Objects, World.ObjectCount, MAX_NODE_COUNT);

    World.Time += dt;
}

void *
BactorialInitWorld() 
{
    pcg32_srandom_r(&Rng, 3908476239044, (int)&Rng);

    World.ObjectCount = 10;

    World.Objects = (object *)malloc(sizeof(object) * MAX_OBJECTS);
    World.Positions = (v2 *)malloc(sizeof(v2) * MAX_OBJECTS);
    World.Velocities = (v2 *)malloc(sizeof(v2) * MAX_OBJECTS);
    World.Radii = (r32 *)malloc(sizeof(r32) * MAX_OBJECTS);

    World.Enemies = (enemy *)malloc(sizeof(enemy) * MAX_ENEMIES);

#if 0
    World.Objects[0].Radius = START_R;

#else
    for (u32 i=0; i<World.ObjectCount; ++i) {
        World.Objects[i].P.x = RandomN() * 25.0f;
        World.Objects[i].P.y = RandomN() * 25.0f;
        World.Objects[i].Radius = START_R;
    }
#endif

    World.Tree.Root = (quad_tree_node *)malloc(MAX_NODE_COUNT * sizeof(quad_tree_node));

    SpawnEnemy();

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
        World.Objects[i].Selected = false;
        r32 Len = Length(P - World.Objects[i].P);
        if (Len < MinDistance) {
            Index = i;
            MinDistance = Len;
        }
    }

    r32 DistanceFromSurface = Max_r32(Length(P - World.Objects[Index].P) - World.Objects[Index].Radius, 0.0f);

    if (DistanceFromSurface < 1) {
        Found = true;    
    }

    World.Objects[Index].Selected = true;
    World.SelectedIndex = Index;
    World.SelectedCount = 1;

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
            Result = World.Enemies + i;
        }
    }

    return Result;
}

void 
BactorialCommenceMitosis()
{
    if (World.SelectedCount == 1) {
        if (World.Objects[World.SelectedIndex].Mode != Mode_Mitosis) {
            World.Objects[World.SelectedIndex].Mode = Mode_Mitosis;
            World.Objects[World.SelectedIndex].MitosisProgress = 0.0f;
        }
    } if (World.SelectedCount > 1) {
        for (u32 i=0; i<World.ObjectCount; ++i) {
            if (World.Objects[i].Selected) {
                World.Objects[i].Mode = Mode_Mitosis;
                World.Objects[i].MitosisProgress = 0.0f;
            }
        }
    }
}

void 
BactorialAttack(enemy *Enemy)
{
    if (World.SelectedCount == 1) {
        World.Objects[World.SelectedIndex].Mode = Mode_Attack;
        World.Objects[World.SelectedIndex].EnemyId = Enemy->Id;
    } if (World.SelectedCount > 1) {
        for (u32 i=0; i<World.ObjectCount; ++i) {
            if (World.Objects[i].Selected) {
                World.Objects[i].Mode = Mode_Attack;
                World.Objects[i].EnemyId = Enemy->Id;
            }
        }
    }
}

void
BactorialDivide() {
    // EM_ASM({
    //   console.log('DIVIDE!!!');
    // });
    BactorialCommenceMitosis();
}

void
BactorialAttackEnemy(float x, float y) {
    // EM_ASM({
    //   console.log('Attack!!!');
    // });
    enemy *Enemy = BactorialFindEnemy(V2(x,y));
    BactorialAttack(Enemy);
}


void 
BactorialSelect(float minx, float miny, float maxx, float maxy)
{
    // EM_ASM({
    //   console.log('Selecting rect: : ' + [$0, $1, $2, $3]);
    // }, minx, miny, maxx, maxy);

    rect Rect = {{minx, miny}, {maxx, maxy}};

    if (Length(Rect.Max - Rect.Min) < 5.0f) {
        v2 ClickP = Rect.Min + (Rect.Max - Rect.Min);
        if (BactorialSelectAtP(ClickP)) {
            u32 a =4;
        } else if (World.SelectedCount) {
            BactorialAttackEnemy(ClickP.x, ClickP.y);
        }
    } else {
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

    // EM_ASM({
    //   console.log('Selected: ' + [$0]);
    // }, World.SelectedCount);
}



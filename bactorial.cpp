#include "include/pcg_basic.h"
#include "include/pcg_basic.c"

#define MAX_OBJECTS 100000
#define MITOSIS_TIME 1.0f
#define START_R 50

static pcg32_random_t Rng;

r32 
RandomN()
{
    return (r32)(pcg32_random_r(&Rng) % 100) / 100.0f;
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
BactorialUpdateWorld(float dt) 
{
    rect Screen = {{0.0f, 0.0f}, {500.0f, 500.0f}};

    r32 MaxDistance = 0.0f;

    // World.AttractorActive = true;

    for (u32 i=0; i<World.ObjectCount; ++i) {
        object *Object = World.Objects + i;
        if (Object->Mode == Mode_Mitosis) {
            Object->MitosisProgress += dt / MITOSIS_TIME;

            if (Object->MitosisProgress >= 1.0f) {
                DivideCell(Object);
            }
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

        BactorialStabilizeColony();

        v2 Slowdown = World.Objects[i].Velocity * -1.0f * 0.02f;
        World.Objects[i].Velocity += Slowdown;

        v2 SummedVelocity = (World.Objects[i].Velocity + World.Objects[i].Velocity2);

        World.Objects[i].P = World.Objects[i].P + SummedVelocity * dt;
        World.Positions[i] = World.Objects[i].P;
        World.Velocities[i] = World.Objects[i].Velocity;
    }

    // if (MaxDistance < 10.0f) {
    //     World.AttractorActive = false;
    // }

    World.Tree.NodeCount = 0;
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

    World.Objects[0].Radius = START_R;
    World.Objects[0].Soul = RandomN();

    for (u32 i=0; i<World.ObjectCount; ++i) {
        World.Objects[i].P.x = RandomN() * 25.0f;
        World.Objects[i].P.y = RandomN() * 25.0f;
        World.Objects[i].Radius = START_R;
    }

    World.Tree.Root = (quad_tree_node *)malloc(MAX_NODE_COUNT * sizeof(quad_tree_node));

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

void 
BactorialSelectAtP(v2 P)
{
    u32 Index = 0;
    r32 MinDistanceSq = 999999999.0f;
    for (u32 i=0; i<World.ObjectCount; ++i) {
        World.Objects[i].Selected = false;

        r32 Lsq = LengthSq(P - World.Objects[i].P);

        if (Lsq < MinDistanceSq) {
            Index = i;
            MinDistanceSq = Lsq;
        }
    }

    World.Objects[Index].Selected = true;
    World.SelectedIndex = Index;
    World.SelectedCount = 1;
}

void 
BactorialSelectInRect(rect Rect)
{
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


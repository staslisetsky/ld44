#include "include/pcg_basic.h"
#include "include/pcg_basic.c"

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
BactorialUpdateWorld(float dt) 
{
    rect Screen = {{0.0f, 0.0f}, {500.0f, 500.0f}};

    r32 MaxDistance = 0.0f;

    World.AttractorActive = true;

    for (u32 i=0; i<World.ObjectCount; ++i) {
        if (World.AttractorActive) {
            World.AttractorP = {(r32)sin(World.Time * 4.0f) * 250.0f + 200.0f, (r32)cos(World.Time * 2.0f) * 250.0f + 200.0f};

            r32 Distance = Length(World.AttractorP - World.Objects[i].P);
            v2 Direction = (World.AttractorP - World.Objects[i].P) / Distance;

            if (Distance > 40.0f) {
                World.Objects[i].Velocity += Direction * 15000.0f / Distance;
            } else {
                World.Objects[i].Velocity -= Direction * 100;
            }
            
        } else {
            World.Objects[i].Velocity.x = (World.Objects[i].Velocity.x - 1, 0);
            World.Objects[i].Velocity.y = (World.Objects[i].Velocity.y - 1, 0);
        }

        r32 Len = Length(World.AttractorP - World.Objects[i].P);
        if (Len > MaxDistance) {
            MaxDistance = Len;
        }

        World.Objects[i].P = World.Objects[i].P + World.Objects[i].Velocity * dt;

        World.Positions[i] = World.Objects[i].P;
        World.Velocities[i] = World.Objects[i].Velocity;
    }

    // if (MaxDistance < 10.0f) {
    //     World.AttractorActive = false;
    // }

    World.Tree.NodeCount = 0;
    BuildQuadTree(&World.Tree, World.Tree.Root, Screen, World.Objects, World.ObjectCount, MAX_NODE_COUNT);

    World.Time += dt;
}

void *
BactorialInitWorld() 
{
    pcg32_random_t rng1, rng2, rng3;
    pcg32_srandom_r(&rng1, 3908476239044, (int)&rng1);

    World.ObjectCount = 100;
    World.Objects = (object *)malloc(sizeof(object) * World.ObjectCount);
    World.Positions = (v2 *)malloc(sizeof(v2) * World.ObjectCount);
    World.Velocities = (v2 *)malloc(sizeof(v2) * World.ObjectCount);

    for (u32 i=0; i<World.ObjectCount; ++i) {
        r32 X = (r32)(pcg32_random_r(&rng1) % 1000) / 1000.0f;
        r32 Y = (r32)(pcg32_random_r(&rng1) % 1000) / 1000.0f;
        World.Objects[i].P = {X * 500.0f,Y* 500.0f};
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

void 
BactorialSelectAtP(v2 P) 
{
    quad_tree_node *Root = World.Tree.Root;

    rect Screen = {{0.0f, 0.0f}, {500.0f, 500.0f}};
    u32 Index;
    if (LocateObjectWithP(Root, Screen, P, &Index)) {
        object *Object = World.Objects + Index;
        // Object->Selected = true;
    }
}

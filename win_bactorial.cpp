#include <stdint.h>
#include <stdio.h>
#include <vector>
#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <stdint.h>
#include <math.h>

#include "bactorial.h"
#include "bactorial.cpp"

#include "win_bactorial.h"

static bool AppRunning = true;

ID2D1Factory *Factory;
ID2D1HwndRenderTarget *RenderTarget;
D2D1_RECT_F WindowLayoutRect; 

template <class T> void 
SafeRelease(T **Thing)
{
    if (*Thing) {
        (*Thing)->Release();
        *Thing = NULL;
    }
}

struct rgba {
    r32 r,g,b,a;
};

rgba RGBA(int r, int g, int b, int a) { return {(r32)r,(r32)g,(r32)b,(r32)a}; }

rgba 
Normalize(rgba Color) 
{ 
    Color.r /= 255.0f;
    Color.g /= 255.0f;
    Color.b /= 255.0f;
    Color.a /= 255.0f;
    return Color;
}

u32
Safecast_s32_u32(s32 Value)
{
    u16 MaxPositive_s32 = (u16)0xffff >> 1;
    Value = Max_s32(0, Value);
    Value = Min_s32(Value, MaxPositive_s32);
    return (u32)Value;
}

void
DrawRectOutline(rect Rect)
{
    ID2D1SolidColorBrush *BlackBrush;
    RenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &BlackBrush);

    D2D1_POINT_2F TopLeft = {Rect.Min.x, Rect.Min.y};
    D2D1_POINT_2F TopRight = {Rect.Max.x, Rect.Min.y};
    D2D1_POINT_2F BottomLeft= {Rect.Min.x, Rect.Max.y};
    D2D1_POINT_2F BottomRight = {Rect.Max.x, Rect.Max.y};

    RenderTarget->DrawLine(TopLeft,TopRight,BlackBrush, 2.0, 0);
    RenderTarget->DrawLine(TopRight,BottomRight,BlackBrush, 2.0, 0);
    RenderTarget->DrawLine(BottomRight,BottomLeft,BlackBrush, 2.0, 0);
    RenderTarget->DrawLine(BottomLeft,TopLeft,BlackBrush, 2.0, 0);

    SafeRelease(&BlackBrush);
}

void
DrawQuadTree(quad_tree_node *Node)
{
    DrawRectOutline(Node->Rect);

    if (Node->Subdivided) {
        for (u32 i=0; i<4; ++i) {
            DrawQuadTree(Node->Sub[i]);
        }
    }
}

void
Render()
{
    ID2D1SolidColorBrush *BlackBrush;
    ID2D1SolidColorBrush *RedBrush;
    RenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &BlackBrush);
    RenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &RedBrush);
    
    RenderTarget->BeginDraw();
    RenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

    DrawQuadTree(World.Tree.Root);

    for (u32 i=0; i<World.ObjectCount; ++i) {
        object Object = World.Objects[i];
        u32 Dim = 10;
        u32 HalfScreen = 250;

        D2D1_POINT_2F ObjectCenter = {Object.P.x + HalfScreen, Object.P.y + HalfScreen};
        D2D1_ELLIPSE ObjectEllipse = {ObjectCenter, (r32)Object.Radius, (r32)Object.Radius};

        if (Object.Selected) {
            RenderTarget->FillEllipse(ObjectEllipse, RedBrush);
        } else {
            RenderTarget->FillEllipse(ObjectEllipse, BlackBrush);
        }
    }
    
    for (u32 i=0; i<World.EnemyCount; ++i) {
        enemy Enemy = World.Enemies[i];
        u32 Dim = 10;
        u32 HalfScreen = 250;

        D2D1_POINT_2F EnemyCenter = {Enemy.P.x + HalfScreen, Enemy.P.y + HalfScreen};
        D2D1_ELLIPSE EnemyEllipse = {EnemyCenter, (r32)Enemy.Radius, (r32)Enemy.Radius};

        RenderTarget->FillEllipse(EnemyEllipse, RedBrush);
    }

    if (World.SelectionMode) {
        rect RenderRect = World.SelectionRect;
        RenderRect.Min += V2(250.0f, 250.0f);
        RenderRect.Max += V2(250.0f, 250.0f);
        DrawRectOutline(RenderRect);
    }

    // RenderTarget->FillRectangle(D2D1::RectF(World.AttractorP.x, World.AttractorP.y, World.AttractorP.x + 40, World.AttractorP.y + 40), RedBrush);

    RenderTarget->EndDraw();
    SafeRelease(&BlackBrush); 
    SafeRelease(&RedBrush); 

#if 0
    rgba SelectionColor = Normalize(RGBA(116, 158, 215, 255));
    rgba DarkGrey = Normalize(RGBA(25, 25, 25, 255));
    
    RenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &WhiteBrush);
    RenderTarget->CreateSolidColorBrush(D2D1::ColorF(SelectionColor.r, SelectionColor.g, SelectionColor.b, 1.0f), &SelectionBrush);
    RenderTarget->CreateSolidColorBrush(D2D1::ColorF(DarkGrey.r, DarkGrey.g, DarkGrey.b, 1.0f), &DarkGreyBrush);

    v2i LayoutP = {};
    
    s32 DebugWindowHeight = 50;

    // if (State.ShowBuffers) {
    
    LayoutP.y = DebugWindowHeight;

    u32 BlockHeight = 10;
    u32 BlockWidth = State.Width - 1;

    r32 HeadLeft = 1;
    r32 HeadRight = ((r32)State.Editable.GapByteP / (r32)EDITABLE_BUFFER_SIZE) * BlockWidth + HeadLeft;
    RenderTarget->FillRectangle(D2D1::RectF(HeadLeft, 1, HeadRight, BlockHeight), SelectionBrush);
    
    r32 GapLeft = HeadRight;
    r32 GapRight = ((r32)State.Editable.GapSize() / (r32)EDITABLE_BUFFER_SIZE) * BlockWidth + GapLeft;
    RenderTarget->FillRectangle(D2D1::RectF(GapLeft, 1, GapRight, BlockHeight), WhiteBrush);

    r32 TailLeft = GapRight;
    r32 TailRight = (r32)(State.Editable.Used + State.Editable.GapSize()) / (r32)EDITABLE_BUFFER_SIZE * BlockWidth + TailLeft;
    RenderTarget->FillRectangle(D2D1::RectF(TailLeft, 1, TailRight, BlockHeight), SelectionBrush);
    // }

    v2i MouseP = {};
    v2i MouseClickP = {};

    MouseP.x = Input.MouseP.x + LayoutP.x; 
    MouseP.y = Input.MouseP.y - LayoutP.y; 
    MouseClickP.x = State.MouseClickP.x + LayoutP.x;
    MouseClickP.y = State.MouseClickP.y - LayoutP.y;

    // u32 RemainingSize = State.RenderBuffer.Size;
    // for (u32 i=0; i<State.Commands.size(); ++i) {
    //     ls_parser Utf8Stream = ls_parser::MakeFromString(State.Commands[i].String);
    //     u32 WroteBytes = Utf8Stream.ConvertToWideString(State.RenderBuffer.Data + State.RenderBuffer.Used, RemainingSize);
    //     State.RenderBuffer.Used += WroteBytes;
    //     RemainingSize -= WroteBytes;
    // }

    u32 CaretOffset = State.RenderBuffer.Used / 2;

    u32 HeadSize = State.Editable.GapByteP;
    u32 TailSize = State.Editable.Size - State.Editable.GapEndByteP();

    if (HeadSize) {
        Assert(State.RenderBuffer.RemainingBytes() >= HeadSize);
        ls_parser HeadString = ls_parser::MakeFromData(State.Editable.Data, State.Editable.GapByteP);
        u32 WroteBytes = HeadString.ConvertToWideString(State.RenderBuffer.Data, State.RenderBuffer.Size);
        State.RenderBuffer.Used += WroteBytes;
    }

    if (TailSize) {
        Assert(State.RenderBuffer.RemainingBytes() >= TailSize);
        ls_parser TailString = ls_parser::MakeFromData(State.Editable.GapEndPointer(), TailSize);
        u32 WroteBytes = TailString.ConvertToWideString(State.RenderBuffer.Data + State.RenderBuffer.Used, State.RenderBuffer.RemainingBytes());
        State.RenderBuffer.Used += WroteBytes;
    }

    IDWriteTextLayout *Layout;
    DWriteFactory->CreateTextLayout((wchar_t *)State.RenderBuffer.Data, State.RenderBuffer.Used, TextFormat, State.Width, State.Height, &Layout);

#if 0
    if (Input.Mouse[0].Down) {
        BOOL Trailing;
        BOOL Inside;
        DWRITE_HIT_TEST_METRICS ClickMetrics = {};
        Layout->HitTestPoint(MouseClickP.x, MouseClickP.y, &Trailing, &Inside, &ClickMetrics);

        s32 One = ClickMetrics.textPosition;
        s32 Two = ClickMetrics.textPosition;

        if (Input.Mouse[0].WentDown) {
            State.CaretP.CharIndex = ClickMetrics.textPosition;
            State.Selection = State.CaretP;
        }

        if (MouseClickP.x != MouseP.x || MouseClickP.y != MouseP.y) {
            Layout->HitTestPoint(MouseP.x, MouseP.y, &Trailing, &Inside, &ClickMetrics);
            Two = ClickMetrics.textPosition;
            
            State.Selection.CharIndex = Two;
        }
    }
#endif

    r32 X, Y;
    DWRITE_HIT_TEST_METRICS Metrics;
    Layout->HitTestTextPosition(State.Editable.CaretP + CaretOffset, false, &X, &Y, &Metrics);
    D2D1_RECT_F CaretRect;

    u32 CaretThickness = 2;
    CaretRect.left = X - CaretThickness / 2.0f + LayoutP.x;
    CaretRect.right  = CaretRect.left + CaretThickness + LayoutP.x;
    CaretRect.top  = Y + LayoutP.y;
    CaretRect.bottom = Y + Metrics.height + LayoutP.y;

    // if (State.CaretP.CharIndex != State.Selection.CharIndex) {
    //     s32 SelectionStart = Min_s32(State.CaretP.CharIndex, State.Selection.CharIndex);
    //     s32 SelectionEnd = Max_s32(State.CaretP.CharIndex, State.Selection.CharIndex);

    //     DWRITE_TEXT_RANGE SelectionRange = { Safecast_s32_u32(SelectionStart), Safecast_s32_u32(SelectionEnd - SelectionStart) };
    //     Layout->SetDrawingEffect(WhiteBrush, SelectionRange);

    //     DWRITE_TEXT_METRICS TextMetrics = {};
    //     Layout->GetMetrics(&TextMetrics);

    //     u32 Size = TextMetrics.lineCount * sizeof(DWRITE_HIT_TEST_METRICS);

    //     // Possible todo: check for E_NOT_SUFFICIENT_BUFFER error
    //     DWRITE_HIT_TEST_METRICS *HitTestMetrics = (DWRITE_HIT_TEST_METRICS *)TmpAlloc(Size);
    //     u32 ActualMetricCount = 0;
    //     Layout->HitTestTextRange(SelectionStart, SelectionEnd - SelectionStart, 0.0f, 0.0f, HitTestMetrics,TextMetrics.lineCount, &ActualMetricCount);

    //     for (u32 i=0; i<ActualMetricCount; ++i) {
    //         DWRITE_HIT_TEST_METRICS SelectionMetrics = HitTestMetrics[i];
    //         D2D1_RECT_F SelectionRect = {};
    //         SelectionRect.left = SelectionMetrics.left + LayoutP.x;
    //         SelectionRect.top = SelectionMetrics.top + LayoutP.y;
    //         SelectionRect.right = SelectionMetrics.left + SelectionMetrics.width + LayoutP.x;
    //         SelectionRect.bottom = SelectionMetrics.top + SelectionMetrics.height + LayoutP.y;
    //         RenderTarget->FillRectangle(SelectionRect, SelectionBrush);
    //     } 
    // }

    RenderTarget->FillRectangle(CaretRect, BlackBrush);
    RenderTarget->DrawTextLayout({(r32)LayoutP.x, (r32)LayoutP.y}, Layout, BlackBrush, D2D1_DRAW_TEXT_OPTIONS_NONE);
    RenderTarget->EndDraw();


    Input.Mouse[0].WentDown = false;
    Input.Mouse[1].WentDown = false;

    Input.Mouse[0].WentUp = false;
    Input.Mouse[1].WentUp = false;

    SafeRelease(&Layout);
    SafeRelease(&BlackBrush); 
    SafeRelease(&WhiteBrush); 
    SafeRelease(&SelectionBrush); 

#endif
}

LRESULT CALLBACK 
WindowProc(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
    int Result = 0;

    switch (Message) {
        case WM_DESTROY: {
            AppRunning = false;
        }; break;
        case WM_KEYDOWN: {
            b32 PreviousStateDown = (b32)((LParam & (1 << 30)) > 0);
            Input.Keys[(key)WParam].Down = true;
            Input.Keys[(key)WParam].WentDown = true;
        }; break;

        case WM_KEYUP: {
            Input.Keys[(key)WParam].Down = false;
            Input.Keys[(key)WParam].WentUp = true;
            Input.Keys[(key)WParam].WentDown = false;
        }; break;

        case WM_MOUSEWHEEL: {
           Input.Wheel = (s32)(WParam >> 16);
        }; break;

        case WM_LBUTTONDOWN: {
            Input.Mouse[0].WentDown = true;
            Input.Mouse[0].Down = true;
        }; break;
        case WM_RBUTTONDOWN: {
            Input.Mouse[1].WentDown = true;
            Input.Mouse[1].Down = true;
        }; break;
        case WM_LBUTTONUP: {
            Input.Mouse[0].WentUp = true;
            Input.Mouse[0].Down = false;
        }; break;
        case WM_RBUTTONUP: {
            Input.Mouse[1].WentUp = true;
            Input.Mouse[1].Down = false;
        }; break;

        case WM_LBUTTONDBLCLK: {
            Input.Mouse[0].WentDown = true;
            Input.Mouse[0].Down = true;
            Input.Mouse[0].DoubleClick = true;
        }; break;
        case WM_RBUTTONDBLCLK: {
            Input.Mouse[1].WentDown = true;
            Input.Mouse[1].Down = true;
            Input.Mouse[1].DoubleClick = true;
        }; break;

        case WM_PAINT: {
            PAINTSTRUCT Paint;
            BeginPaint(Window, &Paint);
            Render();
            EndPaint(Window, &Paint);
        }; break;

        case WM_SIZE: {
            if (RenderTarget != NULL) {
                RECT ClientRect;
                GetClientRect(Window, &ClientRect);

                State.Width =  ClientRect.right - ClientRect.left;
                State.Height =  ClientRect.bottom - ClientRect.top;

                D2D1_SIZE_U Size = D2D1::SizeU(ClientRect.right, ClientRect.bottom);
                WindowLayoutRect = D2D1::RectF((float)ClientRect.left, (float)ClientRect.top, (float)(ClientRect.right - ClientRect.left), (float)(ClientRect.bottom - ClientRect.top));
                RenderTarget->Resize(Size);
                InvalidateRect(Window, NULL, FALSE);
            } 
        }; break;

        default: {
            Result = DefWindowProc(Window, Message, WParam, LParam);
        }
    }

    return Result;
}

int WINAPI 
WinMain(HINSTANCE Instance, HINSTANCE PreviousInstance, LPSTR Args, int WindowShowMode)
{
    HRESULT Result = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &Factory);

    WNDCLASSA WindowClass = {} ;
    WindowClass.style = CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc = (WNDPROC)WindowProc;
    WindowClass.hInstance  = Instance;
    WindowClass.lpszClassName = "WindowClass";

    RegisterClassA(&WindowClass);

    HWND Window = CreateWindowA("WindowClass", "Shi", 
                                WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL, 
                                CW_USEDEFAULT, CW_USEDEFAULT, 500, 500,
                                0, 0, Instance, 0);

    HDC DeviceContext = GetDC(Window);
    ShowScrollBar(Window, SB_BOTH, false);
    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    D2D1_SIZE_U Size = D2D1::SizeU(ClientRect.right, ClientRect.bottom);
    Result = Factory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(),
                                             D2D1::HwndRenderTargetProperties(Window, Size),
                                             &RenderTarget);

    State.Width =  ClientRect.right - ClientRect.left;
    State.Height =  ClientRect.bottom - ClientRect.top;

    // State.StaticBuffer.Size = Megabytes(100);
    // State.StaticBuffer.Data = (u8 *)Allocate(State.StaticBuffer.Size);
    
    // State.RenderBuffer.Size = Kilobytes(64);
    // State.RenderBuffer.Data = (u8 *)Allocate(State.RenderBuffer.Size);
    
    // State.RenderBuffer.Size = Kilobytes(64);
    // State.RenderBuffer.Data = (u8 *)Allocate(State.RenderBuffer.Size);
    
    // State.TmpBuffer.Size = Kilobytes(64);
    // State.TmpBuffer.Data = (u8 *)Allocate(State.TmpBuffer.Size);

    // State.Editable.Size = EDITABLE_BUFFER_SIZE;
    // State.Editable.Data = (u8 *)Allocate(State.Editable.Size);
    // State.Editable.At = State.Editable.Data;

    BactorialInitWorld();
    
    while (AppRunning) {
        MSG Message;
        while (PeekMessageA(&Message, Window, 0, 0, PM_REMOVE)) {
            TranslateMessage(&Message);        
            DispatchMessage(&Message);
        }

        POINT MousePoint;
        GetCursorPos(&MousePoint);
        ScreenToClient(Window, &MousePoint);

        Input.MouseP.x = MousePoint.x - 250.0;
        Input.MouseP.y = MousePoint.y - 250.0;

        if (Input.Mouse[0].WentDown) {
            v2 P = {(r32)Input.MouseP.x, (r32)Input.MouseP.y};
            World.SelectionStart = Input.MouseP;
            World.SelectionRect = {{Input.MouseP.x, Input.MouseP.y}, {Input.MouseP.x, Input.MouseP.y}};
        }

        if (Input.Mouse[0].Down) {
            if (Length(Input.MouseP - World.SelectionRect.Min) > 20.0f) {
                World.SelectionMode = true;
            }

            if (World.SelectionMode) {
                v2 Min = World.SelectionStart;
                v2 Max = Input.MouseP;

                r32 MinX = Min_r32(Min.x, Max.x);
                r32 MinY = Min_r32(Min.y, Max.y);
                r32 MaxX = Max_r32(Min.x, Max.x);
                r32 MaxY = Max_r32(Min.y, Max.y);

                World.SelectionRect = {{MinX, MinY}, {MaxX, MaxY}};
            }
        }

        if (Input.Mouse[0].WentUp) {
            if (World.SelectionMode) {
                World.SelectionMode = false;    
            }
            BactorialSelect(World.SelectionRect.Min.x, World.SelectionRect.Min.y,
                            World.SelectionRect.Max.x, World.SelectionRect.Max.y);
        }

        if (Input.Keys[Key_F1].WentDown) {
            BactorialCommenceMitosis();
        }

        r32 dt = 1.0f/ 60.0f;
        BactorialUpdateWorld(dt);
        InvalidateRect(Window, NULL, FALSE);

        Input.Mouse[0].WentDown = false;
        Input.Mouse[1].WentDown = false;

        Input.Mouse[0].WentUp = false;
        Input.Mouse[1].WentUp = false;

        for (u32 i=0; i<Key_Count; ++i) {
           Input.Keys[i].WentDown = false; 
           Input.Keys[i].WentUp = false; 
        }

        Sleep(1.0f/ 60.0f);
    }
}
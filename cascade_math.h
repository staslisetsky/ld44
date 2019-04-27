#define PI 3.14159265358979323846
#define ONE_DEGREE_IN_RADIANS PI / 180.0f
#define ONE_RADIAN_IN_DEGREES 180.0f / PI

#define M4X4_IDENTITY {{ 1.0f,0.0f, 0.0f, 0.0f, 0.0f,1.0f, 0.0f, 0.0f, 0.0f,0.0f, 1.0f, 0.0f, 0.0f,0.0f, 0.0f, 1.0f }}

union v2 {
    struct {
        r32 x,y;
    };
    r32 E[2];
};

union v2i {
    struct {
        s32 x,y;
    };
    s32 E[2];
};

union v3{
    struct {
        r32 x,y,z;
    };
    struct {
        r32 r,g,b;
    };
    struct {
        r32 Hue,Sat,Val;
    };

    v2 xy;
    r32 E[3];
};

union v4{
    struct {
        r32 x,y,z,w;
    };
    struct {
        r32 r,g,b;
    };
    struct {
        r32 Hue,Sat,Val;
    };
    struct {
        v3 _ignore;
        r32 a;
    };
    v3 rgb;
    v3 xyz;
    v3 hsv;
    r32 E[4];
};

struct mat3x3 {
    r32 E[9];
};

union m4x4 {
    struct {
        v4 Rows[4];
    };
    r32 E[16];
};

union uv {
    struct {
        v2 TopLeft;
        v2 TopRight;
        v2 BottomLeft;
        v2 BottomRight;
    };
    v2 E[4];
};

struct rect {
    v2 Min;
    v2 Max;
};

struct rect4 {
    v2 BottomLeft;
    v2 TopLeft;
    v2 TopRight;
    v2 BottomRight;
};

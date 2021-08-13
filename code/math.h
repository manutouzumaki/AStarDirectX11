#ifndef MATH_H
#define MATH_H

#include <math.h>

struct v2
{
    float X, Y;
};

struct v3
{
    float X, Y, Z;
};

struct mat4
{
    float m[4][4];
};

v2 operator+(v2& A, v2& B)
{
    v2 Result = {};
    Result.X = A.X + B.X;
    Result.Y = A.Y + B.Y;
    return Result;
}

v2 operator-(v2& A, v2& B)
{
    v2 Result = {};
    Result.X = A.X - B.X;
    Result.Y = A.Y - B.Y;
    return Result;
}

v2 operator+(v2& A, float& S)
{
    v2 Result = {};
    Result.X = A.X + S;
    Result.Y = A.Y + S;
    return Result;
}

v2 operator-(v2& A, float& S)
{
    v2 Result = {};
    Result.X = A.X - S;
    Result.Y = A.Y - S;
    return Result;
}

v2 operator*(v2& A, float& S)
{
    v2 Result = {};
    Result.X = A.X * S;
    Result.Y = A.Y * S;
    return Result;
}

v2 operator/(v2& A, float& S)
{
    v2 Result = {};
    Result.X = A.X / S;
    Result.Y = A.Y / S;
    return Result;
}

v3 operator+(v3& A, v3& B)
{
    v3 Result = {};
    Result.X = A.X + B.X;
    Result.Y = A.Y + B.Y;
    Result.Z = A.Z + B.Z;
    return Result;
}

v3 operator-(v3& A, v3& B)
{
    v3 Result = {};
    Result.X = A.X - B.X;
    Result.Y = A.Y - B.Y;
    Result.Z = A.Z - B.Z;
    return Result;
}

v3 operator+(v3& A, float& S)
{
    v3 Result = {};
    Result.X = A.X + S;
    Result.Y = A.Y + S;
    Result.Z = A.Z + S;
    return Result;
}

v3 operator-(v3& A, float& S)
{
    v3 Result = {};
    Result.X = A.X - S;
    Result.Y = A.Y - S;
    Result.Z = A.Z - S;
    return Result;
}

v3 operator*(v3& A, float& S)
{
    v3 Result = {};
    Result.X = A.X * S;
    Result.Y = A.Y * S;
    Result.Z = A.Z * S;
    return Result;
}

v3 operator/(v3& A, float& S)
{
    v3 Result = {};
    Result.X = A.X / S;
    Result.Y = A.Y / S;
    Result.Z = A.Z / S;
    return Result;
}



mat4 operator*(mat4& A, mat4& B)
{
    mat4 Result;
    for(int Y = 0; Y < 4; Y++)
    {
        for(int X = 0; X < 4; X++)
        {
                Result.m[Y][X] =
                A.m[Y][0] * B.m[0][X] +
                A.m[Y][1] * B.m[1][X] +
                A.m[Y][2] * B.m[2][X] +
                A.m[Y][3] * B.m[3][X];
        }
    }
    return Result;
}

mat4 IdentityMat4()
{
    mat4 Result = {{
        {1.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f}
    }};
    return Result;
}

mat4 TranslationMat4(v3 V)
{
    mat4 Result = {{
        {1.0f, 0.0f, 0.0f, V.X},
        {0.0f, 1.0f, 0.0f, V.Y},
        {0.0f, 0.0f, 1.0f, V.Z},
        {0.0f, 0.0f, 0.0f, 1.0f}
    }};
    return Result;
}

mat4 ScaleMat4(v3 V)
{
    mat4 Result = {{
        {V.X,  0.0f, 0.0f, 0.0f},
        {0.0f, V.Y,  0.0f, 0.0f},
        {0.0f, 0.0f, V.Z,  0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f}
    }};
    return Result;
}

mat4 OrthogonalMat4(int width, int height, float znear, float zfar)
{
    float w = (float)width;
    float h = (float)height;
    
    mat4 result = {{
        {2.0f / w, 0.0f,     0.0f,                   0.0f},
        {0.0f,     2.0f / h, 0.0f,                   0.0f},
        {0.0f,     0.0f,     1.0f / (zfar - znear),  0.0f},
        {0.0f,     0.0f,     znear / (znear - zfar), 1.0f}
    }};
    return result;
}

float LengthV3(v3 V)
{
    return sqrtf(V.X*V.X + V.Y*V.Y);
}

v3 NormalizeV3(v3 V)
{
    v3 Result = {};
    Result.X = V.X / LengthV3(V);
    Result.Y = V.Y / LengthV3(V);
    Result.Z = V.Z / LengthV3(V);
    return Result;
}  

float DotV3(v3 A, v3 B)
{
    return (A.X*B.X + A.Y*B.Y + A.Z*B.Z);
}

v3 CrossV3(v3 A, v3 B)
{
    v3 Result = {};
    Result.X = A.Y*B.Z - A.Z*B.Y; 
    Result.Y = A.Z*B.X - A.X*B.Z;
    Result.Z = A.X*B.Y - A.Y*B.X;
    return Result;
}

mat4 ViewMat4(v3 eye, v3 target, v3 up)
{
    v3 z = NormalizeV3(target - eye);
    v3 x = NormalizeV3(CrossV3(up, z));
    v3 y = CrossV3(z, x);
    mat4 result = {{
        { x.X,              y.X,              z.X,             0.0f},
        { x.Y,              y.Y,              z.Y,             0.0f},
        { x.Z,              y.Z,              z.Z,             0.0f},
        {-DotV3(x, eye),   -DotV3(y, eye),   -DotV3(z, eye),   1.0f}
    }}; 
    return result; 
}

#endif

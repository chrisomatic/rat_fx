#pragma once

#include <stdint.h>

typedef struct
{
    float x,y;
} Vector2f;

typedef struct
{
    float x,y,z;
} Vector3f;

typedef struct
{
    Vector2f position;
    Vector2f tex_coord;
} Vertex;

#define PI        3.14159265358f
#define PI_OVER_2 1.57079632679f

#define RAD(x) (((x) * PI) / 180.0f)
#define DEG(x) (((x) * 180.0f) / PI)
#define ABS(x) ((x) < 0 ? -1*(x) : (x))

#define MIN(x,y) ((x) < (y) ? (x) : (y))
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define SQ(x) ((x)*(x))

#define BOUND(a,l,u)    MAX(MIN(a,u),l)

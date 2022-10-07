#pragma once

#include "rat_math.h"

typedef struct
{
    Vector2f pos;
} Camera;

void camera_init();
void camera_move(float x, float y);
void get_camera_pos(float* x, float* y);
Matrix* get_camera_transform();

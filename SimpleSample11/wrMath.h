#pragma once
#include <random>
#include "linmath.h"
#include <cmath>

static inline float randf()
{
    return (float)rand() / RAND_MAX;
}

const float WR_M_PI = 3.141592654f;
#pragma once
#include "vec3.h"

#include "rtweekend.h"

using color = vec3;

void write_color(std::ofstream& out, const color& pixel_color);
double linear_to_gamma(double linear_component);
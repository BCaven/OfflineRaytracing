#pragma once

#include "vec3.h"
#include <spdlog/spdlog.h>


class ray
{
public:
    ray() {}

    ray(const point3& origin, const vec3& direction, double time)
        : orig(origin), dir(direction), tm(time) 
    {
        if (dir.length() != 0)
        {
            //spdlog::warn("direction failed! {}", dir.length());
            dir = dir / dir.length();
        }
    }

    ray(const point3& origin, const vec3& direction)
        : ray(origin, direction, 0) {}
    const point3& origin() const { return orig; }
    const vec3& direction() const { return dir; }

    point3 at(double t) const {
        return orig + t * dir;
    }

    double time() const { return tm; }

private:
    point3 orig;
    vec3 dir;
    double tm;
};


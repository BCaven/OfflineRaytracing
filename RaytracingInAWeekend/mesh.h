#pragma once

#include "hittable.h"
#include "utility.h"
#include "triangle.h"
#include "hittable_list.h"
#include "bvh.h"

class mesh : public hittable
{
public:
	mesh(int vertCount, std::vector<float> vertData, shared_ptr<material> mat);

	static std::shared_ptr<mesh> fromFile(std::string filename, shared_ptr<material> mat);

	aabb bounding_box() const override { return bbox; }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
		return bvh.hit(r, ray_t, rec);
    }

private:	
	std::shared_ptr<spdlog::logger> logger;
	std::vector<shared_ptr<triangle>> triangles;
	aabb bbox;
	hittable_list bvh;
};


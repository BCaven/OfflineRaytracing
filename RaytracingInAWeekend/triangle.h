#pragma once

#include "hittable.h"



class triangle : public hittable {
public:

    triangle(const vertex& V1, const vertex& V2, const vertex& V3, shared_ptr<material> mat)
		: v0(V1), v1(V2), v2(V3), mat(mat)
    
    {
		logger = spdlog::get("triangle");
		if (!logger) logger = spdlog::stdout_color_mt("triangle");

        v0v1 = v1.position - v0.position;
		v0v2 = v2.position - v0.position;
		v1v2 = v2.position - v1.position;
		v2v0 = v0.position - v2.position;
		normal = cross(v0v1, v0v2);
        denom = normal.length_squared();
		normalized_normal = unit_vector(normal);
		area = normal.length() / 2;
		d = -1 * dot(normal, v0.position);
        set_bounding_box();
    }

    virtual void set_bounding_box() {
        // Compute the bounding box
        auto bbox1 = aabb(v0.position, v1.position);
        auto bbox2 = aabb(v0.position, v2.position);
        bbox = aabb(bbox1, bbox2);
    }

    aabb bounding_box() const override { return bbox; }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override 
    {
		// following the scratchapixel barycentric coordinates method for ray-triangle intersection

		double NdotRayDir = dot(normal, r.direction());

        if (fabs(NdotRayDir) < minimus) return false;

		double t = -(dot(normal, r.origin()) + d) / NdotRayDir;

        if (!ray_t.contains(t)) return false;

        vec3 P = r.at(t);

        vec3 C;
		vec3 v1p = P - v1.position;
        C = cross(v1v2, v1p);
        double u, v, w;
        if ((u = dot(normal, C)) < 0) return false;

		vec3 v2p = P - v2.position;
		C = cross(v2v0, v2p);
        if ((v = dot(normal, C)) < 0) return false;

		vec3 v0p = P - v0.position;
		C = cross(v0v1, v0p);
		if (dot(normal, C) < 0) return false;

        u /= denom;
		v /= denom;
		w = 1 - u - v;

		rec.u = u;
		rec.v = v;

        auto norm_t = -(dot(normalized_normal, r.origin()) + d) / dot(normalized_normal, r.direction());
        // need to also set rec.u and rec.v
        rec.t = norm_t;
        rec.p = P;
        rec.mat = mat;

        
        rec.set_face_normal(r, normalized_normal);

        return true;
    }

private:
	std::shared_ptr<spdlog::logger> logger;
    vertex v0, v1, v2;
    vec3 normal;
    vec3 normalized_normal;
    double denom = 0;
    double area;
	double d = 0; // plane equation parameter
	vec3 v0v1;
    vec3 v0v2;
	vec3 v2v0;
    vec3 v1v2;
    shared_ptr<material> mat;
    aabb bbox;
};

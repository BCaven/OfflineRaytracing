#pragma once
#include "hittable.h"
#include "pdf.h"
#include "texture.h"

class scatter_record {
public:
    color attenuation;
    shared_ptr<pdf> pdf_ptr;
    bool skip_pdf;
    ray skip_pdf_ray;
};

class material {
public:
    virtual ~material() = default;

    virtual bool scatter(const ray& r_in, const hit_record& rec, scatter_record& srec) const {
        return false;
    }

    virtual color emitted(
        const ray& r_in, const hit_record& rec, double u, double v, const point3& p
    ) const {
        return color(0.0, 0.0, 0.0);
    }

    virtual double scattering_pdf(const ray& r_in, const hit_record& rec, const ray& scattered)
        const {
        return 0;
    }
};

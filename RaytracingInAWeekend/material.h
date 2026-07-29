#pragma once

#include "base_material.h"
#include "camera.h"


class lambertian : public material {
public:
    lambertian(const color& albedo) : tex(make_shared<solid_color>(albedo)) {}
    lambertian(shared_ptr<texture> tex) : tex(tex) {}

    bool scatter(const ray& r_in, const hit_record& rec, scatter_record& srec) const override {
        srec.attenuation = tex->value(rec.u, rec.v, rec.p);
        srec.pdf_ptr = make_shared<cosine_pdf>(rec.normal);
        srec.skip_pdf = false;
        return true;
    }

    double scattering_pdf(const ray& r_in, const hit_record& rec, const ray& scattered)
        const override {
        auto cos_theta = dot(rec.normal, unit_vector(scattered.direction()));
        return cos_theta < 0 ? 0 : cos_theta / pi;
    }

private:
    shared_ptr<texture> tex;
};

constexpr int SH_COUNT = 16;
constexpr int SH_CHANNEL_COUNT = 3;
constexpr int SH_FLOAT_COUNT = SH_COUNT * SH_CHANNEL_COUNT;
constexpr int SH_REST_FLOAT_COUNT = SH_FLOAT_COUNT - SH_CHANNEL_COUNT;
constexpr int SH_PACKED_VEC4_COUNT = SH_FLOAT_COUNT / 4;

static_assert(SH_FLOAT_COUNT % 4 == 0,
    "Spherical harmonic coefficients must pack into vec4 attributes");

constexpr float SH_C0 = 0.28209479177387814f;
constexpr float SH_C1 = 0.4886025119029199f;
constexpr float SH_C2[] = {
    1.0925484305920792f,
    -1.0925484305920792f,
    0.31539156525252005f,
    -1.0925484305920792f,
    0.5462742152960396f
};
constexpr float SH_C3[] = {
    -0.5900435899266435f,
    2.890611442640554f,
    -0.4570457994644658f,
    0.3731763325901154f,
    -0.4570457994644658f,
    1.445305721320277f,
    -0.5900435899266435f
};
class shMaterial : public material {
public:
    shMaterial(std::array<float, SH_FLOAT_COUNT> sphericalHarmonics, int degree, camera& c): deg(degree), cam(c)
    {
        for (int i = 0; i < SH_COUNT; i++)
        {
            int shIndex = i * SH_CHANNEL_COUNT;
            sh[i] = glm::vec3(sphericalHarmonics[shIndex], sphericalHarmonics[shIndex + 1], sphericalHarmonics[shIndex + 2]);
        }
    }

    bool scatter(const ray& r_in, const hit_record& rec, scatter_record& srec) const override
    {
        // TODO: figure out reflections/transmission from spherical harmonics
        // it would be cool to train a model where the thing its training against is a renderer like this one
        // and allow it to also control splats emission and whatnot
        // use the camera's lookat vector instead of the ray itself
        // cam.lookat - cam.lookfrom = vec starting at lookfrom and going to lookat
        glm::dvec3 cam_lookat = glm::normalize(vec3::toVec3(cam.lookat - cam.lookfrom));
        
        srec.attenuation = colorFromSH(cam_lookat); // r_in.direction() // rec.normal
        srec.pdf_ptr = make_shared<cosine_pdf>(rec.normal);
        srec.skip_pdf = false;
        return true;
    }

    double scattering_pdf(const ray& r_in, const hit_record& rec, const ray& scattered) const override
    {
        auto cos_theta = dot(rec.normal, scattered.direction());
        return cos_theta < 0 ? 0 : cos_theta / pi;
    }

private:
    std::array<glm::vec3, SH_COUNT> sh = {};
    int deg = 0;
    camera& cam;

    inline color colorFromSH(glm::vec3 dir) const
    {
        // Degree is 3, but might as well support the rest
        // https://github.com/graphdeco-inria/diff-gaussian-rasterization/blob/main/cuda_rasterizer/forward.cu
        glm::vec3 result(SH_C0 * sh[0]);
        if (deg > 0)
        {
            float x = dir.x;
            float y = dir.y;
            float z = dir.z;
            result = result - SH_C1 * y * sh[1] + SH_C1 * z * sh[2] - SH_C1 * x * sh[3];


            if (deg > 1)
            {
                float xx = x * x, yy = y * y, zz = z * z;
                float xy = x * y, yz = y * z, xz = x * z;
                result = result +
                    SH_C2[0] * xy * sh[4] +
                    SH_C2[1] * yz * sh[5] +
                    SH_C2[2] * (2.0f * zz - xx - yy) * sh[6] +
                    SH_C2[3] * xz * sh[7] +
                    SH_C2[4] * (xx - yy) * sh[8];

                if (deg > 2)
                {
                    result = result +
                        SH_C3[0] * y * (3.0f * xx - yy) * sh[9] +
                        SH_C3[1] * xy * z * sh[10] +
                        SH_C3[2] * y * (4.0f * zz - xx - yy) * sh[11] +
                        SH_C3[3] * z * (2.0f * zz - 3.0f * xx - 3.0f * yy) * sh[12] +
                        SH_C3[4] * x * (4.0f * zz - xx - yy) * sh[13] +
                        SH_C3[5] * z * (xx - yy) * sh[14] +
                        SH_C3[6] * x * (xx - 3.0f * yy) * sh[15];
                }
            }
        }
        result += 0.5f;
        result = glm::max(result, 0.0f);
        return color(result.x, result.y, result.z);
    }

};

class metal : public material {
public:
    metal(const color& albedo, double fuzz) : albedo(albedo), fuzz(fuzz < 1 ? fuzz : 1) {}

    bool scatter(const ray& r_in, const hit_record& rec, scatter_record& srec) const override {
        vec3 reflected = reflect(r_in.direction(), rec.normal);
        reflected = unit_vector(reflected) + (fuzz * random_unit_vector());
        srec.attenuation = albedo;
        srec.pdf_ptr = nullptr;
        srec.skip_pdf = true;
        srec.skip_pdf_ray = ray(rec.p, reflected, r_in.time());

        return true;
    }

private:
    color albedo;

    // this is a weird way to handle imperfect reflections
    double fuzz;
};

class dielectric : public material {
public:
    dielectric(double refraction_index) : refraction_index(refraction_index) {}

    bool scatter(const ray& r_in, const hit_record& rec, scatter_record& srec) const override {
        srec.attenuation = color(1.0, 1.0, 1.0);
        srec.pdf_ptr = nullptr;
        srec.skip_pdf = true;
        double ri = rec.front_face ? (1.0 / refraction_index) : refraction_index;

        vec3 unit_direction = unit_vector(r_in.direction());
        double cos_theta = std::fmin(dot(-unit_direction, rec.normal), 1.0);
        double sin_theta = std::sqrt(1.0 - cos_theta * cos_theta);

        bool cannot_refract = ri * sin_theta > 1.0;
        vec3 direction;

        if (cannot_refract || reflectance(cos_theta, ri) > random_double())
            direction = reflect(unit_direction, rec.normal);
        else
            direction = refract(unit_direction, rec.normal, ri);

        srec.skip_pdf_ray = ray(rec.p, direction, r_in.time());
        return true;
    }

private:
    // Refractive index in vacuum or air, or the ratio of the material's refractive index over
    // the refractive index of the enclosing media
    double refraction_index;
    
    static double reflectance(double cosine, double refraction_index) {
        // Use Schlick's approximation for reflectance.
        auto r0 = (1 - refraction_index) / (1 + refraction_index);
        r0 = r0 * r0;
        return r0 + (1 - r0) * std::pow((1 - cosine), 5);
    }
};

class diffuse_light : public material {
public:
    diffuse_light(shared_ptr<texture> tex) : tex(tex) {}
    diffuse_light(const color& emit) : tex(make_shared<solid_color>(emit)) {}

    color emitted(const ray& r_in, const hit_record& rec, double u, double v, const point3& p)
        const override {
        if (!rec.front_face)
            return color(0, 0, 0);
        return tex->value(u, v, p);
    }

private:
    shared_ptr<texture> tex;
};

class isotropic : public material {
public:
    isotropic(const color& albedo) : tex(make_shared<solid_color>(albedo)) {}
    isotropic(shared_ptr<texture> tex) : tex(tex) {}

    bool scatter(const ray& r_in, const hit_record& rec, scatter_record& srec) const override {
        srec.attenuation = tex->value(rec.u, rec.v, rec.p);
        srec.pdf_ptr = make_shared<sphere_pdf>();
        srec.skip_pdf = false;
        return true;
    }

    double scattering_pdf(const ray& r_in, const hit_record& rec, const ray& scattered)
        const override {
        return 1 / (4 * pi);
    }

private:
    shared_ptr<texture> tex;
};
#pragma once
#include "hittable.h"
#include "hittable_list.h"
#include "material.h"
#include "vec3.h"
#include <cstdint>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <random>


inline thread_local std::random_device rd{};

// Mersenne twister PRNG, initialized with seed from previous random device instance
inline thread_local std::mt19937 gen{ rd() };

// Standard normal CDF: Phi(x) = 0.5 * (1 + erf(x / sqrt(2)))
static double normalCDF(double x) {
    return 0.5 * std::erfc(-x / std::sqrt(2.0));
}

// Inverse standard normal CDF (Acklam's rational approximation).
// Accurate to ~1.15e-9 relative error. std::erfinv doesn't exist in
// standard C++, so we use this well-known approximation instead.
static double normalPPF(double p) {
    if (p <= 0.0) return -infinity;
    if (p >= 1.0) return  infinity;

    // Coefficients for the rational approximation.
    static const double a[] = { -3.969683028665376e+01,  2.209460984245205e+02,
                                 -2.759285104469687e+02,  1.383577518672690e+02,
                                 -3.066479806614716e+01,  2.506628277459239e+00 };
    static const double b[] = { -5.447609879822406e+01,  1.615858368580409e+02,
                                 -1.556989798598866e+02,  6.680131188771972e+01,
                                 -1.328068155288572e+01 };
    static const double c[] = { -7.784894002430293e-03, -3.223964580411365e-01,
                                 -2.400758277161838e+00, -2.549732539343734e+00,
                                  4.374664141464968e+00,  2.938163982698783e+00 };
    static const double d[] = { 7.784695709041462e-03,  3.224671290700398e-01,
                                  2.445134137142996e+00,  3.754408661907416e+00 };

    const double p_low = 0.02425;
    const double p_high = 1.0 - p_low;
    double q, r, x;

    if (p < p_low) {
        q = std::sqrt(-2.0 * std::log(p));
        x = (((((c[0] * q + c[1]) * q + c[2]) * q + c[3]) * q + c[4]) * q + c[5]) /
            ((((d[0] * q + d[1]) * q + d[2]) * q + d[3]) * q + 1.0);
    }
    else if (p <= p_high) {
        q = p - 0.5;
        r = q * q;
        x = (((((a[0] * r + a[1]) * r + a[2]) * r + a[3]) * r + a[4]) * r + a[5]) * q /
            (((((b[0] * r + b[1]) * r + b[2]) * r + b[3]) * r + b[4]) * r + 1.0);
    }
    else {
        q = std::sqrt(-2.0 * std::log(1.0 - p));
        x = -(((((c[0] * q + c[1]) * q + c[2]) * q + c[3]) * q + c[4]) * q + c[5]) /
            ((((d[0] * q + d[1]) * q + d[2]) * q + d[3]) * q + 1.0);
    }
    return x;
}

class splat :
    public hittable
{
    static void get_sphere_uv(const point3& p, double& u, double& v) {
        // p: a given point on the sphere of radius one, centered at the origin.
        // u: returned value [0,1] of angle around the Y axis from X=-1.
        // v: returned value [0,1] of angle from Y=-1 to Y=+1.
        //     <1 0 0> yields <0.50 0.50>       <-1  0  0> yields <0.00 0.50>
        //     <0 1 0> yields <0.50 1.00>       < 0 -1  0> yields <0.50 0.00>
        //     <0 0 1> yields <0.25 0.50>       < 0  0 -1> yields <0.75 0.50>

        auto theta = std::acos(-p.y());
        auto phi = std::atan2(-p.z(), p.x()) + pi;

        u = phi / (2 * pi);
        v = theta / pi;
    }

    static vec3 random_to_sphere(double radius, double distance_squared) {
        auto r1 = random_double();
        auto r2 = random_double();
        auto z = 1 + r2 * (std::sqrt(1 - radius * radius / distance_squared) - 1);

        auto phi = 2 * pi * r1;
        auto x = std::cos(phi) * std::sqrt(1 - z * z);
        auto y = std::sin(phi) * std::sqrt(1 - z * z);

        return vec3(x, y, z);
    }

    inline double gaussian_pdf(point3 p) const
    {
        auto c = 1.0 / (std::pow(2 * pi, 3 / 2) * std::sqrt(detSigma));
        auto point_to_center = vec3::toVec3(p - center);
        // squared Mahalanobis distance (make sure this is accurate)
        auto squaredMahDist = glm::dot(point_to_center, invSigma * point_to_center);
        auto e = std::exp(-0.5 * squaredMahDist);
        return c * e;
    }

    inline interval calculate_ray_aabb_bounds(const ray& r) const 
    {

        // TODO: calc t_0 and t_1 from elipsoid surrounding gaussian instead of actual aabb
        interval ray_t = interval::universe;
        const point3& ray_orig = r.origin();
        const vec3& ray_dir = r.direction();

        for (int axis = 0; axis < 3; axis++) 
        {
            const interval& ax = bbox.axis_interval(axis);
            const double adinv = 1.0 / ray_dir[axis];

            auto t0 = (ax.min - ray_orig[axis]) * adinv;
            auto t1 = (ax.max - ray_orig[axis]) * adinv;

            if (t0 < t1) {
                if (t0 > ray_t.min) ray_t.min = t0;
                if (t1 < ray_t.max) ray_t.max = t1;
            }
            else {
                if (t1 > ray_t.min) ray_t.min = t1;
                if (t0 < ray_t.max) ray_t.max = t0;
            }
        }
        return ray_t;


    }



public:
    // full splats get stored in a bvh, this is a single splat
    vec3 center;
    double alpha = 0.0;
    glm::dvec3 scale;
    glm::quat rotation;
    glm::dvec3 rvec;
    std::array<float, SH_FLOAT_COUNT> sphericalHarmonics = {};
    aabb bbox;
    shared_ptr<material> mat;
    shared_ptr<material> outline_mat;
    glm::dmat3 invSigma;
    glm::dmat3 sigma;
    double detSigma = 0.0;
    double rho_0 = 0.0;
    double aabb_scale = 0.5;
    double normConst = 0.0;
    double splat_scale = 1.0;
    double radius = 0.1;
    bool valid = true;
    interval t_mean_range = interval::universe;


    splat(glm::vec3 c, glm::vec3 s, glm::quat rot, double a, std::array<float, SH_FLOAT_COUNT> sh_array, int degree) :
        center(c.x, c.y, c.z), scale(s), rotation(rot), alpha(a), sphericalHarmonics(sh_array)
    {
        double r = rotation.w;
        double x = rotation.x;
        double y = rotation.y;
        double z = rotation.z;
        glm::dmat3 R = glm::dmat3(
            1.f - 2.f * (y * y + z * z), 2.f * (x * y - r * z), 2.f * (x * z + r * y),
            2.f * (x * y + r * z), 1.f - 2.f * (x * x + z * z), 2.f * (y * z - r * x),
            2.f * (x * z - r * y), 2.f * (y * z + r * x), 1.f - 2.f * (x * x + y * y)
        );
        double scale_mod = 1;
        glm::dmat3 S(1.0f);
        S[0][0] = scale.x * scale_mod;
        S[1][1] = scale.y * scale_mod;
        S[2][2] = scale.z * scale_mod;

        glm::dmat3 M = S * R;
        sigma = glm::transpose(M) * M;
        invSigma = glm::inverse(sigma);       
        if (glm::any(glm::isnan(invSigma[0])) || glm::any(glm::isnan(invSigma[1])) || glm::any(glm::isnan(invSigma[2])))
        {
            spdlog::info("NANS IN INVSIGMA");
            valid = false;
        }
        detSigma = glm::determinant(sigma);


        if (detSigma <= 0.0)
        {
            spdlog::error("Somehow the determinant of sigma when creating the splat was negative");
            valid = false;
        }
        rvec = sigma * scale; //R * scale;
        

        auto mean_scale = std::pow(std::abs(scale.x * scale.y * scale.z), 1.0f / 3.0f);
        rho_0 = -std::log(1.0f - alpha) / (sqrt2pi * mean_scale);

        normConst = (splat_scale / ( std::pow(2 * pi, 3.0 / 2.0) * std::sqrt(detSigma))) * alpha;

        
        double eps = 0.01;
        double k = std::sqrt(-2.0 * std::log(eps)); // radius of ellipsoid in "S" units

        // ellipsoid semi-axes in world space = k * s_x, k * s_y, k * s_z along R's columns
        glm::dvec3 axis0 = k * scale.x * glm::dvec3(R[0]); // R's basis vectors (columns)
        glm::dvec3 axis1 = k * scale.y * glm::dvec3(R[1]);
        glm::dvec3 axis2 = k * scale.z * glm::dvec3(R[2]);

        // AABB half-extents = sum of |component| along each world axis
        glm::dvec3 halfExtent(
            std::abs(axis0.x) + std::abs(axis1.x) + std::abs(axis2.x),
            std::abs(axis0.y) + std::abs(axis1.y) + std::abs(axis2.y),
            std::abs(axis0.z) + std::abs(axis1.z) + std::abs(axis2.z)
        );

        bbox = aabb(
            center - vec3(halfExtent),
            center + vec3(halfExtent)
        );
        outline_mat = make_shared<diffuse_light>(color(1, 1, 1));
        mat = make_shared<shMaterial>( sphericalHarmonics, degree );
    }

    double probOfHit(const ray& r, interval ray_t, interval t, double A, double B, double C) const
    {
        // probability of a ray being scattered by a gaussian
        // ray = x + tw
        // gaussian: C exp( -1/2 * dot ( (x_t - u), invSigma * (x_t - u) ) )
        // prob is integral from t0 to t1 (entry and exit of ray)
        // 

        /*
        Some formulas around this:
        x = origin of ray
        u = center of splat
        w = direction
        t_0 = ray_t.clamp(entry into splat)
        t_1 = ray_t.clamp(exit of splat)

        X = x - u
        A = dot(w, invSigma * w)
        B = dot(w, invSigma * X) + dot(X, invSigma * w)
        C = dot(X, invSigma * X)

        const = exp( (B^2 / 8A) - ( C/2) )
        erf_t1 = erf( sqrt(A/2)t_1 + B/(2sqrt(2A)) )
        erf_t0 = erf( sqrt(A/2)t_0 + B/(2sqrt(2A)) )
        prob_scatter = const * sqrt(pi / 2A) * ( erf_t1 - erf_t0 )

        */
        double t_0 = t.min;
        double t_1 = t.max;
        double constant = std::exp(((B * B) / (8.0 * A)) - (C / 2.0));
        double sqrtA2 = std::sqrt(A / 2);
        double B2sqrt2A = B / (2.0 * std::sqrt(2.0 * A));
        double erf_t1 = std::erf((sqrtA2 * t_1) + B2sqrt2A);
        double erf_t0 = std::erf((sqrtA2 * t_0) + B2sqrt2A);
        return normConst * constant * std::sqrt(pi / (2 * A)) * (erf_t1 - erf_t0);
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override 
    {
        if (!valid)
        {
            // skip malformed splats
            return false;
        }
        glm::dvec3 x = vec3::toVec3(r.origin());
        glm::dvec3 u = vec3::toVec3(center);
        glm::dvec3 w = vec3::toVec3(r.direction());

        glm::dvec3 X = x - u;
        double A = glm::dot(w, invSigma * w);
        double B = glm::dot(w, invSigma * X) + glm::dot(X, invSigma * w);
        double C = glm::dot(X, invSigma * X);
        
        double eps = 0.01;
        double k2 = -2.0 * std::log(eps);
        double a = A;
        double b = B;
        double c = C - k2;
        double discriminant = (b * b) - (4 * a * c);
        double t_0 = 0.0, t_1 = 0.0;

        if (discriminant >= 0.0 && a > 0.0) {
            double sqrtDisc = std::sqrt(discriminant);
            t_0 = ray_t.clamp((-b - sqrtDisc) / (2.0 * a));
            t_1 = ray_t.clamp((-b + sqrtDisc) / (2.0 * a));
        }
        else
        {
            // missed the ellipsoid
            return false;
        }

        interval t(t_0, t_1);

        double probHit = probOfHit(r, ray_t, t, A, B, C);
        // TODO: the return value is NOT a real probability and can be > 1


        if (probHit == 0.0 || probHit != probHit)
        {
            // sometimes malformed splats create nans (probably they got scaled to zero or something)
            return false;
        }

        double rand = random_double();
        if ( probHit < rand)
        {
            return false;
        }
        
        /*
        calculate time as somewhere between t0 and t1
        might as well use rand since its already calculated
        
        */

        double t_norm = rand / probHit;

        double normRay_t = t_0 + (t_norm * (t_1 - t_0));
        double real_t = normRay_t;
        if (real_t != real_t)
        {
            spdlog::info("returned time was NaN");
            return false;
        }
        rec.t = real_t;
        
        rec.p = r.at(rec.t);
        double test = dot(rec.p, rec.p);
        if (test != test)
        {
            spdlog::info("rec.p was nan when colliding with splat");
        }
        vec3 outward_normal = (rec.p - center);

        outward_normal /= outward_normal.length();
                
        rec.set_face_normal(r, outward_normal);
        get_sphere_uv(outward_normal, rec.u, rec.v);
        
        rec.mat = mat;
        return true;
    }

    aabb bounding_box() const override { return bbox; }

    double pdf_value(const point3& origin, const vec3& direction) const override {

        hit_record rec;
        if (!this->hit(ray(origin, direction), interval(0.001, infinity), rec))
            return 0;

        auto dist_squared = (center - origin).length_squared();
        auto cos_theta_max = std::sqrt(1 - radius * radius / dist_squared);
        auto solid_angle = 2 * pi * (1 - cos_theta_max);

        return  1 / solid_angle;
    }

    vec3 random(const point3& origin) const override {
        vec3 direction = center - origin;
        auto distance_squared = direction.length_squared();
        onb uvw(direction);
        return uvw.transform(random_to_sphere(radius, distance_squared));
    }

};


namespace plyDetail {

    struct Property {
        std::string name;
        std::string type;
        std::size_t offset = 0;
        std::size_t size = 0;
    };

    inline std::size_t scalarSize(const std::string& type) {
        if (type == "char" || type == "uchar" || type == "int8" || type == "uint8") {
            return 1;
        }
        if (type == "short" || type == "ushort" || type == "int16" || type == "uint16") {
            return 2;
        }
        if (type == "int" || type == "uint" || type == "float" || type == "int32" || type == "uint32" ||
            type == "float32") {
            return 4;
        }
        if (type == "double" || type == "float64") {
            return 8;
        }
        throw std::runtime_error("Unsupported PLY scalar type: " + type);
    }

    template <typename T> inline T readScalar(const unsigned char* bytes) {
        T value;
        std::memcpy(&value, bytes, sizeof(T));
        return value;
    }

    inline float readAsFloat(const std::vector<unsigned char>& row, const Property& property) {
        const unsigned char* ptr = row.data() + property.offset;
        const std::string& type = property.type;

        if (type == "float" || type == "float32")
            return readScalar<float>(ptr);
        if (type == "double" || type == "float64")
            return static_cast<float>(readScalar<double>(ptr));
        if (type == "char" || type == "int8")
            return static_cast<float>(readScalar<std::int8_t>(ptr));
        if (type == "uchar" || type == "uint8")
            return static_cast<float>(readScalar<std::uint8_t>(ptr));
        if (type == "short" || type == "int16")
            return static_cast<float>(readScalar<std::int16_t>(ptr));
        if (type == "ushort" || type == "uint16")
            return static_cast<float>(readScalar<std::uint16_t>(ptr));
        if (type == "int" || type == "int32")
            return static_cast<float>(readScalar<std::int32_t>(ptr));
        if (type == "uint" || type == "uint32")
            return static_cast<float>(readScalar<std::uint32_t>(ptr));

        throw std::runtime_error("Unsupported PLY scalar type: " + type);
    }

    inline std::vector<std::string> splitWords(const std::string& line) {
        std::vector<std::string> words;
        std::string word;
        for (char c : line) {
            if (c == ' ' || c == '\t' || c == '\r') {
                if (!word.empty()) {
                    words.push_back(word);
                    word.clear();
                }
            }
            else {
                word.push_back(c);
            }
        }
        if (!word.empty()) {
            words.push_back(word);
        }
        return words;
    }

    inline const Property& requireProperty(const std::unordered_map<std::string, std::size_t>& lookup,
        const std::vector<Property>& properties,
        const std::string& name) {
        auto it = lookup.find(name);
        if (it == lookup.end()) {
            throw std::runtime_error("Missing required vertex property: " + name);
        }
        return properties[it->second];
    }

} // namespace plyDetail

inline hittable_list loadPly(const std::string& path) {
    auto logger = spdlog::get("ply-loader");
    if (!logger)
    {
        logger = spdlog::stdout_color_mt("ply-loader");
    }
    logger->info("Trying to load {}", path);
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Could not open PLY file: " + path);
    }

    std::string line;
    if (!std::getline(file, line) || line != "ply") {
        throw std::runtime_error("Not a PLY file: " + path);
    }

    bool sawFormat = false;
    bool inVertex = false;
    std::size_t vertexCount = 0;
    std::size_t rowStride = 0;
    std::vector<plyDetail::Property> properties;

    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line == "end_header") {
            break;
        }

        std::vector<std::string> words = plyDetail::splitWords(line);
        if (words.empty() || words[0] == "comment") {
            continue;
        }

        if (words[0] == "format") {
            if (words.size() < 3 || words[1] != "binary_little_endian") {
                throw std::runtime_error("Only binary_little_endian PLY files are supported");
            }
            sawFormat = true;
            continue;
        }

        if (words[0] == "element") {
            if (words.size() < 3) {
                throw std::runtime_error("Malformed PLY element line: " + line);
            }
            inVertex = words[1] == "vertex";
            if (inVertex) {
                vertexCount = static_cast<std::size_t>(std::stoull(words[2]));
            }
            continue;
        }

        if (inVertex && words[0] == "property") {
            if (words.size() < 3 || words[1] == "list") {
                throw std::runtime_error("Only scalar vertex properties are supported");
            }

            plyDetail::Property property;
            property.name = words[2];
            property.type = words[1];
            property.offset = rowStride;
            property.size = plyDetail::scalarSize(property.type);
            rowStride += property.size;
            properties.push_back(property);
        }
    }

    if (!sawFormat) {
        throw std::runtime_error("PLY file is missing a format line");
    }
    if (vertexCount == 0) {
        throw std::runtime_error("PLY file has no vertex records");
    }

    std::unordered_map<std::string, std::size_t> lookup;
    for (std::size_t i = 0; i < properties.size(); ++i) {
        lookup[properties[i].name] = i;
    }

    const plyDetail::Property& x = plyDetail::requireProperty(lookup, properties, "x");
    const plyDetail::Property& y = plyDetail::requireProperty(lookup, properties, "y");
    const plyDetail::Property& z = plyDetail::requireProperty(lookup, properties, "z");
    const plyDetail::Property& dcR = plyDetail::requireProperty(lookup, properties, "f_dc_0");
    const plyDetail::Property& dcG = plyDetail::requireProperty(lookup, properties, "f_dc_1");
    const plyDetail::Property& dcB = plyDetail::requireProperty(lookup, properties, "f_dc_2");
    const plyDetail::Property& opacity = plyDetail::requireProperty(lookup, properties, "opacity");
    const plyDetail::Property& scale0 = plyDetail::requireProperty(lookup, properties, "scale_0");
    const plyDetail::Property& scale1 = plyDetail::requireProperty(lookup, properties, "scale_1");
    const plyDetail::Property& scale2 = plyDetail::requireProperty(lookup, properties, "scale_2");
    const plyDetail::Property& rot0 = plyDetail::requireProperty(lookup, properties, "rot_0");
    const plyDetail::Property& rot1 = plyDetail::requireProperty(lookup, properties, "rot_1");
    const plyDetail::Property& rot2 = plyDetail::requireProperty(lookup, properties, "rot_2");
    const plyDetail::Property& rot3 = plyDetail::requireProperty(lookup, properties, "rot_3");

    std::vector<const plyDetail::Property*> shRest;
    for (int i = 0; i < SH_REST_FLOAT_COUNT; ++i) {
        auto it = lookup.find("f_rest_" + std::to_string(i));
        if (it == lookup.end()) {
            break;
        }
        shRest.push_back(&properties[it->second]);
    }
    if (lookup.find("f_rest_" + std::to_string(SH_REST_FLOAT_COUNT)) != lookup.end()) {
        throw std::runtime_error(
            "PLY has more spherical harmonic coefficients than this tutorial loads");
    }

    if (shRest.size() % SH_CHANNEL_COUNT != 0) {
        throw std::runtime_error(
            "PLY f_rest_* properties must contain the same count per RGB channel");
    }

    const std::size_t restCountPerChannel = shRest.size() / SH_CHANNEL_COUNT;

    hittable_list result;
    std::vector<unsigned char> row(rowStride);
    for (std::size_t i = 0; i < vertexCount; ++i) {
        file.read(reinterpret_cast<char*>(row.data()), static_cast<std::streamsize>(row.size()));
        if (!file) {
            throw std::runtime_error("PLY ended before all vertex records were read");
        }

        // TODO: change this constructor to move the splat.update() function into the constructor
        glm::vec3 center(
            plyDetail::readAsFloat(row, x),
            plyDetail::readAsFloat(row, y),
            plyDetail::readAsFloat(row, z)
        );
        std::array<float, SH_FLOAT_COUNT> sphericalHarmonics = {};
        
        
        sphericalHarmonics[0] = plyDetail::readAsFloat(row, dcR);
        sphericalHarmonics[1] = plyDetail::readAsFloat(row, dcG);
        sphericalHarmonics[2] = plyDetail::readAsFloat(row, dcB);
        for (std::size_t channel = 0; channel < SH_CHANNEL_COUNT; ++channel) {
            for (std::size_t coeff = 0; coeff < restCountPerChannel; ++coeff) {
                std::size_t plyRestIndex = channel * restCountPerChannel + coeff;
                std::size_t splatCoeff = coeff + 1;
                std::size_t splatIndex = splatCoeff * SH_CHANNEL_COUNT + channel;
                sphericalHarmonics[splatIndex] =
                    plyDetail::readAsFloat(row, *shRest[plyRestIndex]);
            }
        }
        float raw_alpha = plyDetail::readAsFloat(row, opacity);
        float alpha = 1.0 / (1.0 + std::exp(-raw_alpha));
        glm::vec3 scale(
            std::exp(plyDetail::readAsFloat(row, scale0)),
            std::exp(plyDetail::readAsFloat(row, scale1)),
            std::exp(plyDetail::readAsFloat(row, scale2))
        );

        glm::quat rotation(
            plyDetail::readAsFloat(row, rot0),
            plyDetail::readAsFloat(row, rot1),
            plyDetail::readAsFloat(row, rot2),
            plyDetail::readAsFloat(row, rot3)
        );
        
        splat gs(
            center, scale, rotation, alpha, sphericalHarmonics, SH_REST_FLOAT_COUNT / restCountPerChannel
        );
        result.add(make_shared<splat>(gs));
    }
    logger->info("Finished loading {}", path);
    return result;
}
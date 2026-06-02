#pragma once
#include "hittable.h"
#include "material.h"
#include <thread>
#include <future>
#include <chrono>

using namespace std::chrono_literals;

class camera {
public:
    double aspect_ratio = 1.0;  // Ratio of image width over height
    int    image_width = 100;  // Rendered image width in pixel count
    int    samples_per_pixel = 10;   // Count of random samples for each pixel
    int    max_depth = 10;   // Maximum number of ray bounces into scene
    double vfov = 90;  // Vertical view angle (field of view)

    point3 lookfrom = point3(0, 0, 0);   // Point camera is looking from
    point3 lookat = point3(0, 0, -1);  // Point camera is looking at
    vec3   vup = vec3(0, 1, 0);     // Camera-relative "up" direction

    double defocus_angle = 0;  // Variation angle of rays through each pixel
    double focus_dist = 10;    // Distance from camera lookfrom point to plane of perfect focus

    int num_threads = 16;

    int render_chunk(const hittable& world, interval chunk_width, interval chunk_height, std::vector<color>& outputBuffer)
    {
        std::string name = "cthread_" + std::to_string(int(chunk_width.min));
        auto logger = spdlog::get(name);
        if (!logger)
        {
            logger = spdlog::stdout_color_mt(name);
        }
        int width = int(chunk_width.size());
		logger->info("Thread {} running on chunk width ({}, {}) and height ({}, {})", name, chunk_width.min, chunk_width.max, chunk_height.min, chunk_height.max);
        // this is the original render code except height, width, and both starts are determined by the chunk_width/height
        for (int j = int(chunk_height.min); j < int(chunk_height.max); j++) for (int i = int(chunk_width.min); i < int(chunk_width.max); i++)
        {
            if ((j * chunk_width.size()) + i % 100 == 0)
            {
                logger->info("chunk progress: {} / {}", ((j - chunk_height.min) * chunk_width.size()) + (i - chunk_width.min), chunk_width.size());
            }
            color pixel_color(0, 0, 0);
            for (int sample = 0; sample < samples_per_pixel; sample++) {
                ray r = get_ray(i, j);
                pixel_color += ray_color(r, max_depth, world);
            }
            int out_index = int(i + (j * image_width));
            if (out_index >= outputBuffer.size())
            {
                logger->warn("Trying to write outside the image! {}", out_index);
            }
            else
            {
                outputBuffer[out_index] = pixel_samples_scale * pixel_color;
            }
        }
        return int(chunk_width.min);
    }

    template <typename HITTABLE>
    void render(const HITTABLE& world) {
        initialize();

        chunkBuffer.resize(image_width * image_height);
        logger->info("Chunk buffer has {} elements", chunkBuffer.size());
        int chunk_size = image_width / num_threads;
        int extra = 0;
        for (int t = 0; t < num_threads; t++)
        {
            if (t == num_threads - 1) extra = image_width % num_threads;
            int chunk_min = t * chunk_size, chunk_max = ((t + 1) * chunk_size) + extra;
            logger->info("Building a thread to go from ({}, {})", chunk_min, chunk_max);
            
            threads.push_back(
                std::async(
                    std::launch::async,
                    [this, &world, chunk_min, chunk_max]()
                    {
                        return this->render_chunk(world, interval(chunk_min, chunk_max), interval(0, this->image_height), this->chunkBuffer);
                    }
                ).share()
            );
        }
        
        std::ofstream outputImage("output.ppm");

        if (!outputImage.is_open())
        {
            logger->error("File stream for output.ppm was unable to be opened");
            return;
        }
        outputImage << "P3\n" << image_width << ' ' << image_height << "\n255\n";

        while (!threads.empty())
        {
            auto t = threads.back();
            threads.pop_back();
            if (t.wait_for(0s) == std::future_status::ready)
            {
                auto n = t.get();
                logger->info("Joined thread: {}", n);
            }
            else
            {
                threads.insert(threads.begin(), t);
            }
        }
        
        logger->info("Thread joined, writing file");
        logger->info("Chunk buffer has {} pixels", chunkBuffer.size());
        for (auto pixel : chunkBuffer)
        {
            // already color corrected
            write_color(outputImage, pixel);
        }
        outputImage.close();
    }

private:
    /* Private Camera Variables Here */

    int    image_height;   // Rendered image height
    point3 center;         // Camera center
    point3 pixel00_loc;    // Location of pixel 0, 0
    vec3   pixel_delta_u;  // Offset to pixel to the right
    vec3   pixel_delta_v;  // Offset to pixel below
    double pixel_samples_scale;  // Color scale factor for a sum of pixel samples
    vec3   u, v, w;              // Camera frame basis vectors
    vec3   defocus_disk_u;       // Defocus disk horizontal radius
    vec3   defocus_disk_v;       // Defocus disk vertical radius
    std::vector<color> renderBuffer;

    shared_ptr<spdlog::logger> logger;

    std::vector<color> chunkBuffer;
    std::vector<std::shared_future<int>> threads;

    void initialize() {
        // TODO: switch this to factory that reuses other camera loggers
        logger = spdlog::stdout_color_mt("camera");

        image_height = int(image_width / aspect_ratio);
        image_height = (image_height < 1) ? 1 : image_height;

        pixel_samples_scale = 1.0 / samples_per_pixel;

        center = lookfrom;

        auto theta = degrees_to_radians(vfov);
        auto h = std::tan(theta / 2);
        auto viewport_height = 2 * h * focus_dist;
        auto viewport_width = viewport_height * (double(image_width) / image_height);

        // Calculate the u,v,w unit basis vectors for the camera coordinate frame.
        w = unit_vector(lookfrom - lookat);
        u = unit_vector(cross(vup, w));
        v = cross(w, u);

        // Calculate the vectors across the horizontal and down the vertical viewport edges.
        vec3 viewport_u = viewport_width * u;    // Vector across viewport horizontal edge
        vec3 viewport_v = viewport_height * -v;  // Vector down viewport vertical edge

        // Calculate the horizontal and vertical delta vectors from pixel to pixel.
        pixel_delta_u = viewport_u / image_width;
        pixel_delta_v = viewport_v / image_height;

        // Calculate the location of the upper left pixel.
        auto viewport_upper_left = center - (focus_dist * w) - viewport_u / 2 - viewport_v / 2;
        pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);

        // Calculate the camera defocus disk basis vectors.
        auto defocus_radius = focus_dist * std::tan(degrees_to_radians(defocus_angle / 2));
        defocus_disk_u = u * defocus_radius;
        defocus_disk_v = v * defocus_radius;
    }

    color ray_color(const ray& r, int depth, const hittable& world) const {
        // If we've exceeded the ray bounce limit, no more light is gathered.
        if (depth <= 0)
            return color(0, 0, 0);
        
        hit_record rec;

        if (world.hit(r, interval(0.0001, infinity), rec)) {
            ray scattered;
            color attenuation;
            if (rec.mat->scatter(r, rec, attenuation, scattered))
                return attenuation * ray_color(scattered, depth - 1, world);
            return color(0, 0, 0);
        }

        vec3 unit_direction = unit_vector(r.direction());
        auto a = 0.5 * (unit_direction.y() + 1.0);
        return (1.0 - a) * color(1.0, 1.0, 1.0) + a * color(0.5, 0.7, 1.0);
    }

    ray get_ray(int i, int j) const {
        // Construct a camera ray originating from the origin and directed at randomly sampled
        // point around the pixel location i, j.

        auto offset = sample_square();
        auto pixel_sample = pixel00_loc
            + ((i + offset.x()) * pixel_delta_u)
            + ((j + offset.y()) * pixel_delta_v);

        auto ray_origin = (defocus_angle <= 0) ? center : defocus_disk_sample();

        auto ray_direction = pixel_sample - ray_origin;

        auto ray_time = random_double();

        return ray(ray_origin, ray_direction, ray_time);
    }

    point3 defocus_disk_sample() const {
        // Returns a random point in the camera defocus disk.
        auto p = random_in_unit_disk();
        return center + (p[0] * defocus_disk_u) + (p[1] * defocus_disk_v);
    }

    vec3 sample_square() const {
        // Returns the vector to a random point in the [-.5,-.5]-[+.5,+.5] unit square.
        return vec3(random_double() - 0.5, random_double() - 0.5, 0);
    }

};
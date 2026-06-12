#pragma once
#include "hittable.h"
#include "material.h"
#include "pdf.h"

#include <thread>
#include <future>
#include <chrono>

#include <boost/lockfree/queue.hpp>

using namespace std::chrono_literals;

struct chunk {
    interval width;
	interval height;
};

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

    color  background;               // Scene background color

    int num_threads = 16;
	int chunk_size = 16;

    int render_thread(const hittable& world, const hittable& lights, boost::lockfree::queue<chunk>& chunkQueue, std::vector<color>& outputBuffer, int id)
    {
        std::string name = "cthread_" + std::to_string(id);
        auto logger = spdlog::get(name);
        if (!logger)
        {
            logger = spdlog::stdout_color_mt(name);
        }
        chunk section;
        while (chunkQueue.pop(section))
        {
            render_chunk(world, lights, section.width, section.height, outputBuffer);
        }
        logger->info("Chunk queue is empty, exiting thread {}", id);
        return id;
    }

    int render_chunk(const hittable& world, const hittable& lights, interval chunk_width, interval chunk_height, std::vector<color>& outputBuffer)
    {
        
        int width = int(chunk_width.size());
        // this is the original render code except height, width, and both starts are determined by the chunk_width/height
        for (int j = int(chunk_height.min); j < int(chunk_height.max); j++) for (int i = int(chunk_width.min); i < int(chunk_width.max); i++)
        {
            color pixel_color(0, 0, 0);
            for (int s_j = 0; s_j < sqrt_spp; s_j++) {
                for (int s_i = 0; s_i < sqrt_spp; s_i++) {
                    ray r = get_ray(i, j, s_i, s_j);
                    auto returned_color = ray_color(r, max_depth, world, lights);
                    if (returned_color.x() < 0 || returned_color.y() < 0 || returned_color.z() < 0)
                    {
                        logger->info("Trying to add a color that has a component less than zero!");
                    }
                    else
                    {
                        pixel_color += returned_color;
                    }
                }
            }
            int out_index = int(i + (j * image_width));
            if (out_index >= outputBuffer.size())
            {
                logger->warn("Trying to write outside the image! {}", out_index);
				logger->warn("Chunk width: {} - {}, chunk height: {} - {}, image width: {}, image height: {}", chunk_width.min, chunk_width.max, chunk_height.min, chunk_height.max, image_width, image_height);
            }
            else
            {
                outputBuffer[out_index] = pixel_samples_scale * pixel_color;
            }
        }
		logger->info("Finished rendering chunk width: {} - {}, height: {} - {}", chunk_width.min, chunk_width.max, chunk_height.min, chunk_height.max);
        return int(chunk_width.min);
    }

    template <typename HITTABLE>
    void render(const HITTABLE& world, const hittable& lights) {
        initialize();

        chunkBuffer.resize(image_width * image_height);
        logger->info("Chunk buffer has {} elements", chunkBuffer.size());
        int extra_width = 0;
		int extra_height = 0;

		unsigned int xchunks = image_width / chunk_size;
		unsigned int ychunks = image_height / chunk_size;

		logger->info("Image will be rendered in {} x {} chunks", xchunks, ychunks);
        boost::lockfree::queue<chunk> chunkQueue{ xchunks * ychunks };
        
		for (int i = 0; i < image_width; i+=chunk_size) for (int j = 0; j < image_height; j+=chunk_size)
        {
            if (i + chunk_size > image_width)
            {
                extra_width = image_width - (i + chunk_size);
				logger->info("Chunk at width {} exceeds image width, adding extra width of {}", i, extra_width);
            }
            else
            {
				extra_width = 0;
            }
            if (j + chunk_size > image_height)
            {
                extra_height = image_height - (j + chunk_size);
                logger->info("Chunk at height {} exceeds image height, adding extra height of {}", j, extra_height);
            }
            else
            {
				extra_height = 0;
            }
			chunkQueue.push(
                chunk(interval(i, i + chunk_size + extra_width), interval(j, j + chunk_size + extra_height))
            );
        }

        for (int t = 0; t < num_threads; t++)
        {
            //if (t == num_threads - 1) extra = image_width % num_threads;
            //int chunk_min = t * chunk_size, chunk_max = ((t + 1) * chunk_size) + extra;
            logger->info("Building thread: {}", t);
            
            threads.push_back(
                std::async(
                    std::launch::async,
                    [this, &world, &lights, &chunkQueue, t]()
                    {
						return this->render_thread(world, lights, chunkQueue, this->chunkBuffer, t);
                        //return this->render_chunk(world, interval(chunk_min, chunk_max), interval(0, this->image_height), this->chunkBuffer);
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
        
        logger->info("Threads joined, writing file");
        logger->info("Chunk buffer has {} pixels", chunkBuffer.size());
        for (const color& pixel : chunkBuffer)
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

    int    sqrt_spp;             // Square root of number of samples per pixel
    double recip_sqrt_spp;       // 1 / sqrt_spp

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

        sqrt_spp = int(std::sqrt(samples_per_pixel));
        pixel_samples_scale = 1.0 / (sqrt_spp * sqrt_spp);
        recip_sqrt_spp = 1.0 / sqrt_spp;

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

    color ray_color(const ray& r, int depth, const hittable& world, const hittable& lights)
        const {
        // If we've exceeded the ray bounce limit, no more light is gathered.
        if (depth <= 0)
        {
            //logger->info("Max ray depth reached, returning red");
            return color(0, 0, 0);
        }

        hit_record rec;

        // If the ray hits nothing, return the background color.
        if (!world.hit(r, interval(0.001, infinity), rec))
            return background;

        scatter_record srec;
        color color_from_emission = rec.mat->emitted(r, rec, rec.u, rec.v, rec.p);

        if (!rec.mat->scatter(r, rec, srec))
            return color_from_emission;

        if (srec.skip_pdf) {
            return srec.attenuation * ray_color(srec.skip_pdf_ray, depth - 1, world, lights);
        }

        auto light_ptr = make_shared<hittable_pdf>(lights, rec.p);
        mixture_pdf p(light_ptr, srec.pdf_ptr);

        ray scattered = ray(rec.p, p.generate(), r.time());
        auto pdf_value = p.value(scattered.direction());

        double scattering_pdf = rec.mat->scattering_pdf(r, rec, scattered);

        color sample_color = ray_color(scattered, depth - 1, world, lights);
        color color_from_scatter =
            (srec.attenuation * scattering_pdf * sample_color) / pdf_value;

        
        return color_from_emission + color_from_scatter;
    }

    ray get_ray(int i, int j, int s_i, int s_j) const {
        // Construct a camera ray originating from the defocus disk and directed at a randomly
        // sampled point around the pixel location i, j for stratified sample square s_i, s_j.

        auto offset = sample_square_stratified(s_i, s_j);
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

    vec3 sample_square_stratified(int s_i, int s_j) const {
        // Returns the vector to a random point in the square sub-pixel specified by grid
        // indices s_i and s_j, for an idealized unit square pixel [-.5,-.5] to [+.5,+.5].

        auto px = ((s_i + random_double()) * recip_sqrt_spp) - 0.5;
        auto py = ((s_j + random_double()) * recip_sqrt_spp) - 0.5;

        return vec3(px, py, 0);
    }

    vec3 sample_square() const {
        // Returns the vector to a random point in the [-.5,-.5]-[+.5,+.5] unit square.
        return vec3(random_double() - 0.5, random_double() - 0.5, 0);
    }

};
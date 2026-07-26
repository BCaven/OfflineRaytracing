#include "rtweekend.h"
#include "camera.h"
#include "hittable.h"
#include "hittable_list.h"
#include "sphere.h"
#include "material.h"
#include "bvh.h"
#include "quad.h"
#include "triangle.h"
#include "mesh.h"
#include "constant_medium.h"
#include "splat.h"

#include <chrono>


#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

int bouncing_spheres()
{
    hittable_list world;

    auto checker = make_shared<checker_texture>(0.32, color(.2, .3, .1), color(.9, .9, .9));
    world.add(make_shared<sphere>(point3(0, -1000, 0), 1000, make_shared<lambertian>(checker)));


    for (int a = -5; a < 5; a++) {
        for (int b = -5; b < 5; b++) {
            auto choose_mat = random_double();
            point3 center(a + 0.9 * random_double(), 0.2, b + 0.9 * random_double());

            if ((center - point3(4, 0.2, 0)).length() > 0.9) {
                shared_ptr<material> sphere_material;

                if (choose_mat < 0.8) {
                    // diffuse
                    auto albedo = color::random() * color::random();
                    sphere_material = make_shared<lambertian>(albedo);
                    auto center2 = center + vec3(0, 0, random_double(-0.2, 0.2));
                    world.add(make_shared<sphere>(center, center2, 0.2, sphere_material));
                }
                else if (choose_mat < 0.95) {
                    // metal
                    auto albedo = color::random(0.5, 1);
                    auto fuzz = random_double(0, 0.5);
                    sphere_material = make_shared<metal>(albedo, fuzz);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                }
                else {
                    // glass
                    sphere_material = make_shared<dielectric>(1.5);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                }
            }
        }
    }

    auto material1 = make_shared<dielectric>(1.5);
    world.add(make_shared<sphere>(point3(0, 1, 0), 1.0, material1));

    auto material2 = make_shared<lambertian>(color(0.4, 0.2, 0.1));
    world.add(make_shared<sphere>(point3(-4, 1, 0), 1.0, material2));

    auto material3 = make_shared<metal>(color(0.7, 0.6, 0.5), 0.0);
    world.add(make_shared<sphere>(point3(4, 1, 0), 1.0, material3));

    world = hittable_list(make_shared<bvh_node>(world));


    camera cam;

    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 540;
    cam.samples_per_pixel = 1;
    cam.max_depth = 10;

    cam.vfov = 20;
    cam.lookfrom = point3(13, 2, 3);
    cam.lookat = point3(0, 0, 0);
    cam.vup = vec3(0, 1, 0);

    cam.defocus_angle = 0.6;
    cam.focus_dist = 10.0;
    cam.background = color(0.70, 0.80, 1.00);
    cam.output_file = "output/output.ppm";

    auto empty_material = shared_ptr<material>();
    quad lights(point3(0, 2, 0), vec3(-130, 0, 0), vec3(0, 0, -105), empty_material);

    cam.render(world, lights);

	return 0;
}

void checkered_spheres() {
    hittable_list world;
    hittable_list lights;

    auto checker = make_shared<checker_texture>(0.32, color(.2, .3, .1), color(.9, .9, .9));

    world.add(make_shared<sphere>(point3(0, -10, 0), 10, make_shared<lambertian>(checker)));
    world.add(make_shared<sphere>(point3(0, 10, 0), 10, make_shared<lambertian>(checker)));
    auto light = make_shared<sphere>(point3(0, 0, 10), 10, make_shared<diffuse_light>(color(1, 1, 1)));

    world.add(light);
    lights.add(light);

    camera cam;

    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 540;
    cam.samples_per_pixel = 1;
    cam.max_depth = 10;

    cam.vfov = 20;
    cam.lookfrom = point3(13, 2, 3);
    cam.lookat = point3(0, 0, 0);
    cam.vup = vec3(0, 1, 0);

    cam.defocus_angle = 0.6;
    cam.focus_dist = 10.0;
    cam.background = color(0.70, 0.80, 1.00);
    cam.output_file = "output/output.ppm";

    cam.render(world, lights);
}

void earth() {

    hittable_list world;


    auto earth_texture = make_shared<image_texture>("earthmap.jpg");
    auto earth_surface = make_shared<lambertian>(earth_texture);
    world.add(make_shared<sphere>(point3(2, 0, 0), 2, earth_surface));

    auto blender_texture = make_shared<image_texture>("default_texture.jpg");
	auto blender_surface = make_shared<lambertian>(blender_texture);
	world.add(make_shared<sphere>(point3(-2, 0, 0), 2, blender_surface));



    camera cam;

    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth = 50;

    cam.vfov = 20;
    cam.lookfrom = point3(0, 0, 12);
    cam.lookat = point3(0, 0, 0);
    cam.vup = vec3(0, 1, 0);

    cam.defocus_angle = 0;
    cam.background = color(0.70, 0.80, 1.00);

    auto empty_material = shared_ptr<material>();
    quad lights(point3(343, 554, 332), vec3(-130, 0, 0), vec3(0, 0, -105), empty_material);

    cam.render(world, lights);
}

void perlin_spheres() {
    hittable_list world;

    auto pertext = make_shared<noise_texture>(4);
    world.add(make_shared<sphere>(point3(0, -1000, 0), 1000, make_shared<lambertian>(pertext)));
    world.add(make_shared<sphere>(point3(0, 2, 0), 2, make_shared<lambertian>(pertext)));

    camera cam;

    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 540;
    cam.samples_per_pixel = 200;
    cam.max_depth = 50;

    cam.vfov = 20;
    cam.lookfrom = point3(13, 2, 3);
    cam.lookat = point3(0, 0, 0);
    cam.vup = vec3(0, 1, 0);

    cam.defocus_angle = 0;
    cam.background = color(0.70, 0.80, 1.00);

    auto empty_material = shared_ptr<material>();
    quad lights(point3(343, 554, 332), vec3(-130, 0, 0), vec3(0, 0, -105), empty_material);

    cam.render(world, lights);
}

void quads() {
    hittable_list world;

    // Materials
    //auto blender_texture = make_shared<image_texture>("default_texture.jpg");
    //auto blender_surface = make_shared<lambertian>(blender_texture);


    auto left_red = make_shared<lambertian>(color(1.0, 0.2, 0.2));
    auto back_green = make_shared<lambertian>(color(0.2, 1.0, 0.2));
    auto right_blue = make_shared<lambertian>(color(0.2, 0.2, 1.0));
    auto upper_orange = make_shared<lambertian>(color(1.0, 0.5, 0.0));
    auto lower_teal = make_shared<lambertian>(color(0.2, 0.8, 0.8));

    
    world.add(make_shared<triangle>(
        vertex(
            point3(0, 0, 0), 
            vec3(0, 0, -1), 
            vec3(0, 0, 0)
        ),
        vertex(point3(0, 5, 0), vec3(0, 0, -1), vec3(0, 1, 0)),
        vertex(point3(5, 0, 0), vec3(0, 0, -1), vec3(1, 0, 0)),
        left_red
    ));
    
    world.add(make_shared<quad>(point3(-2, -2, 0), vec3(4, 0, 0), vec3(0, 4, 0), back_green));
    world.add(make_shared<quad>(point3(3, -2, 1), vec3(0, 0, 4), vec3(0, 4, 0), right_blue));
    world.add(make_shared<quad>(point3(-2, 3, 1), vec3(4, 0, 0), vec3(0, 0, 4), upper_orange));
    world.add(make_shared<quad>(point3(-2, -3, 5), vec3(4, 0, 0), vec3(0, 0, -4), lower_teal));

    camera cam;

    cam.aspect_ratio = 1.0;
    cam.image_width = 540;
    cam.samples_per_pixel = 10;
    cam.max_depth = 5;

    cam.vfov = 80;
    cam.lookfrom = point3(0, 0, 9);
    cam.lookat = point3(0, 0, 0);
    cam.vup = vec3(0, 1, 0);

    cam.defocus_angle = 0;
    cam.background = color(0.70, 0.80, 1.00);
    cam.output_file = "output/output.ppm";

    auto empty_material = make_shared<diffuse_light>(color(1, 1, 1));
    quad lights(point3(343, 554, 332), vec3(-130, 0, 0), vec3(0, 0, -105), empty_material);
    world.add(make_shared<quad>(lights));

    cam.render(world, lights);
}

void meshes()
{
    hittable_list world;
    auto left_red = make_shared<lambertian>(color(1.0, 0.2, 0.2));

	auto m = mesh::fromFile("snowball.buvf", left_red);
    world.add(m);
    auto pertext = make_shared<noise_texture>(4);
    world.add(make_shared<sphere>(point3(0, -1010, 0), 1000, make_shared<lambertian>(pertext)));
    
    camera cam;

    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 540;
    cam.samples_per_pixel = 10;
    cam.max_depth = 10;

    cam.vfov = 20;
    cam.lookfrom = point3(13, 9, 3);
    cam.lookat = point3(0, 0, 0);
    cam.vup = vec3(0, 1, 0);

    cam.defocus_angle = 0.6;
    cam.focus_dist = 10.0;
    cam.background = color(0.70, 0.80, 1.00);
    cam.output_file = "output/output.ppm";

    auto empty_material = shared_ptr<material>();
    quad lights(point3(343, 554, 332), vec3(-130, 0, 0), vec3(0, 0, -105), empty_material);

    cam.render(world, lights);

}

void simple_light() {
    hittable_list world;

    auto pertext = make_shared<noise_texture>(4);
    world.add(make_shared<sphere>(point3(0, -1002, 0), 1000, make_shared<lambertian>(color(0.5, 0.5, 0.5))));
    shared_ptr<material> aluminum = make_shared<metal>(color(0.8, 0.85, 0.88), 0.0);

    auto difflight = make_shared<diffuse_light>(color(4, 4, 4));
    //auto snowballlight = make_shared<diffuse_light>(color(1, 0, 0));
    auto snowball_mat = make_shared< lambertian >(color(0.5, 0.5, 0.5));
    auto snowball_glass = make_shared< dielectric >(1.5);
    auto snowball_metal = make_shared< metal >(color(0.5, 0.5, 0.5), 0.1);
    auto m_mat = mesh::fromFile("snowball.buvf", snowball_mat);
    auto m_glass = mesh::fromFile("snowball.buvf", snowball_glass);
    auto m_metal = mesh::fromFile("snowball.buvf", snowball_metal);
    auto m_light = mesh::fromFile("snowball.buvf", difflight);

    for (int i = 0; i < 5; i++) for (int j = 0; j < 5; j++)
    {
        auto m = m_mat;
        auto choice = random_int(0, 3);
        switch (choice)
        {
        case 0:
            m = m_mat;
            break;
        case 1:
            m = m_glass;
            break;
        case 2:
            m = m_metal;
            break;
        case 3:
            m = mesh::fromFile("snowball.buvf", make_shared<diffuse_light>(color(random_double(), random_double(), random_double())));
            break;
        }
        world.add(make_shared<translate>(m, vec3(-30 + (8 * j), 0, -10 + (8 * i))));
    }


    

    // Glass Sphere
    //auto glass = make_shared<dielectric>(1.5);
    //world.add(make_shared<sphere>(point3(0, 0, 0), 3, glass));

    hittable_list lights;

    
    difflight = make_shared<diffuse_light>(color(2, 2, 4));
    auto rlight = make_shared<diffuse_light>(color(4, 2, 0));

    // lights, aka things that recieve more samples than anything else.
    //world.add(make_shared<sphere>(point3(50, 20, 0), 20, difflight));
    lights.add(make_shared<sphere>(point3(0, 20, -25), 20, difflight));
    lights.add(make_shared<sphere>(point3(0, 20, 35), 20, rlight));
    lights.add(make_shared<quad>(point3(343, 554, 332), vec3(-130, 0, 0), vec3(0, 0, -105), difflight));

    camera cam;

    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 600;
    cam.samples_per_pixel = 40;
    cam.max_depth = 50;
    cam.background = color(0, 0, 0);

    cam.vfov = 20;
    cam.lookfrom = point3(26, 8, 0);
    cam.lookat = point3(0, 1, 0);
    cam.vup = vec3(0, 1, 0);

    cam.defocus_angle = 0;
	cam.chunk_size = 16;
    cam.num_threads = 20;
    cam.output_file = "output/output.ppm";

    // make the lights actual things
    world.add(make_shared<hittable_list>(lights));

	auto bvh_world = bvh_node(world);
    cam.render(bvh_world, lights);
}

void gs_test(point3 cam_pos, std::string output_file_name)
{
    hittable_list world;
    hittable_list lights;
    // floor
    world.add(make_shared<sphere>(point3(100, -1002, 0), 1000, make_shared<lambertian>(color(0.5, 0.5, 0.5))));
    
    plyArgs args{};

    auto rawPly = loadPly("local/single_splat.ply", args);

    spdlog::info("Pre-BVH");
    auto bvhPly = make_shared<bvh_node>(rawPly);
    spdlog::info("Post-BVH");

    world.add(bvhPly);

    auto snow = make_shared<lambertian>(color(0.3, 0.3, 0.3));
    auto mirror = make_shared<metal>(color(1, 1, 1), 0.1);

    //auto m = mesh::fromFile("snowball.buvf", snow);
    //world.add(make_shared<translate>(m, vec3(-6, 0, 0)));

    //world.add(make_shared<quad>(vec3(0, 0, 20), vec3(20, 0, 0), vec3(0, 20, 0), mirror));
    auto difflight = make_shared<diffuse_light>(color(2, 2, 2));
    auto rlight = make_shared<diffuse_light>(color(4, 2, 0));

    // lights, aka things that recieve more samples than anything else.
    //world.add(make_shared<sphere>(point3(50, 20, 0), 20, difflight));
    lights.add(make_shared<sphere>(point3(0, 40, -45), 20, difflight));
    lights.add(make_shared<sphere>(point3(0, 40, 45), 20, difflight));
    lights.add(make_shared<quad>(point3(343, 554, 332), vec3(-130, 0, 0), vec3(0, 0, -105), difflight));

    camera cam;

    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 400;
    cam.samples_per_pixel = 10;
    cam.max_depth = 5;
    cam.background = color(0, 0, 0);

    cam.vfov = 40;
    cam.lookfrom = cam_pos;
    cam.lookat = point3(0, 1, 0);
    cam.vup = vec3(0, 1, 0);

    cam.defocus_angle = 0;
    cam.chunk_size = 20;
    cam.num_threads = 20;
    cam.output_file = output_file_name;

    // make the lights actual things
    world.add(make_shared<hittable_list>(lights));

    auto bvh_world = bvh_node(world);
    cam.render(bvh_world, lights);
}

void gs_video()
{
    hittable_list world;
    hittable_list lights;
    // floor
    //world.add(make_shared<sphere>(point3(100, -1001, 0), 1000, make_shared<lambertian>(color(0.5, 0.5, 0.5))));

    plyArgs splat1{ .bHit2 = false };
    plyArgs splat2{ .bHit2 = true };

    std::string splatFile = "local/testing_9.ply";

    auto rawPly1 = loadPly(splatFile, splat1);
    auto rawPly2 = loadPly(splatFile, splat2);


    spdlog::info("Pre-BVH");
    auto bvhPly1 = make_shared<bvh_node>(rawPly1);
    spdlog::info("Post-BVH");
    auto bvhPly2 = make_shared<bvh_node>(rawPly2);

    world.add(make_shared<translate>(bvhPly1, point3(0, 0, -0.1)));

    world.add(make_shared<translate>(bvhPly2, point3(0, 0, 0.1)));

    auto snow = make_shared<lambertian>(color(1, 1, 1));
    auto mirror = make_shared<metal>(color(1, 1, 1), 0.1);

    

    auto tri = make_shared<triangle>(vertex(point3(0, 2, 0), point3(1, 0, 0), point3()), vertex(point3(0, 0, 0), point3(1, 0, 0), point3()), vertex(point3(0, 0, -2), point3(1, 0, 0), point3()), mirror);

    //world.add(tri);

    //auto m = mesh::fromFile("snowball.buvf", snow);
    //world.add(make_shared<rotate_y>( make_shared<translate>(m, vec3(0, 0, 8)), 2 ));

    //world.add(make_shared<sphere>(  point3(0, 0, -14), 5, mirror  ));

    world.add(make_shared<quad>(vec3(-2, -2, 2), vec3(0, 0, -20), vec3(0, 20, 0), snow));
    auto difflight = make_shared<diffuse_light>(color(1, 1, 1));
    auto rlight = make_shared<diffuse_light>(color(1, 0.5, 0));

    // lights, aka things that recieve more samples than anything else.
    //world.add(make_shared<sphere>(point3(50, 20, 0), 20, difflight));
    lights.add(make_shared<sphere>(point3(0, 60, -60), 20, difflight));
    lights.add(make_shared<sphere>(point3(70, 60, 1), 20, rlight));

    //lights.add(make_shared<sphere>(point3(0, 60, 60), 20, difflight));
    //lights.add(make_shared<quad>(point3(343, 554, 332), vec3(-130, 0, 0), vec3(0, 0, -105), difflight));

    // make the lights actual things
    world.add(make_shared<bvh_node>(lights));
    //world.add(make_shared<quad>(point3(300, -300, -300), vec3(-600, 0, 0), vec3(0, 600, 0), snow));
    auto bvh_world = bvh_node(world);

    camera cam;
    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 200;
    cam.samples_per_pixel = 100;
    cam.max_depth = 20;
    cam.background = color(0, 0, 0);

    cam.vfov = 10;
    cam.lookat = point3(0, 0, 0);
    cam.vup = vec3(0, 1, 0);

    cam.defocus_angle = 0;
    cam.chunk_size = 10;
    cam.num_threads = 20;
    //cam.debug_depth = true;
    //cam.debug_skip_pdf = true;
    double dist = 1;
    
    for (double i = pi/4; i < (5 * pi) / 4; i += 10.2)
    {
        
        
        cam.lookfrom = point3(1, 0, 0);
        
        cam.output_file = "output/splat_demo_" + std::to_string(i) + ".ppm";


        cam.render(bvh_world, lights);
    }
}

void the_council() {
    // TODO: figure out why this scene has NaNs

    hittable_list world;
    hittable_list lights;

    world.add(make_shared<sphere>(point3(0, -1002, 0), 1000, make_shared<lambertian>(color(0.5, 0.5, 0.5))));

    auto difflight = make_shared<diffuse_light>(color(4, 4, 4));
    //auto snowballlight = make_shared<diffuse_light>(color(1, 0, 0));
	auto snowball_mat = make_shared< lambertian >(color(0.5, 0.5, 0.5));
    auto snowball_glass = make_shared< dielectric >(1.5);
    auto snowball_metal = make_shared< metal >(color(0.5, 0.5, 0.5), 0.1);

    auto m_mat = mesh::fromFile("snowball.buvf", snowball_mat);
    auto m_glass = mesh::fromFile("snowball.buvf", snowball_glass);
    auto m_metal = mesh::fromFile("snowball.buvf", snowball_metal);
    auto m_light = mesh::fromFile("snowball.buvf", difflight);

    double min_distance = 15.0;
    for (int i = 0; i < 360; i+=30) for (double j = min_distance; j < 30.0; j+=7.0)
    {
        auto m = m_mat;
        auto choice = random_int(0, 3);
        switch (choice)
        {
        case 0:
            m = m_mat;
            break;
        case 1:
            m = m_glass;
            break;
        case 2:
            m = m_metal;
            break;
        case 3:
            m = mesh::fromFile("snowball.buvf", make_shared<diffuse_light>(color(random_double(), random_double(), random_double())));
            break;
        }
		auto rad = degrees_to_radians(i);
        auto rotated = make_shared<rotate_y>(m, 100 + i);

        auto translated = make_shared<translate>(rotated, vec3(
            std::sin(rad) * j,
            0, 
            std::cos(rad) * j
        ));
        world.add(translated);
    }

    auto glass = make_shared<dielectric>(1.5);
	auto otherglass = make_shared<dielectric>(1.2);
    lights.add(make_shared<sphere>(
        point3(0, 2, 0),
        8,
        otherglass
    ));
    world.add(make_shared<sphere>(
        point3(0, 2, 0),
        7,
        glass
    ));
    auto ball = make_shared<sphere>(
        point3(0, 2, 0),
        7.5,
		make_shared<lambertian>(color(0.5, 0.0, 0.0))
    );

    //world.add(make_shared<constant_medium>(ball, 0.2, color(0.8, 0.7, 0.8)));

    auto atmo = make_shared<sphere>(
        point3(0, 2, 0),
        1000,
        make_shared<lambertian>(color(0.5, 0.0, 0.0))
    );
    //world.add(make_shared<constant_medium>(atmo, 0.002, color(0.1, 0.1, 0.1)));


    //world.add(make_shared<sphere>(point3(50, 20, 0), 20, difflight));
    world.add(
        make_shared<rotate_y>(mesh::fromFile("snowball.buvf", difflight), 30)
    );

    // skylight
    auto sky = make_shared<diffuse_light>(color(0.5, 0.5, 0.5));
    world.add(make_shared<quad>(vec3(100, 40, 100), vec3(-200, 0, 0), vec3(0, 0, -200), sky));
    
    camera cam;

    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 500;
    cam.samples_per_pixel = 40;
    cam.max_depth = 50;
    cam.background = color(0.015, 0.015, 0.02);

    cam.vfov = 40;
    cam.lookfrom = point3(40, 12, 6);
    cam.lookat = point3(0, 1, 0);
    cam.vup = vec3(0, 1, 0);

    cam.defocus_angle = 0;
    cam.chunk_size = 32;
    cam.num_threads = 20;

    world.add(make_shared<hittable_list>(lights));
    auto bvh_world = bvh_node(world);
    cam.render(bvh_world, lights);
}

void cornell_box() {
    hittable_list world;
    hittable_list lights;

    auto red = make_shared<lambertian>(color(.65, .05, .05));
    auto white = make_shared<lambertian>(color(.73, .73, .73));
    auto green = make_shared<lambertian>(color(.12, .45, .15));
    auto light_mat = make_shared<diffuse_light>(color(15, 15, 15));

    // Cornell box sides
    world.add(make_shared<quad>(point3(55, 0, 0), vec3(0, 0, 55), vec3(0, 55, 0), green));
    world.add(make_shared<quad>(point3(0, 0, 55), vec3(0, 0, -55), vec3(0, 55, 0), red));
    world.add(make_shared<quad>(point3(0, 55, 0), vec3(555, 0, 0), vec3(0, 0, 55), white));
    world.add(make_shared<quad>(point3(0, 0, 55), vec3(555, 0, 0), vec3(0, 0, -55), white));
    world.add(make_shared<quad>(point3(55, 0, 55), vec3(-55, 0, 0), vec3(0, 55, 0), white));

    plyArgs args{.bHit2 = true};

    auto rawPly = loadPly("local/tomatoes_200x_180.ply", args);

    spdlog::info("Pre-BVH");
    auto bvhPly = make_shared<bvh_node>(rawPly);
    spdlog::info("Post-BVH");


    world.add(make_shared<translate>(bvhPly, vec3(25, 0, 25)));

    //auto sphere_light = make_shared<sphere>(point3(40, 20, 25), 5, light_mat);
    //lights.add(sphere_light);

    //world.add(sphere_light);

    // Light
    auto light = make_shared<quad>(point3(21, 54.9, 23), vec3(13, 0, 0), vec3(0, 0, 11), light_mat);
    auto light_box = make_shared<quad>(point3(21, 54.9, 23), vec3(13, 0, 0), vec3(0, 0, 11), make_shared<material>());
    lights.add(light_box);
    world.add(light);

    camera cam;

    cam.aspect_ratio = 1.0;
    cam.image_width = 100;
    cam.samples_per_pixel = 20;
    cam.max_depth = 5;
    cam.background = color(0, 0, 0);

    cam.vfov = 40;
    cam.lookfrom = point3(28, 28, -70);
    cam.lookat = point3(28, 28, 0);
    cam.vup = vec3(0, 1, 0);

    cam.defocus_angle = 0;
    cam.output_file = "output/output.ppm";

    cam.chunk_size = 5;
    cam.num_threads = 18;

    auto bvh_world = bvh_node(world);
    auto bvh_lights = bvh_node(lights);

    cam.render(bvh_world, bvh_lights);
}

void cornell_box_video()
{
    auto red = make_shared<lambertian>(color(.65, .05, .05));
    auto white = make_shared<lambertian>(color(.73, .73, .73));
    auto green = make_shared<lambertian>(color(.12, .45, .15));
    auto light_mat = make_shared<diffuse_light>(color(15, 15, 15));

    
    plyArgs args{ .bHit2 = true };

    auto rawPly = loadPly("local/tomatoes_200x_180.ply", args);

    spdlog::info("Pre-BVH");
    auto bvhPly = make_shared<bvh_node>(rawPly);
    spdlog::info("Post-BVH");


    

    // Light
    auto light = make_shared<quad>(point3(21, 54.9, 23), vec3(13, 0, 0), vec3(0, 0, 11), light_mat);
    auto light_box = make_shared<quad>(point3(21, 54.9, 23), vec3(13, 0, 0), vec3(0, 0, 11), make_shared<material>());
    //lights.add(light_box);
    //world.add(light);

    camera cam;

    cam.aspect_ratio = 1.0;
    cam.image_width = 100;
    cam.samples_per_pixel = 100;
    cam.max_depth = 5;
    cam.background = color(0, 0, 0);

    cam.vfov = 40;
    cam.lookfrom = point3(28, 28, -70);
    cam.lookat = point3(28, 28, 0);
    cam.vup = vec3(0, 1, 0);

    cam.defocus_angle = 0;

    cam.chunk_size = 5;
    cam.num_threads = 18;

    double radius = 20;

    for (double angle = 0.0; angle < 2 * pi; angle += 0.4)
    {

        hittable_list world;
        hittable_list lights;

        // Cornell box sides
        world.add(make_shared<quad>(point3(55, 0, 0), vec3(0, 0, 55), vec3(0, 55, 0), green));
        world.add(make_shared<quad>(point3(0, 0, 55), vec3(0, 0, -55), vec3(0, 55, 0), red));
        world.add(make_shared<quad>(point3(0, 55, 0), vec3(555, 0, 0), vec3(0, 0, 55), white));
        world.add(make_shared<quad>(point3(0, 0, 55), vec3(555, 0, 0), vec3(0, 0, -55), white));
        world.add(make_shared<quad>(point3(55, 0, 55), vec3(-55, 0, 0), vec3(0, 55, 0), white));

        world.add(make_shared<translate>(bvhPly, vec3(25, 0, 25)));

        auto sphere_light = make_shared<sphere>(point3(radius * std::sin(angle), 30, radius * std::cos(angle)), 2, light_mat);
        lights.add(sphere_light);
        world.add(sphere_light);

        auto bvh_world = bvh_node(world);
        auto bvh_lights = bvh_node(lights);
        cam.output_file = "output/rotating_light_" + std::to_string(angle) +".ppm";
        cam.render(bvh_world, bvh_lights);

    }
}

void benchmark_scene()
{
    //shared_ptr<bvh_node> splatBenchmark;

    int samples = 1;
    int depth = 1;
    int threads = 10;
    int chunk = 10;

    std::string benchmark_file = "output/benchmark.txt";

    // scene constants
    hittable_list world;
    hittable_list lights;

    auto red = make_shared<lambertian>(color(.65, .05, .05));
    auto white = make_shared<lambertian>(color(.73, .73, .73));
    auto green = make_shared<lambertian>(color(.12, .45, .15));
    auto light_mat = make_shared<diffuse_light>(color(15, 15, 15));

    // Cornell box sides
    world.add(make_shared<quad>(point3(55, 0, 0), vec3(0, 0, 55), vec3(0, 55, 0), green));
    world.add(make_shared<quad>(point3(0, 0, 55), vec3(0, 0, -55), vec3(0, 55, 0), red));
    world.add(make_shared<quad>(point3(0, 55, 0), vec3(555, 0, 0), vec3(0, 0, 55), white));
    world.add(make_shared<quad>(point3(0, 0, 55), vec3(555, 0, 0), vec3(0, 0, -55), white));
    world.add(make_shared<quad>(point3(55, 0, 55), vec3(-55, 0, 0), vec3(0, 55, 0), white));

    plyArgs args{ .bHit2 = true };

    auto rawPly = loadPly("local/tomatoes_100x_180.ply", args);

    spdlog::info("Pre-BVH");
    auto bvhPly = make_shared<bvh_node>(rawPly);
    spdlog::info("Post-BVH");


    world.add(make_shared<translate>(bvhPly, vec3(25, 0, 25)));

    auto sphere_light = make_shared<sphere>(point3(40, 20, 25), 5, light_mat);

    // Light
    auto light = make_shared<quad>(point3(21, 55, 23), vec3(13, 0, 0), vec3(0, 0, 11), light_mat);
    auto light_box = make_shared<quad>(point3(21, 55, 23), vec3(13, 0, 0), vec3(0, 0, 11), make_shared<material>());
    lights.add(light_box);
    world.add(light);
    auto bvh_world = bvh_node(world);
    auto bvh_lights = bvh_node(lights);

    camera cam;

    cam.background = color(0, 0, 0);

    cam.vfov = 40;
    cam.lookfrom = point3(28, 28, -80);
    cam.lookat = point3(28, 28, 0);
    cam.vup = vec3(0, 1, 0);

    cam.defocus_angle = 0;
    cam.output_file = "output/output.ppm";
    cam.aspect_ratio = 1.0;
    cam.image_width = 100;

    std::ofstream benchmarkFile;
    benchmarkFile.open(benchmark_file);
    benchmarkFile << "samples" << "," << "depth" << "," << "threads" << "," << "chunk" << "," << "elapsed_seconds" << "\n";
    benchmarkFile.close();

    int& benchmark = samples;
    for (benchmark = 2; benchmark < 257; benchmark *= 2)
    {
        auto start_time = std::chrono::system_clock::now();

        cam.samples_per_pixel = samples;
        cam.max_depth = depth;
        cam.chunk_size = chunk;
        cam.num_threads = threads;
        cam.render(bvh_world, bvh_lights);

        auto end_time = std::chrono::system_clock::now();

        std::chrono::duration<double> elapsed_seconds = end_time - start_time;
        std::ofstream benchmarkFile;
        benchmarkFile.open(benchmark_file);
        benchmarkFile << samples << "," << depth << "," << threads << "," << chunk << "," << elapsed_seconds.count() << "\n";
        benchmarkFile.close();
    }
    

}

int main()
{
    switch (13) 
    {
        case 1: bouncing_spheres();  break;
        case 2: checkered_spheres(); break;
		case 3: earth(); break;
		case 4: perlin_spheres(); break;
		case 5: quads(); break;
		case 6: meshes(); break;
        case 7: simple_light(); break;
		case 8: the_council(); break;
        case 9: gs_test(point3(10, 5, 0), "output.ppm"); break;
        case 10: gs_video(); break;
        case 11: cornell_box(); break;
        case 12: benchmark_scene(); break;
        case 13: cornell_box_video(); break;
        default: break;
    }
}
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
    cam.samples_per_pixel = 100;
    cam.max_depth = 10;

    cam.vfov = 20;
    cam.lookfrom = point3(13, 2, 3);
    cam.lookat = point3(0, 0, 0);
    cam.vup = vec3(0, 1, 0);

    cam.defocus_angle = 0.6;
    cam.focus_dist = 10.0;
    cam.background = color(0.70, 0.80, 1.00);

    auto empty_material = shared_ptr<material>();
    quad lights(point3(0, 2, 0), vec3(-130, 0, 0), vec3(0, 0, -105), empty_material);

    cam.render(world, lights);

	return 0;
}

void checkered_spheres() {
    hittable_list world;

    auto checker = make_shared<checker_texture>(0.32, color(.2, .3, .1), color(.9, .9, .9));

    world.add(make_shared<sphere>(point3(0, -10, 0), 10, make_shared<lambertian>(checker)));
    world.add(make_shared<sphere>(point3(0, 10, 0), 10, make_shared<lambertian>(checker)));

    camera cam;

    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 400;
    cam.samples_per_pixel = 100;
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
    auto blender_texture = make_shared<image_texture>("default_texture.jpg");
    auto blender_surface = make_shared<lambertian>(blender_texture);


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
    cam.samples_per_pixel = 100;
    cam.max_depth = 50;

    cam.vfov = 80;
    cam.lookfrom = point3(0, 0, 9);
    cam.lookat = point3(0, 0, 0);
    cam.vup = vec3(0, 1, 0);

    cam.defocus_angle = 0;
    cam.background = color(0.70, 0.80, 1.00);

    auto empty_material = shared_ptr<material>();
    quad lights(point3(343, 554, 332), vec3(-130, 0, 0), vec3(0, 0, -105), empty_material);

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
    cam.lookfrom = point3(13, 2, 3);
    cam.lookat = point3(0, 0, 0);
    cam.vup = vec3(0, 1, 0);

    cam.defocus_angle = 0.6;
    cam.focus_dist = 10.0;
    cam.background = color(0.70, 0.80, 1.00);

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
    cam.samples_per_pixel = 1000;
    cam.max_depth = 50;
    cam.background = color(0, 0, 0);

    cam.vfov = 20;
    cam.lookfrom = point3(26, 8, 0);
    cam.lookat = point3(0, 1, 0);
    cam.vup = vec3(0, 1, 0);

    cam.defocus_angle = 0;
	cam.chunk_size = 16;
    cam.num_threads = 20;

    // make the lights actual things
    world.add(make_shared<hittable_list>(lights));

	auto bvh_world = bvh_node(world);
    cam.render(bvh_world, lights);
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


int main()
{
    switch (7) 
    {
        case 1: bouncing_spheres();  break;
        case 2: checkered_spheres(); break;
		case 3: earth(); break;
		case 4: perlin_spheres(); break;
		case 5: quads(); break;
		case 6: meshes(); break;
        case 7: simple_light(); break;
		case 8: the_council(); break;
    }
}
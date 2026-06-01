#pragma once
#include "utility.h"
#include <stb_image.h>

class rtw_image
{
	std::shared_ptr<spdlog::logger> logger;
	int image_width = -1;
	int image_height = -1;
	int channels = -1;
	unsigned char* data = nullptr;

	inline bool loadImageFile(std::string fileName, int& width, int& height, int& channels, unsigned char*& data)
	{
		data = stbi_load(fileName.c_str(), &width, &height, &channels, 0);
		if (!data)
		{
			return false;
		}
		return true;
	}
	inline bool freeImage(unsigned char*& data)
	{
		stbi_image_free(data);
		return true;
	}

public:
	rtw_image() : rtw_image("default_texture.jpg") {}
	rtw_image(const char* filename)
	{
		auto logger = spdlog::get("image");
		if (!logger)
		{
			logger = spdlog::stdout_color_mt("image");
		}
		if (!loadImageFile(filename, image_width, image_height, channels, data))
		{
			logger->error("Failed to load image file: {}", filename);
		}
	}

	~rtw_image()
	{
		if (data)
		{
			freeImage(data);
		}
	}

	// wrapper to match tutorial's expected functions
	bool load(const std::string& filename)
	{
		return loadImageFile(filename, image_width, image_height, channels, data);
	}
	int width()  const { return (data == nullptr) ? 0 : image_width; }
	int height() const { return (data == nullptr) ? 0 : image_height; }

	const unsigned char* pixel_data(int x, int y) const
	{
		static unsigned char magenta[] = { 255, 0, 255 };
		if (data == nullptr) return magenta;

		int loc = (y * image_width * channels) + (x * channels);
		if (loc < 0 || loc >= (image_width * image_height * channels))
		{
			logger->warn("Trying to access pixel outside of image! ({}, {})", x, y);
			return magenta;
		}
		// TODO: would be nice to split this better
		return &data[loc];
	}
};


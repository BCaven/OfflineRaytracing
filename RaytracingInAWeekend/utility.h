#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include <fstream>
#include <cstdlib>

// C++ Std Usings

using std::make_shared;
using std::shared_ptr;

// Constants

const double infinity = std::numeric_limits<double>::infinity();
const double pi = 3.1415926535897932385;
const double minimus = 0.00000001;

// Utility Functions

inline double degrees_to_radians(double degrees) {
    return degrees * pi / 180.0;
}

inline double random_double() {
    // Returns a random real in [0,1).
    return std::rand() / (RAND_MAX + 1.0);
}

inline double random_double(double min, double max) {
    // Returns a random real in [min,max).
    return min + (max - min) * random_double();
}

inline int random_int(int min, int max) {
    // Returns a random integer in [min,max].
    return int(random_double(min, max + 1));
}

inline int clamp(int x, int low, int high) {
    // Return the value clamped to the range [low, high).
    if (x < low) return low;
    if (x < high) return x;
    return high - 1;
}

inline unsigned char float_to_byte(float value) {
    if (value <= 0.0)
        return 0;
    if (1.0 <= value)
        return 255;
    return static_cast<unsigned char>(256.0 * value);
}

namespace utility
{
	inline bool loadFile(std::string fileName, std::string& outContent)
	{
		std::ifstream fileStream(fileName, std::ios::in);
		if (!fileStream.is_open())
		{
			spdlog::error("Failed to open file: {}", fileName);
			return false;
		}
		std::string line = "";
		outContent = "";
		while (std::getline(fileStream, line))
		{
			outContent += line + "\n";
		}
		fileStream.close();
		return true;
	}

	inline std::vector<std::string> split(const std::string& str, char delimiter) {
		std::vector<std::string> tokens(0);
		std::stringstream ss(str);
		std::string token;
		while (std::getline(ss, token, delimiter)) {
			tokens.push_back(token);
		}
		return tokens;
	}
}


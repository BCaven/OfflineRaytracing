#include "mesh.h"

std::shared_ptr<mesh> mesh::fromFile(std::string filename, shared_ptr<material> mat)
{
	auto logger = spdlog::get("shape-loader");
	if (!logger)
	{
		logger = spdlog::stdout_color_mt("shape-loader");
	}

	std::vector<float> floatData;
	int vertCount = -1;
	std::string rawFile;
	if (!utility::loadFile(filename, rawFile))
	{
		logger->error("Failed to open {}", filename);
		return nullptr;
	}

	auto splitLines = utility::split(rawFile, ' ');

	for (auto line : splitLines)
	{
		if (vertCount == -1)
		{
			vertCount = std::stoi(line);
			floatData.reserve(vertCount * 8);
		}
		else
		{
			floatData.push_back(std::stof(line));
		}
	}

	if (floatData.size() == vertCount * 2 * 3)
	{
		if (vertCount % 3 != 0)
		{
			logger->warn("Our vertex count is not a multiple of 3 which suggests we do not have triangles");
		}
		for (int i = 0; i < vertCount; i += 3)
		{
			// default coords
			floatData.insert(floatData.end(), { 0, 0, 0, 1, 1, 0 });
		}
	}

	if (floatData.size() != vertCount * 2 * 3 && floatData.size() != vertCount * 8)
	{
		logger->error("Failed to parse {}, expected {} or {} floats but got {}", filename, vertCount * 2 * 3, vertCount * 8, floatData.size());
		return nullptr;
	}

	return make_shared<mesh>(vertCount, floatData, mat);
}

mesh::mesh(int vertCount, std::vector<float> vertData, shared_ptr<material> mat)
{
	logger = spdlog::get("shape");
	if (!logger)
	{
		logger = spdlog::stdout_color_mt("shape");
	}

	auto tmp_hit = hittable_list();


	// vert data is stored as points, normals, uvs
	// points and normals are three floats, uvs are two
	std::vector<vertex> vertices;


	for (int i = 0; i < vertCount * 3; i += 3)
	{
		vertices.push_back(vertex(
			point3(vertData[i], vertData[i + 1], vertData[i + 2]),
			vec3(),
			vec3()
		));	
	}
	if (vertices.size() != vertCount)
	{
		logger->error("Failed to parse vertex data, expected {} vertices but got {}", vertCount, vertices.size());
	}
	int offset = vertCount * 3;
	for (int i = 0; i < vertCount * 3; i += 3)
	{
		vertices[i / 3].normal = vec3(vertData[offset + i], vertData[offset + i + 1], vertData[offset + i + 2]);
	}
	offset += vertCount * 3;
	for (int i = 0; i < vertCount * 2; i += 2)
	{
		vertices[i / 2].uv = vec3(vertData[offset + i], vertData[offset + i + 1], 0);
	}
	
	

	for (int i = 0; i < vertCount; i += 3)
	{
		auto tri = make_shared<triangle>(vertices[i], vertices[i + 1], vertices[i + 2], mat);
		bbox = aabb(bbox, tri->bounding_box());
		tmp_hit.add(tri);
		triangles.push_back(tri);
	}

	if (triangles.size() != vertCount / 3)
	{
		logger->error("Failed to parse triangle data, expected {} triangles but got {}", vertCount / 3, triangles.size());
	}


	// build the bvh
	
	bvh = hittable_list(make_shared<bvh_node>(tmp_hit));
}
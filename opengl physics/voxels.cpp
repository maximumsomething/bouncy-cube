#include "voxels.hpp"
#include <string>
#include <iostream>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "arrayND.hpp"
#include "loaders.hpp"



class VoxelStorage {
public:
	arrayND<bool, 3> storage;
	
	// Every vertex has index of up to 6 connected vertices
	struct Vertex {
		float x, y, z;
		Vertex(float x, float y, float z) : x(x), y(y), z(z) {
			for (auto& i : neighbors) {
				i = -1;
			}
		};
		
		// Order: -x, -y, -z, +x, +y, +z
		int32_t neighbors[6];
	};
	std::vector<Vertex> verts;
	std::vector<uint32_t> edgeIndices;
	
	VoxelStorage(arrayND<bool, 3> storage) : storage(storage) {
		
		verts = getVerts();
		edgeIndices = getEBO();
		
	}
	
	// Every vert is guaranteed to be either a positive x, y, or z in front of the previous vertex
private:
	std::vector<Vertex> getVerts() {
		std::vector<Vertex> verts;
		
		// Version that returns corners between voxels
		/*auto indexMapSize = storage.sizes;
		indexMapSize[0] +=1; indexMapSize[1] += 1; indexMapSize[2] += 1;
		// Array of all potential vertices, and where they are in the verts array
		arrayND<int32_t, 3> indexMap(indexMapSize, -1);
		
		// iterate through the corners
		for (unsigned int z = 0; z <= storage.sizes[2]; ++z)
		for (unsigned int y = 0; y <= storage.sizes[1]; ++y)
		for (unsigned int x = 0; x <= storage.sizes[0]; ++x) {
			std::array<size_t, 3> coord{x, y, z};
			
			// The corner at (x, y, z) is at the middle of the cube of 8 voxels, with the corners (x-1, y-1, z-1) and (x, y, z)
			
			// If any neighboring voxels exist, this vertex exists.
			for (int i = 0; i < 8; ++i) {
				if (storage[x - i % 2][y - (i / 2) % 2][z - (i / 4) % 2]) {
					
					verts.push_back(Vertex(x, y, z));
					indexMap[x][y][z] = verts.size() - 1;
					
					// Get the neighbors above, but not below
					for (int j = 0; j < 3; ++j) {
						if (coord[j] > 0) {

							auto neighborCoord = coord;
							neighborCoord[j] -= 1;
							if (indexMap[neighborCoord] != -1) {
								verts.back().neighbors[j] = indexMap[neighborCoord];
								verts[indexMap[neighborCoord]].neighbors[j + 3] = verts.size() - 1;
							}
						}
					}
					break;
				}
			}
		}*/
		
		// Version that returns the voxels themselves
		
		// Contains the indices of the vertices in toReturn
		arrayND<int32_t, 3> indexMap(storage.sizes, -1);
		
		for (int i = 0; i < storage.total(); ++i) {
			if (storage.linear()[i]) {
				auto coord = storage.ind2coord(i);
				verts.push_back(Vertex(coord[0], coord[1], coord[2]));
				
				indexMap.linear()[i] = verts.size() - 1;
				
				// Get the neighbors above, but not below
				for (int j = 0; j < 3; ++j) {
					if (coord[j] > 0) {

						auto neighborCoord = coord;
						neighborCoord[j] -= 1;
						if (storage[neighborCoord]) {
							assert(indexMap[neighborCoord] >= 0);
							verts.back().neighbors[j] = indexMap[neighborCoord];
							verts[indexMap[neighborCoord]].neighbors[j + 3] = verts.size() - 1;
						}
					}
				}
			}
		}
		
		return verts;
	}
	
	std::vector<uint32_t> getEBO() {
		std::vector<uint32_t> EBO;
		for (uint32_t i = 0; i < verts.size(); ++i) {
			for (auto j : verts[i].neighbors) {
				if (j == -1) {
					EBO.push_back(i);
					break;
				}
			}
		}
		return EBO;
	}
	
	// Version for if the verts were voxel corners
	/*
	std::vector<uint32_t> getEBOData(const std::vector<Vertex>& verts) {
		std::vector<uint32_t> EBO;
		
		// iterate through the voxels, with an extra fake voxel on the end
		for (unsigned int z = 0; z <= storage.sizes[2]; ++z)
		for (unsigned int y = 0; y <= storage.sizes[1]; ++y)
		for (unsigned int x = 0; x <= storage.sizes[0]; ++x) {
			std::array<size_t, 3> coord{x, y, z};
			
			// Get the neighbors above, but not below
			for (int i = 0; i < 3; ++i) {
				auto neighborCoord = coord;
				neighborCoord[i] -= 1;
				
				bool neighbor = storage.inBounds(neighborCoord) && storage[neighborCoord];
				bool me = storage.inBounds({x, y, z}) && storage[x][y][z];
				
				if (me ^ neighbor) {
					// Neighbor is different, make a face
					
					
				}
				
			}
			
		}
	}*/
};

arrayND<bool, 3> genSphere(unsigned int radius) {
	arrayND<bool, 3> sphere({radius*2, radius*2, radius*2});
	
	
	for (int i = 0; i < sphere.total(); ++i) {
		auto coord = sphere.ind2coord(i);
		sphere.linear()[i] =
		pow((float) coord[0] - radius, 2) + pow((float) coord[1] - radius, 2) + pow((float) coord[2] - radius, 2)
		<= pow(radius, 2);
		
	}
	
	return sphere;
}


constexpr int RADIUS = 10;

// Renders everything
class VoxelRendererImpl : public VoxelRenderer {
public:
	
	GLuint shaderProgram;
	
	VoxelStorage toRender{genSphere(RADIUS)};
	
	GLuint VAO, VBO, EBO;
	
	VoxelRendererImpl() :
	shaderProgram(linkShaders({
		loadShader("voxels.vert", GL_VERTEX_SHADER),
		loadShader("voxels.frag", GL_FRAGMENT_SHADER),
		loadShader("voxels.geom", GL_GEOMETRY_SHADER)
	})) {
		
		glGenVertexArrays(1, &VAO);
		glBindVertexArray(VAO);
		
		glGenBuffers(1, &VBO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
	
		glBufferData(GL_ARRAY_BUFFER, toRender.verts.size() * sizeof(VoxelStorage::Vertex), toRender.verts.data(), GL_STATIC_DRAW);
		
		glGenBuffers(1, &EBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, toRender.edgeIndices.size() * sizeof(uint32_t), toRender.edgeIndices.data(), GL_STATIC_DRAW);
		
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VoxelStorage::Vertex), 0);
		glEnableVertexAttribArray(0);
		
		glVertexAttribIPointer(1, 3, GL_INT, sizeof(VoxelStorage::Vertex), (void *) offsetof(VoxelStorage::Vertex, neighbors));
		glEnableVertexAttribArray(1);
		glVertexAttribIPointer(2, 3, GL_INT, sizeof(VoxelStorage::Vertex), (void *) offsetof(VoxelStorage::Vertex, neighbors[3]));
		glEnableVertexAttribArray(2);
		
		
	}
	
	void render(glm::mat4 view, glm::mat4 projection) override {
		
		glm::mat4 model(1.0f);
		model = glm::translate(model, glm::vec3(-RADIUS, -RADIUS, -RADIUS));
		
		auto totalTransform = projection * view * model;
		

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		
		glUseProgram(shaderProgram);
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "transform"), 1, GL_FALSE, glm::value_ptr(totalTransform));
		
		glBindVertexArray(VAO);
		//glPointSize(10);
		//glDrawArrays(GL_POINTS, 0, toRender.verts.size());
		
		glDrawElements(GL_POINTS, toRender.edgeIndices.size(), GL_UNSIGNED_INT, nullptr);
	}
};

std::unique_ptr<VoxelRenderer> getVoxelRenderer() {
	return std::make_unique<VoxelRendererImpl>();
}


int testy() {
	
	arrayND<std::string, 3> test1({50, 30, 20});
	
	test1[49][29][19] = "hello world";
	//std::cout << test1[49][29][19] << std::endl;
	
	auto arr = test1.ind2coord(test1.coord2ind({49,29,19}));

	return 0;
}

int tester = testy();


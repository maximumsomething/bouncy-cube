#ifndef voxels_hpp
#define voxels_hpp
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <memory>


class VoxelRenderer {
public:
	virtual void render(glm::mat4 view, glm::mat4 projection, bool paused) = 0;
	virtual ~VoxelRenderer() = default;
};


std::unique_ptr<VoxelRenderer> getVoxelRenderer();


#endif /* voxels_hpp */

#include "voxels.hpp"
#include <string>
#include <iostream>
#include <cmath>
#include <unistd.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "arrayND.hpp"
#include "loaders.hpp"



class VoxelStorage {
public:
	arrayND<bool, 3> storage;
	
	// Every vertex has index of up to 6 connected vertices
	struct VertData {
		VertData() {
			for (auto& i : neighbors) {
				i = -1;
			}
		};
		
		// Order: -x, -y, -z, +x, +y, +z
		int32_t neighbors[6];
	};
	std::vector<glm::vec3> vertsPos;
	std::vector<VertData> vertsData;
	std::vector<uint32_t> edgeIndices;
	
	VoxelStorage(arrayND<bool, 3> storage) : storage(storage) {
		
		setVerts();
		edgeIndices = getEBO();
		
	}
	
	// Every vert is guaranteed to be either a positive x, y, or z in front of the previous vertex
private:
	void setVerts() {
		vertsPos.clear();
		vertsData.clear();
		
		// Contains the indices of the vertices in toReturn
		arrayND<int32_t, 3> indexMap(storage.sizes, -1);
		
		for (int i = 0; i < storage.total(); ++i) {
			if (storage.linear()[i]) {
				auto coord = storage.ind2coord(i);
				vertsPos.push_back(glm::vec3(coord[0], coord[1], coord[2]));
				vertsData.emplace_back();
				
				indexMap.linear()[i] = vertsData.size() - 1;
				
				// Get the neighbors above, but not below
				for (int j = 0; j < 3; ++j) {
					if (coord[j] > 0) {

						auto neighborCoord = coord;
						neighborCoord[j] -= 1;
						if (storage[neighborCoord]) {
							assert(indexMap[neighborCoord] >= 0);
							vertsData.back().neighbors[j] = indexMap[neighborCoord];
							vertsData[indexMap[neighborCoord]].neighbors[j + 3] = vertsData.size() - 1;
						}
					}
				}
			}
		}
	}
	
	std::vector<uint32_t> getEBO() {
		std::vector<uint32_t> EBO;
		for (uint32_t i = 0; i < vertsData.size(); ++i) {
			for (auto j : vertsData[i].neighbors) {
				if (j == -1) {
					EBO.push_back(i);
					break;
				}
			}
		}
		return EBO;
	}
};

arrayND<bool, 3> genSphere(float radius) {
	unsigned int arrSiz = (unsigned int) ceil(radius*2);
	arrayND<bool, 3> sphere({arrSiz, arrSiz, arrSiz});
	
	
	for (int i = 0; i < sphere.total(); ++i) {
		auto coord = sphere.ind2coord(i);
		sphere.linear()[i] =
		pow((float) coord[0] - radius + 0.5, 2) + pow((float) coord[1] - radius + 0.5, 2) + pow((float) coord[2] - radius + 0.5, 2)
		<= pow(radius + 0.1, 2);
		
	}
	
	return sphere;
}


constexpr float RADIUS = 10;
constexpr int PHYS_STEPS_PER_FRAME = 2;
constexpr int SLOWDOWN_FACTOR = 1;


const GLchar * physOutputs[] = { "outPos", "outTurn", "outVel", "outAngVel", "gl_NextBuffer", "debugFeedback" };

// Renders everything
class VoxelRendererImpl : public VoxelRenderer {
public:
	
	GLuint renderShader, physicsShader;
	GLuint cubeTexture = loadTexture("rubber.jpg");
	//GLuint cubeTexture = loadTexture("astroturf-2.jpeg");
	
	VoxelStorage toRender{genSphere(RADIUS)};
	
	// PhysVBO is read and written by the physics code. DebugFeedbackVBO is written to but not read by the physics code, and DataVBO is read but not written to by the physics code. All three are read by the drawing code.
	GLuint renderVAO, physVAO, physVBO1, physVBO2, debugFeedbackVBO, dataVBO, EBO, physBufTex;
	
	struct PhysData {
		glm::vec3 pos = glm::zero<glm::vec3>();
		glm::vec3 turn = glm::zero<glm::vec3>();
		glm::vec3 vel = glm::zero<glm::vec3>();
		glm::vec3 angVel = glm::zero<glm::vec3>();
	};
	
	size_t physVBOSize, feedbackVBOSize;
	
	
	VoxelRendererImpl() :
	renderShader(linkShaders({
		loadShader("voxels.vert", GL_VERTEX_SHADER),
		loadShader("voxels.frag", GL_FRAGMENT_SHADER),
		loadShader("voxels.geom", GL_GEOMETRY_SHADER)
	})),
	physicsShader(linkShaders({
		loadShader("sim.vert", GL_VERTEX_SHADER)}, [] (GLuint toBeLinked) {
		glTransformFeedbackVaryings(toBeLinked, sizeof(physOutputs) / sizeof(physOutputs[0]), physOutputs, GL_INTERLEAVED_ATTRIBS);
	})) {
		
		glGenVertexArrays(1, &renderVAO);
		glBindVertexArray(renderVAO);
		
		// Gen all VBOs and the EBO
		glGenBuffers(5, &physVBO1);
		
		// Set the initial positions
		std::vector<PhysData> initPhysData(toRender.vertsPos.size());
		for (unsigned i = 0; i < toRender.vertsPos.size(); ++i) {
			initPhysData[i].pos = toRender.vertsPos[i];
			//if (i < 10) initPhysData[i].vel = glm::vec3(1, 1, 1);
			if (i > toRender.vertsPos.size() - 51) initPhysData[i].vel = glm::vec3(100, 0, 0);
			if (i < 50) initPhysData[i].vel = glm::vec3(-100, 0, 0);
			//initPhysData[i].angVel = glm::vec3(0, 30, 0);
		}
		
		physVBOSize = toRender.vertsPos.size() * sizeof(PhysData);
		feedbackVBOSize = toRender.vertsPos.size() * sizeof(float);
		
		glBindBuffer(GL_ARRAY_BUFFER, physVBO1);
		glBufferData(GL_ARRAY_BUFFER, physVBOSize, initPhysData.data(), GL_STREAM_COPY);
		// draw pos
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(PhysData), 0);
		glEnableVertexAttribArray(0);
		// draw turn
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(PhysData), (void*) offsetof(PhysData, turn));
		glEnableVertexAttribArray(1);
		
		glBindBuffer(GL_ARRAY_BUFFER, debugFeedbackVBO);
		float* clearData = (float *) calloc(1, feedbackVBOSize);
		glBufferData(GL_ARRAY_BUFFER, feedbackVBOSize, clearData, GL_STREAM_COPY);
		free(clearData);
		// draw debug feedback
		glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(float), 0);
		glEnableVertexAttribArray(2);
		
		glBindBuffer(GL_ARRAY_BUFFER, dataVBO);
		glBufferData(GL_ARRAY_BUFFER, toRender.vertsData.size() * sizeof(VoxelStorage::VertData), toRender.vertsData.data(), GL_STATIC_DRAW);
		setVertDataAttrs(renderShader);
		
		
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, toRender.edgeIndices.size() * sizeof(uint32_t), toRender.edgeIndices.data(), GL_STATIC_DRAW);
		
		// Physics renderer
		glGenVertexArrays(1, &physVAO);
		glBindVertexArray(physVAO);
		glUseProgram(physicsShader);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glEnableVertexAttribArray(3);
		
		
		setVertDataAttrs(physicsShader);
		glUniform1i(glGetUniformLocation(physicsShader, "allVerts"), 0);
		
		glUniform1f(glGetUniformLocation(physicsShader, "timeDelta"), 1.0/60.0/PHYS_STEPS_PER_FRAME);
		
		glGenTextures(1, &physBufTex);
	}
	
	void setVertDataAttrs(GLuint shader) {
		glBindBuffer(GL_ARRAY_BUFFER, dataVBO);
		glUseProgram(shader);
		
		GLint neighborsM = glGetAttribLocation(shader, "neighborsM");
		glVertexAttribIPointer(neighborsM, 3, GL_INT, sizeof(VoxelStorage::VertData), 0);
		glEnableVertexAttribArray(neighborsM);
		GLint neighborsP = glGetAttribLocation(shader, "neighborsP");
		glVertexAttribIPointer(neighborsP, 3, GL_INT, sizeof(VoxelStorage::VertData), (void *) offsetof(VoxelStorage::VertData, neighbors[3]));
		glEnableVertexAttribArray(neighborsP);
		glCheckError();
	}
	
	void physicsStep(GLuint inVBO, GLuint outVBO) {
		
		// Clear the out buffer
		glBindBuffer(GL_ARRAY_BUFFER, outVBO);
		glBufferData(GL_ARRAY_BUFFER, physVBOSize, nullptr, GL_STREAM_COPY);

		
		glBindBuffer(GL_ARRAY_BUFFER, inVBO);
		// inPos
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(PhysData), 0);
		// inTurn
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(PhysData), (void*) offsetof(PhysData, turn));
		// inVel
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(PhysData), (void*) offsetof(PhysData, vel));
		// inAngVel
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(PhysData), (void*) offsetof(PhysData, angVel));
		
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, outVBO);
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1, debugFeedbackVBO);
		
		
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_BUFFER, physBufTex);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, inVBO);
		
		
		glBeginTransformFeedback(GL_POINTS);
		glDrawArrays(GL_POINTS, 0, toRender.vertsPos.size());
		glEndTransformFeedback();
		//glFlush();
	}
	
	
	void doPhysics() {
		/*static int count = 0;
		++count;
		if (count % 20 != 0) return;*/
		
		glUseProgram(physicsShader);
		glBindVertexArray(physVAO);
		glEnable(GL_RASTERIZER_DISCARD);
		
		assert(PHYS_STEPS_PER_FRAME % 2 == 0);
		assert(PHYS_STEPS_PER_FRAME / 2 % SLOWDOWN_FACTOR == 0);
		
		for (int i = 0; i < PHYS_STEPS_PER_FRAME / 2 / SLOWDOWN_FACTOR; ++i) {
			physicsStep(physVBO1, physVBO2);
			physicsStep(physVBO2, physVBO1);
		}
		
		glCheckError();
		
		glDisable(GL_RASTERIZER_DISCARD);
	}
	
	void render(glm::mat4 view, glm::mat4 projection, bool paused) override {
		
		if (!paused) doPhysics();
		
		glm::mat4 model(1.0f);
		model = glm::translate(model, glm::vec3(-RADIUS, -RADIUS, -RADIUS*2));
		
		auto totalTransform = projection * view * model;
		

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		
		glUseProgram(renderShader);
		glUniformMatrix4fv(glGetUniformLocation(renderShader, "transform"), 1, GL_FALSE, glm::value_ptr(totalTransform));
		
		glBindVertexArray(renderVAO);
		
		glEnable(GL_DEPTH_TEST);
		
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, cubeTexture);
		
		//glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		
		// Draw just the outside voxels
		//glDrawElements(GL_POINTS, toRender.edgeIndices.size(), GL_UNSIGNED_INT, nullptr);
		
		// Draw everything
		glDrawArrays(GL_POINTS, 0, toRender.vertsPos.size());
		
		//glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		
		//usleep(500000);
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


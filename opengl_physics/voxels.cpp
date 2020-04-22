#include "voxels.hpp"
#include <string>
#include <iostream>
#include <cmath>
#include <unistd.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "arrayND.hpp"
#include "input.hpp"
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
	
	
	for (unsigned i = 0; i < sphere.total(); ++i) {
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


const GLchar * physOutputs[] = {
	"outPos", "outVel", "outAngVel",
	"gl_NextBuffer", "outTurn",
	"gl_NextBuffer", "debugFeedback" };

// Renders everything
class VoxelRendererImpl : public VoxelRenderer {
public:
	
	GLuint voxelRenderShader, vectorRenderShader, physicsShader;
	GLuint cubeTexture = loadTexture("rubber.jpg");
	//GLuint cubeTexture = loadTexture("astroturf-2.jpeg");
	
	VoxelStorage toRender{genSphere(RADIUS)};
	
	// PhysVBO is read and written by the physics code. DebugFeedbackVBO is written to but not read by the physics code, and DataVBO is read but not written to by the physics code. All three are read by the drawing code.
	GLuint voxelRenderVAO, vectorRenderVAO, physVAO, debugFeedbackVBO, dataVBO, EBO, physBufTex3D, physBufTex4D;
	
	struct PhysData3D {
		glm::vec3 pos = glm::zero<glm::vec3>();
		//glm::vec3 turn = glm::zero<glm::vec3>();
		glm::vec3 vel = glm::zero<glm::vec3>();
		glm::vec3 angVel = glm::zero<glm::vec3>();
	};
	struct PhysData4D {
		glm::vec4 turn = glm::vec4(0, 0, 0, 1);
	};
	
	struct PhysBuffers {
		GLuint data3D;
		GLuint data4D;
		
		PhysBuffers() {
			glGenBuffers(2, &data3D);
		}
	};
	PhysBuffers physBuf1, physBuf2;
	
	size_t physVBO3DSize, physVBO4DSize, feedbackVBOSize;

	bool paused	= false, doingStep = false;
	
	VoxelRendererImpl() :
	voxelRenderShader(linkShaders({
		loadShader("voxels.vert", GL_VERTEX_SHADER),
		loadShader("voxels.frag", GL_FRAGMENT_SHADER),
		loadShader("voxels.geom", GL_GEOMETRY_SHADER)
	})),
	vectorRenderShader(linkShaders({
		loadShader("vectors.vert", GL_VERTEX_SHADER),
		loadShader("vectors.frag", GL_FRAGMENT_SHADER),
		loadShader("vectors.geom", GL_GEOMETRY_SHADER)
	})),
	physicsShader(linkShaders({
		loadShader("sim.vert", GL_VERTEX_SHADER)}, [] (GLuint toBeLinked) {
		glTransformFeedbackVaryings(toBeLinked, sizeof(physOutputs) / sizeof(physOutputs[0]), physOutputs, GL_INTERLEAVED_ATTRIBS);
	})) {
		
		glGenVertexArrays(3, &voxelRenderVAO);
		glBindVertexArray(voxelRenderVAO);
		
		// Set the initial positions
		std::vector<PhysData3D> initPhysData(toRender.vertsPos.size());
		std::vector<PhysData4D> initTurn(toRender.vertsPos.size());
		for (unsigned i = 0; i < toRender.vertsPos.size(); ++i) {
			initPhysData[i].pos = toRender.vertsPos[i];
			//if (i < 10) initPhysData[i].vel = glm::vec3(1, 1, 1);
			//if (i > toRender.vertsPos.size() - 51) initPhysData[i].vel = glm::vec3(100, 0, 0);
			//if (i < 50) initPhysData[i].vel = glm::vec3(-100, 0, 0);
			//initPhysData[i].angVel = glm::vec3(0, 30, 0);
		}
		
		physVBO3DSize = toRender.vertsPos.size() * sizeof(PhysData3D);
		physVBO4DSize = toRender.vertsPos.size() * sizeof(PhysData4D);
		feedbackVBOSize = toRender.vertsPos.size() * sizeof(float);
		
		glBindBuffer(GL_ARRAY_BUFFER, physBuf1.data3D);
		glBufferData(GL_ARRAY_BUFFER, physVBO3DSize, initPhysData.data(), GL_STREAM_COPY);
		glBindBuffer(GL_ARRAY_BUFFER, physBuf1.data4D);
		glBufferData(GL_ARRAY_BUFFER, physVBO4DSize, initTurn.data(), GL_STREAM_COPY);
		
		setDrawAttrs();
		
		// Generate the non-physics buffers
		glGenBuffers(3, &debugFeedbackVBO);

		glBindBuffer(GL_ARRAY_BUFFER, debugFeedbackVBO);
		float* clearData = (float *) calloc(1, feedbackVBOSize);
		glBufferData(GL_ARRAY_BUFFER, feedbackVBOSize, clearData, GL_STREAM_COPY);
		free(clearData);
		// draw debug feedback
		glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(float), 0);
		glEnableVertexAttribArray(2);
		
		glBindBuffer(GL_ARRAY_BUFFER, dataVBO);
		glBufferData(GL_ARRAY_BUFFER, toRender.vertsData.size() * sizeof(VoxelStorage::VertData), toRender.vertsData.data(), GL_STATIC_DRAW);
		setVertDataAttrs(voxelRenderShader);		
		
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, toRender.edgeIndices.size() * sizeof(uint32_t), toRender.edgeIndices.data(), GL_STATIC_DRAW);
		
		// Drawing debug vectors
		glBindVertexArray(vectorRenderVAO);
		setDrawAttrs();
		glBindBuffer(GL_ARRAY_BUFFER, physBuf1.data3D);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(PhysData3D), (void*) offsetof(PhysData3D, vel));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(PhysData3D), (void*) offsetof(PhysData3D, angVel));
		glEnableVertexAttribArray(3);

		// Physics renderer
		glBindVertexArray(physVAO);
		glUseProgram(physicsShader);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glEnableVertexAttribArray(3);
		
		
		setVertDataAttrs(physicsShader);
		glUniform1i(glGetUniformLocation(physicsShader, "allVerts"), 0);
		
		glUniform1f(glGetUniformLocation(physicsShader, "timeDelta"), 1.0/60.0/PHYS_STEPS_PER_FRAME);
		
		glGenTextures(2, &physBufTex3D);
		
		glUniform1i(glGetUniformLocation(physicsShader, "allVerts3D"), 0);
		glUniform1i(glGetUniformLocation(physicsShader, "allVerts4D"), 1);
		

		addKeyListener(GLFW_KEY_P, [this](int scancode, int action, int mods) {
			if (action == GLFW_PRESS) paused = !paused;
		});
		addKeyListener(GLFW_KEY_PERIOD, [this](int scancode, int action, int mods) {
			if (action == GLFW_PRESS) {
				doingStep = true;
				paused = true;
			}
		});
	}

	void setDrawAttrs() {
		glBindBuffer(GL_ARRAY_BUFFER, physBuf1.data3D);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(PhysData3D), (void*) offsetof(PhysData3D, pos));
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, physBuf1.data4D);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(PhysData4D), (void*) offsetof(PhysData4D, turn));
		glEnableVertexAttribArray(1);
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
	
	void physicsStep(PhysBuffers inVBOs, PhysBuffers outVBOs) {
		
		// Clear the out buffer
		glBindBuffer(GL_ARRAY_BUFFER, outVBOs.data3D);
		glBufferData(GL_ARRAY_BUFFER, physVBO3DSize, nullptr, GL_STREAM_COPY);
		glBindBuffer(GL_ARRAY_BUFFER, outVBOs.data4D);
		glBufferData(GL_ARRAY_BUFFER, physVBO3DSize, nullptr, GL_STREAM_COPY);

		
		glBindBuffer(GL_ARRAY_BUFFER, inVBOs.data3D);
		// inPos
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(PhysData3D), (void*) offsetof(PhysData3D, pos));
		// inVel
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(PhysData3D), (void*) offsetof(PhysData3D, vel));
		// inAngVel
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(PhysData3D), (void*) offsetof(PhysData3D, angVel));
		// inTurn
		glBindBuffer(GL_ARRAY_BUFFER, inVBOs.data4D);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(PhysData4D), (void*) offsetof(PhysData4D, turn));
		
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, outVBOs.data3D);
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1, outVBOs.data4D);
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 2, debugFeedbackVBO);
		
		
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_BUFFER, physBufTex3D);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, inVBOs.data3D);
		
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_BUFFER, physBufTex4D);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, inVBOs.data4D);
		
		
		glBeginTransformFeedback(GL_POINTS);
		glDrawArrays(GL_POINTS, 0, toRender.vertsPos.size());
		glEndTransformFeedback();
		//glFlush();
	}
	
	
	void doPhysics() {
		if (paused && !doingStep) return;

		glUseProgram(physicsShader);
		glBindVertexArray(physVAO);
		glEnable(GL_RASTERIZER_DISCARD);
		
		assert(PHYS_STEPS_PER_FRAME % 2 == 0);
		assert(PHYS_STEPS_PER_FRAME / 2 % SLOWDOWN_FACTOR == 0);
		
		int stepsToDo = PHYS_STEPS_PER_FRAME / 2 / SLOWDOWN_FACTOR;
		if (doingStep) {
			stepsToDo = 1;
			doingStep = false;
		}

		for (int i = 0; i < stepsToDo; ++i) {
			physicsStep(physBuf1, physBuf2);
			physicsStep(physBuf2, physBuf1);
		}
		
		glCheckError();
		
		glDisable(GL_RASTERIZER_DISCARD);
	}
	
	void render(glm::mat4 view, glm::mat4 projection) override {
		
		doPhysics();
		
		glm::mat4 model(1.0f);
		model = glm::translate(model, glm::vec3(-RADIUS, -RADIUS, -RADIUS*2));
		
		auto totalTransform = projection * view * model;
		
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_DEPTH_TEST);
		
		drawVoxels(totalTransform);
		//drawVectors(totalTransform);
		
		//glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		
		//usleep(500000);
	}
	void drawVoxels(glm::mat4 transform) {

		glBindVertexArray(voxelRenderVAO);
		glUseProgram(voxelRenderShader);
		glUniformMatrix4fv(glGetUniformLocation(voxelRenderShader, "transform"), 1, GL_FALSE, glm::value_ptr(transform));

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, cubeTexture);
		
		//glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		
		// Draw just the outside voxels
		//glDrawElements(GL_POINTS, toRender.edgeIndices.size(), GL_UNSIGNED_INT, nullptr);
		
		// Draw everything
		glDrawArrays(GL_POINTS, 0, toRender.vertsPos.size());
	}
	void drawVectors(glm::mat4 transform) {
		glBindVertexArray(vectorRenderVAO);
		glUseProgram(vectorRenderShader);
		glUniformMatrix4fv(glGetUniformLocation(vectorRenderShader, "transform"), 1, GL_FALSE, glm::value_ptr(transform));
		
		glDrawArrays(GL_POINTS, 0, toRender.vertsPos.size());
	}
	
};

std::unique_ptr<VoxelRenderer> getVoxelRenderer() {
	return std::make_unique<VoxelRendererImpl>();
}



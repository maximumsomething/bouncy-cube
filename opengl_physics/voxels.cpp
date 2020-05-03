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
	struct CubeData {
		CubeData() {
			for (auto& i : neighbors) {
				i = -1;
			}
		};
		
		// Order: -x, -y, -z, +x, +y, +z
		int32_t neighbors[6];
	};
	struct VertNeighbors {
		// Order: mmm, mmp, mpm, mpp, pmm, pmp, ppm, ppp
		int32_t neighbors[8];
		VertNeighbors() {
			for (auto& i : neighbors) {
				i = -1;
			}
		};
	};
	
	std::vector<glm::vec3> cubesPos;
	std::vector<CubeData> cubesData;
	//std::vector<uint32_t> edgeIndices;
	std::vector<VertNeighbors> vertsNeighbors;
	std::vector<uint32_t> faceIndices;
	
	VoxelStorage(arrayND<bool, 3> storage) : storage(storage) {
		
		setCubes();
		//edgeIndices = getEBO();
		
	}
	
	// Every vert is guaranteed to be either a positive x, y, or z in front of the previous vertex
private:
	void setCubes() {
		cubesPos.clear();
		cubesData.clear();
		
		// Contains the indices of the vertices in toReturn
		arrayND<int32_t, 3> indexMap(storage.sizes, -1);
		
		for (int i = 0; i < storage.total(); ++i) {
			if (storage.linear()[i]) {
				auto coord = storage.ind2coord(i);
				cubesPos.push_back(glm::vec3(coord[0], coord[1], coord[2]));
				cubesData.emplace_back();
				
				indexMap.linear()[i] = cubesData.size() - 1;
				
				// Get the neighbors above, but not below
				for (int j = 0; j < 3; ++j) {
					if (coord[j] > 0) {

						auto neighborCoord = coord;
						neighborCoord[j] -= 1;
						if (storage[neighborCoord]) {
							assert(indexMap[neighborCoord] >= 0);
							cubesData.back().neighbors[j] = indexMap[neighborCoord];
							cubesData[indexMap[neighborCoord]].neighbors[j + 3] = cubesData.size() - 1;
						}
					}
				}
			}
		}
		
		arrayND<int32_t, 3> cornerIndexMap(
		{ storage.sizes[0] + 1, storage.sizes[1] + 1, storage.sizes[2] + 1 }, -1);
		
		// Get vertices between cubes
		for (int z = 0; z <= storage.sizes[2]; ++z)
		for (int y = 0; y <= storage.sizes[1]; ++y)
		for (int x = 0; x <= storage.sizes[0]; ++x) {
			
			VertNeighbors thisVert;
			bool allNeighExists = true, allNeighAir = true;
			
			for (unsigned int i = 0; i < 8; ++i) {
				int cubeX = x - !(i & 1);
				int cubeY = y - !(i & 2);
				int cubeZ = z - !(i & 4);
				
				if (cubeX >= 0 && cubeY >= 0 && cubeZ >= 0
					&& cubeX < storage.sizes[0] && cubeY < storage.sizes[1] && cubeZ < storage.sizes[2]) {
					thisVert.neighbors[i] = indexMap[cubeX][cubeY][cubeZ];
				}
				allNeighExists = allNeighExists && thisVert.neighbors[i] != -1;
				allNeighAir = allNeighAir && thisVert.neighbors[i] == -1;
			}
			
			if (!allNeighExists && !allNeighAir) {
				vertsNeighbors.push_back(thisVert);
				cornerIndexMap[x][y][z] = vertsNeighbors.size() - 1;
			}
		}
		
		// Make faces from those vertices
		for (int i = 0; i < cubesData.size(); ++i) {
			arrayND<int32_t, 3>::sizesT cubePos = {
				(size_t) cubesPos[i].x,  (size_t) cubesPos[i].y, (size_t) cubesPos[i].z
			};
			for (int j = 0; j < 6; ++j) {
				if (cubesData[i].neighbors[j] == -1) {
					arrayND<int32_t, 3>::sizesT cornerPos = cubePos;
					cornerPos[j % 3] += j / 3;
					/*
					// Draw two triangles to make a face
					faceIndices.push_back(cornerIndexMap[cornerPos]);
					cornerPos[(j + 1) % 3] += 1;
					faceIndices.push_back(cornerIndexMap[cornerPos]);
					cornerPos[(j + 1) % 3] -= 1;
					cornerPos[(j + 2) % 3] += 1;
					faceIndices.push_back(cornerIndexMap[cornerPos]);
					faceIndices.push_back(cornerIndexMap[cornerPos]);
					cornerPos[(j + 1) % 3] += 1;
					faceIndices.push_back(cornerIndexMap[cornerPos]);
					cornerPos[(j + 2) % 3] -= 1;
					faceIndices.push_back(cornerIndexMap[cornerPos]);
					 */
					// Draw a quad which gets processed by geometry shader
					faceIndices.push_back(cornerIndexMap[cornerPos]);
					cornerPos[(j + 1) % 3] += 1;
					faceIndices.push_back(cornerIndexMap[cornerPos]);
					cornerPos[(j + 2) % 3] += 1;
					faceIndices.push_back(cornerIndexMap[cornerPos]);
					cornerPos[(j + 1) % 3] -= 1;
					faceIndices.push_back(cornerIndexMap[cornerPos]);
				}
			}
		}
	}
	
	
	std::vector<uint32_t> getEBO() {
		std::vector<uint32_t> EBO;
		for (uint32_t i = 0; i < cubesData.size(); ++i) {
			for (auto j : cubesData[i].neighbors) {
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

constexpr bool DRAW_CUBES = false;
constexpr bool DRAW_VECTORS = false;


const GLchar * physOutputs[] = {
	"outPos", "outVel", "outAngVel",
	"gl_NextBuffer", "outTurn",
	"gl_NextBuffer", "debugFeedback" };

// Renders everything
class VoxelRendererImpl : public VoxelRenderer {
public:

GLuint voxelRenderShader, vectorRenderShader, physicsShader, pickingShader;
GLuint cubeTexture = loadTexture("rubber.jpg");
//GLuint cubeTexture = loadTexture("astroturf-2.jpeg");

VoxelStorage toRender{genSphere(RADIUS)};

// PhysVBO is read and written by the physics code. DebugFeedbackVBO is written to but not read by the physics code, and DataVBO is read but not written to by the physics code. All three are read by the drawing code.
	GLuint voxelRenderVAO, vectorRenderVAO, physVAO, debugFeedbackVBO, dataVBO, vertNeighborVBO, EBO, physBufTex3D, physBufTex4D, feedbackBufTex;

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

bool paused = true, doingStep = false;



VoxelRendererImpl() :
vectorRenderShader(linkShaders({
	loadShader("vectors.vert", GL_VERTEX_SHADER),
	loadShader("vectors.frag", GL_FRAGMENT_SHADER),
	loadShader("vectors.geom", GL_GEOMETRY_SHADER)
})),
physicsShader(linkShaders({
	loadShader("sim.vert", GL_VERTEX_SHADER)}, true, [] (GLuint toBeLinked) {
	glTransformFeedbackVaryings(toBeLinked, sizeof(physOutputs) / sizeof(physOutputs[0]), physOutputs, GL_INTERLEAVED_ATTRIBS);
})) {
	GLuint voxelVert, voxelGeom;
	
	if (DRAW_CUBES) {
		voxelVert = loadShader("voxels.vert", GL_VERTEX_SHADER);
		voxelGeom = loadShader("voxels.geom", GL_GEOMETRY_SHADER);
	}
	else {
		voxelVert = loadShader("stretchyVoxels.vert", GL_VERTEX_SHADER);
		voxelGeom = loadShader("stretchyVoxels.geom", GL_GEOMETRY_SHADER);
	}
	GLuint voxelFrag = loadShader("voxels.frag", GL_FRAGMENT_SHADER);
	GLuint pickingFrag = loadShader("picking.frag", GL_FRAGMENT_SHADER);
	
	voxelRenderShader = linkShaders({voxelVert, voxelFrag, voxelGeom}, false);
	glDeleteShader(voxelFrag);
	pickingShader = linkShaders({voxelVert, pickingFrag, voxelGeom});
	
	
	
	glGenVertexArrays(3, &voxelRenderVAO);
	glBindVertexArray(voxelRenderVAO);
	
	// Set the initial positions
	std::vector<PhysData3D> initPhysData(toRender.cubesPos.size());
	std::vector<PhysData4D> initTurn(toRender.cubesPos.size());
	for (unsigned i = 0; i < toRender.cubesPos.size(); ++i) {
		initPhysData[i].pos = toRender.cubesPos[i];
		//if (i < 10) initPhysData[i].vel = glm::vec3(1, 1, 1);
		//if (i > toRender.vertsPos.size() - 51) initPhysData[i].vel = glm::vec3(100, 0, 0);
		//if (i < 50) initPhysData[i].vel = glm::vec3(-100, 0, 0);
		//initPhysData[i].angVel = glm::vec3(0, 30, 0);
	}
	
	physVBO3DSize = toRender.cubesPos.size() * sizeof(PhysData3D);
	physVBO4DSize = toRender.cubesPos.size() * sizeof(PhysData4D);
	feedbackVBOSize = toRender.cubesPos.size() * sizeof(float);
	
	glBindBuffer(GL_ARRAY_BUFFER, physBuf1.data3D);
	glBufferData(GL_ARRAY_BUFFER, physVBO3DSize, initPhysData.data(), GL_STREAM_COPY);
	glBindBuffer(GL_ARRAY_BUFFER, physBuf1.data4D);
	glBufferData(GL_ARRAY_BUFFER, physVBO4DSize, initTurn.data(), GL_STREAM_COPY);
	
	if (DRAW_CUBES) setPosTurnDrawAttrs();
	
	// Generate the non-physics buffers
	glGenBuffers(4, &debugFeedbackVBO);


	glBindBuffer(GL_ARRAY_BUFFER, debugFeedbackVBO);
	float* clearData = (float *) calloc(1, feedbackVBOSize);
	glBufferData(GL_ARRAY_BUFFER, feedbackVBOSize, clearData, GL_STREAM_COPY);
	free(clearData);
	if (DRAW_CUBES) {
		// draw debug feedback
		glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(float), 0);
		glEnableVertexAttribArray(2);
	}
	// neighbor data
	glBindBuffer(GL_ARRAY_BUFFER, dataVBO);
	glBufferData(GL_ARRAY_BUFFER, toRender.cubesData.size() * sizeof(VoxelStorage::CubeData), toRender.cubesData.data(), GL_STATIC_DRAW);
	if (DRAW_CUBES) {
		setVertDataAttrs(voxelRenderShader);
		setVertDataAttrs(pickingShader);
	}
	else {
		glBindBuffer(GL_ARRAY_BUFFER, vertNeighborVBO);
		glBufferData(GL_ARRAY_BUFFER, toRender.vertsNeighbors.size() * sizeof(VoxelStorage::VertNeighbors), toRender.vertsNeighbors.data(), GL_STATIC_DRAW);
		
		glVertexAttribIPointer(0, 4, GL_INT, sizeof(int32_t) * 8, (void *) 0);
		glEnableVertexAttribArray(0);
		glVertexAttribIPointer(1, 4, GL_INT, sizeof(int32_t) * 8, (void *) (sizeof(int32_t) * 4));
		glEnableVertexAttribArray(1);

		initBufferTextures(voxelRenderShader);
		initBufferTextures(pickingShader);
		
		glUseProgram(voxelRenderShader);
		glUniform1i(glGetUniformLocation(voxelRenderShader, "debugFeedback"), 3);
	}
	
	glUseProgram(voxelRenderShader);
	glUniform1i(glGetUniformLocation(voxelRenderShader, "cubeTexture"), 2);
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

	if (DRAW_CUBES) {
		//glBufferData(GL_ELEMENT_ARRAY_BUFFER, toRender.edgeIndices.size() * sizeof(uint32_t), toRender.edgeIndices.data(), GL_STATIC_DRAW);
	}
	else {
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, toRender.faceIndices.size() * sizeof(uint32_t), toRender.faceIndices.data(), GL_STATIC_DRAW);
	}
	
	if (DRAW_VECTORS) {
		// Drawing debug vectors
		glBindVertexArray(vectorRenderVAO);
		setPosTurnDrawAttrs();
		glBindBuffer(GL_ARRAY_BUFFER, physBuf1.data3D);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(PhysData3D), (void*) offsetof(PhysData3D, vel));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(PhysData3D), (void*) offsetof(PhysData3D, angVel));
		glEnableVertexAttribArray(3);
	}

	// Physics renderer
	glBindVertexArray(physVAO);
	glUseProgram(physicsShader);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);
	
	setVertDataAttrs(physicsShader);
	
	glUniform1f(glGetUniformLocation(physicsShader, "timeDelta"), 1.0/60.0/PHYS_STEPS_PER_FRAME);
	
	glGenTextures(3, &physBufTex3D);
	initBufferTextures(physicsShader);
	
	initPicking();

	addKeyListener(GLFW_KEY_P, [this](int scancode, int action, int mods) {
		if (action == GLFW_PRESS) paused = !paused;
	});
	addKeyListener(GLFW_KEY_PERIOD, [this](int scancode, int action, int mods) {
		if (action == GLFW_PRESS) {
			doingStep = true;
			paused = true;
		}
	});
	
	addClickListener([this](int button, int action, int mods) {
		if (button == GLFW_MOUSE_BUTTON_LEFT) {
			if (action == GLFW_PRESS) getClickPos();
			else if (action == GLFW_RELEASE) mouseDragEnd();
		}
	});
}
	
void initBufferTextures(GLuint shader) {
	glUseProgram(shader);
	glUniform1i(glGetUniformLocation(shader, "allVerts3D"), 0);
	glUniform1i(glGetUniformLocation(shader, "allVerts4D"), 1);
}
void bindBufferTextures(PhysBuffers VBOs) {
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_BUFFER, physBufTex3D);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, VBOs.data3D);
	
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_BUFFER, physBufTex4D);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, VBOs.data4D);
}

void setPosTurnDrawAttrs() {
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
	glVertexAttribIPointer(neighborsM, 3, GL_INT, sizeof(VoxelStorage::CubeData), 0);
	glEnableVertexAttribArray(neighborsM);
	GLint neighborsP = glGetAttribLocation(shader, "neighborsP");
	glVertexAttribIPointer(neighborsP, 3, GL_INT, sizeof(VoxelStorage::CubeData), (void *) offsetof(VoxelStorage::CubeData, neighbors[3]));
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
	
	
	bindBufferTextures(inVBOs);
	
	glBeginTransformFeedback(GL_POINTS);
	glDrawArrays(GL_POINTS, 0, toRender.cubesPos.size());
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

glm::mat4 prevTransform{1.0f};
	
void render(glm::mat4 view, glm::mat4 projection) override {
	
	doPhysics();
	
	glm::mat4 model(1.0f);
	model = glm::translate(model, glm::vec3(-RADIUS, -RADIUS, -RADIUS*2));
	
	auto totalTransform = projection * view * model;
	prevTransform = totalTransform;
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	
	
	drawVoxels(totalTransform, pickingShader);
	if (DRAW_VECTORS) {
		glUseProgram(vectorRenderShader);
		drawVectors(totalTransform);
	}
	
	//glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	
	//usleep(500000);
}
void drawVoxels(glm::mat4 transform, GLuint shader) {

	glBindVertexArray(voxelRenderVAO);
	glUseProgram(shader);
	glUniformMatrix4fv(glGetUniformLocation(shader, "transform"), 1, GL_FALSE, glm::value_ptr(transform));

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, cubeTexture);
	
	if (DRAW_CUBES) {
	
		// Draw just the outside voxels
		//glDrawElements(GL_POINTS, toRender.edgeIndices.size(), GL_UNSIGNED_INT, nullptr);
		
		// Draw everything
		glDrawArrays(GL_POINTS, 0, toRender.cubesPos.size());
	}
	else {
		bindBufferTextures(physBuf1);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_BUFFER, feedbackBufTex);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, debugFeedbackVBO);
		
		glDrawElements(GL_LINES_ADJACENCY, toRender.faceIndices.size(), GL_UNSIGNED_INT, nullptr);
	}
}
void drawVectors(glm::mat4 transform) {
	glBindVertexArray(vectorRenderVAO);
	glUniformMatrix4fv(glGetUniformLocation(vectorRenderShader, "transform"), 1, GL_FALSE, glm::value_ptr(transform));
	
	glDrawArrays(GL_POINTS, 0, toRender.cubesPos.size());
}

	
GLuint pickerFBO, pickerColorBuf, pickerDepthStencilBuf;
void initPicking() {
	glGenFramebuffers(1, &pickerFBO);
	glGenRenderbuffers(2, &pickerColorBuf);
	
	glBindRenderbuffer(GL_RENDERBUFFER, pickerColorBuf);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, 100, 100);
	glBindRenderbuffer(GL_RENDERBUFFER, pickerDepthStencilBuf);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 100, 100);
	
	glBindFramebuffer(GL_FRAMEBUFFER, pickerFBO);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, pickerColorBuf);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, pickerDepthStencilBuf);
	
	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	std::cerr << "Picking framebuffer is not complete!" << std::endl;
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
	
void getClickPos() {
	glm::dvec2 virtualCursor;
	glfwGetCursorPos(windowData.window, &virtualCursor.x, &virtualCursor.y);
	
	// COLUMN WISE!
	glm::mat4 pixelTransform(
		windowData.width, 0, 0, 0,
		0, windowData.height, 0, 0,
		0, 0, 1, 0,
		windowData.width - virtualCursor.x * 2, virtualCursor.y * 2 - windowData.height, 0, 1
	);
	int oldViewport[4];
	glGetIntegerv(GL_VIEWPORT, oldViewport);
	glBindFramebuffer(GL_FRAMEBUFFER, pickerFBO);
	glViewport(0, 0, 100, 100);
	
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	drawVoxels(pixelTransform * prevTransform, pickingShader);
	
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	//std::vector<uint8_t> data(40000);
	//glReadPixels(0, 0, 100, 100, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
	//dumpImage(data.data(), 100, 100);
	
	uint8_t pickedPixel[4];
	glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pickedPixel);
	
	std::cout << "clicked r:" << (int) pickedPixel[0] << " g:" << (int) pickedPixel[1] << " b:" << (int) pickedPixel[2] << " a:" << (int) pickedPixel[3] << std::endl;
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]);
}

void mouseDragEnd() {
	
}
	
};

std::unique_ptr<VoxelRenderer> getVoxelRenderer() {
	return std::make_unique<VoxelRendererImpl>();
}



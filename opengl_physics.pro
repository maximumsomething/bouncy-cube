# Created by and for Qt Creator This file was created for editing the project sources only.
# You may attempt to use it for building too, by modifying this file here.

TARGET = opengl_physics

HEADERS = \
   $$PWD/opengl_physics/arrayND.hpp \
   $$PWD/opengl_physics/bridge.hpp \
   $$PWD/opengl_physics/loaders.hpp \
   $$PWD/opengl_physics/voxels.hpp \
    opengl_physics/input.hpp

SOURCES = \
   $$PWD/opengl_physics/libs/glad.c \
   $$PWD/opengl_physics/bridge_linux_test.cpp \
   $$PWD/opengl_physics/loaders.cpp \
   $$PWD/opengl_physics/main.cpp \
   $$PWD/opengl_physics/voxels.cpp \
    opengl_physics/input.cpp

INCLUDEPATH = $$PWD/opengl_physics/include/

CONFIG += c++14

LIBS += -ldl -lglfw

#DEFINES = 

DISTFILES += \
    opengl_physics/shaders/axisAngle.glsl \
    opengl_physics/shaders/quaternion.glsl \
    opengl_physics/shaders/vectors.geom \
    opengl_physics/shaders/voxels.geom \
    opengl_physics/shaders/intro.frag \
    opengl_physics/shaders/skybox.frag \
    opengl_physics/shaders/vectors.frag \
    opengl_physics/shaders/voxels.frag \
    opengl_physics/shaders/intro.vert \
    opengl_physics/shaders/sim.vert \
    opengl_physics/shaders/skybox.vert \
    opengl_physics/shaders/vectors.vert \
    opengl_physics/shaders/voxels.vert


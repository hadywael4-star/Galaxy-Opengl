# 3D Solar System

This project is a C++ OpenGL scene built with SFML, GLEW, and GLM. It renders a textured solar system with a cubemap skybox, animated planets, a moon orbiting Earth, and a movable camera.

## Overview

The application opens a 3D window titled `3D Solar System` and draws:

- A textured sun at the center of the scene
- Eight orbiting planets with individual textures
- A moon orbiting Earth
- A surrounding space skybox
- Per-fragment lighting using a simple Phong shader

The scene is animated in real time, with orbit and rotation updated each frame.

## Features

- Textured spheres generated procedurally through recursive subdivision
- Cubemap skybox for the background
- Phong-style lighting with ambient, diffuse, and specular components
- Emissive sun rendering
- Perspective and orthographic projection modes
- Mouse-look camera and keyboard movement

## Controls

- `W`, `A`, `S`, `D`: Move the camera forward, left, backward, and right
- `Space`: Move the camera up
- `Left Shift`: Move the camera down
- Mouse movement: Look around
- `O`: Toggle between perspective and orthographic projection
- `Esc`: Close the window

## Project Structure

- `Project/Source.cpp`: Main application, rendering loop, input handling, and solar-system setup
- `Project/ShaderFunctions.cpp`: Shader loading and compilation helpers
- `Project/VS_Tex.glsl` and `Project/FS_Tex.glsl`: Lighting shader pair for textured spheres
- `Project/Skybox_VS.glsl` and `Project/Skybox_FS.glsl`: Skybox shader pair
- `Textures/`: Planet and sun textures
- `Skybox/`: Cubemap faces used for the space background

## Build Notes

The Visual Studio solution is configured to use the local `OpenGLSFML` dependency folder included in the repository.

To build:

1. Open `Project.sln` in Visual Studio.
2. Select the desired configuration and platform.
3. Build and run the `Project` application.

## Snapshots

The two screenshots below show the solar-system scene from different camera positions.

![Solar system snapshot 1](docs/Solar%20system%20snapshot%201.png)

![Solar system snapshot 2](docs/Solar%20system%20snapshot%202.png)

## Implementation Notes

- The sun acts as the light source at the origin.
- Planet orbits and self-rotation are updated every frame using independent speeds.
- Earth has an additional orbiting moon.
- The camera uses yaw/pitch rotation and can switch between projection modes.

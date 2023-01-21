#version 330 core

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 vertexPosition_modelspace;
layout(location = 1) in vec2 vertexUV;
layout(location = 2) in vec3 vertexNormal_modelspace;

out vec3 Position_modelspace;
out vec2 UV;
out vec3 Normal_modelspace;

void main() {
  // Pass to tesselation control shader
  gl_Position = vec4(vertexPosition_modelspace, 1);

  // Output data
  Position_modelspace = vertexPosition_modelspace;
  UV = vertexUV;
  Normal_modelspace = vertexNormal_modelspace;
}

#version 330 core

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 vertexPosition_modelspace;
layout(location = 1) in vec2 vertexUV;
layout(location = 2) in vec3 vertexNormal_modelspace;

// Output data ; will be interpolated for each fragment.
out vec2 UV;
out vec3 Position_worldspace;
out vec3 EyeDirection_cameraspace;
out vec3 LightDirection_cameraspace;
out vec3 Normal_cameraspace;

out vec3 LightDirection_tangentspace;
out vec3 EyeDirection_tangentspace;

// Values that stay constant for the whole mesh.
uniform sampler2D HeightMapTextureSampler;
uniform float HeightScale;
uniform mat4 MVP;
uniform mat4 V;
uniform mat4 M;
uniform mat3 MV3x3;
uniform vec3 LightPosition_worldspace;

float sampleHeightMap() {
  vec4 sampleNormalised = texture(HeightMapTextureSampler, vertexUV);
  uvec4 sample = uvec4(sampleNormalised * 255);
  float height = float(sample.r << 16 | sample.g << 8 | sample.b);
  return height * HeightScale;
}

void main() {
  // Output position of the vertex, in clip space : MVP * position
  vec3 vertexPosition_displaced = vertexPosition_modelspace;
  vertexPosition_displaced.y = sampleHeightMap();
  gl_Position = MVP * vec4(vertexPosition_displaced, 1);

  // Position of the vertex, in worldspace : M * position
  Position_worldspace = (M * vec4(vertexPosition_displaced, 1)).xyz;

  // Vector that goes from the vertex to the camera, in camera space.
  // In camera space, the camera is at the origin (0,0,0).
  vec3 vertexPosition_cameraspace =
      (V * M * vec4(vertexPosition_displaced, 1)).xyz;
  EyeDirection_cameraspace = vec3(0, 0, 0) - vertexPosition_cameraspace;

  // Vector that goes from the vertex to the light, in camera space. M is
  // ommited because it's identity.
  vec3 LightPosition_cameraspace = (V * vec4(LightPosition_worldspace, 1)).xyz;
  LightDirection_cameraspace =
      -LightPosition_cameraspace; // + EyeDirection_cameraspace;

  // UV of the vertex. No special space for this one.
  UV = vertexUV;

  // model to camera = ModelView
  Normal_cameraspace = MV3x3 * vertexNormal_modelspace;
}

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
out vec3 Normal_modelspace;

out vec3 LightDirection_tangentspace;
out vec3 EyeDirection_tangentspace;

// Values that stay constant for the whole mesh.
uniform sampler2D HeightMapTextureSampler;
uniform vec2 HeightMapUVStepSize; // The distance between pixels in UV space
uniform float HeightScale;
uniform mat4 MVP;
uniform mat4 V;
uniform mat4 M;
uniform mat3 MV3x3;
uniform vec3 LightPosition_worldspace;

float sampleHeightMap(vec2 sampleUV) {
  vec4 sampleNormalised = texture(HeightMapTextureSampler, sampleUV);
  uvec4 sample = uvec4(sampleNormalised * 255);
  // Sample the raw height value
  return float(sample.r << 16 | sample.g << 8 | sample.b) / 255.0;
}

vec3 sampleNormalMap(vec2 sampleUV) {
  float Nx = 0;
  for (int row = -1; row <= 1; row++) {
    float heightDiff = sampleHeightMap(sampleUV + vec2(HeightMapUVStepSize.x, row)) -
                       sampleHeightMap(sampleUV + vec2(-HeightMapUVStepSize.x, row));
    Nx += heightDiff;
  }
  Nx /= 3.0;

  float Nz = 0;
  for (int col = -1; col <= 1; col++) {
    float heightDiff = sampleHeightMap(sampleUV + vec2(col, HeightMapUVStepSize.y)) -
                       sampleHeightMap(sampleUV + vec2(col, -HeightMapUVStepSize.y));
    Nz += heightDiff;
  }
  Nz /= 3.0;

  return normalize(vec3(Nx, 1, Nz));
}

void main() {
  vec3 vertexPosition_displaced = vertexPosition_modelspace;
  // vec3 vertexNormal_displaced = sampleNormalMap(vertexUV);
  /* Uncomment to disable normals */
  vec3 vertexNormal_displaced = vertexNormal_modelspace;

  // Output position of the vertex, in clip space : MVP * position
  float rawHeight = sampleHeightMap(vertexUV);
  // Bring into a more acceptible range
  float height = (rawHeight / 256.0) * HeightScale;

  vertexPosition_displaced.y = height;
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
  Normal_cameraspace = MV3x3 * vertexNormal_displaced;
  Normal_modelspace = vertexNormal_displaced;
}

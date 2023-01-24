#version 410 core

layout(quads, fractional_even_spacing, ccw) in;

in vec3 controlPosition_modelspace[];
in vec2 controlUV[];
in vec3 controlNormal_modelspace[];

out vec2 UV;
out vec3 Position_worldspace;
out vec3 EyeDirection_cameraspace;
out vec3 LightDirection_cameraspace;
out vec3 Normal_cameraspace;
out vec3 Normal_modelspace;

uniform sampler2D HeightMapTextureSampler;
uniform vec2 HeightMapUVStepSize;
uniform float HeightScale;
uniform mat4 MVP;
uniform mat4 M;
uniform mat4 V;
uniform mat3 MV3x3;
uniform vec3 LightPosition_worldspace;

float sampleHeightMap(vec2 sampleUV) {
  vec4 sampleNormalised = texture(HeightMapTextureSampler, sampleUV);
  uvec4 s = uvec4(sampleNormalised * 255);
  // Sample the raw height value
  return float(s.r << 16 | s.g << 8 | s.b) / 255.0;
}

vec3 sampleNormalMap(vec2 sampleUV) {
  vec2 xStep = vec2(HeightMapUVStepSize.x, 0);
  float leftHeight = sampleHeightMap(sampleUV - xStep);
  float rightHeight = sampleHeightMap(sampleUV + xStep);
  vec3 xNormal = vec3(1, 0, rightHeight - leftHeight);

  vec2 yStep = vec2(0, HeightMapUVStepSize.y);
  float topHeight = sampleHeightMap(sampleUV - yStep);
  float bottomHeight = sampleHeightMap(sampleUV + yStep);
  vec3 yNormal = vec3(0, 1, bottomHeight - topHeight);

  return normalize(cross(xNormal, yNormal));
}

// I would make this multi line if Apple decided not to deprecate OpenGL
#define INTERPOLATE_FUNCTION(gentype)                                   \
  gentype interpolate(gentype v0, gentype v1, gentype v2, gentype v3) { \
    gentype a = mix(v0, v1, gl_TessCoord.x);                            \
    gentype b = mix(v2, v3, gl_TessCoord.x);                            \
    return mix(a, b, gl_TessCoord.y);                                   \
  }
INTERPOLATE_FUNCTION(vec2)
INTERPOLATE_FUNCTION(vec3)
INTERPOLATE_FUNCTION(vec4)

void main() {
  vec4 interpPos = interpolate(gl_in[0].gl_Position, gl_in[1].gl_Position,
                               gl_in[2].gl_Position, gl_in[3].gl_Position);
  UV = interpolate(controlUV[0], controlUV[1], controlUV[2], controlUV[3]);

  vec3 vertexPosition_displaced = interpPos.xyz;
  vec3 vertexNormal_displaced = sampleNormalMap(UV);
  // /* Uncomment to disable normals */
  // vertexNormal_displaced = interpolate(controlNormal_modelspace[0], controlNormal_modelspace[1],
  //                                      controlNormal_modelspace[2], controlNormal_modelspace[3]);

  // Output position of the vertex, in clip space : MVP * position
  float rawHeight = sampleHeightMap(UV);
  // Bring into a more acceptible range
  float height = (rawHeight / 256.0) * HeightScale;

  vertexPosition_displaced.y = height;
  gl_Position = MVP * vec4(vertexPosition_displaced, 1);

  // Position of the vertex, in worldspace : M * position
  Position_worldspace = (M * vec4(vertexPosition_displaced, 1)).xyz;

  // Vector that goes from the vertex to the camera, in camera space.
  // In camera space, the camera is at the origin (0,0,0).
  vec3 vertexPosition_cameraspace = (V * M * vec4(vertexPosition_displaced, 1)).xyz;
  EyeDirection_cameraspace = vec3(0, 0, 0) - vertexPosition_cameraspace;

  // Vector that goes from the vertex to the light, in camera space. M is
  // ommited because it's identity.
  vec3 LightPosition_cameraspace = (V * vec4(LightPosition_worldspace, 1)).xyz;
  LightDirection_cameraspace = -LightPosition_cameraspace; // + EyeDirection_cameraspace;

  // model to camera = ModelView
  Normal_cameraspace = MV3x3 * vertexNormal_displaced;
  Normal_modelspace = vertexNormal_displaced;
}
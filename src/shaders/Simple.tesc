#version 410 core
#extension GL_ARB_tessellation_shader : enable

// Input data (most are just being passed
//             through to fragment unchanged)
in vec3 Position_modelspace[];
in vec2 UV[];
in vec3 Normal_modelspace[];

// Output data
out vec3 controlPosition_modelspace[];
out vec2 controlUV[];
out vec3 controlNormal_modelspace[];

layout(vertices = 4) out;

void main() {
  if (gl_InvocationID == 0) {
    gl_TessLevelOuter[0] = 8;
    gl_TessLevelOuter[1] = 8;
    gl_TessLevelOuter[2] = 8;
    gl_TessLevelOuter[3] = 8;

    gl_TessLevelInner[0] = 8;
    gl_TessLevelInner[1] = 8;
  }

  // Pass the vertex position
  gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;

  // Pass the vertex data
  controlPosition_modelspace[gl_InvocationID] = Position_modelspace[gl_InvocationID];
  controlUV[gl_InvocationID] = UV[gl_InvocationID];
  controlNormal_modelspace[gl_InvocationID] = Normal_modelspace[gl_InvocationID];
}
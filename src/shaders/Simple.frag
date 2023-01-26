#version 330 core

// Interpolated values from the vertex shaders
in vec2 UV;
in vec3 Position_worldspace;
in vec3 EyeDirection_cameraspace;
in vec3 LightDirection_cameraspace;
in vec3 Normal_cameraspace;
in vec3 Normal_modelspace;

// Ouput data
out vec3 color;

// Values that stay constant for the whole mesh.
uniform sampler2D TextureASampler;
uniform sampler2D TextureBSampler;
uniform sampler2D TextureCSampler;
uniform sampler2D TextureASpecularMapSampler;
uniform sampler2D TextureBSpecularMapSampler;
uniform sampler2D TextureCSpecularMapSampler;
uniform float HeightScale;
uniform float BandA;
uniform float BandB;
uniform float BandSizes;
uniform mat4 V;
uniform mat4 M;
uniform mat3 MV3x3;
uniform vec3 LightPosition_worldspace;

vec3 getTextureAtHeight(sampler2D sA, sampler2D sB, sampler2D sC, float height, vec2 texCoord) {
  // Lowest band, just return the first texture
  if (height < BandA * HeightScale) {
    return texture(sA, texCoord).rgb;
  }

  // Transition band, mix the two textures
  if (height < (BandA + BandSizes) * HeightScale) {
    float mixFactor = (height - (BandA * HeightScale)) / (BandSizes * HeightScale);
    return mix(texture(sA, texCoord).rgb, texture(sB, texCoord).rgb,
               mixFactor);
  }

  // Below BandB, just return the second texture
  if (height < BandB * HeightScale) {
    return texture(sB, texCoord).rgb;
  }

  // Transition between B and C
  if (height < (BandB + BandSizes) * HeightScale) {
    float mixFactor = (height - (BandB * HeightScale)) / (BandSizes * HeightScale);
    return mix(texture(sB, texCoord).rgb, texture(sC, texCoord).rgb,
               mixFactor);
  }

  // Above BandC, just return the third texture
  return texture(sC, texCoord).rgb;
}

void main() {

  // Some properties
  // should put them as uniforms
  vec3 LightColor = vec3(1, 1, 1);
  float LightPower = 1.0;
  float shininess = 1;

  vec2 texCoord = fract(UV * 6.0);

  // Material properties
  vec3 MaterialDiffuseColor = getTextureAtHeight(TextureASampler, TextureBSampler, TextureCSampler,
                                                 Position_worldspace.y, texCoord);
  vec3 MaterialAmbientColor = vec3(0.1, 0.1, 0.1) * MaterialDiffuseColor;
  vec3 MaterialSpecularColor = getTextureAtHeight(TextureASpecularMapSampler,
                                                  TextureBSpecularMapSampler,
                                                  TextureCSpecularMapSampler,
                                                  Position_worldspace.y, texCoord);

  // Distance to the light
  // float distance = length( LightPosition_worldspace - Position_worldspace );

  // Normal of the computed fragment, in camera space
  vec3 n = Normal_cameraspace;
  // Direction of the light (from the fragment to the light)
  vec3 l = normalize(LightDirection_cameraspace);
  vec3 e = normalize(EyeDirection_cameraspace);

  // Diffuse
  float cosTheta = clamp(dot(n, l), 0, 1);
  vec3 diffuse = MaterialDiffuseColor * LightColor * LightPower *
                 cosTheta; // (distance*distance) ;

  // Specular
  //  Eye vector (towards the camera)
  vec3 E = normalize(EyeDirection_cameraspace);
  // Direction in which the triangle reflects the light
  vec3 B = normalize(l + e);

  float cosB = clamp(dot(n, B), 0, 1);
  cosB = clamp(pow(cosB, shininess), 0, 1);
  cosB = cosB * cosTheta * (shininess + 2) / (2 * radians(180.0f));
  vec3 specular = MaterialSpecularColor * LightPower * cosB; //(distance*distance);

  // /* Uncomment to show normals instead */
  // color = Normal_modelspace;
  // return;

  // /* Uncomment to show UVs instead */
  // color = vec3(texCoord.xy, 0);
  // return;

  // color = specular;
  // return;

  color = MaterialAmbientColor + // Ambient : simulates indirect lighting
          diffuse +              // Diffuse : "color" of the object
          specular;              // Specular : reflective highlight, like a mirror
}
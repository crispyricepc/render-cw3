// Include standard headers
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

using namespace std;

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include <common/controls.hpp>

static const int window_width = 1920;
static const int window_height = 1080;

static const int n_points = 200;
static const float m_scale = 0.1f;
static const float m_band_a = 14;
static const float m_band_b = 25;
static const float m_band_sizes = 4;

// Variables
GLFWwindow *window;

// Shaders
GLuint programID;

// Textures
GLuint TextureA;
GLuint TextureB;
GLuint TextureC;
GLuint HeightMapTexture;

// Model
std::vector<unsigned int> indices;
std::vector<glm::vec3> vertices;
std::vector<glm::vec2> uvs;
std::vector<glm::vec3> normals;

// VAO
GLuint VertexArrayID;

// Buffers for VAO
GLuint vertexbuffer;
GLuint uvbuffer;
GLuint normalbuffer;
GLuint elementbuffer;

glm::vec2 heightMapUVStepSize;

// Processing command line arguments

struct CLIArgs {
  std::string modelPath = "";
  std::string textureA = "grass.bmp";
  std::string textureB = "rocks.bmp";
  std::string textureC = "snow.bmp";
  std::string heightMapPath = "";
};
CLIArgs processCLIArgs(int argc, char *argv[]) {
  CLIArgs args;

  for (int i = 1; i < argc; i++) {
    if (argv[i] == std::string("-m")) {
      args.modelPath = argv[i + 1];
      i++;
      continue;
    }

    if (argv[i] == std::string("-h")) {
      args.heightMapPath = argv[i + 1];
      i++;
      continue;
    }
  }

  return args;
}

void getErrors() {
  GLenum err;
  while ((err = glGetError()) != GL_NO_ERROR) {
    std::cout << "Error " << err << ": " << glewGetErrorString(err) << "\n";
  }
}

GLuint loadBMP_custom(const char *imagepath, GLenum filter_mode,
                      GLenum what_happens_at_edge, int &width, int &height) {

  printf("Reading image %s\n", imagepath);

  // Data read from the header of the BMP file
  unsigned char header[54];
  unsigned int dataPos;
  unsigned int imageSize;
  // Actual RGB data
  unsigned char *data;

  // Open the file
  FILE *file = fopen(imagepath, "rb");
  if (!file) {
    printf("%s could not be opened. Are you in the right directory ? !\n",
           imagepath);
    return 0;
  }

  // Read the header, i.e. the 54 first bytes

  // If less than 54 bytes are read, problem
  if (fread(header, 1, 54, file) != 54) {
    printf("Not a correct BMP file\n");
    fclose(file);
    return 0;
  }
  // A BMP files always begins with "BM"
  if (header[0] != 'B' || header[1] != 'M') {
    printf("Not a correct BMP file\n");
    fclose(file);
    return 0;
  }
  // Make sure this is a 24bpp file
  if (*(int *)&(header[0x1E]) != 0) {
    printf("Not a correct BMP file\n");
    fclose(file);
    return 0;
  }
  if (*(int *)&(header[0x1C]) != 24) {
    printf("Not a correct BMP file\n");
    fclose(file);
    return 0;
  }

  // Read the information about the image
  dataPos = *(int *)&(header[0x0A]);
  imageSize = *(int *)&(header[0x22]);
  width = *(int *)&(header[0x12]);
  height = *(int *)&(header[0x16]);

  // Some BMP files are misformatted, guess missing information
  if (imageSize == 0)
    imageSize = width * height *
                3; // 3 : one byte for each Red, Green and Blue component
  if (dataPos == 0)
    dataPos = 54; // The BMP header is done that way

  // Create a buffer
  data = new unsigned char[imageSize];

  // Read the actual data from the file into the buffer
  fseek(file, dataPos, SEEK_SET);
  fread(data, 1, imageSize, file);

  // Everything is in memory now, the file can be closed.
  fclose(file);

  // Create one OpenGL texture
  GLuint textureID;
  glGenTextures(1, &textureID);

  // "Bind" the newly created texture : all future texture functions will modify
  // this texture
  glBindTexture(GL_TEXTURE_2D, textureID);

  // Give the image to OpenGL
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR,
               GL_UNSIGNED_BYTE, data);

  // OpenGL has now copied the data. Free our own version
  delete[] data;

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, what_happens_at_edge);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, what_happens_at_edge);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter_mode);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter_mode);

  if (filter_mode != GL_NEAREST)
    glGenerateMipmap(GL_TEXTURE_2D);

  // unbind
  glBindTexture(GL_TEXTURE_2D, 0);

  // Return the ID of the texture we just created
  return textureID;
}

bool loadOBJ(const char *path, std::vector<glm::vec3> &out_vertices,
             std::vector<glm::vec2> &out_uvs,
             std::vector<glm::vec3> &out_normals,
             std::vector<unsigned int> &out_indices) {
  printf("Loading OBJ file %s...\n", path);

  std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
  std::vector<glm::vec3> temp_vertices;
  std::vector<glm::vec2> temp_uvs;
  std::vector<glm::vec3> temp_normals;

  FILE *file = fopen(path, "r");
  if (file == NULL) {
    printf("Impossible to open the file ! Are you in the right path ?\n");
    return false;
  }

  while (1) {

    char lineHeader[128];
    // read the first word of the line
    int res = fscanf(file, "%s", lineHeader);
    if (res == EOF)
      break; // EOF = End Of File. Quit the loop.

    // else : parse lineHeader

    if (strcmp(lineHeader, "v") == 0) {
      glm::vec3 vertex;
      fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
      temp_vertices.push_back(vertex);
    } else if (strcmp(lineHeader, "vt") == 0) {
      glm::vec2 uv;
      fscanf(file, "%f %f\n", &uv.x, &uv.y);
      temp_uvs.push_back(uv);
    } else if (strcmp(lineHeader, "vn") == 0) {
      glm::vec3 normal;
      fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
      temp_normals.push_back(normal);
    } else if (strcmp(lineHeader, "f") == 0) {
      std::string vertex1, vertex2, vertex3;
      unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
      int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n",
                           &vertexIndex[0], &uvIndex[0], &normalIndex[0],
                           &vertexIndex[1], &uvIndex[1], &normalIndex[1],
                           &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
      if (matches != 9) {
        printf("File can't be read by our simple parser :-( Try exporting with "
               "other options\n");
        fclose(file);
        return false;
      }
      vertexIndices.push_back(vertexIndex[0]);
      vertexIndices.push_back(vertexIndex[1]);
      vertexIndices.push_back(vertexIndex[2]);
      uvIndices.push_back(uvIndex[0]);
      uvIndices.push_back(uvIndex[1]);
      uvIndices.push_back(uvIndex[2]);
      normalIndices.push_back(normalIndex[0]);
      normalIndices.push_back(normalIndex[1]);
      normalIndices.push_back(normalIndex[2]);

    } else {
      // Probably a comment, eat up the rest of the line
      char stupidBuffer[1000];
      fgets(stupidBuffer, 1000, file);
    }
  }

  // For each vertex of each triangle
  for (unsigned int i = 0; i < vertexIndices.size(); i++) {

    // Get the indices of its attributes
    unsigned int vertexIndex = vertexIndices[i];
    unsigned int uvIndex = uvIndices[i];
    unsigned int normalIndex = normalIndices[i];

    // Get the attributes thanks to the index
    glm::vec3 vertex = temp_vertices[vertexIndex - 1];
    glm::vec2 uv = temp_uvs[uvIndex - 1];
    glm::vec3 normal = temp_normals[normalIndex - 1];

    // Put the attributes in buffers
    out_vertices.push_back(vertex);
    out_uvs.push_back(uv);
    out_normals.push_back(normal);
    out_indices.push_back(i);
  }
  fclose(file);
  return true;
}

int initializeGLFW() {
  // Initialise GLFW
  if (!glfwInit()) {
    fprintf(stderr, "Failed to initialize GLFW\n");
    getchar();
    return -1;
  }

  glfwWindowHint(GLFW_SAMPLES, 1);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,
                 GL_TRUE); // To make MacOS happy; should not be needed
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // Open a window and create its OpenGL context
  window = glfwCreateWindow(window_width, window_height, "OpenGLRenderer", NULL,
                            NULL);
  if (window == NULL) {
    fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, "
                    "they are not 3.3 compatible.\n");
    const char *description;
    int code = glfwGetError(&description);
    printf("Error code: %d, Description:\n  %s\n", code, description);
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);

  // Initialize GLEW
  glewExperimental = true; // Needed for core profile
  if (glewInit() != GLEW_OK) {
    fprintf(stderr, "Failed to initialize GLEW\n");
    getchar();
    glfwTerminate();
    return -1;
  }

  // Ensure we can capture the escape key being pressed below
  glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
  // Hide the mouse and enable unlimited mouvement
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  glfwPollEvents();
  glfwSetCursorPos(window, float(window_width) / 2, float(window_height) / 2);

  return 0;
}

bool readAndCompileShader(const char *shader_path, const GLuint &id) {
  // Read the Vertex Shader code from the file
  string VertexShaderCode;
  ifstream VertexShaderStream(shader_path, std::ios::in);
  if (VertexShaderStream.is_open()) {
    std::stringstream sstr;
    sstr << VertexShaderStream.rdbuf();
    VertexShaderCode = sstr.str();
    VertexShaderStream.close();
  } else {
    printf("Impossible to open %s. Are you in the right directory?\n",
           shader_path);
    return false;
  }

  // Compile Vertex Shader
  printf("Compiling shader : %s\n", shader_path);
  char const *VertexSourcePointer = VertexShaderCode.c_str();
  glShaderSource(id, 1, &VertexSourcePointer, NULL);
  glCompileShader(id);

  GLint Result = GL_FALSE;
  int InfoLogLength;

  // Check  Shader
  glGetShaderiv(id, GL_COMPILE_STATUS, &Result);
  glGetShaderiv(id, GL_INFO_LOG_LENGTH, &InfoLogLength);
  if (InfoLogLength > 0) {
    std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
    glGetShaderInfoLog(id, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
    printf("%s\n", &VertexShaderErrorMessage[0]);
  }
  std::cout << "Compilation of Shader: " << shader_path << " "
            << (Result == GL_TRUE ? "Success" : "Failed!") << std::endl;
  return Result == 1;
}

bool LoadShaders(GLuint &program, const char *vertex_file_path,
                 const char *fragment_file_path,
                 const char *tess_control_path = nullptr,
                 const char *tess_eval_file_path = nullptr,
                 const char *geometry_file_path = nullptr) {
  // Create the shaders - tasks 1 and 2
  GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
  GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

  // Create the shaders - task 3
  GLuint TesselationControlShaderID = 0;
  GLuint TesselationEvalShaderID = 0;

  // Create the shader - task 4
  GLuint GeometryShaderID = 0;

  readAndCompileShader(vertex_file_path, VertexShaderID);
  readAndCompileShader(fragment_file_path, FragmentShaderID);

  if (tess_control_path && tess_eval_file_path) {
    TesselationControlShaderID = glCreateShader(GL_TESS_CONTROL_SHADER);
    TesselationEvalShaderID = glCreateShader(GL_TESS_EVALUATION_SHADER);
    readAndCompileShader(tess_control_path, TesselationControlShaderID);
    readAndCompileShader(tess_eval_file_path, TesselationEvalShaderID);
  }

  if (geometry_file_path) {
    GeometryShaderID = glCreateShader(GL_GEOMETRY_SHADER);
    readAndCompileShader(geometry_file_path, GeometryShaderID);
  }

  GLint Result = GL_FALSE;
  int InfoLogLength;

  // Link the program
  printf("Linking program\n");
  program = glCreateProgram();
  glAttachShader(program, VertexShaderID);
  glAttachShader(program, FragmentShaderID);

  if (tess_control_path && tess_eval_file_path) {
    glAttachShader(program, TesselationControlShaderID);
    glAttachShader(program, TesselationEvalShaderID);
  }

  if (geometry_file_path)
    glAttachShader(program, GeometryShaderID);

  glLinkProgram(program);

  // Check the program
  glGetProgramiv(program, GL_LINK_STATUS, &Result);
  glGetProgramiv(program, GL_INFO_LOG_LENGTH, &InfoLogLength);
  if (InfoLogLength > 0) {
    std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
    glGetProgramInfoLog(program, InfoLogLength, NULL, &ProgramErrorMessage[0]);
    printf("%s\n", &ProgramErrorMessage[0]);
  }
  std::cout << "Linking program: "
            << (Result == GL_TRUE ? "Success" : "Failed!") << std::endl;

  glDeleteShader(VertexShaderID);
  glDeleteShader(FragmentShaderID);
  if (TesselationControlShaderID != 0 && TesselationEvalShaderID != 0) {
    glDeleteShader(TesselationControlShaderID);
    glDeleteShader(TesselationEvalShaderID);
  }
  if (GeometryShaderID != 0) {
    glDeleteShader(GeometryShaderID);
  }

  return true;
}

void UnloadShaders() { glDeleteProgram(programID); }

void LoadModel(string path, GLint mode) {

  glGenVertexArrays(1, &VertexArrayID);
  glBindVertexArray(VertexArrayID);

  if (path == "") {
    // Create mesh of n_points x n_points with normals up, and obvious uv
    // mapping.
    for (int i = 0; i < n_points; i++) {
      for (int j = 0; j < n_points; j++) {
        // Lets center the plane around the zero
        float x = (m_scale * i) - (m_scale * n_points) / 2.0f;
        float z = (m_scale * j) - (m_scale * n_points) / 2.0f;
        vertices.push_back(glm::vec3(x, 0, z));
        uvs.push_back(glm::vec2(float(i + 0.5f) / float(n_points - 1),
                                float(j + 0.5f) / float(n_points - 1)));
        normals.push_back(glm::vec3(0, 1, 0));
      }
    }
    if (mode == GL_TRIANGLES) {
      // now do a trianglestrip
      int n = 0;
      for (int i = 0; i < n_points; i++) {
        for (int j = 0; j < n_points; j++) {
          if (j != n_points - 1 && i != n_points - 1) {
            int topLeft = n;
            int topRight = topLeft + 1;
            int bottomLeft = topLeft + n_points;
            int bottomRight = bottomLeft + 1;
            indices.push_back(topLeft);
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);
            indices.push_back(bottomRight);
          }
          n++;
        }
      }
    } else if (mode == GL_PATCHES) {
      // Patches are Quads with 4 vertices
      int n = 0;
      for (int i = 0; i < n_points; i++) {
        for (int j = 0; j < n_points; j++) {
          if (j != n_points - 1 && i != n_points - 1) {
            // There are now 4 vertices per patch
            int topLeft = n;
            int topRight = topLeft + 1;
            int bottomLeft = topLeft + n_points;
            int bottomRight = bottomLeft + 1;
            // Spiral clockwise
            indices.push_back(topLeft);
            indices.push_back(topRight);
            indices.push_back(bottomRight);
            indices.push_back(bottomLeft);
          }
          n++;
        }
      }
      return;
    } else {
      std::cout << "Can't process that mode..." << endl;
      return;
    }
  } else {
    loadOBJ(path.c_str(), vertices, uvs, normals, indices);
  }

  // Load it into a VBO

  glEnableVertexAttribArray(0);
  glGenBuffers(1, &vertexbuffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3),
               &vertices[0], GL_STATIC_DRAW);
  glVertexAttribPointer(0,        // attribute
                        3,        // size
                        GL_FLOAT, // type
                        GL_FALSE, // normalized?
                        0,        // stride
                        (void *)0 // array buffer offset
  );

  glEnableVertexAttribArray(1);
  glGenBuffers(1, &uvbuffer);
  glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
  glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), &uvs[0],
               GL_STATIC_DRAW);
  glVertexAttribPointer(1,        // attribute
                        2,        // size
                        GL_FLOAT, // type
                        GL_FALSE, // normalized?
                        0,        // stride
                        (void *)0 // array buffer offset
  );

  glEnableVertexAttribArray(2);
  glGenBuffers(1, &normalbuffer);
  glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
  glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0],
               GL_STATIC_DRAW);
  glVertexAttribPointer(2,        // attribute
                        3,        // size
                        GL_FLOAT, // type
                        GL_FALSE, // normalized?
                        0,        // stride
                        (void *)0 // array buffer offset
  );

  // Generate a buffer for the indices as well
  glGenBuffers(1, &elementbuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
               &indices[0], GL_STATIC_DRAW);
}

void UnloadModel() {
  // Cleanup VBO and shader
  glDeleteBuffers(1, &vertexbuffer);
  glDeleteBuffers(1, &uvbuffer);
  glDeleteBuffers(1, &normalbuffer);
  glDeleteBuffers(1, &elementbuffer);
  glDeleteVertexArrays(1, &VertexArrayID);
}

void LoadTextures(const std::string &texAPath,
                  const std::string &texBPath,
                  const std::string &texCPath,
                  const std::string &heightMapPath) {
  // Load the texture
  int w, h;
  TextureA = loadBMP_custom(texAPath.c_str(), GL_LINEAR_MIPMAP_LINEAR,
                            GL_MIRRORED_REPEAT, w, h);
  TextureB = loadBMP_custom(texBPath.c_str(), GL_LINEAR_MIPMAP_LINEAR,
                            GL_MIRRORED_REPEAT, w, h);
  TextureC = loadBMP_custom(texCPath.c_str(), GL_LINEAR_MIPMAP_LINEAR,
                            GL_MIRRORED_REPEAT, w, h);
  HeightMapTexture = loadBMP_custom(heightMapPath.c_str(), GL_LINEAR,
                                    GL_MIRRORED_REPEAT, w, h);
  heightMapUVStepSize = glm::vec2(1.0f / float(w), 1.0f / float(h));
}

void UnloadTextures() {
  glDeleteTextures(1, &TextureA);
  glDeleteTextures(1, &TextureB);
  glDeleteTextures(1, &TextureC);
  glDeleteTextures(1, &HeightMapTexture);
}

int main(int argc, char *argv[]) {
  // Process CLI arguments
  CLIArgs args = processCLIArgs(argc, argv);

  // Initialize and create a window.
  if (initializeGLFW() != 0)
    return -1;

  // Gray background
  glClearColor(0.7f, 0.8f, 1.0f, 0.0f);
  // Enable depth test
  glEnable(GL_DEPTH_TEST);
  // Accept fragment if it closer to the camera than the former one
  glDepthFunc(GL_LESS);
  // Cull triangles which normal is not towards the camera
  glEnable(GL_CULL_FACE);

  LoadShaders(programID,
              "src/shaders/Simple.vert",
              "src/shaders/Simple.frag",
              "src/shaders/Simple.tesc",
              "src/shaders/Simple.tese");

  // Tesselation patches (quads)
  glPatchParameteri(GL_PATCH_VERTICES, 4);

  // Use our shader

  LoadTextures(args.textureA, args.textureB, args.textureC, args.heightMapPath);
  LoadModel(args.modelPath, GL_PATCHES);

  // Our light position is fixed
  glm::vec3 lightPos = glm::vec3(0, -0.5, -0.5);
  //	glm::vec3 lightPos = glm::vec3(0, 4, 4);

  bool reloadShaders = false;
  // For speed computation
  double lastTime = glfwGetTime();
  int nbFrames = 0;
  do {

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
      reloadShaders = true;
    }
    if (reloadShaders && glfwGetKey(window, GLFW_KEY_S) == GLFW_RELEASE) {
      UnloadShaders();
      LoadShaders(programID, "Simple.vert", "Phong.frag",
                  "Control.tesc", "Eval.tese");
      reloadShaders = false;
    }

    // Measure speed
    double currentTime = glfwGetTime();
    nbFrames++;
    if (currentTime - lastTime >=
        1.0) { // If last prinf() was more than 1sec ago
      // printf and reset
      printf("%f ms/frame\n", 1000.0 / double(nbFrames));
      nbFrames = 0;
      lastTime += 1.0;
    }

    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Compute the MVP matrix from keyboard and mouse input
    computeMatricesFromInputs();
    glm::mat4 ProjectionMatrix = getProjectionMatrix();
    glm::mat4 ViewMatrix = getViewMatrix();
    glm::mat4 ModelMatrix = glm::mat4(1.0);
    glm::mat4 ModelViewMatrix = ViewMatrix * ModelMatrix;
    glm::mat3 ModelView3x3Matrix = glm::mat3(ModelViewMatrix);
    glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

    // First pass: Base mesh
    glUseProgram(programID);

    GLuint HeightMapTexutreID = glGetUniformLocation(programID, "HeightMapTextureSampler");
    GLuint TextureAID = glGetUniformLocation(programID, "TextureASampler");
    GLuint TextureBID = glGetUniformLocation(programID, "TextureBSampler");
    GLuint TextureCID = glGetUniformLocation(programID, "TextureCSampler");

    // Set textures
    // Heightmap
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, HeightMapTexture);
    glUniform1i(HeightMapTexutreID, 0);

    // Diffuse textures
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, TextureA);
    glUniform1i(TextureAID, 1);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, TextureB);
    glUniform1i(TextureBID, 2);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, TextureC);
    glUniform1i(TextureCID, 3);

    // Get a handle for our uniforms
    GLuint HeightMapUVStepSizeID = glGetUniformLocation(programID, "HeightMapUVStepSize");
    GLuint HeightScaleID = glGetUniformLocation(programID, "HeightScale");
    GLuint BandAID = glGetUniformLocation(programID, "BandA");
    GLuint BandBID = glGetUniformLocation(programID, "BandB");
    GLuint BandSizesID = glGetUniformLocation(programID, "BandSizes");
    GLuint MatrixID = glGetUniformLocation(programID, "MVP");
    GLuint ViewMatrixID = glGetUniformLocation(programID, "V");
    GLuint ModelMatrixID = glGetUniformLocation(programID, "M");
    GLuint ModelView3x3MatrixID = glGetUniformLocation(programID, "MV3x3");
    GLuint LightID = glGetUniformLocation(programID, "LightPosition_worldspace");

    // Send our transformation to the currently bound shader,
    glUniform2f(HeightMapUVStepSizeID, heightMapUVStepSize.x, heightMapUVStepSize.y);
    glUniform1f(HeightScaleID, m_scale);
    glUniform1f(BandAID, m_band_a);
    glUniform1f(BandBID, m_band_b);
    glUniform1f(BandSizesID, m_band_sizes);
    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
    glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
    glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);
    glUniformMatrix3fv(ModelView3x3MatrixID, 1, GL_FALSE,
                       &ModelView3x3Matrix[0][0]);

    // Set the light position
    glUniform3f(LightID, lightPos.x, lightPos.y, lightPos.z);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    // Draw the triangles !
    glDrawElements(GL_PATCHES,              // mode
                   (GLsizei)indices.size(), // count
                   GL_UNSIGNED_INT,         // type
                   (void *)0                // element array buffer offset
    );

    // Swap buffers
    glfwSwapBuffers(window);
    glfwPollEvents();

  } // Check if the ESC key was pressed or the window was closed
  while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
         glfwWindowShouldClose(window) == 0);

  UnloadModel();
  UnloadTextures();
  UnloadShaders();

  // Close OpenGL window and terminate GLFW
  glfwTerminate();

  return 0;
}

#ifndef CONTROLS_HPP
#define CONTROLS_HPP

#include <glm/fwd.hpp>

void computeMatricesFromInputs();
glm::mat4 getViewMatrix();
glm::mat4 getProjectionMatrix();
glm::vec3 getCameraPosition();
#endif
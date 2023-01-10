project('oglShaders', 'cpp', default_options : ['cpp_std=c++17'])

glew = subproject('glew')
glew_dep = glew.get_variable('glew_dep')
glfw = subproject('glfw')
glfw_dep = glfw.get_variable('glfw_dep')
glm = subproject('glm')
glm_dep = glm.get_variable('glm_dep')

sources = [
  'src/main.cpp'
]

common = [
  'common/controls.cpp'
]

executable('oglShaders',
  sources + common,
  dependencies : [glew_dep, glfw_dep, glm_dep],
)
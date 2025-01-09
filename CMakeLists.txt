# ======================================================================== #
# Copyright 2018-2020 The Contributors                                     #
#                                                                          #
# Licensed under the Apache License, Version 2.0 (the "License");          #
# you may not use this file except in compliance with the License.         #
# You may obtain a copy of the License at                                  #
#                                                                          #
#     http://www.apache.org/licenses/LICENSE-2.0                           #
#                                                                          #
# Unless required by applicable law or agreed to in writing, software      #
# distributed under the License is distributed on an "AS IS" BASIS,        #
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. #
# See the License for the specific language governing permissions and      #
# limitations under the License.                                           #
# ======================================================================== #

cmake_policy(SET CMP0048 NEW)
project(ThRendOWL VERSION 0.1.0 LANGUAGES C CXX CUDA)

cmake_minimum_required(VERSION 3.12)
# iw, 1/3/20: yes, i know this is 'old' cmake style, but on CentOS7
# this is the only way that works, since the 'correct' way of setting
# the cxx11 standard doesnt propagate those flags to nvcc for the cuda
# programs. TODO: check to see if this can be fixed in the
# configure_cuda script instead.
if (NOT WIN32)
set (CMAKE_CXX_FLAGS "--std=c++11")
set (CUDA_PROPAGATE_HOST_FLAGS ON)
endif()

if (NOT CMAKE_CUDA_ARCHITECTURES)
  # 'all-major', since cmake-3.23
  set(CMAKE_CUDA_ARCHITECTURES all-major)
endif()

set(owl_dir ${CMAKE_CURRENT_SOURCE_DIR}/submodules/owl)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${owl_dir}/owl/cmake/")
add_subdirectory(${owl_dir})
include(embed_ptx)
# -------------------------------------------------------
include_directories(${OWL_INCLUDES})
include_directories(${OWL_VIEWER_INCLUDES})
include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/ThRend/glm")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/submodules/owl/owl/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/submodules/owl")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/submodules/owl/3rdParty/glfw/include")
include_directories("/usr/local/cuda/include/")
include_directories("/home/shen/myproject/NVIDIA-OptiX-SDK-7.4.0-linux64-x86_64/include")

add_definitions(${OWL_CXX_FLAGS})

embed_ptx(
  OUTPUT_TARGET
    deviceCode
  PTX_LINK_LIBRARIES
    owl::owl
  SOURCES
    ./ThRend/deviceCode.cu
)

add_executable(owlThRend
  ./ThRend/colormap.h
  ./ThRend/glm/simd/exponential.h
  ./ThRend/glm/simd/matrix.h
  ./ThRend/glm/simd/trigonometric.h
  ./ThRend/glm/simd/vector_relational.h
  ./ThRend/glm/simd/common.h
  ./ThRend/glm/simd/geometric.h
  ./ThRend/glm/simd/integer.h
  ./ThRend/glm/simd/platform.h
  ./ThRend/glm/simd/packing.h
  ./ThRend/reflections.h
  ./ThRend/UCDimporter.h
  ./ThRend/FreeImage.h
  ./ThRend/materialLoader.h
  ./ThRend/ONB.h
  ./ThRend/settingsLoader.h

  #./ThRend/glm/detail/glm.cpp
  ./ThRend/main.cpp
  ./ThRend/Model.cpp
  ./ThRend/OWLRenderer.cpp
  )
target_link_libraries(owlThRend
PRIVATE
  deviceCode
  owl::owl 
  owl_viewer
  )


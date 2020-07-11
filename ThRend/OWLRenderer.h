#pragma once

#include "Model.h"
#include "colormap.h"
#include "Material.h"

#include <owl/owl.h>
#include <owl/common/math/vec.h>

/*! device-side struct for the mesh of trianlges - owl builds this on the device */
struct TrianglesGeom {
  owl::vec3f *vertices;
  owl::vec3i *triangles;
};

/*! device-side struct for the mesh of trianlges - owl builds this on the device */
struct QuadsGeom {
  owl::vec3f *vertices;
  owl::vec4i *quads;
  owl::vec3i* triangles;
};

struct Camera {
#if __CUDA_ARCH__
  inline __device__ owl::Ray generateRay(const owl::vec2f &screen);
#endif
  owl::vec3f origin;
  owl::vec3f screen_00;
  owl::vec3f screen_du;
  owl::vec3f screen_dv;
};


struct DeviceGlobals {
  struct {
    owl::vec2i size;
    uint32_t  *pointer;
  } fb;
  owl::vec4f *accumBuffer;
  float* temperatureBuffer;
  float* skyTemperatureBuffer;

  int* tris_matIDsBuffer;
  int* quads_matIDsBuffer;
  float tmin;
  float tmax;
  int colormapsize;
  owl::vec3f* colormapBuffer;

  Material* matsBuffer;

  struct {
    int deviceIndex;
    int deviceCount;
  } multiGPU;
  
  Camera camera;
  OptixTraversableHandle world;

  /*! accumulatoin buffering ID - will be 0 in the first frame after
      camera motion stops, then 1 in the next frame, then 2, tec; once
      camera, scnee, frame size etc hcange this res-tarts at 0 */
  int accumID;
};

struct OWLRenderer {
  OWLRenderer(const Model &model, Colormap cm, Material* mats, float* tsky);

  void render();
  void resize(const owl::vec2i &fbSize,
              uint32_t *fbPointer);
  void setCamera(const owl::vec3f &lens_center,
                 const owl::vec3f &screen_00,
                 const owl::vec3f &screen_du,
                 const owl::vec3f &screen_dv);
private:
  
  OWLGeom createTrianglesGeom(const Model &model);
  OWLGeom createQuadsGeom(const Model &model);
  void createWorld(const Model &model);
  void createRayGen();
  void createMissProg();
  void createDeviceGlobals();

  int accumID { 0 };
  owl::vec2i fbSize;
  
  OWLContext  context { 0 };
  /*! the ptx module that contains all out device programs */
  OWLModule   module  { 0 };
  OWLParams   globals { 0 };
  OWLRayGen   rayGen  { 0 };
  OWLGroup    world   { 0 };
  OWLMissProg missProg { 0 };
  OWLBuffer   accumBuffer { 0 };
  OWLBuffer   temperatureBuffer{ 0 };
  OWLBuffer   skyTemperatureBuffer{ 0 };
  OWLBuffer   tris_matIDsBuffer{ 0 };
  OWLBuffer   quads_matIDsBuffer{ 0 };
  OWLBuffer   colormapBuffer{ 0 };
  OWLBuffer   matsBuffer{ 0 };

  float tmin;
  float tmax;
  int colormapsize;
};



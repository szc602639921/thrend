#include "OWLRenderer.h"
#include <owl/owl.h>

extern "C" char deviceCode_ptx[];

//OWLRenderer::OWLRenderer(const Model &model, const owl::vec3f* colormap, const int colormapsize_h, const float tmin_h, const float tmax_h)
OWLRenderer::OWLRenderer(const Model& model, Colormap cm, Material* mats, float* tsky)
{
  context = owlContextCreate(nullptr,1);
  module = owlModuleCreate(context,deviceCode_ptx);

  accumBuffer = owlDeviceBufferCreate(context,OWL_FLOAT4,1,nullptr);
  temperatureBuffer = owlDeviceBufferCreate(context, OWL_FLOAT, 1, nullptr);
  skyTemperatureBuffer = owlDeviceBufferCreate(context, OWL_FLOAT, 1, nullptr);
  tris_matIDsBuffer = owlDeviceBufferCreate(context, OWL_INT, 1, nullptr);
  quads_matIDsBuffer = owlDeviceBufferCreate(context, OWL_INT, 1, nullptr);
  colormapBuffer = owlDeviceBufferCreate(context, OWL_FLOAT3, 1, nullptr);
  matsBuffer = owlDeviceBufferCreate(context, OWL_USER_TYPE(mats[0]),64, mats);

  createRayGen();
  createMissProg();
  createDeviceGlobals();

  // allocate and load temperature buffer
  owlBufferResize(temperatureBuffer, model.vertices.size());
  owlBufferUpload(temperatureBuffer, (void*)&model.temps[0]);
  owlParamsSetBuffer(globals, "temperatureBuffer", temperatureBuffer);
  // allocate and load sky temperature buffer
  owlBufferResize(skyTemperatureBuffer, 10);
  owlBufferUpload(skyTemperatureBuffer, (void*)tsky);
  owlParamsSetBuffer(globals, "skyTemperatureBuffer", skyTemperatureBuffer);
  // allocate and load matIDs buffer
  owlBufferResize(tris_matIDsBuffer, model.tris_matIDs.size());
  owlBufferUpload(tris_matIDsBuffer, (void*)&model.tris_matIDs[0]);
  owlParamsSetBuffer(globals, "tris_matIDsBuffer", tris_matIDsBuffer);

  owlBufferResize(quads_matIDsBuffer, model.quads_matIDs.size());
  owlBufferUpload(quads_matIDsBuffer, (void*)&model.quads_matIDs[0]);
  owlParamsSetBuffer(globals, "quads_matIDsBuffer", quads_matIDsBuffer);
  // allocate and load colormap buffer
  owlBufferResize(colormapBuffer, cm.colormap.size());
  owlBufferUpload(colormapBuffer, (void*)&cm.colormap[0]);
  owlParamsSetBuffer(globals, "colormapBuffer", colormapBuffer);
  // set mats buffer in params
  owlParamsSetBuffer(globals, "matsBuffer", matsBuffer);


  owlParamsSet1i(globals, "colormapsize", cm.colormap.size());
  owlParamsSet1f(globals, "tmin", cm.tmin);
  owlParamsSet1f(globals, "tmax", cm.tmax);

  createWorld(model);
}

void OWLRenderer::createRayGen()
{
  rayGen = owlRayGenCreate(context,
                           module,"deviceMain",
                           /* no vars: */0,nullptr,0);
}

void OWLRenderer::createMissProg()
{
  missProg = owlMissProgCreate(context,
                               module,"miss",
                               /* no vars: */0,nullptr,0);
}

void OWLRenderer::createDeviceGlobals()
{
  OWLVarDecl vars[]
    = {
       { "fb.pointer", OWL_RAW_POINTER, OWL_OFFSETOF(DeviceGlobals,fb.pointer) },
       { "fb.size",    OWL_INT2, OWL_OFFSETOF(DeviceGlobals,fb.size) },
       { "accumBuffer",OWL_BUFPTR, OWL_OFFSETOF(DeviceGlobals,accumBuffer) },
       { "temperatureBuffer",OWL_BUFPTR, OWL_OFFSETOF(DeviceGlobals,temperatureBuffer) },
       { "skyTemperatureBuffer",OWL_BUFPTR, OWL_OFFSETOF(DeviceGlobals,skyTemperatureBuffer) },
       { "tris_matIDsBuffer",OWL_BUFPTR, OWL_OFFSETOF(DeviceGlobals,tris_matIDsBuffer) },
       { "quads_matIDsBuffer",OWL_BUFPTR, OWL_OFFSETOF(DeviceGlobals,quads_matIDsBuffer) },
       { "colormapBuffer",OWL_BUFPTR, OWL_OFFSETOF(DeviceGlobals,colormapBuffer) },
       { "matsBuffer",OWL_BUFPTR, OWL_OFFSETOF(DeviceGlobals,matsBuffer) },
       { "tmin",OWL_FLOAT, OWL_OFFSETOF(DeviceGlobals,tmin) },
       { "tmax",OWL_FLOAT, OWL_OFFSETOF(DeviceGlobals,tmax) },
       { "colormapsize",OWL_INT, OWL_OFFSETOF(DeviceGlobals,colormapsize) },
       { "accumID",    OWL_INT, OWL_OFFSETOF(DeviceGlobals,accumID) },
       { "world",      OWL_GROUP, OWL_OFFSETOF(DeviceGlobals,world) },
       { "camera.origin",    OWL_FLOAT3, OWL_OFFSETOF(DeviceGlobals,camera.origin) },
       { "camera.screen_00", OWL_FLOAT3, OWL_OFFSETOF(DeviceGlobals,camera.screen_00) },
       { "camera.screen_du", OWL_FLOAT3, OWL_OFFSETOF(DeviceGlobals,camera.screen_du) },
       { "camera.screen_dv", OWL_FLOAT3, OWL_OFFSETOF(DeviceGlobals,camera.screen_dv) },
       { "multiGPU.deviceCount", OWL_INT, OWL_OFFSETOF(DeviceGlobals,multiGPU.deviceCount) },
       { "multiGPU.deviceIndex", OWL_DEVICE, OWL_OFFSETOF(DeviceGlobals,multiGPU.deviceIndex) },
       { /*sentinel*/nullptr }
  };
  globals = owlParamsCreate(context,sizeof(DeviceGlobals),vars,-1);
  owlParamsSet1i(globals,"multiGPU.deviceCount",owlGetDeviceCount(context));

}

void OWLRenderer::createWorld(const Model &model)
{
  OWLGeom trianglesGeom = createTrianglesGeom(model);
  OWLGeom quadsGeom = createQuadsGeom(model);

  OWLGeom geoms[2] = { trianglesGeom, quadsGeom };
  OWLGroup meshGroup = owlTrianglesGeomGroupCreate(context,2,geoms);
  owlGroupBuildAccel(meshGroup);

  owlBuildPrograms(context);
  owlBuildPipeline(context);
  
  world = owlInstanceGroupCreate(context,1,
                                 &meshGroup);
  owlGroupBuildAccel(world);
  
  owlBuildSBT(context);
}



OWLGeom OWLRenderer::createTrianglesGeom(const Model &model)
{
  OWLVarDecl vars[]
    = {
       { "triangles", OWL_BUFPTR, OWL_OFFSETOF(TrianglesGeom,triangles) },
       { "vertices", OWL_BUFPTR, OWL_OFFSETOF(TrianglesGeom,vertices) },
       { /* sentinel */ nullptr }
  };
  OWLGeomType geomType
    = owlGeomTypeCreate(context,OWL_TRIANGLES,sizeof(TrianglesGeom),
                        vars,-1);
  owlGeomTypeSetClosestHit(geomType,0,module,"TrianglesCH");

  
  OWLGeom geom
    = owlGeomCreate(context,geomType);

  OWLBuffer vertices
    = owlDeviceBufferCreate(context,OWL_FLOAT3,model.vertices.size(),model.vertices.data());
  OWLBuffer triangles
    = owlDeviceBufferCreate(context,OWL_INT3,model.triangles.size(),model.triangles.data());
  
  owlTrianglesSetVertices(geom,vertices,model.vertices.size(),sizeof(owl::vec3f),0);
  owlTrianglesSetIndices(geom,triangles,model.triangles.size(),sizeof(owl::vec3i),0);
  
  owlGeomSetBuffer(geom,"vertices",vertices);
  owlGeomSetBuffer(geom,"triangles",triangles);
  
  return geom;
}

OWLGeom OWLRenderer::createQuadsGeom(const Model &model)
{
  PING;
  OWLVarDecl vars[]
    = {
       { "quads", OWL_BUFPTR, OWL_OFFSETOF(QuadsGeom,quads) },
       { "triangles", OWL_BUFPTR, OWL_OFFSETOF(QuadsGeom,triangles) },
       { "vertices", OWL_BUFPTR, OWL_OFFSETOF(QuadsGeom,vertices) },
       { /* sentinel */ nullptr }
  };
  OWLGeomType geomType
    = owlGeomTypeCreate(context,OWL_TRIANGLES,sizeof(QuadsGeom),
                        vars,-1);
  owlGeomTypeSetClosestHit(geomType,0,module,"QuadsCH");
  
  OWLGeom geom
    = owlGeomCreate(context,geomType);

  std::vector<owl::vec3i> splitQuads;
  for (int i=0;i<model.quads.size();i++) {
    owl::vec4i quad = model.quads[i];
    splitQuads.push_back({quad.x,quad.y,quad.z});
    splitQuads.push_back({quad.x,quad.z,quad.w});
  }
  
  OWLBuffer vertices
    = owlDeviceBufferCreate(context,OWL_FLOAT3,model.vertices.size(),model.vertices.data());
  OWLBuffer quads
    = owlDeviceBufferCreate(context,OWL_INT4,model.quads.size(),model.quads.data());
  OWLBuffer triangles
    = owlDeviceBufferCreate(context,OWL_INT3,splitQuads.size(),splitQuads.data());
  
  owlTrianglesSetVertices(geom,vertices,model.quads.size(),sizeof(owl::vec3f),0);
  owlTrianglesSetIndices(geom,triangles,splitQuads.size(),sizeof(owl::vec3i),0);
  
  owlGeomSetBuffer(geom,"vertices",vertices);
  owlGeomSetBuffer(geom,"quads",quads);
  owlGeomSetBuffer(geom, "triangles", triangles);

  return geom;
}

void OWLRenderer::setCamera(const owl::vec3f &lens_center,
                            const owl::vec3f &screen_00,
                            const owl::vec3f &screen_du,
                            const owl::vec3f &screen_dv)
{
  this->accumID = 0;
  owlParamsSet3f(globals,"camera.origin",   (const owl3f&)lens_center);
  owlParamsSet3f(globals,"camera.screen_00",(const owl3f&)screen_00);
  owlParamsSet3f(globals,"camera.screen_du",(const owl3f&)screen_du);
  owlParamsSet3f(globals,"camera.screen_dv",(const owl3f&)screen_dv);
}

void OWLRenderer::render()
{
  owlParamsSet1i(globals,"accumID",accumID);
  owlParamsSetGroup(globals,"world",world);
  owlLaunch2D(rayGen,fbSize.x,fbSize.y,globals);
  owlLaunchSync(globals);
  accumID++;
}


void OWLRenderer::resize(const owl::vec2i &fbSize,
                         uint32_t *fbPointer)
{
  this->fbSize = fbSize;
  this->accumID = 0;
  owlBufferResize(accumBuffer,fbSize.x*fbSize.y);
  owlParamsSetBuffer(globals,"accumBuffer",accumBuffer);
  owlParamsSet2i(globals,"fb.size",fbSize.x,fbSize.y);
  owlParamsSetPointer(globals,"fb.pointer",fbPointer);
}

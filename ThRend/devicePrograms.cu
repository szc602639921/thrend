#include "owl/owl.h"
#include "OWLRenderer.h"
#include "ONB.h"
#include <owl/common/math/random.h>

using namespace owl;
using namespace owl::common;

typedef owl::common::LCG<16> Random;


__constant__ DeviceGlobals optixLaunchParams;

inline __device__ Ray Camera::generateRay(const vec2f &screen)
{
  const vec3f dir
    = screen_00
    + screen.x * screen_du
    + screen.y * screen_dv;
  return Ray(origin,normalize(dir),1e-3f,1e10f);
}

/*! the per-ray data we use to communicate between closest hit program
    and raygen program */
struct IntersectionResult
{
  Random random;
  int   primID;
  vec3f hitPoint;
  /*! geometry normal */
  vec3f Ng;
  /*! temperature of intersection*/
  float T;
  /*! material of intersection */
  int matID;
};

OPTIX_CLOSEST_HIT_PROGRAM(TrianglesCH)()
{
  auto &isec = owl::getPRD<IntersectionResult>();
  auto &self = owl::getProgramData<TrianglesGeom>();

  isec.primID = optixGetPrimitiveIndex();
  vec3i index = self.triangles[isec.primID];
  const vec3f v0 = self.vertices[index.x];
  const vec3f v1 = self.vertices[index.y];
  const vec3f v2 = self.vertices[index.z];
  isec.Ng = normalize(cross(v1-v0,v2-v0));
  // get triangle barycentrics
  const float u = optixGetTriangleBarycentrics().x;
  const float v = optixGetTriangleBarycentrics().y;
  // get hit point
  isec.hitPoint = (1.f - u - v) * v0
      + u * v1
      + v * v2;
  isec.hitPoint = isec.hitPoint + 1e-3f * isec.Ng;
  // get point temperature
  auto& lp = optixLaunchParams;
  isec.T = (1.f - u - v) * lp.temperatureBuffer[index.x]
      + u * lp.temperatureBuffer[index.y]
      + v * lp.temperatureBuffer[index.z];  
  // get point material
  isec.matID = lp.tris_matIDsBuffer[isec.primID];
}

OPTIX_CLOSEST_HIT_PROGRAM(QuadsCH)()
{
  auto &isec = owl::getPRD<IntersectionResult>();
  auto &self = owl::getProgramData<QuadsGeom>();

  int quadID = optixGetPrimitiveIndex() ;
  isec.primID = quadID;
  vec3i index = self.triangles[isec.primID];
  const vec3f v0 = self.vertices[index.x];
  const vec3f v1 = self.vertices[index.y];
  const vec3f v2 = self.vertices[index.z];
  isec.Ng = normalize(cross(v1-v0,v2-v0));
  // get triangle barycentrics
  const float u = optixGetTriangleBarycentrics().x;
  const float v = optixGetTriangleBarycentrics().y;
  // get hit point
  isec.hitPoint = (1.f - u - v) * v0
      + u * v1
      + v * v2;
  isec.hitPoint = isec.hitPoint + 1e-3f * isec.Ng;
  // get point temperature
  auto& lp = optixLaunchParams;
  isec.T = (1.f - u - v) * lp.temperatureBuffer[index.x]
      + u * lp.temperatureBuffer[index.y]
      + v * lp.temperatureBuffer[index.z];
  // get point material
  isec.matID = lp.quads_matIDsBuffer[isec.primID/2];
}

OPTIX_MISS_PROGRAM(miss)()
{
}

inline __device__
vec3f missColor()
{
  const vec2i pixelID = owl::getLaunchIndex();
  const float t = pixelID.y / (float)optixGetLaunchDimensions().y;
  const vec3f c = (1.0f - t)*vec3f(1.0f, 1.0f, 1.0f) + t * vec3f(0.5f, 0.7f, 1.0f);
  return c;
}

/*inline __device__
void writeAccumulate(const vec3f &pixelColor)
{
  auto &lp = optixLaunchParams;
  const vec2i pixel  = owl::getLaunchIndex();
  const int fbIndex  = pixel.x + lp.fb.size.x*pixel.y;
  float4 *accumBufferValue = ((float4 *)lp.accumBuffer) + fbIndex;
  const int accumID = lp.accumID;
  vec4f pixel4f = vec4f(pixelColor,1.f);
  if (accumID > 0)
    pixel4f += vec4f(*accumBufferValue);
  
  *accumBufferValue = pixel4f;
  lp.fb.pointer[fbIndex] = make_rgba(pixel4f / pixel4f.w);
}*/

/*! get the colormap position from a given temperature */
inline __device__
int getColormapIndex(float T, float tmin, float tmax, int colormapsize) {
    float tt = (T - 273.15);
    if (tt < tmin)
        tt = tmin;
    else if (tt > tmax)
        tt = tmax;

    int ind = floor(((tt - tmin) / (tmax - tmin)) * (colormapsize - 1));
    return ind;
}

inline __device__
void writeAccumulate(const float flux, const float weight)
{
    auto& lp = optixLaunchParams;
    const vec2i pixel = owl::getLaunchIndex();
    const int fbIndex = pixel.x + lp.fb.size.x * pixel.y;
    float4* accumBufferValue = ((float4*)lp.accumBuffer) + fbIndex;
    const int accumID = lp.accumID;
    vec3f colorAux;
    // flux equal -1 means miss
    if (flux != -1) {
        vec4f pixel4f = vec4f(flux * weight, weight, 0.0f, 1.f);
        if (accumID > 0)
            pixel4f += vec4f(*accumBufferValue);

        *accumBufferValue = pixel4f;

        float accumFlux = pixel4f.x / pixel4f.y;
        float apparentTemperature = pow(accumFlux, 0.25f);
        int ind = getColormapIndex(apparentTemperature, lp.tmin, lp.tmax, lp.colormapsize);
        colorAux = lp.colormapBuffer[ind];
    }
    else {
        colorAux = missColor();
    }
    vec4f color = vec4f(colorAux, 1.0f);
    lp.fb.pointer[fbIndex] = make_rgba(color);
}


// Implemented by jpaguerre based on eduardof MATLAB version of:
// [Walter et al] "Microfacet Models for Refraction through Rough Surfaces"
// We use GGX sampling and add weight with G functions
inline __device__
float G1vmGGX(vec3f wo, vec3f nn, vec3f mm, float thetaV, float alphaG) {
    //eq. 34
    float Xplus = (dot(wo, mm) / dot(wo, nn)) > 0 ? 1.0 : 0.0;
    float G1om = Xplus * (2.0f / (1.0f + owl::common::sqrt(1.0f + alphaG * alphaG * tan(thetaV) * tan(thetaV))));
    return G1om;
}

/*! calculate microfacet random direction */
inline __device__
vec3f specLobeMicrofacetGGX(
    ONB onb,
    vec3f wi_world,
    float e1,
    float e2,
    float alphaG,
    float& weight)
{
    //eqs. 35, 36
    vec3f nn(0., 0., 1.);
    float thetam = atan(alphaG * owl::common::sqrt(e1) / owl::common::sqrt(1 - e1));
    float psim = 2.0f * M_PI * e2;

    float x = sin(thetam) * cos(psim);
    float y = sin(thetam) * sin(psim);
    float z = cos(thetam);
    //local microfacet normal
    vec3f mm(x, y, z); mm = normalize(mm);
    //world wi to local wi
    vec3f wi = onb.WorldToLocal(wi_world); wi = normalize(wi);
   
    //specular direction
    vec3f wo = 2.0f * (dot(mm, wi)) * mm - wi; wo = normalize(wo);
    if (wo.z < 0)
        return vec3f(0, 0, 0);
    else {
        //eqs. 23,41 
        float G1om = G1vmGGX(wo, nn, mm, acos(dot(wo, nn)), alphaG);
        float G1im = G1vmGGX(wi, nn, mm, acos(dot(wi, nn)), alphaG);
        float Giom = G1im * G1om;
        weight = abs(dot(wi, mm)) * Giom / (abs(dot(wi, nn)) * abs(dot(mm, nn)));

        //local to world coordinates:
        vec3f rayDir = onb.LocalToWorld(wo);
        return rayDir;
    }
}

inline __device__
float pow4(float num) {
    return num * num * num * num;
}

OPTIX_RAYGEN_PROGRAM(deviceMain)()
{  
    auto &lp = optixLaunchParams;
    Material* mats =lp.matsBuffer;
    float* tsky = lp.skyTemperatureBuffer;
    vec2i pixel = owl::getLaunchIndex();
    if ((pixel.y % lp.multiGPU.deviceCount) != lp.multiGPU.deviceIndex) return;
  
    vec2f screenCoords = (vec2f(pixel)+vec2f(.5f)) / vec2f(owl::getLaunchDims());
  
    Ray ray = lp.camera.generateRay(screenCoords);
  
    IntersectionResult isec;
    //seed the RNG
    isec.random.init(pixel.x + lp.accumID * owl::getLaunchDims().x,
        pixel.y + lp.accumID * owl::getLaunchDims().y);

    isec.primID = -1;
    // trac primary ray
    owl::traceRay(lp.world,ray,isec);

    vec3f color;

    if (isec.primID < 0)
        writeAccumulate(-1.0f, -1.0f);
    else {
        float emittedTemperature = isec.T;
        float reflectedTemperature = isec.T;
        //get intersection normal and ray direction
        vec3f norm = isec.Ng;
        vec3f wi_world = -ray.direction;
        //get intersection material properties
        Material* m = mats + isec.matID;
        float* emiT = m->emisTable;
        float alphaG = m->roughness;
        //create orthonormal basis from norm
        ONB onb(norm);
        //get local wi
        vec3f wi_local = onb.WorldToLocal(-ray.direction);
        //get intersection emissivity
        float local_angle = (asin(wi_local.z) * 180 / M_PI);
        float emissivity ;
        if (local_angle < 0){
            //error
            emissivity = 1.0f;
            //printf("error %f\n", local_angle);
        }
        else {
            int ang1 = floor(local_angle);
            int ang2 = ceil(local_angle);
            float coef = local_angle - ang1;
            emissivity = (1 - coef) * emiT[ang1] + coef * emiT[ang2];
            if(emissivity<0.0f)
                printf("emi %f ang1 %d ang2 %d\n", emissivity,ang1,ang2);
        }
        float e1 = isec.random();
        float e2 = isec.random();
        float weight;
        vec3f reflDir = specLobeMicrofacetGGX(
            onb,
            wi_world,
            e1,
            e2,
            alphaG,
            weight);
        if (length(reflDir) > 0) {
            // trace microfacet reflection
            isec.primID = -1;
            owl::traceRay(lp.world, Ray(isec.hitPoint, normalize(reflDir), 1e-3f, 1e10f), isec);
            if (isec.primID < 0) {
                //calculate sky temperature
                float angle = asin(reflDir.z) * 180 / M_PI;
                if (angle > 0 && angle < 90) {
                    int angleL = floor(angle / 10);
                    int angleU = ceil(angle / 10);
                    float rest = angle / 10 - angleL;
                    reflectedTemperature = (1 - rest) * tsky[9 - angleL] + rest * tsky[9 - angleU];
                }
                else
                    reflectedTemperature = 273.15;
            }
            else {
                //reflected ray is autohit
                reflectedTemperature = isec.T;
            }
        }
        //printf("reflDir %f\n", reflDir.x);
        float radiativeFlux = emissivity * pow4(emittedTemperature) +
            (1.0f - emissivity) * pow4(reflectedTemperature);
        writeAccumulate(radiativeFlux, weight);
    }


}



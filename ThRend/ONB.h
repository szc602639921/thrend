//ORIGINAL FILE WRITTEN BY IGNACIO DECIA
//MODIFICATIONS FROM GLM TO OWL by jpaguerre

#ifndef OrtoNormalBase
#define OrtoNormalBase

#include "owl/common/math/vec.h"
#include "owl/common/math/box.h"
#include <cuda.h>
#include <optix.h>

struct ONB {

public:
#if __CUDA_ARCH__
	inline __device__ ONB(owl::vec3f normal);
	inline __device__ owl::vec3f WorldToLocal(const owl::vec3f& v);
	inline __device__ owl::vec3f LocalToWorld(const owl::vec3f& v);
#endif
private:
	owl::vec3f n;
	owl::vec3f s;
	owl::vec3f t;
};

#if __CUDA_ARCH__
inline ONB::ONB(owl::vec3f normal)
{
	n = normal;
	if (fabs(n.x) > fabs(n.z)) {
		s.x = -n.y;
		s.y = n.x;
		s.z = 0;
	}
	else {

		s.x = 0;
		s.y = -n.z;
		s.z = n.y;
	}

	s = normalize(s);
	t = cross(n, s);
}

inline __device__
owl::vec3f ONB::WorldToLocal(const owl::vec3f &v) {
	return owl::vec3f(dot(v, s), dot(v, t), dot(v, n));
}

inline __device__
owl::vec3f ONB::LocalToWorld(const owl::vec3f &v) {
	return  (v.x * s) + (v.y * t) + (v.z * n);
}
#endif

#endif
#pragma once

#define _USE_MATH_DEFINES
#define GLM_ENABLE_EXPERIMENTAL

#include <string>
#include <vector>
#include <glm.hpp>
#include <fstream>
#include <sstream>
#include <iostream>

struct Colormap{
public:
	std::vector<glm::vec3> colormap;
	//min and max temperatures of the colormap
	float tmin;
	float tmax;
	//min and max temperatures of the reflection colormap
	float tmin_reflected;
	float tmax_reflected;

	int getColor(float t) {
		float tt = (t - 273.15);

		if (tt < tmin)
			tt = tmin;
		else if (tt > tmax)
			tt = tmax;

		int ind = floor(((tt - tmin) / (tmax - tmin)) * (colormap.size() - 1));
		return ind;
	}

	//get temp with other values (for reflections)
	int getColor2(float t) {
		float tt = (t - 273.15);

		if (tt < tmin_reflected)
			tt = tmin_reflected;
		else if (tt > tmax_reflected)
			tt = tmax_reflected;

		int ind = floor(((tt - tmin_reflected) / (tmax_reflected - tmin_reflected)) * (colormap.size() - 1));
		return ind;
	}

};



//get temp with tmin and tmax



#ifndef MATERIALimporter
#define MATERIALimporter

#include "Material.h"

#include <string>
#include <vector>
#include <glm.hpp>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>

// ids is just an aux vec for degub reasons.
std::vector<int> ids;

double deg2rad(float deg) {
	return deg * M_PI / 180.0;
}

void computeEmissivityCurve(Material *m){
	float* emisTable = (float*)malloc(sizeof(float) * 91);
	float e = m->normal_emissivity;
	float df = m->diffuse_fraction;
	float r = 1.0 - e;
	for (int i = 0; i < 91; i++){
		float teta = deg2rad(90 - i);
		float schlick = 1 - (r + (1 - r)*(pow(1 - cos(teta), 5.0)));
		m->emisTable[i] = (df*e) + (1 - df)*schlick;
	}
}


Material* loadMaterials(std::string filename){
	Material* matProps = (Material*)malloc(sizeof(Material) * 64);
	Material m; m.UCD_id = -1; m.custom = 0;
	std::ifstream file(filename.c_str());

	std::cout << "Loading file " << filename << " \n";
	std::string line;
	while (std::getline(file, line)){
		std::stringstream   linestream(line);
		std::string id;
		linestream >> id;
		if (id.size() < 0 || (id.size() > 0 && id.at(0) == '#')){
			
		}
		else if (id == "name"){
			//allocate new material
			if (m.UCD_id != -1){
				ids.push_back(m.UCD_id);
				if (!m.custom)
					computeEmissivityCurve(&m);
				memcpy(matProps + (m.UCD_id), &m, sizeof(Material));
				m.custom = 0;
			}
			std::string x;
			linestream >> x;
			//m.name = x;
		}
		else if (id == "UCD_id"){
			int x;
			linestream >> x;
			m.UCD_id = x;
		}
		else if (id == "normal_emissivity"){
			float x;
			linestream >> x;
			m.normal_emissivity = x;
		}
		else if (id == "diffuse_fraction"){
			float x;
			linestream >> x;
			m.diffuse_fraction = x;
		}
		else if (id == "roughness"){
			float x;
			linestream >> x;
			m.roughness = x;
		}
		else if (id == "emissivity_curve"){
			float x;
			m.custom = 1;
			//m.emisTable = (float*)malloc(sizeof(float) * 91);
			for (int i = 0; i < 91; i++){
				linestream >> x;
				m.emisTable[i] = x;
			}
		}
	}
	//copy last one
	if (m.UCD_id != -1){
		ids.push_back(m.UCD_id);
		if (!m.custom)
			computeEmissivityCurve(&m);
		memcpy(matProps + (m.UCD_id), &m, sizeof(Material));
		m.custom = 0;
	}
	std::cout << "Materials loaded succesfully \n";
	return matProps;
}

void printMaterials(Material* matProps){
	for (int i = 0; i < ids.size(); i++){
		Material m = matProps[ids[i]];
		//std::cout << "name " << m.name << "\n";
		std::cout << "UCD_id " << m.UCD_id << "\n";
		if (!m.custom){
			std::cout << "normal_emissivity " << m.normal_emissivity << "\n";
			std::cout << "diffuse_fraction " << m.diffuse_fraction << "\n";
		}
		std::cout << "roughness " << m.roughness << "\n";

		for (int j = 0; j < 91; j++)
			std::cout << m.emisTable[j] << " ";
		std::cout << "\n";
	}
}


#endif


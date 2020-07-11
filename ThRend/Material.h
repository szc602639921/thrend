#pragma once
  
typedef struct {
	std::string name;
	int UCD_id;
	float normal_emissivity;
	float diffuse_fraction;
	float roughness;
	float emisTable[91];
	bool custom;
} Material;
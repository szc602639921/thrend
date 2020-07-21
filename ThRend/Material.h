#pragma once
  
typedef struct {
	int UCD_id;
	float normal_emissivity;
	float diffuse_fraction;
	float roughness;
	float emisTable[91];
	int custom;
} Material;
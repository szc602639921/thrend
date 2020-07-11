// ours
#include "Model.h"
// std
#include <fstream>
#include <iostream>
#include <sstream>

using namespace std;

void loadUCD(Model &model,
             const std::string &fileName)
// int load_UCD(std::string filename, 
//              std::vector<vec3> &sc_vertices, 
//              std::vector<int> &sc_triangles, 
//              std::vector<int> &sc_quads, 
//              std::vector<int> &matIDs,
//              std::vector<float> &temps){
{
  auto &sc_vertices  = model.vertices;
  auto &sc_triangles = model.triangles;
  auto &sc_quads     = model.quads;
  auto &tris_matIDs  = model.tris_matIDs;
  auto &quads_matIDs = model.quads_matIDs;
  auto &temps        = model.temps;
  
  ifstream file(fileName);
  cout << "Loading file " << fileName << " \n";

  if (!file)
    throw std::runtime_error("could not open "+fileName);

  string line;
  getline(file, line);

  int nPoints; int nFaces;
  stringstream   linestream(line);
  linestream >> nPoints >> nFaces;
  cout << "Total points: " << nPoints << " \n";
  cout << "Total faces: " <<  nFaces << " \n";
  //load vertices
  for (int i = 0; i < nPoints; i++){
    getline(file, line);
    int id; float x, y, z;
    stringstream   linestream(line);
    linestream >> id >> x >> y >> z;
    sc_vertices.push_back(glm::vec3(x, y, z));
  }
  //load faces
  for (int i = 0; i < nFaces; i++){
    getline(file, line);
    int id,mat; 
    string poly;
    int idx1, idx2, idx3, idx4;
    stringstream   linestream(line);
    linestream >> id >> mat >> poly;

    if (poly == "tri"){
      tris_matIDs.push_back(mat);
      linestream >> idx1 >> idx2 >> idx3;
      sc_triangles.push_back({idx1-1,
                              idx2-1,
                              idx3-1});
    }
    else if (poly == "quad"){
      quads_matIDs.push_back(mat);
      linestream >> idx1 >> idx2 >> idx3 >> idx4;
      sc_quads.push_back({idx1-1,
                          idx2-1,
                          idx3-1,
                          idx4-1});
    }
  }
  cout << "Geometry loaded successfully \n";
  //discard two lines
  getline(file, line);
  getline(file, line);
  //load temperatures
  for (int i = 0; i < nPoints; i++){
    getline(file, line);
    int id; float t;
    stringstream   linestream(line);
    linestream >> id >> t;
    temps.push_back(t);
  }
  cout << "Nodal temperatures loaded successfully \n";
}


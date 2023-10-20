#include "raytracer.h"
#include "ray.hpp"
#include "json2scene.h"

#include <iostream>
#include <fstream>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>


const char *PATH = "scenes/";

double fov = 60;
colour3 background_colour(0, 0, 0);

json jscene;
Scene scene;

//------------------------------------------------------------------------
// MAIN FUNCTIONS
//------------------------------------------------------------------------
void choose_scene(char const *fn) {
	if (fn == NULL) {
		std::cout << "Using default input file " << PATH << "c.json\n";
		fn = "c";
	}

	std::cout << "Loading scene " << fn << std::endl;
	
	std::string fname = PATH + std::string(fn) + ".json";
	std::fstream in(fname);
	if (!in.is_open()) {
		std::cout << "Unable to open scene file " << fname << std::endl;
		exit(EXIT_FAILURE);
	}
	
	in >> jscene;
  
  if (json_to_scene(jscene, scene) < 0) {
		std::cout << "Error in scene file " << fname << std::endl;
		exit(EXIT_FAILURE);
  }
  
  fov = scene.camera.field;
  background_colour = scene.camera.background;
}

bool trace(const point3 &e, const point3 &s, colour3 &colour, bool pick) {
    Ray *ray = new Ray(scene, e, s, 1, GLOBAL_MAX_T);
    bool hit = ray->fire_ray(true);
    if(hit) { 
        colour = ray->calc_lighting();
    }

    if(pick) cout << "t=" << ray->get_closest_t() <<endl;
    return hit;
}

//------------------------------------------------------------------------
// UTILITY FUNCTIONS
//------------------------------------------------------------------------
json find(json &j, const std::string key, const std::string value) {
	json::iterator it;
	for (it = j.begin(); it != j.end(); ++it) {
		if (it->find(key) != it->end()) {
			if ((*it)[key] == value) {
				return *it;
			}
		}
	}
	return json();
}

glm::vec3 vector_to_vec3(const std::vector<float> &v) {
	return glm::vec3(v[0], v[1], v[2]);
}
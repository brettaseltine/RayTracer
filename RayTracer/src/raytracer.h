#include <glm/glm.hpp>
#include "schema.h"

typedef glm::vec3 point3;
typedef glm::vec3 colour3;
using namespace std;

extern double fov;
extern colour3 background_colour;

// LOADS IN SCENE
void choose_scene(char const *fn);

// RETURN TRUE IF HIT, OTHERWISE FALSE
bool trace(const point3 &e, const point3 &s, colour3 &colour, bool pick);

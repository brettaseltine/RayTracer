#include <glm/glm.hpp>
#include "schema.h"
#include "raytracer.h"

typedef glm::vec3 point3;
typedef glm::vec3 colour3;
using namespace std;

class Ray {

  private:
    Scene scene;
    point3 e;
    point3 s;
    float min_t;
    float max_t;
    int recursive_depth;
    bool in_object;

    float closest_t;
    Object *closest_obj;
    glm::vec3 closest_obj_normal;

  public:
    Ray(Scene scene, point3 e, point3 s, float min_t, float max_t, int recursive_depth, bool in_object);
    Ray(Scene scene, point3 e, point3 s, float min_t, float max_t, int recursive_depth) : Ray(scene,e,s,min_t,max_t,recursive_depth,false){};
    Ray(Scene scene, point3 e, point3 s, float min_t, float max_t) : Ray(scene,e,s,min_t,max_t,0){};

    float get_closest_t(void);
    point3 get_e(void);
    point3 calc_intersect_pos(void);

    bool fire_ray(bool check_all_objects);
    void collision_check_sphere(Sphere *sphere);
    void collision_check_plane(Plane *plane);
    void colision_check_mesh(Mesh *mesh);
    
    colour3 calc_lighting(void);
    colour3 calc_ambient_diffuse_spec(point3 intersect_pos, glm::vec3 V, colour3 colour, Material material);
    colour3 do_reflection(point3 intersect_pos, glm::vec3 V,  colour3 colour, Material material);
    colour3 do_transmission(point3 intersect_pos, colour3 colour, Material material);
    colour3 do_refraction(point3 intersect_pos, glm::vec3 V, colour3 colour, Material material);

    bool is_reflective(Material m);
    bool is_transmissive(Material m);
    bool is_refractive(Material m);
    
};

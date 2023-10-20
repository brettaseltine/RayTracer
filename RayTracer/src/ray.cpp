// TODO: 
// REFRACTION NEEDS FIXING
// CONSTRUCT SCENES a AND b


#include "ray.hpp"
#include <glm/ext.hpp>
#include <cmath>
#include <iostream>
using namespace std;

Ray::Ray(Scene scene, point3 e, point3 s, float min_t, float max_t, int recursive_depth, bool in_object) {
    this->scene = scene;
    this->e = e;
    this->s = s;
    this->min_t = min_t;
    this->max_t = max_t;
    this->recursive_depth = recursive_depth;
    this->in_object = in_object;

    this->closest_t = max_t+1;
    this->closest_obj = nullptr;
}

float Ray::get_closest_t(void) {return closest_t;}
point3 Ray::get_e(void) {return e;}
point3 Ray::calc_intersect_pos(void) {return e + closest_t*(s-e);}

bool Ray::fire_ray(bool check_all_objects) {
    if(recursive_depth > MAX_RAY_RECURSION) return false;

    // traverse the objects
    for (auto &&object : scene.objects) {
        if(!check_all_objects && closest_obj != nullptr) break;

        if (object->type == "sphere") {
            collision_check_sphere((Sphere *)object);
        } else if(object->type == "plane") {
            collision_check_plane((Plane *)object);
        } else if(object->type == "mesh") {
            colision_check_mesh((Mesh*)object);
        }
    }

    return (closest_obj != nullptr) ? true : false;
}

void Ray::collision_check_sphere(Sphere *sphere) {
    point3 &c = sphere->position;   // center
    float radius = sphere->radius;
    point3 d = s - e;
    float dec = glm::dot(d, e-c);
    float disc = dec * dec - glm::dot(d,d) * (glm::dot(e-c,e-c) - radius * radius);

    // INTERSECTION CHECK
    if (disc > 0) {
        float t1 = (-dec - sqrt(disc)) / glm::dot(d,d);
        float t2 = (-dec + sqrt(disc)) / glm::dot(d,d);

        float t;
        t = min(t1, t2);
        if(t1 < min_t) t = t2;
        else if(t2 < min_t) t = t1;
        
        if(t > min_t && t < max_t && t < closest_t) {
            closest_t = t;
            closest_obj = sphere;
            glm::vec3 pos = e + t*d;
            closest_obj_normal = glm::normalize(pos - c);
        }
    }
}

void Ray::collision_check_plane(Plane *plane) {
    glm::vec3 a = plane->position;
    glm::vec3 n = plane->normal;
    glm::vec3 d = s - e;

    // calculate t
    float t = (glm::dot(n,d) != 0) ? glm::dot(n, (a-e)) / glm::dot(n,d) : max_t+1;

    // set t if applicable
    if(t > min_t && t < max_t && t < closest_t){
        closest_t = t;
        closest_obj = plane;
        closest_obj_normal = glm::normalize(n);
    }
}

void Ray::colision_check_mesh(Mesh *mesh) {
    vector<Triangle> triangles = mesh->triangles;

    for(int i = 0; i < triangles.size(); i++) {
        Triangle tri = triangles.at(i);

        Vertex a = tri.vertices[0];
        Vertex b = tri.vertices[1];
        Vertex c = tri.vertices[2];
        glm::vec3 n = glm::cross(b-a,c-b);
        glm::vec3 d = s-e;

        float t = (glm::dot(n,d) != 0) ? glm::dot(n, (a-e)) / glm::dot(n,d) : max_t+1;
        
        if(t > min_t && t < max_t && t < closest_t) {
            // if here, then triangle not planar
            Vertex pos = e + t*d;
            float term1 = glm::dot( glm::cross(b-a, pos-a), n);
            float term2 = glm::dot( glm::cross(c-b, pos-b), n);
            float term3 = glm::dot( glm::cross(a-c, pos-c), n);

            if(term1 > 0 && term2 > 0 && term3 > 0) {
                // if here, then point in bounds
                closest_t = t;
                closest_obj = mesh;
                closest_obj_normal = glm::normalize(n);
            }
        }
    }
}

colour3 Ray::calc_lighting() {
    Material &material = closest_obj->material;
    colour3 colour = colour3(0,0,0);
    Vertex intersect_pos = e + closest_t*(s-e);
    glm::vec3 V = glm::normalize(e - intersect_pos);

    colour = calc_ambient_diffuse_spec(intersect_pos, V, colour, material);
    if(is_reflective(material)) colour = do_reflection(intersect_pos, V, colour, material);
    if(is_transmissive(material) && !is_refractive(material)) colour = do_transmission(intersect_pos, colour, material);
    if(is_refractive(material)) colour = do_refraction(intersect_pos, V, colour, material);
    
    return colour;
}

colour3 Ray::calc_ambient_diffuse_spec(point3 intersect_pos, glm::vec3 V, colour3 colour, Material material) {
    for(auto &&light : scene.lights) {

        RGB ambient_prod = material.ambient;
        RGB diffuse_prod = material.diffuse * light->color;
        RGB specular_prod = material.specular *light->color;

        if(light->type == "ambient"){
            colour += ambient_prod;
        } 
        else {
            // calc light params depending on type
            glm::vec3 L;
            Vertex light_pos;

            if (light->type == "directional") {
                DirectionalLight *dir = (DirectionalLight *)light;
                L = glm::normalize(-dir->direction);
                light_pos = intersect_pos + L*10;
            } 
            else if(light->type == "point") {
                PointLight *pl = (PointLight *)light;
                L = glm::normalize(pl->position - intersect_pos);
                light_pos = pl->position;
            }
            else if(light->type == "spot") {
                SpotLight *sl = (SpotLight *)light;
                L = glm::normalize(sl->position - intersect_pos);
                light_pos = sl->position;

                if(glm::angle(L, glm::normalize(-sl->direction)) > glm::radians(sl->cutoff) ) continue;
            }

            bool in_shadow = (new Ray(scene, intersect_pos, light_pos, EPSILON, 1))->fire_ray(false);
            if(!in_shadow) {

                // set diffuse & specular
                colour += glm::clamp(diffuse_prod * glm::dot(closest_obj_normal, L), 0.0f, 1.0f);
                glm::vec3 R_light = glm::normalize(  2*glm::dot(closest_obj_normal, L)*closest_obj_normal - L   );
                colour += glm::clamp(specular_prod * glm::pow(glm::dot(R_light, V), material.shininess), 0.0f,1.0f);
            }
        }
    }
    return colour;
}

bool Ray::is_reflective(Material m) {
    return m.reflective != point3(0,0,0);
}
bool Ray::is_transmissive(Material m) {
    return m.transmissive != point3(0,0,0);
}
bool Ray::is_refractive(Material m) {
    return m.refraction != 0;
}

colour3 Ray::do_reflection(point3 intersect_pos, glm::vec3 V, colour3 colour, Material material) {
    point3 R_view = 2*glm::dot(closest_obj_normal, V)*closest_obj_normal - V; // reflection of view_vec through object normal
    Ray *reflect_ray = new Ray(scene, intersect_pos, intersect_pos + R_view, EPSILON, GLOBAL_MAX_T, recursive_depth+1);
    bool hit = reflect_ray->fire_ray(true);
    if(hit) return (1-material.reflective.x)*colour + material.reflective.x*reflect_ray->calc_lighting();
    else return colour;
}

colour3 Ray::do_transmission(point3 intersect_pos, colour3 colour, Material material) {
    float transp_coeff = material.transmissive.x;
    Ray *through_ray = new Ray(scene, intersect_pos, intersect_pos + glm::normalize(intersect_pos), EPSILON, GLOBAL_MAX_T, recursive_depth+1);
    bool hit = through_ray->fire_ray(true);
    if(hit) return (1-transp_coeff)*colour + transp_coeff*through_ray->calc_lighting();
    else return (1-transp_coeff)*colour + transp_coeff*background_colour;
}

colour3 Ray::do_refraction(point3 intersect_pos, glm::vec3 V, colour3 colour, Material material) {
    float transp_coeff = material.transmissive.x;

    point3 dir_vec = -V;
    point3 N = (in_object) ? -closest_obj_normal : closest_obj_normal;
    float ni = (in_object) ? material.refraction : 1;
    float nr = (in_object) ? 1 : material.refraction;
    
    float under_sqrt = 1 - ( (ni*ni) * (1-(pow(glm::dot(dir_vec,N), 2))) / (nr*nr) );
    if(under_sqrt > 0) {
        point3 v_refracted = (ni/nr)*(dir_vec - N*glm::dot(dir_vec,N)) - N*sqrt(under_sqrt);

        Ray *refract_ray = new Ray(scene, intersect_pos, intersect_pos + glm::normalize(v_refracted), EPSILON, GLOBAL_MAX_T, recursive_depth+1, !in_object);
        bool hit = refract_ray->fire_ray(true);
        if(hit) {
            colour = (1-transp_coeff)*colour + transp_coeff * refract_ray->calc_lighting();
        }
        else colour = (1-transp_coeff)*colour + transp_coeff * background_colour;
    } else {
        colour = do_reflection(intersect_pos, V, colour, material);
    }

    return colour;
}
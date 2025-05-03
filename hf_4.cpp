//=============================================================================================
// Computer Graphics Sample Program: Ray-tracing-let
//=============================================================================================
#include "framework.h"
#include <chrono>
using namespace std::chrono;

const int windowWidth = 600, windowHeight = 600;

struct Material {
	vec3 ka, kd, ks;
	float  shininess;
	Material(vec3 _kd, vec3 _ks, float _shininess) : ka(_kd * (float)M_PI), kd(_kd), ks(_ks) { shininess = _shininess; }
};

struct Hit {
	float t;
	vec3 position, normal;
	Material * material;
	Hit() { t = -1; }
};

struct Ray {
	vec3 start, dir;
	Ray(vec3 _start, vec3 _dir) { start = _start; dir = normalize(_dir); }
};

class Intersectable {
protected:
	Material * material;
public:
	virtual Hit intersect(const Ray& ray) = 0;
};

class Sphere : public Intersectable {
	vec3 center;
	float radius;
public:
	Sphere(const vec3& _center, float _radius, Material* _material) {
		center = _center; radius = _radius; material = _material;
	}

	Hit intersect(const Ray& ray) {
		Hit hit;
		vec3 dist = ray.start - center;
		float a = dot(ray.dir, ray.dir);
		float b = dot(dist, ray.dir) * 2.0f;
		float c = dot(dist, dist) - radius * radius;
		float discr = b * b - 4.0f * a * c;
		if (discr < 0) return hit;
		float sqrt_discr = sqrtf(discr);
		float t1 = (-b + sqrt_discr) / 2.0f / a;	// t1 >= t2 for sure
		float t2 = (-b - sqrt_discr) / 2.0f / a;
		if (t1 <= 0) return hit;
		hit.t = (t2 > 0) ? t2 : t1;
		hit.position = ray.start + ray.dir * hit.t;
		hit.normal = (hit.position - center) / radius;
		hit.material = material;
		return hit;
	}
};

class Cylinder : public Intersectable {
	vec3 basepoint;      // Endpoints of the cylinder axis
	float radius;
	float height;
	vec3 axis;        // Axis direction (unit vector)

public:
	Cylinder(const vec3& _bp, const vec3& _axis, float _radius, float _height, Material* _material) {
		basepoint = _bp;
		radius = _radius;
		axis = _axis;
		material = _material;
		height = _height;
	}

	Hit intersect(const Ray& ray) override {
		Hit hit;

		vec3 d = ray.dir;
		vec3 m = ray.start - basepoint;
		vec3 n = axis;

		float mdn = dot(m, n);
		float ddn = dot(d, n);

		// Project ray and cylinder to a plane perpendicular to axis
		vec3 d_proj = d - ddn * n;
		vec3 m_proj = m - mdn * n;

		float a = dot(d_proj, d_proj);
		float b = 2 * dot(d_proj, m_proj);
		float c = dot(m_proj, m_proj) - radius * radius;

		float discr = b * b - 4 * a * c;
		if (discr < 0) return hit;

		float sqrt_discr = sqrtf(discr);
		float t1 = (-b - sqrt_discr) / (2 * a);
		float t2 = (-b + sqrt_discr) / (2 * a);

		// Check both t1 and t2 for valid intersection within cylinder height
		for (float t : { t1, t2 }) {
			if (t <= 0) continue;
			vec3 pos = ray.start + d * t;
			float h = dot(pos - basepoint, n);
			if (pos.y<basepoint.y+height) {
				hit.t = t;
				hit.position = pos;
				vec3 axis_point = basepoint + n * h;
				hit.normal = normalize(pos - axis_point);
				hit.material = material;
				return hit;
			}
		}

		return hit; // No valid intersection
	}
};

class Plane : public Intersectable {
	vec3 point;   // A point on the plane
	vec3 normal;  // The normal vector of the plane (assumed normalized)
public:
	Plane(const vec3& _point, const vec3& _normal, Material* _material) {
		point = _point;
		normal = normalize(_normal);
		material = _material;
	}

	virtual Material* getMaterial(vec3& position) {
		return material;
	}

	Hit intersect(const Ray& ray) {
		Hit hit;
		float denom = dot(ray.dir, normal);
		if (fabs(denom) < 1e-6) return hit; // Ray is parallel to the plane

		float t = dot(point - ray.start, normal) / denom;
		if (t < 0) return hit; // Plane is behind the ray
		hit.position = ray.start + ray.dir * t;
		if (hit.position.x < -10) return hit;
		if (hit.position.x > 10) return hit;
		if (hit.position.z < -10) return hit;
		if (hit.position.z > 10) return hit;


		hit.t = t;
		hit.normal = normal;
		hit.material = getMaterial(hit.position);
		return hit;
	}
};

class CheckeredPlane : public Plane {
	Material* a;
	Material* b;
public:
	CheckeredPlane(const vec3& _point, const vec3& _normal, Material* _a, Material* _b) : Plane(_point, _normal, nullptr) {
		a = _a; b = _b;
	};
	Material* getMaterial(vec3& position) {
		int xi = (int)floor(position.x);
		int zi = (int)floor(position.z);
		bool isBlack = ((xi & 1) == (zi & 1));
		return isBlack ? b : a;
	}
};

class Camera {
	vec3 eye, lookat, right, up;
	float fov;
public:
	void set(vec3 _eye, vec3 _lookat, vec3 vup, float _fov) {
		eye = _eye; lookat = _lookat; fov = _fov;
		vec3 w = eye - lookat;
		float windowSize = length(w) * tanf(fov / 2);
		right = normalize(cross(vup, w)) * (float)windowSize * (float)windowWidth / (float)windowHeight;
		up = normalize(cross(w, right)) * windowSize;
	}

	Ray getRay(int X, int Y) {
		vec3 dir = lookat + right * (2 * (X + 0.5f) / windowWidth - 1) + up * (2 * (Y + 0.5f) / windowHeight - 1) - eye;
		return Ray(eye, dir);
	}

	void Animate() {
		float dt = 0.125;
		vec3 d = eye - lookat;
		eye = vec3(d.x * cos(dt) + d.z * sin(dt), d.y, -d.x * sin(dt) + d.z * cos(dt)) + lookat;
		set(eye, lookat, up, fov);
	}
};

struct Light {
	vec3 direction;
	vec3 Le;
	Light(vec3 _direction, vec3 _Le) {
		direction = normalize(_direction);
		Le = _Le;
	}
};

float rnd() { return (float)rand() / RAND_MAX; }

const float Epsilon = 0.0001f;

class Scene {
	std::vector<Intersectable *> objects;
	std::vector<Light *> lights;
	Camera camera;
	vec3 La;
public:
	void build() {
		vec3 eye = vec3(0, 1, 4), vup = vec3(0, 1, 0), lookat = vec3(0, 0, 0);
		float fov = 45 * (float)M_PI / 180;
		camera.set(eye, lookat, vup, fov);

		La = vec3(0.4f, 0.4f, 0.4f);
		vec3 lightDirection(1, 1, 1), Le(2, 2, 2);
		lights.push_back(new Light(lightDirection, Le));

		vec3 kd1(0.3f, 0.2f, 0.1f), kd2(0.1f, 0.2f, 0.3f), ks(2, 2, 2);
		Material * leftHandCylinder = new Material(kd1, ks, 50);
		Material * material2 = new Material(kd2, ks, 100);

		vec3 z(0,0,0), blue(0.0f,0.1f,0.3f), white(0.3f, 0.3f, 0.3f);
		Material* blueFloor = new Material(blue, z, 0);
		Material* whiteFloor = new Material(white, z, 0);

		objects.push_back(new CheckeredPlane(vec3(0,-1,0), vec3(0,1,0), blueFloor, whiteFloor)); //floor
		objects.push_back(new Cylinder(vec3(1,-1,0), vec3(0.1f,1,0), 0.3f, 2, material2)); //should be shiny and golden, right hand cylinder
		objects.push_back(new Cylinder(vec3(0, -1, -0.8f), vec3(-0.2f,1,-0.1f), 0.3, 2, material2));
		objects.push_back(new Cylinder(vec3(-1,-1,0), vec3(0,1,0.1f), 0.3f, 2, leftHandCylinder)); //left hand cylinder


	}

	void render(std::vector<vec3>& image) {	
		for (int Y = 0; Y < windowHeight; Y++) {
#pragma omp parallel for
			for (int X = 0; X < windowWidth; X++) {
				vec3 color = trace(camera.getRay(X, Y));
				image[Y * windowWidth + X] = vec3(color.x, color.y, color.z);
			}
		}
	}

	Hit firstIntersect(Ray ray) {
		Hit bestHit;
		for (Intersectable * object : objects) {
			Hit hit = object->intersect(ray); //  hit.t < 0 if no intersection
			if (hit.t > 0 && (bestHit.t < 0 || hit.t < bestHit.t))  bestHit = hit;
		}
		if (dot(ray.dir, bestHit.normal) > 0) bestHit.normal = -bestHit.normal;
		return bestHit;
	}

	bool shadowIntersect(Ray ray) {	// for directional lights
		for (Intersectable * object : objects) if (object->intersect(ray).t > 0) return true;
		return false;
	}

	vec3 trace(Ray ray, int depth = 0) {
		Hit hit = firstIntersect(ray);
		if (hit.t < 0) return La;

		vec3 outRadiance = hit.material->ka * La;
		for (Light * light : lights) {
			Ray shadowRay(hit.position + hit.normal * Epsilon, light->direction);
			float cosTheta = dot(hit.normal, light->direction);
			if (cosTheta > 0 && !shadowIntersect(shadowRay)) {	// shadow computation
				outRadiance = outRadiance + light->Le * hit.material->kd * cosTheta;
				vec3 halfway = normalize(-ray.dir + light->direction);
				float cosDelta = dot(hit.normal, halfway);
				if (cosDelta > 0) outRadiance = outRadiance + light->Le * hit.material->ks * powf(cosDelta, hit.material->shininess);
			}
		}
		return outRadiance;
	}

	void Animate() { camera.Animate(); }
};

Scene scene;
GPUProgram gpuProgram; // vertex and fragment shaders

// vertex shader in GLSL
const char * vertexSource = R"(
	#version 330
    precision highp float;

	layout(location = 0) in vec2 cVertexPosition;	// Attrib Array 0
	out vec2 texcoord;

	void main() {
		texcoord = (cVertexPosition + vec2(1, 1))/2;							// -1,1 to 0,1
		gl_Position = vec4(cVertexPosition.x, cVertexPosition.y, 0, 1); 		// transform to clipping space
	}
)";

// fragment shader in GLSL
const char * fragmentSource = R"(
	#version 330
    precision highp float;

	uniform sampler2D textureUnit;
	in  vec2 texcoord;			// interpolated texture coordinates
	out vec4 fragmentColor;		// output that goes to the raster memory as told by glBindFragDataLocation

	void main() { fragmentColor = texture(textureUnit, texcoord); }
)";

class FullScreenTexturedQuad : public Geometry<vec2> {
	Texture* texture;
public:
	FullScreenTexturedQuad() {
		vtx = { vec2(-1, -1),  vec2(1, -1),  vec2(1, 1),  vec2(-1, 1) }; 
		updateGPU();
	}
	void LoadTexture(int width, int height, std::vector<vec3>& image) {
		texture = new Texture(width, height, image);
	}
	void Draw() {
		Bind();
		texture->Bind(0);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);	// draw two triangles forming a quad
	}
};


class RaytraceApp : public glApp {
	Geometry<vec2>* triangle;  // geometria
	GPUProgram gpuProgram;	   // csúcspont és pixel árnyalók
	FullScreenTexturedQuad* fullScreenTexturedQuad;
public:
	RaytraceApp() : glApp("Ray tracing") { }

	// Inicializáció, 
	void onInitialization() {
		glViewport(0, 0, windowWidth, windowHeight);
		scene.build();
		fullScreenTexturedQuad = new FullScreenTexturedQuad;
		gpuProgram.create(vertexSource, fragmentSource); 	// create program for the GPU
	}

	// Ablak újrarajzolás
	void onDisplay() {
		std::vector<vec3> image(windowWidth * windowHeight);
		scene.render(image); 						// Execute ray casting
		fullScreenTexturedQuad->LoadTexture(windowWidth, windowHeight, image); // copy image to GPU as a texture
		fullScreenTexturedQuad->Draw();				// Display rendered image on screen
	}

	void onKeyboard(int key) {
		if (key=='a') {
			scene.Animate();
			refreshScreen();
		}
	}
} app;

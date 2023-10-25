#include <windows.h>
#include <windowsx.h>
#include <iostream>
#include <chrono>
#include <vector>
#include <thread>
#include <unistd.h>
#include <cmath>
#include "util.h"
#include "window.h"

#define PI 3.14159265359
typedef unsigned long uint;

struct ivec2{
	int x;
	int y;
};

struct mouse{
	ivec2 pos;
	bool lmb = false;
	bool rmb = false;
} mouse;

uint window_width = 1200;		//Größe des Fensters
uint window_height = 800;
uint pixel_size = 1;
uint buffer_width = window_width/pixel_size;
uint buffer_height = window_height/pixel_size;
triangle* triangles = new triangle[100000];
uint triangle_count = 0;

inline void addCube(vec3 pos, vec3 size, uint color){
	triangle t[12]; t[0].point[0].pos = {pos.x + size.x, pos.y + size.y, pos.z - size.z}; t[0].point[1].pos = {pos.x + size.x, pos.y - size.y, pos.z - size.z}; t[0].point[2].pos = {pos.x - size.x, pos.y - size.y, pos.z - size.z};
	t[1].point[0].pos = {pos.x - size.x, pos.y - size.y, pos.z - size.z}; t[1].point[1].pos = {pos.x - size.x, pos.y + size.y, pos.z - size.z}; t[1].point[2].pos = {pos.x + size.x, pos.y + size.y, pos.z - size.z};
	t[2].point[0].pos = {pos.x + size.x, pos.y + size.y, pos.z + size.z}; t[2].point[1].pos = {pos.x + size.x, pos.y - size.y, pos.z + size.z}; t[2].point[2].pos = {pos.x - size.x, pos.y - size.y, pos.z + size.z};
	t[3].point[0].pos = {pos.x - size.x, pos.y - size.y, pos.z + size.z}; t[3].point[1].pos = {pos.x - size.x, pos.y + size.y, pos.z + size.z}; t[3].point[2].pos = {pos.x + size.x, pos.y + size.y, pos.z + size.z};
	t[4].point[0].pos = {pos.x + size.x, pos.y + size.y, pos.z - size.z}; t[4].point[1].pos = {pos.x + size.x, pos.y - size.y, pos.z - size.z}; t[4].point[2].pos = {pos.x + size.x, pos.y - size.y, pos.z + size.z};
	t[5].point[0].pos = {pos.x + size.x, pos.y - size.y, pos.z + size.z}; t[5].point[1].pos = {pos.x + size.x, pos.y + size.y, pos.z + size.z}; t[5].point[2].pos = {pos.x + size.x, pos.y + size.y, pos.z - size.z};
	t[6].point[0].pos = {pos.x - size.x, pos.y + size.y, pos.z - size.z}; t[6].point[1].pos = {pos.x - size.x, pos.y - size.y, pos.z - size.z}; t[6].point[2].pos = {pos.x - size.x, pos.y - size.y, pos.z + size.z};
	t[7].point[0].pos = {pos.x - size.x, pos.y - size.y, pos.z + size.z}; t[7].point[1].pos = {pos.x - size.x, pos.y + size.y, pos.z + size.z}; t[7].point[2].pos = {pos.x - size.x, pos.y + size.y, pos.z - size.z};
	t[8].point[0].pos = {pos.x + size.x, pos.y + size.y, pos.z - size.z}; t[8].point[1].pos = {pos.x - size.x, pos.y + size.y, pos.z - size.z}; t[8].point[2].pos = {pos.x - size.x, pos.y + size.y, pos.z + size.z};
	t[9].point[0].pos = {pos.x - size.x, pos.y + size.y, pos.z + size.z}; t[9].point[1].pos = {pos.x + size.x, pos.y + size.y, pos.z + size.z}; t[9].point[2].pos = {pos.x + size.x, pos.y + size.y, pos.z - size.z};
	t[10].point[0].pos = {pos.x + size.x, pos.y - size.y, pos.z - size.z}; t[10].point[1].pos = {pos.x - size.x, pos.y - size.y, pos.z - size.z}; t[10].point[2].pos = {pos.x - size.x, pos.y - size.y, pos.z + size.z};
	t[11].point[0].pos = {pos.x - size.x, pos.y - size.y, pos.z + size.z}; t[11].point[1].pos = {pos.x + size.x, pos.y - size.y, pos.z + size.z}; t[11].point[2].pos = {pos.x + size.x, pos.y - size.y, pos.z - size.z};
	for(uint i=0; i < 12; ++i){
		t[i].point[0].color = {((color>>16) & 0xFF)/255.f, ((color>>8) & 0xFF)/255.f, ((color) & 0xFF)/255.f};
		t[i].point[1].color = {((color>>16) & 0xFF)/255.f, ((color>>8) & 0xFF)/255.f, ((color) & 0xFF)/255.f};
		t[i].point[2].color = {((color>>16) & 0xFF)/255.f, ((color>>8) & 0xFF)/255.f, ((color) & 0xFF)/255.f};
		triangles[triangle_count++] = t[i];
	}
}

struct Slider{
	ivec2 pos = {0, 0};
	ivec2 size = {80, 10};
	short sliderPos = 0;
	float val = 0;
	float min = 0;
	float max = 100;
};

void updateSliders(Slider* sliders, uint count){
	for(uint i=0; i < count; ++i){
		Slider& s = sliders[i];
		int x = mouse.pos.x - s.pos.x;
		int y = mouse.pos.y - s.pos.y;
		if(mouse.lmb && x >= 0 && x <= s.size.x && y >= 0 && y <= s.size.y){
			s.sliderPos = x;
			s.val = ((float)(s.sliderPos))/s.size.x*(s.max-s.min)+s.min;
		}
		for(int y=s.pos.y; y < s.pos.y+s.size.y; ++y){
			for(int x=s.pos.x; x < s.pos.x+s.size.x; ++x){
//				memory[y*buffer_width+x] = 0x303030;
			}
		}
		for(int y=s.pos.y; y < s.pos.y+s.size.y; ++y){
			for(int x=s.pos.x+s.sliderPos; x < s.pos.x+s.sliderPos+2; ++x){
//				memory[y*buffer_width+x] = 0x808080;
			}
		}
	}
}

struct Particle{
	vec2 pos = {0};
	vec2 vel = {0};
	vec2 force = {0};
	float radius = 1.0;
	float mass = 1;
	float density = 1;
	vec2 predicted_pos = {0};
};

static uint numParticles = 2000;
Particle* particles = new Particle[numParticles];

LRESULT CALLBACK window_callback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
	switch(uMsg){
	case WM_DESTROY: {
		exit(0);
		break;
	}
	case WM_SIZE:{
		window_width = LOWORD(lParam);
		window_height = HIWORD(lParam);
		buffer_width = window_width/pixel_size;
		buffer_height = window_height/pixel_size;
    	glViewport(0, 0, LOWORD(lParam), HIWORD(lParam));
    	float aspect_ratio = (float)window_width/window_height;
    	glMatrixMode(GL_PROJECTION);	//Passe die Projektionsmatrix an
    	glLoadIdentity();				//Leere die Projektionsmatrix
    	glFrustum(-.6*aspect_ratio, .6*aspect_ratio, .6, -.6, 1., 100000.);	//Setze die Projektionsmatrix
        break;
	}
	case WM_LBUTTONDOWN:{
		mouse.lmb = true;
		break;
	}
	case WM_LBUTTONUP:{
		mouse.lmb = false;
		break;
	}
	case WM_RBUTTONDOWN:{
		mouse.rmb = true;
		break;
	}
	case WM_RBUTTONUP:{
		mouse.rmb = false;
		break;
	}
	case WM_MOUSEMOVE:{
		mouse.pos.x = GET_X_LPARAM(lParam)/pixel_size;
		mouse.pos.y = GET_Y_LPARAM(lParam)/pixel_size;
		break;
	}
	case WM_KEYDOWN:{
		switch(wParam){
		case 0x57:
			cam.pos.z -= 10;
			break;
		case 0x53:
			cam.pos.z += 10;
			break;
		case 0x41:
			cam.pos.x -= 10;
			break;
		case 0x44:
			cam.pos.x += 10;
			break;
		case VK_SHIFT:
			cam.pos.y += 10;
			break;
		case VK_SPACE:
			cam.pos.y -= 10;
			break;
		case VK_UP:
			cam.rot.y += 0.025;
			break;
		case VK_DOWN:
			cam.rot.y -= 0.025;
			break;
		case VK_LEFT:
			cam.rot.x -= 0.025;
			break;
		case VK_RIGHT:
			cam.rot.x += 0.025;
			break;
		}
		break;
	}
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

static float RADIUS = 30;
static float MASS = 70;

#define DAMPING -0.5
#define BOUNDX buffer_width
#define BOUNDY buffer_height

static uint HASHGRIDX = buffer_width/(RADIUS*2);
static uint HASHGRIDY = buffer_height/(RADIUS*2);
static uint cellParticleCount = numParticles/2;
static uint DEBUGCOUNT = 0;
uint* hashIndices = new uint[cellParticleCount*HASHGRIDX*HASHGRIDY];
uint* particleCount = new uint[HASHGRIDX*HASHGRIDY];	//Speichert wie viele Particle sich in einer Bin befinden
inline void clearCount(void){
	for(uint i=0; i < HASHGRIDX*HASHGRIDY; ++i){
		particleCount[i] = 0;
	}
	DEBUGCOUNT = 0;
}
inline void particlesToGrid(uint start_idx, uint end_idx){
	for(uint i=start_idx; i < end_idx; ++i){
		Particle& p = particles[i];
		uint idx = (uint)(p.pos.y/(BOUNDY+1)*HASHGRIDY)*HASHGRIDX+p.pos.x/(BOUNDX+1)*HASHGRIDX;
		uint count = particleCount[idx];
		if(count > DEBUGCOUNT) DEBUGCOUNT = count;
		particleCount[idx] += 1;
		idx *= cellParticleCount;
		idx += count;
		hashIndices[idx] = i;
	}
}

inline long positionToIndex(vec2 pos){
	return (uint)(pos.y/(BOUNDY+1)*HASHGRIDY)*HASHGRIDX+pos.x/(BOUNDX+1)*HASHGRIDX;
}

void Integrate(double dt, uint start_idx, uint end_idx){
	for(uint i=start_idx; i < end_idx; ++i){
		//Update particle
		Particle& p = particles[i];
		p.vel.x += p.force.x/p.density*dt;
		p.vel.y += p.force.y/p.density*dt;
		p.pos.x += p.vel.x*dt;
		p.pos.y += p.vel.y*dt;
		p.predicted_pos.x = p.pos.x+p.vel.x*dt;
		p.predicted_pos.y = p.pos.y+p.vel.y*dt;

		if(p.pos.y < 0){
			p.pos.y = 0;
			p.vel.y = p.vel.y*DAMPING;
		}
		else if(p.pos.y >= BOUNDY-1){
			p.pos.y = BOUNDY-1;
			p.vel.y = p.vel.y*DAMPING;
		}
		if(p.pos.x < 0){
			p.pos.x = 0;
			p.vel.x = p.vel.x*DAMPING;
		}
		else if(p.pos.x >= BOUNDX-1){
			p.pos.x = BOUNDX-1;
			p.vel.x = p.vel.x*DAMPING;
		}
	}
}

inline float smoothingKernel(float radius, float distance){
	if(distance >= radius) return 0;
	float volume = PI*std::pow(radius, 4)/6.f;
	return (radius-distance)*(radius-distance)/volume;
}
inline float smoothingKernelDerivative(float radius, float distance){
	if(distance >= radius) return 0;
	float scale = 12/std::pow(radius, 4)*PI;
	return (distance-radius)*scale;
}
void computeDensity(uint start_idx, uint end_idx){
	for(uint i=start_idx; i < end_idx; ++i){

		Particle& p = particles[i];
		p.density = 1;

		for(int y=-1; y <= 1; ++y){
			for(int x=-1; x <= 1; ++x){
				uint idx = positionToIndex(p.pos);
				idx += y*HASHGRIDX+x;
				if(idx >= HASHGRIDX*HASHGRIDY) continue;
				uint count = particleCount[idx];
				for(uint j=0; j < count; ++j){
					uint pidx = hashIndices[idx*cellParticleCount+j];
					if(pidx==i) continue;
					float dist = length(p.predicted_pos, particles[pidx].predicted_pos);
					float strength = smoothingKernel(RADIUS, dist);
					p.density += MASS * strength;
				}
			}
		}
	}
}

static float PRESSURE_MULTIPLIER = 5000;
static float TARGET_DENSITY = 1.17;
static float GRAVITY = 0;
inline float densityError(float density){
	float err = density - TARGET_DENSITY;
	return err * PRESSURE_MULTIPLIER;
}
inline float sharedPressure(float densityA, float densityB){
	float pressureA = densityError(densityA);
	float pressureB = densityError(densityB);
	return (pressureA+pressureB)/2.f;
}
void computeForces(uint start_idx, uint end_idx){
	for(uint i=start_idx; i < end_idx; ++i){

		Particle& p = particles[i];
		p.force = {0, 0};

		for(int y=-1; y <= 1; ++y){
			for(int x=-1; x <= 1; ++x){
				uint idx = positionToIndex(p.pos);
				idx += y*HASHGRIDX+x;
				if(idx >= HASHGRIDX*HASHGRIDY) continue;
				uint count = particleCount[idx];
				for(uint j=0; j < count; ++j){
					uint pidx = hashIndices[idx*cellParticleCount+j];
					if(pidx==i) continue;
					float dist = length(p.pos, particles[pidx].pos);
					vec2 dir;
					if(dist == 0){
						float val = (nextrand()%2000001)/1000000-1;
						dir = {(float)cos(val), (float)sin(val)};
					}
					else dir = {(p.pos.x - particles[pidx].pos.x)/dist, (p.pos.y - particles[pidx].pos.y)/dist};
					float strength = smoothingKernelDerivative(RADIUS, dist);
					float pressure = sharedPressure(p.density, particles[pidx].density);
					p.force.x -= pressure*dir.x*MASS*strength/p.density;
					p.force.y -= pressure*dir.y*MASS*strength/p.density;
				}
			}
		}
		p.force.y += GRAVITY;
	}
}

INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow){
	HDC hDC; HGLRC hRC;

	HWND window = CreateOpenGLWindow("Window", 0, 0, window_width, window_height, PFD_TYPE_RGBA, 0, window_callback);
	ShowWindow(window, nCmdShow);
	if(window == NULL) exit(-1);

	hDC = GetDC(window);			//Hole den Device-Kontext
	hRC = wglCreateContext(hDC);	//Erstelle neuen Kontext
	wglMakeCurrent(hDC, hRC);		//Mache den neuen Kontext den Momentanen
	glEnable(GL_DEPTH_TEST);		//Depth-Buffering

	glViewport(0, 0, window_width, window_height);
	float aspect_ratio = (float)window_width/window_height;
	glMatrixMode(GL_PROJECTION);	//Passe die Projektionsmatrix an
	glLoadIdentity();				//Leere die Projektionsmatrix
	glFrustum(-.6*aspect_ratio, .6*aspect_ratio, .6, -.6, 1., 100000.);	//Setze die Projektionsmatrix

	Timer timer;

	//-----------INIT-----------

	//Init particles
	for(uint i=0; i < numParticles; ++i){
		float x = rand()%BOUNDX;
		float y = rand()%BOUNDY;
		particles[i] = {{x, y}, {0}, {0}, 50, 80, 1};
	}

	//Init sliders
	Slider sliders[5];
	sliders[0] = {{(int)buffer_width-100-5, 5}, {100, 15}, 0, 1.3, 0, 3.0};
	sliders[1] = {{(int)buffer_width-100-5, 25}, {100, 15}, 0, 1500000, 10000, 2000000};
	sliders[2] = {{(int)buffer_width-100-5, 45}, {100, 15}, 0, 30, 20, 180};
	sliders[3] = {{(int)buffer_width-100-5, 65}, {100, 15}, 0, 100, 1, 300};
	sliders[4] = {{(int)buffer_width-100-5, 85}, {100, 15}, 0, 1200, 0, 12000};
	uint slider_count = 5;

	cam.pos = {(float)buffer_width/2, (float)buffer_height/2, 650};
	cam.rot = {0, 0};
	cam.focal_length = 1.;

	//-----------END INIT-----------

	while(1){
		MSG msg = {};
		while(PeekMessage(&msg, window, 0, 0, PM_REMOVE)){
		    TranslateMessage(&msg);
		    DispatchMessage(&msg);
		}

		timer.start();

		clearCount();

//#define MULTITHREADING
#ifdef MULTITHREADING
#define THREADCOUNT 6
		std::vector<std::thread> threads;
		uint thread_inc = numParticles/THREADCOUNT;
		for(uint i=0; i < THREADCOUNT; ++i){
			threads.push_back(std::thread(particlesToGrid, thread_inc*i, thread_inc*(i+1)));
		}
		for(auto& i : threads){
			i.join();
		}

		std::vector<std::thread> threads2;
		for(uint i=0; i < THREADCOUNT; ++i){
			threads2.push_back(std::thread(UpdateFluid, particles, 0.008, thread_inc*i, thread_inc*(i+1)));
		}
		for(auto& i : threads2){
			i.join();
		}
#else
	    Integrate(0.009, 0, numParticles);
		particlesToGrid(0, numParticles);
		computeDensity(0, numParticles);
	    computeForces(0, numParticles);
#endif

		std::cout << "Simulationszeit: " << timer.measure_ms() << std::endl;
		std::cout << DEBUGCOUNT << std::endl;

		if(mouse.lmb || mouse.rmb){
			for(uint i=0; i < numParticles; ++i){
				Particle& p = particles[i];
				vec2 dir = {p.pos.x-mouse.pos.x, p.pos.y-mouse.pos.y};
				float dist = sqrt(dir.x*dir.x+dir.y*dir.y);
				dir.x /= dist; dir.y /= dist;
				if(dist < RADIUS*4){
					if(mouse.rmb){
						dir.x *= -1;
						dir.y *= -1;
					}
					p.force.x += dir.x*8000;
					p.force.y += dir.y*8000;
				}
			}
		}

		//Update sliders
		updateSliders(sliders, slider_count);
		TARGET_DENSITY = sliders[0].val;
		PRESSURE_MULTIPLIER = sliders[1].val;
		RADIUS = sliders[2].val;
		MASS = sliders[3].val;
		GRAVITY = sliders[4].val;

		timer.start();

		triangle_count = 0;
		for(uint i=0; i < numParticles; ++i){
			Particle& p = particles[i];
			addCube({p.pos.x, p.pos.y, 0}, {2.5, 2.5, 2.5}, 0x0040FF);
		}

		display(triangles, triangle_count);

		std::cout << "Renderzeit: " << timer.measure_ms() << std::endl;

	}

	delete[] particles;
	delete[] triangles;
    wglMakeCurrent(NULL, NULL);
    ReleaseDC(window, hDC);
    wglDeleteContext(hRC);
    DestroyWindow(window);

}

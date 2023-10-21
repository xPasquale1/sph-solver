#include <windows.h>
#include <windowsx.h>
#include <iostream>
#include <chrono>
#include <vector>
#include <thread>
#include <unistd.h>
#include <cmath>
#include "util.h"

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
uint* memory = nullptr;			//Pointer zum pixel-array

inline __attribute__((always_inline)) void setPixel(int x, int y, uint color){
	if(x < 0 || x >= (int)buffer_width || y < 0 || y >= (int)buffer_height) return;
	memory[y*buffer_width+x] = color;
}

inline void drawCircleOutline(int x, int y, float radius, float thickness, uint color){
	for(int dx = x-radius; dx <= x+radius; ++dx){
		for(int dy = y-radius; dy <= y+radius; ++dy){
			int l1 = dx-x; int l2 = dy-y;
			float mid = l1*l1+l2*l2;
			if(mid <= radius*radius && mid >= (radius-thickness)*(radius-thickness)){
				setPixel(dx, dy, color);
			}
		}
	}
}

inline void drawCircle(int x, int y, float radius, uint color){
	for(int dx = x-radius; dx <= x+radius; ++dx){
		for(int dy = y-radius; dy <= y+radius; ++dy){
			int l1 = dx-x; int l2 = dy-y;
			float mid = l1*l1+l2*l2;
			if(mid <= radius*radius){
				setPixel(dx, dy, color);
			}
		}
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
				memory[y*buffer_width+x] = 0x303030;
			}
		}
		for(int y=s.pos.y; y < s.pos.y+s.size.y; ++y){
			for(int x=s.pos.x+s.sliderPos; x < s.pos.x+s.sliderPos+2; ++x){
				memory[y*buffer_width+x] = 0x808080;
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
	float pressure = 0;
	float density = 1;
	vec2 predicted_pos = {0};
	byte state = 0;
};

static uint numParticles = 3200;
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
        delete[] memory;
        memory = new uint[buffer_width*buffer_height];
        for(uint i=0; i < buffer_width*buffer_height; ++i){
        	memory[i] = 0;
        }
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
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

static float RADIUS = 20;
static float MASS = 70;

#define DAMPING -0.5
#define BOUNDX buffer_width
#define BOUNDY buffer_height

static uint HASHGRIDX = buffer_width/(RADIUS*2);
static uint HASHGRIDY = buffer_height/(RADIUS*2);
uint* hashIndices = new uint[numParticles*HASHGRIDX*HASHGRIDY];
uint* particleCount = new uint[HASHGRIDX*HASHGRIDY];	//Speichert wie viele Particle sich in einer Bin befinden
inline void clearCount(void){
	for(uint i=0; i < HASHGRIDX*HASHGRIDY; ++i){
		particleCount[i] = 0;
	}
	for(uint i=0; i < numParticles; ++i){
		particles[i].state = 0;
	}
}
inline void particlesToGrid(uint start_idx, uint end_idx){
	for(uint i=start_idx; i < end_idx; ++i){
		Particle& p = particles[i];
		uint idx = (uint)(p.pos.y/(BOUNDY+1)*HASHGRIDY)*HASHGRIDX+p.pos.x/(BOUNDX+1)*HASHGRIDX;
		uint count = particleCount[idx];
		particleCount[idx] += 1;
		idx *= numParticles;
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
	float volume = PI*pow(radius, 4)/6.f;
	return (radius-distance)*(radius-distance)/volume;
}
inline float smoothingKernelDerivative(float radius, float distance){
	if(distance >= radius) return 0;
	float scale = 12/pow(radius, 4)*PI;
	return (distance-radius)*scale;
}
void computeDensity(uint start_idx, uint end_idx){
	for(uint i=start_idx; i < end_idx; ++i){

		Particle& p = particles[i];
		p.density = 1;

//		for(uint j=0; j < numParticles; ++j){
//			if(i==j) continue;
//			float dist = length(p.predicted_pos, particles[j].predicted_pos);
//			float strength = smoothingKernel(RADIUS, dist);
//			p.density += MASS * strength;
//		}

		for(int y=-1; y <= 1; ++y){
			for(int x=-1; x <= 1; ++x){
				uint idx = positionToIndex(p.pos);
				idx += y*HASHGRIDX+x;
				if(idx >= HASHGRIDX*HASHGRIDY) continue;
				uint count = particleCount[idx];
				for(uint j=0; j < count; ++j){
					uint pidx = hashIndices[idx*numParticles+j];
					if(pidx==i) continue;
					if(i==50) particles[pidx].state = 1;
					float dist = length(p.predicted_pos, particles[pidx].predicted_pos);
					float strength = smoothingKernel(RADIUS, dist);
					p.density += MASS * strength;
				}
			}
		}
	}
}

void computePressure(uint start_idx, uint end_idx){
	for(uint i=start_idx; i < end_idx; ++i){

		Particle& p = particles[i];
		p.pressure = 0;

//		for(uint j=0; j < numParticles; ++j){
//			if(i==j) continue;
//			float dist = length(p.predicted_pos, particles[j].predicted_pos);
//			float strength = smoothingKernel(RADIUS, dist);
//			p.pressure += particles[j].density * MASS * strength / p.density;
//		}

		for(int y=-1; y <= 1; ++y){
			for(int x=-1; x <= 1; ++x){
				uint idx = positionToIndex(p.pos);
				idx += y*HASHGRIDX+x;
				if(idx >= HASHGRIDX*HASHGRIDY) continue;
				uint count = particleCount[idx];
				for(uint j=0; j < count; ++j){
					uint pidx = hashIndices[idx*numParticles+j];
					if(pidx==i) continue;
					float dist = length(p.predicted_pos, particles[pidx].predicted_pos);
					float strength = smoothingKernel(RADIUS, dist);
					p.pressure += particles[pidx].density * MASS * strength / p.density;
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
					uint pidx = hashIndices[idx*numParticles+j];
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

//		for(uint j=0; j < numParticles; ++j){
//			if(i==j) continue;
//			float dist = length(p.pos, particles[j].pos);
//			vec2 dir;
//			if(dist == 0){
//				float val = (nextrand()%2000001)/1000000-1;
//				dir = {(float)cos(val), (float)sin(val)};
//			}
//			else dir = {(p.pos.x - particles[j].pos.x)/dist, (p.pos.y - particles[j].pos.y)/dist};
//			float strength = smoothingKernelDerivative(RADIUS, dist);
//			float pressure = sharedPressure(p.density, particles[j].density);
//			p.force.x -= pressure*dir.x*MASS*strength/p.density;
//			p.force.y -= pressure*dir.y*MASS*strength/p.density;
//		}

		p.force.y += GRAVITY;
	}
}

void UpdateFluid(Particle* particles, double dt, uint start_idx, uint end_idx){
    Integrate(dt, start_idx, end_idx);
//	particlesToGrid(start_idx, end_idx);
	computeDensity(start_idx, end_idx);
    computeForces(start_idx, end_idx);
}

INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow){

	//Erstelle Fenster Klasse
	WNDCLASS window_class = {};
	window_class.style = CS_HREDRAW | CS_VREDRAW;
	window_class.lpszClassName = "Window-Class";
	window_class.lpfnWndProc = window_callback;

	//Registriere Fenster Klasse
	RegisterClass(&window_class);

	//Erstelle das Fenster
	HWND window = CreateWindow(window_class.lpszClassName, "Window", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, window_width, window_height, NULL, NULL, hInstance, NULL);

	//Bitmap-Info
	BITMAPINFO bitmapInfo = {};
	bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
	bitmapInfo.bmiHeader.biPlanes = 1;
	bitmapInfo.bmiHeader.biBitCount = 32;
	bitmapInfo.bmiHeader.biCompression = BI_RGB;

	Timer timer;

	//-----------INIT-----------
	//Setze die Bildschirmfarbe auf Schwarz
	for(uint y=0; y < buffer_height; ++y){
	    for(uint x=0; x < buffer_width; ++x){
	    	memory[y*buffer_width+x] = 0;
	    }
	}

	//Init particles
	for(uint i=0; i < numParticles; ++i){
		float x = rand()%BOUNDX;
		float y = rand()%BOUNDY;
		particles[i] = {{x, y}, {0}, {0}, 50, 80, 0, 1};
	}

	//Init sliders
	Slider sliders[5];
	sliders[0] = {{(int)buffer_width-100-5, 5}, {100, 15}, 0, 1.3, 0, 3.0};
	sliders[1] = {{(int)buffer_width-100-5, 25}, {100, 15}, 0, 1500000, 10000, 2000000};
	sliders[2] = {{(int)buffer_width-100-5, 45}, {100, 15}, 0, 20, 20, 180};
	sliders[3] = {{(int)buffer_width-100-5, 65}, {100, 15}, 0, 80, 1, 300};
	sliders[4] = {{(int)buffer_width-100-5, 85}, {100, 15}, 0, 0, 0, 12000};
	uint slider_count = 5;

	//-----------END INIT-----------

	while(1){
		MSG msg = {};
		while(PeekMessage(&msg, window, 0, 0, PM_REMOVE)){
		    TranslateMessage(&msg);
		    DispatchMessage(&msg);
		}

		//Clear window
		for(uint i=0; i < buffer_width*buffer_height; ++i) memory[i] = 0;

		timer.start();

		clearCount();

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
			threads2.push_back(std::thread(UpdateFluid, particles, 0.003, thread_inc*i, thread_inc*(i+1)));
		}
		for(auto& i : threads2){
			i.join();
		}

//		UpdateFluid(particles, 0.006, 0, numParticles);

		std::cout << "Simulationszeit: " << timer.measure_ms() << std::endl;

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

		for(uint y=1; y < HASHGRIDY; ++y){
			for(uint x=0; x < buffer_width; ++x) memory[(y*buffer_height/HASHGRIDY)*buffer_width+x] = 0x202020;
		}
		for(uint x=1; x < HASHGRIDX; ++x){
			for(uint y=0; y < buffer_height; ++y) memory[(y)*buffer_width+(x*buffer_width/HASHGRIDX)] = 0x202020;
		}

		//Update sliders
		updateSliders(sliders, slider_count);
		TARGET_DENSITY = sliders[0].val;
		PRESSURE_MULTIPLIER = sliders[1].val;
		RADIUS = sliders[2].val;
		MASS = sliders[3].val;
		GRAVITY = sliders[4].val;

		timer.start();

		for(uint i=0; i < numParticles; ++i){
			Particle& p = particles[i];
			int x = p.pos.x;
			int y = p.pos.y;
			drawCircle(x, y, 3, 0x0040FF);
			if(i==50) drawCircle(x, y, 3, 0xFF0000);
			if(p.state == 1) drawCircle(x, y, 3, 0x0070FF);
		}

		HDC hdc = GetDC(window);
		bitmapInfo.bmiHeader.biWidth = buffer_width;
		bitmapInfo.bmiHeader.biHeight = -buffer_height;
		StretchDIBits(hdc, 0, 0, window_width, window_height, 0, 0, buffer_width, buffer_height, memory, &bitmapInfo, DIB_RGB_COLORS, SRCCOPY);
		ReleaseDC(window, hdc);

		std::cout << "Renderzeit: " << timer.measure_ms() << std::endl;

	}

	delete[] particles;

}

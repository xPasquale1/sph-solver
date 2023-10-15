#include <windows.h>
#include <windowsx.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <unistd.h>
#include <cmath>
#include "util.h"

#define PI 3.14159265359
typedef unsigned int uint;

struct ivec2{
	int x;
	int y;
};

struct mouse{
	ivec2 pos;
	bool lmb = false;
	bool rmb = false;
} mouse;

uint window_width = 800;		//Größe des Fensters
uint window_height = 800;
uint pixel_size = 2;
uint buffer_width = window_width/pixel_size;
uint buffer_height = window_height/pixel_size;
uint* memory = nullptr;			//Pointer zum pixel-array

struct Particle{
	vec2 pos = {0};
	vec2 vel = {0};
	vec2 force = {0};
	float radius = 1.0;
	float mass = 1;
	float pressure = 0;
	float density = 0;
	vec2 predicted_pos = {0};
};

uint numParticles = 400;
#define ADDITIONAL_PARTICLES 400
const uint MAX_PARTICLES = ADDITIONAL_PARTICLES+numParticles;
Particle* particles = new Particle[numParticles+ADDITIONAL_PARTICLES];

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
	case WM_MOUSEMOVE:{
		mouse.pos.x = GET_X_LPARAM(lParam);
		mouse.pos.y = GET_Y_LPARAM(lParam);
		break;
	}
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

#define DAMPING 0.5
void Integrate(double dt){
	for(uint i=0; i < numParticles; ++i){
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
		else if(p.pos.y >= buffer_height-1){
			p.pos.y = buffer_height-4;
			p.vel.y = p.vel.y*DAMPING;
		}
		if(p.pos.x < 0){
			p.pos.x = 0;
			p.vel.x = p.vel.x*DAMPING;
		}
		else if(p.pos.x >= buffer_width){
			p.pos.x = buffer_width-1;
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
void computeDensity(void){
	for(uint i=0; i < numParticles; ++i){

		Particle& p = particles[i];
		p.density = 1;

		for(uint j=0; j < numParticles; ++j){
			if(i==j) continue;
			float dist = length(p.pos, particles[j].pos);
			float strength = smoothingKernel(p.radius, dist);
			p.density += p.mass * strength;
		}
	}
}
float computeDensityPoint(vec2& point, float radius, float mass){

	float density = 1;

	for(uint j=0; j < numParticles; ++j){
		float dist = length(point, particles[j].pos);
		float strength = smoothingKernel(radius, dist);
		density += mass * strength;
	}
	return density;
}

void computePressure(){
	for(uint i=0; i < numParticles; ++i){

		Particle& p = particles[i];
		p.pressure = 0;

		for(uint j=0; j < numParticles; ++j){
			if(i==j) continue;
			float dist = length(p.pos, particles[j].pos);
			float strength = smoothingKernel(p.radius, dist);
			p.pressure += particles[j].density * p.mass * strength / p.density;
		}
	}
}

static float PRESSURE_MULTIPLIER = 5000;
static float TARGET_DENSITY = 1.17;
inline float densityError(float density){
	float err = density - TARGET_DENSITY;
	return err * PRESSURE_MULTIPLIER;
}
inline float sharedPressure(float densityA, float densityB){
	float pressureA = densityError(densityA);
	float pressureB = densityError(densityB);
	return (pressureA+pressureB)/2.f;
}
void computeForces(void){
	for(uint i=0; i < numParticles; ++i){

		Particle& p = particles[i];
		p.force = {0, 0};

		for(uint j=0; j < numParticles; ++j){
			if(i==j) continue;
			float dist = length(p.pos, particles[j].pos);
			vec2 dir;
			if(dist == 0) dir = {1, 0};
			else dir = {(p.pos.x - particles[j].pos.x)/dist, (p.pos.y - particles[j].pos.y)/dist};
			float strength = smoothingKernelDerivative(p.radius, dist);
			float pressure = sharedPressure(p.density, particles[j].density);
			p.force.x -= pressure*dir.x*p.mass*strength/p.density;
			p.force.y -= pressure*dir.y*p.mass*strength/p.density;
		}
//		p.force.y += 2;
	}
}

void UpdateFluid(Particle* particles, double dt){
    computeDensity();
    computeForces();
    Integrate(dt);
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
		float x = rand()%buffer_width;
		float y = rand()%buffer_height;
		particles[i] = {{x, y}, {0}, {0}, 50, 80, 0, 0};
	}
	//-----------END INIT-----------
	while(1){
		MSG msg = {};
		while(PeekMessage(&msg, window, 0, 0, PM_REMOVE)){
		    TranslateMessage(&msg);
		    DispatchMessage(&msg);
		}

		timer.start();

		if(mouse.lmb){
			mouse.lmb = false;
		}

		UpdateFluid(particles, 0.01);

		//Clear window
		for(uint i=0; i < buffer_width*buffer_height; ++i){
			memory[i] = 0;
		}
//#define VISUALIZE_DENSITY
#ifdef VISUALIZE_DENSITY
		float buffer[buffer_width*buffer_height];
		float max = 0;
		float min = 99999999999;
		float avg = 0; float avg_count = 0;
		for(uint y=0; y < buffer_height; y+=2){
			for(uint x=0; x < buffer_width; x+=2){
				vec2 pt = {(float)x, (float)y};
				float val = computeDensityPoint(pt, particles[0].radius, particles[0].mass);
				buffer[y*buffer_width+x] = val;
				avg += val; ++avg_count;
				if(val > max) max = val;
				if(val < min) min = val;
			}
		}
		std::cout << min << ", " << max << ", " << avg/avg_count << std::endl;
		for(uint i=0; i < buffer_width*buffer_height; ++i){
			uint col = 0;
			unsigned char val = (buffer[i]-1)/(max-1)*255;
			if(buffer[i] < TARGET_DENSITY) memory[i] = col | val;
			else memory[i] = col | val<<8;
		}
#endif

		for(uint i=0; i < numParticles; ++i){
			Particle& p = particles[i];
			int x = p.pos.x;
			int y = p.pos.y;
			if(x >= 0 && x < (int)buffer_width && y >= 0 && y < (int)buffer_height){
				memory[y*buffer_width+x] = 0xFFFFFF;
			}
		}

		HDC hdc = GetDC(window);
		bitmapInfo.bmiHeader.biWidth = buffer_width;
		bitmapInfo.bmiHeader.biHeight = -buffer_height;
		StretchDIBits(hdc, 0, 0, window_width, window_height, 0, 0, buffer_width, buffer_height, memory, &bitmapInfo, DIB_RGB_COLORS, SRCCOPY);
		ReleaseDC(window, hdc);

//		std::cout << timer.measure_ms() << std::endl;

	}

}

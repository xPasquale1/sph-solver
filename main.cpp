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

uint window_width = 1200;		//Größe des Fensters
uint window_height = 800;
uint pixel_size = 2;
uint buffer_width = window_width/pixel_size;
uint buffer_height = window_height/pixel_size;
uint* memory = nullptr;			//Pointer zum pixel-array

inline __attribute__((always_inline)) void setPixel(int x, int y, uint color){
	if(x < 0 || x >= (int)buffer_width || y < 0 || y >= (int)buffer_height) return;
	memory[y*buffer_width+x] = color;
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
			std::cout << s.val << std::endl;
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
	float density = 0;
	vec2 predicted_pos = {0};
};

uint numParticles = 600;
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

#define DAMPING -0.5
#define BOUND_X buffer_height
#define BOUND_Y buffer_height
void Integrate(double dt){
	for(uint i=0; i < numParticles; ++i){
		//Update particle
		Particle& p = particles[i];
		p.vel.x += p.force.x/p.density*dt*0.9;
		p.vel.y += p.force.y/p.density*dt*0.9;
		p.pos.x += p.vel.x*dt;
		p.pos.y += p.vel.y*dt;
		p.predicted_pos.x = p.pos.x+p.vel.x*dt;
		p.predicted_pos.y = p.pos.y+p.vel.y*dt;

		if(p.pos.y < 0){
			p.pos.y = 0;
			p.vel.y = p.vel.y*DAMPING;
		}
		else if(p.pos.y >= BOUND_Y-1){
			p.pos.y = BOUND_Y-1;
			p.vel.y = p.vel.y*DAMPING;
		}
		if(p.pos.x < 0){
			p.pos.x = 0;
			p.vel.x = p.vel.x*DAMPING;
		}
		else if(p.pos.x >= BOUND_X-1){
			p.pos.x = BOUND_X-1;
			p.vel.x = p.vel.x*DAMPING;
		}
	}
}

static float RADIUS = 50;
static float MASS = 80;
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
			float dist = length(p.predicted_pos, particles[j].predicted_pos);
			float strength = smoothingKernel(RADIUS, dist);
			p.density += MASS * strength;
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
			float dist = length(p.predicted_pos, particles[j].predicted_pos);
			float strength = smoothingKernel(RADIUS, dist);
			p.pressure += particles[j].density * MASS * strength / p.density;
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
void computeForces(void){
	for(uint i=0; i < numParticles; ++i){

		Particle& p = particles[i];
		p.force = {0, 0};

		for(uint j=0; j < numParticles; ++j){
			if(i==j) continue;
			float dist = length(p.pos, particles[j].pos);
			vec2 dir;
			if(dist == 0){
				float val = (nextrand()%2000001)/1000000-1;
				dir = {(float)cos(val), (float)sin(val)};
			}
			else dir = {(p.pos.x - particles[j].pos.x)/dist, (p.pos.y - particles[j].pos.y)/dist};
			float strength = smoothingKernelDerivative(RADIUS, dist);
			float pressure = sharedPressure(p.density, particles[j].density);
			p.force.x -= pressure*dir.x*MASS*strength/p.density;
			p.force.y -= pressure*dir.y*MASS*strength/p.density;
		}
		p.force.y += GRAVITY;
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

	//Init sliders
	Slider sliders[5];
	sliders[0] = {{(int)buffer_width-100-5, 5}, {100, 15}, 0, 2, 1, 3.0};
	sliders[1] = {{(int)buffer_width-100-5, 25}, {100, 15}, 0, 1000000, 10000, 1000000};
	sliders[2] = {{(int)buffer_width-100-5, 45}, {100, 15}, 0, 35, 30, 180};
	sliders[3] = {{(int)buffer_width-100-5, 65}, {100, 15}, 0, 220, 40, 300};
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
		for(uint i=0; i < buffer_width*buffer_height; ++i){
			memory[i] = 0;
		}

		timer.start();

		if(mouse.rmb){
			for(uint i=0; i < numParticles; ++i){
				Particle& p = particles[i];
				vec2 mouse_pos = {(float)mouse.pos.x, (float)mouse.pos.y};
				float r = length(p.pos, mouse_pos);
				if(r < RADIUS){
					if(r == 0) r = 0.000001;
					vec2 dir = {(p.pos.x - mouse_pos.x)/r, (p.pos.y - mouse_pos.y)/r};
					p.vel.x -= 140*dir.x;
					p.vel.y -= 140*dir.y;
				}
			}
		}

		UpdateFluid(particles, 0.008);

//#define VISUALIZE_DENSITY
#ifdef VISUALIZE_DENSITY
		float buffer[buffer_width*buffer_height];
		float max = 0;
		float min = 99999999999;
		float avg = 0; float avg_count = 0;
		for(uint y=0; y < buffer_height; ++y){
			for(uint x=0; x < buffer_width; ++x){
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

		//Update sliders
		updateSliders(sliders, slider_count);
		TARGET_DENSITY = sliders[0].val;
		PRESSURE_MULTIPLIER = sliders[1].val;
		RADIUS = sliders[2].val;
		MASS = sliders[3].val;
		GRAVITY = sliders[4].val;

		for(uint i=0; i < numParticles; ++i){
			Particle& p = particles[i];
			int x = p.pos.x;
			int y = p.pos.y;
			setPixel(x, y, 0x0040FF);
			setPixel(x+1, y, 0x0040FF);
			setPixel(x, y+1, 0x0040FF);
			setPixel(x-1, y, 0x0040FF);
			setPixel(x, y-1, 0x0040FF);
		}

		HDC hdc = GetDC(window);
		bitmapInfo.bmiHeader.biWidth = buffer_width;
		bitmapInfo.bmiHeader.biHeight = -buffer_height;
		StretchDIBits(hdc, 0, 0, window_width, window_height, 0, 0, buffer_width, buffer_height, memory, &bitmapInfo, DIB_RGB_COLORS, SRCCOPY);
		ReleaseDC(window, hdc);

//		std::cout << timer.measure_ms() << std::endl;

	}

}

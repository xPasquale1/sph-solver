#pragma once

#include <GL/gl.h>
#include <GL/glu.h>
#include "util.h"

struct vertex{
	vec3 pos;
	vec3 color = {1.f, 1.f, 1.f};
	vec2 tex_coord;
};

struct triangle{
	vertex point[3];
};

struct point{
	vec3 pos;
	vec3 color = {1.f, 1.f, 1.f};
};

struct triangle2D{
	point point[3];
};

struct camera{
	float focal_length;
	vec3 pos;
	vec2 rot;	//Yaw, pitch. rot.x ist die Rotation um die Y-Achse weil... uhh ja
}; static camera cam;

typedef LRESULT (*window_callback_function)(HWND, UINT, WPARAM, LPARAM);
HWND CreateOpenGLWindow(const char* title, int x, int y, int width, int height, BYTE type, DWORD flags, window_callback_function callback){
    int         pf;
    HDC         hDC;
    HWND        hWnd;
    WNDCLASS    wc;
    PIXELFORMATDESCRIPTOR pfd = {};
    static HINSTANCE hInstance = 0;
    if(!hInstance){
		hInstance = GetModuleHandle(NULL);
		wc.style         = CS_OWNDC;
		wc.lpfnWndProc   = callback;
		wc.cbClsExtra    = 0;
		wc.cbWndExtra    = 0;
		wc.hInstance     = hInstance;
		wc.hIcon         = LoadIcon(NULL, IDI_WINLOGO);
		wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = NULL;
		wc.lpszMenuName  = NULL;
		wc.lpszClassName = "OpenGL";
		if(!RegisterClass(&wc)){
			MessageBox(NULL, "RegisterClass() failed: Cannot register window class.", "Error", MB_OK);
			return NULL;
		}
    }
    hWnd = CreateWindow("OpenGL", title, WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, x, y, width, height, NULL, NULL, hInstance, NULL);
    if(hWnd == NULL){
    	MessageBox(NULL, "CreateWindow() failed:  Cannot create a window.", "Error", MB_OK);
		return NULL;
    }
    hDC = GetDC(hWnd);
    pfd.nSize        = sizeof(pfd);
    pfd.nVersion     = 1;
    pfd.dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | flags;
    pfd.iPixelType   = type;
    pfd.cColorBits   = 32;
    pf = ChoosePixelFormat(hDC, &pfd);
    if(pf == 0){
    	MessageBox(NULL, "ChoosePixelFormat() failed: Cannot find a suitable pixel format.", "Error", MB_OK);
    	return 0;
    }
    if(SetPixelFormat(hDC, pf, &pfd) == FALSE){
    	MessageBox(NULL, "SetPixelFormat() failed: Cannot set format specified.", "Error", MB_OK);
    	return 0;
    }
    DescribePixelFormat(hDC, pf, sizeof(PIXELFORMATDESCRIPTOR), &pfd);
    ReleaseDC(hWnd, hDC);
    return hWnd;
}

inline void display(triangle* triangles, uint triangle_count, triangle2D* triangles2D, uint triangle2D_count){
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPopMatrix();
	glRotatef(radtodeg(cam.rot.y), 1, 0, 0);
	cam.rot.y = 0;
	glPushMatrix();
	glRotatef(radtodeg(cam.rot.x), 0, 1, 0);
    glBegin(GL_TRIANGLES);

    for(uint i=0; i < triangle_count; ++i){
    	triangle tri = triangles[i];
    	for(uint j=0; j < 3; ++j){
    		tri.point[j].pos.x -= cam.pos.x;
    		tri.point[j].pos.y -= cam.pos.y;
    		tri.point[j].pos.z -= cam.pos.z;

			glColor4f(tri.point[j].color.x, tri.point[j].color.y, tri.point[j].color.z, 1.f);
			glTexCoord2f(tri.point[j].tex_coord.x, tri.point[j].tex_coord.y);
			glVertex3f(tri.point[j].pos.x, tri.point[j].pos.y, tri.point[j].pos.z);
    	}
    }

    glPopMatrix();
    glLoadIdentity();
    for(uint i=0; i < triangle2D_count; ++i){
    	triangle2D tri = triangles2D[i];
    	for(uint j=0; j < 3; ++j){
			glColor4f(tri.point[j].color.x, tri.point[j].color.y, tri.point[j].color.z, 1.f);
			glVertex3f(tri.point[j].pos.x, tri.point[j].pos.y, tri.point[j].pos.z);
    	}
    }

    glEnd();
    glFlush();
    return;
}

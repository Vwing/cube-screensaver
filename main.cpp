#include <windows.h>
#include <scrnsave.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <vector>
#include <cmath>
#include <ctime>
#include <cstdlib>

#pragma comment(lib, "scrnsave.lib")
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

struct Cube {
    float x, y, z;
    float vx, vy, vz;
    float rotation;
    float rotationSpeed;
    COLORREF color;
    bool celebratingCorner;
    int celebrationTimer;
};

struct Monitor {
    RECT bounds;
    HDC hdc;
    HGLRC hglrc;
    Cube cube;
};

std::vector<Monitor> monitors;
const float CUBE_SIZE = 0.1f;
const float SPEED_MULTIPLIER = 0.5f;
const int CELEBRATION_DURATION = 60;

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
    Monitor mon;
    mon.bounds = *lprcMonitor;
    
    float width = static_cast<float>(lprcMonitor->right - lprcMonitor->left);
    float height = static_cast<float>(lprcMonitor->bottom - lprcMonitor->top);
    
    mon.cube.x = (static_cast<float>(rand()) / RAND_MAX) * 0.6f - 0.3f;
    mon.cube.y = (static_cast<float>(rand()) / RAND_MAX) * 0.6f - 0.3f;
    mon.cube.z = -2.0f;
    
    float angle = (static_cast<float>(rand()) / RAND_MAX) * 2.0f * 3.14159f;
    float speed = 0.01f + (static_cast<float>(rand()) / RAND_MAX) * 0.02f;
    mon.cube.vx = cos(angle) * speed * SPEED_MULTIPLIER;
    mon.cube.vy = sin(angle) * speed * SPEED_MULTIPLIER;
    mon.cube.vz = 0;
    
    mon.cube.rotation = 0;
    mon.cube.rotationSpeed = 1.0f + (static_cast<float>(rand()) / RAND_MAX) * 2.0f;
    
    mon.cube.color = RGB(rand() % 128 + 128, rand() % 128 + 128, rand() % 128 + 128);
    mon.cube.celebratingCorner = false;
    mon.cube.celebrationTimer = 0;
    
    monitors.push_back(mon);
    return TRUE;
}

void InitOpenGL(HWND hwnd, Monitor& mon) {
    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR), 1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 24, 8, 0,
        PFD_MAIN_PLANE, 0, 0, 0, 0
    };
    
    mon.hdc = GetDC(hwnd);
    int pixelFormat = ChoosePixelFormat(mon.hdc, &pfd);
    SetPixelFormat(mon.hdc, pixelFormat, &pfd);
    mon.hglrc = wglCreateContext(mon.hdc);
    wglMakeCurrent(mon.hdc, mon.hglrc);
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    
    float lightPos[] = {0.0f, 0.0f, 1.0f, 0.0f};
    float lightAmb[] = {0.2f, 0.2f, 0.2f, 1.0f};
    float lightDiff[] = {0.8f, 0.8f, 0.8f, 1.0f};
    
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiff);
}

void DrawCube(const Cube& cube) {
    glPushMatrix();
    glTranslatef(cube.x, cube.y, cube.z);
    glRotatef(cube.rotation, 1.0f, 1.0f, 0.0f);
    
    float r = GetRValue(cube.color) / 255.0f;
    float g = GetGValue(cube.color) / 255.0f;
    float b = GetBValue(cube.color) / 255.0f;
    
    if (cube.celebratingCorner) {
        float pulse = (sin(cube.celebrationTimer * 0.3f) + 1.0f) / 2.0f;
        r = r * 0.5f + pulse * 0.5f;
        g = g * 0.5f + pulse * 0.5f;
        b = b * 0.5f + pulse * 0.5f;
        glScalef(1.0f + pulse * 0.2f, 1.0f + pulse * 0.2f, 1.0f + pulse * 0.2f);
    }
    
    glColor3f(r, g, b);
    
    glBegin(GL_QUADS);
    // Front face
    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(-CUBE_SIZE, -CUBE_SIZE, CUBE_SIZE);
    glVertex3f(CUBE_SIZE, -CUBE_SIZE, CUBE_SIZE);
    glVertex3f(CUBE_SIZE, CUBE_SIZE, CUBE_SIZE);
    glVertex3f(-CUBE_SIZE, CUBE_SIZE, CUBE_SIZE);
    
    // Back face
    glNormal3f(0.0f, 0.0f, -1.0f);
    glVertex3f(-CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE);
    glVertex3f(-CUBE_SIZE, CUBE_SIZE, -CUBE_SIZE);
    glVertex3f(CUBE_SIZE, CUBE_SIZE, -CUBE_SIZE);
    glVertex3f(CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE);
    
    // Top face
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(-CUBE_SIZE, CUBE_SIZE, -CUBE_SIZE);
    glVertex3f(-CUBE_SIZE, CUBE_SIZE, CUBE_SIZE);
    glVertex3f(CUBE_SIZE, CUBE_SIZE, CUBE_SIZE);
    glVertex3f(CUBE_SIZE, CUBE_SIZE, -CUBE_SIZE);
    
    // Bottom face
    glNormal3f(0.0f, -1.0f, 0.0f);
    glVertex3f(-CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE);
    glVertex3f(CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE);
    glVertex3f(CUBE_SIZE, -CUBE_SIZE, CUBE_SIZE);
    glVertex3f(-CUBE_SIZE, -CUBE_SIZE, CUBE_SIZE);
    
    // Right face
    glNormal3f(1.0f, 0.0f, 0.0f);
    glVertex3f(CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE);
    glVertex3f(CUBE_SIZE, CUBE_SIZE, -CUBE_SIZE);
    glVertex3f(CUBE_SIZE, CUBE_SIZE, CUBE_SIZE);
    glVertex3f(CUBE_SIZE, -CUBE_SIZE, CUBE_SIZE);
    
    // Left face
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glVertex3f(-CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE);
    glVertex3f(-CUBE_SIZE, -CUBE_SIZE, CUBE_SIZE);
    glVertex3f(-CUBE_SIZE, CUBE_SIZE, CUBE_SIZE);
    glVertex3f(-CUBE_SIZE, CUBE_SIZE, -CUBE_SIZE);
    glEnd();
    
    glPopMatrix();
}

void UpdateCube(Cube& cube) {
    cube.x += cube.vx;
    cube.y += cube.vy;
    
    cube.rotation += cube.rotationSpeed;
    if (cube.rotation > 360.0f) cube.rotation -= 360.0f;
    
    bool hitCorner = false;
    const float CORNER_THRESHOLD = 0.05f;
    
    if (cube.x - CUBE_SIZE <= -1.0f) {
        cube.x = -1.0f + CUBE_SIZE;
        cube.vx = -cube.vx;
        if (abs(cube.y - CUBE_SIZE - (-1.0f)) < CORNER_THRESHOLD || 
            abs(cube.y + CUBE_SIZE - 1.0f) < CORNER_THRESHOLD) {
            hitCorner = true;
        }
    } else if (cube.x + CUBE_SIZE >= 1.0f) {
        cube.x = 1.0f - CUBE_SIZE;
        cube.vx = -cube.vx;
        if (abs(cube.y - CUBE_SIZE - (-1.0f)) < CORNER_THRESHOLD || 
            abs(cube.y + CUBE_SIZE - 1.0f) < CORNER_THRESHOLD) {
            hitCorner = true;
        }
    }
    
    if (cube.y - CUBE_SIZE <= -1.0f) {
        cube.y = -1.0f + CUBE_SIZE;
        cube.vy = -cube.vy;
        if (abs(cube.x - CUBE_SIZE - (-1.0f)) < CORNER_THRESHOLD || 
            abs(cube.x + CUBE_SIZE - 1.0f) < CORNER_THRESHOLD) {
            hitCorner = true;
        }
    } else if (cube.y + CUBE_SIZE >= 1.0f) {
        cube.y = 1.0f - CUBE_SIZE;
        cube.vy = -cube.vy;
        if (abs(cube.x - CUBE_SIZE - (-1.0f)) < CORNER_THRESHOLD || 
            abs(cube.x + CUBE_SIZE - 1.0f) < CORNER_THRESHOLD) {
            hitCorner = true;
        }
    }
    
    if (hitCorner && !cube.celebratingCorner) {
        cube.celebratingCorner = true;
        cube.celebrationTimer = CELEBRATION_DURATION;
    }
    
    if (cube.celebratingCorner) {
        cube.celebrationTimer--;
        if (cube.celebrationTimer <= 0) {
            cube.celebratingCorner = false;
        }
    }
}

void RenderScene(Monitor& mon) {
    wglMakeCurrent(mon.hdc, mon.hglrc);
    
    glViewport(0, 0, mon.bounds.right - mon.bounds.left, mon.bounds.bottom - mon.bounds.top);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)(mon.bounds.right - mon.bounds.left) / (mon.bounds.bottom - mon.bounds.top), 0.1, 100.0);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    UpdateCube(mon.cube);
    DrawCube(mon.cube);
    
    SwapBuffers(mon.hdc);
}

LRESULT WINAPI ScreenSaverProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    static UINT_PTR timer;
    
    switch (message) {
    case WM_CREATE:
        srand(static_cast<unsigned>(time(nullptr)));
        EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);
        
        for (auto& mon : monitors) {
            InitOpenGL(hwnd, mon);
        }
        
        timer = SetTimer(hwnd, 1, 16, NULL);
        return 0;
        
    case WM_TIMER:
        for (auto& mon : monitors) {
            RenderScene(mon);
        }
        return 0;
        
    case WM_DESTROY:
        KillTimer(hwnd, timer);
        for (auto& mon : monitors) {
            wglMakeCurrent(NULL, NULL);
            wglDeleteContext(mon.hglrc);
            ReleaseDC(hwnd, mon.hdc);
        }
        monitors.clear();
        PostQuitMessage(0);
        return 0;
    }
    
    return DefScreenSaverProc(hwnd, message, wParam, lParam);
}

BOOL WINAPI ScreenSaverConfigureDialog(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    return FALSE;
}

BOOL WINAPI RegisterDialogClasses(HANDLE hInst) {
    return TRUE;
}
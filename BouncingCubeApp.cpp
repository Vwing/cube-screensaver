#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <vector>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <string>
#include <iostream>

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

#define REGISTRY_KEY "Software\\BouncingCubeScreensaver"

struct Cube {
    float x, y, z;  // Screen space coordinates in pixels
    float vx, vy, vz;  // Velocity in pixels per frame
    float rotationMatrix[16];  // 4x4 rotation matrix to preserve orientation
    float rotationAxisX, rotationAxisY, rotationAxisZ;  // Current rotation axis
    float rotationSpeed;  // Angular velocity
    COLORREF color;
    bool celebratingCorner;
    int celebrationTimer;
    bool active;  // Whether this cube is currently visible
};

struct Monitor {
    RECT bounds;
    HDC hdc;
    HGLRC hglrc;
    HWND hwnd;
};

// Global cube that moves between monitors
Cube globalCube;

std::vector<Monitor> monitors;
float g_CubeSize = 0.1f;  // Default cube scale for 3D rendering
bool g_EnableCelebration = false;  // Default celebration setting
bool g_MirrorMode = true;  // Default mirror mode enabled
const float SPEED_MULTIPLIER = 1.0f;
const int CELEBRATION_DURATION = 60;

// Command line arguments
bool g_PreviewMode = false;
HWND g_PreviewHWND = NULL;
HANDLE g_ExitEvent = NULL;
bool g_StandaloneMode = false;

void MultiplyMatrix4x4(float result[16], const float a[16], const float b[16]) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            result[i * 4 + j] = 0;
            for (int k = 0; k < 4; k++) {
                result[i * 4 + j] += a[i * 4 + k] * b[k * 4 + j];
            }
        }
    }
}

void CreateRotationMatrix(float matrix[16], float angle, float x, float y, float z) {
    float c = cos(angle * 3.14159f / 180.0f);
    float s = sin(angle * 3.14159f / 180.0f);
    float ic = 1.0f - c;
    
    matrix[0] = c + x*x*ic;     matrix[1] = x*y*ic - z*s;   matrix[2] = x*z*ic + y*s;   matrix[3] = 0;
    matrix[4] = y*x*ic + z*s;   matrix[5] = c + y*y*ic;     matrix[6] = y*z*ic - x*s;   matrix[7] = 0;
    matrix[8] = z*x*ic - y*s;   matrix[9] = z*y*ic + x*s;   matrix[10] = c + z*z*ic;    matrix[11] = 0;
    matrix[12] = 0;             matrix[13] = 0;             matrix[14] = 0;             matrix[15] = 1;
}

void LoadSettings() {
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, REGISTRY_KEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD dwSize = sizeof(float);
        RegQueryValueEx(hKey, "CubeSize", NULL, NULL, (LPBYTE)&g_CubeSize, &dwSize);
        
        DWORD dwCelebration = 0;
        DWORD dwCelebrationSize = sizeof(DWORD);
        if (RegQueryValueEx(hKey, "EnableCelebration", NULL, NULL, (LPBYTE)&dwCelebration, &dwCelebrationSize) == ERROR_SUCCESS) {
            g_EnableCelebration = (dwCelebration != 0);
        }
        
        DWORD dwMirrorMode = 1;  // Default to true
        DWORD dwMirrorModeSize = sizeof(DWORD);
        if (RegQueryValueEx(hKey, "MirrorMode", NULL, NULL, (LPBYTE)&dwMirrorMode, &dwMirrorModeSize) == ERROR_SUCCESS) {
            g_MirrorMode = (dwMirrorMode != 0);
        }
        
        RegCloseKey(hKey);
    }
}

float GetCubeSizeInPixels() {
    return g_CubeSize * 500.0f;  // Scale factor to convert to reasonable pixel size
}

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
    MONITORINFO mi = { sizeof(MONITORINFO) };
    if (GetMonitorInfo(hMonitor, &mi)) {
        Monitor mon;
        mon.bounds = mi.rcMonitor;  // Use full monitor bounds instead of work area
        mon.hwnd = NULL;
        mon.hdc = NULL;
        mon.hglrc = NULL;
        
        monitors.push_back(mon);
    }
    return TRUE;
}

void InitializeCube() {
    // Get total desktop bounds
    RECT physicsBounds = {0};
    for (const auto& mon : monitors) {
        if (mon.bounds.left < physicsBounds.left) physicsBounds.left = mon.bounds.left;
        if (mon.bounds.top < physicsBounds.top) physicsBounds.top = mon.bounds.top;
        if (mon.bounds.right > physicsBounds.right) physicsBounds.right = mon.bounds.right;
        if (mon.bounds.bottom > physicsBounds.bottom) physicsBounds.bottom = mon.bounds.bottom;
    }
    
    // Start cube in center of primary monitor
    if (!monitors.empty()) {
        POINT origin = {0, 0};
        HMONITOR hPrimary = MonitorFromPoint(origin, MONITOR_DEFAULTTOPRIMARY);
        
        const Monitor* primary = &monitors[0];  // fallback
        for (const auto& mon : monitors) {
            MONITORINFO mi = { sizeof(MONITORINFO) };
            HMONITOR hMon = MonitorFromRect(&mon.bounds, MONITOR_DEFAULTTONEAREST);
            if (hMon == hPrimary) {
                primary = &mon;
                break;
            }
        }
        globalCube.x = (primary->bounds.left + primary->bounds.right) / 2.0f;
        globalCube.y = (primary->bounds.top + primary->bounds.bottom) / 2.0f;
        globalCube.z = 0.0f;
        
        float angle = (static_cast<float>(rand()) / RAND_MAX) * 2.0f * 3.14159f;
        float speed = 2.0f + (static_cast<float>(rand()) / RAND_MAX) * 3.0f;
        globalCube.vx = cos(angle) * speed * SPEED_MULTIPLIER;
        globalCube.vy = sin(angle) * speed * SPEED_MULTIPLIER;
        globalCube.vz = 0;
        
        // Initialize rotation matrix as identity
        for (int i = 0; i < 16; i++) globalCube.rotationMatrix[i] = 0.0f;
        globalCube.rotationMatrix[0] = globalCube.rotationMatrix[5] = globalCube.rotationMatrix[10] = globalCube.rotationMatrix[15] = 1.0f;
        
        // Random rotation axis (normalized)
        float axisX = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
        float axisY = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
        float axisZ = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
        float axisLength = sqrt(axisX * axisX + axisY * axisY + axisZ * axisZ);
        globalCube.rotationAxisX = axisX / axisLength;
        globalCube.rotationAxisY = axisY / axisLength;
        globalCube.rotationAxisZ = axisZ / axisLength;
        globalCube.rotationSpeed = ((rand() % 2 == 0) ? 1 : -1) * (0.5f + (static_cast<float>(rand()) / RAND_MAX) * 2.0f);
        globalCube.color = RGB(rand() % 128 + 128, rand() % 128 + 128, rand() % 128 + 128);
        globalCube.celebratingCorner = false;
        globalCube.celebrationTimer = 0;
        globalCube.active = true;
    }
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

void DrawCube(const Cube& cube, const Monitor& mon) {
    float aspect = (float)(mon.bounds.right - mon.bounds.left) / (mon.bounds.bottom - mon.bounds.top);
    
    float monitorWidth = (float)(mon.bounds.right - mon.bounds.left);
    float monitorHeight = (float)(mon.bounds.bottom - mon.bounds.top);
    float relPosX = (cube.x - mon.bounds.left) / monitorWidth;
    float relPosY = (cube.y - mon.bounds.top) / monitorHeight;
    
    float relX = (relPosX * 4.0f * aspect) - (2.0f * aspect);
    float relY = -((relPosY * 4.0f) - 2.0f);
    
    float cubeScale = g_CubeSize;
    
    glPushMatrix();
    glTranslatef(relX, relY, -5.0f);
    glMultMatrixf(cube.rotationMatrix);
    
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
    glVertex3f(-cubeScale, -cubeScale, cubeScale);
    glVertex3f(cubeScale, -cubeScale, cubeScale);
    glVertex3f(cubeScale, cubeScale, cubeScale);
    glVertex3f(-cubeScale, cubeScale, cubeScale);
    
    // Back face
    glNormal3f(0.0f, 0.0f, -1.0f);
    glVertex3f(-cubeScale, -cubeScale, -cubeScale);
    glVertex3f(-cubeScale, cubeScale, -cubeScale);
    glVertex3f(cubeScale, cubeScale, -cubeScale);
    glVertex3f(cubeScale, -cubeScale, -cubeScale);
    
    // Top face
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(-cubeScale, cubeScale, -cubeScale);
    glVertex3f(-cubeScale, cubeScale, cubeScale);
    glVertex3f(cubeScale, cubeScale, cubeScale);
    glVertex3f(cubeScale, cubeScale, -cubeScale);
    
    // Bottom face
    glNormal3f(0.0f, -1.0f, 0.0f);
    glVertex3f(-cubeScale, -cubeScale, -cubeScale);
    glVertex3f(cubeScale, -cubeScale, -cubeScale);
    glVertex3f(cubeScale, -cubeScale, cubeScale);
    glVertex3f(-cubeScale, -cubeScale, cubeScale);
    
    // Right face
    glNormal3f(1.0f, 0.0f, 0.0f);
    glVertex3f(cubeScale, -cubeScale, -cubeScale);
    glVertex3f(cubeScale, cubeScale, -cubeScale);
    glVertex3f(cubeScale, cubeScale, cubeScale);
    glVertex3f(cubeScale, -cubeScale, cubeScale);
    
    // Left face
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glVertex3f(-cubeScale, -cubeScale, -cubeScale);
    glVertex3f(-cubeScale, -cubeScale, cubeScale);
    glVertex3f(-cubeScale, cubeScale, cubeScale);
    glVertex3f(-cubeScale, cubeScale, -cubeScale);
    glEnd();
    
    glPopMatrix();
}

void UpdateCube() {
    if (!globalCube.active) return;
    
    globalCube.x += globalCube.vx;
    globalCube.y += globalCube.vy;
    
    float rotMatrix[16];
    CreateRotationMatrix(rotMatrix, globalCube.rotationSpeed, 
                        globalCube.rotationAxisX, globalCube.rotationAxisY, globalCube.rotationAxisZ);
    
    float newMatrix[16];
    MultiplyMatrix4x4(newMatrix, rotMatrix, globalCube.rotationMatrix);
    
    for (int i = 0; i < 16; i++) {
        globalCube.rotationMatrix[i] = newMatrix[i];
    }
    
    // Get bounds for physics (either primary monitor only or total desktop)
    RECT physicsBounds = {0};
    if (g_MirrorMode && !monitors.empty()) {
        POINT origin = {0, 0};
        HMONITOR hPrimary = MonitorFromPoint(origin, MONITOR_DEFAULTTOPRIMARY);
        
        for (const auto& mon : monitors) {
            MONITORINFO mi = { sizeof(MONITORINFO) };
            HMONITOR hMon = MonitorFromRect(&mon.bounds, MONITOR_DEFAULTTONEAREST);
            if (hMon == hPrimary) {
                physicsBounds = mon.bounds;
                break;
            }
        }
        
        if (physicsBounds.right == 0) {
            physicsBounds = monitors[0].bounds;
        }
    } else {
        for (const auto& mon : monitors) {
            if (mon.bounds.left < physicsBounds.left) physicsBounds.left = mon.bounds.left;
            if (mon.bounds.top < physicsBounds.top) physicsBounds.top = mon.bounds.top;
            if (mon.bounds.right > physicsBounds.right) physicsBounds.right = mon.bounds.right;
            if (mon.bounds.bottom > physicsBounds.bottom) physicsBounds.bottom = mon.bounds.bottom;
        }
    }
    
    bool hitCorner = false;
    const float CUBE_SIZE = GetCubeSizeInPixels();
    const float CORNER_THRESHOLD = CUBE_SIZE * 2;
    
    // Check boundaries - simplified version of original collision detection
    if (globalCube.x - CUBE_SIZE <= physicsBounds.left) {
        globalCube.x = physicsBounds.left + CUBE_SIZE;
        globalCube.vx = -globalCube.vx;
        
        float currentAngle = atan2(globalCube.vy, globalCube.vx);
        float randomOffset = (static_cast<float>(rand()) / RAND_MAX) * 0.052f;
        if (globalCube.vy > 0) randomOffset = -randomOffset;
        
        float speed = sqrt(globalCube.vx * globalCube.vx + globalCube.vy * globalCube.vy);
        float newAngle = currentAngle + randomOffset;
        globalCube.vx = cos(newAngle) * speed;
        globalCube.vy = sin(newAngle) * speed;
        
        float axisX = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
        float axisY = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
        float axisZ = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
        float axisLength = sqrt(axisX * axisX + axisY * axisY + axisZ * axisZ);
        globalCube.rotationAxisX = axisX / axisLength;
        globalCube.rotationAxisY = axisY / axisLength;
        globalCube.rotationAxisZ = axisZ / axisLength;
        globalCube.rotationSpeed = ((rand() % 2 == 0) ? 1 : -1) * (0.5f + (static_cast<float>(rand()) / RAND_MAX) * 3.0f);
        
        if (abs(globalCube.y - physicsBounds.top) < CORNER_THRESHOLD || 
            abs(globalCube.y - physicsBounds.bottom) < CORNER_THRESHOLD) {
            hitCorner = true;
        }
    } else if (globalCube.x + CUBE_SIZE >= physicsBounds.right) {
        globalCube.x = physicsBounds.right - CUBE_SIZE;
        globalCube.vx = -globalCube.vx;
        
        float currentAngle = atan2(globalCube.vy, globalCube.vx);
        float randomOffset = (static_cast<float>(rand()) / RAND_MAX) * 0.052f;
        if (globalCube.vy > 0) randomOffset = randomOffset;
        else randomOffset = -randomOffset;
        
        float speed = sqrt(globalCube.vx * globalCube.vx + globalCube.vy * globalCube.vy);
        float newAngle = currentAngle + randomOffset;
        globalCube.vx = cos(newAngle) * speed;
        globalCube.vy = sin(newAngle) * speed;
        
        float axisX = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
        float axisY = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
        float axisZ = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
        float axisLength = sqrt(axisX * axisX + axisY * axisY + axisZ * axisZ);
        globalCube.rotationAxisX = axisX / axisLength;
        globalCube.rotationAxisY = axisY / axisLength;
        globalCube.rotationAxisZ = axisZ / axisLength;
        globalCube.rotationSpeed = ((rand() % 2 == 0) ? 1 : -1) * (0.5f + (static_cast<float>(rand()) / RAND_MAX) * 3.0f);
        
        if (abs(globalCube.y - physicsBounds.top) < CORNER_THRESHOLD || 
            abs(globalCube.y - physicsBounds.bottom) < CORNER_THRESHOLD) {
            hitCorner = true;
        }
    }
    
    if (globalCube.y - CUBE_SIZE <= physicsBounds.top) {
        globalCube.y = physicsBounds.top + CUBE_SIZE;
        globalCube.vy = -globalCube.vy;
        
        float currentAngle = atan2(globalCube.vy, globalCube.vx);
        float randomOffset = (static_cast<float>(rand()) / RAND_MAX) * 0.052f;
        if (globalCube.vx > 0) randomOffset = randomOffset;
        else randomOffset = -randomOffset;
        
        float speed = sqrt(globalCube.vx * globalCube.vx + globalCube.vy * globalCube.vy);
        float newAngle = currentAngle + randomOffset;
        globalCube.vx = cos(newAngle) * speed;
        globalCube.vy = sin(newAngle) * speed;
        
        float axisX = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
        float axisY = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
        float axisZ = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
        float axisLength = sqrt(axisX * axisX + axisY * axisY + axisZ * axisZ);
        globalCube.rotationAxisX = axisX / axisLength;
        globalCube.rotationAxisY = axisY / axisLength;
        globalCube.rotationAxisZ = axisZ / axisLength;
        globalCube.rotationSpeed = ((rand() % 2 == 0) ? 1 : -1) * (0.5f + (static_cast<float>(rand()) / RAND_MAX) * 3.0f);
        
        if (abs(globalCube.x - physicsBounds.left) < CORNER_THRESHOLD || 
            abs(globalCube.x - physicsBounds.right) < CORNER_THRESHOLD) {
            hitCorner = true;
        }
    } else if (globalCube.y + CUBE_SIZE >= physicsBounds.bottom) {
        globalCube.y = physicsBounds.bottom - CUBE_SIZE;
        globalCube.vy = -globalCube.vy;
        
        float currentAngle = atan2(globalCube.vy, globalCube.vx);
        float randomOffset = (static_cast<float>(rand()) / RAND_MAX) * 0.052f;
        if (globalCube.vx > 0) randomOffset = -randomOffset;
        else randomOffset = randomOffset;
        
        float speed = sqrt(globalCube.vx * globalCube.vx + globalCube.vy * globalCube.vy);
        float newAngle = currentAngle + randomOffset;
        globalCube.vx = cos(newAngle) * speed;
        globalCube.vy = sin(newAngle) * speed;
        
        float axisX = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
        float axisY = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
        float axisZ = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
        float axisLength = sqrt(axisX * axisX + axisY * axisY + axisZ * axisZ);
        globalCube.rotationAxisX = axisX / axisLength;
        globalCube.rotationAxisY = axisY / axisLength;
        globalCube.rotationAxisZ = axisZ / axisLength;
        globalCube.rotationSpeed = ((rand() % 2 == 0) ? 1 : -1) * (0.5f + (static_cast<float>(rand()) / RAND_MAX) * 3.0f);
        
        if (abs(globalCube.x - physicsBounds.left) < CORNER_THRESHOLD || 
            abs(globalCube.x - physicsBounds.right) < CORNER_THRESHOLD) {
            hitCorner = true;
        }
    }
    
    if (hitCorner && !globalCube.celebratingCorner && g_EnableCelebration) {
        globalCube.celebratingCorner = true;
        globalCube.celebrationTimer = CELEBRATION_DURATION;
    }
    
    if (globalCube.celebratingCorner) {
        globalCube.celebrationTimer--;
        if (globalCube.celebrationTimer <= 0) {
            globalCube.celebratingCorner = false;
        }
    }
}

void RenderScene(Monitor& mon) {
    BOOL result = wglMakeCurrent(mon.hdc, mon.hglrc);
    if (!result) {
        if (mon.hwnd != NULL) {
            wglDeleteContext(mon.hglrc);
            InitOpenGL(mon.hwnd, mon);
            wglMakeCurrent(mon.hdc, mon.hglrc);
        }
        return;
    }
    
    glViewport(0, 0, mon.bounds.right - mon.bounds.left, mon.bounds.bottom - mon.bounds.top);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float aspect = (float)(mon.bounds.right - mon.bounds.left) / (mon.bounds.bottom - mon.bounds.top);
    gluPerspective(45.0, aspect, 0.1, 100.0);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    if (globalCube.active) {
        if (g_MirrorMode) {
            DrawCube(globalCube, mon);
        } else {
            const float CUBE_SIZE = GetCubeSizeInPixels();
            if (globalCube.x + CUBE_SIZE >= mon.bounds.left &&
                globalCube.x - CUBE_SIZE <= mon.bounds.right &&
                globalCube.y + CUBE_SIZE >= mon.bounds.top &&
                globalCube.y - CUBE_SIZE <= mon.bounds.bottom) {
                DrawCube(globalCube, mon);
            }
        }
    }
    
    SwapBuffers(mon.hdc);
}

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    static UINT_PTR timer;
    
    switch (message) {
    case WM_CREATE:
        {
            srand(static_cast<unsigned>(time(nullptr)) ^ GetCurrentProcessId());
            LoadSettings();
            monitors.clear();
            EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);
            InitializeCube();
            
            // Create fullscreen windows for each monitor
            for (auto& mon : monitors) {
                HWND monitorWnd = CreateWindowEx(
                    WS_EX_TOPMOST,
                    "BouncingCubeMonitor",
                    "BouncingCube",
                    WS_POPUP | WS_VISIBLE,
                    mon.bounds.left, mon.bounds.top,
                    mon.bounds.right - mon.bounds.left,
                    mon.bounds.bottom - mon.bounds.top,
                    hwnd, NULL, GetModuleHandle(NULL), NULL
                );
                
                mon.hwnd = monitorWnd;
                InitOpenGL(monitorWnd, mon);
            }
            
            timer = SetTimer(hwnd, 1, 16, NULL);
            return 0;
        }
        
    case WM_TIMER:
        UpdateCube();
        
        for (auto& mon : monitors) {
            if (mon.hglrc != NULL) {
                RenderScene(mon);
            }
        }
        return 0;
        
    case WM_KEYDOWN:
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MOUSEMOVE:
        if (!g_PreviewMode && !g_StandaloneMode) {
            PostQuitMessage(0);
        } else if (g_StandaloneMode && message == WM_KEYDOWN && wParam == VK_ESCAPE) {
            // In standalone mode, only exit on Escape key
            PostQuitMessage(0);
        }
        return 0;
        
    case WM_DESTROY:
        KillTimer(hwnd, timer);
        for (auto& mon : monitors) {
            if (mon.hwnd && mon.hwnd != hwnd) {
                DestroyWindow(mon.hwnd);
            }
            if (mon.hglrc) {
                wglMakeCurrent(NULL, NULL);
                wglDeleteContext(mon.hglrc);
                ReleaseDC(mon.hwnd, mon.hdc);
            }
        }
        PostQuitMessage(0);
        return 0;
    }
    
    return DefWindowProc(hwnd, message, wParam, lParam);
}

LRESULT CALLBACK MonitorWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_KEYDOWN:
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MOUSEMOVE:
        if (!g_PreviewMode && !g_StandaloneMode) {
            // Find the main window and send the message
            HWND parent = GetParent(hwnd);
            if (parent) {
                PostMessage(parent, message, wParam, lParam);
            }
        } else if (g_StandaloneMode && message == WM_KEYDOWN && wParam == VK_ESCAPE) {
            // In standalone mode, only exit on Escape key
            HWND parent = GetParent(hwnd);
            if (parent) {
                PostMessage(parent, message, wParam, lParam);
            }
        }
        return 0;
    }
    
    return DefWindowProc(hwnd, message, wParam, lParam);
}

void ParseCommandLine(LPWSTR cmdLine) {
    // Parse command line arguments
    // --preview --parentHWND <hwnd>
    // --monitors all --exitEvent <name>
    // --standalone (for debugging)
    
    std::wstring args(cmdLine);
    
    // Check for standalone mode (no arguments or --standalone)
    if (args.empty() || args.find(L"--standalone") != std::wstring::npos) {
        g_StandaloneMode = true;
        return;
    }
    
    if (args.find(L"--preview") != std::wstring::npos) {
        g_PreviewMode = true;
        
        size_t hwndPos = args.find(L"--parentHWND");
        if (hwndPos != std::wstring::npos) {
            hwndPos += 12; // length of "--parentHWND"
            while (hwndPos < args.length() && args[hwndPos] == L' ') hwndPos++;
            
            if (hwndPos < args.length()) {
                g_PreviewHWND = (HWND)(uintptr_t)wcstoul(args.c_str() + hwndPos, nullptr, 10);
            }
        }
    }
    
    size_t exitEventPos = args.find(L"--exitEvent");
    if (exitEventPos != std::wstring::npos) {
        exitEventPos += 11; // length of "--exitEvent"
        while (exitEventPos < args.length() && args[exitEventPos] == L' ') exitEventPos++;
        
        if (exitEventPos < args.length()) {
            std::wstring eventName = args.substr(exitEventPos);
            size_t spacePos = eventName.find(L' ');
            if (spacePos != std::wstring::npos) {
                eventName = eventName.substr(0, spacePos);
            }
            
            g_ExitEvent = OpenEventW(SYNCHRONIZE, FALSE, eventName.c_str());
        }
    }
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Convert to wide string
    LPWSTR cmdLineW = GetCommandLineW();
    ParseCommandLine(cmdLineW);
    
    // Register window classes
    WNDCLASS wc = {};
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "BouncingCubeApp";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    RegisterClass(&wc);
    
    wc.lpfnWndProc = MonitorWndProc;
    wc.lpszClassName = "BouncingCubeMonitor";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    RegisterClass(&wc);
    
    // Create main window (hidden)
    HWND mainWnd = CreateWindow(
        "BouncingCubeApp",
        "BouncingCube",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 0, 0,
        NULL, NULL, hInstance, NULL
    );
    
    if (!mainWnd) {
        return 1;
    }
    
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        
        // Check exit event if provided
        if (g_ExitEvent && WaitForSingleObject(g_ExitEvent, 0) == WAIT_OBJECT_0) {
            break;
        }
    }
    
    if (g_ExitEvent) {
        CloseHandle(g_ExitEvent);
    }
    
    return 0;
}
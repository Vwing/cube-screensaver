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
const float CUBE_SIZE = 50.0f;  // Size in pixels
const float SPEED_MULTIPLIER = 1.0f;
const int CELEBRATION_DURATION = 60;

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

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
    Monitor mon;
    mon.bounds = *lprcMonitor;
    mon.hwnd = NULL;
    mon.hdc = NULL;
    mon.hglrc = NULL;
    
    monitors.push_back(mon);
    return TRUE;
}

void InitializeCube() {
    // Get total desktop bounds
    RECT totalBounds = {0};
    for (const auto& mon : monitors) {
        if (mon.bounds.left < totalBounds.left) totalBounds.left = mon.bounds.left;
        if (mon.bounds.top < totalBounds.top) totalBounds.top = mon.bounds.top;
        if (mon.bounds.right > totalBounds.right) totalBounds.right = mon.bounds.right;
        if (mon.bounds.bottom > totalBounds.bottom) totalBounds.bottom = mon.bounds.bottom;
    }
    
    // Start cube in center of primary monitor
    if (!monitors.empty()) {
        const Monitor& primary = monitors[0];
        globalCube.x = (primary.bounds.left + primary.bounds.right) / 2.0f;
        globalCube.y = (primary.bounds.top + primary.bounds.bottom) / 2.0f;
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
    // Convert world coordinates to screen-relative coordinates accounting for aspect ratio
    float aspect = (float)(mon.bounds.right - mon.bounds.left) / (mon.bounds.bottom - mon.bounds.top);
    float relX = (cube.x - mon.bounds.left) / (mon.bounds.right - mon.bounds.left) * 4.0f * aspect - 2.0f * aspect;
    float relY = -((cube.y - mon.bounds.top) / (mon.bounds.bottom - mon.bounds.top) * 4.0f - 2.0f);
    
    // Fixed cube size for perspective projection
    float cubeScale = 0.1f;
    
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
    
    // Update position
    globalCube.x += globalCube.vx;
    globalCube.y += globalCube.vy;
    
    // Update rotation by applying incremental rotation to current matrix
    float rotMatrix[16];
    CreateRotationMatrix(rotMatrix, globalCube.rotationSpeed, 
                        globalCube.rotationAxisX, globalCube.rotationAxisY, globalCube.rotationAxisZ);
    
    float newMatrix[16];
    MultiplyMatrix4x4(newMatrix, rotMatrix, globalCube.rotationMatrix);
    
    // Copy result back
    for (int i = 0; i < 16; i++) {
        globalCube.rotationMatrix[i] = newMatrix[i];
    }
    
    // Get total desktop bounds
    RECT totalBounds = {0};
    for (const auto& mon : monitors) {
        if (mon.bounds.left < totalBounds.left) totalBounds.left = mon.bounds.left;
        if (mon.bounds.top < totalBounds.top) totalBounds.top = mon.bounds.top;
        if (mon.bounds.right > totalBounds.right) totalBounds.right = mon.bounds.right;
        if (mon.bounds.bottom > totalBounds.bottom) totalBounds.bottom = mon.bounds.bottom;
    }
    
    bool hitCorner = false;
    const float CORNER_THRESHOLD = CUBE_SIZE * 2;
    
    // Check boundaries against total desktop area
    if (globalCube.x - CUBE_SIZE <= totalBounds.left) {
        globalCube.x = totalBounds.left + CUBE_SIZE;
        globalCube.vx = -globalCube.vx;
        
        // Add randomness that makes bounce more extreme (1-3 degrees in current direction)
        float currentAngle = atan2(globalCube.vy, globalCube.vx);
        float randomOffset = (static_cast<float>(rand()) / RAND_MAX) * 0.052f; // 0-3 degrees in radians
        if (globalCube.vy > 0) randomOffset = -randomOffset; // Make bounce more extreme away from wall
        else randomOffset = randomOffset;
        
        float speed = sqrt(globalCube.vx * globalCube.vx + globalCube.vy * globalCube.vy);
        float newAngle = currentAngle + randomOffset;
        globalCube.vx = cos(newAngle) * speed;
        globalCube.vy = sin(newAngle) * speed;
        
        // Change to new random rotation axis and speed - matrix preserves current orientation
        float axisX = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
        float axisY = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
        float axisZ = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
        float axisLength = sqrt(axisX * axisX + axisY * axisY + axisZ * axisZ);
        globalCube.rotationAxisX = axisX / axisLength;
        globalCube.rotationAxisY = axisY / axisLength;
        globalCube.rotationAxisZ = axisZ / axisLength;
        globalCube.rotationSpeed = ((rand() % 2 == 0) ? 1 : -1) * (0.5f + (static_cast<float>(rand()) / RAND_MAX) * 3.0f);
        
        if (abs(globalCube.y - totalBounds.top) < CORNER_THRESHOLD || 
            abs(globalCube.y - totalBounds.bottom) < CORNER_THRESHOLD) {
            hitCorner = true;
        }
    } else if (globalCube.x + CUBE_SIZE >= totalBounds.right) {
        globalCube.x = totalBounds.right - CUBE_SIZE;
        globalCube.vx = -globalCube.vx;
        
        // Add randomness that makes bounce more extreme (1-3 degrees in current direction)
        float currentAngle = atan2(globalCube.vy, globalCube.vx);
        float randomOffset = (static_cast<float>(rand()) / RAND_MAX) * 0.052f; // 0-3 degrees in radians
        if (globalCube.vy > 0) randomOffset = randomOffset; // Make bounce more extreme away from wall
        else randomOffset = -randomOffset;
        
        float speed = sqrt(globalCube.vx * globalCube.vx + globalCube.vy * globalCube.vy);
        float newAngle = currentAngle + randomOffset;
        globalCube.vx = cos(newAngle) * speed;
        globalCube.vy = sin(newAngle) * speed;
        
        // Change to new random rotation axis and speed - matrix preserves current orientation
        float axisX = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
        float axisY = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
        float axisZ = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
        float axisLength = sqrt(axisX * axisX + axisY * axisY + axisZ * axisZ);
        globalCube.rotationAxisX = axisX / axisLength;
        globalCube.rotationAxisY = axisY / axisLength;
        globalCube.rotationAxisZ = axisZ / axisLength;
        globalCube.rotationSpeed = ((rand() % 2 == 0) ? 1 : -1) * (0.5f + (static_cast<float>(rand()) / RAND_MAX) * 3.0f);
        
        if (abs(globalCube.y - totalBounds.top) < CORNER_THRESHOLD || 
            abs(globalCube.y - totalBounds.bottom) < CORNER_THRESHOLD) {
            hitCorner = true;
        }
    }
    
    if (globalCube.y - CUBE_SIZE <= totalBounds.top) {
        globalCube.y = totalBounds.top + CUBE_SIZE;
        globalCube.vy = -globalCube.vy;
        
        // Add randomness that makes bounce more extreme (1-3 degrees in current direction)
        float currentAngle = atan2(globalCube.vy, globalCube.vx);
        float randomOffset = (static_cast<float>(rand()) / RAND_MAX) * 0.052f; // 0-3 degrees in radians
        if (globalCube.vx > 0) randomOffset = randomOffset; // Make bounce more extreme away from wall
        else randomOffset = -randomOffset;
        
        float speed = sqrt(globalCube.vx * globalCube.vx + globalCube.vy * globalCube.vy);
        float newAngle = currentAngle + randomOffset;
        globalCube.vx = cos(newAngle) * speed;
        globalCube.vy = sin(newAngle) * speed;
        
        // Change to new random rotation axis and speed - matrix preserves current orientation
        float axisX = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
        float axisY = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
        float axisZ = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
        float axisLength = sqrt(axisX * axisX + axisY * axisY + axisZ * axisZ);
        globalCube.rotationAxisX = axisX / axisLength;
        globalCube.rotationAxisY = axisY / axisLength;
        globalCube.rotationAxisZ = axisZ / axisLength;
        globalCube.rotationSpeed = ((rand() % 2 == 0) ? 1 : -1) * (0.5f + (static_cast<float>(rand()) / RAND_MAX) * 3.0f);
        
        if (abs(globalCube.x - totalBounds.left) < CORNER_THRESHOLD || 
            abs(globalCube.x - totalBounds.right) < CORNER_THRESHOLD) {
            hitCorner = true;
        }
    } else if (globalCube.y + CUBE_SIZE >= totalBounds.bottom) {
        globalCube.y = totalBounds.bottom - CUBE_SIZE;
        globalCube.vy = -globalCube.vy;
        
        // Add randomness that makes bounce more extreme (1-3 degrees in current direction)
        float currentAngle = atan2(globalCube.vy, globalCube.vx);
        float randomOffset = (static_cast<float>(rand()) / RAND_MAX) * 0.052f; // 0-3 degrees in radians
        if (globalCube.vx > 0) randomOffset = -randomOffset; // Make bounce more extreme away from wall
        else randomOffset = randomOffset;
        
        float speed = sqrt(globalCube.vx * globalCube.vx + globalCube.vy * globalCube.vy);
        float newAngle = currentAngle + randomOffset;
        globalCube.vx = cos(newAngle) * speed;
        globalCube.vy = sin(newAngle) * speed;
        
        // Change to new random rotation axis and speed - matrix preserves current orientation
        float axisX = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
        float axisY = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
        float axisZ = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
        float axisLength = sqrt(axisX * axisX + axisY * axisY + axisZ * axisZ);
        globalCube.rotationAxisX = axisX / axisLength;
        globalCube.rotationAxisY = axisY / axisLength;
        globalCube.rotationAxisZ = axisZ / axisLength;
        globalCube.rotationSpeed = ((rand() % 2 == 0) ? 1 : -1) * (0.5f + (static_cast<float>(rand()) / RAND_MAX) * 3.0f);
        
        if (abs(globalCube.x - totalBounds.left) < CORNER_THRESHOLD || 
            abs(globalCube.x - totalBounds.right) < CORNER_THRESHOLD) {
            hitCorner = true;
        }
    }
    
    if (hitCorner && !globalCube.celebratingCorner) {
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
    wglMakeCurrent(mon.hdc, mon.hglrc);
    
    glViewport(0, 0, mon.bounds.right - mon.bounds.left, mon.bounds.bottom - mon.bounds.top);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // Use perspective projection for proper 3D appearance
    float aspect = (float)(mon.bounds.right - mon.bounds.left) / (mon.bounds.bottom - mon.bounds.top);
    gluPerspective(45.0, aspect, 0.1, 100.0);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // Check if cube is visible on this monitor
    if (globalCube.active &&
        globalCube.x + CUBE_SIZE >= mon.bounds.left &&
        globalCube.x - CUBE_SIZE <= mon.bounds.right &&
        globalCube.y + CUBE_SIZE >= mon.bounds.top &&
        globalCube.y - CUBE_SIZE <= mon.bounds.bottom) {
        DrawCube(globalCube, mon);
    }
    
    SwapBuffers(mon.hdc);
}

LRESULT WINAPI ScreenSaverProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    static UINT_PTR timer;
    static bool isFirstMonitor = true;
    static HWND lastUpdater = NULL;
    
    switch (message) {
    case WM_CREATE:
        srand(static_cast<unsigned>(time(nullptr)));
        if (isFirstMonitor) {
            monitors.clear();
            EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);
            InitializeCube();
            isFirstMonitor = false;
        }
        
        for (auto& mon : monitors) {
            if (mon.hwnd == NULL) {
                mon.hwnd = hwnd;
                InitOpenGL(hwnd, mon);
                break;
            }
        }
        
        timer = SetTimer(hwnd, 1, 16, NULL);
        return 0;
        
    case WM_TIMER:
        // Update cube once per timer tick
        if (lastUpdater == NULL || lastUpdater == hwnd) {
            UpdateCube();
            lastUpdater = hwnd;
        }
        
        // Render on all monitors
        for (auto& mon : monitors) {
            if (mon.hwnd == hwnd && mon.hglrc != NULL) {
                RenderScene(mon);
                break;
            }
        }
        return 0;
        
    case WM_DESTROY:
        KillTimer(hwnd, timer);
        for (auto& mon : monitors) {
            if (mon.hwnd == hwnd) {
                wglMakeCurrent(NULL, NULL);
                wglDeleteContext(mon.hglrc);
                ReleaseDC(hwnd, mon.hdc);
                mon.hwnd = NULL;
                mon.hglrc = NULL;
                break;
            }
        }
        if (isFirstMonitor == false) {
            bool allClosed = true;
            for (auto& mon : monitors) {
                if (mon.hwnd != NULL) {
                    allClosed = false;
                    break;
                }
            }
            if (allClosed) {
                monitors.clear();
                isFirstMonitor = true;
            }
        }
        PostQuitMessage(0);
        return 0;
        
    case WM_DISPLAYCHANGE:
        monitors.clear();
        isFirstMonitor = true;
        return 0;
    }
    
    return DefScreenSaverProc(hwnd, message, wParam, lParam);
}

BOOL WINAPI ScreenSaverConfigureDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG:
        return TRUE;
        
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }
        break;
    }
    return FALSE;
}

BOOL WINAPI RegisterDialogClasses(HANDLE hInst) {
    return TRUE;
}
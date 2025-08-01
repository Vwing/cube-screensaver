#include <windows.h>
#include <scrnsave.h>
#include <commctrl.h>
#include <string>
#include <sstream>
#include <vector>

#pragma comment(lib, "scrnsave.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comctl32.lib")

#define IDC_CUBE_SIZE_SLIDER 1001
#define IDC_CUBE_SIZE_LABEL 1002
#define IDC_ENABLE_CELEBRATION 1003
#define IDC_ENABLE_MIRROR_MODE 1004
#define REGISTRY_KEY "Software\\BouncingCubeScreensaver"

// Global variables for child process management
PROCESS_INFORMATION g_ChildProcess = {0};
HANDLE g_ExitEvent = NULL;
std::wstring g_ExitEventName;

float g_CubeSize = 0.1f;  // Default cube scale for 3D rendering
bool g_EnableCelebration = false;  // Default celebration setting
bool g_MirrorMode = false;  // Default mirror mode disabled for multi-monitor support

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
        
        DWORD dwMirrorMode = 0;  // Default to false for multi-monitor support
        DWORD dwMirrorModeSize = sizeof(DWORD);
        if (RegQueryValueEx(hKey, "MirrorMode", NULL, NULL, (LPBYTE)&dwMirrorMode, &dwMirrorModeSize) == ERROR_SUCCESS) {
            g_MirrorMode = (dwMirrorMode != 0);
        }
        
        RegCloseKey(hKey);
    }
}

void SaveSettings() {
    HKEY hKey;
    if (RegCreateKeyEx(HKEY_CURRENT_USER, REGISTRY_KEY, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueEx(hKey, "CubeSize", 0, REG_BINARY, (LPBYTE)&g_CubeSize, sizeof(float));
        
        DWORD dwCelebration = g_EnableCelebration ? 1 : 0;
        RegSetValueEx(hKey, "EnableCelebration", 0, REG_DWORD, (LPBYTE)&dwCelebration, sizeof(DWORD));
        
        DWORD dwMirrorMode = g_MirrorMode ? 1 : 0;
        RegSetValueEx(hKey, "MirrorMode", 0, REG_DWORD, (LPBYTE)&dwMirrorMode, sizeof(DWORD));
        
        RegCloseKey(hKey);
    }
}

float CubeSizeSliderToScale(int sliderPos) {
    return 0.05f + (sliderPos / 100.0f) * 0.55f;
}

int CubeScaleToSlider(float scale) {
    return (int)((scale - 0.05f) / 0.55f * 100.0f);
}

const char* GetCubeSizeLabel(int sliderPos) {
    if (sliderPos < 25) return "Small";
    else if (sliderPos < 75) return "Medium";
    else return "Large";
}

std::wstring GetApplicationPath() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    
    // Replace .scr with App.exe for the application
    std::wstring appPath(path);
    size_t scrPos = appPath.find_last_of(L'.');
    if (scrPos != std::wstring::npos) {
        appPath = appPath.substr(0, scrPos) + L"App.exe";
    }
    
    return appPath;
}

PROCESS_INFORMATION LaunchChild(const std::wstring& commandLine) {
    PROCESS_INFORMATION pi = {0};
    STARTUPINFOW si = {sizeof(STARTUPINFOW)};
    
    // Create command line string
    std::wstring appPath = GetApplicationPath();
    std::wstring cmdLine = L"\"" + appPath + L"\" " + commandLine;
    
    // Convert to modifiable string
    std::vector<wchar_t> cmdBuffer(cmdLine.begin(), cmdLine.end());
    cmdBuffer.push_back(0);
    
    // Check if file exists
    DWORD attrs = GetFileAttributesW(appPath.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES) {
        // File doesn't exist, show error
        std::wstring error = L"Cannot find BouncingCubeApp.exe at: " + appPath;
        MessageBoxW(NULL, error.c_str(), L"Screensaver Error", MB_OK | MB_ICONERROR);
        return pi;
    }
    
    if (CreateProcessW(
        NULL, // Let system find the executable
        cmdBuffer.data(),
        NULL, NULL, FALSE, 0, NULL, NULL,
        &si, &pi)) {
        
        return pi;
    } else {
        // Get error code
        DWORD error = GetLastError();
        wchar_t errorMsg[256];
        swprintf_s(errorMsg, L"Failed to launch child process. Error code: %d\nCommand: %s", error, cmdLine.c_str());
        MessageBoxW(NULL, errorMsg, L"Screensaver Error", MB_OK | MB_ICONERROR);
    }
    
    return pi;
}

void CreateExitEvent() {
    // Create a unique event name
    DWORD processId = GetCurrentProcessId();
    DWORD threadId = GetCurrentThreadId();
    SYSTEMTIME st;
    GetSystemTime(&st);
    
    std::wstringstream ss;
    ss << L"Global\\BouncingCubeExit_" << processId << L"_" << threadId << L"_" << st.wMilliseconds;
    g_ExitEventName = ss.str();
    
    g_ExitEvent = CreateEventW(NULL, TRUE, FALSE, g_ExitEventName.c_str());
}

LRESULT WINAPI ScreenSaverProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    static bool initialized = false;
    
    switch (message) {
    case WM_CREATE:
        if (!initialized) {
            CreateExitEvent();
            
            // Launch the child application
            LoadSettings(); // Load settings including mirror mode
            std::wstring cmdLine = L"--monitors all --exitEvent " + g_ExitEventName;
            if (g_MirrorMode) {
                cmdLine += L" --mirror";
            }
            g_ChildProcess = LaunchChild(cmdLine);
            
            initialized = true;
        }
        return 0;
        
    case WM_KEYDOWN:
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MOUSEMOVE:
        // Signal child to exit
        if (g_ExitEvent) {
            SetEvent(g_ExitEvent);
        }
        
        // Wait for child to exit (with timeout)
        if (g_ChildProcess.hProcess) {
            WaitForSingleObject(g_ChildProcess.hProcess, 1000);
            CloseHandle(g_ChildProcess.hProcess);
            CloseHandle(g_ChildProcess.hThread);
            g_ChildProcess = {0};
        }
        
        PostQuitMessage(0);
        return 0;
        
    case WM_DESTROY:
        // Clean up
        if (g_ExitEvent) {
            SetEvent(g_ExitEvent);
            CloseHandle(g_ExitEvent);
            g_ExitEvent = NULL;
        }
        
        if (g_ChildProcess.hProcess) {
            WaitForSingleObject(g_ChildProcess.hProcess, 1000);
            TerminateProcess(g_ChildProcess.hProcess, 0);
            CloseHandle(g_ChildProcess.hProcess);
            CloseHandle(g_ChildProcess.hThread);
            g_ChildProcess = {0};
        }
        
        PostQuitMessage(0);
        return 0;
    }
    
    return DefScreenSaverProc(hwnd, message, wParam, lParam);
}

BOOL WINAPI ScreenSaverConfigureDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    static int currentSliderPos = 50;  // Default to medium
    
    switch (message) {
    case WM_INITDIALOG:
        {
            // Initialize common controls
            InitCommonControls();
            
            // Load current settings
            LoadSettings();
            currentSliderPos = CubeScaleToSlider(g_CubeSize);
            
            // Set up the slider
            HWND hSlider = GetDlgItem(hDlg, IDC_CUBE_SIZE_SLIDER);
            SendMessage(hSlider, TBM_SETRANGE, TRUE, MAKELONG(0, 100));
            SendMessage(hSlider, TBM_SETPOS, TRUE, currentSliderPos);
            SendMessage(hSlider, TBM_SETTICFREQ, 25, 0);
            
            // Update label
            SetDlgItemText(hDlg, IDC_CUBE_SIZE_LABEL, GetCubeSizeLabel(currentSliderPos));
            
            // Set checkbox state
            CheckDlgButton(hDlg, IDC_ENABLE_CELEBRATION, g_EnableCelebration ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hDlg, IDC_ENABLE_MIRROR_MODE, g_MirrorMode ? BST_CHECKED : BST_UNCHECKED);
            
            return TRUE;
        }
        
    case WM_HSCROLL:
        if ((HWND)lParam == GetDlgItem(hDlg, IDC_CUBE_SIZE_SLIDER)) {
            currentSliderPos = (int)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
            SetDlgItemText(hDlg, IDC_CUBE_SIZE_LABEL, GetCubeSizeLabel(currentSliderPos));
            return TRUE;
        }
        break;
        
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK) {
            // Save settings
            g_CubeSize = CubeSizeSliderToScale(currentSliderPos);
            g_EnableCelebration = (IsDlgButtonChecked(hDlg, IDC_ENABLE_CELEBRATION) == BST_CHECKED);
            g_MirrorMode = (IsDlgButtonChecked(hDlg, IDC_ENABLE_MIRROR_MODE) == BST_CHECKED);
            SaveSettings();
            EndDialog(hDlg, IDOK);
            return TRUE;
        } else if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

BOOL WINAPI RegisterDialogClasses(HANDLE hInst) {
    return TRUE;
}
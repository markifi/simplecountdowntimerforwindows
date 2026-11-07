#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <shellapi.h>
#include <string>
#include <sstream>
#include <thread>
#include <atomic>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <ctime>

// Configuration Structure
struct AppConfig {
    int fontSize = 80;
    std::wstring fontFace = L"Atkinson Hyperlegible Mono Light";
    COLORREF bgColor = RGB(24, 24, 24);
    COLORREF normalColor = RGB(240, 240, 240);
    COLORREF overtimeColor = RGB(255, 0, 0); // #FF0000
    COLORREF promptColor = RGB(180, 180, 180);
};

AppConfig config;

// Application States
enum State { STATE_INPUT, STATE_RUNNING, STATE_FINISHED };
std::atomic<State> currentState(STATE_INPUT);
std::atomic<long long> remainingSeconds(0);
std::atomic<bool> exitThread(false);
std::thread timerThread;

HWND hMainWnd = NULL;
std::wstring userInput = L"";
bool beepPlayed = false;

// Forward Declarations
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void StartTimer(long long seconds);
void TimerLoop();
long long ParseDuration(const std::wstring& input);
void LoadConfiguration();
COLORREF ParseHexColor(const std::string& str, COLORREF defaultColor);
std::wstring GetConfigFilePath();

int main() {
    LoadConfiguration();

    HINSTANCE hInstance = GetModuleHandle(NULL);
    int nCmdShow = SW_SHOW;

    // Parse command line arguments
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argc > 1) {
        long long cmdSecs = ParseDuration(argv[1]);
        if (cmdSecs > 0) {
            remainingSeconds = cmdSecs;
            currentState = STATE_RUNNING;
        }
    }
    if (argv) LocalFree(argv);

    const wchar_t CLASS_NAME[] = L"SimpleCountdownTimerClass";
    
    WNDCLASSW wc = {};
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClassW(&wc);

    // Borderless popup window with double size (840 x 480)
    hMainWnd = CreateWindowExW(
        WS_EX_APPWINDOW, CLASS_NAME, L"simplecountdowntimerforwindows",
        WS_POPUP | WS_SYSMENU | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 840, 480,
        NULL, NULL, hInstance, NULL
    );

    if (hMainWnd == NULL) return 0;

    ShowWindow(hMainWnd, nCmdShow);

    MSG msg = {};
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    exitThread = true;
    if (timerThread.joinable()) {
        timerThread.join();
    }

    return 0;
}

// Resolves %appdata%\simplecountdowntimerforwindows\simplecountdowntimerforwindows.conf
std::wstring GetConfigFilePath() {
    wchar_t appData[MAX_PATH];
    if (GetEnvironmentVariableW(L"APPDATA", appData, MAX_PATH) > 0) {
        std::wstring dir = std::wstring(appData) + L"\\simplecountdowntimerforwindows";
        CreateDirectoryW(dir.c_str(), NULL);
        return dir + L"\\simplecountdowntimerforwindows.conf";
    }
    return L"simplecountdowntimerforwindows.conf";
}

COLORREF ParseHexColor(const std::string& str, COLORREF defaultColor) {
    std::string s = str;
    if (!s.empty() && s[0] == '#') {
        s = s.substr(1);
    }
    if (s.length() != 6) return defaultColor;
    unsigned int r = 0, g = 0, b = 0;
    if (sscanf(s.c_str(), "%02x%02x%02x", &r, &g, &b) == 3) {
        return RGB(r, g, b);
    }
    return defaultColor;
}

void LoadConfiguration() {
    std::wstring confPath = GetConfigFilePath();
    std::string filename(confPath.begin(), confPath.end());
    
    // Ensure default config file exists in MPV style with hex color codes
    {
        std::ifstream checkFile(filename);
        if (!checkFile.is_open()) {
            std::ofstream outfile(filename);
            if (outfile.is_open()) {
                outfile << "font-size = 80\n";
                outfile << "font-face = Atkinson Hyperlegible Mono Light\n";
                outfile << "background-rgb = 181818\n";
                outfile << "normal-text-rgb = F0F0F0\n";
                outfile << "overtime-text-rgb = FF0000\n";
                outfile << "prompt-text-rgb = B4B4B4\n";
                outfile.close();
            }
        }
    }

    std::ifstream file(filename);
    if (!file.is_open()) return;

    std::string line;
    while (std::getline(file, line)) {
        std::string trimmed = line;
        trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));
        trimmed.erase(trimmed.find_last_not_of(" \t\r\n") + 1);

        if (trimmed.empty() || trimmed[0] == '#' || trimmed[0] == ';') continue;

        size_t delim = trimmed.find('=');
        std::string key, val;
        if (delim != std::string::npos) {
            key = trimmed.substr(0, delim);
            val = trimmed.substr(delim + 1);
        } else {
            size_t space = trimmed.find_first_of(" \t");
            if (space == std::string::npos) continue;
            key = trimmed.substr(0, space);
            val = trimmed.substr(space);
        }

        // Normalize key (lowercase, remove hyphens/underscores)
        key.erase(std::remove_if(key.begin(), key.end(), [](char c){ return c == ' ' || c == '\t' || c == '-' || c == '_'; }), key.end());
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);

        // Trim value and strip optional surrounding quotes
        val.erase(0, val.find_first_not_of(" \t\r\n"));
        val.erase(val.find_last_not_of(" \t\r\n") + 1);
        if (val.length() >= 2 && val.front() == '"' && val.back() == '"') {
            val = val.substr(1, val.length() - 2);
        }

        if (key == "fontsize") {
            config.fontSize = std::stoi(val);
        } else if (key == "fontface") {
            config.fontFace = std::wstring(val.begin(), val.end());
        } else if (key == "backgroundrgb" || key == "bg") {
            config.bgColor = ParseHexColor(val, config.bgColor);
        } else if (key == "normaltextrgb" || key == "normalcolor") {
            config.normalColor = ParseHexColor(val, config.normalColor);
        } else if (key == "overtimetextrgb" || key == "overtimecolor") {
            config.overtimeColor = ParseHexColor(val, config.overtimeColor);
        } else if (key == "prompttextrgb" || key == "promptcolor") {
            config.promptColor = ParseHexColor(val, config.promptColor);
        }
    }
}

// Parses duration inputs (supports relative like "5m", raw seconds, and absolute like "7am", "7:00am", "10:45pm")
long long ParseDuration(const std::wstring& input) {
    if (input.empty()) return 0;

    // Clean input: lowercase, remove spaces and periods
    std::wstring clean = input;
    clean.erase(std::remove(clean.begin(), clean.end(), L' '), clean.end());
    clean.erase(std::remove(clean.begin(), clean.end(), L'.'), clean.end());
    std::transform(clean.begin(), clean.end(), clean.begin(), ::towlower);

    bool isPM = (clean.find(L"pm") != std::wstring::npos);
    bool isAM = (clean.find(L"am") != std::wstring::npos);
    bool hasColon = (clean.find(L':') != std::wstring::npos);

    if (isAM || isPM || hasColon) {
        if (isPM) {
            size_t pos = clean.find(L"pm");
            clean.erase(pos, 2);
        } else if (isAM) {
            size_t pos = clean.find(L"am");
            clean.erase(pos, 2);
        }

        int targetHour = 0;
        int targetMin = 0;

        size_t colonPos = clean.find(L':');
        if (colonPos != std::wstring::npos) {
            std::wstring hStr = clean.substr(0, colonPos);
            std::wstring mStr = clean.substr(colonPos + 1);
            try {
                targetHour = std::stoi(hStr);
                if (!mStr.empty()) targetMin = std::stoi(mStr);
            } catch (...) { return 0; }
        } else {
            std::wstring digits = L"";
            for (wchar_t c : clean) {
                if (iswdigit(c)) digits += c;
            }
            if (digits.empty()) return 0;

            if (digits.length() <= 2) {
                targetHour = std::stoi(digits);
                targetMin = 0;
            } else if (digits.length() == 3) {
                targetHour = std::stoi(digits.substr(0, 1));
                targetMin = std::stoi(digits.substr(1, 2));
            } else if (digits.length() >= 4) {
                targetHour = std::stoi(digits.substr(0, 2));
                targetMin = std::stoi(digits.substr(2, 2));
            }
        }

        if (isPM && targetHour < 12) targetHour += 12;
        if (isAM && targetHour == 12) targetHour = 0;

        std::time_t now = std::time(nullptr);
        std::tm* localTime = std::localtime(&now);
        if (localTime) {
            long long currentTotalSec = localTime->tm_hour * 3600 + localTime->tm_min * 60 + localTime->tm_sec;
            long long targetTotalSec = targetHour * 3600 + targetMin * 60;

            long long diffSec = targetTotalSec - currentTotalSec;
            if (diffSec <= 0) {
                diffSec += 24 * 3600; // Roll over to tomorrow if passed
            }
            return diffSec;
        }
    }

    // Otherwise, parse relative duration (e.g., 5m, 30s, or raw seconds)
    bool pureNumber = true;
    for (wchar_t c : input) {
        if (!iswdigit(c)) {
            pureNumber = false;
            break;
        }
    }
    if (pureNumber) return std::stoll(input);

    long long totalSec = 0;
    long long currentVal = 0;
    for (size_t i = 0; i < input.length(); ++i) {
        wchar_t c = input[i];
        if (iswdigit(c)) {
            currentVal = currentVal * 10 + (c - L'0');
        } else {
            wchar_t unit = towlower(c);
            long long multiplier = 1;
            if (unit == L's') multiplier = 1;
            else if (unit == L'm') {
                if (i + 1 < input.length() && towlower(input[i+1]) == L'o') {
                    multiplier = 30LL * 24 * 3600;
                    i++;
                } else {
                    multiplier = 60;
                }
            }
            else if (unit == L'h') multiplier = 3600;
            else if (unit == L'd') multiplier = 24 * 3600;
            else if (unit == L'w') multiplier = 7 * 24 * 3600;
            
            totalSec += currentVal * multiplier;
            currentVal = 0;
        }
    }
    totalSec += currentVal;
    return totalSec;
}

// Background worker thread counting down and continuing upwards into overtime
void TimerLoop() {
    while (!exitThread) {
        if (currentState == STATE_RUNNING) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            if (currentState == STATE_RUNNING) {
                remainingSeconds--;
                InvalidateRect(hMainWnd, NULL, TRUE);
                
                if (remainingSeconds == 0 && !beepPlayed) {
                    beepPlayed = true;
                    // Three crisp, consecutive loud digital clock beeps
                    for (int i = 0; i < 3; ++i) {
                        Beep(2000, 150);
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    }
                    FlashWindow(hMainWnd, TRUE);
                }
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

void StartTimer(long long seconds) {
    if (seconds <= 0) return;
    remainingSeconds = seconds;
    currentState = STATE_RUNNING;
    beepPlayed = false;
    InvalidateRect(hMainWnd, NULL, TRUE);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        timerThread = std::thread(TimerLoop);
        break;
    }
    case WM_NCHITTEST: {
        LRESULT hit = DefWindowProcW(hwnd, uMsg, wParam, lParam);
        if (hit == HTCLIENT) return HTCAPTION;
        return hit;
    }
    case WM_CHAR: {
        if (currentState == STATE_INPUT) {
            wchar_t ch = (wchar_t)wParam;
            if (ch == L'\b') {
                if (!userInput.empty()) {
                    userInput.pop_back();
                    InvalidateRect(hwnd, NULL, TRUE);
                }
            } else if (ch >= 32 && ch != L'e' && ch != L'E') {
                userInput += ch;
                InvalidateRect(hwnd, NULL, TRUE);
            }
        }
        break;
    }
    case WM_KEYDOWN: {
        if (wParam == VK_RETURN) {
            if (currentState == STATE_INPUT) {
                long long secs = ParseDuration(userInput);
                if (secs > 0) {
                    StartTimer(secs);
                }
            }
        } else if (wParam == VK_BACK) {
            if (currentState == STATE_INPUT && !userInput.empty()) {
                userInput.pop_back();
                InvalidateRect(hwnd, NULL, TRUE);
            }
        } else if (wParam == 'E') {
            // Open config in Notepad, wait until closed, then reload & redraw
            std::wstring confPath = GetConfigFilePath();
            std::thread([confPath]() {
                SHELLEXECUTEINFOW sei = { sizeof(sei) };
                sei.fMask = SEE_MASK_NOCLOSEPROCESS;
                sei.lpFile = L"notepad.exe";
                sei.lpParameters = confPath.c_str();
                sei.nShow = SW_SHOWNORMAL;
                if (ShellExecuteExW(&sei)) {
                    WaitForSingleObject(sei.hProcess, INFINITE);
                    CloseHandle(sei.hProcess);
                    LoadConfiguration();
                    InvalidateRect(hMainWnd, NULL, TRUE);
                }
            }).detach();
        }
        break;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rect;
        GetClientRect(hwnd, &rect);

        HBRUSH bgBrush = CreateSolidBrush(config.bgColor);
        FillRect(hdc, &rect, bgBrush);
        DeleteObject(bgBrush);

        SetBkMode(hdc, TRANSPARENT);
        
        if (currentState == STATE_INPUT) {
            SetTextColor(hdc, config.promptColor);
            HFONT hFontPrompt = CreateFontW(config.fontSize / 2.5, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 
                                            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, 
                                            DEFAULT_PITCH | FF_MODERN, config.fontFace.c_str());
            SelectObject(hdc, hFontPrompt);
            
            RECT promptRect = { 40, 80, rect.right - 40, 180 };
            DrawTextW(hdc, L"Type duration & Press Enter:", -1, &promptRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
            DeleteObject(hFontPrompt);

            SetTextColor(hdc, config.normalColor);
            HFONT hFontInput = CreateFontW(config.fontSize * 0.6, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 
                                           OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, 
                                           DEFAULT_PITCH | FF_MODERN, config.fontFace.c_str());
            SelectObject(hdc, hFontInput);

            RECT inputRect = { 80, 220, rect.right - 80, 320 };
            std::wstring displayInput = userInput + L"_";
            DrawTextW(hdc, displayInput.c_str(), -1, &inputRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            DeleteObject(hFontInput);
        } else {
            long long rem = remainingSeconds;
            bool isOvertime = (rem < 0);
            long long absRem = isOvertime ? -rem : rem;

            long long h = absRem / 3600;
            long long m = (absRem % 3600) / 60;
            long long s = absRem % 60;

            std::wostringstream wss;
            if (isOvertime) wss << L"-";
            if (h > 0) {
                wss << std::setfill(L'0') << std::setw(2) << h << L":"
                    << std::setw(2) << m << L":" << std::setw(2) << s;
            } else {
                wss << std::setfill(L'0') << std::setw(2) << m << L":"
                    << std::setw(2) << s;
            }

            std::wstring timeStr = wss.str();

            SetTextColor(hdc, isOvertime ? config.overtimeColor : config.normalColor);
            
            HFONT hFont = CreateFontW(config.fontSize, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 
                                      OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, 
                                      DEFAULT_PITCH | FF_MODERN, config.fontFace.c_str());
            SelectObject(hdc, hFont);

            DrawTextW(hdc, timeStr.c_str(), -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            DeleteObject(hFont);
        }

        EndPaint(hwnd, &ps);
        break;
    }
    case WM_DESTROY:
        exitThread = true;
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}
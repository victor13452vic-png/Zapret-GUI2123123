#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <windowsx.h>
#include <objidl.h>

#ifndef PROPID
typedef ULONG PROPID;
#endif

#include <gdiplus.h>
#include <shellapi.h>
#include <commctrl.h>
#include <shlobj.h>
#include <commdlg.h>
#include <dwmapi.h>
#include <tlhelp32.h>
#include <string>
#include <vector>
#include <filesystem>
#include <functional>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <fstream>

#include <thread>
#include <atomic>
#include <mutex>

#include "strategies.h"

namespace fs = std::filesystem;

#define IDI_APPICON 101

#define APP_VERSION L"1.0.0"

namespace Colors
{
    inline Gdiplus::Color BgDark()      { return Gdiplus::Color(255, 8, 14, 12); }
    inline Gdiplus::Color BgPanel()     { return Gdiplus::Color(255, 12, 20, 17); }
    inline Gdiplus::Color BgTitle()     { return Gdiplus::Color(255, 6, 12, 10); }
    inline Gdiplus::Color BgCard()      { return Gdiplus::Color(255, 18, 30, 25); }
    inline Gdiplus::Color BgHover()     { return Gdiplus::Color(255, 22, 38, 32); }

    inline Gdiplus::Color Accent()      { return Gdiplus::Color(255, 46, 222, 162); }
    inline Gdiplus::Color AccentHover() { return Gdiplus::Color(255, 80, 240, 180); }
    inline Gdiplus::Color AccentBright(){ return Gdiplus::Color(255, 120, 255, 200); }
    inline Gdiplus::Color AccentDeep()  { return Gdiplus::Color(255, 28, 180, 130); }

    inline Gdiplus::Color Success()     { return Gdiplus::Color(255, 46, 222, 162); }
    inline Gdiplus::Color Danger()      { return Gdiplus::Color(255, 248, 113, 113); }
    inline Gdiplus::Color Warning()     { return Gdiplus::Color(255, 251, 191, 36); }

    inline Gdiplus::Color Text()        { return Gdiplus::Color(255, 230, 250, 240); }
    inline Gdiplus::Color TextDim()     { return Gdiplus::Color(255, 120, 145, 130); }
    inline Gdiplus::Color TextMuted()   { return Gdiplus::Color(255, 80, 100, 90); }

    inline Gdiplus::Color Border()      { return Gdiplus::Color(255, 30, 50, 42); }
}

enum class IntroPhase
{
    FadeInBackground, FadeInZ, HoldZ, ZMovesLeft,
    ApretFliesUp, ApretGlow, HoldFull, FadeOut, Done
};

enum class GameFilterMode
{
    Disabled,
    TcpAndUdp,
    TcpOnly,
    UdpOnly
};

enum class IPSetMode
{
    Loaded,
    None,
    Any
}; 

enum class TestResult
{
    Pending,
    Testing,
    Working,
    Failed,
    Skipped
};

struct DomainTestResult
{
    std::wstring domain;
    TestResult result = TestResult::Pending;
    int pingMs = -1;
};

struct StrategyTestResult
{
    std::wstring strategyName;
    int strategyIndex = -1;
    std::vector<DomainTestResult> domains;
    int workingCount = 0;
    int failedCount = 0;
    int avgPing = 0;
    TestResult overallStatus = TestResult::Pending;
};

enum class TestPhase
{
    Idle,
    Preparing,
    Running,
    Finished,
    Cancelled
};

struct SettingsState
{
    bool autoStart = false;
    bool autoConnect = false;
    bool minimizeToTray = false;
    bool startMinimized = false;
    GameFilterMode gameFilter = GameFilterMode::Disabled;
    IPSetMode ipsetMode = IPSetMode::Loaded;
};

class ZapretGUI
{
public:
    ZapretGUI(HINSTANCE hInstance);
    ~ZapretGUI();
    int Run(int nCmdShow);

private:
    HINSTANCE m_hInstance;
    HWND m_hWnd = nullptr;
    HWND m_hIntroWnd = nullptr;
    HWND m_hSettingsWnd = nullptr;
    ULONG_PTR m_gdiplusToken;
    HICON m_hAppIcon = nullptr;
    Gdiplus::Bitmap* m_logoBitmap = nullptr;

    bool m_introActive = true;
    IntroPhase m_introPhase = IntroPhase::FadeInBackground;
    float m_introTime = 0.0f;
    std::chrono::steady_clock::time_point m_introStart;
    UINT_PTR m_introTimer = 0;

    bool m_minBtnHovered = false;
    bool m_maxBtnHovered = false;
    bool m_closeBtnHovered = false;
    bool m_isMaximized = false;

    bool m_connected = false;
    bool m_connectBtnHovered = false;
    bool m_connectBtnPressed = false;
    bool m_settingsBtnHovered = false;
    bool m_folderBtnHovered = false;
    int  m_hoveredBatIndex = -1;
    int  m_selectedBatIndex = -1;
    int  m_sidebarScroll = 0;
    float m_pulseAnim = 0.0f;
    float m_globalAnim = 0.0f;
    UINT_PTR m_animTimer = 0;
    UINT_PTR m_processCheckTimer = 0;

    fs::path m_zapretPath;
    fs::path m_binPath;
    fs::path m_listsPath;
    bool m_pathsValid = false;

    std::vector<Strategy> m_strategies;

    std::wstring m_gameTcpPorts = L"12";
    std::wstring m_gameUdpPorts = L"12";
    SettingsState m_settings;

    bool m_wasConnected = false;

    HANDLE m_processHandle = nullptr;

    int m_settingsScroll = 0;
    int m_settingsHoveredItem = -1;
    bool m_settingsCloseHovered = false;
    bool m_settingsSaveHovered = false;
    bool m_settingsDraggingScroll = false;

    HWND m_hTestWnd = nullptr;
    std::vector<StrategyTestResult> m_testResults;
    std::atomic<TestPhase> m_testPhase{TestPhase::Idle};
    std::atomic<int> m_testCurrentStrategy{0};
    std::atomic<int> m_testCurrentDomain{0};
    std::atomic<bool> m_testCancelRequested{false};
    std::thread m_testThread;
    std::mutex m_testResultsMutex;
    bool m_savedConnectionState = false;
    int m_savedStrategyIndex = -1;

    int m_testScrollY = 0;
    bool m_testCloseHovered = false;
    bool m_testStartHovered = false;
    bool m_testCancelHovered = false;
    bool m_testExportHovered = false;
    int m_testSortMode = 0;

    const int SIDEBAR_W   = 290;
    const int TITLEBAR_H  = 36;
    const int TOPBAR_H    = 56;

    void CreateIntroWindow();
    static LRESULT CALLBACK IntroWndProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT HandleIntroMessage(UINT msg, WPARAM wParam, LPARAM lParam);
    void OnIntroPaint();
    void UpdateIntroPhase();
    void DrawIntroFrame(Gdiplus::Graphics& g, int w, int h);
    void DrawGlowingText(Gdiplus::Graphics& g, const std::wstring& text,
                         float x, float y, float fontSize, float alpha,
                         float glowIntensity, Gdiplus::Color baseColor);
    float EaseOutCubic(float t);
    float EaseInOutCubic(float t);
    float EaseOutBack(float t);
    float EaseOutExpo(float t);
    void EndIntro();

    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);
    void RegisterWindowClass();
    void CreateMainWindow(int nCmdShow);
    void LoadLogoBitmap();

    void OnPaint();
    void DrawBackground(Gdiplus::Graphics& g, int w, int h);
    void DrawTitleBar(Gdiplus::Graphics& g, int w);
    void DrawTitleButton(Gdiplus::Graphics& g, int x, int y, int w, int h, int type, bool hovered);
    void DrawTopBar(Gdiplus::Graphics& g, int w);
    void DrawSettingsButton(Gdiplus::Graphics& g, int x, int y, int size);
    void DrawCenter(Gdiplus::Graphics& g, int w, int h);
    void DrawConnectButton(Gdiplus::Graphics& g, int cx, int cy, int radius);
    void DrawPowerIcon(Gdiplus::Graphics& g, int cx, int cy, int size, Gdiplus::Color color);
    void DrawSidebar(Gdiplus::Graphics& g, int w, int h);
    void DrawBatItem(Gdiplus::Graphics& g, int x, int y, int w, int index);
    void DrawFolderButton(Gdiplus::Graphics& g, int x, int y, int w, int h);
    void FillRoundRect(Gdiplus::Graphics& g, Gdiplus::Brush* brush, int x, int y, int w, int h, int r);
    void DrawRoundRect(Gdiplus::Graphics& g, Gdiplus::Pen* pen, int x, int y, int w, int h, int r);
    void DrawGradientCircle(Gdiplus::Graphics& g, int cx, int cy, int radius,
                            Gdiplus::Color inner, Gdiplus::Color outer);

    void OnMouseMove(int x, int y);
    void OnMouseDown(int x, int y);
    void OnMouseUp(int x, int y);
    void OnMouseWheel(int delta, int x, int y);
    LRESULT OnNcHitTest(int x, int y);

    void DetectZapretPaths();
    void EnsureUserLists();
    void ToggleConnection();
    void Connect();
    void Disconnect();
    void StartWinws(const Strategy& strategy);
    void StopWinws();
    bool IsWinwsRunning();
    void CheckConnectionStatus();
    std::wstring BuildArguments(const Strategy& strategy);
    void ApplyGameFilterMode();
    void ApplyIPSetMode();
    IPSetMode DetectIPSetMode();
    void OpenSettings();
    void SelectFolder();
    void ToggleMaximize();
    void SaveConfig();
    void LoadConfig();
    void OpenConfigFile();
    void OpenGitHub();
    void UpdateHostsFile();
    bool DownloadFile(const std::wstring& url, const std::wstring& savePath);
    bool DownloadToString(const std::wstring& url, std::string& result);
    bool CopyTextToClipboard(const std::wstring& text);

    void OpenTestWindow();
    static LRESULT CALLBACK TestWndProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT HandleTestMessage(UINT msg, WPARAM wParam, LPARAM lParam);
    void DrawTestWindow(HDC hdc, int w, int h);
    void OnTestMouseMove(int x, int y);
    void OnTestClick(int x, int y);
    void OnTestWheel(int delta);
    void StartTest();
    void CancelTest();
    void RunTestThread();
    void RestoreConnectionState();
    std::vector<std::wstring> LoadTestDomains();
    bool TestDomain(const std::wstring& domain, int& pingMs);
    std::wstring ResultToString(TestResult r);
    void ExportTestResults();

    void SetAutoStart(bool enable);
    bool IsAutoStartEnabled();
    void HandleAutoConnect();

    static LRESULT CALLBACK SettingsWndProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT HandleSettingsMessage(UINT msg, WPARAM wParam, LPARAM lParam);
    void DrawSettingsWindow(HDC hdc, int w, int h);
    void OnSettingsClick(int x, int y);
    void OnSettingsMouseMove(int x, int y);
    void OnSettingsWheel(int delta);
    int GetSettingsItemAt(int x, int y);
    int GetSettingsContentHeight();

    bool PtInCircle(int px, int py, int cx, int cy, int r);
    bool PtInRect(int px, int py, int rx, int ry, int rw, int rh);
    RECT GetMinBtnRect(int w);
    RECT GetMaxBtnRect(int w);
    RECT GetCloseBtnRect(int w);
    std::wstring ReplaceAll(std::wstring str, const std::wstring& from, const std::wstring& to);
};
#include "gui.h"

#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")

using std::min;
using std::max;

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "ole32.lib")

ZapretGUI::ZapretGUI(HINSTANCE hInstance) : m_hInstance(hInstance)
{
    Gdiplus::GdiplusStartupInput gsi;
    Gdiplus::GdiplusStartup(&m_gdiplusToken, &gsi, nullptr);

    INITCOMMONCONTROLSEX icex = { sizeof(icex), ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icex);

    m_hAppIcon = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_APPICON),
                                    IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);

    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    m_zapretPath = fs::path(exePath).parent_path();

    m_strategies = GetAllStrategies();

    LoadLogoBitmap();
    LoadConfig();
    DetectZapretPaths();
    ApplyGameFilterMode();
    if (m_pathsValid) m_settings.ipsetMode = DetectIPSetMode();
}
ZapretGUI::~ZapretGUI()
{
    if (m_testPhase != TestPhase::Idle && m_testPhase != TestPhase::Finished &&
        m_testPhase != TestPhase::Cancelled)
    {
        m_testCancelRequested = true;
    }

    if (m_testThread.joinable())
    {
        m_testThread.join();
    }

    m_wasConnected = IsWinwsRunning();
    SaveConfig();

    if (m_logoBitmap) delete m_logoBitmap;
    Gdiplus::GdiplusShutdown(m_gdiplusToken);
}

void ZapretGUI::LoadLogoBitmap()
{
    fs::path logoPath = m_zapretPath / L"Assets" / L"icon.png";
    if (fs::exists(logoPath))
    {
        m_logoBitmap = new Gdiplus::Bitmap(logoPath.wstring().c_str());
        if (m_logoBitmap->GetLastStatus() != Gdiplus::Ok)
        {
            delete m_logoBitmap;
            m_logoBitmap = nullptr;
        }
    }
}

int ZapretGUI::Run(int nCmdShow)
{
    RegisterWindowClass();

    bool silentStart = false;
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    for (int i = 1; i < argc; i++)
    {
        if (std::wstring(argv[i]) == L"--autostart") silentStart = true;
    }
    LocalFree(argv);

    if (silentStart)
    {
        CreateMainWindow(SW_SHOWMINIMIZED);
        HandleAutoConnect();
    }
    else
    {
        CreateIntroWindow();
    }

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

float ZapretGUI::EaseOutCubic(float t) { t = t - 1.0f; return t * t * t + 1.0f; }
float ZapretGUI::EaseInOutCubic(float t) { return t < 0.5f ? 4.0f * t * t * t : 1.0f - powf(-2.0f * t + 2.0f, 3.0f) / 2.0f; }
float ZapretGUI::EaseOutBack(float t)
{
    const float c1 = 1.70158f, c3 = c1 + 1.0f;
    return 1.0f + c3 * powf(t - 1.0f, 3.0f) + c1 * powf(t - 1.0f, 2.0f);
}
float ZapretGUI::EaseOutExpo(float t) { return t >= 1.0f ? 1.0f : 1.0f - powf(2.0f, -10.0f * t); }

void ZapretGUI::CreateIntroWindow()
{
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = IntroWndProc;
    wc.hInstance = m_hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = L"ZapretIntro";
    wc.hIcon = m_hAppIcon;
    RegisterClassExW(&wc);

    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);

    m_hIntroWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED,
        L"ZapretIntro", L"", WS_POPUP,
        0, 0, sw, sh, nullptr, nullptr, m_hInstance, this);

    SetLayeredWindowAttributes(m_hIntroWnd, 0, 0, LWA_ALPHA);
    ShowWindow(m_hIntroWnd, SW_SHOW);

    m_introStart = std::chrono::steady_clock::now();
    m_introTimer = SetTimer(m_hIntroWnd, 100, 16, nullptr);
}

LRESULT CALLBACK ZapretGUI::IntroWndProc(HWND hWnd, UINT msg, WPARAM w, LPARAM l)
{
    ZapretGUI* p = nullptr;
    if (msg == WM_CREATE)
    {
        auto cs = reinterpret_cast<CREATESTRUCT*>(l);
        p = reinterpret_cast<ZapretGUI*>(cs->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(p));
        p->m_hIntroWnd = hWnd;
    }
    else p = reinterpret_cast<ZapretGUI*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (p) return p->HandleIntroMessage(msg, w, l);
    return DefWindowProc(hWnd, msg, w, l);
}

LRESULT ZapretGUI::HandleIntroMessage(UINT msg, WPARAM w, LPARAM l)
{
    switch (msg)
    {
    case WM_PAINT: OnIntroPaint(); return 0;
    case WM_ERASEBKGND: return 1;
    case WM_TIMER:
        if (w == 100)
        {
            auto now = std::chrono::steady_clock::now();
            m_introTime = std::chrono::duration<float>(now - m_introStart).count();
            UpdateIntroPhase();
            InvalidateRect(m_hIntroWnd, nullptr, FALSE);
        }
        return 0;
    case WM_KEYDOWN:
    case WM_LBUTTONDOWN:
        EndIntro();
        return 0;
    }
    return DefWindowProc(m_hIntroWnd, msg, w, l);
}

void ZapretGUI::UpdateIntroPhase()
{
    float t = m_introTime;
    if (t < 0.5f) m_introPhase = IntroPhase::FadeInBackground;
    else if (t < 1.5f) m_introPhase = IntroPhase::FadeInZ;
    else if (t < 2.0f) m_introPhase = IntroPhase::HoldZ;
    else if (t < 2.8f) m_introPhase = IntroPhase::ZMovesLeft;
    else if (t < 3.6f) m_introPhase = IntroPhase::ApretFliesUp;
    else if (t < 4.6f) m_introPhase = IntroPhase::ApretGlow;
    else if (t < 6.6f) m_introPhase = IntroPhase::HoldFull;
    else if (t < 7.4f) m_introPhase = IntroPhase::FadeOut;
    else { m_introPhase = IntroPhase::Done; EndIntro(); }
}

void ZapretGUI::OnIntroPaint()
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(m_hIntroWnd, &ps);
    RECT cr; GetClientRect(m_hIntroWnd, &cr);
    int w = cr.right, h = cr.bottom;

    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBmp = CreateCompatibleBitmap(hdc, w, h);
    HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

    Gdiplus::Graphics g(memDC);
    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);

    DrawIntroFrame(g, w, h);

    BitBlt(hdc, 0, 0, w, h, memDC, 0, 0, SRCCOPY);
    SelectObject(memDC, oldBmp);
    DeleteObject(memBmp);
    DeleteDC(memDC);
    EndPaint(m_hIntroWnd, &ps);
}

void ZapretGUI::DrawIntroFrame(Gdiplus::Graphics& g, int w, int h)
{
    float t = m_introTime;

    float bgAlpha = 0.0f;
    if (t < 0.5f) bgAlpha = (t / 0.5f) * 220.0f;
    else if (t >= 6.6f && t < 7.4f) bgAlpha = (1.0f - EaseInOutCubic((t - 6.6f) / 0.8f)) * 220.0f;
    else if (t >= 7.4f) bgAlpha = 0.0f;
    else bgAlpha = 220.0f;

    int windowAlpha = (int)std::min(255.0f, std::max(0.0f, bgAlpha + 35.0f));
    if (m_introPhase == IntroPhase::Done) windowAlpha = 0;
    SetLayeredWindowAttributes(m_hIntroWnd, 0, (BYTE)std::min(255, std::max(0, windowAlpha)), LWA_ALPHA);

    Gdiplus::SolidBrush bgBrush(Gdiplus::Color((int)bgAlpha, 4, 10, 8));
    g.FillRectangle(&bgBrush, 0, 0, w, h);

    if (bgAlpha > 50)
    {
        Gdiplus::GraphicsPath radialPath;
        radialPath.AddEllipse(w / 2 - 400, h / 2 - 400, 800, 800);
        Gdiplus::PathGradientBrush radialBrush(&radialPath);
        radialBrush.SetCenterColor(Gdiplus::Color((int)(bgAlpha * 0.25f), 46, 222, 162));
        Gdiplus::Color surr[] = { Gdiplus::Color(0, 0, 0, 0) };
        int count = 1;
        radialBrush.SetSurroundColors(surr, &count);
        radialBrush.SetCenterPoint(Gdiplus::PointF((float)(w / 2), (float)(h / 2)));
        g.FillEllipse(&radialBrush, w / 2 - 400, h / 2 - 400, 800, 800);
    }

    if (bgAlpha > 80)
    {
        int gridAlpha = (int)(bgAlpha * 0.08f);
        Gdiplus::Pen gridPen(Gdiplus::Color(gridAlpha, 46, 222, 162), 0.5f);
        for (int x = 0; x < w; x += 80) g.DrawLine(&gridPen, x, 0, x, h);
        for (int y = 0; y < h; y += 80) g.DrawLine(&gridPen, 0, y, w, y);
    }

    Gdiplus::FontFamily ff(L"Segoe UI");
    float fontSize = 96.0f;
    Gdiplus::Font measureFont(&ff, fontSize, Gdiplus::FontStyleBold);
    Gdiplus::RectF zBounds, apretBounds, zapretBounds;
    g.MeasureString(L"Z", -1, &measureFont, Gdiplus::PointF(0, 0), &zBounds);
    g.MeasureString(L"APRET", -1, &measureFont, Gdiplus::PointF(0, 0), &apretBounds);
    g.MeasureString(L"ZAPRET", -1, &measureFont, Gdiplus::PointF(0, 0), &zapretBounds);

    float zWidth = zBounds.Width, zHeight = zBounds.Height;
    float cx = w / 2.0f, cy = h / 2.0f;
    float finalZapretX = cx - zapretBounds.Width / 2.0f;
    float finalZX = finalZapretX;
    float finalApretX = finalZapretX + zWidth - 18.0f;
    float textY = cy - zHeight / 2.0f - 10.0f;

    float zAlpha = 0.0f, zGlow = 0.0f, zX = cx - zWidth / 2.0f, zY = textY, zScale = 1.0f;
    if (t < 0.5f) zAlpha = 0.0f;
    else if (t < 1.5f)
    {
        float p = (t - 0.5f) / 1.0f;
        zAlpha = EaseOutExpo(p); zGlow = EaseOutCubic(p) * 0.85f;
        zX = cx - zWidth / 2.0f;
        zScale = 0.8f + 0.2f * EaseOutBack(std::min(1.0f, p * 1.3f));
    }
    else if (t < 2.0f) { zAlpha = 1.0f; zGlow = 0.8f + 0.2f * sinf(t * 3.0f); zX = cx - zWidth / 2.0f; }
    else if (t < 2.8f)
    {
        float p = (t - 2.0f) / 0.8f, ease = EaseInOutCubic(p);
        float startX = cx - zWidth / 2.0f;
        zX = startX + (finalZX - startX) * ease;
        zAlpha = 1.0f; zGlow = 0.8f;
    }
    else if (t < 6.6f) { zX = finalZX; zAlpha = 1.0f; zGlow = 0.7f + 0.3f * sinf(t * 2.0f); }
    else if (t < 7.4f)
    {
        float p = (t - 6.6f) / 0.8f;
        zX = finalZX; zAlpha = 1.0f - EaseInOutCubic(p);
        zGlow = (0.7f + 0.3f * sinf(t * 2.0f)) * (1.0f - p);
    }

    if (zAlpha > 0.01f)
    {
        Gdiplus::GraphicsState state = g.Save();
        float zCenterX = zX + zWidth / 2.0f, zCenterY = textY + zHeight / 2.0f;
        g.TranslateTransform(zCenterX, zCenterY);
        g.ScaleTransform(zScale, zScale);
        g.TranslateTransform(-zCenterX, -zCenterY);
        DrawGlowingText(g, L"Z", zX, zY, fontSize, zAlpha, zGlow, Colors::Accent());
        g.Restore(state);
    }

    float apretAlpha = 0.0f, apretGlow = 0.0f, apretX = finalApretX, apretY = textY, apretColorBlend = 0.0f;
    if (t < 2.8f) apretAlpha = 0.0f;
    else if (t < 3.6f)
    {
        float p = (t - 2.8f) / 0.8f, ease = EaseOutCubic(p);
        apretAlpha = ease; apretY = textY + (h * 0.3f) * (1.0f - ease);
    }
    else if (t < 4.6f)
    {
        float p = (t - 3.6f) / 1.0f, ease = EaseInOutCubic(p);
        apretAlpha = 1.0f; apretGlow = ease * 0.8f; apretColorBlend = ease;
    }
    else if (t < 6.6f) { apretAlpha = 1.0f; apretGlow = 0.7f + 0.3f * sinf(t * 2.0f); apretColorBlend = 1.0f; }
    else if (t < 7.4f)
    {
        float p = (t - 6.6f) / 0.8f;
        apretAlpha = 1.0f - EaseInOutCubic(p);
        apretGlow = (0.7f + 0.3f * sinf(t * 2.0f)) * (1.0f - p);
        apretColorBlend = 1.0f;
    }

    if (apretAlpha > 0.01f)
    {
        int r = (int)(255 + (46 - 255) * apretColorBlend);
        int gv = (int)(255 + (222 - 255) * apretColorBlend);
        int b = (int)(255 + (162 - 255) * apretColorBlend);
        DrawGlowingText(g, L"APRET", apretX, apretY, fontSize, apretAlpha, apretGlow, Gdiplus::Color(255, r, gv, b));
    }

    if (t >= 4.6f && t < 7.4f)
    {
        float subAlpha = 0.0f;
        if (t < 5.2f) subAlpha = EaseOutCubic((t - 4.6f) / 0.6f);
        else if (t >= 6.6f) subAlpha = 1.0f - EaseInOutCubic((t - 6.6f) / 0.8f);
        else subAlpha = 1.0f;

        if (subAlpha > 0.01f)
        {
            Gdiplus::Font subFont(&ff, 14, Gdiplus::FontStyleRegular);
            Gdiplus::SolidBrush subBrush(Gdiplus::Color((int)(subAlpha * 170), 120, 200, 170));
            Gdiplus::StringFormat sf;
            sf.SetAlignment(Gdiplus::StringAlignmentCenter);
            Gdiplus::RectF subRect(0, textY + zHeight + 5, (float)w, 40);
            g.DrawString(L"G U I   \u00B7   \u041E\u0431\u0445\u043E\u0434 D P I", -1, &subFont, subRect, &sf, &subBrush);
        }
    }
}

void ZapretGUI::DrawGlowingText(Gdiplus::Graphics& g, const std::wstring& text,
                                  float x, float y, float fontSize, float alpha,
                                  float glowIntensity, Gdiplus::Color baseColor)
{
    Gdiplus::FontFamily ff(L"Segoe UI");
    Gdiplus::Font font(&ff, fontSize, Gdiplus::FontStyleBold);

    if (glowIntensity > 0.01f)
    {
        for (int i = 8; i >= 1; i--)
        {
            int glowAlpha = std::min(255, std::max(0, (int)(alpha * glowIntensity * 14.0f * (float)(9 - i) / 8.0f)));
            float offset = (float)i * 2.5f;
            Gdiplus::SolidBrush glowBrush(Gdiplus::Color(glowAlpha, baseColor.GetR(), baseColor.GetG(), baseColor.GetB()));
            for (int d = 0; d < 8; d++)
            {
                float angle = d * 3.14159f / 4.0f;
                g.DrawString(text.c_str(), -1, &font,
                             Gdiplus::PointF(x + cosf(angle) * offset, y + sinf(angle) * offset), &glowBrush);
            }
        }
        int innerAlpha = std::min(200, std::max(0, (int)(alpha * glowIntensity * 70.0f)));
        for (int d = 0; d < 8; d++)
        {
            float angle = d * 3.14159f / 4.0f;
            Gdiplus::SolidBrush innerBrush(Gdiplus::Color(innerAlpha, 180, 255, 220));
            g.DrawString(text.c_str(), -1, &font,
                         Gdiplus::PointF(x + cosf(angle) * 1.5f, y + sinf(angle) * 1.5f), &innerBrush);
        }
    }

    int mainAlpha = std::min(255, std::max(0, (int)(alpha * 255)));
    Gdiplus::SolidBrush mainBrush(Gdiplus::Color(mainAlpha, baseColor.GetR(), baseColor.GetG(), baseColor.GetB()));
    g.DrawString(text.c_str(), -1, &font, Gdiplus::PointF(x, y), &mainBrush);

    if (glowIntensity > 0.3f)
    {
        int brightAlpha = std::min(140, std::max(0, (int)(alpha * (glowIntensity - 0.3f) * 120.0f)));
        Gdiplus::SolidBrush brightBrush(Gdiplus::Color(brightAlpha, 220, 255, 235));
        g.DrawString(text.c_str(), -1, &font, Gdiplus::PointF(x, y), &brightBrush);
    }
}

void ZapretGUI::EndIntro()
{
    if (!m_introActive) return;
    m_introActive = false;
    if (m_introTimer) { KillTimer(m_hIntroWnd, m_introTimer); m_introTimer = 0; }
    DestroyWindow(m_hIntroWnd);
    m_hIntroWnd = nullptr;
    CreateMainWindow(SW_SHOW);
    HandleAutoConnect();
}

void ZapretGUI::RegisterWindowClass()
{
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = m_hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = L"ZapretGUI";
    wc.hIcon = m_hAppIcon;
    wc.hIconSm = m_hAppIcon;
    RegisterClassExW(&wc);
}

void ZapretGUI::CreateMainWindow(int nCmdShow)
{
    m_hWnd = CreateWindowExW(
        0, L"ZapretGUI", L"Zapret Gui",
        WS_POPUP | WS_THICKFRAME | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_CAPTION,
        CW_USEDEFAULT, CW_USEDEFAULT, 1000, 640,
        nullptr, nullptr, m_hInstance, this);

    SetWindowLong(m_hWnd, GWL_STYLE, GetWindowLong(m_hWnd, GWL_STYLE) & ~WS_CAPTION);

    if (m_hAppIcon)
    {
        SendMessage(m_hWnd, WM_SETICON, ICON_BIG, (LPARAM)m_hAppIcon);
        SendMessage(m_hWnd, WM_SETICON, ICON_SMALL, (LPARAM)m_hAppIcon);
    }

    SetWindowPos(m_hWnd, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);

    m_animTimer = SetTimer(m_hWnd, 1, 16, nullptr);
    m_processCheckTimer = SetTimer(m_hWnd, 2, 1500, nullptr);

    ShowWindow(m_hWnd, nCmdShow);
    UpdateWindow(m_hWnd);
}

LRESULT CALLBACK ZapretGUI::WndProc(HWND hWnd, UINT msg, WPARAM w, LPARAM l)
{
    ZapretGUI* p = nullptr;
    if (msg == WM_CREATE)
    {
        auto cs = reinterpret_cast<CREATESTRUCT*>(l);
        p = reinterpret_cast<ZapretGUI*>(cs->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(p));
        p->m_hWnd = hWnd;
    }
    else p = reinterpret_cast<ZapretGUI*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (p) return p->HandleMessage(msg, w, l);
    return DefWindowProc(hWnd, msg, w, l);
}

LRESULT ZapretGUI::HandleMessage(UINT msg, WPARAM w, LPARAM l)
{
    switch (msg)
    {
    case WM_NCCALCSIZE: if (w == TRUE) return 0; break;
    case WM_NCHITTEST: return OnNcHitTest(GET_X_LPARAM(l), GET_Y_LPARAM(l));
    case WM_PAINT: OnPaint(); return 0;
    case WM_ERASEBKGND: return 1;
    case WM_MOUSEMOVE: OnMouseMove(GET_X_LPARAM(l), GET_Y_LPARAM(l)); return 0;
    case WM_LBUTTONDOWN: OnMouseDown(GET_X_LPARAM(l), GET_Y_LPARAM(l)); return 0;
    case WM_LBUTTONUP: OnMouseUp(GET_X_LPARAM(l), GET_Y_LPARAM(l)); return 0;
    case WM_MOUSEWHEEL:
    {
        POINT pt = { GET_X_LPARAM(l), GET_Y_LPARAM(l) };
        ScreenToClient(m_hWnd, &pt);
        OnMouseWheel(GET_WHEEL_DELTA_WPARAM(w), pt.x, pt.y);
        return 0;
    }
    case WM_TIMER:
        if (w == 1)
        {
            m_globalAnim += 0.016f;
            if (m_globalAnim > 1000.0f) m_globalAnim = 0;
            if (m_connected || m_connectBtnHovered)
            {
                m_pulseAnim += 0.05f;
                if (m_pulseAnim > 6.28f) m_pulseAnim = 0;
            }
            InvalidateRect(m_hWnd, nullptr, FALSE);
        }
        else if (w == 2) CheckConnectionStatus();
        return 0;
    case WM_SIZE: m_isMaximized = (w == SIZE_MAXIMIZED); InvalidateRect(m_hWnd, nullptr, FALSE); return 0;
    case WM_GETMINMAXINFO:
    {
        auto mmi = reinterpret_cast<MINMAXINFO*>(l);
        mmi->ptMinTrackSize.x = 880; mmi->ptMinTrackSize.y = 560;
        return 0;
    }
    case WM_NCLBUTTONDBLCLK: if (w == HTCAPTION) { ToggleMaximize(); return 0; } break;
    case WM_DESTROY:
        if (m_animTimer) KillTimer(m_hWnd, m_animTimer);
        if (m_processCheckTimer) KillTimer(m_hWnd, m_processCheckTimer);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(m_hWnd, msg, w, l);
}

LRESULT ZapretGUI::OnNcHitTest(int x, int y)
{
    RECT wr; GetWindowRect(m_hWnd, &wr);
    int localX = x - wr.left, localY = y - wr.top;
    int w = wr.right - wr.left, h = wr.bottom - wr.top;
    const int BORDER = 6;

    RECT minBtn = GetMinBtnRect(w), maxBtn = GetMaxBtnRect(w), closeBtn = GetCloseBtnRect(w);
    if (PtInRect(localX, localY, minBtn.left, minBtn.top, minBtn.right - minBtn.left, minBtn.bottom - minBtn.top)) return HTCLIENT;
    if (PtInRect(localX, localY, maxBtn.left, maxBtn.top, maxBtn.right - maxBtn.left, maxBtn.bottom - maxBtn.top)) return HTCLIENT;
    if (PtInRect(localX, localY, closeBtn.left, closeBtn.top, closeBtn.right - closeBtn.left, closeBtn.bottom - closeBtn.top)) return HTCLIENT;

    if (!m_isMaximized)
    {
        if (localX < BORDER && localY < BORDER) return HTTOPLEFT;
        if (localX > w - BORDER && localY < BORDER) return HTTOPRIGHT;
        if (localX < BORDER && localY > h - BORDER) return HTBOTTOMLEFT;
        if (localX > w - BORDER && localY > h - BORDER) return HTBOTTOMRIGHT;
        if (localY < BORDER) return HTTOP;
        if (localY > h - BORDER) return HTBOTTOM;
        if (localX < BORDER) return HTLEFT;
        if (localX > w - BORDER) return HTRIGHT;
    }
    if (localY < TITLEBAR_H) return HTCAPTION;
    return HTCLIENT;
}

void ZapretGUI::OnPaint()
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(m_hWnd, &ps);
    RECT cr; GetClientRect(m_hWnd, &cr);
    int w = cr.right, h = cr.bottom;

    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBmp = CreateCompatibleBitmap(hdc, w, h);
    HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

    Gdiplus::Graphics g(memDC);
    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

    DrawBackground(g, w, h);
    DrawTitleBar(g, w);
    DrawTopBar(g, w);
    DrawCenter(g, w, h);
    DrawSidebar(g, w, h);

    BitBlt(hdc, 0, 0, w, h, memDC, 0, 0, SRCCOPY);
    SelectObject(memDC, oldBmp);
    DeleteObject(memBmp);
    DeleteDC(memDC);
    EndPaint(m_hWnd, &ps);
}

void ZapretGUI::DrawBackground(Gdiplus::Graphics& g, int w, int h)
{
    Gdiplus::SolidBrush bg(Colors::BgDark());
    g.FillRectangle(&bg, 0, 0, w, h);

    int centerW = w - SIDEBAR_W;
    int cx = centerW / 2, cy = h / 2, gr = 400;
    Gdiplus::GraphicsPath radialPath;
    radialPath.AddEllipse(cx - gr, cy - gr, gr * 2, gr * 2);
    Gdiplus::PathGradientBrush radialBrush(&radialPath);
    int intensity = m_connected ? 50 : 25;
    radialBrush.SetCenterColor(Gdiplus::Color(intensity, 46, 222, 162));
    Gdiplus::Color surround[] = { Gdiplus::Color(0, 0, 0, 0) };
    int count = 1;
    radialBrush.SetSurroundColors(surround, &count);
    radialBrush.SetCenterPoint(Gdiplus::PointF((float)cx, (float)cy));
    g.FillEllipse(&radialBrush, cx - gr, cy - gr, gr * 2, gr * 2);

    Gdiplus::Pen gridPen(Gdiplus::Color(8, 46, 222, 162), 0.5f);
    for (int x = 0; x < w; x += 50) g.DrawLine(&gridPen, x, TITLEBAR_H + TOPBAR_H, x, h);
    for (int y = TITLEBAR_H + TOPBAR_H; y < h; y += 50) g.DrawLine(&gridPen, 0, y, w - SIDEBAR_W, y);
}

void ZapretGUI::DrawTitleBar(Gdiplus::Graphics& g, int w)
{
    Gdiplus::SolidBrush bg(Colors::BgTitle());
    g.FillRectangle(&bg, 0, 0, w, TITLEBAR_H);
    Gdiplus::Pen border(Gdiplus::Color(255, 20, 35, 28), 1);
    g.DrawLine(&border, 0, TITLEBAR_H, w, TITLEBAR_H);

    int logoSize = 22, logoX = 10, logoY = (TITLEBAR_H - logoSize) / 2;

    if (m_logoBitmap)
    {
        g.DrawImage(m_logoBitmap, logoX, logoY, logoSize, logoSize);
    }
    else
    {
        Gdiplus::SolidBrush fallbackBg(Colors::Accent());
        FillRoundRect(g, &fallbackBg, logoX, logoY, logoSize, logoSize, 4);

        Gdiplus::FontFamily ff(L"Segoe UI");
        Gdiplus::Font zFont(&ff, 12, Gdiplus::FontStyleBold);
        Gdiplus::SolidBrush zBrush(Gdiplus::Color(255, 8, 14, 12));
        Gdiplus::StringFormat sf;
        sf.SetAlignment(Gdiplus::StringAlignmentCenter);
        sf.SetLineAlignment(Gdiplus::StringAlignmentCenter);
        Gdiplus::RectF zRect((float)logoX, (float)logoY, (float)logoSize, (float)logoSize);
        g.DrawString(L"Z", -1, &zFont, zRect, &sf, &zBrush);
    }

    Gdiplus::FontFamily ff(L"Segoe UI");
    Gdiplus::Font titleFont(&ff, 10, Gdiplus::FontStyleRegular);
    Gdiplus::SolidBrush titleBrush(Colors::Text());
    g.DrawString(L"Zapret Gui", -1, &titleFont,
                 Gdiplus::PointF((float)(logoX + logoSize + 10), 10), &titleBrush);

    RECT minBtn = GetMinBtnRect(w), maxBtn = GetMaxBtnRect(w), closeBtn = GetCloseBtnRect(w);
    DrawTitleButton(g, minBtn.left, minBtn.top, minBtn.right - minBtn.left, minBtn.bottom - minBtn.top, 0, m_minBtnHovered);
    DrawTitleButton(g, maxBtn.left, maxBtn.top, maxBtn.right - maxBtn.left, maxBtn.bottom - maxBtn.top, 1, m_maxBtnHovered);
    DrawTitleButton(g, closeBtn.left, closeBtn.top, closeBtn.right - closeBtn.left, closeBtn.bottom - closeBtn.top, 2, m_closeBtnHovered);
}

void ZapretGUI::DrawTitleButton(Gdiplus::Graphics& g, int x, int y, int w, int h, int type, bool hovered)
{
    if (hovered)
    {
        Gdiplus::Color hoverColor = (type == 2) ? Gdiplus::Color(255, 232, 17, 35) : Colors::BgHover();
        Gdiplus::SolidBrush hb(hoverColor);
        g.FillRectangle(&hb, x, y, w, h);
    }

    int cx = x + w / 2, cy = y + h / 2;
    Gdiplus::Color iconColor = (hovered && type == 2) ? Gdiplus::Color(255, 255, 255, 255) : Colors::Text();
    Gdiplus::Pen pen(iconColor, 1.2f);

    if (type == 0) g.DrawLine(&pen, cx - 5, cy + 4, cx + 5, cy + 4);
    else if (type == 1)
    {
        if (m_isMaximized) { g.DrawRectangle(&pen, cx - 3, cy - 5, 7, 7); g.DrawRectangle(&pen, cx - 5, cy - 3, 7, 7); }
        else g.DrawRectangle(&pen, cx - 5, cy - 5, 10, 10);
    }
    else if (type == 2)
    {
        g.DrawLine(&pen, cx - 5, cy - 5, cx + 5, cy + 5);
        g.DrawLine(&pen, cx + 5, cy - 5, cx - 5, cy + 5);
    }
}

RECT ZapretGUI::GetMinBtnRect(int w) { return { w - 138, 0, w - 92, TITLEBAR_H }; }
RECT ZapretGUI::GetMaxBtnRect(int w) { return { w - 92, 0, w - 46, TITLEBAR_H }; }
RECT ZapretGUI::GetCloseBtnRect(int w) { return { w - 46, 0, w, TITLEBAR_H }; }

void ZapretGUI::DrawTopBar(Gdiplus::Graphics& g, int w)
{
    int yOff = TITLEBAR_H;
    Gdiplus::SolidBrush panel(Gdiplus::Color(230, 12, 20, 17));
    g.FillRectangle(&panel, 0, yOff, w, TOPBAR_H);
    Gdiplus::Pen border(Colors::Border(), 1);
    g.DrawLine(&border, 0, yOff + TOPBAR_H, w, yOff + TOPBAR_H);

    int iconSize = 36, iconX = 18, iconY = yOff + (TOPBAR_H - iconSize) / 2;
    if (m_logoBitmap)
    {
        for (int i = 3; i >= 1; i--)
        {
            Gdiplus::SolidBrush glowBrush(Gdiplus::Color(15 - i * 3, 46, 222, 162));
            g.FillEllipse(&glowBrush, iconX - i * 2, iconY - i * 2, iconSize + i * 4, iconSize + i * 4);
        }
        g.DrawImage(m_logoBitmap, iconX, iconY, iconSize, iconSize);
    }

    Gdiplus::FontFamily ff(L"Segoe UI");
    Gdiplus::Font titleFont(&ff, 18, Gdiplus::FontStyleBold);
    int textX = iconX + iconSize + 12;

    for (int i = 2; i >= 1; i--)
    {
        Gdiplus::SolidBrush gb(Gdiplus::Color(12 * i, 46, 222, 162));
        for (int d = 0; d < 4; d++)
        {
            float a = d * 3.14159f / 2.0f;
            g.DrawString(L"Zapret", -1, &titleFont,
                         Gdiplus::PointF((float)(textX + cosf(a) * i), (float)(yOff + 6 + sinf(a) * i)), &gb);
        }
    }
    Gdiplus::SolidBrush accBrush(Colors::Accent());
    g.DrawString(L"Zapret", -1, &titleFont, Gdiplus::PointF((float)textX, (float)(yOff + 6)), &accBrush);

    Gdiplus::Font subFont(&ff, 9, Gdiplus::FontStyleBold);
    Gdiplus::SolidBrush subBrush(Gdiplus::Color(255, 100, 200, 160));
    g.DrawString(L"G U I", -1, &subFont, Gdiplus::PointF((float)(textX + 2), (float)(yOff + 34)), &subBrush);

    Gdiplus::RectF zapretBounds;
    g.MeasureString(L"Zapret", -1, &titleFont, Gdiplus::PointF(0, 0), &zapretBounds);
    int pillStartX = textX + (int)zapretBounds.Width + 25;

    Gdiplus::Font statusFont(&ff, 10, Gdiplus::FontStyleBold);
    std::wstring statusText = m_connected
        ? L"\u041F\u041E\u0414\u041A\u041B\u042E\u0427\u0415\u041D\u041E"
        : L"\u041E\u0422\u041A\u041B\u042E\u0427\u0415\u041D\u041E";

    Gdiplus::RectF stBounds;
    g.MeasureString(statusText.c_str(), -1, &statusFont, Gdiplus::PointF(0, 0), &stBounds);
    int pillW = (int)stBounds.Width + 36, pillH = 26;
    int pillY = yOff + (TOPBAR_H - pillH) / 2;

    Gdiplus::Color pillBg = m_connected ? Gdiplus::Color(40, 46, 222, 162) : Gdiplus::Color(40, 248, 113, 113);
    Gdiplus::SolidBrush pillBrush(pillBg);
    FillRoundRect(g, &pillBrush, pillStartX, pillY, pillW, pillH, 13);
    Gdiplus::Color pillBorder = m_connected ? Colors::Accent() : Colors::Danger();
    Gdiplus::Pen pillPen(pillBorder, 1.5f);
    DrawRoundRect(g, &pillPen, pillStartX, pillY, pillW, pillH, 13);

    int dotSize = 10, dotX = pillStartX + 10, dotY = pillY + (pillH - dotSize) / 2;
    if (m_connected)
    {
        float pulse = (sinf(m_globalAnim * 3.0f) + 1.0f) * 0.5f;
        for (int i = 3; i >= 1; i--)
        {
            int alpha = (int)(20 + pulse * 30), extra = i * 2;
            Gdiplus::SolidBrush gb(Gdiplus::Color(alpha, 46, 222, 162));
            g.FillEllipse(&gb, dotX - extra, dotY - extra, dotSize + extra * 2, dotSize + extra * 2);
        }
    }
    Gdiplus::SolidBrush dotBrush(pillBorder);
    g.FillEllipse(&dotBrush, dotX, dotY, dotSize, dotSize);

    Gdiplus::SolidBrush stBrush(pillBorder);
    Gdiplus::StringFormat stSf;
    stSf.SetAlignment(Gdiplus::StringAlignmentNear);
    stSf.SetLineAlignment(Gdiplus::StringAlignmentCenter);
    Gdiplus::RectF stRect((float)(pillStartX + 26), (float)pillY, (float)(pillW - 30), (float)pillH);
    g.DrawString(statusText.c_str(), -1, &statusFont, stRect, &stSf, &stBrush);

    DrawSettingsButton(g, w - 50, yOff + (TOPBAR_H - 34) / 2, 34);
}

void ZapretGUI::DrawSettingsButton(Gdiplus::Graphics& g, int x, int y, int size)
{
    if (m_settingsBtnHovered)
    {
        Gdiplus::SolidBrush hb(Colors::BgHover());
        FillRoundRect(g, &hb, x, y, size, size, 8);
        Gdiplus::Pen bp(Colors::Accent(), 1);
        DrawRoundRect(g, &bp, x, y, size, size, 8);
    }

    int cx = x + size / 2, cy = y + size / 2;
    Gdiplus::Color iconColor = m_settingsBtnHovered ? Colors::Accent() : Colors::TextDim();
    Gdiplus::Pen pen(iconColor, 1.8f);

    for (int i = 0; i < 8; i++)
    {
        float a = i * 3.14159f / 4.0f;
        g.DrawLine(&pen, cx + 7.5f * cosf(a), cy + 7.5f * sinf(a),
                   cx + 10.5f * cosf(a), cy + 10.5f * sinf(a));
    }
    g.DrawEllipse(&pen, cx - 6, cy - 6, 12, 12);
    Gdiplus::SolidBrush ic(iconColor);
    g.FillEllipse(&ic, cx - 2, cy - 2, 4, 4);
}

void ZapretGUI::DrawCenter(Gdiplus::Graphics& g, int w, int h)
{
    int yOff = TITLEBAR_H + TOPBAR_H;
    int centerW = w - SIDEBAR_W;
    int cx = centerW / 2;
    int cy = yOff + (h - yOff) / 2 - 35;
    int radius = 95;

    DrawConnectButton(g, cx, cy, radius);

    Gdiplus::FontFamily ff(L"Segoe UI");
    Gdiplus::StringFormat sf;
    sf.SetAlignment(Gdiplus::StringAlignmentCenter);

    if (m_connected)
    {
        Gdiplus::Font boldFont(&ff, 18, Gdiplus::FontStyleBold);
        for (int i = 2; i >= 1; i--)
        {
            Gdiplus::SolidBrush gb(Gdiplus::Color(30 * i, 46, 222, 162));
            for (int d = 0; d < 4; d++)
            {
                float a = d * 3.14159f / 2.0f;
                Gdiplus::RectF gr((float)(cx - 200 + cosf(a) * i), (float)(cy + radius + 30 + sinf(a) * i), 400, 35);
                g.DrawString(L"\u0417\u0430\u0449\u0438\u0449\u0435\u043D\u043E", -1, &boldFont, gr, &sf, &gb);
            }
        }
        Gdiplus::SolidBrush sb(Colors::Accent());
        Gdiplus::RectF tr((float)(cx - 200), (float)(cy + radius + 30), 400, 35);
        g.DrawString(L"\u0417\u0430\u0449\u0438\u0449\u0435\u043D\u043E", -1, &boldFont, tr, &sf, &sb);
    }
    else
    {
        Gdiplus::Font textFont(&ff, 16, Gdiplus::FontStyleRegular);
        Gdiplus::SolidBrush db(Colors::Text());
        Gdiplus::RectF tr((float)(cx - 250), (float)(cy + radius + 30), 500, 30);
        g.DrawString(L"\u0413\u043E\u0442\u043E\u0432 \u043A \u043F\u043E\u0434\u043A\u043B\u044E\u0447\u0435\u043D\u0438\u044E", -1, &textFont, tr, &sf, &db);
    }

    Gdiplus::Font subFont(&ff, 11, Gdiplus::FontStyleRegular);
    Gdiplus::SolidBrush dimBrush(Colors::TextDim());

    if (m_selectedBatIndex >= 0 && m_selectedBatIndex < (int)m_strategies.size())
    {
        std::wstring txt = L"\u0421\u0442\u0440\u0430\u0442\u0435\u0433\u0438\u044F  \u00B7  " + m_strategies[m_selectedBatIndex].displayName;
        Gdiplus::RectF tr((float)(cx - 250), (float)(cy + radius + 68), 500, 25);
        g.DrawString(txt.c_str(), -1, &subFont, tr, &sf, &dimBrush);
    }
    else
    {
        Gdiplus::Font itFont(&ff, 11, Gdiplus::FontStyleItalic);
        Gdiplus::RectF tr((float)(cx - 250), (float)(cy + radius + 68), 500, 25);
        g.DrawString(L"\u2192  \u0412\u044B\u0431\u0435\u0440\u0438\u0442\u0435 \u0441\u0442\u0440\u0430\u0442\u0435\u0433\u0438\u044E \u0441\u043F\u0440\u0430\u0432\u0430", -1, &itFont, tr, &sf, &dimBrush);
    }
}

void ZapretGUI::DrawConnectButton(Gdiplus::Graphics& g, int cx, int cy, int radius)
{
    Gdiplus::Color innerColor, outerColor, borderColor, iconColor;

    if (m_connected)
    {
        innerColor = Colors::AccentBright(); outerColor = Colors::AccentDeep();
        borderColor = Colors::Accent(); iconColor = Gdiplus::Color(255, 8, 14, 12);
    }
    else if (m_connectBtnPressed)
    {
        innerColor = Colors::AccentDeep(); outerColor = Colors::AccentDeep();
        borderColor = Colors::AccentHover(); iconColor = Colors::Text();
    }
    else if (m_connectBtnHovered)
    {
        innerColor = Colors::Accent(); outerColor = Colors::AccentDeep();
        borderColor = Colors::AccentBright(); iconColor = Gdiplus::Color(255, 8, 14, 12);
    }
    else
    {
        innerColor = Colors::BgCard(); outerColor = Colors::BgPanel();
        borderColor = Colors::Accent(); iconColor = Colors::Accent();
    }

    if (m_connected)
    {
        float pulse = (sinf(m_pulseAnim) + 1.0f) * 0.5f;
        for (int i = 8; i >= 1; i--)
        {
            int alpha = (int)(5 + pulse * 8), extra = i * 7;
            Gdiplus::SolidBrush gl(Gdiplus::Color(alpha, 46, 222, 162));
            g.FillEllipse(&gl, cx - radius - extra, cy - radius - extra, (radius + extra) * 2, (radius + extra) * 2);
        }
        float ringR = radius + 10 + pulse * 8;
        int ringAlpha = (int)((1.0f - pulse) * 100);
        Gdiplus::Pen ringPen(Gdiplus::Color(ringAlpha, 46, 222, 162), 2.0f);
        g.DrawEllipse(&ringPen, cx - (int)ringR, cy - (int)ringR, (int)ringR * 2, (int)ringR * 2);
    }
    else if (m_connectBtnHovered)
    {
        for (int i = 5; i >= 1; i--)
        {
            int alpha = 7 * (6 - i), extra = i * 5;
            Gdiplus::SolidBrush gl(Gdiplus::Color(alpha, 46, 222, 162));
            g.FillEllipse(&gl, cx - radius - extra, cy - radius - extra, (radius + extra) * 2, (radius + extra) * 2);
        }
    }

    DrawGradientCircle(g, cx, cy, radius, innerColor, outerColor);

    Gdiplus::Pen borderPen(borderColor, 3.0f);
    g.DrawEllipse(&borderPen, cx - radius, cy - radius, radius * 2, radius * 2);

    Gdiplus::Pen innerPen(Gdiplus::Color(40, 255, 255, 255), 1.0f);
    g.DrawEllipse(&innerPen, cx - radius + 8, cy - radius + 8, (radius - 8) * 2, (radius - 8) * 2);

    DrawPowerIcon(g, cx, cy, 52, iconColor);
}

void ZapretGUI::DrawGradientCircle(Gdiplus::Graphics& g, int cx, int cy, int radius,
                                     Gdiplus::Color inner, Gdiplus::Color outer)
{
    Gdiplus::GraphicsPath path;
    path.AddEllipse(cx - radius, cy - radius, radius * 2, radius * 2);
    Gdiplus::PathGradientBrush brush(&path);
    brush.SetCenterColor(inner);
    Gdiplus::Color surround[] = { outer };
    int count = 1;
    brush.SetSurroundColors(surround, &count);
    brush.SetCenterPoint(Gdiplus::PointF((float)(cx - radius / 4), (float)(cy - radius / 4)));
    g.FillEllipse(&brush, cx - radius, cy - radius, radius * 2, radius * 2);
}

void ZapretGUI::DrawPowerIcon(Gdiplus::Graphics& g, int cx, int cy, int size, Gdiplus::Color color)
{
    Gdiplus::Pen pen(color, 5.5f);
    pen.SetLineCap(Gdiplus::LineCapRound, Gdiplus::LineCapRound, Gdiplus::DashCapRound);
    g.DrawLine(&pen, cx, cy - size / 2, cx, cy - size / 6);
    Gdiplus::GraphicsPath path;
    path.AddArc(cx - size / 2, cy - size / 2 + 4, size, size, -60, 300);
    g.DrawPath(&pen, &path);
}

void ZapretGUI::DrawSidebar(Gdiplus::Graphics& g, int w, int h)
{
    int yOff = TITLEBAR_H + TOPBAR_H;
    int sx = w - SIDEBAR_W;

    Gdiplus::SolidBrush panel(Colors::BgPanel());
    g.FillRectangle(&panel, sx, yOff, SIDEBAR_W, h - yOff);
    Gdiplus::Pen border(Colors::Border(), 1);
    g.DrawLine(&border, sx, yOff, sx, h);

    Gdiplus::FontFamily ff(L"Segoe UI");
    Gdiplus::Font hdr(&ff, 14, Gdiplus::FontStyleBold);
    Gdiplus::SolidBrush tb(Colors::Text());
    g.DrawString(L"\u0421\u0422\u0420\u0410\u0422\u0415\u0413\u0418\u0418", -1, &hdr,
                 Gdiplus::PointF((float)(sx + 20), (float)(yOff + 16)), &tb);

    std::wstring cntStr = std::to_wstring(m_strategies.size());
    Gdiplus::Font cf(&ff, 9, Gdiplus::FontStyleBold);
    Gdiplus::RectF cntBounds;
    g.MeasureString(cntStr.c_str(), -1, &cf, Gdiplus::PointF(0, 0), &cntBounds);
    int badgeW = std::max(22, (int)cntBounds.Width + 14);
    int badgeX = w - badgeW - 18, badgeY = yOff + 20;

    Gdiplus::SolidBrush badgeBrush(Gdiplus::Color(50, 46, 222, 162));
    FillRoundRect(g, &badgeBrush, badgeX, badgeY, badgeW, 18, 9);
    Gdiplus::Pen badgePen(Colors::Accent(), 1);
    DrawRoundRect(g, &badgePen, badgeX, badgeY, badgeW, 18, 9);
    Gdiplus::SolidBrush cntBrush(Colors::Accent());
    Gdiplus::StringFormat cntSf;
    cntSf.SetAlignment(Gdiplus::StringAlignmentCenter);
    cntSf.SetLineAlignment(Gdiplus::StringAlignmentCenter);
    Gdiplus::RectF cntRect((float)badgeX, (float)badgeY, (float)badgeW, 18);
    g.DrawString(cntStr.c_str(), -1, &cf, cntRect, &cntSf, &cntBrush);

    Gdiplus::LinearGradientBrush divBrush(
        Gdiplus::PointF((float)(sx + 16), 0), Gdiplus::PointF((float)(w - 16), 0),
        Gdiplus::Color(0, 46, 222, 162), Gdiplus::Color(80, 46, 222, 162));
    g.FillRectangle(&divBrush, sx + 16, yOff + 50, SIDEBAR_W - 32, 1);

    int listY = yOff + 60;
    int listH = h - listY - 65;

    Gdiplus::Region clip(Gdiplus::Rect(sx, listY, SIDEBAR_W, listH));
    g.SetClip(&clip);

    int itemH = 46;
    int totalH = (int)m_strategies.size() * itemH;
    int maxScroll = (totalH > listH) ? -(totalH - listH) : 0;
    if (m_sidebarScroll < maxScroll) m_sidebarScroll = maxScroll;
    if (m_sidebarScroll > 0) m_sidebarScroll = 0;

    for (int i = 0; i < (int)m_strategies.size(); i++)
    {
        int iy = listY + i * itemH + m_sidebarScroll;
        if (iy + itemH < listY || iy > listY + listH) continue;
        DrawBatItem(g, sx + 12, iy, SIDEBAR_W - 24, i);
    }

    g.ResetClip();

    if (totalH > listH)
    {
        int sbX = w - 6;
        int sbH = (int)((float)listH * listH / totalH);
        int sbY = listY + (int)((float)(-m_sidebarScroll) * listH / totalH);
        Gdiplus::SolidBrush sbBrush(Gdiplus::Color(120, 46, 222, 162));
        FillRoundRect(g, &sbBrush, sbX, sbY, 4, sbH, 2);
    }

    Gdiplus::LinearGradientBrush fadeBrush(
        Gdiplus::PointF(0, (float)(h - 80)), Gdiplus::PointF(0, (float)(h - 55)),
        Gdiplus::Color(0, 12, 20, 17), Colors::BgPanel());
    g.FillRectangle(&fadeBrush, sx, h - 80, SIDEBAR_W, 25);

    DrawFolderButton(g, sx + 16, h - 52, SIDEBAR_W - 32, 40);
}

void ZapretGUI::DrawBatItem(Gdiplus::Graphics& g, int x, int y, int w, int index)
{
    bool sel = (index == m_selectedBatIndex);
    bool hov = (index == m_hoveredBatIndex);
    auto& bat = m_strategies[index];
    int itemH = 40;

    if (sel)
    {
        Gdiplus::LinearGradientBrush gradBrush(
            Gdiplus::PointF((float)x, (float)y), Gdiplus::PointF((float)(x + w), (float)y),
            Gdiplus::Color(255, 28, 180, 130), Gdiplus::Color(255, 46, 222, 162));
        FillRoundRect(g, &gradBrush, x, y, w, itemH, 10);
        Gdiplus::Pen glowPen(Gdiplus::Color(120, 46, 222, 162), 1);
        DrawRoundRect(g, &glowPen, x - 1, y - 1, w + 2, itemH + 2, 11);
    }
    else if (hov)
    {
        Gdiplus::SolidBrush b(Colors::BgHover());
        FillRoundRect(g, &b, x, y, w, itemH, 10);
        Gdiplus::Pen p(Gdiplus::Color(60, 46, 222, 162), 1);
        DrawRoundRect(g, &p, x, y, w, itemH, 10);
    }

    int iconSize = 26, iconX = x + 9, iconY = y + (itemH - iconSize) / 2;
    if (sel)
    {
        Gdiplus::SolidBrush ib(Gdiplus::Color(80, 255, 255, 255));
        g.FillEllipse(&ib, iconX, iconY, iconSize, iconSize);
    }
    else
    {
        Gdiplus::GraphicsPath p;
        p.AddEllipse(iconX, iconY, iconSize, iconSize);
        Gdiplus::PathGradientBrush pgb(&p);
        pgb.SetCenterColor(Colors::Accent());
        Gdiplus::Color surr[] = { Colors::AccentDeep() };
        int cnt = 1;
        pgb.SetSurroundColors(surr, &cnt);
        g.FillEllipse(&pgb, iconX, iconY, iconSize, iconSize);
    }

    Gdiplus::FontFamily ff(L"Segoe UI");
    Gdiplus::Font numFont(&ff, 9, Gdiplus::FontStyleBold);
    Gdiplus::SolidBrush nb(sel ? Colors::Text() : Gdiplus::Color(255, 8, 14, 12));
    Gdiplus::StringFormat nsf;
    nsf.SetAlignment(Gdiplus::StringAlignmentCenter);
    nsf.SetLineAlignment(Gdiplus::StringAlignmentCenter);

    std::wstring numStr;
    if (bat.id == L"general") numStr = L"G";
    else
    {
        for (wchar_t c : bat.displayName) if (iswdigit(c)) numStr += c;
        if (numStr.empty()) numStr = L"A";
    }

    Gdiplus::RectF nr((float)iconX, (float)iconY, (float)iconSize, (float)iconSize);
    g.DrawString(numStr.c_str(), -1, &numFont, nr, &nsf, &nb);

    Gdiplus::Font nf(&ff, 11, Gdiplus::FontStyleBold);
    Gdiplus::SolidBrush textBrush(Colors::Text());
    g.DrawString(bat.displayName.c_str(), -1, &nf,
                 Gdiplus::PointF((float)(x + 45), (float)(y + 11)), &textBrush);

    if (sel)
    {
        Gdiplus::Pen cp(Colors::Text(), 2.5f);
        cp.SetLineCap(Gdiplus::LineCapRound, Gdiplus::LineCapRound, Gdiplus::DashCapRound);
        int chX = x + w - 26, chY = y + 20;
        g.DrawLine(&cp, chX, chY, chX + 5, chY + 5);
        g.DrawLine(&cp, chX + 5, chY + 5, chX + 13, chY - 6);
    }
}

void ZapretGUI::DrawFolderButton(Gdiplus::Graphics& g, int x, int y, int w, int h)
{
    Gdiplus::Color bg = m_folderBtnHovered ? Colors::BgHover() : Colors::BgCard();
    Gdiplus::SolidBrush b(bg);
    FillRoundRect(g, &b, x, y, w, h, 10);
    Gdiplus::Color borderCol = m_folderBtnHovered ? Colors::Accent() : Colors::Border();
    Gdiplus::Pen bp(borderCol, 1.5f);
    DrawRoundRect(g, &bp, x, y, w, h, 10);

    int iconX = x + 14, iconY = y + h / 2 - 7;
    Gdiplus::Color iconCol = m_folderBtnHovered ? Colors::Accent() : Colors::TextDim();
    Gdiplus::Pen ip(iconCol, 1.8f);

    Gdiplus::GraphicsPath folderPath;
    folderPath.AddLine(iconX, iconY + 12, iconX, iconY + 3);
    folderPath.AddLine(iconX, iconY + 3, iconX + 5, iconY + 3);
    folderPath.AddLine(iconX + 5, iconY + 3, iconX + 7, iconY + 1);
    folderPath.AddLine(iconX + 7, iconY + 1, iconX + 15, iconY + 1);
    folderPath.AddLine(iconX + 15, iconY + 1, iconX + 15, iconY + 12);
    folderPath.AddLine(iconX + 15, iconY + 12, iconX, iconY + 12);
    g.DrawPath(&ip, &folderPath);

    Gdiplus::FontFamily ff(L"Segoe UI");
    Gdiplus::Font fnt(&ff, 11, Gdiplus::FontStyleBold);
    Gdiplus::SolidBrush tb(m_folderBtnHovered ? Colors::Text() : Colors::TextDim());
    g.DrawString(L"\u041E\u0442\u043A\u0440\u044B\u0442\u044C \u043F\u0430\u043F\u043A\u0443 Zapret", -1, &fnt,
                 Gdiplus::PointF((float)(iconX + 28), (float)(y + h / 2 - 9)), &tb);
}

void ZapretGUI::FillRoundRect(Gdiplus::Graphics& g, Gdiplus::Brush* brush, int x, int y, int w, int h, int r)
{
    Gdiplus::GraphicsPath path;
    path.AddArc(x, y, r * 2, r * 2, 180, 90);
    path.AddArc(x + w - r * 2, y, r * 2, r * 2, 270, 90);
    path.AddArc(x + w - r * 2, y + h - r * 2, r * 2, r * 2, 0, 90);
    path.AddArc(x, y + h - r * 2, r * 2, r * 2, 90, 90);
    path.CloseFigure();
    g.FillPath(brush, &path);
}

void ZapretGUI::DrawRoundRect(Gdiplus::Graphics& g, Gdiplus::Pen* pen, int x, int y, int w, int h, int r)
{
    Gdiplus::GraphicsPath path;
    path.AddArc(x, y, r * 2, r * 2, 180, 90);
    path.AddArc(x + w - r * 2, y, r * 2, r * 2, 270, 90);
    path.AddArc(x + w - r * 2, y + h - r * 2, r * 2, r * 2, 0, 90);
    path.AddArc(x, y + h - r * 2, r * 2, r * 2, 90, 90);
    path.CloseFigure();
    g.DrawPath(pen, &path);
}

void ZapretGUI::OnMouseMove(int x, int y)
{
    RECT cr; GetClientRect(m_hWnd, &cr);
    int w = cr.right, h = cr.bottom;
    bool redraw = false;

    RECT minBtn = GetMinBtnRect(w), maxBtn = GetMaxBtnRect(w), closeBtn = GetCloseBtnRect(w);
    bool wasMin = m_minBtnHovered;
    m_minBtnHovered = PtInRect(x, y, minBtn.left, minBtn.top, minBtn.right - minBtn.left, minBtn.bottom - minBtn.top);
    if (wasMin != m_minBtnHovered) redraw = true;

    bool wasMax = m_maxBtnHovered;
    m_maxBtnHovered = PtInRect(x, y, maxBtn.left, maxBtn.top, maxBtn.right - maxBtn.left, maxBtn.bottom - maxBtn.top);
    if (wasMax != m_maxBtnHovered) redraw = true;

    bool wasClose = m_closeBtnHovered;
    m_closeBtnHovered = PtInRect(x, y, closeBtn.left, closeBtn.top, closeBtn.right - closeBtn.left, closeBtn.bottom - closeBtn.top);
    if (wasClose != m_closeBtnHovered) redraw = true;

    int yOff = TITLEBAR_H + TOPBAR_H;
    int cx = (w - SIDEBAR_W) / 2, cy = yOff + (h - yOff) / 2 - 35;
    bool was = m_connectBtnHovered;
    m_connectBtnHovered = PtInCircle(x, y, cx, cy, 95);
    if (was != m_connectBtnHovered) redraw = true;

    was = m_settingsBtnHovered;
    m_settingsBtnHovered = PtInRect(x, y, w - 50, TITLEBAR_H + 11, 34, 34);
    if (was != m_settingsBtnHovered) redraw = true;

    was = m_folderBtnHovered;
    int sx = w - SIDEBAR_W;
    m_folderBtnHovered = PtInRect(x, y, sx + 16, h - 52, SIDEBAR_W - 32, 40);
    if (was != m_folderBtnHovered) redraw = true;

    int old = m_hoveredBatIndex;
    m_hoveredBatIndex = -1;
    int listY = yOff + 60;
    int listH = h - listY - 65;
    if (x >= sx + 12 && x <= w - 12 && y >= listY && y < listY + listH)
    {
        int rel = y - listY - m_sidebarScroll;
        int idx = rel / 46;
        if (idx >= 0 && idx < (int)m_strategies.size())
            m_hoveredBatIndex = idx;
    }
    if (old != m_hoveredBatIndex) redraw = true;

    if (m_connectBtnHovered || m_settingsBtnHovered || m_folderBtnHovered ||
        m_hoveredBatIndex >= 0 || m_minBtnHovered || m_maxBtnHovered || m_closeBtnHovered)
        SetCursor(LoadCursor(nullptr, IDC_HAND));

    if (redraw) InvalidateRect(m_hWnd, nullptr, FALSE);
}

void ZapretGUI::OnMouseDown(int x, int y)
{
    if (m_connectBtnHovered)
    {
        m_connectBtnPressed = true;
        InvalidateRect(m_hWnd, nullptr, FALSE);
    }
}

void ZapretGUI::OnMouseUp(int x, int y)
{
    if (m_minBtnHovered) { ShowWindow(m_hWnd, SW_MINIMIZE); return; }
    if (m_maxBtnHovered) { ToggleMaximize(); return; }
    if (m_closeBtnHovered) { PostMessage(m_hWnd, WM_CLOSE, 0, 0); return; }

    if (m_connectBtnPressed)
    {
        m_connectBtnPressed = false;
        if (m_connectBtnHovered) ToggleConnection();
        InvalidateRect(m_hWnd, nullptr, FALSE);
        return;
    }
    if (m_settingsBtnHovered) { OpenSettings(); return; }
    if (m_folderBtnHovered) { SelectFolder(); return; }
    if (m_hoveredBatIndex >= 0 && m_hoveredBatIndex < (int)m_strategies.size())
    {
        m_selectedBatIndex = m_hoveredBatIndex;
        try { SaveConfig(); } catch (...) {}
        InvalidateRect(m_hWnd, nullptr, FALSE);
    }
}

void ZapretGUI::OnMouseWheel(int delta, int x, int y)
{
    RECT cr; GetClientRect(m_hWnd, &cr);
    int sx = cr.right - SIDEBAR_W;
    if (x < sx) return;

    m_sidebarScroll += (delta > 0) ? 46 : -46;
    InvalidateRect(m_hWnd, nullptr, FALSE);
}

void ZapretGUI::ToggleMaximize()
{
    WINDOWPLACEMENT wp = { sizeof(wp) };
    GetWindowPlacement(m_hWnd, &wp);
    if (wp.showCmd == SW_MAXIMIZE) ShowWindow(m_hWnd, SW_RESTORE);
    else ShowWindow(m_hWnd, SW_MAXIMIZE);
}

void ZapretGUI::DetectZapretPaths()
{
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    fs::path exeFolder = fs::path(exePath).parent_path();

    m_zapretPath = exeFolder;
    m_binPath = exeFolder / L"bin";
    m_listsPath = exeFolder / L"lists";

    fs::path winwsExe = m_binPath / L"winws.exe";

    if (fs::exists(winwsExe) && fs::exists(m_listsPath))
    {
        m_pathsValid = true;
        EnsureUserLists();
    }
    else
    {
        m_pathsValid = false;
        std::wstring errMsg = L"\u041D\u0435 \u043D\u0430\u0439\u0434\u0435\u043D\u044B \u0444\u0430\u0439\u043B\u044B Zapret.\n\n";
        errMsg += L"\u0420\u044F\u0434\u043E\u043C \u0441 EXE \u0434\u043E\u043B\u0436\u043D\u044B \u0431\u044B\u0442\u044C \u043F\u0430\u043F\u043A\u0438 bin\\ \u0438 lists\\.\n\n";
        errMsg += L"\u0422\u0435\u043A\u0443\u0449\u0430\u044F \u043F\u0430\u043F\u043A\u0430:\n";
        errMsg += exeFolder.wstring();
        MessageBoxW(nullptr, errMsg.c_str(), L"Zapret Gui", MB_ICONERROR);
    }
}

void ZapretGUI::EnsureUserLists()
{
    if (!m_pathsValid) return;

    auto createIfMissing = [&](const std::wstring& name, const std::wstring& content)
    {
        fs::path p = m_listsPath / name;
        if (!fs::exists(p))
        {
            std::wofstream f(p);
            if (f.is_open())
            {
                f << content << L"\n";
                f.close();
            }
        }
    };

    createIfMissing(L"ipset-exclude-user.txt", L"203.0.113.113/32");
    createIfMissing(L"list-general-user.txt", L"domain.example.abc");
    createIfMissing(L"list-exclude-user.txt", L"domain.example.abc");
}

std::wstring ZapretGUI::ReplaceAll(std::wstring str, const std::wstring& from, const std::wstring& to)
{
    size_t pos = 0;
    while ((pos = str.find(from, pos)) != std::wstring::npos)
    {
        str.replace(pos, from.length(), to);
        pos += to.length();
    }
    return str;
}

std::wstring ZapretGUI::BuildArguments(const Strategy& strategy)
{
    std::wstring binStr = m_binPath.wstring() + L"\\";
    std::wstring listsStr = m_listsPath.wstring() + L"\\";

    std::wstring args = strategy.args;
    args = ReplaceAll(args, L"{BIN}", binStr);
    args = ReplaceAll(args, L"{LISTS}", listsStr);
    args = ReplaceAll(args, L"{GAME_TCP}", m_gameTcpPorts);
    args = ReplaceAll(args, L"{GAME_UDP}", m_gameUdpPorts);

    return args;
}

void ZapretGUI::ApplyGameFilterMode()
{
    switch (m_settings.gameFilter)
    {
    case GameFilterMode::Disabled:
        m_gameTcpPorts = L"12";
        m_gameUdpPorts = L"12";
        break;
    case GameFilterMode::TcpAndUdp:
        m_gameTcpPorts = L"1024-65535";
        m_gameUdpPorts = L"1024-65535";
        break;
    case GameFilterMode::TcpOnly:
        m_gameTcpPorts = L"1024-65535";
        m_gameUdpPorts = L"12";
        break;
    case GameFilterMode::UdpOnly:
        m_gameTcpPorts = L"12";
        m_gameUdpPorts = L"1024-65535";
        break;
    }
}

void ZapretGUI::ToggleConnection()
{
    bool actuallyRunning = IsWinwsRunning();
    if (actuallyRunning || m_connected) Disconnect();
    else Connect();
}

void ZapretGUI::Connect()
{
    if (!m_pathsValid)
    {
        MessageBoxW(m_hWnd,
                    L"\u041D\u0435 \u043D\u0430\u0439\u0434\u0435\u043D\u044B \u0444\u0430\u0439\u043B\u044B Zapret.",
                    L"Zapret Gui", MB_ICONERROR);
        return;
    }

    if (m_selectedBatIndex < 0 || m_selectedBatIndex >= (int)m_strategies.size())
    {
        MessageBoxW(m_hWnd,
                    L"\u0421\u043D\u0430\u0447\u0430\u043B\u0430 \u0432\u044B\u0431\u0435\u0440\u0438\u0442\u0435 \u0441\u0442\u0440\u0430\u0442\u0435\u0433\u0438\u044E.",
                    L"Zapret Gui", MB_ICONWARNING);
        return;
    }

    StopWinws();
    StartWinws(m_strategies[m_selectedBatIndex]);
    m_wasConnected = true;
    SaveConfig();
    InvalidateRect(m_hWnd, nullptr, FALSE);
}

void ZapretGUI::Disconnect()
{
    StopWinws();
    m_connected = false;
    m_wasConnected = false;
    SaveConfig();
    InvalidateRect(m_hWnd, nullptr, FALSE);
}

void ZapretGUI::StartWinws(const Strategy& strategy)
{
    fs::path winwsPath = m_binPath / L"winws.exe";
    if (!fs::exists(winwsPath)) return;

    std::wstring args = BuildArguments(strategy);
    std::wstring cmdLine = L"\"" + winwsPath.wstring() + L"\" " + args;

    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {};

    std::vector<wchar_t> cmdBuf(cmdLine.begin(), cmdLine.end());
    cmdBuf.push_back(0);

    if (CreateProcessW(nullptr, cmdBuf.data(), nullptr, nullptr, FALSE,
                       CREATE_NO_WINDOW | DETACHED_PROCESS, nullptr,
                       m_binPath.wstring().c_str(), &si, &pi))
    {
        if (m_processHandle) CloseHandle(m_processHandle);
        m_processHandle = pi.hProcess;
        CloseHandle(pi.hThread);
    }
}

void ZapretGUI::StopWinws()
{
    if (m_processHandle)
    {
        TerminateProcess(m_processHandle, 0);
        WaitForSingleObject(m_processHandle, 1000);
        CloseHandle(m_processHandle);
        m_processHandle = nullptr;
    }

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot != INVALID_HANDLE_VALUE)
    {
        PROCESSENTRY32W pe = {};
        pe.dwSize = sizeof(pe);
        if (Process32FirstW(snapshot, &pe))
        {
            do
            {
                std::wstring name = pe.szExeFile;
                std::transform(name.begin(), name.end(), name.begin(), ::towlower);
                if (name == L"winws.exe")
                {
                    HANDLE hProc = OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE, FALSE, pe.th32ProcessID);
                    if (hProc)
                    {
                        TerminateProcess(hProc, 0);
                        WaitForSingleObject(hProc, 1000);
                        CloseHandle(hProc);
                    }
                }
            } while (Process32NextW(snapshot, &pe));
        }
        CloseHandle(snapshot);
    }

    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {};
    wchar_t cmd[] = L"taskkill /f /im winws.exe";
    if (CreateProcessW(nullptr, cmd, nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
    {
        WaitForSingleObject(pi.hProcess, 2000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

bool ZapretGUI::IsWinwsRunning()
{
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32W pe = {};
    pe.dwSize = sizeof(pe);
    bool found = false;

    if (Process32FirstW(snapshot, &pe))
    {
        do
        {
            std::wstring name = pe.szExeFile;
            std::transform(name.begin(), name.end(), name.begin(), ::towlower);
            if (name == L"winws.exe") { found = true; break; }
        } while (Process32NextW(snapshot, &pe));
    }
    CloseHandle(snapshot);
    return found;
}

void ZapretGUI::CheckConnectionStatus()
{
    bool actuallyRunning = IsWinwsRunning();
    if (actuallyRunning != m_connected)
    {
        m_connected = actuallyRunning;
        if (m_connected) m_pulseAnim = 0;
        InvalidateRect(m_hWnd, nullptr, FALSE);
    }
}

void ZapretGUI::SelectFolder()
{
    ShellExecuteW(nullptr, L"open", m_zapretPath.wstring().c_str(),
                  nullptr, nullptr, SW_SHOWNORMAL);
}

void ZapretGUI::OpenConfigFile()
{
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    fs::path cfg = fs::path(exePath).parent_path() / L"config.ini";
    if (!fs::exists(cfg)) SaveConfig();
    ShellExecuteW(nullptr, L"open", L"notepad.exe", cfg.wstring().c_str(), nullptr, SW_SHOWNORMAL);
}

void ZapretGUI::OpenGitHub()
{
    ShellExecuteW(nullptr, L"open", L"https://github.com/Flowseal/zapret-discord-youtube", nullptr, nullptr, SW_SHOWNORMAL);
}

void ZapretGUI::SaveConfig()
{
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    fs::path cfgPath = fs::path(exePath).parent_path() / L"config.ini";

    try
    {
        std::wofstream f(cfgPath);
        if (!f.is_open()) return;

        f << L"[Zapret]\n";
        f << L"Strategy=" << m_selectedBatIndex << L"\n";
        f << L"GameFilter=" << (int)m_settings.gameFilter << L"\n";
        f << L"IPSetMode=" << (int)m_settings.ipsetMode << L"\n";
        f << L"AutoStart=" << (m_settings.autoStart ? 1 : 0) << L"\n";
        f << L"AutoConnect=" << (m_settings.autoConnect ? 1 : 0) << L"\n";
        f << L"StartMinimized=" << (m_settings.startMinimized ? 1 : 0) << L"\n";
        f << L"WasConnected=" << (m_wasConnected ? 1 : 0) << L"\n";
        f.close();
    }
    catch (...) {}
}

void ZapretGUI::LoadConfig()
{
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    fs::path cfgPath = fs::path(exePath).parent_path() / L"config.ini";

    if (!fs::exists(cfgPath)) return;

    std::wifstream f(cfgPath);
    if (!f.is_open()) return;

    std::wstring line;
    while (std::getline(f, line))
    {
        size_t eq = line.find(L'=');
        if (eq == std::wstring::npos) continue;
        std::wstring key = line.substr(0, eq);
        std::wstring val = line.substr(eq + 1);

        if (key == L"Strategy")
        {
            int idx = _wtoi(val.c_str());
            if (idx >= 0 && idx < (int)m_strategies.size())
                m_selectedBatIndex = idx;
            else
                m_selectedBatIndex = -1;
        }
        else if (key == L"IPSetMode")
        {
            int ip = _wtoi(val.c_str());
            if (ip >= 0 && ip <= 2) m_settings.ipsetMode = (IPSetMode)ip;
        }
        else if (key == L"AutoStart") m_settings.autoStart = (_wtoi(val.c_str()) == 1);
        else if (key == L"AutoConnect") m_settings.autoConnect = (_wtoi(val.c_str()) == 1);
        else if (key == L"StartMinimized") m_settings.startMinimized = (_wtoi(val.c_str()) == 1);
        else if (key == L"WasConnected") m_wasConnected = (_wtoi(val.c_str()) == 1);
    }
    f.close();

    m_settings.autoStart = IsAutoStartEnabled();
}

void ZapretGUI::SetAutoStart(bool enable)
{
    HKEY hKey;
    LONG result = RegOpenKeyExW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0, KEY_SET_VALUE, &hKey);
    if (result != ERROR_SUCCESS) return;

    if (enable)
    {
        wchar_t exePath[MAX_PATH];
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        std::wstring cmd = L"\"" + std::wstring(exePath) + L"\" --autostart";
        RegSetValueExW(hKey, L"ZapretGui", 0, REG_SZ,
                       (BYTE*)cmd.c_str(),
                       (DWORD)((cmd.length() + 1) * sizeof(wchar_t)));
    }
    else
    {
        RegDeleteValueW(hKey, L"ZapretGui");
    }

    RegCloseKey(hKey);
}

bool ZapretGUI::IsAutoStartEnabled()
{
    HKEY hKey;
    LONG result = RegOpenKeyExW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0, KEY_QUERY_VALUE, &hKey);
    if (result != ERROR_SUCCESS) return false;

    wchar_t buf[1024];
    DWORD bufSize = sizeof(buf);
    DWORD type = 0;
    result = RegQueryValueExW(hKey, L"ZapretGui", nullptr, &type, (BYTE*)buf, &bufSize);
    RegCloseKey(hKey);

    return result == ERROR_SUCCESS;
}

void ZapretGUI::HandleAutoConnect()
{
    if (m_settings.autoConnect && m_wasConnected && m_pathsValid &&
        m_selectedBatIndex >= 0 && m_selectedBatIndex < (int)m_strategies.size())
    {
        if (!IsWinwsRunning())
        {
            StartWinws(m_strategies[m_selectedBatIndex]);
        }
    }
}

void ZapretGUI::OpenSettings()
{
    if (m_hSettingsWnd)
    {
        SetForegroundWindow(m_hSettingsWnd);
        return;
    }

    static bool registered = false;
    if (!registered)
    {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = SettingsWndProc;
        wc.hInstance = m_hInstance;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = nullptr;
        wc.lpszClassName = L"ZapretSettings";
        wc.hIcon = m_hAppIcon;
        RegisterClassExW(&wc);
        registered = true;
    }

    RECT mainRect;
    GetWindowRect(m_hWnd, &mainRect);
    int sw = 520, sh = 560;
    int sx = mainRect.left + (mainRect.right - mainRect.left - sw) / 2;
    int sy = mainRect.top + (mainRect.bottom - mainRect.top - sh) / 2;

    m_settingsScroll = 0;
    m_settingsHoveredItem = -1;

    m_hSettingsWnd = CreateWindowExW(
        WS_EX_TOOLWINDOW,
        L"ZapretSettings", L"",
        WS_POPUP | WS_VISIBLE,
        sx, sy, sw, sh,
        m_hWnd, nullptr, m_hInstance, this);

    BOOL darkMode = TRUE;
    DwmSetWindowAttribute(m_hSettingsWnd, 20, &darkMode, sizeof(darkMode));

    ShowWindow(m_hSettingsWnd, SW_SHOW);
    UpdateWindow(m_hSettingsWnd);
}

LRESULT CALLBACK ZapretGUI::SettingsWndProc(HWND hWnd, UINT msg, WPARAM w, LPARAM l)
{
    ZapretGUI* p = nullptr;
    if (msg == WM_CREATE)
    {
        auto cs = reinterpret_cast<CREATESTRUCT*>(l);
        p = reinterpret_cast<ZapretGUI*>(cs->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(p));
    }
    else p = reinterpret_cast<ZapretGUI*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (p) return p->HandleSettingsMessage(msg, w, l);
    return DefWindowProc(hWnd, msg, w, l);
}

int ZapretGUI::GetSettingsContentHeight()
{
    return 11 * 70 + 60;
}

int ZapretGUI::GetSettingsItemAt(int x, int y)
{
    RECT cr; GetClientRect(m_hSettingsWnd, &cr);
    int contentY = 50;
    int contentBottom = cr.bottom - 60;

    if (y < contentY || y > contentBottom) return -1;
    if (x < 20 || x > cr.right - 20) return -1;

    int rel = y - contentY - m_settingsScroll;
    int idx = rel / 70;
    if (idx >= 0 && idx < 11) return idx;
    return -1;
}

LRESULT ZapretGUI::HandleSettingsMessage(UINT msg, WPARAM w, LPARAM l)
{
    switch (msg)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(m_hSettingsWnd, &ps);
        RECT cr; GetClientRect(m_hSettingsWnd, &cr);

        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBmp = CreateCompatibleBitmap(hdc, cr.right, cr.bottom);
        HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

        DrawSettingsWindow(memDC, cr.right, cr.bottom);

        BitBlt(hdc, 0, 0, cr.right, cr.bottom, memDC, 0, 0, SRCCOPY);
        SelectObject(memDC, oldBmp);
        DeleteObject(memBmp);
        DeleteDC(memDC);
        EndPaint(m_hSettingsWnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND: return 1;
    case WM_MOUSEMOVE:
        OnSettingsMouseMove(GET_X_LPARAM(l), GET_Y_LPARAM(l));
        return 0;
    case WM_LBUTTONDOWN:
    {
        int x = GET_X_LPARAM(l), y = GET_Y_LPARAM(l);
        RECT cr; GetClientRect(m_hSettingsWnd, &cr);

        bool onInteractive =
            PtInRect(x, y, cr.right - 38, 8, 30, 28) ||
            PtInRect(x, y, cr.right / 2 - 70, cr.bottom - 50, 140, 36) ||
            (GetSettingsItemAt(x, y) >= 0);

        if (!onInteractive && y < 44)
        {
            ReleaseCapture();
            SendMessage(m_hSettingsWnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
        }
        return 0;
    }
    case WM_LBUTTONUP:
        OnSettingsClick(GET_X_LPARAM(l), GET_Y_LPARAM(l));
        return 0;
    case WM_MOUSEWHEEL:
    {
        int delta = GET_WHEEL_DELTA_WPARAM(w);
        OnSettingsWheel(delta);
        return 0;
    }
    case WM_CLOSE:
        DestroyWindow(m_hSettingsWnd);
        return 0;
    case WM_DESTROY:
        m_hSettingsWnd = nullptr;
        return 0;
    }
    return DefWindowProc(m_hSettingsWnd, msg, w, l);
}

void ZapretGUI::OnSettingsMouseMove(int x, int y)
{
    RECT cr; GetClientRect(m_hSettingsWnd, &cr);

    bool wasClose = m_settingsCloseHovered;
    bool wasSave = m_settingsSaveHovered;
    int wasItem = m_settingsHoveredItem;

    m_settingsCloseHovered = PtInRect(x, y, cr.right - 38, 8, 30, 28);
    m_settingsSaveHovered = PtInRect(x, y, cr.right / 2 - 70, cr.bottom - 50, 140, 36);

    if (!m_settingsCloseHovered && !m_settingsSaveHovered)
        m_settingsHoveredItem = GetSettingsItemAt(x, y);
    else
        m_settingsHoveredItem = -1;

    if (wasClose != m_settingsCloseHovered || wasSave != m_settingsSaveHovered ||
        wasItem != m_settingsHoveredItem)
        InvalidateRect(m_hSettingsWnd, nullptr, FALSE);

    if (m_settingsCloseHovered || m_settingsSaveHovered || m_settingsHoveredItem >= 0)
        SetCursor(LoadCursor(nullptr, IDC_HAND));
    else
        SetCursor(LoadCursor(nullptr, IDC_ARROW));
}

void ZapretGUI::OnSettingsWheel(int delta)
{
    RECT cr; GetClientRect(m_hSettingsWnd, &cr);
    int contentH = GetSettingsContentHeight();
    int viewH = cr.bottom - 50 - 60;

    if (contentH <= viewH)
    {
        m_settingsScroll = 0;
        return;
    }

    m_settingsScroll += (delta > 0) ? 50 : -50;
    int maxScroll = -(contentH - viewH);
    if (m_settingsScroll > 0) m_settingsScroll = 0;
    if (m_settingsScroll < maxScroll) m_settingsScroll = maxScroll;

    InvalidateRect(m_hSettingsWnd, nullptr, FALSE);
}

void ZapretGUI::OnSettingsClick(int x, int y)
{
    if (!m_hSettingsWnd) return;

    if (m_settingsCloseHovered)
    {
        DestroyWindow(m_hSettingsWnd);
        return;
    }

    if (m_settingsSaveHovered)
    {
        SaveConfig();
        DestroyWindow(m_hSettingsWnd);
        return;
    }

    int item = GetSettingsItemAt(x, y);
    if (item < 0) return;

    switch (item)
    {
    case 0:
        m_settings.autoStart = !m_settings.autoStart;
        SetAutoStart(m_settings.autoStart);
        SaveConfig();
        break;
    case 1:
        m_settings.autoConnect = !m_settings.autoConnect;
        SaveConfig();
        break;
    case 2:
        m_settings.startMinimized = !m_settings.startMinimized;
        SaveConfig();
        break;
    case 3:
        m_settings.gameFilter = (GameFilterMode)(((int)m_settings.gameFilter + 1) % 4);
        ApplyGameFilterMode();
        SaveConfig();
        if (m_connected)
        {
            StopWinws();
            if (m_selectedBatIndex >= 0)
                StartWinws(m_strategies[m_selectedBatIndex]);
        }
        break;
    case 4:
        m_settings.ipsetMode = (IPSetMode)(((int)m_settings.ipsetMode + 1) % 3);
        ApplyIPSetMode();
        SaveConfig();
        if (m_connected)
        {
            StopWinws();
            if (m_selectedBatIndex >= 0)
                StartWinws(m_strategies[m_selectedBatIndex]);
        }
        break;
    case 5:
        UpdateHostsFile();
        break;
    case 6:
        OpenTestWindow();
        break;
    case 7:
        OpenConfigFile();
        break;
    case 8:
        SelectFolder();
        break;
    case 9:
        OpenGitHub();
        break;
    case 10:
    {
        std::wstring info = L"Zapret Gui v";
        info += APP_VERSION;
        info += L"\n\n";
        info += L"\u041D\u0435\u043E\u0444\u0438\u0446\u0438\u0430\u043B\u044C\u043D\u044B\u0439 \u0433\u0440\u0430\u0444\u0438\u0447\u0435\u0441\u043A\u0438\u0439 \u0438\u043D\u0442\u0435\u0440\u0444\u0435\u0439\u0441 \u0434\u043B\u044F Zapret.\n\n";
        info += L"\u0421\u0442\u0440\u0430\u0442\u0435\u0433\u0438\u0439 \u0432 \u0431\u0430\u0437\u0435: ";
        info += std::to_wstring(m_strategies.size());
        info += L"\n";
        info += L"\u041F\u0430\u043F\u043A\u0430: ";
        info += m_zapretPath.wstring();
        MessageBoxW(m_hSettingsWnd, info.c_str(), L"\u041E \u043F\u0440\u043E\u0433\u0440\u0430\u043C\u043C\u0435", MB_ICONINFORMATION);
        break;
    }
    }

    InvalidateRect(m_hSettingsWnd, nullptr, FALSE);
}

void ZapretGUI::DrawSettingsWindow(HDC hdc, int w, int h)
{
    Gdiplus::Graphics g(hdc);
    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

    Gdiplus::SolidBrush bg(Colors::BgDark());
    g.FillRectangle(&bg, 0, 0, w, h);

    Gdiplus::Pen border(Colors::Accent(), 2);
    g.DrawRectangle(&border, 0, 0, w - 1, h - 1);

    Gdiplus::SolidBrush titleBg(Colors::BgTitle());
    g.FillRectangle(&titleBg, 1, 1, w - 2, 44);

    Gdiplus::FontFamily ff(L"Segoe UI");
    Gdiplus::Font titleFont(&ff, 14, Gdiplus::FontStyleBold);
    Gdiplus::SolidBrush titleBrush(Colors::Accent());
    g.DrawString(L"\u041D\u0430\u0441\u0442\u0440\u043E\u0439\u043A\u0438", -1, &titleFont,
                 Gdiplus::PointF(18, 12), &titleBrush);

    int closeX = w - 38, closeY = 8;
    if (m_settingsCloseHovered)
    {
        Gdiplus::SolidBrush ch(Gdiplus::Color(255, 232, 17, 35));
        g.FillRectangle(&ch, closeX, closeY, 30, 28);
    }
    Gdiplus::Pen closePen(Colors::Text(), 1.5f);
    int ccx = closeX + 15, ccy = closeY + 14;
    g.DrawLine(&closePen, ccx - 6, ccy - 6, ccx + 6, ccy + 6);
    g.DrawLine(&closePen, ccx + 6, ccy - 6, ccx - 6, ccy + 6);

    int contentY = 50;
    int contentBottom = h - 60;
    Gdiplus::Region clip(Gdiplus::Rect(0, contentY, w, contentBottom - contentY));
    g.SetClip(&clip);

    struct Item {
        std::wstring title;
        std::wstring desc;
        std::wstring value;
        int type;
    };

    std::wstring gfText;
    switch (m_settings.gameFilter)
    {
    case GameFilterMode::Disabled: gfText = L"\u0412\u044B\u043A\u043B\u044E\u0447\u0435\u043D"; break;
    case GameFilterMode::TcpAndUdp: gfText = L"TCP + UDP"; break;
    case GameFilterMode::TcpOnly: gfText = L"\u0422\u043E\u043B\u044C\u043A\u043E TCP"; break;
    case GameFilterMode::UdpOnly: gfText = L"\u0422\u043E\u043B\u044C\u043A\u043E UDP"; break;
    }

    std::wstring ipText;
    switch (m_settings.ipsetMode)
    {
    case IPSetMode::Loaded: ipText = L"loaded"; break;
    case IPSetMode::None: ipText = L"none"; break;
    case IPSetMode::Any: ipText = L"any"; break;
    }

    Item items[11] = {
        { L"\u0410\u0432\u0442\u043E\u0437\u0430\u043F\u0443\u0441\u043A \u0441 Windows",
          L"\u0417\u0430\u043F\u0443\u0441\u043A\u0430\u0442\u044C \u043F\u0440\u0438 \u0432\u0445\u043E\u0434\u0435 \u0432 \u0441\u0438\u0441\u0442\u0435\u043C\u0443",
          L"", 0 },
        { L"\u0410\u0432\u0442\u043E-\u043F\u043E\u0434\u043A\u043B\u044E\u0447\u0435\u043D\u0438\u0435",
          L"\u0412\u043E\u0441\u0441\u0442\u0430\u043D\u0430\u0432\u043B\u0438\u0432\u0430\u0442\u044C \u0441\u043E\u0441\u0442\u043E\u044F\u043D\u0438\u0435 \u043F\u043E\u0441\u043B\u0435 \u043F\u0435\u0440\u0435\u0437\u0430\u0433\u0440\u0443\u0437\u043A\u0438",
          L"", 0 },
        { L"\u0417\u0430\u043F\u0443\u0441\u043A\u0430\u0442\u044C \u0441\u0432\u0435\u0440\u043D\u0443\u0442\u044B\u043C",
          L"\u041E\u043A\u043D\u043E \u043D\u0435 \u043F\u043E\u044F\u0432\u043B\u044F\u0435\u0442\u0441\u044F \u043F\u0440\u0438 \u0430\u0432\u0442\u043E\u0437\u0430\u043F\u0443\u0441\u043A\u0435",
          L"", 0 },
        { L"\u0418\u0433\u0440\u043E\u0432\u043E\u0439 \u0444\u0438\u043B\u044C\u0442\u0440",
          L"\u041E\u0431\u0445\u043E\u0434 \u0431\u043B\u043E\u043A\u0438\u0440\u043E\u0432\u043E\u043A \u0432 \u0438\u0433\u0440\u0430\u0445",
          gfText, 1 },
        { L"IPSet \u0444\u0438\u043B\u044C\u0442\u0440",
          L"\u0420\u0435\u0436\u0438\u043C \u0440\u0430\u0431\u043E\u0442\u044B ipset-all.txt",
          ipText, 1 },
        { L"\u041E\u0431\u043D\u043E\u0432\u0438\u0442\u044C hosts \u0438 ipset",
          L"\u0421\u043A\u0430\u0447\u0430\u0442\u044C \u0430\u043A\u0442\u0443\u0430\u043B\u044C\u043D\u044B\u0435 \u0441\u043F\u0438\u0441\u043A\u0438 + \u0432 \u0431\u0443\u0444\u0435\u0440",
          L"", 2 },
        { L"\u0422\u0435\u0441\u0442 \u0441\u0442\u0440\u0430\u0442\u0435\u0433\u0438\u0439",
          L"\u041F\u0440\u043E\u0432\u0435\u0440\u0438\u0442\u044C \u0432\u0441\u0435 \u0441\u0442\u0440\u0430\u0442\u0435\u0433\u0438\u0438 \u043F\u043E \u0434\u043E\u043C\u0435\u043D\u0430\u043C",
          L"", 2 },
        { L"\u041E\u0442\u043A\u0440\u044B\u0442\u044C config.ini",
          L"\u0420\u0435\u0434\u0430\u043A\u0442\u0438\u0440\u043E\u0432\u0430\u0442\u044C \u0444\u0430\u0439\u043B \u043D\u0430\u0441\u0442\u0440\u043E\u0435\u043A",
          L"", 2 },
        { L"\u041E\u0442\u043A\u0440\u044B\u0442\u044C \u043F\u0430\u043F\u043A\u0443 Zapret",
          L"\u041F\u043E\u043A\u0430\u0437\u0430\u0442\u044C \u0432 \u043F\u0440\u043E\u0432\u043E\u0434\u043D\u0438\u043A\u0435",
          L"", 2 },
        { L"\u0418\u0441\u0445\u043E\u0434\u043D\u0438\u043A Zapret",
          L"\u041E\u0442\u043A\u0440\u044B\u0442\u044C GitHub \u0440\u0435\u043F\u043E\u0437\u0438\u0442\u043E\u0440\u0438\u0439",
          L"", 2 },
        { L"\u041E \u043F\u0440\u043E\u0433\u0440\u0430\u043C\u043C\u0435",
          L"\u0412\u0435\u0440\u0441\u0438\u044F \u0438 \u0438\u043D\u0444\u043E\u0440\u043C\u0430\u0446\u0438\u044F",
          std::wstring(L"v") + APP_VERSION, 2 },
    };

    Gdiplus::Font itemFont(&ff, 11, Gdiplus::FontStyleBold);
    Gdiplus::Font descFont(&ff, 9, Gdiplus::FontStyleRegular);
    Gdiplus::Font valFont(&ff, 10, Gdiplus::FontStyleBold);

    for (int i = 0; i < 11; i++)
    {
        int y = contentY + i * 70 + m_settingsScroll;
        if (y + 60 < contentY || y > contentBottom) continue;

        int x = 20, ww = w - 40, hh = 60;
        bool hov = (m_settingsHoveredItem == i);

        Gdiplus::Color bgc = hov ? Colors::BgHover() : Colors::BgCard();
        Gdiplus::SolidBrush bb(bgc);
        FillRoundRect(g, &bb, x, y, ww, hh, 8);
        Gdiplus::Pen bp(hov ? Colors::Accent() : Colors::Border(), 1);
        DrawRoundRect(g, &bp, x, y, ww, hh, 8);

        Gdiplus::SolidBrush tb(Colors::Text());
        g.DrawString(items[i].title.c_str(), -1, &itemFont,
                     Gdiplus::PointF((float)(x + 16), (float)(y + 10)), &tb);

        Gdiplus::SolidBrush db(Colors::TextDim());
        g.DrawString(items[i].desc.c_str(), -1, &descFont,
                     Gdiplus::PointF((float)(x + 16), (float)(y + 32)), &db);

        if (items[i].type == 0)
        {
            bool on = false;
            if (i == 0) on = m_settings.autoStart;
            else if (i == 1) on = m_settings.autoConnect;
            else if (i == 2) on = m_settings.startMinimized;

            int tx = x + ww - 60, ty = y + 18, tw = 44, th = 24;
            Gdiplus::Color toggleBg = on ? Colors::Accent() : Colors::BgPanel();
            Gdiplus::SolidBrush tbb(toggleBg);
            FillRoundRect(g, &tbb, tx, ty, tw, th, 12);

            int knobX = on ? tx + tw - 22 : tx + 2;
            Gdiplus::SolidBrush kb(Gdiplus::Color(255, 255, 255, 255));
            g.FillEllipse(&kb, knobX, ty + 2, 20, 20);
        }
        else if (items[i].type == 1)
        {
            Gdiplus::RectF vbn;
            g.MeasureString(items[i].value.c_str(), -1, &valFont, Gdiplus::PointF(0, 0), &vbn);
            int vw = (int)vbn.Width + 24;
            int vx = x + ww - vw - 12, vy = y + 18, vh = 24;

            Gdiplus::SolidBrush vbb(Gdiplus::Color(50, 46, 222, 162));
            FillRoundRect(g, &vbb, vx, vy, vw, vh, 12);
            Gdiplus::Pen vp(Colors::Accent(), 1);
            DrawRoundRect(g, &vp, vx, vy, vw, vh, 12);

            Gdiplus::SolidBrush vtb(Colors::Accent());
            Gdiplus::StringFormat vsf;
            vsf.SetAlignment(Gdiplus::StringAlignmentCenter);
            vsf.SetLineAlignment(Gdiplus::StringAlignmentCenter);
            Gdiplus::RectF vr((float)vx, (float)vy, (float)vw, (float)vh);
            g.DrawString(items[i].value.c_str(), -1, &valFont, vr, &vsf, &vtb);
        }
        else if (items[i].type == 2)
        {
            int arrX = x + ww - 24, arrY = y + 26;
            Gdiplus::Pen ap(Colors::TextDim(), 2);
            g.DrawLine(&ap, arrX, arrY - 5, arrX + 6, arrY);
            g.DrawLine(&ap, arrX + 6, arrY, arrX, arrY + 5);

            if (!items[i].value.empty())
            {
                Gdiplus::Font vf(&ff, 9, Gdiplus::FontStyleRegular);
                Gdiplus::SolidBrush vb(Colors::TextDim());
                Gdiplus::RectF vbn;
                g.MeasureString(items[i].value.c_str(), -1, &vf, Gdiplus::PointF(0, 0), &vbn);
                g.DrawString(items[i].value.c_str(), -1, &vf,
                             Gdiplus::PointF((float)(x + ww - 50 - vbn.Width), (float)(y + 22)), &vb);
            }
        }
    }

    g.ResetClip();

    int contentH = GetSettingsContentHeight();
    int viewH = contentBottom - contentY;
    if (contentH > viewH)
    {
        int sbX = w - 8;
        int sbH = (int)((float)viewH * viewH / contentH);
        int sbY = contentY + (int)((float)(-m_settingsScroll) * viewH / contentH);
        Gdiplus::SolidBrush sbBrush(Gdiplus::Color(120, 46, 222, 162));
        FillRoundRect(g, &sbBrush, sbX, sbY, 4, sbH, 2);
    }

    Gdiplus::SolidBrush footerBg(Colors::BgTitle());
    g.FillRectangle(&footerBg, 1, h - 60, w - 2, 59);
    Gdiplus::Pen footerLine(Colors::Border(), 1);
    g.DrawLine(&footerLine, 1, h - 60, w - 2, h - 60);

    int bx = w / 2 - 70, by = h - 50, bw = 140, bh = 36;
    Gdiplus::Color sbgc = m_settingsSaveHovered ? Colors::AccentHover() : Colors::Accent();
    Gdiplus::SolidBrush bb(sbgc);
    FillRoundRect(g, &bb, bx, by, bw, bh, 10);

    Gdiplus::Font sfo(&ff, 11, Gdiplus::FontStyleBold);
    Gdiplus::SolidBrush stb(Gdiplus::Color(255, 8, 14, 12));
    Gdiplus::StringFormat ssf;
    ssf.SetAlignment(Gdiplus::StringAlignmentCenter);
    ssf.SetLineAlignment(Gdiplus::StringAlignmentCenter);
    Gdiplus::RectF sr((float)bx, (float)by, (float)bw, (float)bh);
    g.DrawString(L"\u0413\u043E\u0442\u043E\u0432\u043E", -1, &sfo, sr, &ssf, &stb);
}

bool ZapretGUI::PtInCircle(int px, int py, int cx, int cy, int r)
{
    int dx = px - cx, dy = py - cy;
    return dx * dx + dy * dy <= r * r;
}

bool ZapretGUI::PtInRect(int px, int py, int rx, int ry, int rw, int rh)
{
    return px >= rx && px < rx + rw && py >= ry && py < ry + rh;
}

IPSetMode ZapretGUI::DetectIPSetMode()
{
    if (!m_pathsValid) return IPSetMode::Loaded;

    fs::path listFile = m_listsPath / L"ipset-all.txt";
    if (!fs::exists(listFile)) return IPSetMode::Loaded;

    std::error_code ec;
    auto size = fs::file_size(listFile, ec);
    if (ec) return IPSetMode::Loaded;

    if (size == 0) return IPSetMode::Any;

    std::wifstream f(listFile);
    std::wstring line;
    bool onlyStub = true;
    int lineCount = 0;

    while (std::getline(f, line))
    {
        if (line.empty()) continue;
        lineCount++;
        if (line.find(L"203.0.113.113") == std::wstring::npos)
        {
            onlyStub = false;
            break;
        }
    }
    f.close();

    if (lineCount == 0) return IPSetMode::Any;
    if (onlyStub && lineCount == 1) return IPSetMode::None;
    return IPSetMode::Loaded;
}

void ZapretGUI::ApplyIPSetMode()
{
    if (!m_pathsValid) return;

    fs::path listFile = m_listsPath / L"ipset-all.txt";
    fs::path backupFile = m_listsPath / L"ipset-all.txt.backup";

    IPSetMode currentMode = DetectIPSetMode();
    if (currentMode == m_settings.ipsetMode) return;

    std::error_code ec;

    if (m_settings.ipsetMode == IPSetMode::Loaded)
    {
        if (fs::exists(backupFile))
        {
            fs::remove(listFile, ec);
            fs::rename(backupFile, listFile, ec);
        }
        else
        {
            MessageBoxW(m_hSettingsWnd ? m_hSettingsWnd : m_hWnd,
                L"\u0420\u0435\u0437\u0435\u0440\u0432\u043D\u0430\u044F \u043A\u043E\u043F\u0438\u044F \u0441\u043F\u0438\u0441\u043A\u0430 \u043D\u0435 \u043D\u0430\u0439\u0434\u0435\u043D\u0430.\n"
                L"\u041E\u0431\u043D\u043E\u0432\u0438\u0442\u0435 \u0441\u043F\u0438\u0441\u043E\u043A \u0447\u0435\u0440\u0435\u0437 \"Update Hosts\".",
                L"Zapret Gui", MB_ICONWARNING);
            m_settings.ipsetMode = currentMode;
            return;
        }
    }
    else if (m_settings.ipsetMode == IPSetMode::None)
    {
        if (fs::exists(listFile) && !fs::exists(backupFile))
        {
            auto size = fs::file_size(listFile, ec);
            if (size > 100) fs::copy_file(listFile, backupFile, ec);
        }

        std::wofstream f(listFile, std::ios::trunc);
        if (f.is_open())
        {
            f << L"203.0.113.113/32\n";
            f.close();
        }
    }
    else if (m_settings.ipsetMode == IPSetMode::Any)
    {
        if (fs::exists(listFile) && !fs::exists(backupFile))
        {
            auto size = fs::file_size(listFile, ec);
            if (size > 100) fs::copy_file(listFile, backupFile, ec);
        }

        std::wofstream f(listFile, std::ios::trunc);
        if (f.is_open()) f.close();
    }
}


bool ZapretGUI::DownloadToString(const std::wstring& url, std::string& result)
{
    URL_COMPONENTS uc = {};
    uc.dwStructSize = sizeof(uc);
    wchar_t host[256] = {0}, path[1024] = {0};
    uc.lpszHostName = host;
    uc.dwHostNameLength = 256;
    uc.lpszUrlPath = path;
    uc.dwUrlPathLength = 1024;

    if (!WinHttpCrackUrl(url.c_str(), 0, 0, &uc)) return false;

    HINTERNET hSession = WinHttpOpen(L"ZapretGui/1.0",
        WINHTTP_ACCESS_TYPE_NO_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return false;

    DWORD timeout = 10000;
    WinHttpSetTimeouts(hSession, timeout, timeout, timeout, timeout);

    HINTERNET hConnect = WinHttpConnect(hSession, host, uc.nPort, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return false; }

    DWORD flags = (uc.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path,
        nullptr, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hRequest)
    {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    DWORD secFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
                     SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
                     SECURITY_FLAG_IGNORE_CERT_CN_INVALID;
    WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &secFlags, sizeof(secFlags));

    BOOL ok = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    if (ok) ok = WinHttpReceiveResponse(hRequest, nullptr);

    result.clear();
    if (ok)
    {
        DWORD bytesAvail = 0;
        do
        {
            bytesAvail = 0;
            if (!WinHttpQueryDataAvailable(hRequest, &bytesAvail)) break;
            if (bytesAvail == 0) break;

            std::vector<char> buf(bytesAvail + 1, 0);
            DWORD bytesRead = 0;
            if (!WinHttpReadData(hRequest, buf.data(), bytesAvail, &bytesRead)) break;
            result.append(buf.data(), bytesRead);
        } while (bytesAvail > 0);
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return ok && !result.empty();
}

bool ZapretGUI::DownloadFile(const std::wstring& url, const std::wstring& savePath)
{
    std::string content;
    if (!DownloadToString(url, content)) return false;

    HANDLE hFile = CreateFileW(savePath.c_str(), GENERIC_WRITE, 0, nullptr,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    DWORD written = 0;
    BOOL ok = WriteFile(hFile, content.data(), (DWORD)content.size(), &written, nullptr);
    CloseHandle(hFile);

    return ok && written == content.size();
}

bool ZapretGUI::CopyTextToClipboard(const std::wstring& text)
{
    if (!OpenClipboard(nullptr)) return false;

    EmptyClipboard();

    size_t size = (text.length() + 1) * sizeof(wchar_t);
    HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, size);
    if (!hGlobal)
    {
        CloseClipboard();
        return false;
    }

    void* pData = GlobalLock(hGlobal);
    if (!pData)
    {
        GlobalFree(hGlobal);
        CloseClipboard();
        return false;
    }

    memcpy(pData, text.c_str(), size);
    GlobalUnlock(hGlobal);

    SetClipboardData(CF_UNICODETEXT, hGlobal);
    CloseClipboard();
    return true;
}

void ZapretGUI::UpdateHostsFile()
{
    if (!m_pathsValid)
    {
        MessageBoxW(m_hSettingsWnd ? m_hSettingsWnd : m_hWnd,
            L"\u041F\u0430\u043F\u043A\u0430 Zapret \u043D\u0435 \u043D\u0430\u0439\u0434\u0435\u043D\u0430.",
            L"Zapret Gui", MB_ICONERROR);
        return;
    }

    HWND parent = m_hSettingsWnd ? m_hSettingsWnd : m_hWnd;

    int answer = MessageBoxW(parent,
        L"\u0411\u0443\u0434\u0435\u0442 \u0432\u044B\u043F\u043E\u043B\u043D\u0435\u043D\u043E:\n\n"
        L"1. \u0421\u043A\u0430\u0447\u0438\u0432\u0430\u043D\u0438\u0435 ipset-all.txt \u0441 GitHub\n"
        L"2. \u0421\u043A\u0430\u0447\u0438\u0432\u0430\u043D\u0438\u0435 \u0430\u043A\u0442\u0443\u0430\u043B\u044C\u043D\u043E\u0433\u043E hosts\n"
        L"3. \u041A\u043E\u043F\u0438\u0440\u043E\u0432\u0430\u043D\u0438\u0435 hosts \u0432 \u0431\u0443\u0444\u0435\u0440 \u043E\u0431\u043C\u0435\u043D\u0430\n\n"
        L"\u041F\u0440\u043E\u0434\u043E\u043B\u0436\u0438\u0442\u044C?",
        L"\u041E\u0431\u043D\u043E\u0432\u043B\u0435\u043D\u0438\u0435 \u0444\u0430\u0439\u043B\u043E\u0432",
        MB_ICONQUESTION | MB_YESNO);

    if (answer != IDYES) return;

    SetCursor(LoadCursor(nullptr, IDC_WAIT));

    fs::path ipsetPath = m_listsPath / L"ipset-all.txt";
    fs::path ipsetBackup = m_listsPath / L"ipset-all.txt.backup";

    std::error_code ec;
    if (fs::exists(ipsetPath))
    {
        auto size = fs::file_size(ipsetPath, ec);
        if (size > 100) fs::copy_file(ipsetPath, ipsetBackup, fs::copy_options::overwrite_existing, ec);
    }

    bool ipsetOk = DownloadFile(
        L"https://raw.githubusercontent.com/Flowseal/zapret-discord-youtube/refs/heads/main/.service/ipset-service.txt",
        ipsetPath.wstring());

    std::string hostsContent;
    bool hostsOk = DownloadToString(
        L"https://raw.githubusercontent.com/Flowseal/zapret-discord-youtube/refs/heads/main/.service/hosts",
        hostsContent);

    bool clipboardOk = false;
    if (hostsOk && !hostsContent.empty())
    {
        std::wstring wide(hostsContent.begin(), hostsContent.end());
        clipboardOk = CopyTextToClipboard(wide);
    }

    SetCursor(LoadCursor(nullptr, IDC_ARROW));

    if (m_settings.ipsetMode == IPSetMode::Loaded && ipsetOk)
    {
        if (m_connected)
        {
            StopWinws();
            if (m_selectedBatIndex >= 0)
                StartWinws(m_strategies[m_selectedBatIndex]);
        }
    }

    std::wstring result;
    result += L"\u0420\u0435\u0437\u0443\u043B\u044C\u0442\u0430\u0442:\n\n";
    result += ipsetOk
        ? L"[\u2713] ipset-all.txt \u043E\u0431\u043D\u043E\u0432\u043B\u0451\u043D\n"
        : L"[\u2717] ipset-all.txt \u043D\u0435 \u0443\u0434\u0430\u043B\u043E\u0441\u044C \u0441\u043A\u0430\u0447\u0430\u0442\u044C\n";
    result += hostsOk
        ? L"[\u2713] hosts \u0441\u043A\u0430\u0447\u0430\u043D\n"
        : L"[\u2717] hosts \u043D\u0435 \u0443\u0434\u0430\u043B\u043E\u0441\u044C \u0441\u043A\u0430\u0447\u0430\u0442\u044C\n";
    result += clipboardOk
        ? L"[\u2713] hosts \u0441\u043A\u043E\u043F\u0438\u0440\u043E\u0432\u0430\u043D \u0432 \u0431\u0443\u0444\u0435\u0440 \u043E\u0431\u043C\u0435\u043D\u0430\n\n"
        : L"[\u2717] \u041A\u043E\u043F\u0438\u0440\u043E\u0432\u0430\u043D\u0438\u0435 \u043D\u0435 \u0443\u0434\u0430\u043B\u043E\u0441\u044C\n\n";

    if (clipboardOk)
    {
        result += L"\u041E\u0442\u043A\u0440\u043E\u0439\u0442\u0435 \u0444\u0430\u0439\u043B \u0432 \u0431\u043B\u043E\u043A\u043D\u043E\u0442\u0435 \u043E\u0442 \u0438\u043C\u0435\u043D\u0438 \u0430\u0434\u043C\u0438\u043D\u0438\u0441\u0442\u0440\u0430\u0442\u043E\u0440\u0430:\n";
        result += L"C:\\Windows\\System32\\drivers\\etc\\hosts\n\n";
        result += L"\u0418 \u0432\u0441\u0442\u0430\u0432\u044C\u0442\u0435 (Ctrl+V).";
    }

    UINT icon = (ipsetOk && hostsOk && clipboardOk) ? MB_ICONINFORMATION : MB_ICONWARNING;
    MessageBoxW(parent, result.c_str(), L"\u041E\u0431\u043D\u043E\u0432\u043B\u0435\u043D\u0438\u0435", icon);
}

void ZapretGUI::OpenTestWindow()
{
    if (m_hTestWnd)
    {
        SetForegroundWindow(m_hTestWnd);
        return;
    }

    if (!m_pathsValid)
    {
        MessageBoxW(m_hSettingsWnd ? m_hSettingsWnd : m_hWnd,
            L"\u041F\u0430\u043F\u043A\u0430 Zapret \u043D\u0435 \u043D\u0430\u0439\u0434\u0435\u043D\u0430.",
            L"Zapret Gui", MB_ICONERROR);
        return;
    }

    static bool registered = false;
    if (!registered)
    {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = TestWndProc;
        wc.hInstance = m_hInstance;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = nullptr;
        wc.lpszClassName = L"ZapretTest";
        wc.hIcon = m_hAppIcon;
        RegisterClassExW(&wc);
        registered = true;
    }

    RECT mainRect;
    GetWindowRect(m_hWnd, &mainRect);
    int sw = 720, sh = 620;
    int sx = mainRect.left + (mainRect.right - mainRect.left - sw) / 2;
    int sy = mainRect.top + (mainRect.bottom - mainRect.top - sh) / 2;

    m_testResults.clear();
    for (size_t i = 0; i < m_strategies.size(); i++)
    {
        StrategyTestResult r;
        r.strategyName = m_strategies[i].displayName;
        r.strategyIndex = (int)i;
        m_testResults.push_back(r);
    }

    m_testScrollY = 0;
    m_testCloseHovered = false;
    m_testStartHovered = false;
    m_testCancelHovered = false;
    m_testExportHovered = false;

    m_hTestWnd = CreateWindowExW(
        WS_EX_TOOLWINDOW,
        L"ZapretTest", L"",
        WS_POPUP | WS_VISIBLE,
        sx, sy, sw, sh,
        m_hWnd, nullptr, m_hInstance, this);

    BOOL darkMode = TRUE;
    DwmSetWindowAttribute(m_hTestWnd, 20, &darkMode, sizeof(darkMode));

    SetTimer(m_hTestWnd, 1, 200, nullptr);

    ShowWindow(m_hTestWnd, SW_SHOW);
    UpdateWindow(m_hTestWnd);
}

LRESULT CALLBACK ZapretGUI::TestWndProc(HWND hWnd, UINT msg, WPARAM w, LPARAM l)
{
    ZapretGUI* p = nullptr;
    if (msg == WM_CREATE)
    {
        auto cs = reinterpret_cast<CREATESTRUCT*>(l);
        p = reinterpret_cast<ZapretGUI*>(cs->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(p));
    }
    else p = reinterpret_cast<ZapretGUI*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (p) return p->HandleTestMessage(msg, w, l);
    return DefWindowProc(hWnd, msg, w, l);
}

LRESULT ZapretGUI::HandleTestMessage(UINT msg, WPARAM w, LPARAM l)
{
    switch (msg)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(m_hTestWnd, &ps);
        RECT cr; GetClientRect(m_hTestWnd, &cr);

        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBmp = CreateCompatibleBitmap(hdc, cr.right, cr.bottom);
        HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

        DrawTestWindow(memDC, cr.right, cr.bottom);

        BitBlt(hdc, 0, 0, cr.right, cr.bottom, memDC, 0, 0, SRCCOPY);
        SelectObject(memDC, oldBmp);
        DeleteObject(memBmp);
        DeleteDC(memDC);
        EndPaint(m_hTestWnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND: return 1;
    case WM_TIMER:
        if (w == 1) InvalidateRect(m_hTestWnd, nullptr, FALSE);
        return 0;
    case WM_MOUSEMOVE:
        OnTestMouseMove(GET_X_LPARAM(l), GET_Y_LPARAM(l));
        return 0;
    case WM_LBUTTONDOWN:
    {
        int x = GET_X_LPARAM(l), y = GET_Y_LPARAM(l);
        if (y < 44 && !m_testCloseHovered)
        {
            ReleaseCapture();
            SendMessage(m_hTestWnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
        }
        return 0;
    }
    case WM_LBUTTONUP:
        OnTestClick(GET_X_LPARAM(l), GET_Y_LPARAM(l));
        return 0;
    case WM_MOUSEWHEEL:
    {
        int delta = GET_WHEEL_DELTA_WPARAM(w);
        OnTestWheel(delta);
        return 0;
    }
    case WM_CLOSE:
        if (m_testPhase == TestPhase::Running || m_testPhase == TestPhase::Preparing)
        {
            int answer = MessageBoxW(m_hTestWnd,
                L"\u0422\u0435\u0441\u0442 \u0432\u044B\u043F\u043E\u043B\u043D\u044F\u0435\u0442\u0441\u044F. \u041E\u0441\u0442\u0430\u043D\u043E\u0432\u0438\u0442\u044C \u0438 \u0437\u0430\u043A\u0440\u044B\u0442\u044C?",
                L"Zapret Gui", MB_ICONQUESTION | MB_YESNO);
            if (answer != IDYES) return 0;
            CancelTest();
            if (m_testThread.joinable()) m_testThread.join();
        }
        KillTimer(m_hTestWnd, 1);
        DestroyWindow(m_hTestWnd);
        return 0;
    case WM_DESTROY:
        m_hTestWnd = nullptr;
        return 0;
    }
    return DefWindowProc(m_hTestWnd, msg, w, l);
}

void ZapretGUI::DrawTestWindow(HDC hdc, int w, int h)
{
    Gdiplus::Graphics g(hdc);
    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

    Gdiplus::SolidBrush bg(Colors::BgDark());
    g.FillRectangle(&bg, 0, 0, w, h);

    Gdiplus::Pen border(Colors::Accent(), 2);
    g.DrawRectangle(&border, 0, 0, w - 1, h - 1);

    Gdiplus::SolidBrush titleBg(Colors::BgTitle());
    g.FillRectangle(&titleBg, 1, 1, w - 2, 44);

    Gdiplus::FontFamily ff(L"Segoe UI");
    Gdiplus::Font titleFont(&ff, 14, Gdiplus::FontStyleBold);
    Gdiplus::SolidBrush titleBrush(Colors::Accent());
    g.DrawString(L"\u0422\u0435\u0441\u0442 \u0441\u0442\u0440\u0430\u0442\u0435\u0433\u0438\u0439", -1, &titleFont,
                 Gdiplus::PointF(18, 12), &titleBrush);

    int closeX = w - 38, closeY = 8;
    if (m_testCloseHovered)
    {
        Gdiplus::SolidBrush ch(Gdiplus::Color(255, 232, 17, 35));
        g.FillRectangle(&ch, closeX, closeY, 30, 28);
    }
    Gdiplus::Pen closePen(Colors::Text(), 1.5f);
    int ccx = closeX + 15, ccy = closeY + 14;
    g.DrawLine(&closePen, ccx - 6, ccy - 6, ccx + 6, ccy + 6);
    g.DrawLine(&closePen, ccx + 6, ccy - 6, ccx - 6, ccy + 6);

    TestPhase phase = m_testPhase.load();
    int curStrat = m_testCurrentStrategy.load();
    int curDom = m_testCurrentDomain.load();

    int statusY = 55;
    Gdiplus::Font statusFont(&ff, 11, Gdiplus::FontStyleBold);
    Gdiplus::SolidBrush statusBrush(Colors::Text());

    std::wstring statusText;
    switch (phase)
    {
    case TestPhase::Idle:
        statusText = L"\u0413\u043E\u0442\u043E\u0432 \u043A \u0437\u0430\u043F\u0443\u0441\u043A\u0443";
        break;
    case TestPhase::Preparing:
        statusText = L"\u041F\u043E\u0434\u0433\u043E\u0442\u043E\u0432\u043A\u0430...";
        break;
    case TestPhase::Running:
    {
        std::wstring sName = (curStrat < (int)m_strategies.size())
            ? m_strategies[curStrat].displayName : L"";
        statusText = L"\u0422\u0435\u0441\u0442: " + sName +
                     L"  (" + std::to_wstring(curStrat + 1) + L"/" +
                     std::to_wstring(m_strategies.size()) + L")";
        break;
    }
    case TestPhase::Finished:
        statusText = L"\u0422\u0435\u0441\u0442 \u0437\u0430\u0432\u0435\u0440\u0448\u0451\u043D";
        break;
    case TestPhase::Cancelled:
        statusText = L"\u0422\u0435\u0441\u0442 \u043E\u0442\u043C\u0435\u043D\u0451\u043D";
        break;
    }

    g.DrawString(statusText.c_str(), -1, &statusFont,
                 Gdiplus::PointF(20, (float)statusY), &statusBrush);

    if (phase == TestPhase::Running || phase == TestPhase::Preparing)
    {
        int pbX = 20, pbY = 80, pbW = w - 40, pbH = 14;
        Gdiplus::SolidBrush pbBg(Colors::BgCard());
        FillRoundRect(g, &pbBg, pbX, pbY, pbW, pbH, 7);

        float progress = 0.0f;
        if (!m_testResults.empty())
        {
            int totalDomains = 0, doneDomains = 0;
            {
                std::lock_guard<std::mutex> lk(m_testResultsMutex);
                for (auto& sr : m_testResults)
                {
                    for (auto& dr : sr.domains)
                    {
                        totalDomains++;
                        if (dr.result != TestResult::Pending && dr.result != TestResult::Testing)
                            doneDomains++;
                    }
                }
            }
            if (totalDomains > 0)
                progress = (float)doneDomains / totalDomains;
        }

        int fillW = (int)(pbW * progress);
        if (fillW > 0)
        {
            Gdiplus::LinearGradientBrush gradBrush(
                Gdiplus::PointF((float)pbX, (float)pbY),
                Gdiplus::PointF((float)(pbX + pbW), (float)pbY),
                Colors::AccentDeep(), Colors::Accent());
            FillRoundRect(g, &gradBrush, pbX, pbY, fillW, pbH, 7);
        }

        Gdiplus::Font pctFont(&ff, 9, Gdiplus::FontStyleBold);
        Gdiplus::SolidBrush pctBrush(Colors::TextDim());
        std::wstring pctText = std::to_wstring((int)(progress * 100)) + L"%";
        g.DrawString(pctText.c_str(), -1, &pctFont,
                     Gdiplus::PointF((float)(w - 60), (float)statusY), &pctBrush);
    }
    else if (phase == TestPhase::Finished)
    {
        int totalDomains = 0, workingDomains = 0;
        std::wstring bestName;
        int bestPing = 0;
        int bestWorking = 0;

        {
            std::lock_guard<std::mutex> lk(m_testResultsMutex);
            int bestScore = -1;
            for (auto& sr : m_testResults)
            {
                for (auto& dr : sr.domains)
                {
                    totalDomains++;
                    if (dr.result == TestResult::Working) workingDomains++;
                }
                if (sr.overallStatus == TestResult::Working && sr.workingCount > 0)
                {
                    int score = sr.workingCount * 1000 - sr.avgPing;
                    if (score > bestScore)
                    {
                        bestScore = score;
                        bestName = sr.strategyName;
                        bestPing = sr.avgPing;
                        bestWorking = sr.workingCount;
                    }
                }
            }
        }

        Gdiplus::Font sumFont(&ff, 10, Gdiplus::FontStyleRegular);
        Gdiplus::SolidBrush sumBrush(Colors::TextDim());
        std::wstring sumText = L"\u0420\u0430\u0431\u043E\u0447\u0438\u0445: " +
            std::to_wstring(workingDomains) + L"/" + std::to_wstring(totalDomains);
        g.DrawString(sumText.c_str(), -1, &sumFont,
                     Gdiplus::PointF((float)(w - 200), (float)statusY), &sumBrush);

        if (!bestName.empty())
        {
            int boxX = 20, boxY = 75, boxW = w - 40, boxH = 28;
            Gdiplus::SolidBrush bestBg(Gdiplus::Color(60, 46, 222, 162));
            FillRoundRect(g, &bestBg, boxX, boxY, boxW, boxH, 6);
            Gdiplus::Pen bestBorder(Colors::Accent(), 1);
            DrawRoundRect(g, &bestBorder, boxX, boxY, boxW, boxH, 6);

            Gdiplus::Font bestFont(&ff, 10, Gdiplus::FontStyleBold);
            Gdiplus::SolidBrush bestBrush(Colors::AccentBright());
            std::wstring bestText = L"\u2605 \u041B\u0443\u0447\u0448\u0430\u044F \u0441\u0442\u0440\u0430\u0442\u0435\u0433\u0438\u044F: " + bestName +
                L"  (" + std::to_wstring(bestWorking) + L" \u0434\u043E\u043C\u0435\u043D\u043E\u0432, " +
                std::to_wstring(bestPing) + L" \u043C\u0441) - \u0432\u043A\u043B\u044E\u0447\u0435\u043D\u0430";
            g.DrawString(bestText.c_str(), -1, &bestFont,
                         Gdiplus::PointF((float)(boxX + 10), (float)(boxY + 6)), &bestBrush);
        }
    }

    int contentY = 105;
    int contentBottom = h - 60;
    int listH = contentBottom - contentY;

    int colStrategy = 20;
    int colStatus = 200;
    int colStats = 300;
    int colDetails = 420;

    Gdiplus::SolidBrush headerBg(Colors::BgPanel());
    g.FillRectangle(&headerBg, 10, contentY - 28, w - 20, 26);

    Gdiplus::Font hdrFont(&ff, 9, Gdiplus::FontStyleBold);
    Gdiplus::SolidBrush hdrBrush(Colors::TextDim());
    g.DrawString(L"\u0421\u0422\u0420\u0410\u0422\u0415\u0413\u0418\u042F", -1, &hdrFont,
                 Gdiplus::PointF((float)colStrategy, (float)(contentY - 22)), &hdrBrush);
    g.DrawString(L"\u0421\u0422\u0410\u0422\u0423\u0421", -1, &hdrFont,
                 Gdiplus::PointF((float)colStatus, (float)(contentY - 22)), &hdrBrush);
    g.DrawString(L"\u0420\u0410\u0411\u041E\u0427\u0418\u0425", -1, &hdrFont,
                 Gdiplus::PointF((float)colStats, (float)(contentY - 22)), &hdrBrush);
    g.DrawString(L"\u0421\u0420. \u041F\u0418\u041D\u0413", -1, &hdrFont,
                 Gdiplus::PointF((float)colDetails, (float)(contentY - 22)), &hdrBrush);

    Gdiplus::Region clip(Gdiplus::Rect(10, contentY, w - 20, listH));
    g.SetClip(&clip);

    int itemH = 44;
    std::vector<StrategyTestResult> snapshot;
    {
        std::lock_guard<std::mutex> lk(m_testResultsMutex);
        snapshot = m_testResults;
    }

    for (size_t i = 0; i < snapshot.size(); i++)
    {
        int y = contentY + (int)i * itemH + m_testScrollY;
        if (y + itemH < contentY || y > contentBottom) continue;

        auto& sr = snapshot[i];

        if (phase == TestPhase::Running && (int)i == curStrat)
        {
            Gdiplus::SolidBrush hb(Gdiplus::Color(40, 46, 222, 162));
            g.FillRectangle(&hb, 10, y, w - 20, itemH);
        }
        else if (i % 2 == 0)
        {
            Gdiplus::SolidBrush hb(Gdiplus::Color(255, 14, 22, 19));
            g.FillRectangle(&hb, 10, y, w - 20, itemH);
        }

        Gdiplus::Font nameFont(&ff, 11, Gdiplus::FontStyleBold);
        Gdiplus::SolidBrush nameBrush(Colors::Text());
        g.DrawString(sr.strategyName.c_str(), -1, &nameFont,
                     Gdiplus::PointF((float)colStrategy, (float)(y + 12)), &nameBrush);

        Gdiplus::Font statFont(&ff, 10, Gdiplus::FontStyleBold);
        std::wstring statTxt;
        Gdiplus::Color statColor = Colors::TextDim();

        if (phase == TestPhase::Running && (int)i == curStrat)
        {
            statTxt = L"\u26A1 \u0422\u0435\u0441\u0442...";
            statColor = Colors::Warning();
        }
        else if (sr.overallStatus == TestResult::Pending)
        {
            statTxt = L"\u041E\u0436\u0438\u0434\u0430\u043D\u0438\u0435";
            statColor = Colors::TextDim();
        }
        else if (sr.overallStatus == TestResult::Working)
        {
            statTxt = L"\u2713 OK";
            statColor = Colors::Accent();
        }
        else if (sr.overallStatus == TestResult::Failed)
        {
            statTxt = L"\u2717 \u041F\u043B\u043E\u0445\u043E";
            statColor = Colors::Danger();
        }
        else if (sr.overallStatus == TestResult::Skipped)
        {
            statTxt = L"\u2014";
        }

        Gdiplus::SolidBrush sBr(statColor);
        g.DrawString(statTxt.c_str(), -1, &statFont,
                     Gdiplus::PointF((float)colStatus, (float)(y + 13)), &sBr);

        if (sr.overallStatus == TestResult::Working || sr.overallStatus == TestResult::Failed)
        {
            std::wstring workTxt = std::to_wstring(sr.workingCount) + L"/" +
                std::to_wstring((int)sr.domains.size());
            Gdiplus::SolidBrush wBr(Colors::Text());
            g.DrawString(workTxt.c_str(), -1, &statFont,
                         Gdiplus::PointF((float)colStats, (float)(y + 13)), &wBr);

            if (sr.workingCount > 0)
            {
                std::wstring pingTxt = std::to_wstring(sr.avgPing) + L" \u043C\u0441";
                Gdiplus::SolidBrush pBr(Colors::Accent());
                g.DrawString(pingTxt.c_str(), -1, &statFont,
                             Gdiplus::PointF((float)colDetails, (float)(y + 13)), &pBr);
            }
        }
    }

    g.ResetClip();

    Gdiplus::SolidBrush footerBg(Colors::BgTitle());
    g.FillRectangle(&footerBg, 1, h - 60, w - 2, 59);
    Gdiplus::Pen footerLine(Colors::Border(), 1);
    g.DrawLine(&footerLine, 1, h - 60, w - 2, h - 60);

    int btnY = h - 48;

    if (phase == TestPhase::Idle || phase == TestPhase::Finished || phase == TestPhase::Cancelled)
    {
        int bx = 20, bw = 160, bh = 36;
        Gdiplus::Color bgc = m_testStartHovered ? Colors::AccentHover() : Colors::Accent();
        Gdiplus::SolidBrush bb(bgc);
        FillRoundRect(g, &bb, bx, btnY, bw, bh, 10);

        Gdiplus::Font bf(&ff, 11, Gdiplus::FontStyleBold);
        Gdiplus::SolidBrush bt(Gdiplus::Color(255, 8, 14, 12));
        Gdiplus::StringFormat sf;
        sf.SetAlignment(Gdiplus::StringAlignmentCenter);
        sf.SetLineAlignment(Gdiplus::StringAlignmentCenter);
        Gdiplus::RectF br((float)bx, (float)btnY, (float)bw, (float)bh);
        std::wstring btnTxt = (phase == TestPhase::Idle)
            ? L"\u25B6 \u0417\u0430\u043F\u0443\u0441\u0442\u0438\u0442\u044C \u0442\u0435\u0441\u0442"
            : L"\u21BB \u0417\u0430\u043F\u0443\u0441\u0442\u0438\u0442\u044C \u0437\u0430\u043D\u043E\u0432\u043E";
        g.DrawString(btnTxt.c_str(), -1, &bf, br, &sf, &bt);

        if (phase == TestPhase::Finished || phase == TestPhase::Cancelled)
        {
            int ex = 200, ew = 140;
            Gdiplus::Color ebgc = m_testExportHovered ? Colors::BgHover() : Colors::BgCard();
            Gdiplus::SolidBrush eb(ebgc);
            FillRoundRect(g, &eb, ex, btnY, ew, bh, 10);
            Gdiplus::Pen ep(Colors::Border(), 1);
            DrawRoundRect(g, &ep, ex, btnY, ew, bh, 10);

            Gdiplus::SolidBrush et(Colors::Text());
            Gdiplus::RectF er((float)ex, (float)btnY, (float)ew, (float)bh);
            g.DrawString(L"\u0421\u043E\u0445\u0440\u0430\u043D\u0438\u0442\u044C TXT", -1, &bf, er, &sf, &et);
        }
    }
    else if (phase == TestPhase::Running || phase == TestPhase::Preparing)
    {
        int bx = 20, bw = 160, bh = 36;
        Gdiplus::Color bgc = m_testCancelHovered ? Gdiplus::Color(255, 220, 60, 60) : Colors::Danger();
        Gdiplus::SolidBrush bb(bgc);
        FillRoundRect(g, &bb, bx, btnY, bw, bh, 10);

        Gdiplus::Font bf(&ff, 11, Gdiplus::FontStyleBold);
        Gdiplus::SolidBrush bt(Colors::Text());
        Gdiplus::StringFormat sf;
        sf.SetAlignment(Gdiplus::StringAlignmentCenter);
        sf.SetLineAlignment(Gdiplus::StringAlignmentCenter);
        Gdiplus::RectF br((float)bx, (float)btnY, (float)bw, (float)bh);
        g.DrawString(L"\u25A0 \u041E\u0441\u0442\u0430\u043D\u043E\u0432\u0438\u0442\u044C", -1, &bf, br, &sf, &bt);
    }

    Gdiplus::Font hintFont(&ff, 9, Gdiplus::FontStyleRegular);
    Gdiplus::SolidBrush hintBrush(Colors::TextMuted());
    g.DrawString(L"\u0422\u0435\u0441\u0442 \u043F\u0440\u043E\u0432\u0435\u0440\u044F\u0435\u0442 \u0434\u043E\u043C\u0435\u043D\u044B \u0438\u0437 list-general.txt", -1,
                 &hintFont, Gdiplus::PointF((float)(w - 320), (float)(btnY + 12)), &hintBrush);
}

void ZapretGUI::OnTestMouseMove(int x, int y)
{
    RECT cr; GetClientRect(m_hTestWnd, &cr);

    bool wasClose = m_testCloseHovered;
    bool wasStart = m_testStartHovered;
    bool wasCancel = m_testCancelHovered;
    bool wasExport = m_testExportHovered;

    m_testCloseHovered = PtInRect(x, y, cr.right - 38, 8, 30, 28);

    int btnY = cr.bottom - 48;
    TestPhase phase = m_testPhase.load();

    if (phase == TestPhase::Idle || phase == TestPhase::Finished || phase == TestPhase::Cancelled)
    {
        m_testStartHovered = PtInRect(x, y, 20, btnY, 160, 36);
        m_testCancelHovered = false;
        if (phase == TestPhase::Finished || phase == TestPhase::Cancelled)
            m_testExportHovered = PtInRect(x, y, 200, btnY, 140, 36);
        else
            m_testExportHovered = false;
    }
    else
    {
        m_testStartHovered = false;
        m_testExportHovered = false;
        m_testCancelHovered = PtInRect(x, y, 20, btnY, 160, 36);
    }

    if (wasClose != m_testCloseHovered || wasStart != m_testStartHovered ||
        wasCancel != m_testCancelHovered || wasExport != m_testExportHovered)
        InvalidateRect(m_hTestWnd, nullptr, FALSE);

    if (m_testCloseHovered || m_testStartHovered || m_testCancelHovered || m_testExportHovered)
        SetCursor(LoadCursor(nullptr, IDC_HAND));
    else
        SetCursor(LoadCursor(nullptr, IDC_ARROW));
}

void ZapretGUI::OnTestClick(int x, int y)
{
    if (!m_hTestWnd) return;

    if (m_testCloseHovered)
    {
        PostMessage(m_hTestWnd, WM_CLOSE, 0, 0);
        return;
    }

    if (m_testStartHovered)
    {
        StartTest();
        return;
    }

    if (m_testCancelHovered)
    {
        CancelTest();
        return;
    }

    if (m_testExportHovered)
    {
        ExportTestResults();
        return;
    }
}

void ZapretGUI::OnTestWheel(int delta)
{
    RECT cr; GetClientRect(m_hTestWnd, &cr);
    int viewH = cr.bottom - 105 - 60;
    int contentH = (int)m_testResults.size() * 44;

    if (contentH <= viewH) { m_testScrollY = 0; return; }

    m_testScrollY += (delta > 0) ? 44 : -44;
    int maxScroll = -(contentH - viewH);
    if (m_testScrollY > 0) m_testScrollY = 0;
    if (m_testScrollY < maxScroll) m_testScrollY = maxScroll;

    InvalidateRect(m_hTestWnd, nullptr, FALSE);
}

std::vector<std::wstring> ZapretGUI::LoadTestDomains()
{
    std::vector<std::wstring> domains;
    if (!m_pathsValid) return domains;

    fs::path listFile = m_listsPath / L"list-general.txt";
    if (!fs::exists(listFile)) return domains;

    std::wifstream f(listFile);
    if (!f.is_open()) return domains;

    std::wstring line;
    int count = 0;
    const int MAX_DOMAINS = 15;

    while (std::getline(f, line) && count < MAX_DOMAINS)
    {
        while (!line.empty() && (line.back() == L'\r' || line.back() == L' ' || line.back() == L'\t'))
            line.pop_back();
        while (!line.empty() && (line.front() == L' ' || line.front() == L'\t'))
            line.erase(0, 1);

        if (line.empty()) continue;
        if (line[0] == L'#') continue;
        if (line[0] == L';') continue;

        if (line.find(L'.') == std::wstring::npos) continue;

        domains.push_back(line);
        count++;
    }
    f.close();

    if (domains.empty())
    {
        domains.push_back(L"www.youtube.com");
        domains.push_back(L"discord.com");
        domains.push_back(L"www.google.com");
        domains.push_back(L"github.com");
        domains.push_back(L"twitter.com");
    }

    return domains;
}

bool ZapretGUI::TestDomain(const std::wstring& domain, int& pingMs)
{
    pingMs = -1;

    HINTERNET hSession = WinHttpOpen(L"ZapretGui/1.0",
        WINHTTP_ACCESS_TYPE_NO_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);

    if (!hSession) return false;

    DWORD timeout = 5000;
    WinHttpSetTimeouts(hSession, timeout, timeout, timeout, timeout);

    auto start = std::chrono::steady_clock::now();

    HINTERNET hConnect = WinHttpConnect(hSession, domain.c_str(),
        INTERNET_DEFAULT_HTTPS_PORT, 0);

    if (!hConnect)
    {
        WinHttpCloseHandle(hSession);
        return false;
    }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"HEAD", L"/",
        nullptr, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE);

    if (!hRequest)
    {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    DWORD flags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
                  SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
                  SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
                  SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;
    WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &flags, sizeof(flags));

    BOOL ok = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        WINHTTP_NO_REQUEST_DATA, 0, 0, 0);

    if (ok) ok = WinHttpReceiveResponse(hRequest, nullptr);

    DWORD statusCode = 0;
    if (ok)
    {
        DWORD sz = sizeof(statusCode);
        WinHttpQueryHeaders(hRequest,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &sz,
            WINHTTP_NO_HEADER_INDEX);
    }

    auto end = std::chrono::steady_clock::now();
    pingMs = (int)std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return ok && statusCode > 0 && statusCode < 500;
}

std::wstring ZapretGUI::ResultToString(TestResult r)
{
    switch (r)
    {
    case TestResult::Pending: return L"Pending";
    case TestResult::Testing: return L"Testing";
    case TestResult::Working: return L"Work";
    case TestResult::Failed: return L"Not worked";
    case TestResult::Skipped: return L"Skipped";
    }
    return L"";
}

void ZapretGUI::StartTest()
{
    if (m_testPhase == TestPhase::Running || m_testPhase == TestPhase::Preparing)
        return;

    if (m_testThread.joinable()) m_testThread.join();

    auto domains = LoadTestDomains();
    if (domains.empty())
    {
        MessageBoxW(m_hTestWnd,
            L"\u041D\u0435 \u043D\u0430\u0439\u0434\u0435\u043D\u044B \u0434\u043E\u043C\u0435\u043D\u044B \u0434\u043B\u044F \u0442\u0435\u0441\u0442\u0430.\n"
            L"\u041F\u0440\u043E\u0432\u0435\u0440\u044C\u0442\u0435 lists/list-general.txt",
            L"Zapret Gui", MB_ICONWARNING);
        return;
    }

    m_savedConnectionState = IsWinwsRunning();
    m_savedStrategyIndex = m_selectedBatIndex;

    {
        std::lock_guard<std::mutex> lk(m_testResultsMutex);
        m_testResults.clear();
        for (size_t i = 0; i < m_strategies.size(); i++)
        {
            StrategyTestResult sr;
            sr.strategyName = m_strategies[i].displayName;
            sr.strategyIndex = (int)i;
            for (auto& d : domains)
            {
                DomainTestResult dr;
                dr.domain = d;
                sr.domains.push_back(dr);
            }
            m_testResults.push_back(sr);
        }
    }

    m_testCancelRequested = false;
    m_testCurrentStrategy = 0;
    m_testCurrentDomain = 0;
    m_testPhase = TestPhase::Preparing;
    m_testScrollY = 0;

    m_testThread = std::thread(&ZapretGUI::RunTestThread, this);
}

void ZapretGUI::CancelTest()
{
    m_testCancelRequested = true;
}

void ZapretGUI::RestoreConnectionState()
{
    StopWinws();

    if (m_savedConnectionState && m_savedStrategyIndex >= 0 &&
        m_savedStrategyIndex < (int)m_strategies.size())
    {
        m_selectedBatIndex = m_savedStrategyIndex;
        StartWinws(m_strategies[m_savedStrategyIndex]);
    }
}

void ZapretGUI::RunTestThread()
{
    m_testPhase = TestPhase::Running;

    StopWinws();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    int totalStrategies;
    {
        std::lock_guard<std::mutex> lk(m_testResultsMutex);
        totalStrategies = (int)m_testResults.size();
    }

    for (int s = 0; s < totalStrategies; s++)
    {
        if (m_testCancelRequested) break;

        m_testCurrentStrategy = s;

        int stratIdx;
        std::vector<std::wstring> domainsToTest;
        {
            std::lock_guard<std::mutex> lk(m_testResultsMutex);
            stratIdx = m_testResults[s].strategyIndex;
            for (auto& d : m_testResults[s].domains)
                domainsToTest.push_back(d.domain);
        }

        if (stratIdx < 0 || stratIdx >= (int)m_strategies.size()) continue;

        StartWinws(m_strategies[stratIdx]);

        for (int wait = 0; wait < 30; wait++)
        {
            if (m_testCancelRequested) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        if (m_testCancelRequested) { StopWinws(); break; }

        int totalPing = 0, workingCount = 0, failedCount = 0;

        for (size_t d = 0; d < domainsToTest.size(); d++)
        {
            if (m_testCancelRequested) break;

            m_testCurrentDomain = (int)d;

            {
                std::lock_guard<std::mutex> lk(m_testResultsMutex);
                m_testResults[s].domains[d].result = TestResult::Testing;
            }

            int ping = -1;
            bool ok = TestDomain(domainsToTest[d], ping);

            {
                std::lock_guard<std::mutex> lk(m_testResultsMutex);
                m_testResults[s].domains[d].result = ok ? TestResult::Working : TestResult::Failed;
                m_testResults[s].domains[d].pingMs = ping;
            }

            if (ok) { workingCount++; totalPing += ping; }
            else failedCount++;
        }

        {
            std::lock_guard<std::mutex> lk(m_testResultsMutex);
            m_testResults[s].workingCount = workingCount;
            m_testResults[s].failedCount = failedCount;
            m_testResults[s].avgPing = (workingCount > 0) ? (totalPing / workingCount) : 0;

            if (m_testCancelRequested) m_testResults[s].overallStatus = TestResult::Skipped;
            else if (workingCount > (int)domainsToTest.size() / 2)
                m_testResults[s].overallStatus = TestResult::Working;
            else
                m_testResults[s].overallStatus = TestResult::Failed;
        }

        StopWinws();
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    if (!m_testCancelRequested)
    {
        int bestIdx = -1;
        int bestScore = -1;
        int bestPing = 99999;

        {
            std::lock_guard<std::mutex> lk(m_testResultsMutex);
            for (size_t i = 0; i < m_testResults.size(); i++)
            {
                auto& sr = m_testResults[i];
                if (sr.overallStatus != TestResult::Working) continue;
                if (sr.workingCount == 0) continue;

                int score = sr.workingCount * 1000 - sr.avgPing;

                if (score > bestScore ||
                    (score == bestScore && sr.avgPing < bestPing))
                {
                    bestScore = score;
                    bestIdx = sr.strategyIndex;
                    bestPing = sr.avgPing;
                }
            }
        }

        if (bestIdx >= 0 && bestIdx < (int)m_strategies.size())
        {
            m_selectedBatIndex = bestIdx;
            m_savedStrategyIndex = bestIdx;
            m_savedConnectionState = true;
            SaveConfig();
        }
    }

    RestoreConnectionState();

    m_testPhase = m_testCancelRequested ? TestPhase::Cancelled : TestPhase::Finished;
}
void ZapretGUI::ExportTestResults()
{
    wchar_t path[MAX_PATH] = L"zapret_test_results.txt";

    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hTestWnd;
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"Text Files\0*.txt\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrDefExt = L"txt";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

    if (!GetSaveFileNameW(&ofn)) return;

    std::wofstream f(path);
    if (!f.is_open())
    {
        MessageBoxW(m_hTestWnd, L"\u041D\u0435 \u0443\u0434\u0430\u043B\u043E\u0441\u044C \u0441\u043E\u0445\u0440\u0430\u043D\u0438\u0442\u044C \u0444\u0430\u0439\u043B.",
            L"Zapret Gui", MB_ICONERROR);
        return;
    }

    f << L"Zapret Gui - Test Results\n";
    f << L"========================================\n\n";

    std::lock_guard<std::mutex> lk(m_testResultsMutex);
    for (auto& sr : m_testResults)
    {
        f << L"Strategy: " << sr.strategyName << L"\n";
        f << L"Working: " << sr.workingCount << L"/" << sr.domains.size();
        if (sr.workingCount > 0) f << L"   Avg ping: " << sr.avgPing << L" ms";
        f << L"\n";
        f << L"----------------------------------------\n";

        for (auto& dr : sr.domains)
        {
            f << L"  " << dr.domain;
            int pad = 35 - (int)dr.domain.length();
            for (int i = 0; i < pad; i++) f << L' ';
            f << ResultToString(dr.result);
            if (dr.result == TestResult::Working && dr.pingMs >= 0)
                f << L"  (" << dr.pingMs << L" ms)";
            f << L"\n";
        }
        f << L"\n";
    }

    f.close();

    MessageBoxW(m_hTestWnd, L"\u0420\u0435\u0437\u0443\u043B\u044C\u0442\u0430\u0442\u044B \u0441\u043E\u0445\u0440\u0430\u043D\u0435\u043D\u044B.",
        L"Zapret Gui", MB_ICONINFORMATION);
}
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdbool.h>

#include "input.h"
#include "sim.h"
#include "ui.h"

typedef struct
{
    SimState sim;
    UiState ui;
    LARGE_INTEGER perf_freq;
    double last_tick_s;
} AppState;

static double app_query_time(const AppState *app)
{
    LARGE_INTEGER counter;
    if ((app != NULL) && (app->perf_freq.QuadPart != 0) && QueryPerformanceCounter(&counter))
    {
        return (double)counter.QuadPart / (double)app->perf_freq.QuadPart;
    }

    return (double)GetTickCount64() / 1000.0;
}

static void app_update(AppState *app, double dt)
{
    const double clamped_dt = (dt > 0.05) ? 0.05 : (dt > 0.0 ? dt : (1.0 / 60.0));
    sim_step(&app->sim, clamped_dt);
}

static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    AppState *app = (AppState *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (message)
    {
        case WM_CREATE:
        {
            CREATESTRUCT *create_struct = (CREATESTRUCT *)lParam;
            app = (AppState *)create_struct->lpCreateParams;
            if (app == NULL)
            {
                return -1;
            }

            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)app);
            sim_init(&app->sim);
            if (!QueryPerformanceFrequency(&app->perf_freq))
            {
                app->perf_freq.QuadPart = 0;
            }
            app->last_tick_s = app_query_time(app);
            ui_init(&app->ui, hwnd);
            SetTimer(hwnd, 1U, 16U, NULL);
            return 0;
        }
        case WM_SIZE:
            if (app != NULL)
            {
                const int width = (int)LOWORD(lParam);
                const int height = (int)HIWORD(lParam);
                ui_resize(&app->ui, hwnd, width, height);
            }
            return 0;
        case WM_TIMER:
            if ((app != NULL) && (wParam == 1U))
            {
                const double now = app_query_time(app);
                double dt = now - app->last_tick_s;
                if (dt < 0.0)
                {
                    dt = 0.0;
                }
                app->last_tick_s = now;
                app_update(app, dt);
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
        case WM_ERASEBKGND:
            return 1;
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            if (app != NULL)
            {
                const bool is_repeat = ((lParam & (1L << 30)) != 0);
                input_handle_key(&app->sim, (unsigned int)wParam, INPUT_EVENT_KEY_DOWN, is_repeat);
            }
            return 0;
        case WM_KEYUP:
        case WM_SYSKEYUP:
            if (app != NULL)
            {
                input_handle_key(&app->sim, (unsigned int)wParam, INPUT_EVENT_KEY_UP, false);
            }
            return 0;
        case WM_PAINT:
            if (app != NULL)
            {
                PAINTSTRUCT ps;
                HDC dc = BeginPaint(hwnd, &ps);
                ui_render(&app->ui, dc, &app->sim);
                EndPaint(hwnd, &ps);
                return 0;
            }
            break;
        case WM_DESTROY:
            if (app != NULL)
            {
                KillTimer(hwnd, 1U);
                ui_destroy(&app->ui);
            }
            PostQuitMessage(0);
            return 0;
        default:
            break;
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE prev, LPWSTR cmd, int show)
{
    (void)prev;
    (void)cmd;

    WNDCLASSEXW wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"CockpitWindowClass";
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

    if (RegisterClassExW(&wc) == 0)
    {
        return 0;
    }

    AppState app_state;
    ZeroMemory(&app_state, sizeof(app_state));

    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"HVAC Cockpit Simulator",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720,
        NULL, NULL, instance, &app_state);

    if (hwnd == NULL)
    {
        return 0;
    }

    ShowWindow(hwnd, show);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}

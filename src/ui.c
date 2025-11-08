#include "ui.h"

#include <math.h>
#include <stdio.h>

static HFONT ui_create_font(int height, int weight)
{
    return CreateFontW(height, 0, 0, 0, weight, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, VARIABLE_PITCH, L"Segoe UI");
}

static void ui_release_backbuffer(UiState *ui)
{
    if (ui->back_dc != NULL)
    {
        if (ui->back_bitmap != NULL)
        {
            if (ui->back_dc_old != NULL)
            {
                SelectObject(ui->back_dc, ui->back_dc_old);
            }
            DeleteObject(ui->back_bitmap);
            ui->back_bitmap = NULL;
        }
        DeleteDC(ui->back_dc);
        ui->back_dc = NULL;
        ui->back_dc_old = NULL;
    }
}

static double clamp01(double value)
{
    double result = value;
    if (result < 0.0)
    {
        result = 0.0;
    }
    else if (result > 1.0)
    {
        result = 1.0;
    }
    else
    {
        /* no action */
    }
    return result;
}

static double deg_to_rad(double degrees)
{
    return degrees * (3.14159265358979323846 / 180.0);
}

static int ui_round_to_int(double value)
{
    return (int)((value >= 0.0) ? (value + 0.5) : (value - 0.5));
}

static POINT ui_polar_point(int cx, int cy, int radius, double angle_rad)
{
    POINT pt;
    pt.x = cx + ui_round_to_int(cos(angle_rad) * (double)radius);
    pt.y = cy - ui_round_to_int(sin(angle_rad) * (double)radius);
    return pt;
}

static void ui_draw_gauge_band(HDC dc, int cx, int cy, int radius, double start_deg,
    double sweep_deg, double start_fraction, double end_fraction, COLORREF color, int thickness)
{
    if ((dc == NULL) || (radius <= 0) || (thickness <= 0))
    {
        return;
    }

    const double start_angle = deg_to_rad(start_deg - (start_fraction * sweep_deg));
    const double end_angle = deg_to_rad(start_deg - (end_fraction * sweep_deg));
    const POINT start_pt = ui_polar_point(cx, cy, radius, start_angle);
    const POINT end_pt = ui_polar_point(cx, cy, radius, end_angle);

    LOGBRUSH brush;
    brush.lbStyle = BS_SOLID;
    brush.lbColor = color;
    brush.lbHatch = 0;

    HPEN band_pen = ExtCreatePen(
        PS_GEOMETRIC | PS_ENDCAP_ROUND | PS_JOIN_ROUND, thickness, &brush, 0, NULL);
    if (band_pen == NULL)
    {
        return;
    }

    HGDIOBJ prev_pen = SelectObject(dc, band_pen);
    Arc(dc, cx - radius, cy - radius, cx + radius, cy + radius,
        start_pt.x, start_pt.y, end_pt.x, end_pt.y);
    SelectObject(dc, prev_pen);
    DeleteObject(band_pen);
}

static void ui_draw_gauge(HDC dc, int cx, int cy, int radius, double value, double min_value,
    double max_value, const wchar_t *label, const wchar_t *unit, COLORREF accent)
{
    if (radius <= 0)
    {
        return;
    }

    const double normalized = clamp01((value - min_value) / (max_value - min_value));
    const double start_deg = 135.0;
    const double sweep_deg = 270.0;
    const double angle_deg = start_deg - (normalized * sweep_deg);
    const double angle_rad = deg_to_rad(angle_deg);
    const int inner_radius = radius - 16;

    HPEN ring_pen = CreatePen(PS_SOLID, 3, accent);
    HBRUSH face_brush = CreateSolidBrush(RGB(25, 25, 25));
    HPEN tick_pen = CreatePen(PS_SOLID, 1, RGB(180, 180, 180));
    HPEN needle_pen = CreatePen(PS_SOLID, 4, RGB(220, 80, 50));
    HBRUSH center_brush = CreateSolidBrush(RGB(40, 40, 40));

    HGDIOBJ prev_pen = SelectObject(dc, ring_pen);
    HGDIOBJ prev_brush = SelectObject(dc, face_brush);
    Ellipse(dc, cx - radius, cy - radius, cx + radius, cy + radius);

    const int band_radius = radius - 6;
    const int band_thickness = 12;
    const struct
    {
        double start_fraction;
        double end_fraction;
        COLORREF color;
    } bands[] = {
        {0.0, 0.6, RGB(70, 180, 120)},
        {0.6, 0.85, RGB(230, 200, 120)},
        {0.85, 1.0, RGB(235, 100, 90)},
    };
    for (int i = 0; i < (int)(sizeof(bands) / sizeof(bands[0])); ++i)
    {
        ui_draw_gauge_band(dc, cx, cy, band_radius, start_deg, sweep_deg,
            bands[i].start_fraction, bands[i].end_fraction, bands[i].color, band_thickness);
    }

    SelectObject(dc, tick_pen);
    for (int i = 0; i <= 10; ++i)
    {
        const double fraction = (double)i / 10.0;
        const double tick_angle = deg_to_rad(start_deg - (fraction * sweep_deg));
        const int long_tick = (i % 2 == 0) ? 12 : 6;
        const int x_outer = cx + (int)(cos(tick_angle) * (radius - 4));
        const int y_outer = cy - (int)(sin(tick_angle) * (radius - 4));
        const int x_inner = cx + (int)(cos(tick_angle) * (radius - 4 - long_tick));
        const int y_inner = cy - (int)(sin(tick_angle) * (radius - 4 - long_tick));
        MoveToEx(dc, x_inner, y_inner, NULL);
        LineTo(dc, x_outer, y_outer);

        if ((i % 2) == 0)
        {
            const double label_radius = (double)radius - 32.0;
            const int label_x = cx + ui_round_to_int(cos(tick_angle) * label_radius);
            const int label_y = cy - ui_round_to_int(sin(tick_angle) * label_radius);
            const double tick_value = min_value + (max_value - min_value) * fraction;
            wchar_t tick_text[16];
            (void)_snwprintf_s(tick_text, sizeof(tick_text) / sizeof(tick_text[0]),
                _TRUNCATE, L"%d", ui_round_to_int(tick_value));
            RECT tick_rect = {label_x - 25, label_y - 12, label_x + 25, label_y + 12};
            DrawTextW(dc, tick_text, -1, &tick_rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
    }

    SelectObject(dc, needle_pen);
    MoveToEx(dc, cx, cy, NULL);
    const int needle_x = cx + (int)(cos(angle_rad) * inner_radius);
    const int needle_y = cy - (int)(sin(angle_rad) * inner_radius);
    LineTo(dc, needle_x, needle_y);

    SelectObject(dc, center_brush);
    SelectObject(dc, ring_pen);
    Ellipse(dc, cx - 10, cy - 10, cx + 10, cy + 10);

    SelectObject(dc, prev_pen);
    SelectObject(dc, prev_brush);
    DeleteObject(ring_pen);
    DeleteObject(face_brush);
    DeleteObject(tick_pen);
    DeleteObject(needle_pen);
    DeleteObject(center_brush);

    wchar_t value_text[32];
    (void)_snwprintf_s(value_text, sizeof(value_text) / sizeof(value_text[0]),
        _TRUNCATE, L"%0.0f %ls", value, unit);
    RECT text_rect = {cx - radius, cy + radius - 40, cx + radius, cy + radius};
    DrawTextW(dc, value_text, -1, &text_rect, DT_CENTER | DT_TOP);

    RECT label_rect = {cx - radius, cy + radius - 70, cx + radius, cy + radius - 40};
    DrawTextW(dc, label, -1, &label_rect, DT_CENTER | DT_BOTTOM);
}

static void ui_draw_fuel(HDC dc, RECT bounds, double fuel_pct)
{
    const double clamped = clamp01(fuel_pct / 100.0);
    HBRUSH frame_brush = CreateSolidBrush(RGB(60, 60, 60));
    FrameRect(dc, &bounds, frame_brush);
    DeleteObject(frame_brush);

    RECT fill_rect = bounds;
    fill_rect.right = fill_rect.left + (int)((bounds.right - bounds.left) * clamped);
    HBRUSH fill_brush = CreateSolidBrush(RGB(120, 200, 80));
    FillRect(dc, &fill_rect, fill_brush);
    DeleteObject(fill_brush);

    wchar_t text[16];
    (void)_snwprintf_s(text, sizeof(text) / sizeof(text[0]), _TRUNCATE, L"FUEL %0.0f%%", fuel_pct);
    DrawTextW(dc, text, -1, &bounds, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

static void ui_draw_indicator(HDC dc, RECT bounds, const wchar_t *label, bool active, COLORREF on_color)
{
    HBRUSH brush = CreateSolidBrush(active ? on_color : RGB(40, 40, 40));
    FillRect(dc, &bounds, brush);
    DeleteObject(brush);
    FrameRect(dc, &bounds, (HBRUSH)GetStockObject(GRAY_BRUSH));
    DrawTextW(dc, label, -1, &bounds, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

static void ui_draw_button(HDC dc, RECT bounds, const wchar_t *label, bool active)
{
    const COLORREF active_color = RGB(70, 140, 220);
    const COLORREF inactive_color = RGB(60, 60, 60);
    HBRUSH brush = CreateSolidBrush(active ? active_color : inactive_color);
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(120, 120, 120));
    HGDIOBJ old_pen = SelectObject(dc, pen);
    HGDIOBJ old_brush = SelectObject(dc, brush);
    RoundRect(dc, bounds.left, bounds.top, bounds.right, bounds.bottom, 10, 10);
    SelectObject(dc, old_pen);
    SelectObject(dc, old_brush);
    DeleteObject(brush);
    DeleteObject(pen);

    DrawTextW(dc, label, -1, &bounds, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

static void ui_draw_fan_bars(HDC dc, RECT bounds, int fan_level)
{
    const int total_bars = 7;
    const int bar_spacing = 4;
    const int bar_width = (bounds.right - bounds.left - ((total_bars - 1) * bar_spacing)) / total_bars;
    if (bar_width <= 0)
    {
        return;
    }
    int x = bounds.left;
    for (int i = 0; i < total_bars; ++i)
    {
        RECT bar = {x, bounds.top, x + bar_width, bounds.bottom};
        HBRUSH brush = CreateSolidBrush((i < fan_level) ? RGB(255, 170, 70) : RGB(60, 60, 60));
        FillRect(dc, &bar, brush);
        DeleteObject(brush);
        x += bar_width + bar_spacing;
    }
}

static void ui_draw_airflow_icons(HDC dc, RECT bounds, HvacAirflowMode mode)
{
    const int icon_width = (bounds.right - bounds.left) / 3;
    if (icon_width <= 0)
    {
        return;
    }
    for (int i = 0; i < 3; ++i)
    {
        RECT icon = {bounds.left + i * icon_width + 4, bounds.top + 4,
            bounds.left + (i + 1) * icon_width - 4, bounds.bottom - 4};
        const bool active = (i == (int)mode);
        HBRUSH brush = CreateSolidBrush(active ? RGB(80, 180, 220) : RGB(50, 50, 50));
        HPEN pen = CreatePen(PS_SOLID, 1, RGB(120, 120, 120));
        HGDIOBJ old_pen = SelectObject(dc, pen);
        HGDIOBJ old_brush = SelectObject(dc, brush);
        RoundRect(dc, icon.left, icon.top, icon.right, icon.bottom, 12, 12);
        SelectObject(dc, old_pen);
        SelectObject(dc, old_brush);
        DeleteObject(brush);
        DeleteObject(pen);

        HPEN shape_pen = CreatePen(PS_SOLID, 2, RGB(220, 220, 220));
        HGDIOBJ prev_shape_pen = SelectObject(dc, shape_pen);

        switch (i)
        {
            case 0: /* face icon */
            {
                int cx = (icon.left + icon.right) / 2;
                int cy = icon.top + (icon.bottom - icon.top) / 3;
                Ellipse(dc, cx - 8, cy - 8, cx + 8, cy + 8);
                MoveToEx(dc, cx, cy + 8, NULL);
                LineTo(dc, cx, icon.bottom - 8);
                break;
            }
            case 1: /* bi-level */
            {
                int mid = (icon.top + icon.bottom) / 2;
                Rectangle(dc, icon.left + 6, icon.top + 6, icon.right - 6, mid - 4);
                Rectangle(dc, icon.left + 6, mid + 4, icon.right - 6, icon.bottom - 6);
                break;
            }
            case 2: /* foot */
            {
                int base = icon.bottom - 6;
                MoveToEx(dc, icon.left + 8, base, NULL);
                LineTo(dc, icon.right - 8, base);
                Arc(dc, icon.left + 4, icon.top + 6, icon.right - 4, icon.bottom, icon.left + 4, base, icon.right - 4, base);
                break;
            }
            default:
                break;
        }

        SelectObject(dc, prev_shape_pen);
        DeleteObject(shape_pen);
    }
}

void ui_init(UiState *ui, HWND hwnd)
{
    if ((ui == NULL) || (hwnd == NULL))
    {
        return;
    }

    ui->back_dc = NULL;
    ui->back_bitmap = NULL;
    ui->back_dc_old = NULL;
    ui->width = 1;
    ui->height = 1;
    ui->label_font = ui_create_font(-24, FW_SEMIBOLD);
    ui->small_font = ui_create_font(-18, FW_NORMAL);
    ui_resize(ui, hwnd, 800, 600);
}

void ui_resize(UiState *ui, HWND hwnd, int width, int height)
{
    if ((ui == NULL) || (hwnd == NULL))
    {
        return;
    }

    ui->width = (width > 0) ? width : 1;
    ui->height = (height > 0) ? height : 1;

    HDC window_dc = GetDC(hwnd);
    if (window_dc == NULL)
    {
        return;
    }

    if (ui->back_dc == NULL)
    {
        ui->back_dc = CreateCompatibleDC(window_dc);
    }

    if (ui->back_dc != NULL)
    {
        if (ui->back_bitmap != NULL)
        {
            SelectObject(ui->back_dc, ui->back_dc_old);
            DeleteObject(ui->back_bitmap);
            ui->back_bitmap = NULL;
        }

        ui->back_bitmap = CreateCompatibleBitmap(window_dc, ui->width, ui->height);
        if (ui->back_bitmap != NULL)
        {
            HGDIOBJ previous = SelectObject(ui->back_dc, ui->back_bitmap);
            if (ui->back_dc_old == NULL)
            {
                ui->back_dc_old = previous;
            }
        }
    }

    ReleaseDC(hwnd, window_dc);
}

static void ui_draw_background(HDC dc, int width, int height)
{
    RECT rect = {0, 0, width, height};
    HBRUSH bg = CreateSolidBrush(RGB(20, 20, 20));
    FillRect(dc, &rect, bg);
    DeleteObject(bg);
}

static void ui_draw_panel_outline(HDC dc, RECT bounds)
{
    HBRUSH brush = CreateSolidBrush(RGB(35, 35, 35));
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(80, 80, 80));
    HGDIOBJ old_pen = SelectObject(dc, pen);
    HGDIOBJ old_brush = SelectObject(dc, brush);
    RoundRect(dc, bounds.left, bounds.top, bounds.right, bounds.bottom, 20, 20);
    SelectObject(dc, old_pen);
    SelectObject(dc, old_brush);
    DeleteObject(brush);
    DeleteObject(pen);
}

void ui_render(UiState *ui, HDC target_dc, const SimState *sim)
{
    if ((ui == NULL) || (target_dc == NULL) || (sim == NULL))
    {
        return;
    }

    if ((ui->back_dc == NULL) || (ui->back_bitmap == NULL))
    {
        ui_draw_background(target_dc, ui->width, ui->height);
        return;
    }

    HDC dc = ui->back_dc;
    ui_draw_background(dc, ui->width, ui->height);

    if (ui->label_font != NULL)
    {
        SelectObject(dc, ui->label_font);
    }
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(230, 230, 230));

    const int gauge_area_height = ui->height / 2;
    int gauge_radius = ui->width / 4;
    const int max_radius = gauge_area_height - 32;
    if ((max_radius > 0) && (max_radius < gauge_radius))
    {
        gauge_radius = max_radius;
    }
    if (gauge_radius < 60)
    {
        gauge_radius = 60;
    }
    const int gauge_center_y = gauge_area_height - 20;
    const int speed_center_x = ui->width / 4;
    const int rpm_center_x = (ui->width * 3) / 4;

    ui_draw_gauge(dc, speed_center_x, gauge_center_y, gauge_radius,
        sim->velocity_kmh, 0.0, 200.0, L"SPEED", L"km/h", RGB(90, 180, 230));
    ui_draw_gauge(dc, rpm_center_x, gauge_center_y, gauge_radius,
        sim->rpm, 0.0, 7000.0, L"RPM", L"rpm", RGB(230, 150, 80));

    if (ui->small_font != NULL)
    {
        SelectObject(dc, ui->small_font);
    }

    RECT fuel_rect;
    fuel_rect.left = ui->width / 4;
    fuel_rect.right = (ui->width * 3) / 4;
    fuel_rect.top = gauge_area_height;
    fuel_rect.bottom = fuel_rect.top + 30;
    ui_draw_fuel(dc, fuel_rect, sim->fuel_pct);

    RECT indicator_area = {ui->width / 2 - 180, 10, ui->width / 2 + 180, 70};
    const int indicator_width = 90;
    RECT left_rect = {indicator_area.left, indicator_area.top,
        indicator_area.left + indicator_width, indicator_area.bottom};
    RECT hazard_rect = {left_rect.right + 10, indicator_area.top,
        left_rect.right + 10 + indicator_width, indicator_area.bottom};
    RECT right_rect = {hazard_rect.right + 10, indicator_area.top,
        hazard_rect.right + 10 + indicator_width, indicator_area.bottom};
    RECT headlight_rect = {right_rect.right + 10, indicator_area.top,
        right_rect.right + 10 + indicator_width, indicator_area.bottom};

    const IndicatorState *ind = &sim->indicators;
    const bool left_on = (ind->hazard_enabled || ind->left_enabled) && ind->blink_on;
    const bool right_on = (ind->hazard_enabled || ind->right_enabled) && ind->blink_on;
    const bool hazard_on = ind->hazard_enabled && ind->blink_on;

    ui_draw_indicator(dc, left_rect, L"LEFT", left_on, RGB(120, 220, 120));
    ui_draw_indicator(dc, hazard_rect, L"HAZ", hazard_on, RGB(220, 120, 120));
    ui_draw_indicator(dc, right_rect, L"RIGHT", right_on, RGB(120, 220, 120));
    ui_draw_indicator(dc, headlight_rect, L"HEAD", ind->headlight_on, RGB(120, 180, 255));

    RECT panel = {20, gauge_area_height + 50, ui->width - 20, ui->height - 20};
    ui_draw_panel_outline(dc, panel);

    RECT inner = panel;
    InflateRect(&inner, -20, -20);
    const int segment_height = (inner.bottom - inner.top) / 3;

    RECT temps_rect = {inner.left, inner.top, inner.right, inner.top + segment_height};
    RECT fan_rect = {inner.left, temps_rect.bottom + 10, inner.right, temps_rect.bottom + 10 + segment_height / 2};
    RECT buttons_rect = {inner.left, fan_rect.bottom + 10, inner.right, inner.bottom};

    wchar_t setpoint_text[32];
    (void)_snwprintf_s(setpoint_text, sizeof(setpoint_text) / sizeof(setpoint_text[0]),
        _TRUNCATE, L"SET %0.1f C", sim->hvac.setpoint_c);
    RECT setpoint_rect = temps_rect;
    setpoint_rect.right = inner.left + (inner.right - inner.left) / 3;
    DrawTextW(dc, setpoint_text, -1, &setpoint_rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    wchar_t cabin_text[32];
    (void)_snwprintf_s(cabin_text, sizeof(cabin_text) / sizeof(cabin_text[0]),
        _TRUNCATE, L"CABIN %0.1f C", sim->hvac.cabin_temp_c);
    RECT cabin_rect = temps_rect;
    cabin_rect.left = setpoint_rect.right;
    cabin_rect.right = cabin_rect.left + (inner.right - inner.left) / 3;
    DrawTextW(dc, cabin_text, -1, &cabin_rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    wchar_t outside_text[32];
    (void)_snwprintf_s(outside_text, sizeof(outside_text) / sizeof(outside_text[0]),
        _TRUNCATE, L"OUT %0.1f C", sim->hvac.outside_temp_c);
    RECT outside_rect = temps_rect;
    outside_rect.left = cabin_rect.right;
    DrawTextW(dc, outside_text, -1, &outside_rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    RECT fan_label_rect = fan_rect;
    fan_label_rect.bottom = fan_label_rect.top + 24;
    DrawTextW(dc, L"FAN SPEED", -1, &fan_label_rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    RECT fan_bar_rect = fan_rect;
    fan_bar_rect.top = fan_label_rect.bottom + 4;
    ui_draw_fan_bars(dc, fan_bar_rect, sim->hvac.fan_level);

    RECT airflow_rect = fan_rect;
    airflow_rect.left = fan_rect.right - 220;
    airflow_rect.top = fan_label_rect.top;
    airflow_rect.bottom = fan_rect.bottom;
    DrawTextW(dc, L"AIRFLOW", -1, &airflow_rect, DT_RIGHT | DT_TOP | DT_SINGLELINE);
    airflow_rect.top += 20;
    ui_draw_airflow_icons(dc, airflow_rect, sim->hvac.airflow_mode);

    const int button_width = 100;
    const int button_height = 40;
    int button_x = buttons_rect.left;
    const int button_gap = 12;

    RECT ac_rect = {button_x, buttons_rect.top, button_x + button_width, buttons_rect.top + button_height};
    ui_draw_button(dc, ac_rect, L"AC", sim->hvac.ac_on);
    button_x += button_width + button_gap;

    RECT auto_rect = {button_x, buttons_rect.top, button_x + button_width, buttons_rect.top + button_height};
    ui_draw_button(dc, auto_rect, L"AUTO", sim->hvac.auto_mode);
    button_x += button_width + button_gap;

    RECT recirc_rect = {button_x, buttons_rect.top, button_x + button_width, buttons_rect.top + button_height};
    ui_draw_button(dc, recirc_rect, L"RECIRC", sim->hvac.recirculation_on);
    button_x += button_width + button_gap;

    RECT defrost_rect = {button_x, buttons_rect.top, button_x + button_width, buttons_rect.top + button_height};
    ui_draw_button(dc, defrost_rect, L"DEF", sim->hvac.defrost_on);
    button_x += button_width + button_gap;

    RECT mode_rect = {button_x, buttons_rect.top, button_x + button_width, buttons_rect.top + button_height};
    const wchar_t *mode_label = L"FACE";
    if (sim->hvac.airflow_mode == HVAC_AIRFLOW_BI_LEVEL)
    {
        mode_label = L"BI";
    }
    else if (sim->hvac.airflow_mode == HVAC_AIRFLOW_FOOT)
    {
        mode_label = L"FOOT";
    }
    ui_draw_button(dc, mode_rect, mode_label, true);

    BitBlt(target_dc, 0, 0, ui->width, ui->height, dc, 0, 0, SRCCOPY);
}

void ui_destroy(UiState *ui)
{
    if (ui == NULL)
    {
        return;
    }

    if (ui->label_font != NULL)
    {
        DeleteObject(ui->label_font);
        ui->label_font = NULL;
    }
    if (ui->small_font != NULL)
    {
        DeleteObject(ui->small_font);
        ui->small_font = NULL;
    }

    ui_release_backbuffer(ui);
}

#ifndef UI_H
#define UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>

#include "sim.h"

typedef struct
{
    HDC back_dc;
    HBITMAP back_bitmap;
    HGDIOBJ back_dc_old;
    HFONT label_font;
    HFONT small_font;
    int width;
    int height;
} UiState;

void ui_init(UiState *ui, HWND hwnd);
void ui_resize(UiState *ui, HWND hwnd, int width, int height);
void ui_render(UiState *ui, HDC target_dc, const SimState *sim);
void ui_destroy(UiState *ui);

#ifdef __cplusplus
}
#endif

#endif /* UI_H */

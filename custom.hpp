#pragma once

#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"

inline int tab = 0;
inline int subtab = 0;
inline float content_anim = 0.f;

inline ImFont* icons;
inline ImFont* icons2;
inline ImFont* font_menu;
inline ImFont* tahoma;

struct slider_float {
    const char* label;
    float min, max;
};

struct slider_int {
    const char* label;
    int min, max;
};

struct combo {
    const char* label;
    const char* items_separated_by_zeros;
};

struct color_edit {
    float col[4];
};



namespace custom {
    void render_text(const char* text, ImVec2 position, ImColor color, bool center, bool outline, bool sub_heigt = false, float sub_factor = 0.0f);
}

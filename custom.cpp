#include <custom.hpp>

#include <string>

using namespace ImGui;

void custom::render_text(const char* text, ImVec2 position, ImColor color, bool center, bool outline, bool sub_heigt, float sub_factor) {
    auto text_size = ImGui::CalcTextSize(text);

    if (center) position.x -= (text_size.x / 2);
    if (sub_heigt) position.y -= (text_size.y * sub_factor);

    if (outline) {
        ImGui::GetBackgroundDrawList()->AddText(tahoma, 12.f, ImVec2(position.x + 1, position.y + 1), ImColor(0, 0, 0, 255), text);
        ImGui::GetBackgroundDrawList()->AddText(tahoma, 12.f, ImVec2(position.x - 1, position.y - 1), ImColor(0, 0, 0, 255), text);
        ImGui::GetBackgroundDrawList()->AddText(tahoma, 12.f, ImVec2(position.x + 1, position.y - 1), ImColor(0, 0, 0, 255), text);
        ImGui::GetBackgroundDrawList()->AddText(tahoma, 12.f, ImVec2(position.x - 1, position.y + 1), ImColor(0, 0, 0, 255), text);
    }
    ImGui::GetBackgroundDrawList()->AddText(tahoma, 12.f, { position.x, position.y }, color, text);

}
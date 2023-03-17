#include <esp.hpp>
#include <imgui.h>
#include <ue4.h>
#include <custom.hpp>
#include <string>

using namespace uknowncheats;

void esp::render() {
    try {
        auto world = *(uintptr_t*)(module_base_address + 0x97ded08);
        if (!world) return;

        auto owning_game_instance = *(uintptr_t*)((std::uintptr_t)world + 0x190);
        if (!owning_game_instance) return;

        auto local_player = *(uintptr_t*)(*(uintptr_t*)(owning_game_instance + 0x38));
        if (!local_player) return;

        APlayerController* local_player_controller = *(APlayerController**)(local_player + 0x30);
        if (!local_player_controller) return;

        auto acknowledged_pawn = *(uintptr_t*)((uintptr_t)local_player_controller + 0x318);
        if (!acknowledged_pawn) return;

        PlayerCameraManager* player_camera = local_player_controller->get_camera_manager();
        if (!player_camera) return;

        auto local_root_component = *(uintptr_t*)(acknowledged_pawn + 0x190);
        if (!local_root_component) return;

        FVector local_position = *(FVector*)(local_root_component + 0x158);

        auto persistent_level = *(uintptr_t*)(world + 0x30);
        if (!persistent_level) return;

        auto actors = *(uintptr_t*)(persistent_level + 0x98);
        if (!actors) return;

        auto actor_count = *(int32_t*)(persistent_level + 0xa0);
        if (actor_count <= 0) return;

        for (std::int32_t i = 0; i < actor_count; i++)
        {
            auto current_actor = *(uintptr_t*)(actors + i * sizeof(std::uintptr_t));
            if (!current_actor) continue;

            if ((uintptr_t)current_actor == (uintptr_t)acknowledged_pawn) continue;

            auto current_actor_name = GetObjectName((UObject*)current_actor);
            if (!wcsstr(current_actor_name.c_str(), L"BP_DiscoveryCharacter_C")) continue;

            auto mesh = *(uintptr_t*)(current_actor + 0x2f8);
            if (!mesh) continue;

            auto root_bone = GetBoneMatrix(mesh, 0);
            if (root_bone.x == 0 || root_bone.y == 0) continue;

            auto distance = (int)(root_bone.distance(local_position) / 100);
            if (distance <= 2) continue;

            auto wts_position_feet = local_player_controller->w2s(root_bone);
            if (wts_position_feet.x <= 0 || wts_position_feet.y <= 0) continue;

            root_bone = GetBoneMatrix(mesh, 15);
            if (root_bone.x == 0 || root_bone.y == 0) continue;

            auto wts_position_head = local_player_controller->w2s(root_bone);
            if (wts_position_head.x <= 0 || wts_position_head.y <= 0) continue;

            auto box_height = wts_position_head.y - wts_position_feet.y;
            auto box_width = box_height / 2;

            auto box_left = wts_position_head.x - (box_width / 2);
            auto box_right = wts_position_head.x + (box_width / 2);

            auto screen_size = ImGui::GetIO().DisplaySize;

            auto color = ImColor(0, 255, 0, 255);

            //ImGui::GetBackgroundDrawList()->AddLine({ screen_size.x / 2,screen_size.y }, { (float)wts_position_feet.x, (float)wts_position_feet.y }, color, 1.f);
            ImGui::GetBackgroundDrawList()->AddRect({ (float)box_left, (float)wts_position_feet.y }, { (float)box_right, (float)wts_position_head.y }, color);
            custom::render_text(std::to_string((int)distance).append(" m").c_str(), {floor((float)wts_position_feet.x), floor((float)wts_position_feet.y + 5)}, color, true, false);


        }
    }
    catch (...) {
        //Just so that the game doesnt always crash while debugging.
        return;
    }
    
}
#include <aimbot.hpp>
#include <ue4.h>
#include <imgui.h>

using namespace uknowncheats;

void aimbot::tick() {
	try {
        auto world = *(uintptr_t*)(module_base_address + 0x8028a88);
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

        uintptr_t best_actor = 0;
        double best_distance = DBL_MAX;

        FVector2D crosshair_position = { ImGui::GetIO().DisplaySize.x / 2, ImGui::GetIO().DisplaySize.y / 2 };

        for (int i = 0; i < actor_count; i++) {
            auto current_actor = *(uintptr_t*)(actors + i * sizeof(std::uintptr_t));
            if (!current_actor) continue;

            auto current_actor_name = GetObjectName((UObject*)current_actor);
            if (!wcsstr(current_actor_name.c_str(), L"BP_DiscoveryCharacter_C")) continue;

            auto mesh = *(uintptr_t*)(current_actor + 0x2f8);
            if (!mesh) continue;

            auto root_bone = GetBoneMatrix(mesh, 15);
            if (root_bone.x == 0 || root_bone.y == 0) continue;

            auto wts_position = local_player_controller->w2s(root_bone);
            if (wts_position.x <= 0 || wts_position.y <= 0) continue;

            auto dx = wts_position.x - (crosshair_position.x);
            auto dy = wts_position.y - (crosshair_position.y);
            auto distance = sqrtf(dx * dx + dy * dy);

            if (distance <= 360 && distance < best_distance) {
                best_distance = distance;
                best_actor = current_actor;
            }
        }


	}
	catch (...) {
        //Just so that the game doesnt always crash while debugging.
		return;
	}
}
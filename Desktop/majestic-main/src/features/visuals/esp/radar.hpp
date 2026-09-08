	void radar() {
		ImGui::SetNextWindowSizeConstraints(ImVec2(200, 200), ImVec2(FLT_MAX, FLT_MAX));
		ImGui::Begin("Radarbox", 0, ImGuiWindowFlags_NoTitleBar);
		ImGuiContext& g = *GImGui;
		g.IO.ConfigWindowsResizeFromEdges = false;
		const ImVec2& pos = ImGui::GetWindowPos();
		const ImVec2& region = ImGui::GetContentRegionMax();
	
		ImGui::GetBackgroundDrawList()->AddRectFilled(pos, pos + ImVec2(region), ImGui::GetColorU32(ImVec4(0.07f, 0.07f, 0.09f, 0.7f)), 0.f);

		ImVec2 vMin = ImGui::GetWindowContentRegionMin();
		ImVec2 vMax = ImGui::GetWindowContentRegionMax();
		ImVec2 vSizes = ImGui::GetWindowSize();
		vMin.x += ImGui::GetWindowPos().x;
		vMin.y += ImGui::GetWindowPos().y;
		vMax.x += ImGui::GetWindowPos().x;
		vMax.y += ImGui::GetWindowPos().y;
		ImVec2 Center = ImVec2(vMin.x + (vSizes.x / 2), vMin.y + (vSizes.y / 2));

		renderer.RenderLine(ImVec2(Center.x, vMin.y), ImVec2(Center.x, vMax.y), RGBA(255, 255, 255, 66), 1.0f);
		renderer.RenderLine(ImVec2(vMin.x, Center.y), ImVec2(vMax.x, Center.y), RGBA(255, 255, 255, 66), 1.0f);

		RGBA visible_color = RGBA(
			config::get("visual", "visible_r", 0.f) * 255,
			config::get("visual", "visible_g", 1.f) * 255,
			config::get("visual", "visible_b", 0.f) * 255,
			config::get("visual", "visible_a", 1.f) * 255
		);
		RGBA invisible_color = RGBA(
			config::get("visual", "invisible_r", 1.f) * 255,
			config::get("visual", "invisible_g", 0.f) * 255,
			config::get("visual", "invisible_b", 0.f) * 255,
			config::get("visual", "invisible_a", 1.f) * 255
		);

		{
			std::lock_guard<std::mutex> lock(game::ped_list_mutex);
			for(pair<CObject*, DataPed>& entity : game::ped_list) {
				CObject* ped = entity.first;
				DataPed data = entity.second;
				if(local.player == ped) continue;
				if(get_distance(local.player->fPosition, ped->fPosition) > config::get("hack", "max_range", 1000.f)) continue;
				CModelInfo* _mi4 = ped->ModelInfo();
				if (!IsValidPtr(_mi4)) continue;
				DWORD hash = _mi4->GetHash();
				if(!game::isValidPlayer(hash, ped)) continue;

				RGBA current_color = RGBA(255, 255, 255, 255);

				if(data.visible) {
					current_color = visible_color;
				} else {
					current_color = invisible_color;
				}
				if(data.group.name != "\0") {
					current_color = data.group.color;
				}
				ImVec2 RadarPos = utils::WorldToRadar(ped->fPosition, Center.x, Center.y, vSizes.x, config::get("visual", "radar_zoom", 1.f));
				renderer.RenderCircleFilled(RadarPos, 5.f, current_color, 16.f);
			}
		}

		ImGui::End();
	}

	map<string, Vector3>waypoint_cache;
	void draw_waypoints() {
		if(!IsValidPtr(local.player)) return;

		map<string, Vector3> waypoint_list = waypoint_cache;

		RGBA color = RGBA(
			config::get("misc", "waypoint_color_r", 1.f) * 255,
			config::get("misc", "waypoint_color_g", 1.f) * 255,
			config::get("misc", "waypoint_color_b", 1.f) * 255,
			config::get("misc", "waypoint_color_a", 1.f) * 255
		);

		map<string, Vector3>::iterator it;
		for(it = waypoint_list.begin(); it != waypoint_list.end(); it++) {

			string name = it->first;
			Vector3 pos = it->second;

			ImVec2 waypoint2d;
			if(!WorldToScreen(pos, &waypoint2d)) continue;
			if(!isW2SValid(waypoint2d)) continue;

			float dist = get_distance(local.player->fPosition, pos);
			renderer.RenderText(name, waypoint2d, 13, color, true, true);

			renderer.RenderText((std::to_string((int)floor(dist + 0.5)) + "m").c_str(), ImVec2(waypoint2d.x, waypoint2d.y + 15), 13, color, true, true);
		}

	}



// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/features/visuals/esp/radar.hpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_0e8daf115c4473e16be423824df0d465
#define NOCTUA_LICENSE_MARK_0e8daf115c4473e16be423824df0d465
namespace noctua_license { namespace mark_0e8daf115c4473e16be423824df0d465 {
    inline constexpr unsigned long long kMarkId = 0xaf460dd44ba9c234ull;
    inline constexpr char kMarkFile[] = "src/features/visuals/esp/radar.hpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0x9b, 0x36, 0xf4, 0xcf, 0x21, 0xaf, 0x36, 0x78, 0x74, 0x84, 0x2f, 0xf5, 0xb4, 0xa1, 0x13, 0x70 };
    inline constexpr unsigned long  kMarkSalts[]  = { 0x2ec72814ul, 0x43f5b5d0ul, 0x0019d55dul, 0x9cfbb49ful, 0x42027555ul, 0xc354a59cul, 0x41b3ef17ul, 0x52063393ul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_0e8daf115c4473e16be423824df0d465
#endif // NOCTUA_LICENSE_MARK_0e8daf115c4473e16be423824df0d465

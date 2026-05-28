#include "SandboxGbaRoom.h"

#include "Hazel/Tilemap/GbaRoomLoader.h"
#include "Hazel/Scene/Scene.h"
#include "Hazel/Scene/Components.h"
#include "Hazel/Scene/SceneRenderContext.h"

#include <imgui/imgui.h>
#include <fstream>
#include <sstream>

// ============================================================
// .area 配置解析 (简易 YAML-like)
// ============================================================

static std::string ExtractQuoted(const std::string& line)
{
	auto start = line.find('"');
	auto end = line.find('"', start + 1);
	if (start != std::string::npos && end != std::string::npos)
		return line.substr(start + 1, end - start - 1);
	return {};
}

static int ExtractInt(const std::string& line)
{
	auto pos = line.find(':');
	if (pos != std::string::npos)
		return std::stoi(line.substr(pos + 1));
	return 0;
}

// ============================================================
// SandboxGbaRoom
// ============================================================

SandboxGbaRoom::SandboxGbaRoom()
	: Layer("SandboxGbaRoom")
	, m_CameraController(1280.0f / 720.0f)
{
}

void SandboxGbaRoom::LoadAreaFromConfig(const std::string& areaConfigPath, const std::string& basePath)
{
	std::ifstream file(areaConfigPath);
	if (!file.is_open())
	{
		HZ_CORE_ERROR("SandboxGbaRoom: Cannot open area config: {0}", areaConfigPath);
		return;
	}

	std::string areaDir = areaConfigPath.substr(0, areaConfigPath.rfind('/') + 1);

	std::string line;
	std::string areaName;

	Hazel::RoomDef currentDef;
	enum class State { None, InRooms, InRoom, InBottom, InTop };
	State state = State::None;

	int areaId = (int)Hazel::AreaId::COUNT; // will match by areaDir name

	while (std::getline(file, line))
	{
		// Trim leading whitespace
		size_t indent = line.find_first_not_of(" \t\r");
		if (indent == std::string::npos) continue;
		std::string content = line.substr(indent);

		if (content.find("Name:") != std::string::npos && state == State::None)
		{
			areaName = ExtractQuoted(content);
			// Match area name to AreaId (simplified: use directory index)
			if (areaName.find("MinishWoods") != std::string::npos)
				areaId = (int)Hazel::AreaId::MinishWoods;
			else if (areaName.find("HyruleField") != std::string::npos)
				areaId = (int)Hazel::AreaId::HyruleField;
		}
		else if (content.find("Rooms:") != std::string::npos)
		{
			state = State::InRooms;
		}
		else if (content.find("- Name:") != std::string::npos && state == State::InRooms)
		{
			currentDef = {};
			currentDef.Header.TileSetId = 0;
			m_Rooms.push_back({ (Hazel::AreaId)areaId, (uint8_t)(m_Rooms.size()), ExtractQuoted(content) });
			state = State::InRoom;
		}
		else if (content.find("Width:") != std::string::npos && state == State::InRoom)
		{
			currentDef.Header.PixelWidth = (uint16_t)(ExtractInt(content) * 16);
		}
		else if (content.find("Height:") != std::string::npos && state == State::InRoom)
		{
			currentDef.Header.PixelHeight = (uint16_t)(ExtractInt(content) * 16);
		}
		else if (content.find("Bottom:") != std::string::npos && state == State::InRoom)
		{
			state = State::InBottom;
		}
		else if (content.find("Top:") != std::string::npos && state == State::InRoom)
		{
			state = State::InTop;
		}
		else if (content.find("MapData:") != std::string::npos)
		{
			std::string relPath = ExtractQuoted(content);
			std::string fullPath = areaDir + relPath;
			if (state == State::InBottom)
				currentDef.BottomMapPath = fullPath;
			else if (state == State::InTop)
				currentDef.TopMapPath = fullPath;
		}
		else if (content.find("SubTiles:") != std::string::npos)
		{
			std::string subPath = areaDir + ExtractQuoted(content);
				if (state == State::InBottom)
					currentDef.BottomSubTilePath = subPath;
				else if (state == State::InTop)
					currentDef.TopSubTilePath = subPath;
		}
		else if (content.find("TileTypes:") != std::string::npos)
		{
			std::string typePath = areaDir + ExtractQuoted(content);
				if (state == State::InBottom)
					currentDef.BottomTileTypePath = typePath;
				else if (state == State::InTop)
					currentDef.TopTileTypePath = typePath;
		}
		else if (content.find("TileSet:") != std::string::npos)
		{
			currentDef.TileSetPath = areaDir + ExtractQuoted(content);
		}
		else if (content.find("Palette:") != std::string::npos)
		{
			currentDef.PalettePath = areaDir + ExtractQuoted(content);
		}
		else if (indent <= 2 && state != State::None)
		{
			// Back to higher level — register completed room
			if (state == State::InBottom || state == State::InTop)
			{
				currentDef.Index = (uint8_t)(m_Rooms.size() - 1);
				m_RoomManager.RegisterRoom((Hazel::AreaId)areaId, currentDef);
			}
			state = State::InRooms;
		}
	}

	// Register final room
	if (state == State::InBottom || state == State::InTop)
	{
		currentDef.Index = (uint8_t)(m_Rooms.size() - 1);
		m_RoomManager.RegisterRoom((Hazel::AreaId)areaId, currentDef);
	}
}

void SandboxGbaRoom::OnAttach()
{
	HZ_PROFILE_FUNCTION();

	// ---------- 加载 Area 配置 ----------
	const std::string basePath = "D:/dev/haze/Hazel/Hazelnut/Resources/Areas";
	LoadAreaFromConfig(basePath + "/000_MinishWoods/000_MinishWoods.area", basePath);

	m_RoomManager.BuildIndex();

	// 补充 tileset/palette 路径（按命名约定，从 tileset_id 推导）
	const std::string areaDir = basePath + "/000_MinishWoods";
	for (auto& entry : m_Rooms)
	{
		auto* def = const_cast<Hazel::RoomDef*>(
			m_RoomManager.GetRoomDef(entry.Area, entry.RoomIdx));
		if (!def) continue;

		uint16_t tsId = def->Header.TileSetId;
		def->TileSetPath = areaDir + "/tileSets/" + std::to_string(tsId)
			+ "/gAreaTileSet_MinishWoods_" + std::to_string(tsId) + "_0.htileset";
		def->PalettePath = areaDir + "/palette.hpal";
	}

	HZ_CORE_INFO("SandboxGbaRoom: {0} room(s) loaded from config", m_Rooms.size());

	// ---------- 创建 Scene ----------
	m_Scene = Hazel::CreateRef<Hazel::Scene>();

	// 相机实体 — CameraSystem 找 Primary=true
	{
		auto cam = m_Scene->CreateEntity("MainCamera");
		auto& cc = cam.AddComponent<Hazel::CameraComponent>();
		cc.Primary = true;
		cc.FixedAspectRatio = false;
		cc.Camera.SetViewportSize(1280, 720);
		auto& camTrans = cam.GetComponent<Hazel::TransformComponent>();
		camTrans.Translation = { 0, 0, 10 };
	}

	// ---------- 加载默认房间 ----------
	if (!m_Rooms.empty())
		SwitchToRoom(0);
}

void SandboxGbaRoom::SwitchToRoom(int index)
{
	if (index < 0 || index >= (int)m_Rooms.size()) return;

	auto& entry = m_Rooms[index];

	// 如果已有房间实体, 先清理
	if (m_RoomEntity)
	{
		m_Scene->DestroyEntity(m_RoomEntity);
		m_RoomEntity = {};
	}

	// 通过 RoomManager::InitRoom 创建 Entity
	m_RoomEntity = m_RoomManager.InitRoom(m_Scene.get(), entry.Area, entry.RoomIdx);

	m_CurrentRoomIndex = index;

	HZ_CORE_INFO("SandboxGbaRoom: Switched to room [{0}] {1}",
		index, entry.Name);
}

void SandboxGbaRoom::OnDetach()
{
	HZ_PROFILE_FUNCTION();
	m_Scene.reset();
}

void SandboxGbaRoom::OnUpdate(Hazel::Timestep ts)
{
	HZ_PROFILE_FUNCTION();

	// ---------- 渲染 ----------
	Hazel::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1 });
	Hazel::RenderCommand::Clear();

	if (m_Scene)
	{
		m_Scene->OnViewportResize(1280, 720);

		// 暂停模式下只跑 CameraSystem + RenderSystem2D，跳过 Physics/脚本
		m_Scene->SetPaused(true);
		m_Scene->OnUpdateRuntime(ts);
	}

	// ---------- 上层 UI overlay (调试) ----------
	{
		HZ_PROFILE_SCOPE("Overlay");
		Hazel::Renderer2D::BeginScene(m_CameraController.GetCamera());
		Hazel::Renderer2D::EndScene();
	}
}

void SandboxGbaRoom::OnImGuiRender()
{
	HZ_PROFILE_FUNCTION();

	ImGui::Begin("GBA Room Demo");

	auto stats = Hazel::Renderer2D::GetStats();
	ImGui::Text("Renderer2D Stats:");
	ImGui::Text("  Draw Calls: %d", stats.DrawCalls);
	ImGui::Text("  Quads: %d", stats.QuadCount);
	ImGui::Separator();

	ImGui::Text("Room Manager:");
	auto& current = m_RoomManager.GetCurrentRoom();
	ImGui::Text("  Current: Area=%d Room=%d", (int)current.CurrentArea, current.CurrentRoom);
	ImGui::Separator();

	// Room list + switch
	if (ImGui::BeginListBox("Rooms"))
	{
		for (int i = 0; i < (int)m_Rooms.size(); i++)
		{
			bool selected = (i == m_CurrentRoomIndex);
			if (ImGui::Selectable(m_Rooms[i].Name.c_str(), selected))
				SwitchToRoom(i);
			if (selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndListBox();
	}

	ImGui::End();
}

void SandboxGbaRoom::OnEvent(Hazel::Event& e)
{
	m_CameraController.OnEvent(e);
}

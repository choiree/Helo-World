#pragma once

#include "Hazel.h"
#include "Hazel/Tilemap/RoomManager.h"

class SandboxGbaRoom : public Hazel::Layer
{
public:
	SandboxGbaRoom();
	virtual ~SandboxGbaRoom() = default;

	void OnAttach() override;
	void OnDetach() override;
	void OnUpdate(Hazel::Timestep ts) override;
	void OnImGuiRender() override;
	void OnEvent(Hazel::Event& e) override;

private:
	void LoadAreaFromConfig(const std::string& areaConfigPath, const std::string& basePath);

	Hazel::OrthographicCameraController m_CameraController;
	Hazel::RoomManager m_RoomManager;
	Hazel::Ref<Hazel::Scene> m_Scene;

	Hazel::Entity m_RoomEntity;

	struct RoomEntry
	{
		Hazel::AreaId Area;
		uint8_t RoomIdx;
		std::string Name;
	};
	std::vector<RoomEntry> m_Rooms;
	int m_CurrentRoomIndex = 0;

	void SwitchToRoom(int index);
};

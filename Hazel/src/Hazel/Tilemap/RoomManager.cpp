#include "hzpch.h"
#include "RoomManager.h"

#include "Hazel/Scene/Scene.h"
#include "Hazel/Scene/Components.h"
#include "Hazel/Scene/Serializers/TileMapSerializer.h"
#include "Hazel/Tilemap/GbaRoomLoader.h"

#include <algorithm>

namespace Hazel {

	// ============================================================
	// 配置加载
	// ============================================================

	void RoomManager::RegisterRoom(AreaId area, RoomDef def)
	{
		def.Index = 0;

		// 查找或创建对应的 AreaDef
		auto it = m_AreaIndex.find(area);
		if (it != m_AreaIndex.end())
		{
			def.Index = (uint8_t)m_Areas[it->second].Rooms.size();
			m_Areas[it->second].Rooms.push_back(std::move(def));
		}
		else
		{
			AreaDef ad;
			ad.Id = area;
			def.Index = 0;
			ad.Rooms.push_back(std::move(def));
			m_Areas.push_back(std::move(ad));
		}
	}

	void RoomManager::BuildIndex()
	{
		m_AreaIndex.clear();
		for (size_t i = 0; i < m_Areas.size(); i++)
			m_AreaIndex[m_Areas[i].Id] = i;
	}

	const AreaDef* RoomManager::GetAreaDef(AreaId id) const
	{
		auto it = m_AreaIndex.find(id);
		if (it != m_AreaIndex.end())
			return &m_Areas[it->second];
		return nullptr;
	}

	const RoomDef* RoomManager::GetRoomDef(AreaId area, uint8_t roomIdx) const
	{
		const AreaDef* ad = GetAreaDef(area);
		if (ad && roomIdx < ad->Rooms.size())
			return &ad->Rooms[roomIdx];
		return nullptr;
	}

	uint8_t RoomManager::GetRoomCount(AreaId area) const
	{
		const AreaDef* ad = GetAreaDef(area);
		return ad ? (uint8_t)ad->Rooms.size() : 0;
	}

	// ============================================================
	// 资源缓存
	// ============================================================

	Ref<TileSetAsset> RoomManager::GetOrLoadTileset(uint16_t tilesetId, const std::string& path, uint32_t subtileCount)
	{
		auto it = m_TilesetCache.find(tilesetId);
		if (it != m_TilesetCache.end())
			return it->second;

		Ref<TileSetAsset> asset;
		if (path.ends_with(".htileset"))
			asset = TileMapSerializer::LoadTileset(path);
		else
			asset = GbaRoomLoader::LoadTileset(path, subtileCount);

		if (asset)
			m_TilesetCache[tilesetId] = asset;
		return asset;
	}

	Ref<PaletteAsset> RoomManager::GetOrLoadPalette(uint16_t tilesetId, const std::string& path)
	{
		auto it = m_PaletteCache.find(tilesetId);
		if (it != m_PaletteCache.end())
			return it->second;

		auto asset = GbaRoomLoader::LoadPalette(path);
		if (asset)
			m_PaletteCache[tilesetId] = asset;
		return asset;
	}

	// ============================================================
	// InitRoom — 核心流程
	// ============================================================

	Entity RoomManager::InitRoom(Scene* scene, AreaId area, uint8_t roomIdx)
	{
		const RoomDef* def = GetRoomDef(area, roomIdx);
		if (!def)
		{
			HZ_CORE_ERROR("RoomManager::InitRoom: Room not found area={0} room={1}",
				(int)area, roomIdx);
			return {};
		}

		HZ_CORE_INFO("RoomManager: Loading area={0} room={1} ({2}x{3} tiles)",
			(int)area, roomIdx, def->Header.TilesWide(), def->Header.TilesHigh());

		// Step 1: 加载 tileset + palette (带缓存)
		uint16_t tsId = def->Header.TileSetId;
		uint32_t subtileCount = 1024; // TODO: 从 tileset 文件实际大小推断
		Ref<TileSetAsset> tileset  = GetOrLoadTileset(tsId, def->TileSetPath, subtileCount);
		Ref<PaletteAsset> palette  = GetOrLoadPalette(tsId, def->PalettePath);

		// Step 2: 加载房间 TileMapAsset
		Ref<TileMapAsset> tileMap = GbaRoomLoader::LoadRoom(*def);
		if (!tileMap)
		{
			HZ_CORE_ERROR("RoomManager::InitRoom: Failed to load room data");
			return {};
		}

		// Step 3: 创建 Entity + TileMapComponent
		Entity entity = scene->CreateEntity("Room");
		auto& tmc = entity.AddComponent<TileMapComponent>();
		tmc.Map                 = tileMap;
		tmc.Visible             = true;
		tmc.CurrentBottomTileSet = tileset;
		tmc.CurrentTopTileSet    = tileset;
		tmc.CurrentPalette       = palette;

		// Step 4: 记录状态
		m_CurrentRoom.CurrentArea = area;
		m_CurrentRoom.CurrentRoom = roomIdx;
		m_CurrentTileMap = tileMap;

		return entity;
	}

} // namespace Hazel

#pragma once

#include "Hazel/Tilemap/RoomDef.h"
#include "Hazel/Tilemap/GbaRoomLoader.h"
#include "Hazel/Scene/Entity.h"

#include <unordered_map>
#include <memory>

namespace Hazel {

	class Scene;

	// ============================================================
	// RoomManager — 房间生命周期管理
	// ============================================================
	//
	// 职责:
	//   1. 管理 AreaDef 配置数据 (从 room_headers 等数据源构建)
	//   2. 维护当前房间 / 默认房间状态
	//   3. InitRoom() 协调 GbaRoomLoader 加载 + Entity 创建
	//   4. 缓存 tileset/palette (按 tileset_id)，跨房间共享

	class RoomManager
	{
	public:
		RoomManager() = default;

		// ---------- 配置加载 ----------

		// 手动注册一个 RoomDef。实际使用时可以循环调用此方法从 CSV/二进制构建
		void RegisterRoom(AreaId area, RoomDef def);

		// 所有 Area 注册完毕后调用，构建内部索引
		void BuildIndex();

		const AreaDef* GetAreaDef(AreaId id) const;
		const RoomDef* GetRoomDef(AreaId area, uint8_t roomIdx) const;
		uint8_t GetRoomCount(AreaId area) const;

		// ---------- 默认房间 ----------

		void SetDefaultRoom(AreaId area, uint8_t roomIdx)
		{
			m_DefaultArea = area;
			m_DefaultRoom = roomIdx;
		}

		AreaId  GetDefaultArea() const { return m_DefaultArea; }
		uint8_t GetDefaultRoomIdx() const { return m_DefaultRoom; }

		// ---------- 资源缓存 ----------

		// 按 tileset_id 缓存，多次调用同一 id 返回已加载的 asset
		// 自动检测格式: .htileset → TileSetSerializer, .4bpp → GbaRoomLoader
		Ref<TileSetAsset> GetOrLoadTileset(uint16_t tilesetId, const std::string& path, uint32_t subtileCount);

		// palette: 文件不存在时自动返回默认灰度 palette
		Ref<PaletteAsset> GetOrLoadPalette(uint16_t tilesetId, const std::string& path);

		// ---------- 运行时 ----------

		// 初始化房间: 加载 TileMapAsset + 创建 Entity + 挂 TileMapComponent
		// 返回创建的 Entity (外部可进一步配置相机等)
		Entity InitRoom(Scene* scene, AreaId area, uint8_t roomIdx);

		// 当前房间信息
		const RuntimeRoomInfo& GetCurrentRoom() const { return m_CurrentRoom; }

		// 地图地形数据 (只读访问)
		Ref<TileMapAsset> GetCurrentTileMap() const { return m_CurrentTileMap; }

	private:
		// Area 配置
		std::vector<AreaDef> m_Areas;

		// 快速查找: AreaId → Areas 索引
		std::unordered_map<AreaId, size_t> m_AreaIndex;

		// 资源缓存
		std::unordered_map<uint16_t, Ref<TileSetAsset>> m_TilesetCache;
		std::unordered_map<uint16_t, Ref<PaletteAsset>> m_PaletteCache;

		// 运行时状态
		RuntimeRoomInfo   m_CurrentRoom;
		Ref<TileMapAsset> m_CurrentTileMap;

		// 默认房间
		AreaId  m_DefaultArea = AreaId::MinishWoods;
		uint8_t m_DefaultRoom = 0;
	};

} // namespace Hazel

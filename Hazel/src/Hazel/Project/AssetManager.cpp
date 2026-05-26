#include "hzpch.h"
#include "AssetManager.h"

#include "Hazel/Project/Project.h"
#include "Hazel/Renderer/SpriteSheet.h"
#include "Hazel/Renderer/AnimationClip.h"
#include "Hazel/Scene/Serializers/SpriteSheetSerializer.h"
#include "Hazel/Scene/Serializers/AnimationClipSerializer.h"
#include "Hazel/Scene/Serializers/TileMapSerializer.h"
#include "Hazel/Renderer/TileSetAsset.h"
#include "Hazel/Renderer/PaletteAsset.h"
#include "Hazel/Renderer/TileMapAsset.h"

namespace Hazel {

	void AssetManager::Init()
	{
		if (s_Initialized)
			return;
		s_Initialized = true;
		s_Registry.clear();
		s_SceneStack.clear();
	}

	void AssetManager::Shutdown()
	{
		UnloadAll();
		s_Initialized = false;
	}

	bool AssetManager::IsLoaded(const std::filesystem::path& path)
	{
		return s_Registry.find(MakeKey(path)) != s_Registry.end();
	}

	void AssetManager::Unload(const std::filesystem::path& path)
	{
		std::string key = MakeKey(path);
		auto it = s_Registry.find(key);
		if (it != s_Registry.end())
		{
			it->second.Handle.reset();
			s_Registry.erase(it);
		}
	}

	void AssetManager::UnloadAll()
	{
		for (auto& [key, entry] : s_Registry)
			entry.Handle.reset();
		s_Registry.clear();
		s_SceneStack.clear();
	}

	void AssetManager::TrackInScene(const std::string& key)
	{
		if (!s_SceneStack.empty())
			s_SceneStack.back().Keys.push_back(key);
	}

	void AssetManager::BeginScene()
	{
		s_SceneStack.push_back({});
	}

	void AssetManager::EndScene()
	{
		// Pop the current (back) scene and unload all assets tracked within it.
		// Used on the Deserialize error path — discards everything that was
		// loaded for a scene that failed to construct.
		if (s_SceneStack.empty())
			return;

		auto& scene = s_SceneStack.back();
		for (auto& key : scene.Keys)
		{
			auto it = s_Registry.find(key);
			if (it == s_Registry.end())
				continue;

			it->second.RefCount--;
			if (it->second.RefCount <= 0)
			{
				it->second.Handle.reset();
				s_Registry.erase(it);
			}
		}
		s_SceneStack.pop_back();
	}

	void AssetManager::CloseScene()
	{
		if (s_SceneStack.empty())
			return;

		auto& closingScene = s_SceneStack.front();
		std::vector<std::string> remainingKeys;

		// Collect keys from remaining scenes
		for (size_t i = 1; i < s_SceneStack.size(); i++)
		{
			for (auto& key : s_SceneStack[i].Keys)
				remainingKeys.push_back(key);
		}

		// Unload assets exclusive to the closing scene
		for (auto& key : closingScene.Keys)
		{
			auto it = s_Registry.find(key);
			if (it == s_Registry.end())
				continue;

			bool inRemaining = false;
			for (auto& rk : remainingKeys)
			{
				if (rk == key) { inRemaining = true; break; }
			}
			if (!inRemaining)
			{
				it->second.RefCount--;
				if (it->second.RefCount <= 0)
				{
					it->second.Handle.reset();
					s_Registry.erase(it);
				}
			}
		}

		s_SceneStack.erase(s_SceneStack.begin());
	}

	std::filesystem::path AssetManager::ResolvePath(const std::filesystem::path& path)
	{
		if (path.is_absolute())
			return path;

		// If a project is active, resolve relative to the asset directory first.
		// Fall back to CWD if that file doesn't exist (editor resources, etc.).
		if (Project::GetActive())
		{
			auto assetPath = Project::GetAssetFileSystemPath(path);
			if (std::filesystem::exists(assetPath))
				return assetPath;
		}

		// No active project, or asset-relative path doesn't exist — resolve
		// relative to current working directory.
		return std::filesystem::absolute(path);
	}

	std::string AssetManager::MakeKey(const std::filesystem::path& path)
	{
		// Normalize to generic string for consistent lookup
		return path.generic_string();
	}

	// ---- Load<SpriteSheet> ----

	template<>
	Ref<SpriteSheet> AssetManager::Load<SpriteSheet>(const std::filesystem::path& path)
	{
		std::string key = MakeKey(path);

		auto it = s_Registry.find(key);
		if (it != s_Registry.end())
		{
			it->second.RefCount++;
			return std::static_pointer_cast<SpriteSheet>(it->second.Handle);
		}

		auto resolvedPath = ResolvePath(path);
		auto sheet = SpriteSheetSerializer::Deserialize(resolvedPath.string());

		if (sheet)
		{
			AssetEntry entry;
			entry.Type = AssetType::SpriteSheet;
			entry.Handle = sheet;
			entry.RefCount = 1;
			s_Registry[key] = entry;
			TrackInScene(key);
		}

		return sheet;
	}

	// ---- Load<AnimationClip> ----

	template<>
	Ref<AnimationClip> AssetManager::Load<AnimationClip>(const std::filesystem::path& path)
	{
		std::string key = MakeKey(path);

		auto it = s_Registry.find(key);
		if (it != s_Registry.end())
		{
			it->second.RefCount++;
			return std::static_pointer_cast<AnimationClip>(it->second.Handle);
		}

		auto resolvedPath = ResolvePath(path);
		auto clip = AnimationClipSerializer::Deserialize(resolvedPath.string());

		if (clip)
		{
			AssetEntry entry;
			entry.Type = AssetType::AnimationClip;
			entry.Handle = clip;
			entry.RefCount = 1;
			s_Registry[key] = entry;
			TrackInScene(key);
		}

		return clip;
	}

	// ---- Load<TileSetAsset> ----

	template<>
	Ref<TileSetAsset> AssetManager::Load<TileSetAsset>(const std::filesystem::path& path)
	{
		std::string key = MakeKey(path);

		auto it = s_Registry.find(key);
		if (it != s_Registry.end())
		{
			it->second.RefCount++;
			return std::static_pointer_cast<TileSetAsset>(it->second.Handle);
		}

		auto resolvedPath = ResolvePath(path);
		auto asset = TileMapSerializer::LoadTileset(resolvedPath.string());

		if (asset)
		{
			AssetEntry entry;
			entry.Type = AssetType::Tileset;
			entry.Handle = asset;
			entry.RefCount = 1;
			s_Registry[key] = entry;
			TrackInScene(key);
		}

		return asset;
	}

	// ---- Load<PaletteAsset> ----

	template<>
	Ref<PaletteAsset> AssetManager::Load<PaletteAsset>(const std::filesystem::path& path)
	{
		std::string key = MakeKey(path);

		auto it = s_Registry.find(key);
		if (it != s_Registry.end())
		{
			it->second.RefCount++;
			return std::static_pointer_cast<PaletteAsset>(it->second.Handle);
		}

		auto resolvedPath = ResolvePath(path);
		auto asset = TileMapSerializer::LoadPalette(resolvedPath.string());

		if (asset)
		{
			AssetEntry entry;
			entry.Type = AssetType::Palette;
			entry.Handle = asset;
			entry.RefCount = 1;
			s_Registry[key] = entry;
			TrackInScene(key);
		}

		return asset;
	}

	// ---- Load<TileMapAsset> ----

	template<>
	Ref<TileMapAsset> AssetManager::Load<TileMapAsset>(const std::filesystem::path& path)
	{
		std::string key = MakeKey(path);

		auto it = s_Registry.find(key);
		if (it != s_Registry.end())
		{
			it->second.RefCount++;
			return std::static_pointer_cast<TileMapAsset>(it->second.Handle);
		}

		auto resolvedPath = ResolvePath(path);
		auto asset = TileMapSerializer::LoadTileMap(resolvedPath.string());

		if (asset)
		{
			AssetEntry entry;
			entry.Type = AssetType::TileMap;
			entry.Handle = asset;
			entry.RefCount = 1;
			s_Registry[key] = entry;
			TrackInScene(key);
		}

		return asset;
	}

}

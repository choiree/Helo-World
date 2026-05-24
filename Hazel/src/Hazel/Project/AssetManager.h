#pragma once

#include "Hazel/Core/Base.h"
#include "Hazel/Renderer/Texture.h"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace Hazel {

	class SpriteSheet;
	class AnimationClip;

	enum class AssetType : uint8_t
	{
		None = 0,
		Texture,
		SpriteSheet,
		AnimationClip
	};

	struct AssetEntry
	{
		AssetType Type = AssetType::None;
		Ref<void> Handle;
		int RefCount = 0;
		std::vector<std::string> Dependencies;
	};

	class AssetManager
	{
	public:
		static void Init();
		static void Shutdown();

		// Type-safe load. Path can be absolute (filesystem) or asset-relative.
		template<typename T>
		static Ref<T> Load(const std::filesystem::path& path);

		// Query
		static bool IsLoaded(const std::filesystem::path& path);

		// Lifecycle
		static void Unload(const std::filesystem::path& path);
		static void UnloadAll();

		// Scene-level lifecycle
		static void BeginScene();
		static void EndScene();       // pop + unload current scene (error cleanup)
		static void CloseScene();     // pop oldest + unload exclusive assets

	private:
		static void TrackInScene(const std::string& key);
		static std::filesystem::path ResolvePath(const std::filesystem::path& path);
		static std::string MakeKey(const std::filesystem::path& path);

		friend class SpriteSheet;   // allow serializer callbacks
		friend class AnimationClip;

		inline static std::unordered_map<std::string, AssetEntry> s_Registry;
		inline static bool s_Initialized = false;

		struct SceneAssetList
		{
			std::vector<std::string> Keys;
		};
		inline static std::vector<SceneAssetList> s_SceneStack;
	};

	// ---- Texture2D specialization (inline, depends only on Texture.h) ----

	template<>
	inline Ref<Texture2D> AssetManager::Load<Texture2D>(const std::filesystem::path& path)
	{
		std::string key = MakeKey(path);

		auto it = s_Registry.find(key);
		if (it != s_Registry.end())
		{
			it->second.RefCount++;
			return std::static_pointer_cast<Texture2D>(it->second.Handle);
		}

		auto resolvedPath = ResolvePath(path);
		auto texture = Texture2D::Create(resolvedPath.string());

		if (texture && texture->IsLoaded())
		{
			AssetEntry entry;
			entry.Type = AssetType::Texture;
			entry.Handle = texture;
			entry.RefCount = 1;
			s_Registry[key] = entry;
			TrackInScene(key);
		}

		return texture;
	}

	// ---- SpriteSheet / AnimationClip specializations (declared here, defined in .cpp) ----

	template<>
	Ref<SpriteSheet> AssetManager::Load<SpriteSheet>(const std::filesystem::path& path);

	template<>
	Ref<AnimationClip> AssetManager::Load<AnimationClip>(const std::filesystem::path& path);

}

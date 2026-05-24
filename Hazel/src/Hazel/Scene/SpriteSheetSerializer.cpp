#include "hzpch.h"
#include "SpriteSheetSerializer.h"

#include "Hazel/Project/Project.h"
#include "Hazel/Project/AssetManager.h"

#include <yaml-cpp/yaml.h>
#include <fstream>

namespace Hazel {

	static Ref<SpriteSheet> DeserializeSpriteSheet(const std::string& filepath)
	{
		YAML::Node data;
		try
		{
			data = YAML::LoadFile(filepath);
		}
		catch (YAML::ParserException e)
		{
			HZ_CORE_ERROR("Failed to load .hsprite file '{0}'\n     {1}", filepath, e.what());
			return nullptr;
		}

		auto sheetNode = data["SpriteSheet"];
		if (!sheetNode)
			return nullptr;

		auto sheet = SpriteSheet::Create();

		uint32_t cellW = sheetNode["CellWidth"] ? sheetNode["CellWidth"].as<uint32_t>() : 0;
		uint32_t cellH = sheetNode["CellHeight"] ? sheetNode["CellHeight"].as<uint32_t>() : 0;
		sheet->SetCellSize(cellW, cellH);

		std::string texPath = sheetNode["TexturePath"] ? sheetNode["TexturePath"].as<std::string>() : "";
		if (!texPath.empty())
		{
			auto tex = AssetManager::Load<Texture2D>(texPath);
			if (tex && tex->IsLoaded())
				sheet->SetTexture(tex);
		}

		sheet->SetSourcePath(filepath);
		return sheet;
	}

	void SpriteSheetSerializer::Serialize(const std::string& filepath, const Ref<SpriteSheet>& sheet)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "SpriteSheet" << YAML::Value;
		out << YAML::BeginMap;

		out << YAML::Key << "Name" << YAML::Value << "Unnamed";
		out << YAML::Key << "CellWidth" << YAML::Value << sheet->GetCellWidth();
		out << YAML::Key << "CellHeight" << YAML::Value << sheet->GetCellHeight();

		if (sheet->GetTexture())
		{
			std::string texPath = sheet->GetTexture()->GetPath();
			if (!texPath.empty())
				out << YAML::Key << "TexturePath" << YAML::Value << texPath;
		}

		out << YAML::EndMap;
		out << YAML::EndMap;

		std::ofstream fout(filepath);
		fout << out.c_str();
	}

	Ref<SpriteSheet> SpriteSheetSerializer::Deserialize(const std::string& filepath)
	{
		// filepath is already a resolved filesystem path from caller
		auto sheet = DeserializeSpriteSheet(filepath);
		if (sheet)
			sheet->SetSourcePath(filepath);
		return sheet;
	}

}

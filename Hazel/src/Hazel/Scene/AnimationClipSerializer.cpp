#include "hzpch.h"
#include "AnimationClipSerializer.h"
#include "SpriteSheetSerializer.h"

#include "Hazel/Project/Project.h"
#include "Hazel/Renderer/SpriteSheet.h"

#include <yaml-cpp/yaml.h>
#include <fstream>

namespace YAML {

	template<>
	struct convert<glm::vec2>
	{
		static Node encode(const glm::vec2& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::vec2& rhs)
		{
			if (!node.IsSequence() || node.size() != 2)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			return true;
		}
	};

}

namespace Hazel {

	static YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec2& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
		return out;
	}

	void AnimationClipSerializer::Serialize(const std::string& filepath, const Ref<AnimationClip>& clip)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "AnimationClip" << YAML::Value;
		out << YAML::BeginMap;

		out << YAML::Key << "Name" << YAML::Value << clip->GetName();
		out << YAML::Key << "Loop" << YAML::Value << clip->IsLooping();

		if (!clip->GetSpriteSheetPath().empty())
			out << YAML::Key << "SpriteSheet" << YAML::Value << clip->GetSpriteSheetPath();

		// Texture path from first frame (for backward compat / no SpriteSheet case)
		if (clip->GetFrameCount() > 0)
		{
			const auto& firstFrame = clip->GetFrame(0);
			if (firstFrame.SubTexture && firstFrame.SubTexture->GetTexture())
			{
				std::string texPath = firstFrame.SubTexture->GetTexture()->GetPath();
				if (!texPath.empty())
					out << YAML::Key << "TexturePath" << YAML::Value << texPath;
			}
		}

		if (clip->GetFrameCount() > 0)
		{
			out << YAML::Key << "Frames" << YAML::Value << YAML::BeginSeq;
			for (size_t i = 0; i < clip->GetFrameCount(); i++)
			{
				const auto& frame = clip->GetFrame(i);
				out << YAML::BeginMap;
				out << YAML::Key << "Duration" << YAML::Value << frame.Duration;

				if (frame.CellX >= 0 && frame.CellY >= 0)
				{
					out << YAML::Key << "Cell" << YAML::Value;
					out << YAML::Flow << YAML::BeginSeq << frame.CellX << frame.CellY << YAML::EndSeq;
				}
				else
				{
					out << YAML::Key << "UV0" << YAML::Value << frame.SubTexture->GetUV0();
					out << YAML::Key << "UV1" << YAML::Value << frame.SubTexture->GetUV1();
				}

				out << YAML::EndMap;
			}
			out << YAML::EndSeq;
		}

		out << YAML::EndMap;
		out << YAML::EndMap;

		std::ofstream fout(filepath);
		fout << out.c_str();
	}

	Ref<AnimationClip> AnimationClip::CreateFromFile(const std::string& filepath)
	{
		auto fullPath = Project::GetAssetFileSystemPath(filepath);
		return AnimationClipSerializer::Deserialize(fullPath.string());
	}

	Ref<AnimationClip> AnimationClipSerializer::Deserialize(const std::string& filepath)
	{
		YAML::Node data;
		try
		{
			data = YAML::LoadFile(filepath);
		}
		catch (YAML::ParserException e)
		{
			HZ_CORE_ERROR("Failed to load .hclip file '{0}'\n     {1}", filepath, e.what());
			return nullptr;
		}

		auto clipNode = data["AnimationClip"];
		if (!clipNode)
			return nullptr;

		std::string name = clipNode["Name"] ? clipNode["Name"].as<std::string>() : "Unnamed";
		bool loop = clipNode["Loop"] ? clipNode["Loop"].as<bool>() : true;

		auto clip = AnimationClip::Create(name, loop);

		// Load SpriteSheet if present
		std::string sheetPath = clipNode["SpriteSheet"] ? clipNode["SpriteSheet"].as<std::string>() : "";
		Ref<SpriteSheet> spriteSheet;
		if (!sheetPath.empty())
		{
			auto resolvedSheet = Project::GetAssetFileSystemPath(sheetPath);
			spriteSheet = SpriteSheetSerializer::Deserialize(resolvedSheet.string());
			if (spriteSheet)
				clip->SetSpriteSheetPath(sheetPath);
		}

		// Load texture (from direct TexturePath or from SpriteSheet)
		Ref<Texture2D> texture;
		if (spriteSheet && spriteSheet->GetTexture())
		{
			texture = spriteSheet->GetTexture();
		}
		else
		{
			std::string texturePath = clipNode["TexturePath"] ? clipNode["TexturePath"].as<std::string>() : "";
			if (!texturePath.empty())
			{
				auto resolvedPath = Project::GetAssetFileSystemPath(texturePath);
				texture = Texture2D::Create(resolvedPath.string());
			}
		}

		auto framesNode = clipNode["Frames"];
		if (framesNode)
		{
			for (auto frameNode : framesNode)
			{
				float duration = frameNode["Duration"] ? frameNode["Duration"].as<float>() : 0.1f;
				int cellX = -1, cellY = -1;
				Ref<SubTexture2D> subTex;

				if (frameNode["Cell"] && spriteSheet)
				{
					auto cellNode = frameNode["Cell"];
					cellX = cellNode[0].as<int>();
					cellY = cellNode[1].as<int>();
					glm::vec2 uv0 = spriteSheet->CellToUV0(cellX, cellY);
					glm::vec2 uv1 = spriteSheet->CellToUV1(cellX, cellY);
					subTex = texture ? SubTexture2D::Create(texture, uv0, uv1) : nullptr;
				}
				else
				{
					glm::vec2 uv0 = frameNode["UV0"] ? frameNode["UV0"].as<glm::vec2>() : glm::vec2(0.0f);
					glm::vec2 uv1 = frameNode["UV1"] ? frameNode["UV1"].as<glm::vec2>() : glm::vec2(1.0f);
					subTex = texture ? SubTexture2D::Create(texture, uv0, uv1) : nullptr;
				}

				clip->AddFrame(subTex, duration);
				if (cellX >= 0)
				{
					auto& frames = const_cast<std::vector<AnimationFrame>&>(clip->GetFrames());
					frames.back().CellX = cellX;
					frames.back().CellY = cellY;
				}
			}
		}

		clip->SetSourcePath(filepath);
		return clip;
	}

}

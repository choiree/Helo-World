#pragma once

#include "SubTexture2D.h"

#include <string>
#include <vector>

namespace Hazel {

	struct AnimationFrame
	{
		Ref<SubTexture2D> SubTexture;
		float Duration = 0.1f;
	};

	class AnimationClip
	{
	public:
		static Ref<AnimationClip> Create(const std::string& name, bool loop = true)
		{
			return CreateRef<AnimationClip>(name, loop);
		}

		AnimationClip(const std::string& name, bool loop = true)
			: m_Name(name), m_Loop(loop) {}

		const std::string& GetName() const { return m_Name; }
		bool IsLooping() const { return m_Loop; }
		size_t GetFrameCount() const { return m_Frames.size(); }

		const std::vector<AnimationFrame>& GetFrames() const { return m_Frames; }
		const AnimationFrame& GetFrame(size_t index) const { return m_Frames[index]; }

		void SetLooping(bool loop) { m_Loop = loop; }

		void AddFrame(const Ref<SubTexture2D>& subTexture, float duration = 0.1f)
		{
			m_Frames.push_back({ subTexture, duration });
		}

		float GetTotalDuration() const
		{
			float total = 0.0f;
			for (const auto& frame : m_Frames)
				total += frame.Duration;
			return total;
		}

	private:
		std::string m_Name;
		bool m_Loop = true;
		std::vector<AnimationFrame> m_Frames;
	};

}

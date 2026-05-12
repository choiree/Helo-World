#include <Hazel.h>
#include <Hazel/Core/EntryPoint.h>

#include "EditorLayer.h"

namespace Hazel {

	class Hazelnut : public Application
	{
	public:
		Hazelnut(const ApplicationSpecification& spec)
			: Application(spec)
		{
			PushLayer(new EditorLayer());
		}
	};

	Application* CreateApplication(ApplicationCommandLineArgs args)
	{
		ApplicationSpecification spec;
		spec.Name = "Hazelnut";
		spec.CommandLineArgs = args;
		
		// 设置工作目录为 Hazelnut 项目目录（自动找到 assets 文件夹）
		std::filesystem::path exePath = __FILE__;
		spec.WorkingDirectory = exePath.parent_path().parent_path().string();

		return new Hazelnut(spec);
	}

}

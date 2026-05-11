#include <Hazel.h>
#include <Hazel/Core/EntryPoint.h>

#include "Sandbox2D.h"
#include "ExampleLayer.h"
#include "Test.h"

class Sandbox : public Hazel::Application
{
public:
	Sandbox(const Hazel::ApplicationSpecification& specification)
		: Hazel::Application(specification)
	{
		// PushLayer(new ExampleLayer());
		PushLayer(new Sandbox2D());
		//PushLayer(new TestLayer());
	}

	~Sandbox()
	{
	}
};

Hazel::Application* Hazel::CreateApplication(Hazel::ApplicationCommandLineArgs args)
	{
		ApplicationSpecification spec;
		spec.Name = "Sandbox";
		// Set working directory to project root so shaders and other assets can be found
		spec.WorkingDirectory = "D:/dev/haze/Hazel/Hazelnut";
		spec.CommandLineArgs = args;

		return new Sandbox(spec);
	}

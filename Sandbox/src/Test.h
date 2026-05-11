#pragma once

#include "Hazel.h"
#include "Hazel/Scene/Scene.h"
#include "Hazel/Scene/Entity.h"
#include "Hazel/Scene/Components.h"
#include <imgui.h>

class TestLayer : public Hazel::Layer
{
public:
	TestLayer() 
		: Layer("ECSTextGameLayer")
	{}

	virtual ~TestLayer() = default;

	virtual void OnAttach() override 
	{
	}

	virtual void OnDetach() override 
	{
		HZ_INFO("=== ECS Text Game Layer Detached ===");
	}

	void OnUpdate(Hazel::Timestep ts) override
	{
	}

	virtual void OnImGuiRender() override 
	{
		
	}

	void OnEvent(Hazel::Event& e) override {}

};

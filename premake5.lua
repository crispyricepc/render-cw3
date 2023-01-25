workspace "oglShadersRelease"
	language "C++"
	cppdialect "C++17"

	platforms { "x64" }
	configurations { "debug", "release" }

	flags "NoPCH"
	flags "MultiProcessorCompile"

	startproject "main"

	debugdir "%{wks.location}"
	objdir "_build_/%{cfg.buildcfg}-%{cfg.platform}-%{cfg.toolset}"
	targetsuffix "-%{cfg.buildcfg}-%{cfg.platform}-%{cfg.toolset}"
	
	-- Default toolset options
	filter "toolset:gcc or toolset:clang"
		linkoptions { "-pthread" }
		buildoptions { "-march=native", "-Wall", "-pthread" }

	filter "toolset:msc-*"
		defines { "_CRT_SECURE_NO_WARNINGS=1" }
		defines { "_SCL_SECURE_NO_WARNINGS=1" }
		buildoptions { "/utf-8" }
	
	filter "*"

	-- default libraries
	filter "system:linux"
		links "dl"
		links "GL"
		links "GLX"
	
	filter "system:windows"
		links "OpenGL32"

	filter "*"

	-- default outputs
	filter "kind:StaticLib"
		targetdir "lib/"

	filter "kind:ConsoleApp"
		targetdir "bin/"
		targetextension ".exe"
	
	filter "*"

	--configurations
	filter "debug"
		symbols "On"
		defines { "_DEBUG=1" }

	filter "release"
		optimize "On"
		defines { "NDEBUG=1" }

	filter "*"

-- Third party dependencies
include "external" 

-- Projects
project "main"
	local sources = { 
		"src/**.cpp",
		"src/**.hpp",
	}

	kind "ConsoleApp"
	location "src"

	files( sources )

	links "common"
	links "x-glfw"
	links "x-glew"

	includedirs( "." );

	dependson "x-glm" 

project "common"
	local sources = { 
		"common/**.cpp",
		"common/**.hpp"
	}

	kind "StaticLib"
	location "common"

	files( sources )

--EOF

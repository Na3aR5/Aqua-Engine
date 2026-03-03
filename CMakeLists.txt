cmake_minimum_required(VERSION 3.16)

project(Aqua)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# APIs
set(ENGINE_SUPPORT_GLFW TRUE)
set(ENGINE_SUPPORT_VULKAN TRUE)

set(ENGINE_ENABLE_SANDBOX TRUE)

add_library(Aqua STATIC
	include/aqua/pch.h

	include/aqua/Build.h
	include/aqua/Debug.h
	include/aqua/Assert.h
	include/aqua/Config.h
	include/aqua/CrashReport.h
	include/aqua/Platform.h
	include/aqua/Error.h
	include/aqua/Logger.h

	include/aqua/engine/Defines.h
	include/aqua/engine/ForwardSystems.h
	include/aqua/engine/Event.h
	include/aqua/engine/Engine.h
	include/aqua/engine/MemorySystem.h
	include/aqua/engine/WindowSystem.h
	include/aqua/engine/EventSystem.h
	include/aqua/engine/LayerSystem.h
	include/aqua/engine/RenderHardwareInterface.h

	include/aqua/engine/graphics/api/VulkanAPI.h

	include/aqua/engine/graphics/wapi/GLFWAPI.h

	include/aqua/utility/StringLiteral.h
	include/aqua/utility/Memory.h

	include/aqua/math/Vector.h

	include/aqua/datastructures/StringBuffer.h
	include/aqua/datastructures/Array.h
	include/aqua/datastructures/Queue.h

	src/Config.cpp
	src/CrashReport.cpp
	src/Platform.cpp
	src/Error.cpp
	src/Logger.cpp

	src/Engine.cpp
	src/MemorySystem.cpp
	src/WindowSystem.cpp
	src/EventSystem.cpp
	src/RenderHardwareInterface.cpp
	src/LayerSystem.cpp
	src/Event.cpp

	src/VulkanAPI.cpp

	src/GLFWAPI.cpp

	src/String.cpp
)

target_precompile_headers(Aqua PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/aqua/pch.h)

if(ENGINE_SUPPORT_GLFW)
	add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/lib/glfw)
	target_link_libraries(Aqua PUBLIC glfw)
endif()

if(ENGINE_SUPPORT_VULKAN)
	find_package(Vulkan REQUIRED)
	target_link_libraries(Aqua PUBLIC Vulkan::Vulkan)
endif()

target_include_directories(Aqua PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${CMAKE_CURRENT_SOURCE_DIR}/external/lib/glfw/include
)

add_executable(Logger ${CMAKE_CURRENT_SOURCE_DIR}/utility/logger/Logger.cpp)
set_target_properties(Logger PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(ENGINE_ENABLE_SANDBOX)
	add_executable(Sandbox ${CMAKE_CURRENT_SOURCE_DIR}/test/sandbox/main.cpp)
endif()

if(MSVC AND CMAKE_CONFIGURATION_TYPES)
    foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
        set_target_properties(Logger PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/bin
        )
    endforeach()
endif()
cmake_minimum_required(VERSION 3.10)

project(Aqua)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_library(Aqua STATIC
	include/aqua/Build.h
	include/aqua/Debug.h
	include/aqua/Assert.h

	include/aqua/Config.h
	src/Config.cpp

	include/aqua/CrashReport.h
	src/CrashReport.cpp

	include/aqua/Platform.h
	src/Platform.cpp

	include/aqua/Error.h
	src/Error.cpp

	include/aqua/Logger.h
	src/Logger.cpp

	include/aqua/engine/Defines.h
	include/aqua/engine/ForwardSystems.h

	include/aqua/engine/Engine.h
	src/Engine.cpp

	include/aqua/engine/MemorySystem.h
	src/MemorySystem.cpp

	include/aqua/engine/WindowSystem.h
	src/WindowSystem.cpp

	include/aqua/engine/EventSystem.h
	src/EventSystem.cpp

	include/aqua/engine/GraphicsAPI.h
	src/GraphicsAPI.cpp

	include/aqua/engine/LayerSystem.h
	src/LayerSystem.cpp

	include/aqua/engine/Event.h
	src/Event.cpp

	include/aqua/utility/StringLiteral.h
	include/aqua/utility/Memory.h

	include/aqua/math/Vector.h

	include/aqua/datastructures/String.h
	src/String.cpp

	include/aqua/datastructures/StringBuffer.h
	include/aqua/datastructures/Array.h
	include/aqua/datastructures/Queue.h
)

add_executable(Logger ${CMAKE_CURRENT_SOURCE_DIR}/utility/logger/Logger.cpp)

set_target_properties(Logger PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
if(MSVC AND CMAKE_CONFIGURATION_TYPES)
    foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
        set_target_properties(Logger PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/bin
        )
    endforeach()
endif()

target_include_directories(Aqua PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${CMAKE_CURRENT_SOURCE_DIR}/../glfw/include
	${CMAKE_CURRENT_SOURCE_DIR}/../glad/include
)
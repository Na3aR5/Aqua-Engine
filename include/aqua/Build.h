#ifndef AQUA_BUILD_HEADER
#define AQUA_BUILD_HEADER

#ifndef NDEBUG
	#define AQUA_DEBUG 1
	#define AQUA_RELEASE 0
#else
	#define AQUA_DEBUG 0
	#define AQUA_RELEASE 1
#endif // !NDEBUG

// ------------------------------------------ Debug --------------------------------------------

#define AQUA_DEBUG_ENABLE_LOG			  AQUA_DEBUG
#define AQUA_DEBUG_ENABLE_WARNINGS		  AQUA_DEBUG
#define AQUA_DEBUG_ENABLE_LOG_INFO		  AQUA_DEBUG
#define AQUA_DEBUG_ENABLE_LOG_ALLOCATIONS AQUA_DEBUG

#define AQUA_DEBUG_UNIQUE_DATA_TRACK_MEMORY_SIZE (AQUA_DEBUG && 1)
#define AQUA_DEBUG_SHARED_DATA_TRACK_MEMORY_SIZE (AQUA_DEBUG && 1)

#define AQUA_DEBUG_ENABLE_REFERENCE_COUNT (AQUA_DEBUG && 1)

// ----------------------------------------- Release -------------------------------------------

#define AQUA_RELEASE_ENABLE_LOG				AQUA_RELEASE
#define AQUA_RELEASE_ENABLE_WARNINGS		AQUA_RELEASE
#define AQUA_RELEASE_ENABLE_LOG_INFO        0
#define AQUA_RELEASE_ENABLE_LOG_ALLOCATIONS 0

// ---------------------------------------- Build Type -----------------------------------------

#define AQUA_BUILD_TYPE_ENABLE_LOG \
	((AQUA_DEBUG && AQUA_DEBUG_ENABLE_LOG) || (AQUA_RELEASE && AQUA_RELEASE_ENABLE_LOG))

#define AQUA_BUILD_TYPE_ENABLE_WARNINGS \
	((AQUA_DEBUG && AQUA_DEBUG_ENABLE_WARNINGS) || (AQUA_RELEASE && AQUA_RELEASE_ENABLE_WARNINGS))

#define AQUA_BUILD_TYPE_ENABLE_LOG_INFO \
	((AQUA_DEBUG && AQUA_DEBUG_ENABLE_LOG_INFO) || (AQUA_RELEASE && AQUA_RELEASE_ENABLE_LOG_INFO))

#define AQUA_BUILD_TYPE_ENABLE_LOG_ALLOCATIONS \
	((AQUA_DEBUG && AQUA_DEBUG_ENABLE_LOG_ALLOCATIONS) || (AQUA_RELEASE && AQUA_RELEASE_ENABLE_LOG_ALLOCATIONS))

// ------------------------------------------ OpenGL ------------------------------------------

#define AQUA_OPENGL_GRAPHICS_API 1

#endif // !AQUA_BUILD_HEADER
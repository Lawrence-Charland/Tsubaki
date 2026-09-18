file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS "*.cc")

add_library(core STATIC ${CORE_SOURCES})
target_include_directories(core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/")
target_include_directories(core PUBLIC "${CMAKE_SOURCE_DIR}/")
target_link_libraries(core PUBLIC plugins)

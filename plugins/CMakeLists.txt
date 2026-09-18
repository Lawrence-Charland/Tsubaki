file(GLOB PLUGIN_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
)

find_package(OpenSSL REQUIRED)

add_library(plugins STATIC "${PLUGIN_SOURCES}")

target_link_libraries(plugins PUBLIC OpenSSL::Crypto)

find_package(GTest QUIET)

if(NOT GTest_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

add_executable(tsubaki_tests
    test_arg_parser.cc
    test_cmp_dup.cc
    test_common.cc
    test_encoder.cc
    test_file_list.cc
    test_lock_free_queue.cc
    test_progress_bar.cc
    test_trie.cc
)

target_link_libraries(tsubaki_tests PRIVATE core plugins GTest::gtest_main)

target_include_directories(tsubaki_tests PRIVATE
    "${CMAKE_SOURCE_DIR}/core"
    "${CMAKE_SOURCE_DIR}/plugins"
)

include(GoogleTest)
gtest_discover_tests(tsubaki_tests)

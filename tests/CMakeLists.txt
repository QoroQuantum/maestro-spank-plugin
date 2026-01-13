cmake_minimum_required(VERSION 3.14)

# Download GoogleTest
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Mock Library
add_library(spank_mock
    mocks/mock_spank.cpp
)
target_include_directories(spank_mock PUBLIC mocks)

# We need to compile the plugin logic but link against our MOCK instead of real SLURM
# The plugin code is in ../src/maestro_spank_plugin.cpp.
# BUT, that file includes "slurm/slurm.h". We must ensure our mock headers come first in update include path.

# Define the test executable
add_executable(plugin_tests
    test_main.cpp
    ../src/maestro_spank_plugin.cpp
)

# Important: ensure mocks/ is searched BEFORE system paths or other paths so we pick up our slurm.h
target_include_directories(plugin_tests PRIVATE mocks ../src)

target_link_libraries(plugin_tests GTest::gtest_main spank_mock)

include(GoogleTest)
gtest_discover_tests(plugin_tests)

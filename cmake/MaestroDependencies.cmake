include(ExternalProject)
include(FetchContent)

ExternalProject_Add(
	maestro
	GIT_REPOSITORY https://github.com/QoroQuantum/maestro.git
	GIT_TAG main
	UPDATE_COMMAND ""
	CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR} -DBUILD_TESTING=OFF -DMAESTRO_BUILD_DOCS=OFF
    PATCH_COMMAND sed -i.bak "s/set(COMPILE_TESTS True)/option(COMPILE_TESTS \"Compile tests\" OFF)/" <SOURCE_DIR>/CMakeLists.txt
    BUILD_COMMAND ${CMAKE_COMMAND} --build .
    INSTALL_COMMAND ${CMAKE_COMMAND} --install .
	)

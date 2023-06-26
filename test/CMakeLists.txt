set(INSTALL_GTEST OFF)

# Setup gtest
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)


FetchContent_MakeAvailable(googletest)
include(GoogleTest)

SET(DEPENDENCY_LIBRARIES 
    rend
)

FILE(GLOB TEST_SOURCES LIST_DIRECTORIES true ${CMAKE_CURRENT_SOURCE_DIR}/*_test.cpp)
message(${CMAKE_CURRENT_SOURCE_DIR}/test/*_test.cpp)
foreach(file_path ${TEST_SOURCES})
    string( REPLACE ".cpp" "" new_name ${file_path} )
    get_filename_component(filename ${new_name} NAME)
    message("\t${file_path}")
    add_executable( ${filename} ${file_path} )
    target_link_libraries(${filename} ${DEPENDENCY_LIBRARIES} GTest::gtest_main)
    gtest_discover_tests(${filename})
endforeach()
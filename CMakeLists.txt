cmake_minimum_required(VERSION 3.17)
project(Car VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

if(WIN32)
    set(CAR_CXX_FLAGS /W4 /WX /DUNICODE /D_UNICODE)
endif()

find_package(GTest CONFIG REQUIRED)
find_package(ZLIB REQUIRED)

add_library(CarLib CarLib/Archive.cpp CarLib/Archive.h)
target_compile_options(CarLib PRIVATE ${CAR_CXX_FLAGS})
target_include_directories(CarLib PUBLIC .)
target_link_libraries(CarLib PUBLIC ZLIB::ZLIB)

add_executable(CarLib.Test CarLib.Test/ArchiveTest.cpp)
target_compile_options(CarLib.Test PRIVATE ${CAR_CXX_FLAGS})
target_link_libraries(CarLib.Test PRIVATE CarLib GTest::gtest GTest::gtest_main)
add_test(CarLib.Test CarLib.Test)

add_executable(Car Car/main.cpp)
target_compile_options(Car PRIVATE ${CAR_CXX_FLAGS})
target_link_libraries(Car PRIVATE CarLib Ame)
if(WIN32)
    # Unix style globbing, on Windows. Wowzers!
    set_target_properties(Car PROPERTIES LINK_FLAGS "setargv.obj")
endif()
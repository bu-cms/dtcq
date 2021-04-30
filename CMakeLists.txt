cmake_minimum_required(VERSION 3.14)
project(dtcq)

# Google Test requires at least C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--no-as-needed -ldl -lpthread")

# list executables here to be compiled
set(EXECUTABLES
	demo_fifo
	demo_multiple_fifo
	)

include(FetchContent)
FetchContent_Declare(
	googletest
	URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
	)

FetchContent_MakeAvailable(googletest)

enable_testing()

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

file(GLOB srcs ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)


foreach( exe ${EXECUTABLES} )
	add_executable(${exe} ${CMAKE_CURRENT_SOURCE_DIR}/src/${exe}.cc ${srcs})
	target_link_libraries(${exe})
endforeach()

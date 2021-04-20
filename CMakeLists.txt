cmake_minimum_required(VERSION 3.14)
project(dtcq)

# Google Test requires at least C++11
set(CMAKE_CXX_STANDARD 11)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -pthread -pedantic -Wall -Wextra -Wnon-virtual-dtor")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-return-type-c-linkage -Wno-gnu-zero-variadic-macro-arguments -Wno-vla-extension")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--no-as-needed -ldl -lpthread -I${CMAKE_CURRENT_SOURCE_DIR}")

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
    ${CMAKE_CURRENT_SOURCE_DIR}/dspatch
    ${CMAKE_CURRENT_SOURCE_DIR}/dspatch/include
    ${CMAKE_CURRENT_SOURCE_DIR}/dspatch/include/dspatch
    ${CMAKE_CURRENT_SOURCE_DIR}/dspatch/src
    ${CMAKE_CURRENT_SOURCE_DIR}/dspatch/src/internal
)

file(GLOB srcs ${CMAKE_CURRENT_SOURCE_DIR}/src/EventBoundaryFinderStreamAligned.cc ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB dspatch_srcs ${CMAKE_CURRENT_SOURCE_DIR}/dspatch/src/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/dspatch/src/internal/*.cpp)

add_subdirectory("dspatch")
link_directories("dspatch")

add_executable(demo_evtboundary ${CMAKE_CURRENT_SOURCE_DIR}/src/demo_evtboundary.cc ${srcs} ${dspatch_srcs})

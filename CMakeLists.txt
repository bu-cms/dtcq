cmake_minimum_required(VERSION 3.14)
project(dtcq LANGUAGES CXX)


# Google Test requires at least C++11
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--no-as-needed -ldl -lpthread -O3") # maximum compiler optimization
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--no-as-needed -ldl -lpthread -g") # for gdb debugging
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--no-as-needed -ldl -lpthread -O3 -pg -no-pie") #for gprof

# list executables here to be compiled
set(EXECUTABLES
	demo_fifo_verification
	demo_multiple_fifo
	demo_evtboundary
	dtc
	)

# Find packages Boost, ROOT
find_package(Boost 1.71.0 REQUIRED COMPONENTS filesystem system)
find_package(ROOT CONFIG REQUIRED COMPONENTS RIO Core Tree TreePlayer)

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
    target_link_libraries(${exe} ROOT::Core ROOT::RIO ROOT::Tree ROOT::TreePlayer Boost::filesystem)
endforeach()

cmake_minimum_required(VERSION 3.20)
project(DDModernControls LANGUAGES C CXX)

if(NOT CMAKE_GENERATOR_PLATFORM AND NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(WARNING "DD_CD.EXE is a 32-bit process. Configure with -A Win32 "
                     "(e.g. cmake -B build -A Win32) so the mod DLL and injector are 32-bit too.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(third_party/minhook)
add_subdirectory(moddll)
add_subdirectory(injector)

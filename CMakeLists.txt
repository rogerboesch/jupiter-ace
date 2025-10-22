cmake_minimum_required(VERSION 3.13)
project(jupiter C CXX)

# Set C++ standard for all targets
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build information
message("-- CMAKE_SYSTEM_INFO_FILE: ${CMAKE_SYSTEM_INFO_FILE}")
message("-- CMAKE_SYSTEM_NAME:      ${CMAKE_SYSTEM_NAME}")
message("-- CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")
message("-- CMAKE_SYSTEM:           ${CMAKE_SYSTEM}")

if(APPLE)    
    message("\n>>> Detected Apple platform")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "armv6l") 
        message("\n>>> Detected 32-bit Linux platform")
    else()
        message("\n>>> Detected 64-bit Linux platform")
    endif()

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffast-math")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffast-math")
else()
    message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}. Only Linux and Apple platforms are supported yet.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Put all executables into build/bin
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# SDL2 discovery (Config mode first, then pkg-config fallback)
# We include the SDL "include/SDL2" directory so #include <SDL.h> works.
set(SDL2_TARGET SDL2::SDL2)
set(SDL2MAIN_TARGET "")
set(SDL2_INCLUDE_DIRS "")

# Try modern CMake config package
find_package(SDL2 CONFIG QUIET)
if (SDL2_FOUND)
    if (NOT TARGET SDL2::SDL2)
        message(FATAL_ERROR "Found SDL2 config, but no SDL2::SDL2 target exposed.")
    endif()
    if (TARGET SDL2::SDL2main)
        set(SDL2MAIN_TARGET SDL2::SDL2main)
    endif()
  
    # Extract include dirs from target for #include <SDL.h>
    get_target_property(_SDL2_INCS SDL2::SDL2 INTERFACE_INCLUDE_DIRECTORIES)
    set(SDL2_INCLUDE_DIRS "${_SDL2_INCS}")
else()
    # Fallback to pkg-config
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)
    # add_library(SDL2::SDL2 INTERFACE IMPORTED)
    set_target_properties(SDL2::SDL2 PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIRS}"
    INTERFACE_LINK_LIBRARIES "${SDL2_LIBRARIES}")
    set(SDL2_INCLUDE_DIRS "${SDL2_INCLUDE_DIRS}")
endif()

# Helpful defines across all targets
add_compile_definitions(SDL_MAIN_HANDLED)

# Emulator sources
set(JUPITER_SOURCES
	src/z80.c
	src/jupiter.c
	src/jupiter_keyboard_handler.c
	src/jupiter_file_handler.c
)

# === JUPITER-ACE: SDL only ====================================================

add_executable(jupiter ${JUPITER_SOURCES}
    platform/main_sdl.c
    platform/rb_platform.c
)

target_include_directories(jupiter PRIVATE
    src/
    platform/
)

target_link_libraries(jupiter PRIVATE SDL2::SDL2)

if (SDL2MAIN_TARGET)
    target_link_libraries(jupiter PRIVATE ${SDL2MAIN_TARGET})
endif()

# Nice warnings (optional)
if (MSVC)
    target_compile_options(jupiter PRIVATE /W4)
else()
    target_compile_options(jupiter PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()

target_link_libraries(jupiter PRIVATE m)

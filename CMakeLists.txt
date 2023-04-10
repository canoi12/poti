cmake_minimum_required(VERSION 3.0.2 FATAL_ERROR)

project(poti VERSION 0.1.0 LANGUAGES C)

file(GLOB EMBED_FILES embed/*)

message("Compiling for ${CMAKE_SYSTEM_NAME}")

if (CMAKE_SYSTEM_NAME MATCHES "Emscripten")
    set(USE_FLAGS "-s USE_SDL=2 -s EXPORTED_FUNCTIONS=\"['_malloc']\"")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${USE_FLAGS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${USE_FLAGS}")
    set(CMAKE_EXECUTABLE_SUFFIX .html)
    set(EMSCRIPTEN 1)
endif()

add_executable(gen gen.c)
if (NOT CMAKE_CROSSCOMPILING)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/embed.h
        COMMAND gen -n EMBED_H -t "embed.h" ${EMBED_FILES}
        DEPENDS gen ${EMBED_FILES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
    add_custom_target(embed ALL DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/embed.h)
endif()

include_directories(./ external)
include_directories(external/lua/src external/glad/include)

if (WIN32 AND MSVC)
    set(SDL2_DIR external/SDL2/MSVC/cmake)
endif()

if (NOT CMAKE_CROSSCOMPILING)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
else()
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/${CMAKE_SYSTEM_NAME})
endif()

if (NOT CMAKE_CROSSCOMPILING)
endif()
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

include_directories(${OPENGL_INCLUDE_DIR})
include_directories(${SDL2_INCLUDE_DIRS})

set(LIBRARIES ${OPENGL_LIBRARIES} ${SDL2_LIBRARIES})

if (WIN32)
    if (MINGW)
        set(LIBRARIES mingw32 ${LIBRARIES})
    endif()
    set(LIBRARIES ${LIBRARIES} -mwindows gdi32 winmm)
else()
    set(LIBRARIES ${LIBRARIES} dl m pthread)
endif()

file(GLOB poti_source poti.c src/*.c impl.c)
file(GLOB lua_source external/lua/src/*.c)

add_library(lua STATIC ${lua_source})

add_executable(poti ${poti_source})
target_link_libraries(poti ${LIBRARIES} lua)

if (NOT CMAKE_CROSSCOMPILING)
    add_dependencies(poti embed)
endif()

if (WIN32 AND COPY_DLL)
    file(GLOB SDL_LIB /usr/local/cross-tools/${TOOLCHAIN_PREFIX}/bin/SDL2.dll)
    add_custom_command(TARGET poti POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SDL_LIB} "$<TARGET_FILE_DIR:poti>")
endif()
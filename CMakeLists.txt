cmake_minimum_required(VERSION 3.0.2 FATAL_ERROR)

project(poti VERSION 0.1.0 LANGUAGES C)

file(GLOB EMBED_FILES embed/*)

add_executable(gen gen.c)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/embed.h
    COMMAND gen -n EMBED_H -t "embed.h" ${EMBED_FILES}
    DEPENDS gen ${EMBED_FILES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_target(embed ALL DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/embed.h)

include_directories(./ external)
include_directories(external/lua/src external/glad/include)

if (WIN32 AND MSVC)
    message("Compiling for Windows")
    
    set(SDL2_DIR external/SDL2/MSVC/cmake)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

set(LIBRARIES ${SDL2_LIBRARIES} ${OPENGL_LIBRARIES})

if (WIN32)
    set(LIBRARIES ${LIBRARIES} gdi32 winmm)
else()
    set(LIBRARIES ${LIBRARIES} dl m pthread)
endif()

file(GLOB poti_source poti.c src/*.c impl.c)
file(GLOB lua_source external/lua/src/*.c)

file(GLOB SDL_LIB external/SDL2/MSVC/lib/x64/SDL2.dll)

add_library(lua STATIC ${lua_source})

add_executable(poti ${poti_source})
target_link_libraries(poti ${LIBRARIES} lua)
add_dependencies(poti embed)

if (WIN32)
    add_custom_command(TARGET poti POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SDL_LIB} "$<TARGET_FILE_DIR:poti>")
endif()
cmake_minimum_required(VERSION 3.15)
project(Minesweeper VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# WebAssembly
if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    message(STATUS "Building for WebAssembly with Emscripten")
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s USE_SDL=2 -s USE_SDL_TTF=2")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} \
        -s USE_SDL=2 \
        -s USE_SDL_TTF=2 \
        -s ALLOW_MEMORY_GROWTH=1 \
        -s EXPORTED_FUNCTIONS='[_main,_SDL_Init,_SDL_Quit]' \
	-s EXPORTED_RUNTIME_METHODS='[ccall,cwrap]' \
        --preload-file ${CMAKE_SOURCE_DIR}/assets@/assets \
        --use-preload-plugins \
        -s WASM=1 \
        -s SINGLE_FILE=1 \
        -o ${PROJECT_NAME}.html")

    add_executable(${PROJECT_NAME}
        main.c
    )
    
    target_link_libraries(${PROJECT_NAME} PRIVATE m)
    
else()
    find_package(SDL2 REQUIRED)
    find_package(SDL2_ttf REQUIRED)
    
    add_executable(${PROJECT_NAME}
        main.c
    )
    
    target_link_libraries(${PROJECT_NAME} PRIVATE
        SDL2::SDL2 SDL2_ttf::SDL2_ttf m
    )
endif()

target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
#    -Werror
    -Wextra
    -pedantic
    -O2
)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    set(ASSETS_DIR "${CMAKE_SOURCE_DIR}/assets")
    set(BUILD_ASSETS_DIR "${CMAKE_BINARY_DIR}/assets")
    
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${ASSETS_DIR}"
            "${BUILD_ASSETS_DIR}"
        COMMENT "Copying assets to build directory"
    )
endif()

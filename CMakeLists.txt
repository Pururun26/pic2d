cmake_minimum_required(VERSION 3.10)
project(pic2d C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ==========================================================
# --- Оптимизация по размеру ---
# ==========================================================
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE MinSizeRel CACHE STRING "Choose the type of build." FORCE)
endif()

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} -s")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
endif()
# ==========================================================

# ==========================================================
# --- Пути к библиотекам ---
# ==========================================================
set(RAYLIB_DIR ${CMAKE_SOURCE_DIR}/raylib)
set(LUA_DIR ${CMAKE_SOURCE_DIR}/lua)
set(MINIZ_DIR ${CMAKE_SOURCE_DIR}/miniz)

include_directories(
    ${RAYLIB_DIR}/include
    ${LUA_DIR}/include
    picolib
    ${MINIZ_DIR}
)

link_directories(
    ${RAYLIB_DIR}/lib
    ${LUA_DIR}/lib
)

# ==========================================================
# --- Поиск статической библиотеки RAYLIB ---
# ==========================================================
find_library(RAYLIB_LIBRARY
    NAMES libraylib.a raylib
    PATHS ${RAYLIB_DIR}/lib
    NO_DEFAULT_PATH
)
if(NOT RAYLIB_LIBRARY)
    find_library(RAYLIB_LIBRARY raylib)
endif()
if(NOT RAYLIB_LIBRARY)
    message(FATAL_ERROR "Не удалось найти библиотеку raylib!")
endif()

# ==========================================================
# --- Поиск статической библиотеки LUA ---
# ==========================================================
find_library(LUA_LIBRARY
    NAMES liblua.a lua
    PATHS ${LUA_DIR}/lib
    NO_DEFAULT_PATH
)
if(NOT LUA_LIBRARY)
    find_library(LUA_LIBRARY lua)
endif()
if(NOT LUA_LIBRARY)
    message(FATAL_ERROR "Не удалось найти библиотеку lua!")
endif()

# ==========================================================
# --- Исполняемый файл ---
# ==========================================================
add_executable(pic2d
    src/main.c
    src/picolib-lua.c
    src/pic2d-core.c
    miniz/miniz.c
    picolib/picolib.c
)

# ==========================================================
# --- Линковка ---
# ==========================================================
target_link_libraries(pic2d
    ${RAYLIB_LIBRARY}
    ${LUA_LIBRARY}
    gdi32
    winmm
    shell32
    opengl32
)

# ==========================================================
# --- Полная статическая линковка ---
# ==========================================================
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_link_options(pic2d PRIVATE -static)
endif()


# ==========================================================
# --- Скрываем окно консоли ---
# ==========================================================
set_target_properties(pic2d PROPERTIES WIN32_EXECUTABLE TRUE)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_link_options(pic2d PRIVATE -mwindows)
endif()
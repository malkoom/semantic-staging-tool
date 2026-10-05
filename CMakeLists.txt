# 1. Fuerza a CMake a tragar con dependencias que piden versiones muy antiguas (salva a Raylib)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "Minimum policy version" FORCE)

# 2. Usa sintaxis de rango para silenciar el warning de deprecación de la línea 1
cmake_minimum_required(VERSION 3.24...3.30)
project(SemanticStagingTool VERSION 1.0.0 LANGUAGES C CXX)

# 3. Silencia el warning de marcas de tiempo al extraer los .tar.gz
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# --- Raylib ---
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
)
FetchContent_MakeAvailable(raylib)

# --- nlohmann/json ---
FetchContent_Declare(json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)

# --- CPR (C++ Requests) ---
set(CPR_USE_SYSTEM_CURL OFF CACHE BOOL "" FORCE)
set(CPR_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(cpr
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG 1.10.5
)
FetchContent_MakeAvailable(cpr)

# --- Dear ImGui & rlImGui ---
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.7
)
FetchContent_Declare(rlimgui
    GIT_REPOSITORY https://github.com/raylib-extras/rlImGui.git
    GIT_TAG RL55ImGui19207
)
FetchContent_MakeAvailable(imgui rlimgui)

# --- ImGuizmo -----

FetchContent_Declare(
    imguizmo
    GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git
    GIT_TAG        master
)
FetchContent_GetProperties(imguizmo)
if(NOT imguizmo_POPULATED)
    FetchContent_MakeAvailable(imguizmo)

    # ImGuizmo necesita saber dónde está imgui.h
    target_include_directories(imguizmo PUBLIC
        ${imguizmo_SOURCE_DIR}
        ${imgui_SOURCE_DIR} # Ruta a las cabeceras de ImGui/rlImGui
    )
endif()

# --- Fuentes del proyecto ---
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
    "src/*.cpp"
    "src/*.hpp"
)

set(IMGUI_SRC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${rlimgui_SOURCE_DIR}/rlImGui.cpp
)

# --- Configuración del Target ---
add_executable(${PROJECT_NAME} ${SRC_FILES} ${IMGUI_SRC})

target_include_directories(${PROJECT_NAME} PRIVATE
    src
    ${imgui_SOURCE_DIR}
    ${rlimgui_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    raylib
    cpr::cpr
    nlohmann_json::nlohmann_json
    imguizmo::imguizmo
)

# Copia la carpeta assets al directorio de salida del ejecutable tras compilar
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
    COMMENT "Copiando assets al directorio de ejecucion..."
)

# El fragment shader se carga en tiempo de ejecución junto con los assets.
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders"
    COMMENT "Copiando shaders al directorio de ejecucion..."
)

# Install rules are used by the release workflow to create a self-contained
# distribution folder.  On Windows CMake discovers and copies non-system DLLs
# needed by the executable (for example, dependencies that are built as shared
# libraries by FetchContent).
install(TARGETS ${PROJECT_NAME}
    RUNTIME_DEPENDENCY_SET ${PROJECT_NAME}_runtime_dependencies
    RUNTIME DESTINATION "."
)

if(WIN32)
    install(RUNTIME_DEPENDENCY_SET ${PROJECT_NAME}_runtime_dependencies
        DESTINATION "."
        PRE_EXCLUDE_REGEXES
            "api-ms-win-.*"
            "ext-ms-win-.*"
            "kernel32\.dll"
            "user32\.dll"
            "gdi32\.dll"
            "advapi32\.dll"
            "shell32\.dll"
            "ole32\.dll"
            "oleaut32\.dll"
            "ws2_32\.dll"
            "winmm\.dll"
    )
endif()

install(DIRECTORY assets shaders DESTINATION ".")
install(FILES README.md LICENSE DESTINATION ".")

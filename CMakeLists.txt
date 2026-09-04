cmake_minimum_required(VERSION 3.20)

if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()

project(noctua LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    foreach(flag_var
        CMAKE_C_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_RELEASE
    )
        string(REPLACE "/DNDEBUG" "" ${flag_var} "${${flag_var}}")
        string(REPLACE "/D NDEBUG" "" ${flag_var} "${${flag_var}}")
    endforeach()
endif()

set(NOCTUA_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(NOCTUA_THIRDPARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty")
set(NOCTUA_PAYLOAD_JS "${NOCTUA_SRC_DIR}/executor/payload.js")
set(NOCTUA_FREECAM_JS "${NOCTUA_SRC_DIR}/features/misc/freecam.js")
set(NOCTUA_EMBEDDED_PAYLOAD_HEADER "${CMAKE_CURRENT_BINARY_DIR}/generated/executor_payload_embedded.hpp")
set(NOCTUA_EMBEDDED_FREECAM_HEADER "${CMAKE_CURRENT_BINARY_DIR}/generated/freecam_script_embedded.hpp")
set(DXSDK_DIR "$ENV{DXSDK_DIR}")
if(NOT DXSDK_DIR)
    set(DXSDK_DIR "D:/Program Files (x86)/Microsoft DirectX SDK (June 2010)")
endif()

option(NOCTUA_ENABLE_RELEASE_LOGS "Enable runtime logs in release builds" OFF)
option(NOCTUA_ENABLE_CRASH_LOGS "Enable runtime crash log files in release builds" OFF)

# --- Per-build license fingerprint -----------------------------------------
# Mint a fresh, unique identity for this build (UUID, timestamp, entropy) and
# emit it into a generated header the runtime references. This runs on every
# configure so that no two users ever produce a byte-identical binary. The
# resulting short tag is also folded into the output filename below.
set(NOCTUA_FINGERPRINT_HEADER "${CMAKE_CURRENT_BINARY_DIR}/generated/license_fingerprint_generated.hpp")
include("${NOCTUA_SRC_DIR}/platform/gen_license_fingerprint.cmake")

file(GLOB_RECURSE NOCTUA_SOURCES CONFIGURE_DEPENDS
    "${NOCTUA_SRC_DIR}/*.cpp"
    "${NOCTUA_THIRDPARTY_DIR}/dependencies/minhook/src/*.c"
    "${NOCTUA_THIRDPARTY_DIR}/dependencies/ixwebsocket/*.cpp"
    "${NOCTUA_THIRDPARTY_DIR}/kiero/*.cpp"
    "${NOCTUA_THIRDPARTY_DIR}/imgui/*.cpp"
)

list(FILTER NOCTUA_SOURCES EXCLUDE REGEX ".*IXSocket(OpenSSL|MbedTLS|AppleSSL).*")
list(FILTER NOCTUA_SOURCES EXCLUDE REGEX ".*IXSelectInterruptPipe.*")
list(FILTER NOCTUA_SOURCES EXCLUDE REGEX ".*imgui_demo\\.cpp$")
list(FILTER NOCTUA_SOURCES EXCLUDE REGEX ".*[/\\\\]imgui_freetype\\.cpp$")
list(FILTER NOCTUA_SOURCES EXCLUDE REGEX ".*[/\\\\]runtime[/\\\\](init|tasks)\\.cpp$")
list(FILTER NOCTUA_SOURCES EXCLUDE REGEX ".*[/\\\\]ui[/\\\\]preview[/\\\\]model_preview\\.cpp$")

add_library(noctua SHARED ${NOCTUA_SOURCES})

target_sources(noctua PRIVATE
    "${NOCTUA_SRC_DIR}/ui/preview/model_preview.cpp"
)

function(noctua_embed_script target_name input output namespace)
    add_custom_command(
        OUTPUT "${output}"
        COMMAND "${CMAKE_COMMAND}" "-DINPUT=${input}" "-DOUTPUT=${output}" "-DNAMESPACE=${namespace}" -P "${NOCTUA_SRC_DIR}/executor/embed_payload.cmake"
        DEPENDS "${input}" "${NOCTUA_SRC_DIR}/executor/embed_payload.cmake"
        VERBATIM
    )
    add_custom_target("${target_name}" DEPENDS "${output}")
    add_dependencies(noctua "${target_name}")
endfunction()

noctua_embed_script(noctua_embedded_payload "${NOCTUA_PAYLOAD_JS}" "${NOCTUA_EMBEDDED_PAYLOAD_HEADER}" executor_payload)
noctua_embed_script(noctua_embedded_freecam "${NOCTUA_FREECAM_JS}" "${NOCTUA_EMBEDDED_FREECAM_HEADER}" freecam_script)

set_target_properties(noctua PROPERTIES
    OUTPUT_NAME "noctua_${NOCTUA_BUILD_TAG}"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build"
    # multi-config generators ( VS ) append the config subfolder by default -
    # keep the DLL directly in build/ for every configuration
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/build"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/build"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_CURRENT_SOURCE_DIR}/build"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_CURRENT_SOURCE_DIR}/build"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib"
    MSVC_RUNTIME_LIBRARY "MultiThreaded"
)

target_include_directories(noctua PRIVATE
    "${NOCTUA_SRC_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}"
    "${NOCTUA_SRC_DIR}/core"
    "${NOCTUA_SRC_DIR}/render"
    "${NOCTUA_SRC_DIR}/ui/menu"
    "${NOCTUA_THIRDPARTY_DIR}"
    "${NOCTUA_THIRDPARTY_DIR}/imgui"
    "${NOCTUA_THIRDPARTY_DIR}/simplemath"
    "${NOCTUA_THIRDPARTY_DIR}/dependencies"
    "${NOCTUA_THIRDPARTY_DIR}/dependencies/minhook/include"
    "${NOCTUA_THIRDPARTY_DIR}/dependencies/ixwebsocket"
    "${NOCTUA_THIRDPARTY_DIR}/kiero"
    "${NOCTUA_THIRDPARTY_DIR}/nlohmann_json/build/native/include"
    "${DXSDK_DIR}/Include"
)

target_compile_definitions(noctua PRIVATE
    NOCTUA_EXPORTS
    _WINDOWS
    _USRDLL
    IMGUI_DEFINE_MATH_OPERATORS
    _CRT_SECURE_NO_WARNINGS
    _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING
    _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
    # runtime asserts ( imgui IM_ASSERT etc. ) must never pop a modal dialog
    # inside the game - a tripped assert would freeze the render thread. the
    # important stack-recovery code paths run outside of assert()
    NDEBUG
)

target_link_directories(noctua PRIVATE
    "${DXSDK_DIR}/Lib/x64"
)

if(MSVC)
    foreach(flag_var
        CMAKE_C_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_DEBUG
    )
        string(REPLACE "/RTC1" "" ${flag_var} "${${flag_var}}")
        string(REPLACE "/RTCs" "" ${flag_var} "${${flag_var}}")
        string(REPLACE "/RTCu" "" ${flag_var} "${${flag_var}}")
        string(REPLACE "/MDd" "/MT" ${flag_var} "${${flag_var}}")
        string(REPLACE "/MTd" "/MT" ${flag_var} "${${flag_var}}")
    endforeach()

    target_compile_options(noctua PRIVATE
        /EHsc
        /FS
        /Zc:__cplusplus
        $<$<CONFIG:Debug>:/Od>
        $<$<CONFIG:Debug>:/Ob0>
        $<$<CONFIG:Release>:/Od>
        $<$<CONFIG:Release>:/Ob0>
    )
    target_compile_definitions(noctua PRIVATE
        $<$<CONFIG:Debug>:NOCTUA_DEBUG_BUILD>
        $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>
        $<$<CONFIG:Debug>:_HAS_ITERATOR_DEBUGGING=0>
        $<$<CONFIG:Release>:NOCTUA_PRODUCTION_BUILD>
        $<$<AND:$<CONFIG:Release>,$<BOOL:${NOCTUA_ENABLE_RELEASE_LOGS}>>:NOCTUA_ENABLE_RUNTIME_LOGS>
        $<$<AND:$<CONFIG:Release>,$<BOOL:${NOCTUA_ENABLE_CRASH_LOGS}>>:NOCTUA_ENABLE_CRASH_LOGS>
    )
    target_link_options(noctua PRIVATE
        $<$<CONFIG:Release>:/INCREMENTAL>
        $<$<CONFIG:Debug>:/DEBUG>
        $<$<CONFIG:Debug>:/MAP:${CMAKE_CURRENT_SOURCE_DIR}/build/noctua_debug.map>
    )
endif()

target_link_libraries(noctua PRIVATE
    d3d11
    Dwmapi
    d3dcompiler
    ws2_32
    winmm
    bcrypt
    advapi32
    crypt32
    windowscodecs
)

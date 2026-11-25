# Imports the MSVC-built engine DLL from libs/<Debug|Release> as the target "cocos2d" (RelWithDebInfo and Release use libs/Release).
set(TANKS_LIBS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/libs)

foreach(TanksConfig Debug Release)
    if(NOT EXISTS ${TANKS_LIBS_ROOT}/${TanksConfig}/libcocos2d.lib)
        message(FATAL_ERROR "libs/${TanksConfig} is missing: run tools\\engine_libs.ps1")
    endif()
endforeach()

add_library(cocos2d SHARED IMPORTED GLOBAL)
set_target_properties(cocos2d PROPERTIES
    IMPORTED_CONFIGURATIONS "DEBUG;RELEASE;RELWITHDEBINFO;MINSIZEREL"
    IMPORTED_IMPLIB_DEBUG ${TANKS_LIBS_ROOT}/Debug/libcocos2d.lib
    IMPORTED_LOCATION_DEBUG ${TANKS_LIBS_ROOT}/Debug/libcocos2d.dll
    IMPORTED_IMPLIB_RELEASE ${TANKS_LIBS_ROOT}/Release/libcocos2d.lib
    IMPORTED_LOCATION_RELEASE ${TANKS_LIBS_ROOT}/Release/libcocos2d.dll
    IMPORTED_IMPLIB_RELWITHDEBINFO ${TANKS_LIBS_ROOT}/Release/libcocos2d.lib
    IMPORTED_LOCATION_RELWITHDEBINFO ${TANKS_LIBS_ROOT}/Release/libcocos2d.dll
    IMPORTED_IMPLIB_MINSIZEREL ${TANKS_LIBS_ROOT}/Release/libcocos2d.lib
    IMPORTED_LOCATION_MINSIZEREL ${TANKS_LIBS_ROOT}/Release/libcocos2d.dll
)

target_include_directories(cocos2d INTERFACE
    ${COCOS2DX_ROOT_PATH}
    ${COCOS2DX_ROOT_PATH}/cocos
    ${COCOS2DX_ROOT_PATH}/cocos/base
    ${COCOS2DX_ROOT_PATH}/cocos/platform
    ${COCOS2DX_ROOT_PATH}/cocos/platform/win32
    ${COCOS2DX_ROOT_PATH}/cocos/editor-support
    ${COCOS2DX_ROOT_PATH}/cocos/audio/include
    ${COCOS2DX_ROOT_PATH}/extensions
    ${COCOS2DX_ROOT_PATH}/external
    ${COCOS2DX_ROOT_PATH}/external/chipmunk/include/chipmunk
    ${COCOS2DX_ROOT_PATH}/external/glfw3/include/win32
    ${COCOS2DX_ROOT_PATH}/external/win32-specific/gles/include/OGLES
    ${COCOS2DX_ROOT_PATH}/external/freetype2/include/win32
    ${COCOS2DX_ROOT_PATH}/external/freetype2/include/win32/freetype2
    ${COCOS2DX_ROOT_PATH}/external/openssl/include/win32
)

target_compile_definitions(cocos2d INTERFACE
    WIN32 _WINDOWS _USE_MATH_DEFINES GL_GLEXT_PROTOTYPES CC_ENABLE_CHIPMUNK_INTEGRATION=1
    _CRT_SECURE_NO_WARNINGS _SCL_SECURE_NO_WARNINGS
    $<$<CONFIG:Debug>:COCOS2D_DEBUG=1>
)

target_link_directories(cocos2d INTERFACE
    $<IF:$<CONFIG:Debug>,${TANKS_LIBS_ROOT}/Debug,${TANKS_LIBS_ROOT}/Release>
)

target_link_libraries(cocos2d INTERFACE
    libSpine.lib opengl32.lib glew32.lib libzlib.lib libwebp.lib libiconv.lib freetype.lib winmm.lib ws2_32.lib
    libbox2d.lib libpng.lib libjpeg.lib libtiff.lib libchipmunk.lib glfw3.lib libbullet.lib libcurl.lib
)

# The MSVC-built engine has no __SSE__, so Mat4 and Vec4 are 4-byte aligned there; clang-cl defines it and would change the Node layout.
target_compile_options(cocos2d INTERFACE $<$<CXX_COMPILER_ID:Clang>:/clang:-U__SSE__>)

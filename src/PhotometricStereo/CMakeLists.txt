# 光度立体算法动态库 —— CMake 工程
#
# 配置命令（OpenCV_DIR 指向带 OpenCVConfig.cmake 的目录）：
#   cmake -S . -B build -A x64 -DOpenCV_DIR=D:/QT6/opencv-4.6.0/opencv/build
# 构建命令：
#   cmake --build build --config Release
#
# 产物落到 src/bin/：PhotometricStereoDLL.dll（运行期）+ .lib（链接期）+ ExampleMain.exe

cmake_minimum_required(VERSION 3.22)
project(PhotometricStereo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 源码是 UTF-8 无 BOM（带中文注释），MSVC 默认按系统代码页（GBK）读，
# 会报 C4819 并且注释可能乱码。前端那边 Qt 的 mkspec 会自动加 /utf-8，CMake 这边得自己加。
if(MSVC)
    add_compile_options(/utf-8)
endif()

# ---- 产物输出目录 ----
# CMAKE_CURRENT_SOURCE_DIR 是 src/PhotometricStereo，所以 ../bin 就是 src/bin，
# 也就是前端 ZhuZhaoGUI 的 DESTDIR。两者在这里汇合。
set(ZZ_OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../bin")

# 三类产物（可执行 / 动态库 / 静态库导入库）都要指，且 Debug、Release 各指一遍，
# 否则多配置生成器（VS）会按配置名建子目录，产物就散开了
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${ZZ_OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${ZZ_OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${ZZ_OUTPUT_DIR})

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${ZZ_OUTPUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${ZZ_OUTPUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${ZZ_OUTPUT_DIR})

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${ZZ_OUTPUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${ZZ_OUTPUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${ZZ_OUTPUT_DIR})

# ---- 第三方库 ----
# 正常情况 find_package 就够了。但 OpenCV 官方 Windows 包里的 VC 版本标记
# （4.6.0 只带 vc14 / vc15）会跟新编译器对不上，CMake 直接判 OpenCV_FOUND = FALSE，
# 报 "Found OpenCV Windows Pack but it has no binaries compatible with your configuration"。
# 库本身是 ABI 兼容的，只是配置文件的版本检查过不去，所以这里回退到手动指定路径。
find_package(OpenCV QUIET)

if(NOT OpenCV_FOUND)
    message(STATUS "find_package(OpenCV) 没通过版本检查，回退到手动指定路径")
    set(ZZ_OPENCV_ROOT "D:/QT6/opencv-4.6.0/opencv/build" CACHE PATH "OpenCV 根目录")
    set(OpenCV_INCLUDE_DIRS "${ZZ_OPENCV_ROOT}/include")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(OpenCV_LIBS "${ZZ_OPENCV_ROOT}/x64/vc15/lib/opencv_world460d.lib")
    else()
        set(OpenCV_LIBS "${ZZ_OPENCV_ROOT}/x64/vc15/lib/opencv_world460.lib")
    endif()
endif()

# ---- 算法动态库 ----
# 修：源工程用 FILE(GLOB ...) 收集源文件。GLOB 是 CMake 的反模式 ——
#     新增源文件不会触发重新配置，构建结果依赖上一次 configure 留下的缓存，
#     排查起来非常费劲。直接列文件名更可靠。
add_library(PhotometricStereoDLL SHARED
    PhotometricStereo.cpp
    PhotometricStereo.h
)

# 修（关键）：源工程用的是目录级的 add_compile_definitions(ALGO_EXPORT)，
# 它会让【同目录下所有目标】都拿到这个宏，包括下面的 ExampleMain。
# 那样 ExampleMain 里 ZZ_API 也展开成 dllexport，导出方向就反了。
# 正确做法是只给本目标加，并且用 PRIVATE：
#     编译本 DLL 时 → ALGO_EXPORT 已定义 → ZZ_API = dllexport「导出」
#     编译使用方   → 宏不存在          → ZZ_API = dllimport「导入」
target_compile_definitions(PhotometricStereoDLL PRIVATE ALGO_EXPORT)
target_include_directories(PhotometricStereoDLL PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(PhotometricStereoDLL PRIVATE ${OpenCV_LIBS})

# ---- 算法的独立验证程序 ----
add_executable(ExampleMain ExampleMain.cpp)
target_include_directories(ExampleMain PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(ExampleMain PRIVATE ${OpenCV_LIBS} PhotometricStereoDLL)

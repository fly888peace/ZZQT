# ==========================================================
# 烛照 ZhuZhaoGUI —— 上位机工程文件（骨架版）
#
# 本文件已按【本机环境】改好：
#   Qt 6.11.2 + MSVC2022 64bit + OpenCV 4.6.0
# 课程原版环境是 Qt 5.14.2 + VS2019 + OpenCV 4.5.5，直接拿来用会编不过。
#
# 骨架版的特点：SOURCES / HEADERS 里只列【当前已存在的文件】。
# 跟着课程写代码时，每新增一个类，就来这里补两行
#（.cpp 加进 SOURCES，.h 加进 HEADERS），然后重新执行 qmake。
# 如果文件还没写就先把路径列上，qmake 会直接报错卡住你。
# ==========================================================

# 添加依赖的 Qt 模块
QT += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

# 声明 C++ 语言规范版本
# 课程原版写的是 c++11，但 Qt 6 要求至少 C++17，这里改成 c++17
CONFIG += c++17
DEFINES += QT_DEPRECATED_WARNINGS

# 说明：中文源码的编码问题不用额外配置。
# Qt 6 的 win32-msvc mkspec 会自动给编译器加上 /utf-8，
# 所以你在 .cpp/.h 里直接写中文注释和 tr("中文") 都没问题，
# 不会像老的 Qt5 + MSVC 组合那样出现 "常量中有换行符" 之类的报错。

# ----------------------------------------------------------
# 源文件 / 头文件
# 目前只有骨架，能编出一个空窗口
# ----------------------------------------------------------
SOURCES += \
    main.cpp \
    MainWindow.cpp

HEADERS += \
    MainWindow.h

# ----------------------------------------------------------
# 资源文件（按钮图标 + 中文翻译）
# ----------------------------------------------------------
RESOURCES += \
    ZhuzhaoGuiRes.qrc

# ----------------------------------------------------------
# OpenCV 配置
#
# 课程原版（本机不存在，故已替换）：
#   INCLUDEPATH += S:/OPENCV/opencv_4.5.5_install/opencv/build/include
#   LIBS += -lS:/OPENCV/opencv_4.5.5_install/opencv/build/x64/vc15/lib/opencv_world455d
#
# 本机实际位置与库名（版本 4.5.5 -> 4.6.0，库名前缀 opencv_world455 -> opencv_world460）：
#   Debug   链接 opencv_world460d.lib（带 d 后缀）
#   Release 链接 opencv_world460.lib
#
# 注意：改用了 qmake 的标准写法 -L<库目录> -l<库名>，
# 比原版把完整路径塞进 -l 后面更稳妥，编译链接都不容易出岔子。
# 另外 OpenCV 4.6.0 的库是用 vc14/vc15 工具链编的，
# MSVC2022 与它们 ABI 兼容，可以直接链接。
# ----------------------------------------------------------
OPENCV_ROOT = D:/QT6/opencv-4.6.0/opencv/build

INCLUDEPATH += $$OPENCV_ROOT/include

Debug: {
    LIBS += -L$$OPENCV_ROOT/x64/vc15/lib -lopencv_world460d
}
Release: {
    LIBS += -L$$OPENCV_ROOT/x64/vc15/lib -lopencv_world460
}

# ----------------------------------------------------------
# 光度立体算法动态库
#
# 课程「阶段三」才用到，现在先整段注释掉 —— 因为算法库还没编译出来，
# 提前打开会让链接环节直接失败。
#
# 等你在 src/PhotometricStereo/ 里用 CMake 编译出
# PhotometricStereoDLL.lib / .dll（产物会落在 src/bin/）之后，
# 把下面这些行前面的 # 去掉即可。
# ----------------------------------------------------------
# INCLUDEPATH += $$PWD/../PhotometricStereo
# Debug: {
#     LIBS += -L$$PWD/../bin -lPhotometricStereoDLL
# }
# Release: {
#     LIBS += -L$$PWD/../bin -lPhotometricStereoDLL
# }

# ----------------------------------------------------------
# 生成路径：把编译产物输出到 src/bin，方便统一管理
# 注意 $$PWD 是【本 .pro 文件所在目录】，即 src/ZhuZhaoGUI，
# 所以 $$PWD/../bin 就是 src/bin
# ----------------------------------------------------------
CONFIG(debug, debug|release){
    DESTDIR = $$PWD/../bin
}else{
    DESTDIR = $$PWD/../bin
}

# 设置窗口图标
# 课程原版写的是 zhuzhao_logo.ico，但工程里并没有这个文件（实际文件叫 zhuzhao.ico）
# 不改的话编译时会因为找不到图标文件而报错
RC_ICONS = Resouce/icon/zhuzhao.ico

# 添加翻译文件
# 课程原版是 TRANSLATIONS += language_ch.ts
# 但仓库里只有编译好的 language_ch.qm（qrc 里已包含），没有 .ts 源文件，
# 一旦执行 lrelease 就会报错，所以先注释掉。等你需要维护翻译时再打开。
# TRANSLATIONS += language_ch.ts

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

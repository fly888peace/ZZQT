# 烛照 ZhuZhaoGUI —— 上位机前端工程（qmake）
# 环境：Qt 6.11.2 + MSVC2022 64bit + OpenCV 4.6.0
# 注意：必须选 MSVC Kit。本工程链接 MSVC 格式的 .lib，MinGW 只认 .a

QT += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
DEFINES += QT_DEPRECATED_WARNINGS

# 新增类时同步补 SOURCES / HEADERS，然后重新执行 qmake
SOURCES += \
    main.cpp \
    MainWindow.cpp \
    ZZListener.cpp \
    ZZConfigWidget/ZZConfigWidget.cpp \
    ZZConfigWidget/ZZOneParamWidget.cpp \
    ZZConfigWidget/ZZProcessThread.cpp \
    ZZLogWidget/ZZLogMessage.cpp \
    ZZLogWidget/ZZLogWidget.cpp \
    ZZThumWidget/HThumbnailList.cpp \
    ZZThumWidget/LitImgItemWidget.cpp \
    ZZThumWidget/VThumbnailList.cpp \
    ZZViewWidget/CustomGraphicsView.cpp \
    ZZViewWidget/CustomImageItem.cpp \
    ZZViewWidget/CustomImageView.cpp

HEADERS += \
    MainWindow.h \
    ZZListener.h \
    ZZConfigWidget/ImageConvert.h \
    ZZConfigWidget/ZZConfigWidget.h \
    ZZConfigWidget/ZZOneParamWidget.h \
    ZZConfigWidget/ZZProcessThread.h \
    ZZLogWidget/ZZLogMessage.h \
    ZZLogWidget/ZZLogWidget.h \
    ZZThumWidget/HThumbnailList.h \
    ZZThumWidget/LitImgItemWidget.h \
    ZZThumWidget/VThumbnailList.h \
    ZZViewWidget/CustomGraphicsView.h \
    ZZViewWidget/CustomImageItem.h \
    ZZViewWidget/CustomImageView.h

RESOURCES += \
    ZhuzhaoGuiRes.qrc

# ---- OpenCV ----
# Debug / Release 的 CRT 不通用，两套产物必须各自配对 DLL
OPENCV_ROOT = D:/QT6/opencv-4.6.0/opencv/build
INCLUDEPATH += $$OPENCV_ROOT/include
Debug:   LIBS += -L$$OPENCV_ROOT/x64/vc15/lib -lopencv_world460d
Release: LIBS += -L$$OPENCV_ROOT/x64/vc15/lib -lopencv_world460

# ---- 算法动态库 PhotometricStereoDLL ----
# -L 指目录，-l 才是库名。链接期需要 .lib，运行期需要 .dll（放在 exe 同目录 src/bin）
INCLUDEPATH += $$PWD/../PhotometricStereo
Debug:   LIBS += -L$$PWD/../bin -lPhotometricStereoDLL
Release: LIBS += -L$$PWD/../bin -lPhotometricStereoDLL

# ---- 产物统一输出到 src/bin ----
# $$PWD = src/ZhuZhaoGUI，所以 $$PWD/../bin = src/bin
CONFIG(debug, debug|release){
    DESTDIR = $$PWD/../bin
}else{
    DESTDIR = $$PWD/../bin
}

RC_ICONS = Resouce/icon/zhuzhao.ico

# 只有编译好的 .qm 参与构建；language_ch.ts 是翻译源文件，改动后需 lrelease 重新生成 .qm
# TRANSLATIONS += language_ch.ts

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

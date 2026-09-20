<!-- Dev specification for ZhuZhaoGUI (烛照). 按骨架七章填写。 -->
# Developer Specification (DEV_SPEC)

> 版本：1.0 — 烛照项目复刻版
> 项目：**ZhuZhaoGUI（烛照）** —— 机器视觉光度立体缺陷检测上位机
> 课程来源：周旋机器视觉《烛照：光度立体缺陷检测》 https://www.roundvision.cc
> 参照源工程：`D:\QT6\000workspace\ZhuZhao-V1.2.0`（只读，勿改动）
> 本文档定位：**既是复刻目标规格，也是本机环境适配记录**

## 目录

- 项目概述
- 核心特点
- 技术选型
- 测试方案
- 系统架构与模块设计
- 项目排期
- 可扩展性与未来展望

---

## 1. 项目概述

本项目是一个完整的机器视觉应用：**Qt 上位机（前端） + 光度立体算法动态库（后端）**。
它用 4 张不同光照角度拍摄的灰度图，反推出被测物体的**高度场、梯度场、反射率图**，用于缺陷检测。

从工程视角看，它是一次「前端到算法的完整闭环」练习，覆盖：

| 层次 | 涉及内容 |
|------|---------|
| 工程组织 | 目录分层、qmake / CMake 双构建体系、前后端分离 |
| C++ 设计模式 | 单例模式、观察者模式、工厂式对象组织 |
| Qt 开发 | 手写布局、自定义控件、QGraphicsView 视觉窗口、多线程、国际化 |
| 图像处理 | Qt 与 OpenCV 的图像数据互通、灰度图批处理 |
| 算法 | 光度立体（Photometric Stereo）的数学推导与实现 |

### 设计理念 (Design Philosophy)

> **核心定位：以复刻驱动学习 (Learn by Replication)**
>
> 本项目的目标是**不照抄、而是理解后重建**周旋课程中的烛照上位机。
> 复刻的每一步都要求能回答三个问题：这段代码解决什么问题？为什么用这种设计？
> 如果换一种写法会怎样？因此本文档不仅是"要做什么"的清单，
> 也是"为什么这么做"的依据，同时记录了本机环境与课程环境的全部差异。

---

## 2. 核心特点

### 前后端分离：UI 与算法互不干扰

算法被独立封装成**动态库** `PhotometricStereoDLL`，通过一个 C 风格导出函数暴露能力：

```cpp
uint32_t ZZ_API PhotometricStereo(const vector<Mat>& srcImages,
                                  Mat& HeightField,
                                  Mat& Gradient,
                                  Mat& Albedo,
                                  int ImageCount,
                                  vector<float> Slants,
                                  vector<float> Tilts);
```

带来的好处：

- UI 改动不需要重新编译算法库，算法迭代也不影响 UI 工程；
- 算法可以被别的程序复用（源工程里的 `ExampleMain.exe` 就是独立调用示例）；
- 两套代码用两套构建体系：前端 qmake（`.pro`），算法 CMake（`CMakeLists.txt`）。

### 观察者模式驱动的控件解耦

界面上的控件（配置区、日志区、视觉窗口、两组缩略图列表）之间**几乎不直接通信**。
它们统一通过一个全局单例 `ListenerManger` 广播事件，由 `MainWindow` 集中处理：

```
控件发生动作 → ListenerManger::notify(消息) → 所有注册过的监听者 RespondMessage(消息)
```

这样做避免了控件之间互相持有指针、形成"牵一发动全身"的信号槽网。
本项目只有 4 个消息、1 个监听者，属于该模式的**最小可用规模**。

### 单例模式的两种经典实现并存

项目里有两个单例，正好演示了两种写法：

| 单例 | 写法 | 位置 |
|------|------|------|
| `ListenerManger` | **饿汉式**：静态初始化时直接 `new` | `ZZListener.cpp:4` |
| `ZZLogMessage` | **懒汉式 + 双检锁** | `ZZLogMessage.cpp:106` |

### 多线程运行算法，界面不卡

算法在子线程 `ZZProcessThread::run()` 里执行，主线程只负责组织参数和刷新界面。
线程结束通过 `QThread::finished` 信号回到主线程刷新缩略图，是整个项目里**唯一使用 Qt 信号槽做跨对象通信**的地方。

### 手写布局，不使用 .ui 文件

全部界面由代码构造：`new 控件 → 设尺寸 → 塞进 QLayout → setCentralWidget`。
好处是布局逻辑和业务逻辑写在同一个文件里，便于理解控件树；代价是「所见即所得」的调整体验较差。

### 资源与国际化

- 图标与翻译文件打包进 `.qrc`，运行时通过 `:/Resouce/...` 路径读取；
- 中文翻译由 `language_ch.qm` 提供，`main.cpp` 里 `installTranslator` 装载；
- 界面上所有可见文字都走 `tr("...")`，为将来做英文版留了口子。

---

## 3. 技术选型

### 3.1 课程环境 vs 本机环境（**复刻必读**）

课程录制于 2021 年前后，本机环境与之存在代差，**直接把源工程的配置拿来用一定编不过**。

| 项目 | 课程环境 | 本机环境 | 处理方式 |
|------|---------|---------|---------|
| C++ 标准 | C++11 | **C++17**（Qt 6 最低要求） | `.pro` 改 `CONFIG += c++17` |
| Qt | **5.14.2** | **6.11.2** | 注意 Qt5 → Qt6 的 API 变更 |
| 编译器 | VS2019 | **MSVC2022 Kit**（实际工具链是 Visual Studio 2026 / MSVC 14.51） | 沿用 msvc2022_64 Kit |
| OpenCV | 4.5.5（装在 `S:/OPENCV/...`，本机无此盘） | **4.6.0**，位于 `D:\QT6\opencv-4.6.0\opencv\build` | 改 `INCLUDEPATH` 与 `LIBS` |
| CMake | 3.22.3 | 随 Qt 安装 | — |
| 随附 exe | Qt5 版本（`Qt5Core.dll` 等） | 需自编译 | — |

### 3.2 界面与控制层设计

- **不使用 Qt Designer**，全部手写布局，控件树在 `MainWindow::InitWidget()` 中一次性搭建；
- **不使用模型/视图框架**做缩略图，而是 `QListWidget + setItemWidget` 绑定自定义控件；
- 视觉窗口基于 `QGraphicsView + QGraphicsScene + QGraphicsPixmapItem` 三件套自建。

### 3.3 算法层设计（光度立体数学链路）

算法分四步，是理解本项目的技术核心：

1. **建立光照方程**：每张图的像素亮度 `I = ρ · (N · L)`，其中 `ρ` 是反照率，`N` 是表面法向量，`L` 是光源方向单位向量。
2. **由 Slant / Tilt 算光源向量**：
   ```
   z  = cos(Slant)
   xy = sin(Slant)
   x  = sin(Tilt) * xy
   y  = cos(Tilt) * xy
   ```
   4 张图 → 4 个光源向量 → 组成 4×3 矩阵 `Lights`。
3. **最小二乘反解**：把 4 张图的同位置像素组成向量 `I`，则 `ρN = L⁻¹ · I`。
   实现上用 `cv::invert(Lights, LightsInv, DECOMP_SVD)` 求**伪逆**（因为 4×3 不是方阵）。
   得到向量后归一化即为法向量 `N`，长度即为反照率 `ρ`。
4. **由法向量到高度场**：`p = Nx/Nz`、`q = Ny/Nz` 得到梯度场，
   再用**傅里叶域积分**（`globalHeights()`，利用 `cv::dft` 在频率域做除法）把梯度积分成高度 `Z`。
   这一步是全项目最"数学"的地方。

最后三张结果图统一用 `cv::normalize(..., 0, 255, NORM_MINMAX, CV_8U)` 转成可显示的 8 位图。

### 3.4 本机适配清单（已落地，勿重复踩坑）

`.pro` 相对源工程共改动 6 处：

| 原内容 | 现内容 |
|--------|--------|
| `INCLUDEPATH += S:/OPENCV/opencv_4.5.5_install/.../include` | `OPENCV_ROOT = D:/QT6/opencv-4.6.0/opencv/build` → `INCLUDEPATH += $$OPENCV_ROOT/include` |
| `LIBS += -lS:/.../opencv_world455d` | `LIBS += -L$$OPENCV_ROOT/x64/vc15/lib -lopencv_world460d`（Debug） |
| `LIBS += -lS:/.../opencv_world455` | `LIBS += -L$$OPENCV_ROOT/x64/vc15/lib -lopencv_world460`（Release） |
| `CONFIG += c++11` | `CONFIG += c++17` |
| `RC_ICONS = zhuzhao_logo.ico`（该文件不存在） | `RC_ICONS = Resouce/icon/zhuzhao.ico` |
| `SOURCES/HEADERS` 一次列全 14 个类 | 只列已存在的文件，写完一个补一个 |

其他改动：

- **算法库那一段整体注释掉**（`INCLUDEPATH += $$PWD/../PhotometricStereo` 与 `-l$$PWD/../bin/PhotometricStereoDLL`），
  等阶段 E 编译出 `.lib` 再打开，否则链接必然失败；
- **`TRANSLATIONS += language_ch.ts` 注释掉**：仓库里只有编译好的 `.qm`，没有 `.ts` 源文件，执行 lrelease 会报错；
- 不需要手写 `QMAKE_CXXFLAGS += /utf-8`：**Qt 6 的 win32-msvc mkspec 默认就会加 `-utf-8`**，
  中文注释与 `tr("中文")` 都不会乱码；
- 运行时 DLL 已放入 `src/bin/`：`opencv_world460d.dll`（Debug）、`opencv_world460.dll`（Release）。
  **Debug / Release 的 CRT 不通用，两套产物必须各自配对的 DLL。**

### 3.5 Qt Creator 侧的三个"非代码"坑（本机实测）

1. **`.pro` 之外的工程配置不在 `.pro` 旁边**：Qt Creator 20 把工程配置写在
   `src/ZhuZhaoGUI/.qtcreator/ZhuZhaoGUI.pro.user`，不是 `ZhuZhaoGUI.pro.user`。
2. **配置项目页要手动确认**：「构建和运行」页勾选 Kit 后，**还必须点页面右下角的 `Configure Project` 按钮**
   才算提交；否则底部运行栏一直显示「未配置」，且 `.pro.user` 不会生成。
3. **必须先取消 `Hide unsuitable kits`**，否则列表里看不到 `Desktop Qt 6.11.2 MSVC2022 64bit`。

> ⚠️ **Kit 只能选 MSVC，不能选 MinGW。**
> `.pro` 链接的是 MSVC 格式的 `.lib`（`opencv_world460d.lib`、`PhotometricStereoDLL.lib`），
> MinGW 链接器只认 `.a` / `.dll.a`，选错 Kit 会在链接阶段成片报错。

---

## 4. 测试方案

本项目规模小、无自动化测试，验收以**手工验证清单**为主，算法部分用独立示例程序验证。

### 4.1 分阶段手工验收

每个阶段做完，都要能在 Qt Creator 里构建成功并观察到预期现象（详见第 6 章各步的「验收标准」）。
统一前提：构建后 `src/bin/ZhuZhaoGUI.exe` 生成；Debug 构建使用 `opencv_world460d.dll`。

### 4.2 算法正确性验证

源工程的 `PhotometricStereo/ExampleMain.cpp` 是一个独立验证程序：
读取 `images/test_0..3.png`（光照角度 0/90/180/270，Slant 均为 45°），
调用算法后 `imshow` 出源图、反照率图、高度图三张窗口，肉眼判断凹凸是否合理。

示例数据共 5 组（`blister` / `braille` / `tire` / `toothpaste` / `test`），
角度参数记录在 `images/Tilts_Slants.txt`。**GUI 里 4 个参数控件的初始值是写死在代码里的**，
改数据时要同步改角度（`ZZConfigWidget.cpp:84-91`）。

### 4.3 环境问题排查表

| 现象 | 原因 | 处理 |
|------|------|------|
| 编译通过但 exe 起不来，提示「应用程序控制策略已阻止此文件」 | 本机开启**智能应用控制 (SAC)**，拦截未签名 exe | 事件日志 CodeIntegrity EventID 3077；需放行或关闭 SAC |
| 启动即退出，提示缺 DLL | `src/bin/` 缺 OpenCV 运行时库 | 从 `D:\QT6\opencv-4.6.0\opencv\build\x64\vc15\bin` 拷对应版本 |
| 链接报 `cannot find -lopencv_world460d` 或成片 `undefined reference` | 误选了 MinGW Kit | 换回 MSVC Kit |
| 中文变成乱码 / 报「常量中有换行符」 | 源文件编码与编译器解析不一致 | Qt 6 mkspec 已自带 `/utf-8`；若自定义编译参数需自行确认 |

---

## 5. 系统架构与模块设计

### 5.1 整体架构

```
┌─────────────────────────── ZhuZhaoGUI.exe (前端, qmake) ───────────────────────────┐
│                                                                                    │
│  MainWindow  ── 继承 ──▶ ZZListener  ◀──注册/广播──  ListenerManger (单例)          │
│      │                                                    ▲                        │
│      │ 持有 6 个成员                                       │ notify                │
│      ├── ZZConfigWidget ─┬─ ZZOneParamWidget ×4 ────────┘                          │
│      │                   └─ …（Reset / RunOnce 按钮发消息）                        │
│      ├── ZZLogWidget ◀── 信号 ── ZZLogMessage (单例, 接管 qDebug)                  │
│      ├── CustomImageView ──▶ CustomGraphicsView ──▶ CustomImageItem                │
│      ├── HThumbnailList / VThumbnailList ──▶ LitImgItemWidget                      │
│      └── ZZProcessThread (QThread) ──调用──▶ PhotometricStereo(...)                │
│                                                    │                               │
└────────────────────────────────────────────────────┼───────────────────────────────┘
                                                     ▼
                              PhotometricStereoDLL.dll (算法后端, CMake)
                              /  PhotometricStereo.cpp + OpenCV  /
```

**唯一的跨层接口**就是那一个导出函数，前端完全不关心算法内部实现。

### 5.2 目录结构

```
<repo root>/
├─ .gitignore
├─ DEV_SPEC.md                     ← 本文档
└─ src/
   ├─ ZhuZhaoGUI/                  ← 前端工程（qmake）
   │  ├─ ZhuZhaoGUI.pro
   │  ├─ main.cpp
   │  ├─ MainWindow.h / .cpp
   │  ├─ ZZListener.h / .cpp
   │  ├─ ZhuzhaoGuiRes.qrc
   │  ├─ Resouce/icon/             ← 8 个图标（qrc 引用）
   │  ├─ Resouce/translate/        ← language_ch.qm
   │  ├─ ZZConfigWidget/           ← ZZConfigWidget / ZZOneParamWidget / ZZProcessThread / ImageConvert.h
   │  ├─ ZZLogWidget/              ← ZZLogMessage / ZZLogWidget
   │  ├─ ZZThumWidget/             ← HThumbnailList / VThumbnailList / LitImgItemWidget
   │  └─ ZZViewWidget/             ← CustomGraphicsView / CustomImageItem / CustomImageView
   ├─ PhotometricStereo/           ← 算法动态库（CMake）
   │  ├─ CMakeLists.txt
   │  ├─ PhotometricStereo.h / .cpp
   │  └─ ExampleMain.cpp           ← 独立验证程序
   └─ bin/                         ← DESTDIR：编译产物与运行资源
      ├─ PhotometricStereoDLL.dll / .lib
      ├─ images/                   ← 5 组示例图 + Tilts_Slants.txt
      └─ opencv_world460(d).dll
```

**层级不可改**：`.pro` 里用的是相对路径 `$$PWD/../PhotometricStereo` 与 `$$PWD/../bin`，
工程一旦挪位置，算法库头文件和输出目录全部找不到。

### 5.3 模块说明

| 模块 | 类 | 职责 | 关键点 |
|------|----|------|--------|
| 程序入口 | `main.cpp` | 设置界面样式、安装日志处理器、装载翻译、显示主窗口 | 顺序有讲究：日志处理器要早于任何 `qDebug` |
| 主窗口 | `MainWindow` | 搭建全部布局、注册监听、响应消息 | 构造时 `throw std::bad_alloc()` 作为初始化失败兜底 |
| 观察者 | `ZZListener` / `ListenerManger` | 事件定义、注册、广播 | `MESSAGE` 枚举用位或组合传参 |
| 参数配置 | `ZZConfigWidget` | 承载 4 个参数控件，向外提供参数 | `GetPhotometricStereoParams()` 是唯一出口 |
| 单参数 | `ZZOneParamWidget` | 一张输入图 + Slant/Tilt + 加载按钮 | 图像存在成员 `m_qImage` 里，非立即上传 |
| 算法线程 | `ZZProcessThread` | 转换图像格式、调用算法、保存结果 | 继承 `QThread` 重写 `run()`；**没有 `Q_OBJECT`** |
| 图像转换 | `ImageConvert.h` | `QImage ↔ cv::Mat` 双向转换 | 全部是**定义在头文件里的非 inline 自由函数** |
| 日志 | `ZZLogMessage` | 单例 + `qInstallMessageHandler` 接管 | 落盘到 `log/yyyy-MM-dd/log.txt`，超 1 MB 轮转 |
| 日志面板 | `ZZLogWidget` | 显示日志、清空、打开帮助链接 | 通过 `sigDebugHtmlData` 信号追加带颜色的 HTML |
| 缩略图列表 | `HThumbnailList` / `VThumbnailList` | 横向（结果图）与纵向（输入图）列表 | 两者代码几乎相同，差别在 flow 方向与 item 尺寸算法 |
| 缩略图元素 | `LitImgItemWidget` | 自绘一张缩略图与选中红框 | 重写 `paintEvent`，空图时用 `nullImg.png` 兜底 |
| 视觉窗口 | `CustomGraphicsView` | 缩放、自适应、居中、显示鼠标像素 RGB | 棋盘格背景 + `FullViewportUpdate` 消除残影 |
| 图像元素 | `CustomImageItem` | 鼠标悬停上报坐标与 RGB | 信号 `RGBValue(QString)` 驱动左下角标签 |
| 视觉窗口外壳 | `CustomImageView` | 包一层，提供槽函数给缩略图列表连接 | `OnSendImage(QImage&)` 是列表 → 窗口的入口 |

### 5.4 消息流与数据流

**主链路（点 RunOnce 到出结果）：**

```
ZZConfigWidget::OnRunOnceBtnClicked
  → ListenerManger::notify(ZHUZHAO_RUNONCE)
    → MainWindow::RespondMessage(RUNONCE)
      → 取 4 组参数 → ZZProcessThread::SetPhotometricStereoParams()
      → ZZProcessThread::start()            （子线程开始）
        → run()：QImage → cv::Mat → PhotometricStereo() → cv::Mat → QImage
      → finished 信号 → MainWindow::OnProcessThreadFinished()
        → m_pHThumList->addImages(三张结果图)
```

**副链路（加载输入图）：**

```
ZZOneParamWidget::OnSigLoadImageBtnClicked
  → 保存 QImage → notify(ZHUZHAO_UPDATE_SRCIMAGE)
    → MainWindow::RespondMessage(UPDATE_SRCIMAGE)
      → ZZConfigWidget::GetPhotometricStereoParams() → VThumbnailList::addImages()
```

**缩略图 → 视觉窗口：**

```
QListWidget::currentRowChanged → (H|V)ThumbnailList::OnCurrentRowChanged
  → emit SigSelectImageChanged(QImage&) → CustomImageView::OnSendImage → CustomGraphicsView::SetImage
```

### 5.5 资源与国际化

| 资源 | 路径 | 用途 |
|------|------|------|
| `zhuzhao.ico` | `Resouce/icon/` | exe 与窗口图标（`RC_ICONS`） |
| `loadimg.png` `reset.png` `runonce.png` | `Resouce/icon/` | 按钮图标 |
| `clear.png` `helper.png` | `Resouce/icon/` | 日志面板按钮图标 |
| `nullImg.png` | `Resouce/icon/` | 缩略图空状态占位 |
| `language_ch.qm` | `Resouce/translate/` | 中文翻译（由 `main.cpp` 装载） |

`ZhuzhaoGuiRes.qrc` 把它们打包进可执行文件，运行期通过 `:/Resouce/...` 访问。

---

## 6. 项目排期

### 阶段总览

| 阶段 | 目的 | 步骤数 | 状态 |
|------|------|--------|------|
| **A** 工程骨架与构建基座 | 先能编译、能跑出空窗口 | 4 | ✅ 已完成 |
| **B** 前端地基：观察者 + 日志 | 建立全局通信与日志基础设施 | 5 | ✅ 已完成 |
| **C** 主窗口与视觉窗口 | 把界面骨架和图像显示做出来 | 8 | ⬜ 待开始 |
| **D** 参数配置与多线程 | 打通"点按钮 → 跑算法 → 出结果" | 4 | ⬜ 待开始 |
| **E** 光度立体算法动态库 | 实现算法并让前端链接 | 6 | ⬜ 待开始 |

> 总步数 27。每一步都可在 30~90 分钟内完成并单独验证——**做完一步就构建一次**，
> 不要攒着写，否则排错成本会成倍上升。

### 📊 进度跟踪表

#### 阶段 A：工程骨架与构建基座 ✅

- [x] **A1** 建立课程要求的目录层级（`src/ZhuZhaoGUI` + `src/PhotometricStereo` + `src/bin`）
- [x] **A2** `.pro` 适配本机环境（OpenCV 路径 / C++17 / `RC_ICONS` / `DESTDIR`）
- [x] **A3** 拷贝资源与示例数据（qrc、8 个图标、`language_ch.qm`、20 张示例图、算法 DLL）
- [x] **A4** 配置 `.gitignore` 并完成首次编译（产出 `src/bin/ZhuZhaoGUI.exe`）

> 验收标准：Qt Creator 配好 MSVC Kit 后 `Ctrl+B` 成功，运行出现空白窗口。

#### 阶段 B：前端地基：观察者 + 日志 ✅

- [x] **B1** `ZZListener.h` —— 定义 `MESSAGE` 枚举与 `ZZListener` 抽象基类（纯虚 `RespondMessage`）
- [x] **B2** `ZZListener.cpp` —— `ListenerManger` 单例、`registerMessage()` 位拆包注册、`notify()` 广播
- [x] **B3** `ZZLogMessage.h` —— 日志宏（`QDEBUG` 等）与单例接口声明
- [x] **B4** `ZZLogMessage.cpp` —— 消息处理器重定向、格式化、落盘、超 1 MB 轮转
- [x] **B5** `ZZLogWidget` —— `QTextBrowser` 面板 + Clear / Helper 按钮，接 `sigDebugHtmlData`

> 验收标准：`main.cpp` 里加一行 `QDEBUG("启动日志系统")`，能在 `log/` 下看到日志文件。

#### 阶段 C：主窗口与视觉窗口

- [ ] **C1** `MainWindow` 布局骨架（三层嵌套：左右 Splitter + 右侧水平布局）
- [ ] **C2** `ImageConvert.h` —— `QImage2cvMat` / `cvMat2QImage` 双向转换
- [ ] **C3** `LitImgItemWidget` —— 重写 `paintEvent` 自绘缩略图与选中红框
- [ ] **C4** `HThumbnailList` —— 横向 `QListWidget` + `setItemWidget`，发选中信号
- [ ] **C5** `VThumbnailList` —— 纵向版本（flow 与 item 尺寸算法不同）
- [ ] **C6** `CustomImageItem` —— 悬停上报像素坐标与 RGB
- [ ] **C7** `CustomGraphicsView` —— 滚轮缩放、双击自适应、棋盘格背景、坐标标签
- [ ] **C8** `CustomImageView` —— 外壳控件与 `OnSendImage` 槽

> 验收标准：窗口能显示完整界面；用文件对话框加载一张图，能在缩略图列表和视觉窗口里看到它。

#### 阶段 D：参数配置与多线程

- [ ] **D1** `ZZOneParamWidget` —— 标题 + Slant/Tilt 输入框 + 加载按钮
- [ ] **D2** `ZZConfigWidget` —— 创建 4 个参数控件 + Reset / RunOnce 按钮 + 参数出口函数
- [ ] **D3** `ZZProcessThread` —— 继承 `QThread` 重写 `run()`，封装算法调用与结果收集
- [ ] **D4** `MainWindow::RespondMessage` / `OnProcessThreadFinished` —— 把链路串起来

> 验收标准：加载 4 张图、填好角度、点 RunOnce，结果区出现三张图（此时算法可先用占位实现）。

#### 阶段 E：光度立体算法动态库

- [ ] **E1** `CMakeLists.txt` —— `add_library(... SHARED)` + `add_compile_definitions(ALGO_EXPORT)` + 产物输出到 `../bin`
- [ ] **E2** `PhotometricStereo.h` —— 导出宏 `ZZ_API` 与函数声明（注意 `extern "C"` 的取舍）
- [ ] **E3** 光源向量计算与 SVD 伪逆求解 `ρN`
- [ ] **E4** 法向量归一化、`p/q` 梯度场计算
- [ ] **E5** `globalHeights()` 傅里叶域积分求高度场 + 三张结果图归一化输出
- [ ] **E6** 打开 `.pro` 里注释的算法库链接段，跑通端到端

> 验收标准：`ExampleMain.exe` 能对 `images/test_*.png` 输出合理的高度图；
> GUI 端 RunOnce 出的结果与示例程序一致。

---

## 7. 可扩展性与未来展望

| 方向 | 说明 |
|------|------|
| **光源数量泛化** | 当前 `NUM_IMGS` 硬编码为 4。改成运行期决定（用 `Slants.size()`），即可支持 6 / 8 / 12 光源 |
| **批量处理** | `ZZProcessThread` 现在只跑一批。可加队列，把整个文件夹的图组依次处理 |
| **结果导出** | 目前结果只显示不落盘，可加"保存高度图/反照率图"功能 |
| **算法增强** | 引入鲁棒估计（RANSAC / 加权最小二乘）处理高光与阴影；或用 GPU 加速 `dft` 积分 |
| **界面国际化** | 补齐 `language_ch.ts` 源文件，加英文翻译，实现运行期切换语言 |
| **工程化** | 引入自动化测试（算法可用 gtest 做数值回归）、CI 构建；把 OpenCV 依赖纳入包管理 |

---

## 附：已知代码隐患清单（复刻时建议修正）

复刻不是照抄，以下问题在源工程中真实存在，建议写自己的版本时避开。

| 位置 | 问题 | 建议 |
|------|------|------|
| `ZZListener.h:22-27` | `MESSAGE` 用位与判断，但枚举值是 `1,2,3,4` —— **不是 2 的幂**。`RESET = 3` 会让注册 `1\|2` 的人被误注册成 RESET 监听者 | 全部改成 2 的幂：`0x01/0x02/0x04/0x08` |
| `ImageConvert.h` | `QImage2cvMat` / `cvMat2QImage` 是**非 inline 的自由函数**定义在头文件里，目前只被一个 cpp 包含才没报重定义 | 加 `inline`，或拆成 `.h` + `.cpp` |
| `ZZLogMessage.cpp:110` | `Instance()` 里 `QMutex muter;` 是**局部变量**，函数返回即析构，双检锁形同虚设 | 把 mutex 提为静态成员 |
| `ZZLogMessage.h:16` | `QFATAL` 宏写成 `qFatal() << ...`，`qFatal` 是 printf 风格，不接受 `<<`（未被调用故未暴露） | 改用 `qCritical()` 或改为 printf 写法 |
| `ZZProcessThread.h` | 类没有 `Q_OBJECT`，只用到继承来的 `finished` 信号才编得过 | 若将来要新增信号，必须补 `Q_OBJECT` |
| `ZZProcessThread::run()` | 直接读写成员变量与外部数据，无锁保护 | 至少加注释说明"仅在子线程运行期间访问" |
| `ZZLogMessage.cpp:87` | 文件轮转判断 `if(file.size() < 1024*1024) return;` 在 `close()` 之后执行，此时 `size()` 已不可靠 | 关闭前记录大小 |
| `ZZLogWidget.cpp:44` | `sigDebugHtmlData` 是跨线程发射（日志可能来自子线程），`QTextBrowser::append` 直接连接存在线程风险 | 用 `Qt::QueuedConnection` |

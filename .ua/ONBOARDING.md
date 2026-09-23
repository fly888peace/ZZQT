# 烛照 ZhuZhaoGUI · 新人上手指南（Onboarding Guide）

> 由 Understand-Anything 方法分析生成 · 数据源：`.ua/knowledge-graph.json`
> 配套交互仪表盘：双击打开 `.ua/dashboard.html`

## 这个项目是什么

一句话：**Qt 上位机（前端） + 光度立体算法动态库（后端）** 的机器视觉应用。
用 4 张不同光照角度拍摄的灰度图，反推出被测物体的**高度场、梯度场、反射率图**，用于缺陷检测。

- 课程来源：周旋机器视觉《烛照：光度立体缺陷检测》
- 环境基线：Qt 6.11.2 (MSVC2022) + OpenCV 4.6.0 + C++17
- 完整规格书：[DEV_SPEC.md](../DEV_SPEC.md)（**先读它，再看本指南**）

## 五分钟建立全局认知

| 关键词 | 一句话解释 | 代码入口 |
|--------|-----------|---------|
| 前后端分离 | 算法独立成 `PhotometricStereoDLL.dll`，跨层接口只有一个导出函数 | `src/PhotometricStereo/` |
| 观察者模式 | 控件互不通信，经 `ListenerManger` 单例广播，`MainWindow` 集中响应 | `ZZListener.h/.cpp` |
| 双单例 | 饿汉式（`ListenerManger`）与懒汉+双检锁（`ZZLogMessage`）并存对照 | `ZZListener.cpp:8` / `ZZLogMessage.cpp:148` |
| 多线程 | 算法在 `QThread::run()` 子线程跑，界面不卡；结束靠 `finished` 信号 | `ZZProcessThread.cpp` |
| 手写布局 | 不用 .ui 文件，全部 `new 控件 → QLayout → setCentralWidget` | `MainWindow::InitWidget()` |

## 架构分层（自下而上）

```
┌─ 文档层       DEV_SPEC.md / SPRINT.md / interview.md
├─ 构建系统层   ZhuZhaoGUI.pro (qmake) + CMakeLists.txt (算法) → 产物统一 src/bin/
├─ 图像展示层   缩略图列表 (H/VThumbnailList + LitImgItemWidget)
│              视觉窗口 (CustomGraphicsView / CustomImageItem / CustomImageView)
├─ 参数配置层   ZZConfigWidget → ZZOneParamWidget ×4 ；ZZProcessThread ；ImageConvert
├─ 基础设施层   观察者 (ZZListener/ListenerManger) ；日志 (ZZLogMessage/ZZLogWidget)
├─ 应用入口层   main.cpp → MainWindow
└─ 算法后端层   PhotometricStereo.cpp（DLL）+ ExampleMain（独立验证程序）
```

## 主链路：点 RunOnce 到出结果

```
ZZConfigWidget::OnRunOnceBtnClicked
  → ListenerManger::notify(ZHUZHAO_RUNONCE)          【观察者广播】
    → MainWindow::RespondMessage(RUNONCE)             【集中响应】
      → GetPhotometricStereoParams() 取 4 组参数
      → ZZProcessThread::SetPhotometricStereoParams() + start()   【进子线程】
        → run(): QImage→cv::Mat → ZhuZhao::PhotometricStereo() → cv::Mat→QImage
      → finished 信号 → OnProcessThreadFinished()      【回主线程】
        → m_pHThumList->addImages(三张结果图)           【刷新结果条】
```

副链路（加载输入图）与「缩略图→视觉窗口」链路见 DEV_SPEC 5.4 节。

## 推荐阅读顺序（与 dashboard 导览一致）

1. **DEV_SPEC.md** — 全局地图；重点 5.1 架构图与 5.4 消息流
2. **main.cpp**（30 行）— 初始化顺序为什么不能乱：日志 handler → 翻译器 → 窗口
3. **ZZListener.h/.cpp** — 观察者模式最小可用规模；位运算组合消息
4. **MainWindow.cpp** — 组装中枢：三层嵌套布局 + RespondMessage 调度
5. **ZZOneParamWidget / ZZConfigWidget** — 输入侧：「只广播不刷新」的解耦实践
6. **ZZProcessThread** — 唯一 QThread：锁内拷参数、锁外计算
7. **ImageConvert.h** — Qt↔OpenCV 桥梁：零拷贝的悬空风险与 inline 的 ODR 教训
8. **PhotometricStereo.cpp** — 算法核心：SVD 伪逆 + 频域积分（全项目最数学）
9. **ZZLogMessage.cpp** — 工程质量最高的一页：static 锁、锁外 emit、日志轮转
10. **缩略图三件套 / 视觉窗口三件套** — 输出侧：setItemWidget 与 GraphicsView
11. **两个构建文件** — PRIVATE 宏的作用域、产物目录统一
12. 回看 DEV_SPEC 附录「已知代码隐患清单」收尾

## 常见任务从哪改起

| 我想… | 去哪 | 注意 |
|-------|------|------|
| 换一组测试图 | `ZZConfigWidget.cpp:97-104` 写死的角度初值 | 同步改 `images/Tilts_Slants.txt` 记录的角度 |
| 改算法精度 | `PhotometricStereo.cpp` 的 lambda/mu 正则权重 | 频域积分的分母，压高频噪声 |
| 支持 6/8 光源 | `NUM_IMGS`（constexpr，模板参数） | 需把 `cv::Vec<float,N>` 换成动态 `cv::Mat` |
| 加新消息 | `ZZListener.h` 枚举 + `registerMessage` 的 kAllMessages 表 | 枚举值必须是 2 的幂 |
| 加自己的日志 | 任意 .cpp 里 `QDEBUG("...")` | 必须 include `ZZLogMessage.h` |
| 结果图落盘 | `MainWindow::OnProcessThreadFinished` | 目前只显示不保存（见 DEV_SPEC 第 7 章） |

## 环境坑速查（踩过的坑）

- **Kit 只能 MSVC 不能 MinGW**：链接的是 MSVC 格式 `.lib`，MinGW 只认 `.a`
- **Debug/Release 的 CRT 不通用**：两套产物必须各自配对 DLL（`opencv_world460d` vs `460`）
- **目录层级不可改**：`.pro` 用相对路径 `$$PWD/../PhotometricStereo` 与 `../bin`
- **Qt Creator 配置**：取消 `Hide unsuitable kits` → 勾选 Kit → **必须点 Configure Project**
- **智能应用控制 (SAC)** 会拦未签名 exe：事件日志 CodeIntegrity 3077

## 相关文档

- [DEV_SPEC.md](../DEV_SPEC.md) — 开发规格书（架构 / 数学链路 / 排期 / 隐患清单）
- [SPRINT.md](../SPRINT.md) — 冲刺计划
- [interview.md](../interview.md) — 面试问答
- `.ua/knowledge-graph.json` — 结构化知识图谱（68 节点 / 96 边 / 7 层 / 13 站导览）
- `.ua/dashboard.html` — 交互式知识图谱仪表盘（浏览器直接打开）

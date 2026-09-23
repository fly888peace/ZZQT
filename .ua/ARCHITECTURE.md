# ZhuZhaoGUI 知识图谱分析报告

> 生成时间：2026-09-23 · Understand-Anything 手动分析
> 项目：ZhuZhaoGUI（烛照）—— 机器视觉光度立体缺陷检测上位机

## 分析摘要

| 维度 | 数值 |
|------|------|
| 分析文件总数 | 33（源码 26 + 配置 3 + 文档 3 + gitignore 1） |
| 知识图谱节点 | 68（文件 26 / 类 14 / 函数 13 / 概念 4 / 配置 3 / 文档 3 / 其他 5） |
| 知识图谱边 | 96（contains 38 / imports 23 / calls 9 / depends_on 6 / related 8 / 其他 12） |
| 架构分层 | 7（算法 / 入口 / 基础设施 / 配置 / 展示 / 构建 / 文档） |
| 导览站点 | 13 站（从规格书到构建体系） |
| 主语言 | C++（CMake + qmake 双构建） |
| 依赖框架 | Qt 6.11, OpenCV 4.6 |

## 涵盖的设计模式与知识点

- **观察者模式** —— 控件通过 `ListenerManger` 单例广播解耦
- **单例模式** —— 饿汉式（`ListenerManger`）与懒汉式+双检锁（`ZZLogMessage`）两种实现并存
- **多线程** —— `QThread::run()` 子线程跑算法 + `QMutex` 保护数据
- **Qt 信号槽** —— 全工程唯一跨对象信号槽（`finished` → 刷新界面）
- **DLL 导出宏** —— `ZZ_API` + `PRIVATE ALGO_EXPORT` 的作用域控制
- **Qt/OpenCV 互操作** —— `QImage ↔ cv::Mat` 双向转换与零拷贝风险
- **光度立体的数学链路** —— SVD 伪逆 + 傅里叶域积分
- **ODR（单一定义规则）** —— 头文件函数的 inline 教训

## 输出文件

| 文件 | 用途 |
|------|------|
| `.ua/knowledge-graph.json` | 结构化知识图谱（仪表盘数据源） |
| `.ua/dashboard.html` | 交互式可视化仪表盘（浏览器双击即可打开） |
| `.ua/ONBOARDING.md` | 新人上手指南 |
| `.ua/ARCHITECTURE.md` | 本报告 |
| `.ua/meta.json` | 分析元信息（commit / 版本 / 语言） |

## 如何使用

1. **看仪表盘**：双击 `.ua/dashboard.html` 用浏览器打开
   - 滚轮缩放、拖拽平移、点击节点查看详情
   - 左侧面板控制分层显示与节点类型过滤
   - 底部学习导览按依赖顺序带读 13 个站点
   - 顶部搜索框支持按名称/摘要/标签/路径搜索

2. **读文档**：先看 `ONBOARDING.md` 建立全局认知，再配合 DEV_SPEC 深入

3. **重新分析**：改完代码后重新执行 Understand-Anything 的 `/understand` 命令即可增量更新

## 与 Understand-Anything 插件的关系

本目录遵循插件标准数据目录约定（`.ua/`）。若已安装插件：
- 运行 `/understand-dashboard` 会自动读取 `knowledge-graph.json` 并启动官方 Vite 仪表盘
- 运行 `/understand` 会增量更新图谱（本次为手动分析，下次可走增量流程）
- 运行 `/understand-onboard` 会基于图谱生成类似 `ONBOARDING.md` 的指南

本次为未装插件情况下的手动分析，仪表盘为精简自包含版，核心功能（分层渲染/搜索/详情/导览）齐全。

#ifndef _ZZ_PHOTOMETRIC_STEREO_H_
#define _ZZ_PHOTOMETRIC_STEREO_H_

#include <cstdint>
#include <vector>
#include <opencv2/opencv.hpp>

// ---- 导出宏 ----
// 这个宏决定 ZZ_API 展开成哪一半，关键看【编译时有没有定义 ALGO_EXPORT】：
//   编译本 DLL 时  → CMake 传 -DALGO_EXPORT → ZZ_API = __declspec(dllexport)  「我要导出」
//   被前端使用时   → 没定义这个宏           → ZZ_API = __declspec(dllimport)  「从别处导入」
// 所以 ALGO_EXPORT 由构建脚本给，不是谁在代码里 #define 的（见 CMakeLists.txt）。
#ifdef ALGO_EXPORT
#define ZZ_API __declspec(dllexport)
#else
#define ZZ_API __declspec(dllimport)
#endif

// 关于 extern "C"：源工程在开头定义过一个 EXTERN_C 宏（#ifdef __cplusplus → extern "C"），
// 但下面这个函数从头到尾没用它。这是对的 —— extern "C" 的唯一作用是关掉 C++ 名字修饰，
// 而本函数签名里全是 C++ 类型（std::vector / cv::Mat），C 链接根本表达不了，
// 套上它既得不到什么好处，还会让 .def / 名字修饰的讨论变得混乱。
// 真正需要跨语言调用时才值得把接口换成 C 风格（裸指针 + 长度）。

namespace ZhuZhao
{
/*
 * 光度立体重建：由多张不同光照方向的图，反推出物体表面的高度与反射率。
 *
 * @param srcImages   输入图像，至少 3 张，每张的光照方向不同（单通道灰度图）
 * @param HeightField 输出 —— 重建出的高度信息图
 * @param Gradient    输出 —— 重建出的梯度信息图
 * @param Albedo      输出 —— 重建出的反射率信息图
 * @param ImageCount  输入图像数量，常见的有 4、6、12
 * @param Slants      每张图对应的 Slant 角度
 * @param Tilts       每张图对应的 Tilt 角度
 * @return 0 表示成功，-1 表示参数非法
 */
uint32_t ZZ_API PhotometricStereo(const std::vector<cv::Mat>& srcImages,
                                  cv::Mat& HeightField,
                                  cv::Mat& Gradient,
                                  cv::Mat& Albedo,
                                  int ImageCount,
                                  std::vector<float> Slants,
                                  std::vector<float> Tilts);

} // namespace ZhuZhao

#endif // !_ZZ_PHOTOMETRIC_STEREO_H_

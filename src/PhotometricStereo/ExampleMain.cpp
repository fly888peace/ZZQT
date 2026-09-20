#include <iostream>
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include "PhotometricStereo.h"

// 算法库的独立验证程序：不依赖 Qt，用来单独确认算法库编得对不对、算得对不对。
// 读 4 张图 → 跑算法 → 把源图 / 反照率图 / 高度图三个窗口显示出来。
//
// 示例数据共 5 组，它们的 4 个光源角度记录在 images/Tilts_Slants.txt：
//     blister  / braille / toothpaste : Tilts {6.1, 95.0, -176.1, -86.8}
//                                       Slants {41.4, 42.6, 41.7, 40.9}
//     tire    : Tilts {180, 270, 0, 90}  Slants 全部 45
//     test    : Tilts {0, 90, 180, 270}  Slants 全部 45
int main()
{
    constexpr int NUM_IMGS = 4;

    // 示例图在 src/bin/images/ 下，本程序的产物也在 src/bin，
    // 所以从 src/bin 目录直接运行时，相对路径就是 "images/test_"。
    // 换到别的工作目录运行的话，把这里改成绝对路径即可。
    const std::string model = "images/test_";
    const std::vector<float> tilts = { 0, 90, 180, 270 };
    const std::vector<float> slants = { 45, 45, 45, 45 };

    // ---- 读图 ----
    std::vector<cv::Mat> modelImages;
    for (int i = 0; i < NUM_IMGS; i++)
    {
        const std::string path = model + std::to_string(i) + ".png";
        const cv::Mat image = cv::imread(path, cv::IMREAD_GRAYSCALE);
        if (image.empty())
        {
            std::cout << "Read Image " << path << " failed!" << std::endl;
            return -1;
        }
        modelImages.push_back(image);
    }

    // ---- 跑算法 ----
    cv::Mat heightMap;
    cv::Mat gradientMap;
    cv::Mat albedoMap;
    const uint32_t ret = ZhuZhao::PhotometricStereo(modelImages,
                                                    heightMap,
                                                    gradientMap,
                                                    albedoMap,
                                                    NUM_IMGS,
                                                    slants,
                                                    tilts);
    if (ret != 0)
    {
        std::cout << "Algo Run failed!" << std::endl;
        return -1;
    }

    // ---- 看结果 ----
    cv::imshow("SrcImage", cv::imread(model + "1.png", cv::IMREAD_GRAYSCALE));
    cv::imshow("Albedomap", albedoMap);   // 反照率图
    cv::imshow("Heightmap", heightMap);   // 高度图：凹凸是否合理，肉眼就能判断

    cv::waitKey();
    return 0;
}

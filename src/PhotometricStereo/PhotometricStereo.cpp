#include "PhotometricStereo.h"

#include <cmath>

namespace
{
// 频域积分求高度场。
//
// 为什么不直接在空域把梯度累加？因为逐点累加会把每一步的误差一路带下去，
// 图像越大漂得越厉害。频域积分是「全局」的做法：把梯度场变换到频率域，
// 在那里做一次除法（相当于解一个全局的方程），再逆变换回空域。
//
// lambda / mu 是正则项权重，用来压制高频噪声在除法里被放大。
cv::Mat globalHeights(const cv::Mat& Pgrads, const cv::Mat& Qgrads)
{
    cv::Mat P(Pgrads.rows, Pgrads.cols, CV_32FC2, cv::Scalar::all(0));
    cv::Mat Q(Pgrads.rows, Qgrads.cols, CV_32FC2, cv::Scalar::all(0));
    cv::Mat Z(Pgrads.rows, Pgrads.cols, CV_32FC2, cv::Scalar::all(0));

    const float lambda = 1.0f;
    const float mu = 1.0f;

    // 正变换：拿到 P、Q 的频谱。复数的实部/虚部分别存在 Vec2f 的两个分量里
    cv::dft(Pgrads, P, cv::DFT_COMPLEX_OUTPUT);
    cv::dft(Qgrads, Q, cv::DFT_COMPLEX_OUTPUT);

    for (int i = 0; i < Pgrads.rows; i++)
    {
        for (int j = 0; j < Pgrads.cols; j++)
        {
            if (i == 0 && j == 0)
            {
                continue;   // 直流分量对应平均高度，本身无解，留 0（下面再补一行注释）
            }

            const float u = std::sin(static_cast<float>(i * 2 * CV_PI / Pgrads.rows));
            const float v = std::sin(static_cast<float>(j * 2 * CV_PI / Pgrads.cols));

            const float uv = u * u + v * v;                        // 频率模长的平方
            const float d = (1.0f + lambda) * uv + mu * uv * uv;   // 分母，正则项在这里起作用

            Z.at<cv::Vec2f>(i, j)[0] = (u * P.at<cv::Vec2f>(i, j)[1] + v * Q.at<cv::Vec2f>(i, j)[1]) / d;
            Z.at<cv::Vec2f>(i, j)[1] = (-u * P.at<cv::Vec2f>(i, j)[0] - v * Q.at<cv::Vec2f>(i, j)[0]) / d;
        }
    }

    // 平均高度是未知量：把整体的高度基准定在 0（高度图只有相对意义）
    Z.at<cv::Vec2f>(0, 0)[0] = 0.0f;
    Z.at<cv::Vec2f>(0, 0)[1] = 0.0f;

    // 逆变换回到空域，DFT_SCALE 负责把 dft 正变换时被放大的系数除回去
    cv::dft(Z, Z, cv::DFT_INVERSE | cv::DFT_SCALE | cv::DFT_REAL_OUTPUT);

    return Z;
}

} // namespace

uint32_t ZhuZhao::PhotometricStereo(const std::vector<cv::Mat>& srcImages,
                                    cv::Mat& HeightField,
                                    cv::Mat& Gradient,
                                    cv::Mat& Albedo,
                                    int ImageCount,
                                    std::vector<float> Slants,
                                    std::vector<float> Tilts)
{
    if (srcImages.size() < static_cast<size_t>(ImageCount) ||
        Slants.size() < static_cast<size_t>(ImageCount) ||
        Tilts.size() < static_cast<size_t>(ImageCount))
    {
        return -1;
    }

    // 【1】建立光源矩阵：每行是一个光源的单位方向向量，共 4 行
    //
    // 这里写 constexpr 而不是用传进来的 ImageCount，是因为下面 cv::Vec<float, NUM_IMGS>
    // 是模板，模板参数必须是编译期常量。代价就是当前实现固定按 4 张解算 ——
    // 想支持 6 / 8 光源，得把那个 Vec 换成动态矩阵（cv::Mat），不能只改这个数。
    constexpr int NUM_IMGS = 4;

    std::vector<cv::Mat> modelImages;
    cv::Mat Lights(NUM_IMGS, 3, CV_32F);

    for (int i = 0; i < NUM_IMGS; i++)
    {
        const cv::Mat model = srcImages[i];
        if (model.empty())
        {
            return -1;
        }

        // 由 Slant / Tilt 算光源向量：
        //   Slant 是与 Z 轴（垂直向外）的夹角 → 决定 z 分量和水平分量的大小
        //   Tilt  是水平面内的方位角        → 决定水平分量怎么分给 x、y
        // 注意 x、y 的配法跟常见教材相反（源工程原文标了 "x,y are swapped here"），
        // 是为了跟这套示例数据的角度记录方式对齐。改回去会让所有结果方向不正。
        const float z = std::cos(static_cast<float>(CV_2PI * Slants[i] / 360.0));
        const float xy = std::sin(static_cast<float>(CV_2PI * Slants[i] / 360.0));
        const float x = std::sin(static_cast<float>(CV_2PI * Tilts[i] / 360.0)) * xy;
        const float y = std::cos(static_cast<float>(CV_2PI * Tilts[i] / 360.0)) * xy;

        Lights.at<float>(i, 0) = x;
        Lights.at<float>(i, 1) = y;
        Lights.at<float>(i, 2) = z;

        modelImages.push_back(model);
    }

    const int height = modelImages[0].rows;
    const int width = modelImages[0].cols;

    // 【2】解光照方程 I = ρ·(N·L)
    // 每一张图在该像素处给一个方程，4 张图就是 4 个方程、3 个未知量（法向量三分量），
    // 属于超定方程组。写成矩阵形式是 ρN = L⁻¹ · I，其中 L 是 4×3、不是方阵，
    // 没有常规逆矩阵，所以用 SVD 求【伪逆】（DECOMP_SVD 就是干这个的）。
    cv::Mat LightsInv;
    cv::invert(Lights, LightsInv, cv::DECOMP_SVD);

    cv::Mat Normals(height, width, CV_32FC3, cv::Scalar::all(0));   // 表面法向量
    cv::Mat AlbedoMap(height, width, CV_32F, cv::Scalar::all(0));   // 反照率 ρ
    cv::Mat Pgrads(height, width, CV_32F, cv::Scalar::all(0));      // ∂z/∂x
    cv::Mat Qgrads(height, width, CV_32F, cv::Scalar::all(0));      // ∂z/∂y

    for (int x = 0; x < width; x++)
    {
        for (int y = 0; y < height; y++)
        {
            // 把 4 张图在 (x,y) 这一点的亮度凑成一个列向量 I
            cv::Vec<float, NUM_IMGS> I;
            for (int i = 0; i < NUM_IMGS; i++)
            {
                I[i] = modelImages[i].at<uchar>(cv::Point(x, y));
            }

            // ρN = L⁻¹·I。得到的三维向量的【长度是反照率 ρ，方向就是单位法向量 N】
            cv::Mat n = LightsInv * cv::Mat(I);
            const float kd = std::sqrt(static_cast<float>(n.dot(n)));
            if (kd > 0)
            {
                n = n / kd;   // 归一化 → 单位法向量
            }
            if (n.at<float>(2, 0) == 0)
            {
                n.at<float>(2, 0) = 1.0f;   // 兜底：Nz 为 0 时下面的除法会炸
            }

            AlbedoMap.at<float>(cv::Point(x, y)) = kd / 255.0f;

            // 由法向量求梯度：p = ∂z/∂x = Nx/Nz，q = ∂z/∂y = Ny/Nz
            //
            // 源工程这里还有一段 legit 判断：
            //     legit *= modelImages[i].at<uchar>(Point(x,y)) >= 0;
            // 但像素类型是 uchar，恒 >= 0，条件永远成立 → legit 恒为 1，
            // 那个 else 分支从来没执行过。这里直接删掉，保留与原实现等价的结果。
            Normals.at<cv::Vec3f>(cv::Point(x, y)) = n;
            Pgrads.at<float>(cv::Point(x, y)) = n.at<float>(0, 0) / n.at<float>(2, 0);
            Qgrads.at<float>(cv::Point(x, y)) = n.at<float>(1, 0) / n.at<float>(2, 0);
        }
    }

    // 【3】结果输出
    // 上面算出来都是浮点数据，量纲各不相同，用 min-max 归一化压到 0~255
    // 的 8 位图，才能直接当图像显示。
    cv::normalize(AlbedoMap, AlbedoMap, 0, 255, cv::NORM_MINMAX, CV_8U);
    Albedo = AlbedoMap;

    cv::Mat HeightMap = globalHeights(Pgrads, Qgrads);
    cv::normalize(HeightMap, HeightMap, 0, 255, cv::NORM_MINMAX, CV_8U);
    HeightField = HeightMap;

    cv::normalize(Pgrads, Pgrads, 0, 255, cv::NORM_MINMAX, CV_8U);
    Gradient = Pgrads;

    return 0;
}

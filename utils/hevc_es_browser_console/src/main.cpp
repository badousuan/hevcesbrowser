#include <iostream>
#include <fstream>

//#include <boost/program_options.hpp>
#include "cxxopts.hpp"
#include <HevcParser.h>

#include "HEVCInfoWriter.h"
#include "HEVCInfoAltWriter.h"


// 跨平台处理 popen
#ifdef _WIN32
#define POPEN _popen
#define PCLOSE _pclose
#else
#define POPEN popen
#define PCLOSE pclose
#endif

// 核心函数：执行命令并读取二进制输出
bool convertToH265_Pipe(const std::string &inputPath, std::vector<uint8_t> &outputBuffer) {
    // 1. 构建命令
    // -y: 覆盖输出(如果有的话)
    // -loglevel error: 减少日志输出，防止日志混入数据（数据走stdout，日志走stderr，分离是安全的，但保持清爽更好）
    // -i: 输入
    // -c:v libx265: 编码器
    // -preset ultrafast: 牺牲一些压缩率换取最快速度（可选）
    // -an: 移除音频
    // -f hevc: 裸流格式
    // -: 输出到 stdout
    std::string cmd = "ffmpeg -y -loglevel error -i \"" + inputPath + "\" -c:v libx265 -preset ultrafast -an -f hevc -";

    // 2. 打开管道 (以二进制读模式 "rb")
    FILE *pipe = POPEN(cmd.c_str(), "rb");
    if (!pipe) {
        std::cerr << "无法启动 FFmpeg 进程" << std::endl;
        return false;
    }

    // 3. 读取数据
    // 定义一个 4KB 的临时缓冲区
    std::array<uint8_t, 4096> buffer;
    size_t bytesRead;

    // 清空输出容器
    outputBuffer.clear();

    // 循环读取直到结束
    while ((bytesRead = fread(buffer.data(), 1, buffer.size(), pipe)) > 0) {
        outputBuffer.insert(outputBuffer.end(), buffer.begin(), buffer.begin() + bytesRead);
    }

    // 4. 关闭管道并检查返回值
    int returnCode = PCLOSE(pipe);

    // returnCode == 0 通常表示成功
    if (returnCode != 0) {
        std::cerr << "FFmpeg 转换过程中出错或被终止，返回码: " << returnCode << std::endl;
        return false;
    }

    return true;
}

// 辅助函数：判断后缀 (复用之前的逻辑)
bool isMp4(const std::string &path) {
    if (path.length() < 4) return false;
    std::string ext = path.substr(path.length() - 4);
    return ext == ".mp4" || ext == ".MP4";
}

int main2() {
    std::string filename = "video/test.mp4"; // 确保此文件存在

    if (isMp4(filename)) {
        std::cout << "检测到 MP4，开始调用 FFmpeg 命令行转换..." << std::endl;

        std::vector<uint8_t> h265Data;

        // 预分配一些内存避免频繁扩容 (假设视频有 1MB)
        h265Data.reserve(1024 * 1024);

        if (convertToH265_Pipe(filename, h265Data)) {
            std::cout << "转换成功！" << std::endl;
            std::cout << "内存中 H.265 数据大小: " << h265Data.size() << " bytes" << std::endl;

            // 此时 h265Data 中就是完整的裸流，可以直接发送给解码器或写入文件验证
            // 验证代码：写入文件看看是否能播放
            // std::ofstream debugFile("output_debug.h265", std::ios::binary);
            // debugFile.write(reinterpret_cast<const char*>(h265Data.data()), h265Data.size());
        } else {
            std::cout << "转换失败。" << std::endl;
        }
    } else {
        std::cout << "非 .mp4 文件" << std::endl;
    }

    return 0;
}

int main(int argc, char **argv) {

    try {
        // 1. 创建 Options 对象
        cxxopts::Options options("MyProgram", "cxxopts 使用示例");

        // 2. 添加参数
        options.add_options()
                // 添加布尔值 (Flag)
                // 格式: "短参数,长参数", "描述", cxxopts::value<类型>
                ("altwriter", "user alternative writer", cxxopts::value<bool>()->default_value("false"))

                // 添加字符串参数
                ("i,input", "输入文件名 (String)", cxxopts::value<std::string>())

                // 添加单个字符参数
                ("o,output", "输出文件名 (String)", cxxopts::value<std::string>()->default_value(""))

                // 添加帮助选项
                ("h,help", "打印帮助信息");

        // 3. 解析参数
        auto result = options.parse(argc, argv);

        // 4. 处理帮助信息
        if (result.count("help")) {
            std::cout << options.help() << std::endl;
            return 0;
        }

        if (!result.count("input")) {
            std::cout << options.help() << std::endl;
            return 1;
        }

        // 5. 获取并使用参数值

        std::ostream *pout = &std::cout;
        std::ofstream fileOut;

        if (result.count("output")) {
            fileOut.open(result["output"].as<std::string>().c_str());
            if (!fileOut.good()) {
                std::cerr << "problem with opening file `" << result["output"].as<std::string>() << "` for writing";
                return 2;
            }
            pout = &fileOut;
        }

        std::string inPath = result["input"].as<std::string>();


        std::ifstream in(result["input"].as<std::string>().c_str(), std::ios_base::binary);
        if (!in.good()) {
            std::cerr << "problem with opening file `" << result["input"].as<std::string>() << "` for reading";
            return 2;
        }

        in.seekg(0, std::ios::end);
        std::size_t size = in.tellg();
        in.seekg(0, std::ios::beg);

        std::vector<uint8_t> h265Data;
        if (isMp4(inPath)) {
            std::cout << "检测到 MP4，开始调用 FFmpeg 命令行转换..." << std::endl;

            // 预分配一些内存避免频繁扩容 (假设视频有 1MB)
            h265Data.reserve(size);

            if (convertToH265_Pipe(inPath, h265Data)) {
                std::cout << "转换成功！" << std::endl;
                std::cout << "内存中 H.265 数据大小: " << h265Data.size() << " bytes" << std::endl;

                // 此时 h265Data 中就是完整的裸流，可以直接发送给解码器或写入文件验证
                // 验证代码：写入文件看看是否能播放
                // std::ofstream debugFile("output_debug.h265", std::ios::binary);
                // debugFile.write(reinterpret_cast<const char*>(h265Data.data()), h265Data.size());
            } else {
                std::cout << "转换失败。" << std::endl;
            }
        } else {
            std::cout << "非 .mp4 文件" << std::endl;
        }

        uint8_t *pdata = nullptr;


        if (h265Data.size() > 0) {
            pdata = h265Data.data();
        } else {
            h265Data.reserve(size);
            in.read((char *) h265Data.data(), size);
            pdata = h265Data.data();
        }

        if (!pdata) {
            std::cerr << "Problem with memory allocation. Try to restart computer\n";
            return 3;
        }

        HEVC::Parser *pparser = HEVC::Parser::create();
        HEVCInfoWriter *writer = nullptr;
        if (result["altwriter"].as<bool>())
            writer = new HEVCInfoAltWriter();
        else
            writer = new HEVCInfoWriter();
        pparser->addConsumer(writer);

        pparser->process((const uint8_t *) pdata, size);

        HEVC::Parser::release(pparser);

        *pout << result["input"].as<std::string>() << std::endl;
        *pout << "=======================" << std::endl;
        writer->write(*pout);
        delete writer;
    }
    catch (std::exception &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    catch (...) {
        std::cerr << "Exception of unknown type!\n";
        return 1;
    }

    return 0;
}

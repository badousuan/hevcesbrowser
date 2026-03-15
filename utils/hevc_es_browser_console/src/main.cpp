#include <iostream>
#include <fstream>
#include <array>
#include <memory>

#include "cxxopts.hpp"
#include <HevcParser.h>

#include "HEVCInfoWriter.h"
#include "HEVCInfoAltWriter.h"
#include "FrameDependencyAnalyzer.h"

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
    std::string cmd = "ffmpeg -y -loglevel error -i \"" + inputPath + "\" -c:v libx265 -preset ultrafast -an -f hevc -";

    FILE *pipe = POPEN(cmd.c_str(), "rb");
    if (!pipe) {
        std::cerr << "无法启动 FFmpeg 进程" << std::endl;
        return false;
    }

    std::array<uint8_t, 4096> buffer;
    size_t bytesRead;
    outputBuffer.clear();

    while ((bytesRead = fread(buffer.data(), 1, buffer.size(), pipe)) > 0) {
        outputBuffer.insert(outputBuffer.end(), buffer.begin(), buffer.begin() + bytesRead);
    }

    int returnCode = PCLOSE(pipe);
    if (returnCode != 0) {
        std::cerr << "FFmpeg 转换过程中出错或被终止，返回码: " << returnCode << std::endl;
        return false;
    }

    return true;
}

bool isMp4(const std::string &path) {
    if (path.length() < 4) return false;
    std::string ext = path.substr(path.length() - 4);
    return ext == ".mp4" || ext == ".MP4";
}

int main(int argc, char **argv) {
    try {
        cxxopts::Options options("hevc_console_parser", "HEVC ES Parser Console Tool");

        options.add_options()
            ("altwriter", "Use alternative writer", cxxopts::value<bool>()->default_value("false"))
            ("i,input", "Input file name", cxxopts::value<std::string>())
            ("o,output", "Output file name", cxxopts::value<std::string>()->default_value(""))
            ("deps", "Analyze and print frame dependencies", cxxopts::value<bool>()->default_value("true"))
            ("h,help", "Print help");

        auto result = options.parse(argc, argv);

        if (result.count("help") || !result.count("input")) {
            std::cout << options.help() << std::endl;
            return result.count("help") ? 0 : 1;
        }

        std::ostream *pout = &std::cout;
        std::ofstream fileOut;

        if (result.count("output")) {
            fileOut.open(result["output"].as<std::string>());
            if (!fileOut.good()) {
                std::cerr << "Problem opening output file `" << result["output"].as<std::string>() << "` for writing." << std::endl;
                return 2;
            }
            pout = &fileOut;
        }

        std::string inPath = result["input"].as<std::string>();
        std::vector<uint8_t> fileData;
        size_t dataSize = 0;

        if (isMp4(inPath)) {
            std::cout << "Detected MP4, converting with FFmpeg..." << std::endl;
            if (convertToH265_Pipe(inPath, fileData)) {
                std::cout << "Conversion successful. In-memory H.265 data size: " << fileData.size() << " bytes" << std::endl;
                dataSize = fileData.size();
            } else {
                std::cerr << "Conversion failed." << std::endl;
                return 1;
            }
        } else {
            std::ifstream in(inPath, std::ios_base::binary);
            if (!in.good()) {
                std::cerr << "Problem opening input file `" << inPath << "` for reading." << std::endl;
                return 2;
            }
            in.seekg(0, std::ios::end);
            dataSize = in.tellg();
            in.seekg(0, std::ios::beg);
            fileData.resize(dataSize);
            in.read(reinterpret_cast<char*>(fileData.data()), dataSize);
        }

        if (fileData.empty()) {
            std::cerr << "No data to process." << std::endl;
            return 3;
        }

        HEVC::Parser *pparser = HEVC::Parser::create();
        std::unique_ptr<HEVC::Parser::Consumer> consumer;

        if (result["deps"].as<bool>()) {
            consumer.reset(new FrameDependencyAnalyzer());
        } else if (result["altwriter"].as<bool>()) {
            consumer.reset(new HEVCInfoAltWriter());
        } else {
            consumer.reset(new HEVCInfoWriter());
        }

        pparser->addConsumer(consumer.get());
        pparser->process(fileData.data(), dataSize);
        HEVC::Parser::release(pparser);

        *pout << "Input file: " << inPath << std::endl;
        *pout << "=======================" << std::endl;

        if (result["deps"].as<bool>()) {
            dynamic_cast<FrameDependencyAnalyzer*>(consumer.get())->writeDependencies(*pout);
        } else {
            dynamic_cast<HEVCInfoWriter*>(consumer.get())->write(*pout);
        }

    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "An unknown exception occurred." << std::endl;
        return 1;
    }

    return 0;
}

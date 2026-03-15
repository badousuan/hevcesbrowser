#ifndef FRAME_DEPENDENCY_ANALYZER_H_
#define FRAME_DEPENDENCY_ANALYZER_H_

#include <HevcParser.h>
#include <vector>
#include <map>
#include <ostream>
#include <set>
#include <string>

class FrameDependencyAnalyzer : public HEVC::Parser::Consumer {
public:
    FrameDependencyAnalyzer() : m_lastPoc(0) {}
    virtual void onNALUnit(std::shared_ptr<HEVC::NALUnit> pNALUnit, const HEVC::Parser::Info* pInfo) override;
    virtual void onWarning(const std::string& warning, const HEVC::Parser::Info* pInfo, HEVC::Parser::WarningType type) override {};

    void writeDependencies(std::ostream& out);
    void processRPS(std::shared_ptr<HEVC::Slice> pSlice);

private:
    struct FrameData {
        HEVC::NALUnitType nalUnitType;
        int sliceType;
        int temporalId;
        std::set<int> dependencies;
    };

    std::string sliceTypeToString(int sliceType);

    std::map<uint32_t, std::shared_ptr<HEVC::SPS>> m_spsMap;
    std::map<uint32_t, std::shared_ptr<HEVC::PPS>> m_ppsMap;

    std::map<int, FrameData> m_frameData;

    int m_lastPoc;
};

#endif // FRAME_DEPENDENCY_ANALYZER_H_

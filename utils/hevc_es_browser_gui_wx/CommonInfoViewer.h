#ifndef COMMON_INFO_VIEWER_H_
#define COMMON_INFO_VIEWER_H_

#include <vector>
#include <map>
#include <wx/wx.h>
#include <wx/listctrl.h>
#include <HevcParser.h>
#include "types.h"

class CommonInfoViewer : public wxListCtrl, public HEVC::Parser::Consumer
{
public:
    CommonInfoViewer(wxWindow* parent);

    void saveCustomData();
    void clear();

    virtual void onNALUnit(std::shared_ptr<HEVC::NALUnit> pNALUnit, const HEVC::Parser::Info* pInfo) override;
    virtual void onWarning(const std::string& warning, const HEVC::Parser::Info* pInfo, HEVC::Parser::WarningType) override {}

    void OnItemSelected(wxListEvent& event);
    std::pair<std::shared_ptr<HEVC::NALUnit>, ParserInfo> getSelectedNALUInfo();

    const VPSMap& getVPSMap() const { return m_vpsMap; }
    const SPSMap& getSPSMap() const { return m_spsMap; }
    const PPSMap& getPPSMap() const { return m_ppsMap; }

private:
    void readCustomData();

    struct NALUInfo
    {
        std::shared_ptr<HEVC::NALUnit> m_pNALUnit;
        HEVC::Parser::Info             m_info;
    };

    std::vector<NALUInfo> m_nalus;

    std::map<uint32_t, std::shared_ptr<HEVC::VPS>> m_vpsMap;
    std::map<uint32_t, std::shared_ptr<HEVC::SPS>> m_spsMap;
    std::map<uint32_t, std::shared_ptr<HEVC::PPS>> m_ppsMap;

    int m_selectedIdx = -1;
};

#endif

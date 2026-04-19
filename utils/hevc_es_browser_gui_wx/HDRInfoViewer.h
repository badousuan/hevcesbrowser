#ifndef HDR_INFO_VIEWER_H_
#define HDR_INFO_VIEWER_H_

#include <wx/wx.h>
#include <HevcParser.h>
#include <memory>

class HDRInfoViewer : public wxFrame, public HEVC::Parser::Consumer
{
public:
    HDRInfoViewer(wxWindow* parent);

    void clear();

    virtual void onNALUnit(std::shared_ptr<HEVC::NALUnit> pNALUnit, const HEVC::Parser::Info* pInfo) override;
    virtual void onWarning(const std::string& warning, const HEVC::Parser::Info* pInfo, HEVC::Parser::WarningType) override {}

private:
    void updateList();
    void OnClose(wxCloseEvent& event);

    wxListBox* m_plist;
    std::shared_ptr<HEVC::SPS> m_psps;
    std::shared_ptr<HEVC::MasteringDisplayInfo> m_pMasteringDisplayInfo;
    std::shared_ptr<HEVC::ContentLightLevelInfo> m_pCLLInfo;

    wxDECLARE_EVENT_TABLE();
};

#endif

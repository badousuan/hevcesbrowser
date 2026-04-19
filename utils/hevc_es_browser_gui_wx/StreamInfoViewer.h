#ifndef STREAM_INFO_VIEWER_H_
#define STREAM_INFO_VIEWER_H_

#include <wx/wx.h>
#include <HevcParser.h>

class StreamInfoViewer : public wxFrame, public HEVC::Parser::Consumer
{
public:
    StreamInfoViewer(wxWindow* parent);

    void clear();

    virtual void onNALUnit(std::shared_ptr<HEVC::NALUnit> pNALUnit, const HEVC::Parser::Info* pInfo) override;
    virtual void onWarning(const std::string& warning, const HEVC::Parser::Info* pInfo, HEVC::Parser::WarningType) override {}

private:
    void updateList();
    void OnClose(wxCloseEvent& event);

    wxListBox* m_plist;
    size_t m_nalusNumber;
    size_t m_INumber;
    size_t m_PNumber;
    size_t m_BNumber;
    size_t m_profile;
    size_t m_level;
    size_t m_tier;
    HEVC::Slice::SliceType m_prevSliceType;

    wxDECLARE_EVENT_TABLE();
};

#endif

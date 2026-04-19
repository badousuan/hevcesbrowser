#ifndef WARNINGS_VIEWER_H_
#define WARNINGS_VIEWER_H_

#include <wx/wx.h>
#include <wx/listctrl.h>
#include <vector>
#include <HevcParser.h>

class WarningsViewer : public wxFrame, public HEVC::Parser::Consumer
{
public:
    WarningsViewer(wxWindow* parent);

    void clear();

    virtual void onNALUnit(std::shared_ptr<HEVC::NALUnit> pNALUnit, const HEVC::Parser::Info* pInfo) override;
    virtual void onWarning(const std::string& warning, const HEVC::Parser::Info* pInfo, HEVC::Parser::WarningType) override;

private:
    void OnFilterChanged(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);

    struct Warning
    {
        uint32_t m_position;
        wxString m_message;
        HEVC::Parser::WarningType m_type;
        bool operator<(const Warning& rhs) const { return m_position < rhs.m_position; }
    };

    std::vector<Warning> m_warnings;
    wxListCtrl* m_ptable;
    wxChoice* m_filter;

    void updateTable();
    void readCustomData();
    void saveCustomData();

    wxDECLARE_EVENT_TABLE();
};

#endif

#include "StreamInfoViewer.h"
#include <wx/sizer.h>

wxBEGIN_EVENT_TABLE(StreamInfoViewer, wxFrame)
    EVT_CLOSE(StreamInfoViewer::OnClose)
wxEND_EVENT_TABLE()

StreamInfoViewer::StreamInfoViewer(wxWindow* parent)
    : wxFrame(parent, wxID_ANY, "Stream Info", wxDefaultPosition, wxSize(400, 300)),
      m_nalusNumber(0), m_INumber(0), m_PNumber(0), m_BNumber(0),
      m_profile(0), m_level(0), m_tier(0)
{
    m_plist = new wxListBox(this, wxID_ANY);
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_plist, 1, wxEXPAND);
    SetSizer(sizer);
}

void StreamInfoViewer::clear()
{
    m_nalusNumber = 0; m_INumber = 0; m_PNumber = 0; m_BNumber = 0;
    m_profile = 0; m_level = 0; m_tier = 0;
    m_plist->Clear();
}

void StreamInfoViewer::onNALUnit(std::shared_ptr<HEVC::NALUnit> pNALUnit, const HEVC::Parser::Info* pInfo)
{
    m_nalusNumber++;
    if (pNALUnit->m_nalHeader.type == HEVC::NAL_VPS) {
        auto vps = std::dynamic_pointer_cast<HEVC::VPS>(pNALUnit);
        m_profile = vps->profile_tier_level.general_profile_idc;
        m_level = vps->profile_tier_level.general_level_idc;
        m_tier = vps->profile_tier_level.general_tier_flag;
    }
    // ... count I/P/B slices ...
    updateList();
}

void StreamInfoViewer::updateList()
{
    m_plist->Clear();
    m_plist->Append(wxString::Format("Total NALUs: %zu", m_nalusNumber));
    m_plist->Append(wxString::Format("Profile: %zu", m_profile));
    m_plist->Append(wxString::Format("Level: %zu", m_level));
}

void StreamInfoViewer::OnClose(wxCloseEvent& event)
{
    Hide();
}

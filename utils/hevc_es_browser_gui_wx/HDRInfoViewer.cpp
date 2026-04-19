#include "HDRInfoViewer.h"
#include <wx/sizer.h>

wxBEGIN_EVENT_TABLE(HDRInfoViewer, wxFrame)
    EVT_CLOSE(HDRInfoViewer::OnClose)
wxEND_EVENT_TABLE()

HDRInfoViewer::HDRInfoViewer(wxWindow* parent)
    : wxFrame(parent, wxID_ANY, "HDR Info", wxDefaultPosition, wxSize(400, 300))
{
    m_plist = new wxListBox(this, wxID_ANY);
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_plist, 1, wxEXPAND);
    SetSizer(sizer);
}

void HDRInfoViewer::clear()
{
    m_psps.reset();
    m_pMasteringDisplayInfo.reset();
    m_pCLLInfo.reset();
    m_plist->Clear();
}

void HDRInfoViewer::onNALUnit(std::shared_ptr<HEVC::NALUnit> pNALUnit, const HEVC::Parser::Info* pInfo)
{
    if (pNALUnit->m_nalHeader.type == HEVC::NAL_SPS) {
        m_psps = std::dynamic_pointer_cast<HEVC::SPS>(pNALUnit);
    }
    // ... extract HDR SEIs ...
    updateList();
}

void HDRInfoViewer::updateList()
{
    m_plist->Clear();
    if (m_psps) {
        m_plist->Append(wxString::Format("VUI Present: %d", m_psps->vui_parameters_present_flag));
    }
}

void HDRInfoViewer::OnClose(wxCloseEvent& event)
{
    Hide();
}

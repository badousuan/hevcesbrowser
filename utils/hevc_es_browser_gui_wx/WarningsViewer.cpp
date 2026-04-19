#include "WarningsViewer.h"
#include <wx/sizer.h>
#include <wx/config.h>
#include <algorithm>

wxBEGIN_EVENT_TABLE(WarningsViewer, wxFrame)
    EVT_CLOSE(WarningsViewer::OnClose)
wxEND_EVENT_TABLE()

WarningsViewer::WarningsViewer(wxWindow* parent)
    : wxFrame(parent, wxID_ANY, "Warnings", wxDefaultPosition, wxSize(600, 400))
{
    wxPanel* panel = new wxPanel(this);
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    m_filter = new wxChoice(panel, wxID_ANY);
    m_filter->Append("All");
    m_filter->Append("General");
    m_filter->Append("Profile Conformance");
    m_filter->SetSelection(0);
    m_filter->Bind(wxEVT_CHOICE, &WarningsViewer::OnFilterChanged, this);

    m_ptable = new wxListCtrl(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT);
    m_ptable->AppendColumn("Offset", wxLIST_FORMAT_LEFT, 100);
    m_ptable->AppendColumn("Message", wxLIST_FORMAT_LEFT, 400);

    mainSizer->Add(m_filter, 0, wxEXPAND | wxALL, 5);
    mainSizer->Add(m_ptable, 1, wxEXPAND | wxALL, 5);
    panel->SetSizer(mainSizer);

    readCustomData();
}

void WarningsViewer::clear()
{
    m_warnings.clear();
    m_ptable->DeleteAllItems();
}

void WarningsViewer::onNALUnit(std::shared_ptr<HEVC::NALUnit> pNALUnit, const HEVC::Parser::Info* pInfo)
{
}

void WarningsViewer::onWarning(const std::string& warning, const HEVC::Parser::Info* pInfo, HEVC::Parser::WarningType type)
{
    Warning w;
    w.m_position = pInfo ? pInfo->m_position : 0;
    w.m_message = warning;
    w.m_type = type;
    m_warnings.push_back(w);
    updateTable();
}

void WarningsViewer::OnFilterChanged(wxCommandEvent& event)
{
    updateTable();
}

void WarningsViewer::updateTable()
{
    m_ptable->DeleteAllItems();
    int filterIdx = m_filter->GetSelection();

    for (const auto& w : m_warnings)
    {
        bool show = false;
        if (filterIdx == 0) show = true;
        else if (filterIdx == 1 && w.m_type != HEVC::Parser::PROFILE_CONFORMANCE) show = true;
        else if (filterIdx == 2 && w.m_type == HEVC::Parser::PROFILE_CONFORMANCE) show = true;

        if (show)
        {
            long row = m_ptable->GetItemCount();
            m_ptable->InsertItem(row, wxString::Format("0x%X", w.m_position));
            m_ptable->SetItem(row, 1, w.m_message);
        }
    }
}

void WarningsViewer::OnClose(wxCloseEvent& event)
{
    saveCustomData();
    Hide();
}

void WarningsViewer::saveCustomData()
{
    wxConfig* config = (wxConfig*)wxConfig::Get();
    int w, h; GetSize(&w, &h);
    config->Write("WarningsViewer/width", w);
    config->Write("WarningsViewer/height", h);
}

void WarningsViewer::readCustomData()
{
    wxConfig* config = (wxConfig*)wxConfig::Get();
    int w = 600, h = 400;
    config->Read("WarningsViewer/width", &w);
    config->Read("WarningsViewer/height", &h);
    SetSize(w, h);
}

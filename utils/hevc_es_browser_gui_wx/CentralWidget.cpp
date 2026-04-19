#include "CentralWidget.h"
#include <wx/sizer.h>
#include <wx/config.h>

CentralWidget::CentralWidget(wxWindow* parent)
    : wxPanel(parent)
{
    m_psplitterV = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE);
    m_psplitterH = new wxSplitterWindow(m_psplitterV, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE);

    m_pcomInfoViewer = new CommonInfoViewer(m_psplitterH);
    m_phexViewer = new wxHexView(m_psplitterH);

    m_psplitterH->SplitHorizontally(m_pcomInfoViewer, m_phexViewer);
    m_psplitterH->SetMinimumPaneSize(50);

    m_psyntaxViewer = new SyntaxViewer(m_psplitterV);
    
    m_psplitterV->SplitVertically(m_psplitterH, m_psyntaxViewer);
    m_psplitterV->SetMinimumPaneSize(100);

    wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(m_psplitterV, 1, wxEXPAND);
    SetSizer(sizer);

    // Mediation of events
    m_pcomInfoViewer->Bind(wxEVT_LIST_ITEM_SELECTED, [this](wxListEvent& event) {
        m_pcomInfoViewer->OnItemSelected(event);
        
        // Synchronize parameter sets to SyntaxViewer
        m_psyntaxViewer->setParameretsSets(m_pcomInfoViewer->getVPSMap(), 
                                           m_pcomInfoViewer->getSPSMap(), 
                                           m_pcomInfoViewer->getPPSMap());

        auto info = m_pcomInfoViewer->getSelectedNALUInfo();
        if (info.first) {
            m_psyntaxViewer->onNalUChanged(info.first, info.second);
            m_phexViewer->showFromOffset(info.second.m_position);
        }
    });

    readCustomData();
}

CentralWidget::~CentralWidget()
{
    saveCustomData();
}

void CentralWidget::saveCustomData()
{
    wxConfig* config = (wxConfig*)wxConfig::Get();
    config->Write("CentralWidget/HSplitterPos", m_psplitterH->GetSashPosition());
    config->Write("CentralWidget/VSplitterPos", m_psplitterV->GetSashPosition());
}

void CentralWidget::readCustomData()
{
    wxConfig* config = (wxConfig*)wxConfig::Get();
    int hPos = 300, vPos = 600;
    config->Read("CentralWidget/HSplitterPos", &hPos);
    config->Read("CentralWidget/VSplitterPos", &vPos);
    m_psplitterH->SetSashPosition(hPos);
    m_psplitterV->SetSashPosition(vPos);
}

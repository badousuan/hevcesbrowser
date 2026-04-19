#ifndef CENTRAL_WIDGET_H_
#define CENTRAL_WIDGET_H_

#include <wx/wx.h>
#include <wx/splitter.h>
#include <memory>

#include "CommonInfoViewer.h"
#include "SyntaxViewer.h"
#include "wxHexView.h"

class CentralWidget : public wxPanel
{
public:
    CentralWidget(wxWindow* parent);
    virtual ~CentralWidget();

    CommonInfoViewer* m_pcomInfoViewer;
    SyntaxViewer*     m_psyntaxViewer;
    wxHexView*        m_phexViewer;

    void saveCustomData();

private:
    void readCustomData();
    void OnNaluSelected(wxCommandEvent& event);

    wxSplitterWindow* m_psplitterV;
    wxSplitterWindow* m_psplitterH;
};

#endif

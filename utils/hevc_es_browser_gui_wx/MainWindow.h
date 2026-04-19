#ifndef MAIN_WINDOW_H_
#define MAIN_WINDOW_H_

#include <wx/wx.h>
#include <wx/dnd.h>

class CentralWidget;

class MainWindow : public wxFrame
{
public:
    MainWindow(wxWindow* parent = nullptr);
    virtual ~MainWindow();

    void openFile(const wxString& fileName);

private:
    void OnOpen(wxCommandEvent& event);
    void OnShowWarningsViewer(wxCommandEvent& event);
    void OnShowInfoViewer(wxCommandEvent& event);
    void OnShowHDRInfoViewer(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);

    void process(const wxString& fileName);
    void saveCustomData();
    void readCustomData();

    class FileDropTarget : public wxFileDropTarget
    {
    public:
        FileDropTarget(MainWindow* owner) : m_owner(owner) {}
        virtual bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames) override
        {
            if (filenames.GetCount() > 0)
            {
                m_owner->openFile(filenames[0]);
                return true;
            }
            return false;
        }
    private:
        MainWindow* m_owner;
    };

    wxWindow* m_pwarnViewer;
    wxWindow* m_pinfoViewer;
    wxWindow* m_phdrInfoViewer;
    CentralWidget* m_pcentralWidget;

    wxDECLARE_EVENT_TABLE();
};

enum
{
    ID_OPEN = wxID_OPEN,
    ID_SHOW_WARNINGS = wxID_HIGHEST + 1,
    ID_SHOW_INFO,
    ID_SHOW_HDR_INFO,
    ID_ABOUT = wxID_ABOUT
};

#endif

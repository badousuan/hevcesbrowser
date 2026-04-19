#include "MainWindow.h"
#include "CentralWidget.h"
#include "WarningsViewer.h"
#include "StreamInfoViewer.h"
#include "HDRInfoViewer.h"
#include "ProfileConformanceAnalyzer.h"
#include "CommonInfoViewer.h"
#include "wxHexView.h"

#include <wx/artprov.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/progdlg.h>
#include <wx/config.h>
#include <wx/filename.h>
#include <wx/utils.h>
#include <wx/aboutdlg.h>
#include <wx/stdpaths.h>

#include <HevcParser.h>

// Dummy version if not generated
#ifndef VERSION_STR
#define VERSION_STR "1.0.0"
#endif

wxBEGIN_EVENT_TABLE(MainWindow, wxFrame)
    EVT_MENU(ID_OPEN, MainWindow::OnOpen)
    EVT_MENU(ID_SHOW_WARNINGS, MainWindow::OnShowWarningsViewer)
    EVT_MENU(ID_SHOW_INFO, MainWindow::OnShowInfoViewer)
    EVT_MENU(ID_SHOW_HDR_INFO, MainWindow::OnShowHDRInfoViewer)
    EVT_MENU(ID_ABOUT, MainWindow::OnAbout)
    EVT_CLOSE(MainWindow::OnClose)
wxEND_EVENT_TABLE()

MainWindow::MainWindow(wxWindow* parent)
    : wxFrame(parent, wxID_ANY, "HEVCESBrowser", wxDefaultPosition, wxSize(1024, 768))
{
    // Menu
    wxMenuBar* menuBar = new wxMenuBar();
    wxMenu* fileMenu = new wxMenu();
    fileMenu->Append(ID_OPEN, "&Open...\tCtrl+O");
    fileMenu->AppendSeparator();
    fileMenu->Append(ID_SHOW_WARNINGS, "Warnings...");
    fileMenu->Append(ID_SHOW_INFO, "Info...");
    fileMenu->Append(ID_SHOW_HDR_INFO, "HDR Info...");
    fileMenu->AppendSeparator();
    fileMenu->Append(wxID_EXIT, "E&xit");
    menuBar->Append(fileMenu, "&File");

    wxMenu* helpMenu = new wxMenu();
    helpMenu->Append(ID_ABOUT, "&About HEVCESBrowser...");
    menuBar->Append(helpMenu, "&Help");

    SetMenuBar(menuBar);

    // Toolbar (optional, but original had it)
    wxToolBar* toolBar = CreateToolBar();
    toolBar->AddTool(ID_OPEN, "Open", wxArtProvider::GetBitmap(wxART_FILE_OPEN), "Open HEVC file");
    toolBar->AddTool(ID_SHOW_WARNINGS, "Warnings", wxArtProvider::GetBitmap(wxART_WARNING), "Show Warnings");
    toolBar->AddTool(ID_SHOW_INFO, "Info", wxArtProvider::GetBitmap(wxART_INFORMATION), "Show Stream Info");
    toolBar->AddTool(ID_SHOW_HDR_INFO, "HDR", wxArtProvider::GetBitmap(wxART_REPORT_VIEW), "Show HDR Info");
    toolBar->Realize();

    // Viewers
    m_pwarnViewer = new WarningsViewer(this);
    m_pwarnViewer->Hide();
    m_pinfoViewer = new StreamInfoViewer(this);
    m_pinfoViewer->Hide();
    m_phdrInfoViewer = new HDRInfoViewer(this);
    m_phdrInfoViewer->Hide();

    m_pcentralWidget = new CentralWidget(this);

    SetDropTarget(new FileDropTarget(this));

    readCustomData();
}

MainWindow::~MainWindow()
{
}

void MainWindow::OnOpen(wxCommandEvent& WXUNUSED(event))
{
    wxConfig* config = (wxConfig*)wxConfig::Get();
    wxString prevDir;
    config->Read("MainWindow/PrevDir", &prevDir);

    wxFileDialog openFileDialog(this, "HEVC ES File", prevDir, "", "All files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (openFileDialog.ShowModal() == wxID_CANCEL)
        return;

    openFile(openFileDialog.GetPath());
}

void MainWindow::openFile(const wxString& fileName)
{
    if (fileName.IsEmpty() || !wxFileExists(fileName))
        return;

    wxConfig* config = (wxConfig*)wxConfig::Get();
    
    m_pcentralWidget->m_pcomInfoViewer->clear();
    m_pcentralWidget->m_psyntaxViewer->DeleteAllItems();
    ((WarningsViewer*)m_pwarnViewer)->clear();
    ((StreamInfoViewer*)m_pinfoViewer)->clear();
    ((HDRInfoViewer*)m_phdrInfoViewer)->clear();

    wxString input_fileName = fileName;
    wxString tmp265File = "";

    if (!fileName.Lower().EndsWith(".265") && !fileName.Lower().EndsWith(".h265"))
    {
        tmp265File = fileName + "_output.265";
        wxString cmd = wxString::Format("ffmpeg -i \"%s\" -an -vcodec copy \"%s\" -y", fileName, tmp265File);
        
        wxProgressDialog progressDialog("Loading...", "FFmpeg is converting file to HEVC stream", 100, this, wxPD_APP_MODAL | wxPD_AUTO_HIDE);
        
        long result = wxExecute(cmd, wxEXEC_SYNC);
        if (result != 0)
        {
            wxMessageBox("FFmpeg convert failed", "Error", wxOK | wxICON_ERROR);
            if (wxFileExists(tmp265File)) wxRemoveFile(tmp265File);
            return;
        }
        
        if (!wxFileExists(tmp265File) || wxFileName::GetSize(tmp265File) == 0)
        {
            wxMessageBox("File convert to hevc nal stream failed", "Error", wxOK | wxICON_ERROR);
            if (wxFileExists(tmp265File)) wxRemoveFile(tmp265File);
            return;
        }
        input_fileName = tmp265File;
    }

    process(input_fileName);

    wxFileName fn(fileName);
    config->Write("MainWindow/PrevDir", fn.GetPath());
    SetTitle(fn.GetFullName());

    if (!tmp265File.IsEmpty() && wxFileExists(tmp265File))
    {
        wxRemoveFile(tmp265File);
    }
}

void MainWindow::process(const wxString& fileName)
{
    wxFile file(fileName);
    if (!file.IsOpened())
    {
        wxMessageBox("Problem with open file `" + fileName + "` for reading", "File opening problem", wxOK | wxICON_ERROR);
        return;
    }

    HEVC::Parser* pparser = HEVC::Parser::create();
    pparser->addConsumer(m_pcentralWidget->m_pcomInfoViewer);
    pparser->addConsumer((WarningsViewer*)m_pwarnViewer);
    pparser->addConsumer((StreamInfoViewer*)m_pinfoViewer);
    pparser->addConsumer((HDRInfoViewer*)m_phdrInfoViewer);

    ProfileConformanceAnalyzer profConfAnalyzer;
    profConfAnalyzer.m_pconsumer = (WarningsViewer*)m_pwarnViewer;
    pparser->addConsumer(&profConfAnalyzer);

    size_t position = 0;
    size_t fileSize = file.Length();

    wxProgressDialog progressDialog("Opening...", "Parsing HEVC stream", 100, this, wxPD_APP_MODAL | wxPD_AUTO_HIDE | wxPD_CAN_ABORT);

    const size_t bufferSize = 4 * (1 << 20);
    std::vector<uint8_t> buffer(bufferSize);

    while (!file.Eof())
    {
        ssize_t read = file.Read(buffer.data(), bufferSize);
        if (read <= 0) break;

        size_t parsed = pparser->process(buffer.data(), read, position);
        position += parsed;
        
        file.Seek(position);
        
        int percent = (int)(position * 100 / fileSize);
        if (!progressDialog.Update(percent))
            break; // Cancelled
    }

    // Update last NALU length if needed (simplified from original logic)
    // ...

    file.Seek(0);
    m_pcentralWidget->m_phexViewer->setData(new wxHexView::DataStorageFile(fileName));

    HEVC::Parser::release(pparser);
    file.Close();
}

void MainWindow::OnShowWarningsViewer(wxCommandEvent& WXUNUSED(event))
{
    m_pwarnViewer->Show();
    m_pwarnViewer->Raise();
}

void MainWindow::OnShowInfoViewer(wxCommandEvent& WXUNUSED(event))
{
    m_pinfoViewer->Show();
    m_pinfoViewer->Raise();
}

void MainWindow::OnShowHDRInfoViewer(wxCommandEvent& WXUNUSED(event))
{
    m_phdrInfoViewer->Show();
    m_phdrInfoViewer->Raise();
}

void MainWindow::OnClose(wxCloseEvent& event)
{
    saveCustomData();
    m_pcentralWidget->m_pcomInfoViewer->saveCustomData();

    m_pwarnViewer->Close(true);
    m_pinfoViewer->Close(true);
    m_phdrInfoViewer->Close(true);

    Destroy();
}

void MainWindow::OnAbout(wxCommandEvent& WXUNUSED(event))
{
    wxAboutDialogInfo info;
    info.SetName("HEVCESBrowser");
    info.SetVersion(VERSION_STR);
    info.SetDescription("HEVC Elementary Stream Browser");
    info.SetCopyright("virinext@gmail.com");
    info.AddDeveloper("virinext");
    wxAboutBox(info);
}

void MainWindow::saveCustomData()
{
    wxConfig* config = (wxConfig*)wxConfig::Get();
    int w, h, x, y;
    GetSize(&w, &h);
    GetPosition(&x, &y);
    config->Write("MainWindow/width", w);
    config->Write("MainWindow/height", h);
    config->Write("MainWindow/x", x);
    config->Write("MainWindow/y", y);
}

void MainWindow::readCustomData()
{
    wxConfig* config = (wxConfig*)wxConfig::Get();
    int w = 1024, h = 768, x = -1, y = -1;
    config->Read("MainWindow/width", &w);
    config->Read("MainWindow/height", &h);
    config->Read("MainWindow/x", &x);
    config->Read("MainWindow/y", &y);
    if (x != -1 && y != -1)
        SetSize(x, y, w, h);
    else
        SetSize(w, h);
}

#ifndef WX_HEX_VIEW_H_
#define WX_HEX_VIEW_H_

#include <wx/wx.h>
#include <wx/scrolwin.h>
#include <wx/file.h>
#include <vector>
#include <memory>
#include <string>

class wxHexView : public wxScrolledWindow
{
public:
    class DataStorage
    {
    public:
        virtual ~DataStorage() {}
        virtual std::vector<uint8_t> getData(size_t position, size_t length) = 0;
        virtual size_t size() = 0;
    };

    class DataStorageArray : public DataStorage
    {
    public:
        DataStorageArray(const std::vector<uint8_t>& arr);
        virtual std::vector<uint8_t> getData(size_t position, size_t length) override;
        virtual size_t size() override;
    private:
        std::vector<uint8_t> m_data;
    };

    class DataStorageFile : public DataStorage
    {
    public:
        DataStorageFile(const wxString& fileName);
        virtual std::vector<uint8_t> getData(size_t position, size_t length) override;
        virtual size_t size() override;
    private:
        wxFile m_file;
    };

    wxHexView(wxWindow* parent, wxWindowID id = wxID_ANY);
    virtual ~wxHexView();

    void setData(DataStorage* pData);
    void clear();
    void showFromOffset(size_t offset);

protected:
    void OnPaint(wxPaintEvent& event);
    void OnKeyDown(wxKeyEvent& event);
    void OnLeftDown(wxMouseEvent& event);
    void OnLeftUp(wxMouseEvent& event);
    void OnMouseMove(wxMouseEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnMouseCaptureLost(wxMouseCaptureLostEvent& event);

private:
    DataStorage* m_pdata;
    size_t m_posAddr;
    size_t m_posHex;
    size_t m_posAscii;
    int m_charWidth;
    int m_charHeight;

    size_t m_selectBegin;
    size_t m_selectEnd;
    size_t m_selectInit;
    size_t m_cursorPos;

    wxSize GetFullSize() const;
    void ResetSelection();
    void ResetSelection(int pos);
    void SetSelection(int pos);
    void EnsureVisible();
    void SetCursorPos(int pos);
    int GetCursorPosFromMouse(const wxPoint& position);
    void UpdateScrollbars();

    wxDECLARE_EVENT_TABLE();
};

#endif

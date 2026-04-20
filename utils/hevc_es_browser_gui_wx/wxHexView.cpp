#include "wxHexView.h"
#include <wx/dcclient.h>
#include <wx/dcbuffer.h>
#include <wx/msgdlg.h>
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/settings.h>

const int HEXCHARS_IN_LINE = 47;
const int GAP_ADR_HEX = 10;
const int GAP_HEX_ASCII = 16;
const int BYTES_PER_LINE = 16;

wxBEGIN_EVENT_TABLE(wxHexView, wxScrolledWindow)
    EVT_PAINT(wxHexView::OnPaint)
    EVT_KEY_DOWN(wxHexView::OnKeyDown)
    EVT_LEFT_DOWN(wxHexView::OnLeftDown)
    EVT_LEFT_UP(wxHexView::OnLeftUp)
    EVT_MOTION(wxHexView::OnMouseMove)
    EVT_SIZE(wxHexView::OnSize)
    EVT_MOUSE_CAPTURE_LOST(wxHexView::OnMouseCaptureLost)
wxEND_EVENT_TABLE()

wxHexView::wxHexView(wxWindow* parent, wxWindowID id)
    : wxScrolledWindow(parent, id, wxDefaultPosition, wxDefaultSize, wxVSCROLL | wxHSCROLL | wxWANTS_CHARS),
      m_pdata(nullptr),
      m_selectBegin(0),
      m_selectEnd(0),
      m_selectInit(0),
      m_cursorPos(0)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    wxFont font(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    SetFont(font);

    wxClientDC dc(this);
    dc.SetFont(font);
    m_charWidth = dc.GetCharWidth();
    m_charHeight = dc.GetCharHeight();

    m_posAddr = 0;
    m_posHex = 10 * m_charWidth + GAP_ADR_HEX;
    m_posAscii = m_posHex + HEXCHARS_IN_LINE * m_charWidth + GAP_HEX_ASCII;

    SetMinSize(wxSize(m_posAscii + (BYTES_PER_LINE * m_charWidth), -1));
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
}

wxHexView::~wxHexView()
{
    delete m_pdata;
}

void wxHexView::setData(DataStorage* pData)
{
    delete m_pdata;
    m_pdata = pData;
    m_cursorPos = 0;
    ResetSelection(0);
    UpdateScrollbars();
    Refresh();
}

void wxHexView::clear()
{
    setData(nullptr);
}

void wxHexView::showFromOffset(size_t offset)
{
    if (m_pdata && offset < m_pdata->size())
    {
        SetCursorPos(offset * 2);
        EnsureVisible();
        Refresh();
    }
}

wxSize wxHexView::GetFullSize() const
{
    if (!m_pdata)
        return wxSize(0, 0);

    int width = m_posAscii + (BYTES_PER_LINE * m_charWidth);
    int height = (m_pdata->size() / BYTES_PER_LINE);
    if (m_pdata->size() % BYTES_PER_LINE)
        height++;

    height *= m_charHeight;
    return wxSize(width, height);
}

void wxHexView::UpdateScrollbars()
{
    wxSize full = GetFullSize();
    SetScrollbars(m_charWidth, m_charHeight, full.x / m_charWidth, full.y / m_charHeight);
}

void wxHexView::OnPaint(wxPaintEvent& WXUNUSED(event))
{
    wxAutoBufferedPaintDC dc(this);
    PrepareDC(dc);

    dc.SetBackground(wxBrush(GetBackgroundColour()));
    dc.Clear();

    if (!m_pdata)
        return;

    wxRect rect = GetUpdateRegion().GetBox();
    CalcUnscrolledPosition(rect.x, rect.y, &rect.x, &rect.y);

    int viewStart_x, viewStart_y;
    GetViewStart(&viewStart_x, &viewStart_y);
    int firstLineIdx = viewStart_y;
    
    int clientW, clientH;
    GetClientSize(&clientW, &clientH);
    int linesInView = clientH / m_charHeight + 2;

    int lastLineIdx = firstLineIdx + linesInView;
    size_t totalLines = m_pdata->size() / BYTES_PER_LINE;
    if (m_pdata->size() % BYTES_PER_LINE) totalLines++;
    if (lastLineIdx > (int)totalLines) lastLineIdx = totalLines;

    // Draw address area background
    wxColour addressAreaColor(0xd4, 0xd4, 0xd4);
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBrush(wxBrush(addressAreaColor));
    dc.DrawRectangle(m_posAddr, firstLineIdx * m_charHeight, m_posHex - GAP_ADR_HEX + 2, (lastLineIdx - firstLineIdx) * m_charHeight);

    // Draw separator line
    int linePos = m_posAscii - (GAP_HEX_ASCII / 2);
    dc.SetPen(wxPen(*wxGREY_PEN));
    dc.DrawLine(linePos, firstLineIdx * m_charHeight, linePos, lastLineIdx * m_charHeight);

    dc.SetTextForeground(*wxBLACK);
    wxBrush selectedBrush(wxColour(0x6d, 0x9e, 0xff));
    wxBrush defaultBrush(GetBackgroundColour());

    std::vector<uint8_t> data = m_pdata->getData(firstLineIdx * BYTES_PER_LINE, (lastLineIdx - firstLineIdx) * BYTES_PER_LINE);

    for (int lineIdx = firstLineIdx; lineIdx < lastLineIdx; ++lineIdx)
    {
        int yPos = lineIdx * m_charHeight;
        wxString address = wxString::Format("%010llX", (unsigned long long)(lineIdx * 16));
        dc.DrawText(address, m_posAddr, yPos);

        for (int i = 0; i < BYTES_PER_LINE; ++i)
        {
            size_t dataIdx = (lineIdx - firstLineIdx) * BYTES_PER_LINE + i;
            if (dataIdx >= data.size()) break;

            size_t pos = (lineIdx * BYTES_PER_LINE + i) * 2;
            int xPos = m_posHex + i * 3 * m_charWidth;

            // Hex part
            for (int bit = 0; bit < 2; ++bit)
            {
                size_t subPos = pos + bit;
                if (subPos >= m_selectBegin && subPos < m_selectEnd)
                {
                    dc.SetBrush(selectedBrush);
                    dc.SetTextBackground(selectedBrush.GetColour());
                    dc.SetBackgroundMode(wxBRUSHSTYLE_SOLID);
                }
                else
                {
                    dc.SetBrush(defaultBrush);
                    dc.SetTextBackground(GetBackgroundColour());
                    dc.SetBackgroundMode(wxBRUSHSTYLE_TRANSPARENT);
                }

                uint8_t val = (bit == 0) ? (data[dataIdx] >> 4) : (data[dataIdx] & 0x0F);
                dc.DrawText(wxString::Format("%X", val), xPos + bit * m_charWidth, yPos);
            }

            // Ascii part
            int xPosAscii = m_posAscii + i * m_charWidth;
            char ch = data[dataIdx];
            if (ch < 0x20 || ch > 0x7e) ch = '.';
            
            dc.SetBackgroundMode(wxBRUSHSTYLE_TRANSPARENT);
            dc.DrawText(wxString(ch), xPosAscii, yPos);
        }
    }

    if (HasFocus())
    {
        int x = (m_cursorPos % (2 * BYTES_PER_LINE));
        int y = m_cursorPos / (2 * BYTES_PER_LINE);
        int cursorX = (((x / 2) * 3) + (x % 2)) * m_charWidth + m_posHex;
        int cursorY = y * m_charHeight;
        dc.SetPen(*wxBLACK_PEN);
        dc.DrawLine(cursorX, cursorY, cursorX, cursorY + m_charHeight);
    }
}

void wxHexView::OnKeyDown(wxKeyEvent& event)
{
    bool setVisible = false;
    int keyCode = event.GetKeyCode();
    bool shift = event.ShiftDown();

    int prevPos = m_cursorPos;
    int nextPos = m_cursorPos;

    if (keyCode == WXK_RIGHT) nextPos++;
    else if (keyCode == WXK_LEFT) nextPos--;
    else if (keyCode == WXK_UP) nextPos -= BYTES_PER_LINE * 2;
    else if (keyCode == WXK_DOWN) nextPos += BYTES_PER_LINE * 2;
    else if (keyCode == WXK_PAGEUP) {
        int clientH; GetClientSize(nullptr, &clientH);
        nextPos -= (clientH / m_charHeight - 1) * 2 * BYTES_PER_LINE;
    }
    else if (keyCode == WXK_PAGEDOWN) {
        int clientH; GetClientSize(nullptr, &clientH);
        nextPos += (clientH / m_charHeight - 1) * 2 * BYTES_PER_LINE;
    }
    else if (keyCode == WXK_HOME) nextPos = m_cursorPos - (m_cursorPos % (2 * BYTES_PER_LINE));
    else if (keyCode == WXK_END) nextPos = m_cursorPos - (m_cursorPos % (2 * BYTES_PER_LINE)) + (2 * BYTES_PER_LINE - 1);
    else if (event.GetModifiers() == wxMOD_CONTROL && keyCode == 'A')
    {
        ResetSelection(0);
        if (m_pdata) SetSelection(2 * m_pdata->size());
        Refresh();
        return;
    }
    else if (event.GetModifiers() == wxMOD_CONTROL && keyCode == 'C')
    {
        if (m_pdata && m_selectEnd > m_selectBegin)
        {
            wxString res;
            std::vector<uint8_t> data = m_pdata->getData(m_selectBegin / 2, (m_selectEnd - m_selectBegin) / 2 + 1);
            for (size_t i = 0; i < (m_selectEnd - m_selectBegin); i += 2)
            {
                res += wxString::Format("%02X ", data[i/2]);
                if ((i/2) % BYTES_PER_LINE == (BYTES_PER_LINE - 1)) res += "\n";
            }
            if (wxTheClipboard->Open())
            {
                wxTheClipboard->SetData(new wxTextDataObject(res));
                wxTheClipboard->Close();
            }
        }
        return;
    }
    else {
        event.Skip();
        return;
    }

    SetCursorPos(nextPos);
    if (shift) SetSelection(m_cursorPos);
    else ResetSelection(m_cursorPos);
    
    EnsureVisible();
    Refresh();
}

void wxHexView::OnLeftDown(wxMouseEvent& event)
{
    SetFocus();
    wxPoint pos = event.GetPosition();
    CalcUnscrolledPosition(pos.x, pos.y, &pos.x, &pos.y);
    int cPos = GetCursorPosFromMouse(pos);

    if (cPos != -1)
    {
        if (event.ShiftDown()) SetSelection(cPos);
        else ResetSelection(cPos);
        SetCursorPos(cPos);
        CaptureMouse();
    }
    Refresh();
}

void wxHexView::OnLeftUp(wxMouseEvent& event)
{
    if (HasCapture())
    {
        ReleaseMouse();
    }
}

void wxHexView::OnMouseMove(wxMouseEvent& event)
{
    if (event.Dragging() && event.LeftIsDown() && HasCapture())
    {
        wxPoint pos = event.GetPosition();
        CalcUnscrolledPosition(pos.x, pos.y, &pos.x, &pos.y);
        int actPos = GetCursorPosFromMouse(pos);
        if (actPos != -1)
        {
            SetCursorPos(actPos);
            SetSelection(actPos);
            EnsureVisible();
            Refresh();
        }
    }
}

void wxHexView::OnSize(wxSizeEvent& event)
{
    UpdateScrollbars();
    event.Skip();
}

void wxHexView::OnMouseCaptureLost(wxMouseCaptureLostEvent& WXUNUSED(event))
{
}

int wxHexView::GetCursorPosFromMouse(const wxPoint& position)
{
    if (position.x >= (int)m_posHex && position.x < (int)(m_posHex + HEXCHARS_IN_LINE * m_charWidth))
    {
        int x = (position.x - m_posHex) / m_charWidth;
        if ((x % 3) == 0) x = (x / 3) * 2;
        else x = ((x / 3) * 2) + 1;

        int y = (position.y / m_charHeight) * 2 * BYTES_PER_LINE;
        return x + y;
    }
    return -1;
}

void wxHexView::ResetSelection() { m_selectBegin = m_selectEnd = m_selectInit; }

void wxHexView::ResetSelection(int pos)
{
    if (pos < 0) pos = 0;
    m_selectInit = m_selectBegin = m_selectEnd = pos;
}

void wxHexView::SetSelection(int pos)
{
    if (pos < 0) pos = 0;
    if (pos >= (int)m_selectInit) { m_selectEnd = pos; m_selectBegin = m_selectInit; }
    else { m_selectBegin = pos; m_selectEnd = m_selectInit; }
}

void wxHexView::SetCursorPos(int position)
{
    if (position < 0) position = 0;
    size_t maxPos = m_pdata ? m_pdata->size() * 2 : 0;
    if (position > (int)maxPos) position = maxPos;
    m_cursorPos = position;
}

void wxHexView::EnsureVisible()
{
    int viewStart_x, viewStart_y;
    GetViewStart(&viewStart_x, &viewStart_y);
    int clientW, clientH;
    GetClientSize(&clientW, &clientH);
    
    int cursorY = m_cursorPos / (2 * BYTES_PER_LINE);
    if (cursorY < viewStart_y) Scroll(-1, cursorY);
    else if (cursorY >= viewStart_y + clientH / m_charHeight) Scroll(-1, cursorY - clientH / m_charHeight + 1);
}

// DataStorage implementations
wxHexView::DataStorageArray::DataStorageArray(const std::vector<uint8_t>& arr) : m_data(arr) {}
std::vector<uint8_t> wxHexView::DataStorageArray::getData(size_t position, size_t length)
{
    if (position >= m_data.size()) return {};
    size_t actualLength = std::min(length, m_data.size() - position);
    return std::vector<uint8_t>(m_data.begin() + position, m_data.begin() + position + actualLength);
}
size_t wxHexView::DataStorageArray::size() { return m_data.size(); }

wxHexView::DataStorageFile::DataStorageFile(const wxString& fileName) { m_file.Open(fileName); }
std::vector<uint8_t> wxHexView::DataStorageFile::getData(size_t position, size_t length)
{
    if (!m_file.IsOpened()) return {};
    m_file.Seek(position);
    std::vector<uint8_t> buffer(length);
    ssize_t read = m_file.Read(buffer.data(), length);
    if (read < (ssize_t)length) buffer.resize(std::max((ssize_t)0, read));
    return buffer;
}
size_t wxHexView::DataStorageFile::size() { return m_file.IsOpened() ? m_file.Length() : 0; }

#include <wx/wx.h>
#include "MainWindow.h"

class MyApp : public wxApp
{
public:
    virtual bool OnInit()
    {
        MainWindow* frame = new MainWindow(nullptr);
        frame->Show(true);

        if (argc >= 2) {
            frame->openFile(argv[1]);
        }

        return true;
    }
};

wxIMPLEMENT_APP(MyApp);

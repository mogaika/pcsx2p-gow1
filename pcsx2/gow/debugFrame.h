#pragma once

#include "wx/frame.h"
#include "wx/panel.h"
#include "wx/notebook.h"

#include <map>

namespace gow {
		
struct stStackAllocatorInfo {
    u32 begin, size;
    char name[16];
	u32 parent, level;
};

class DebugMemoryMap;

class DebugFrame : public wxFrame {
protected:
    wxNotebook *notebook;
	DebugMemoryMap *tabMemmap;

public:
    DebugFrame(wxString title);
	DebugMemoryMap *GetMemoryMap() { return tabMemmap; }
};

class DebugMemoryMap : public wxPanel {
protected:
    std::map<u32, stStackAllocatorInfo> allocators;

public:
    DebugMemoryMap(wxWindow *parent);

    void eventPaint(wxPaintEvent &evt);

    void AddAllocator(u32 begin, u32 size, char *name);
    void RemoveAllocator(u32 begin);
    DECLARE_EVENT_TABLE()
};

} // namespace gow

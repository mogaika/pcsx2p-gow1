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
class DebugTextures;

class DebugFrame : public wxFrame {
protected:
    wxNotebook *notebook;
	DebugMemoryMap *tabMemmap;
    DebugTextures *tabTextures;

public:
    DebugFrame(wxString title);
	DebugMemoryMap *GetMemoryMap() { return tabMemmap; }
    DebugTextures *GetTextures() { return tabTextures; }
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

class DebugTextures : public wxPanel {
protected:
    wxListBox *texturesList;
    wxButton *buttonPreview;
    wxButton *buttonHidePreview;
    wxStaticText *staticInfo;

	void updateStaticText();
public:
	DebugTextures(wxWindow *parent);

    void OnButtonPreview(wxCommandEvent &event);
    void OnButtonHidePreview(wxCommandEvent &event);

    void OnLoadedTexture(u32 offset, char *name);
    void OnUnLoadedTexture(u32 offset);
};

} // namespace gow

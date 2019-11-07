#pragma once

#include "wx/frame.h"
#include "wx/panel.h"
#include "wx/notebook.h"
#include "wx/textctrl.h"
#include "wx/treectrl.h"

#include <map>

namespace gow {
		
struct stStackAllocatorInfo {
    u32 begin, size;
    char name[16];
	u32 parent, level;
};

class DebugMemoryMap;
class DebugTextures;
class DebugRenderer;
class DebugWadEvents;

class DebugFrame : public wxFrame {
protected:
    wxNotebook *notebook;
	DebugMemoryMap *tabMemmap;
    DebugTextures *tabTextures;
	DebugRenderer *tabRenderer;
	DebugWadEvents *tabWadEvents;

public:
    DebugFrame(wxString title);
	DebugMemoryMap &GetMemoryMap() { return *tabMemmap; }
    DebugTextures &GetTextures() { return *tabTextures; }
    DebugWadEvents &GetWadEvents() { return *tabWadEvents; }
};

class DebugMemoryMap : public wxPanel {
protected:
    std::map<u32, stStackAllocatorInfo> allocators;

public:
    DebugMemoryMap(wxWindow *parent);

    void eventPaint(wxPaintEvent &evt);
	void eventSize(wxSizeEvent &evt);

    void AddAllocator(u32 begin, u32 size, char *name);
    void RemoveAllocator(u32 begin);
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

class DebugRenderer : public wxPanel {
protected:
	wxButton *buttonReloadShaders;
    wxCheckBox *checkboxBlueClearColor;
    wxButton *buttonDumpFrame;

public:
    DebugRenderer(wxWindow *parent);

    void OnCheckboxClueClearColor(wxCommandEvent &event);
    void OnButtonReloadShaders(wxCommandEvent &event);
    void OnButtonDumpFrame(wxCommandEvent &event);
};

class DebugWadEvents : public wxPanel {
protected:
	wxListBox *eventsList;

public:
	DebugWadEvents(wxWindow *parent);
	
	void OnNewEvent(u16 eventId, u16 param1, u32 param2, char *name);
};

} // namespace gow

#include "PrecompiledHeader.h"

#include "gow/debugFrame.h"

#include "wx/sizer.h"

using namespace gow;

DebugFrame::DebugFrame(wxString title):
	wxFrame(nullptr, wxID_ANY, title) {
    
	wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    notebook = new wxNotebook(this, wxID_ANY, wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), wxBK_DEFAULT);
    this->SetSizer(sizer);
    sizer->Add(notebook, 1, wxEXPAND);

	tabMemmap = new DebugMemoryMap(notebook);
    notebook->AddPage(tabMemmap, _("Memmap"), false);

	SetName(wxT("GoW debug"));
    SetSize(600, 400);
}

BEGIN_EVENT_TABLE(DebugMemoryMap, wxPanel)
EVT_PAINT(DebugMemoryMap::eventPaint)
END_EVENT_TABLE()

DebugMemoryMap::DebugMemoryMap(wxWindow *parent) : wxPanel(parent) {
    SetWindowStyle(GetWindowStyle() | wxFULL_REPAINT_ON_RESIZE);
}

void DebugMemoryMap::AddAllocator(u32 begin, u32 size, char *name) {
    stStackAllocatorInfo allocator;
	allocator.begin = begin;
	allocator.size = size;
	strcpy(allocator.name, name);

	// search for parent allocator
	// its start must be closer to start
	// its end must be > begin+size
	stStackAllocatorInfo *parent = nullptr;
	for (auto i = allocators.begin(); i != allocators.end(); ++i) {
        if (i->second.begin < begin && i->second.begin + i->second.size > begin + size) {
			if (!parent || i->second.begin > parent->begin) {
				parent = &i->second;
			}
		}
	}
	allocator.parent = parent ? parent->begin : 0;
    allocator.level = 0;
    for (auto c = allocator; c.parent; c = allocators[c.parent]) { allocator.level++; }
	allocators[begin] = allocator;
}

void DebugMemoryMap::RemoveAllocator(u32 begin) {
	allocators.erase(begin);
}

void DebugMemoryMap::eventPaint(wxPaintEvent &evt) {
    wxPaintDC dc(this);

	auto size = GetSize();
    float stepPerKbyte = float(size.GetWidth()) / float((32 * 1024));

    for (auto k = allocators.begin(); k != allocators.end(); ++k) {
        auto a = k->second;

		wxCoord xstart = (a.begin / 1024) * stepPerKbyte;
        wxCoord xsize = (a.size / 1024) * stepPerKbyte;

		switch (a.level) {
            case 0: dc.SetBrush(*wxGREEN_BRUSH); break;
            case 1: dc.SetBrush(*wxYELLOW_BRUSH); break;
            case 2: dc.SetBrush(*wxWHITE_BRUSH); break;
            case 3: dc.SetBrush(*wxRED_BRUSH); break;
            default:  dc.SetBrush(*wxCYAN_BRUSH); break;
        }
        dc.DrawRectangle(xstart, a.level * 70, xsize, size.GetHeight());
    }

	dc.SetBrush(*wxBLACK_BRUSH);
    for (auto k = allocators.begin(); k != allocators.end(); ++k) {
        auto a = k->second;
        wxCoord xstart = (a.begin / 1024) * stepPerKbyte;
        dc.DrawRotatedText(wxString::Format("%s", a.name), xstart + 17, a.level * 80, -90.0);
    }

	// dc.SetBrush(*wxBLUE_BRUSH);                   // blue filling
    // dc.SetPen(wxPen(wxColor(255, 175, 175), 10)); // 10-pixels-thick pink outline
    // dc.DrawRectangle(300, 100, 400, 200);
    //
}

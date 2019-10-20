#include "PrecompiledHeader.h"

#include "gow/debugFrame.h"
#include "gow/gow.h"

#include "wx/sizer.h"

using namespace gow;

DebugFrame::DebugFrame(wxString title):
	wxFrame(nullptr, wxID_ANY, title) {
    
	wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(sizer);

    notebook = new wxNotebook(this, wxID_ANY, wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), wxBK_DEFAULT);
    sizer->Add(notebook, 1, wxEXPAND);

	tabMemmap = new DebugMemoryMap(notebook);
    notebook->AddPage(tabMemmap, _("Memmap"), false);

	tabTextures = new DebugTextures(notebook);
    notebook->AddPage(tabTextures, _("Textures"), false);

	SetName(wxT("GoW debug"));
    SetSize(600, 400);
}

BEGIN_EVENT_TABLE(DebugMemoryMap, wxPanel)
EVT_PAINT(DebugMemoryMap::eventPaint)
END_EVENT_TABLE()

DebugMemoryMap::DebugMemoryMap(wxWindow *parent):
	wxPanel(parent) {
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
}

void gow::DebugTextures::updateStaticText() {
	staticInfo->SetLabelText(wxString::Format("Total textures: %d", texturesList->GetCount()));
}

DebugTextures::DebugTextures(wxWindow *parent)
    :
	wxPanel(parent) {

	wxBoxSizer *sizerVertical = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(sizerVertical);

	texturesList = new wxListBox(this, wxID_ANY);
    texturesList->SetWindowStyle(texturesList->GetWindowStyle() | wxLB_SINGLE);
    texturesList->Bind(wxEVT_LISTBOX_DCLICK, &DebugTextures::OnButtonPreview, this);
	sizerVertical->Add(texturesList, 1, wxEXPAND, 5);

	wxBoxSizer *sizerBottomButtons = new wxBoxSizer(wxHORIZONTAL);
    sizerVertical->Add(sizerBottomButtons, 0, wxEXPAND, 5);

	buttonPreview = new wxButton(this, wxID_ANY, _("Preview texture"));
	buttonPreview->Bind(wxEVT_BUTTON, &DebugTextures::OnButtonPreview, this);
    sizerBottomButtons->Add(buttonPreview, 0, wxALL, 5);

    buttonHidePreview = new wxButton(this, wxID_ANY, _("Hide preview"));
    buttonHidePreview->Bind(wxEVT_BUTTON, &DebugTextures::OnButtonHidePreview, this);
    sizerBottomButtons->Add(buttonHidePreview, 0, wxALL, 5);

	staticInfo = new wxStaticText(this, wxID_ANY, _(""));
    sizerBottomButtons->Add(staticInfo, 1, wxALL | wxALIGN_CENTER, 5);
}

void DebugTextures::OnButtonPreview(wxCommandEvent &event) {
	auto index = texturesList->GetSelection();
	if (index == wxNOT_FOUND) {
		return;
	}

    core->Renderer()->PreviewTextureQuad((u32)texturesList->GetClientData(index));
}

void DebugTextures::OnButtonHidePreview(wxCommandEvent &event) {
	core->Renderer()->PreviewTextureQuad(0);
}

void DebugTextures::OnLoadedTexture(u32 offset, char *name) {
    auto index = texturesList->Append(wxString::Format("%.8x %s", offset, name), (void*)offset);
    texturesList->EnsureVisible(index);
    updateStaticText();
}

void DebugTextures::OnUnLoadedTexture(u32 offset) {
	for (unsigned int i = 0; i < texturesList->GetCount(); i++) {
        if ((u32)texturesList->GetClientData(i) == offset) {
			texturesList->Delete(i);
            updateStaticText();
			return;
		}
	}
}

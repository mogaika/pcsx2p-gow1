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
    tabTextures = new DebugTextures(notebook);
	tabRenderer = new DebugRenderer(notebook);
    tabWadEvents = new DebugWadEvents(notebook);

    notebook->AddPage(tabMemmap, _("Memmap"), false);
	notebook->AddPage(tabTextures, _("Textures"), false);
	notebook->AddPage(tabRenderer, _("Renderer"), false);
	notebook->AddPage(tabWadEvents, _("WadEvents"), false);

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

DebugTextures::DebugTextures(wxWindow *parent) :
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
    sizerBottomButtons->Add(staticInfo, 0, wxALL | wxALIGN_CENTER, 5);
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

DebugRenderer::DebugRenderer(wxWindow *parent):
	wxPanel(parent) {
    wxBoxSizer *sizerVertical = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(sizerVertical);

    buttonReloadShaders = new wxButton(this, wxID_ANY, _("Reload shaders"));
    buttonReloadShaders->Bind(wxEVT_BUTTON, &DebugRenderer::OnButtonReloadShaders, this);
    sizerVertical->Add(buttonReloadShaders, 0, wxALL, 5);

    textSize1 = new wxTextCtrl(this, wxID_ANY, _("1,0"));
    textSize1->SetWindowStyle(textSize1->GetWindowStyle() | wxTE_PROCESS_ENTER);
    textSize1->Bind(wxEVT_COMMAND_TEXT_ENTER, &DebugRenderer::OnTextSize1Change, this);
    sizerVertical->Add(textSize1, 0, wxALL, 2);

    textSize2 = new wxTextCtrl(this, wxID_ANY, _("0,001953125"));
    textSize2->SetWindowStyle(textSize2->GetWindowStyle() | wxTE_PROCESS_ENTER);
    textSize2->Bind(wxEVT_COMMAND_TEXT_ENTER, &DebugRenderer::OnTextSize2Change, this);
    sizerVertical->Add(textSize2, 0, wxALL, 2);

	buttonDumpFrame = new wxButton(this, wxID_ANY, _("Dump frame"));
    buttonDumpFrame->Bind(wxEVT_BUTTON, &DebugRenderer::OnButtonDumpFrame, this);
    sizerVertical->Add(buttonDumpFrame, 0, wxALL, 5);
}

void DebugRenderer::OnTextSize1Change(wxCommandEvent &event) {
    double v;
    if (textSize1->GetValue().ToDouble(&v)) {
        DevCon.WriteLn("changing size1 to %f", v);
        core->Renderer()->size1 = v;
    } else {
        DevCon.WriteLn("invalid floating %s", textSize1->GetValue());
    }
}

void DebugRenderer::OnTextSize2Change(wxCommandEvent &event) {
    double v;
    if (textSize2->GetValue().ToDouble(&v)) {
        DevCon.WriteLn("changing size2 to %f", v);
        core->Renderer()->size2 = v;
    } else {
        DevCon.WriteLn("invalid floating %s", textSize2->GetValue());
    }
}

void DebugRenderer::OnButtonReloadShaders(wxCommandEvent &event) {
    core->Renderer()->ReloadShaders();
}

void DebugRenderer::OnButtonDumpFrame(wxCommandEvent &event) {
    core->Renderer()->DumpFrame();
}

DebugWadEvents::DebugWadEvents(wxWindow *parent):
	wxPanel(parent) {
    wxBoxSizer *sizerVertical = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(sizerVertical);

	eventsList = new wxListBox(this, wxID_ANY);
    sizerVertical->Add(eventsList, 1, wxALL | wxEXPAND);
}

void DebugWadEvents::OnNewEvent(u16 eventId, u16 param1, u32 param2, char *name) {
	char *e = "";
	switch (eventId) {
        case 1: e = "unload scene wad"; break;
        case 3: e = "load scene wad"; break;
		case 5: e = "unload hero wad"; break;
        case 6: e = "load hero wad"; break;
		case 8: e = "unload slot wad"; break;
        case 9: e = "load slot wad"; break;
		case 0xb: e = "unload weapon/skill wad"; break;
        case 0xc: e = "load weapon/skill wad"; break;
		case 0xe: e = "discard scene wad"; break;
		case 0x10: e = "restart scene wad"; break;
        case 0x11: e = "load shell wad"; break;
        case 0x13: e = "load savept wad"; break;
	}
    eventsList->Insert(wxString::Format("f:%.8x id:%.4x p1:%.4x p2:%.8x %-16s %s",
		offsets::uFrameCounter, eventId, param1, param2, name, e), eventsList->GetCount());
}

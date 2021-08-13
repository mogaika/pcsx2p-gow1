#include "PrecompiledHeader.h"

#include "gow/debugFrame.h"
#include "gow/gow.h"
#include "gow/hooker.h"


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

DebugMemoryMap::DebugMemoryMap(wxWindow *parent):
	wxPanel(parent) {
	Bind(wxEVT_PAINT, &DebugMemoryMap::eventPaint, this);
	Bind(wxEVT_SIZE, &DebugMemoryMap::eventSize, this);
}

void DebugMemoryMap::AddAllocator(u32 begin, u32 size, char *name) {
    stStackAllocatorInfo allocator;
	allocator.begin = begin;
	allocator.size = size;
	strcpy(allocator.name, name);

	// search for parent allocator
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

void DebugMemoryMap::eventSize(wxSizeEvent &evt) {
	Refresh();
	evt.Skip();
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

	core->Renderer()->TexturePreview.SetTextureTXR((u32) texturesList->GetClientData(index), 0);
}

void DebugTextures::OnButtonHidePreview(wxCommandEvent &event) {
	core->Renderer()->TexturePreview.SetTextureGL(0);
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

	buttonDumpFrame = new wxButton(this, wxID_ANY, _("Dump frame"));
    buttonDumpFrame->Bind(wxEVT_BUTTON, &DebugRenderer::OnButtonDumpFrame, this);
    sizerVertical->Add(buttonDumpFrame, 0, wxALL, 5);

	labelHashesCount = new wxStaticText(this, wxID_ANY, _(""));
    sizerVertical->Add(labelHashesCount, 0, wxALL, 5);

	buttonDumpHashes = new wxButton(this, wxID_ANY, _("Dump hashes"));
	buttonDumpHashes->Bind(wxEVT_BUTTON, &DebugRenderer::OnButtonDumpHashes, this);
	sizerVertical->Add(buttonDumpHashes, 0, wxALL, 5);

	checkboxBlueClearColor = new wxCheckBox(this, wxID_ANY, _("Use blue clear color"));
	checkboxBlueClearColor->Bind(wxEVT_CHECKBOX, &DebugRenderer::OnCheckboxClueClearColor, this);
	sizerVertical->Add(checkboxBlueClearColor, 0, wxALL, 5);
}

void DebugRenderer::OnButtonReloadShaders(wxCommandEvent &event) {
    core->Renderer()->ReloadShaders();
}

void DebugRenderer::OnButtonDumpFrame(wxCommandEvent &event) {
    core->Renderer()->DumpFrame();
}

void DebugRenderer::OnButtonDumpHashes(wxCommandEvent &event) {
	auto f = fopen("hashes.dump.txt", "wb");
	for (auto i = hooker->hashesMap.begin(); i != hooker->hashesMap.end(); ++i) {
		fprintf(f, "%.8x:%.8x:%s\n", i->first.first, i->first.second, i->second.c_str());
	}
	fclose(f);
}

void DebugRenderer::OnCheckboxClueClearColor(wxCommandEvent &event) {
	core->Renderer()->Master.blueClearColor = checkboxBlueClearColor->IsChecked();
}

void DebugRenderer::UpdateDumpHashesCount(int count) {
    labelHashesCount->SetLabelText(wxString::Format("hashes: %d", count));
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


DebugServers::DebugServers(wxWindow *parent) :
	wxPanel(parent) {
	wxBoxSizer *sizerVertical = new wxBoxSizer(wxVERTICAL);
	this->SetSizer(sizerVertical);

	wxTreeCtrl *treeServers = new wxTreeCtrl(this, wxID_ANY);
	sizerVertical->Add(treeServers, 1, wxALL | wxEXPAND);
}

void DebugServers::UpdateServerList() {
	treeServers->DeleteAllItems();

	u32 pRootServer = rmem<u32>(0x32E848);



}

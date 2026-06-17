#include "PluginDefinition.h"

#include "Scintilla.h"
#include "menuCmdID.h"

#include "DelimiterNormalizer.h"
#include "DictionaryCache.h"
#include "FieldRenderer.h"
#include "FixParser.h"
#include "MessageFramer.h"
#include "ParseResult.h"
#include "PrettyPrinter.h"
#include "FixPanel.h"

#include <windows.h>

#include <map>
#include <memory>
#include <string>

using namespace fixparser;

// ---------------------------------------------------------------------------
// Globals shared with dllmain.cpp.
// ---------------------------------------------------------------------------
NppData nppData;
FuncItem funcItem[nbFunc];

namespace {

HANDLE g_hModule = nullptr;
FixPanel g_panel;
bool g_panelRegistered = false;
std::unique_ptr<DictionaryCache> g_dictCache;

struct DocState {
    int mode = 1;        // 1 = hover, 2 = dock
    int activeRow = -1;  // line that last updated the dock via double-click
};
std::map<LRESULT, DocState> g_docState;

constexpr int kDwellMs = 300;

// --- Scintilla / Notepad++ helpers ----------------------------------------

HWND currentScintilla() {
    int which = -1;
    ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0,
                  reinterpret_cast<LPARAM>(&which));
    if (which == 0) return nppData._scintillaMainHandle;
    if (which == 1) return nppData._scintillaSecondHandle;
    return nullptr;
}

LRESULT currentBufferId() {
    return ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0);
}

std::string lineTextUtf8(HWND sci, Sci_Position line) {
    LRESULT len = ::SendMessage(sci, SCI_LINELENGTH, (WPARAM)line, 0);
    if (len <= 0) return std::string();
    std::string s(static_cast<size_t>(len), '\0');
    ::SendMessage(sci, SCI_GETLINE, (WPARAM)line, reinterpret_cast<LPARAM>(s.data()));
    // Trim trailing CR/LF.
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
    return s;
}

std::string moduleDirUtf8() {
    wchar_t path[MAX_PATH] = {};
    ::GetModuleFileNameW(reinterpret_cast<HMODULE>(g_hModule), path, MAX_PATH);
    std::wstring w(path);
    size_t slash = w.find_last_of(L"\\/");
    if (slash != std::wstring::npos) w.resize(slash);
    int n = ::WideCharToMultiByte(CP_ACP, 0, w.data(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s(n, '\0');
    ::WideCharToMultiByte(CP_ACP, 0, w.data(), (int)w.size(), s.data(), n, nullptr, nullptr);
    return s;
}

DictionaryCache* dictCache() {
    if (!g_dictCache) {
        std::string dir = moduleDirUtf8();
        dir += "\\dictionaries";
        g_dictCache = std::make_unique<DictionaryCache>(dir);
    }
    return g_dictCache.get();
}

std::string fieldValue(const ParseResult& r, int tag) {
    for (const auto* vec : {&r.header, &r.body, &r.trailer})
        for (const auto& f : *vec)
            if (f.tag == tag) return f.value;
    return std::string();
}

bool isAdminMsgType(const std::string& m) {
    return m == "0" || m == "1" || m == "2" || m == "3" || m == "4" || m == "5" || m == "A";
}

// Parse a line and, if it is FIX, re-parse with the version-appropriate dictionary.
ParseResult parseLine(const std::string& line) {
    ParseResult r = FixParser::parse(line, nullptr);
    if (!r.isFix) return r;

    std::string applVerId = fieldValue(r, 1128);
    const Dictionary* d =
        dictCache()->forMessage(r.beginString, applVerId, isAdminMsgType(r.msgType));
    if (d) r = FixParser::parse(line, d);
    return r;
}

void ensurePanelRegistered() {
    if (g_panelRegistered) return;
    g_panel.init(reinterpret_cast<HINSTANCE>(g_hModule), nppData._nppHandle);

    static std::wstring moduleName;
    wchar_t path[MAX_PATH] = {};
    ::GetModuleFileNameW(reinterpret_cast<HMODULE>(g_hModule), path, MAX_PATH);
    std::wstring w(path);
    size_t slash = w.find_last_of(L"\\/");
    moduleName = (slash == std::wstring::npos) ? w : w.substr(slash + 1);

    tTbData data{};
    g_panel.create(&data);
    data.uMask = DWS_DF_CONT_RIGHT;
    data.pszModuleName = moduleName.c_str();
    data.dlgID = 1; // index of the "Show panel" command
    ::SendMessage(nppData._nppHandle, NPPM_DMMREGASDCKDLG, 0,
                  reinterpret_cast<LPARAM>(&data));
    g_panelRegistered = true;
}

std::wstring makeHistoryLabel(Sci_Position line, const ParseResult& r) {
    std::wstring label = L"Line " + std::to_wstring(line + 1) + L": 35=";
    label += std::wstring(r.msgType.begin(), r.msgType.end());
    if (!r.msgTypeName.empty())
        label += L" " + std::wstring(r.msgTypeName.begin(), r.msgTypeName.end());
    return label;
}

// --- Interaction handlers --------------------------------------------------

void onDwellStart(SCNotification* n) {
    HWND sci = currentScintilla();
    if (!sci) return;
    Sci_Position pos = n->position;
    if (pos < 0) return;
    Sci_Position line = ::SendMessage(sci, SCI_LINEFROMPOSITION, (WPARAM)pos, 0);
    ParseResult r = parseLine(lineTextUtf8(sci, line));
    if (!r.isFix) { ::SendMessage(sci, SCI_CALLTIPCANCEL, 0, 0); return; }
    std::string tip = FieldRenderer::toCalltip(r);
    ::SendMessage(sci, SCI_CALLTIPSHOW, (WPARAM)pos, reinterpret_cast<LPARAM>(tip.c_str()));
}

void onDwellEnd() {
    HWND sci = currentScintilla();
    if (sci) ::SendMessage(sci, SCI_CALLTIPCANCEL, 0, 0);
}

void onDoubleClick(SCNotification* n) {
    HWND sci = currentScintilla();
    if (!sci) return;
    Sci_Position line = n->line;
    if (line < 0) line = ::SendMessage(sci, SCI_LINEFROMPOSITION, (WPARAM)n->position, 0);

    ParseResult r = parseLine(lineTextUtf8(sci, line));
    if (!r.isFix) return; // double-click on non-FIX line does nothing

    DocState& st = g_docState[currentBufferId()];
    if (st.mode == 2 && st.activeRow == static_cast<int>(line)) {
        g_panel.display(false);          // exit Mode 2
        st.mode = 1;
        st.activeRow = -1;
        return;
    }

    ensurePanelRegistered();
    g_panel.showParse(r, makeHistoryLabel(line, r));
    g_panel.display(true);
    st.mode = 2;
    st.activeRow = static_cast<int>(line);
}

void pollPanelClosed() {
    if (g_panel.consumeCloseRequest()) {
        DocState& st = g_docState[currentBufferId()];
        st.mode = 1;
        st.activeRow = -1;
    }
}

void enableDwellOnViews() {
    if (nppData._scintillaMainHandle)
        ::SendMessage(nppData._scintillaMainHandle, SCI_SETMOUSEDWELLTIME, kDwellMs, 0);
    if (nppData._scintillaSecondHandle)
        ::SendMessage(nppData._scintillaSecondHandle, SCI_SETMOUSEDWELLTIME, kDwellMs, 0);
}

} // namespace

// ---------------------------------------------------------------------------
// Lifecycle.
// ---------------------------------------------------------------------------
void pluginInit(HANDLE hModule) { g_hModule = hModule; }

void pluginCleanUp() {}

void commandMenuInit() {
    setCommand(0, L"Pretty-print FIX log", cmdPrettyPrint, nullptr, false);
    setCommand(1, L"Show FIX Parser panel", cmdTogglePanel, nullptr, false);
    setCommand(2, L"", nullptr, nullptr, false); // separator
    setCommand(3, L"About", cmdAbout, nullptr, false);
}

void commandMenuCleanUp() {}

bool setCommand(size_t index, const wchar_t* cmdName, PFUNCPLUGINCMD pFunc,
                ShortcutKey* sk, bool check0nInit) {
    if (index >= nbFunc) return false;
    if (!pFunc && cmdName[0] != L'\0') return false;
    lstrcpyW(funcItem[index]._itemName, cmdName);
    funcItem[index]._pFunc = pFunc;
    funcItem[index]._init2Check = check0nInit;
    funcItem[index]._pShKey = sk;
    return true;
}

// ---------------------------------------------------------------------------
// Commands.
// ---------------------------------------------------------------------------
void cmdPrettyPrint() {
    HWND sci = currentScintilla();
    if (!sci) return;

    // Encoding guard: only UTF-8 (65001) or ANSI (0) buffers are supported.
    LRESULT cp = ::SendMessage(sci, SCI_GETCODEPAGE, 0, 0);
    if (cp != SC_CP_UTF8 && cp != 0) {
        ::MessageBoxW(nppData._nppHandle,
                      L"Unsupported buffer encoding. Save as UTF-8 and retry.",
                      L"FIX Parser", MB_OK | MB_ICONWARNING);
        return;
    }

    const char* doc = reinterpret_cast<const char*>(
        ::SendMessage(sci, SCI_GETCHARACTERPOINTER, 0, 0));
    LRESULT len = ::SendMessage(sci, SCI_GETLENGTH, 0, 0);
    if (!doc || len <= 0) return;

    std::string_view raw(doc, static_cast<size_t>(len));
    NormalizedFix norm = DelimiterNormalizer::normalize(raw);

    if (norm.confidence == DetectionConfidence::None) {
        ::MessageBoxW(nppData._nppHandle,
                      L"No FIX field delimiter detected in this buffer.",
                      L"FIX Parser", MB_OK | MB_ICONINFORMATION);
        return;
    }
    if (norm.confidence == DetectionConfidence::Ambiguous) {
        int choice = ::MessageBoxW(
            nppData._nppHandle,
            L"Delimiter detection is ambiguous. Continue with the best guess?",
            L"FIX Parser", MB_OKCANCEL | MB_ICONINFORMATION);
        if (choice == IDCANCEL) return;
    }

    auto bounds = MessageFramer::findBoundaries(norm.canonical);
    if (bounds.empty()) {
        ::MessageBoxW(nppData._nppHandle, L"No FIX messages found.",
                      L"FIX Parser", MB_OK | MB_ICONINFORMATION);
        return;
    }

    // Determine the document EOL so we never create a mixed-EOL buffer.
    LRESULT eolMode = ::SendMessage(sci, SCI_GETEOLMODE, 0, 0);
    const char* eol = (eolMode == SC_EOL_CRLF) ? "\r\n"
                    : (eolMode == SC_EOL_CR)   ? "\r" : "\n";

    std::string out;
    for (size_t i = 0; i < bounds.size(); ++i) {
        if (i > 0) out += eol;
        std::string_view msg(norm.canonical.data() + bounds[i].start,
                             bounds[i].end - bounds[i].start);
        out += PrettyPrinter::formatOne(msg, OutputStyle::PipePerLine);
    }

    ::SendMessage(sci, SCI_BEGINUNDOACTION, 0, 0);
    ::SendMessage(sci, SCI_SETTARGETSTART, 0, 0);
    ::SendMessage(sci, SCI_SETTARGETEND, (WPARAM)len, 0);
    ::SendMessage(sci, SCI_REPLACETARGET, out.size(),
                  reinterpret_cast<LPARAM>(out.c_str()));
    ::SendMessage(sci, SCI_ENDUNDOACTION, 0, 0);
}

void cmdTogglePanel() {
    ensurePanelRegistered();
    if (g_panel.isClosed()) {
        g_panel.display(true);
        g_panel.setClosed(false);
    } else {
        g_panel.display(false);
        g_panel.setClosed(true);
    }
}

void cmdAbout() {
    ::MessageBoxW(nppData._nppHandle,
                  L"FIX Parser for Notepad++\n"
                  L"Pretty-print and inspect FIX protocol messages.\n"
                  L"GPL-2.0. Dictionaries: QuickFIX (QuickFIX Software License).",
                  L"About FIX Parser", MB_OK | MB_ICONINFORMATION);
}

// ---------------------------------------------------------------------------
// Notification hub.
// ---------------------------------------------------------------------------
void handleNotification(SCNotification* notify) {
    pollPanelClosed();
    switch (notify->nmhdr.code) {
        case NPPN_READY:
            enableDwellOnViews();
            break;
        case SCN_DWELLSTART:
            onDwellStart(notify);
            break;
        case SCN_DWELLEND:
            onDwellEnd();
            break;
        case SCN_DOUBLECLICK:
            onDoubleClick(notify);
            break;
        case NPPN_SHUTDOWN:
            commandMenuCleanUp();
            break;
        default:
            break;
    }
}

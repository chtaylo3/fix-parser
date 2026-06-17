#include "FixPanel.h"

#include "FieldRenderer.h"
#include "resource.h"

#include <commctrl.h>

namespace fixparser {

namespace {

std::wstring utf8ToWide(const std::string& s) {
    if (s.empty()) return std::wstring();
    int n = ::MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
    std::wstring w(n, L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), w.data(), n);
    return w;
}

HTREEITEM addNode(HWND tree, HTREEITEM parent, const std::wstring& text) {
    TVINSERTSTRUCTW tvis{};
    tvis.hParent = parent;
    tvis.hInsertAfter = TVI_LAST;
    tvis.item.mask = TVIF_TEXT;
    tvis.item.pszText = const_cast<LPWSTR>(text.c_str());
    return reinterpret_cast<HTREEITEM>(
        ::SendMessageW(tree, TVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&tvis)));
}

void addFields(HWND tree, HTREEITEM parent, const std::vector<ParsedField>& fields) {
    for (const auto& f : fields)
        addNode(tree, parent, utf8ToWide(FieldRenderer::formatField(f)));
}

} // namespace

FixPanel::FixPanel() : DockingDlgInterface(IDD_FIX_PANEL) {}

bool FixPanel::consumeCloseRequest() {
    bool v = _closeRequested;
    _closeRequested = false;
    return v;
}

void FixPanel::createControls() {
    INITCOMMONCONTROLSEX icc{ sizeof(icc), ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES };
    ::InitCommonControlsEx(&icc);

    _hHistory = ::CreateWindowExW(
        WS_EX_CLIENTEDGE, L"LISTBOX", nullptr,
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_HASSTRINGS,
        0, 0, 0, 0, _hSelf, reinterpret_cast<HMENU>(IDC_FIX_HISTORY),
        reinterpret_cast<HINSTANCE>(_hInst), nullptr);

    _hTree = ::CreateWindowExW(
        WS_EX_CLIENTEDGE, WC_TREEVIEWW, nullptr,
        WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS,
        0, 0, 0, 0, _hSelf, reinterpret_cast<HMENU>(IDC_FIX_TREE),
        reinterpret_cast<HINSTANCE>(_hInst), nullptr);

    layout();
}

void FixPanel::layout() {
    if (!_hHistory || !_hTree) return;
    RECT rc{};
    ::GetClientRect(_hSelf, &rc);
    const int w = rc.right - rc.left;
    const int h = rc.bottom - rc.top;
    const int historyH = h / 4;
    ::MoveWindow(_hHistory, 0, 0, w, historyH, TRUE);
    ::MoveWindow(_hTree, 0, historyH, w, h - historyH, TRUE);
}

void FixPanel::renderTree(const ParseResult& result) {
    if (!_hTree) return;
    ::SendMessageW(_hTree, TVM_DELETEITEM, 0, reinterpret_cast<LPARAM>(TVI_ROOT));

    std::wstring head = L"Header";
    if (!result.msgType.empty()) {
        head += L"  (";
        head += utf8ToWide(result.msgType);
        if (!result.msgTypeName.empty()) { head += L" "; head += utf8ToWide(result.msgTypeName); }
        head += L")";
    }
    HTREEITEM hHeader = addNode(_hTree, TVI_ROOT, head);
    addFields(_hTree, hHeader, result.header);

    HTREEITEM hBody = addNode(_hTree, TVI_ROOT, L"Body");
    addFields(_hTree, hBody, result.body);

    if (!result.trailer.empty()) {
        HTREEITEM hTrailer = addNode(_hTree, TVI_ROOT, L"Trailer");
        addFields(_hTree, hTrailer, result.trailer);
    }

    if (!result.diagnostics.empty()) {
        HTREEITEM hDiag = addNode(_hTree, TVI_ROOT, L"Diagnostics");
        for (const auto& d : result.diagnostics)
            addNode(_hTree, hDiag, utf8ToWide(d.message));
    }

    ::SendMessageW(_hTree, TVM_EXPAND, TVE_EXPAND, reinterpret_cast<LPARAM>(hHeader));
    ::SendMessageW(_hTree, TVM_EXPAND, TVE_EXPAND, reinterpret_cast<LPARAM>(hBody));
}

void FixPanel::showHistoryEntry(int index) {
    if (index < 0 || static_cast<std::size_t>(index) >= _history.size()) return;
    renderTree(_history[static_cast<std::size_t>(index)].result);
}

void FixPanel::showParse(const ParseResult& result, const std::wstring& label) {
    _history.insert(_history.begin(), Entry{label, result});
    if (_history.size() > kMaxHistory) _history.pop_back();

    if (_hHistory) {
        ::SendMessageW(_hHistory, LB_INSERTSTRING, 0,
                       reinterpret_cast<LPARAM>(label.c_str()));
        const int count = (int)::SendMessageW(_hHistory, LB_GETCOUNT, 0, 0);
        while (count > (int)kMaxHistory)
            ::SendMessageW(_hHistory, LB_DELETESTRING, kMaxHistory, 0);
        ::SendMessageW(_hHistory, LB_SETCURSEL, 0, 0);
    }
    renderTree(result);
}

INT_PTR CALLBACK FixPanel::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_INITDIALOG:
            createControls();
            return TRUE;

        case WM_SIZE:
            layout();
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_FIX_HISTORY && HIWORD(wParam) == LBN_SELCHANGE) {
                int sel = (int)::SendMessageW(_hHistory, LB_GETCURSEL, 0, 0);
                showHistoryEntry(sel);
                return TRUE;
            }
            break;

        case WM_NOTIFY: {
            auto* pnmh = reinterpret_cast<LPNMHDR>(lParam);
            if (pnmh->hwndFrom == _hParent && LOWORD(pnmh->code) == DMN_CLOSE) {
                _closeRequested = true;
                setClosed(true);
                return TRUE;
            }
            break;
        }

        default:
            break;
    }
    return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
}

} // namespace fixparser

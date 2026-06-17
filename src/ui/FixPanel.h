#pragma once

#include <windows.h>

#include "DockingDlgInterface.h"
#include "ParseResult.h"

#include <string>
#include <vector>

namespace fixparser {

// Dockable panel: a history list (top) of inspected messages and a field tree
// (bottom) showing the currently selected parse.
class FixPanel : public DockingDlgInterface {
public:
    FixPanel();

    // Show a parse and push it onto the history (newest first).
    void showParse(const ParseResult& result, const std::wstring& label);

    // True once and resets if the user closed the panel via its X (DMN_CLOSE),
    // so the host can return the document to Mode 1.
    bool consumeCloseRequest();

protected:
    INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;

private:
    void createControls();
    void layout();
    void renderTree(const ParseResult& result);
    void showHistoryEntry(int index);

    HWND _hHistory = nullptr; // ListBox of labels, newest first
    HWND _hTree = nullptr;    // TreeView of fields

    struct Entry {
        std::wstring label;
        ParseResult result;
    };
    std::vector<Entry> _history; // index 0 == newest
    bool _closeRequested = false;

    static constexpr std::size_t kMaxHistory = 100;
};

} // namespace fixparser

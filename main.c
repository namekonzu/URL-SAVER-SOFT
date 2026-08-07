#define UNICODE
#define _UNICODE
#define ID_TREE 100
#define ID_ADD_FOLDER 101
#define ID_NAME_EDIT 102
#define ID_RENAME 103
#define ID_BUTTON 1
#define ID_FILE_NAME_EDIT 104
#define ID_URL_EDIT       105
#define ID_ADD_FILE       106
#include <windows.h>
#include <commctrl.h>
#include<shellapi.h>
#include <stdlib.h>
#include <string.h>

HWND g_tree;
HWND g_nameEdit;
HWND g_fileNameEdit;
HWND g_urlEdit;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        g_tree = CreateWindowW(
            WC_TREEVIEWW,
            L"",
            WS_VISIBLE |
            WS_CHILD |
            WS_BORDER |
            TVS_HASLINES |
            TVS_LINESATROOT |
            TVS_HASBUTTONS,
            20,
            20,
            400,
            500,
            hwnd,
            (HMENU)ID_TREE,
            NULL,
            NULL
        );
        TreeView_SetItemHeight(g_tree, 40);
        TVINSERTSTRUCTW item = {0};

        item.hParent = TVI_ROOT;
        item.hInsertAfter = TVI_LAST;
        item.item.mask = TVIF_TEXT;
        item.item.pszText = L"青学";
        
        HTREEITEM aogaku = TreeView_InsertItem(g_tree, &item);

        item.hParent = aogaku;
        item.hInsertAfter = TVI_LAST;
        item.item.mask = TVIF_TEXT|TVIF_PARAM;
        item.item.pszText = L"青学ホームページ";
        item.item.lParam = (LPARAM)L"https://www.aoyama.ac.jp/";

        TreeView_InsertItem(g_tree, &item);
        CreateWindowW(
            L"BUTTON",
            L"フォルダ追加",
            WS_VISIBLE | WS_CHILD,
            440,
            20,
            120,
            35,
            hwnd,
            (HMENU)ID_ADD_FOLDER,
            NULL,
            NULL
        );
        g_nameEdit = CreateWindowW(
            L"EDIT",
            L"",
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            440,
            70,
            200,
            30,
            hwnd,
            (HMENU)ID_NAME_EDIT,
            NULL,
            NULL
        );
        CreateWindowW(
            L"BUTTON",
            L"名前変更",
            WS_VISIBLE | WS_CHILD,
            440,
            110,
            120,
            35,
            hwnd,
            (HMENU)ID_RENAME,
            NULL,
            NULL
        );
    }
    case WM_COMMAND:
    {
    if (LOWORD(wParam) == ID_ADD_FOLDER)
    {
        TVINSERTSTRUCTW item = {0};

        item.hParent = TVI_ROOT;
        item.hInsertAfter = TVI_LAST;
        item.item.mask = TVIF_TEXT;
        item.item.pszText = L"新しいフォルダ";

        TreeView_InsertItem(
            g_tree,
            &item
        );
    }
    if (LOWORD(wParam) == ID_RENAME)
    {
        HTREEITEM selected =
            TreeView_GetSelection(g_tree);

    if (selected != NULL)
        {
            WCHAR newName[256];

            GetWindowTextW(
                g_nameEdit,
                newName,
                256
            );

            if (newName[0] != L'\0')
            {
                TVITEMW item = {0};

                item.mask = TVIF_TEXT;
                item.hItem = selected;
                item.pszText = newName;

                TreeView_SetItem(
                    g_tree,
                    &item
                );
            }
        }
    }

    return 0;
    }
    case WM_NOTIFY:
    {
        LPNMHDR header = (LPNMHDR)lParam;
        if (header->idFrom == ID_TREE &&
            header->code == NM_DBLCLK||header->code==NM_RCLICK)
        {
            HWND tree = header->hwndFrom;
            HTREEITEM selected =
                TreeView_GetSelection(tree);

            if (selected != NULL)
            {
                TVITEMW selectedItem = {0};

                selectedItem.mask = TVIF_PARAM;
                selectedItem.hItem = selected;

                TreeView_GetItem(
                    tree,
                    &selectedItem
                );

                if (selectedItem.lParam != 0)
                {
                    LPCWSTR url =
                        (LPCWSTR)selectedItem.lParam;

                    ShellExecuteW(
                        hwnd,
                        L"open",
                        url,
                        NULL,
                        NULL,
                        SW_SHOWNORMAL
                    );
                }
            }
        }

        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
{
    INITCOMMONCONTROLSEX icex = {0};

    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_TREEVIEW_CLASSES;

    InitCommonControlsEx(&icex);


    WNDCLASSW wc = {0};

    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"MainWindow";

    RegisterClassW(&wc);

    HWND hwnd = CreateWindowW(
        L"MainWindow",
        L"URLBox",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        800,
        600,
        NULL,
        NULL,
        hInstance,
        NULL);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
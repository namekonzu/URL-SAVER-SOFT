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
#define ID_DELETE         107
#include <windows.h>
#include <commctrl.h>
#include<shellapi.h>
#include <stdlib.h>
#include <string.h>
#include <windowsx.h>
#include <sqlite3.h>
#include <wchar.h>

HWND g_tree;
HWND g_nameEdit;
HWND g_fileNameEdit;
HWND g_urlEdit;
sqlite3 *g_db = NULL;
typedef struct
{
    sqlite3_int64 id;
    WCHAR *url;
} ItemData;

int InitDatabase(void)
{
    int result;

    result = sqlite3_open(
        "urlbox.db",
        &g_db
    );

    if (result != SQLITE_OK)
    {
        MessageBoxA(
            NULL,
            sqlite3_errmsg(g_db),
            "Database Error",
            MB_OK | MB_ICONERROR
        );

        return 0;
    }

    result = sqlite3_exec(
        g_db,
        "PRAGMA foreign_keys = ON;",
        NULL,
        NULL,
        NULL
    );

    if (result != SQLITE_OK)
    {
        MessageBoxA(
            NULL,
            sqlite3_errmsg(g_db),
            "Database Error",
            MB_OK | MB_ICONERROR
        );

        return 0;
    }

    const char *sql =
        "CREATE TABLE IF NOT EXISTS items ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "parent_id INTEGER,"
        "type TEXT NOT NULL,"
        "name TEXT NOT NULL,"
        "url TEXT,"
        "FOREIGN KEY(parent_id) "
        "REFERENCES items(id) "
        "ON DELETE CASCADE"
        ");";

    char *errorMessage = NULL;

    result = sqlite3_exec(
        g_db,
        sql,
        NULL,
        NULL,
        &errorMessage
    );

    if (result != SQLITE_OK)
    {
        MessageBoxA(
            NULL,
            errorMessage,
            "Database Error",
            MB_OK | MB_ICONERROR
        );

        sqlite3_free(errorMessage);

        return 0;
    }

    return 1;
}

sqlite3_int64 InsertFolderToDatabase(
    LPCWSTR name
)
{
    sqlite3_stmt *stmt = NULL;

    const WCHAR *sql =
        L"INSERT INTO items "
        L"(parent_id, type, name, url) "
        L"VALUES "
        L"(NULL, 'folder', ?, NULL);";

    int result =
        sqlite3_prepare16_v2(
            g_db,
            sql,
            -1,
            &stmt,
            NULL
        );

    if (result != SQLITE_OK)
    {
        MessageBoxA(
            NULL,
            sqlite3_errmsg(g_db),
            "Database Error",
            MB_OK | MB_ICONERROR
        );

        return -1;
    }


    result =
        sqlite3_bind_text16(
            stmt,
            1,
            name,
            -1,
            SQLITE_TRANSIENT
        );

    if (result != SQLITE_OK)
    {
        sqlite3_finalize(stmt);
        return -1;
    }


    result =
        sqlite3_step(stmt);

    if (result != SQLITE_DONE)
    {
        MessageBoxA(
            NULL,
            sqlite3_errmsg(g_db),
            "Database Error",
            MB_OK | MB_ICONERROR
        );

        sqlite3_finalize(stmt);

        return -1;
    }


    sqlite3_int64 id =
        sqlite3_last_insert_rowid(g_db);

    sqlite3_finalize(stmt);

    return id;
}

sqlite3_int64 InsertUrlToDatabase(
    sqlite3_int64 parentId,
    LPCWSTR name,
    LPCWSTR url
)
{
    sqlite3_stmt *stmt = NULL;

    const WCHAR *sql =
        L"INSERT INTO items "
        L"(parent_id, type, name, url) "
        L"VALUES (?, 'url', ?, ?);";

    int result =
        sqlite3_prepare16_v2(
            g_db,
            sql,
            -1,
            &stmt,
            NULL
        );

    if (result != SQLITE_OK)
    {
        MessageBoxA(
            NULL,
            sqlite3_errmsg(g_db),
            "Database Error",
            MB_OK | MB_ICONERROR
        );

        return -1;
    }

    /*
     * 1番目の ? → parent_id
     */
    result =
        sqlite3_bind_int64(
            stmt,
            1,
            parentId
        );

    if (result != SQLITE_OK)
    {
        sqlite3_finalize(stmt);
        return -1;
    }

    /*
     * 2番目の ? → name
     */
    result =
        sqlite3_bind_text16(
            stmt,
            2,
            name,
            -1,
            SQLITE_TRANSIENT
        );

    if (result != SQLITE_OK)
    {
        sqlite3_finalize(stmt);
        return -1;
    }

    /*
     * 3番目の ? → url
     */
    result =
        sqlite3_bind_text16(
            stmt,
            3,
            url,
            -1,
            SQLITE_TRANSIENT
        );

    if (result != SQLITE_OK)
    {
        sqlite3_finalize(stmt);
        return -1;
    }

    result =
        sqlite3_step(stmt);

    if (result != SQLITE_DONE)
    {
        MessageBoxA(
            NULL,
            sqlite3_errmsg(g_db),
            "Database Error",
            MB_OK | MB_ICONERROR
        );

        sqlite3_finalize(stmt);

        return -1;
    }

    sqlite3_int64 id =
        sqlite3_last_insert_rowid(g_db);

    sqlite3_finalize(stmt);

    return id;
}

int UpdateItemNameInDatabase(
    sqlite3_int64 id,
    LPCWSTR newName
)
{
    if (id <= 0)
    {
        return 1;
    }

    sqlite3_stmt *stmt = NULL;

    const WCHAR *sql =
        L"UPDATE items "
        L"SET name = ? "
        L"WHERE id = ?;";

    int result =
        sqlite3_prepare16_v2(
            g_db,
            sql,
            -1,
            &stmt,
            NULL
        );

    if (result != SQLITE_OK)
    {
        MessageBoxA(
            NULL,
            sqlite3_errmsg(g_db),
            "Database Error",
            MB_OK | MB_ICONERROR
        );

        return 0;
    }

    sqlite3_bind_text16(
        stmt,
        1,
        newName,
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_int64(
        stmt,
        2,
        id
    );

    result =
        sqlite3_step(stmt);

    if (result != SQLITE_DONE)
    {
        MessageBoxA(
            NULL,
            sqlite3_errmsg(g_db),
            "Database Error",
            MB_OK | MB_ICONERROR
        );

        sqlite3_finalize(stmt);

        return 0;
    }

    sqlite3_finalize(stmt);

    return 1;
}

void FreeTreeItemData(
    HWND tree,
    HTREEITEM item
)
{
    HTREEITEM child =
        TreeView_GetChild(tree, item);

    while (child != NULL)
    {
        HTREEITEM next =
            TreeView_GetNextSibling(
                tree,
                child
            );


        FreeTreeItemData(
            tree,
            child
        );

        child = next;
    }


    TVITEMW treeItem = {0};

    treeItem.mask = TVIF_PARAM;
    treeItem.hItem = item;

    TreeView_GetItem(
        tree,
        &treeItem
    );


    if (treeItem.lParam != 0)
    {
        ItemData *data =
            (ItemData *)treeItem.lParam;

        if (data->url != NULL)
        {
            free(data->url);
        }

        free(data);
    }
}

int DeleteItemFromDatabase(
    sqlite3_int64 id
)
{
    if (id <= 0)
    {
        return 1;
    }

    sqlite3_stmt *stmt = NULL;

    const WCHAR *sql =
        L"DELETE FROM items "
        L"WHERE id = ?;";

    int result =
        sqlite3_prepare16_v2(
            g_db,
            sql,
            -1,
            &stmt,
            NULL
        );

    if (result != SQLITE_OK)
    {
        MessageBoxA(
            NULL,
            sqlite3_errmsg(g_db),
            "Database Error",
            MB_OK | MB_ICONERROR
        );

        return 0;
    }

    sqlite3_bind_int64(
        stmt,
        1,
        id
    );

    result =
        sqlite3_step(stmt);

    if (result != SQLITE_DONE)
    {
        MessageBoxA(
            NULL,
            sqlite3_errmsg(g_db),
            "Database Error",
            MB_OK | MB_ICONERROR
        );

        sqlite3_finalize(stmt);

        return 0;
    }

    sqlite3_finalize(stmt);

    return 1;
}

HTREEITEM FindTreeItemByDatabaseId(
    HWND tree,
    HTREEITEM item,
    sqlite3_int64 id
)
{
    while (item != NULL)
    {
        TVITEMW treeItem = {0};

        treeItem.mask = TVIF_PARAM;
        treeItem.hItem = item;

        TreeView_GetItem(
            tree,
            &treeItem
        );

        if (treeItem.lParam != 0)
        {
            ItemData *data =
                (ItemData *)treeItem.lParam;

            if (data->id == id)
            {
                return item;
            }
        }

        HTREEITEM child =
            TreeView_GetChild(
                tree,
                item
            );

        if (child != NULL)
        {
            HTREEITEM found =
                FindTreeItemByDatabaseId(
                    tree,
                    child,
                    id
                );

            if (found != NULL)
            {
                return found;
            }
        }

        item =
            TreeView_GetNextSibling(
                tree,
                item
            );
    }

    return NULL;
}

int LoadItemsFromDatabase(void)
{
    sqlite3_stmt *stmt = NULL;

    const WCHAR *folderSql =
        L"SELECT id, name "
        L"FROM items "
        L"WHERE type = 'folder' "
        L"ORDER BY id;";

    int result =
        sqlite3_prepare16_v2(
            g_db,
            folderSql,
            -1,
            &stmt,
            NULL
        );

    if (result != SQLITE_OK)
    {
        MessageBoxA(
            NULL,
            sqlite3_errmsg(g_db),
            "Database Error",
            MB_OK | MB_ICONERROR
        );

        return 0;
    }


    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        sqlite3_int64 id =
            sqlite3_column_int64(
                stmt,
                0
            );

        LPCWSTR name =
            (LPCWSTR)sqlite3_column_text16(
                stmt,
                1
            );


        ItemData *data =
            malloc(sizeof(ItemData));

        if (data == NULL)
        {
            sqlite3_finalize(stmt);
            return 0;
        }

        data->id = id;
        data->url = NULL;


        TVINSERTSTRUCTW item = {0};

        item.hParent =
            TVI_ROOT;

        item.hInsertAfter =
            TVI_LAST;

        item.item.mask =
            TVIF_TEXT |
            TVIF_PARAM;

        item.item.pszText =
            (LPWSTR)name;

        item.item.lParam =
            (LPARAM)data;


        HTREEITEM inserted =
            TreeView_InsertItem(
                g_tree,
                &item
            );

        if (inserted == NULL)
        {
            free(data);

            sqlite3_finalize(stmt);

            return 0;
        }
    }


    sqlite3_finalize(stmt);
    stmt = NULL;

    const WCHAR *urlSql =
        L"SELECT id, parent_id, name, url "
        L"FROM items "
        L"WHERE type = 'url' "
        L"ORDER BY id;";


    result =
        sqlite3_prepare16_v2(
            g_db,
            urlSql,
            -1,
            &stmt,
            NULL
        );

    if (result != SQLITE_OK)
    {
        MessageBoxA(
            NULL,
            sqlite3_errmsg(g_db),
            "Database Error",
            MB_OK | MB_ICONERROR
        );

        return 0;
    }


    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        sqlite3_int64 id =
            sqlite3_column_int64(
                stmt,
                0
            );

        sqlite3_int64 parentId =
            sqlite3_column_int64(
                stmt,
                1
            );

        LPCWSTR name =
            (LPCWSTR)sqlite3_column_text16(
                stmt,
                2
            );

        LPCWSTR databaseUrl =
            (LPCWSTR)sqlite3_column_text16(
                stmt,
                3
            );

        HTREEITEM parent =
            FindTreeItemByDatabaseId(
                g_tree,
                TreeView_GetRoot(g_tree),
                parentId
            );

        if (parent == NULL)
        {
            continue;
        }


        int urlLength =
            (int)wcslen(databaseUrl);

        WCHAR *savedUrl =
            malloc(
                (urlLength + 1) *
                sizeof(WCHAR)
            );

        if (savedUrl == NULL)
        {
            sqlite3_finalize(stmt);
            return 0;
        }

        wcscpy(
            savedUrl,
            databaseUrl
        );


        ItemData *data =
            malloc(sizeof(ItemData));

        if (data == NULL)
        {
            free(savedUrl);

            sqlite3_finalize(stmt);

            return 0;
        }

        data->id = id;
        data->url = savedUrl;


        TVINSERTSTRUCTW item = {0};

        item.hParent =
            parent;

        item.hInsertAfter =
            TVI_LAST;

        item.item.mask =
            TVIF_TEXT |
            TVIF_PARAM;

        item.item.pszText =
            (LPWSTR)name;

        item.item.lParam =
            (LPARAM)data;


        HTREEITEM inserted =
            TreeView_InsertItem(
                g_tree,
                &item
            );

        if (inserted == NULL)
        {
            free(savedUrl);
            free(data);

            sqlite3_finalize(stmt);

            return 0;
        }
    }


    sqlite3_finalize(stmt);

    return 1;
}

void CopyTextToClipboard(
    HWND hwnd,
    LPCWSTR text
)
{
    if(text==NULL)
    {
        return;
    }

    size_t size =
        (wcslen(text) + 1) *
        sizeof(WCHAR);

    HGLOBAL memory =
        GlobalAlloc(
            GMEM_MOVEABLE,
            size
        );

    if (memory == NULL)
        return;

    WCHAR *buffer =
        (WCHAR *)GlobalLock(memory);

    if (buffer == NULL)
    {
        GlobalFree(memory);
        return;
    }

    memcpy(
        buffer,
        text,
        size
    );

    GlobalUnlock(memory);

    if (OpenClipboard(hwnd))
    {
        EmptyClipboard();

        SetClipboardData(
            CF_UNICODETEXT,
            memory
        );

        CloseClipboard();
    }
    else
    {
        GlobalFree(memory);
    }
}

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

        if(!LoadItemsFromDatabase())
        {
            MessageBoxW(
                hwnd,
                L"データベースからの読み込みに失敗しました。",
                L"Error",
                MB_OK | MB_ICONERROR
            );
        }
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
        CreateWindowW(
            L"STATIC",
            L"サイト名",
            WS_VISIBLE | WS_CHILD,
            440,
            160,
            250,
            20,
            hwnd,
            NULL,
            NULL,
            NULL
        );
        g_fileNameEdit = CreateWindowW(
            L"EDIT",
            L"",
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            440,
            180,
            250,
            30,
            hwnd,
            (HMENU)ID_FILE_NAME_EDIT,
            NULL,
            NULL
        );
        CreateWindowW(
            L"STATIC",
            L"URL",
            WS_VISIBLE | WS_CHILD,
            440,
            200,
            250,
            20,
            hwnd,
            NULL,
            NULL,
            NULL
        );
        g_urlEdit = CreateWindowW(
            L"EDIT",
            L"",
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            440,
            220,
            250,
            30,
            hwnd,
            (HMENU)ID_URL_EDIT,
            NULL,
            NULL
        );
        CreateWindowW(
            L"BUTTON",
            L"ファイル追加",
            WS_VISIBLE | WS_CHILD,
            440,
            260,
            120,
            35,
            hwnd,
            (HMENU)ID_ADD_FILE,
            NULL,
            NULL
        );
        CreateWindowW(
            L"BUTTON",
            L"削除",
            WS_VISIBLE | WS_CHILD,
            440,
            320,
            120,
            35,
            hwnd,
            (HMENU)ID_DELETE,
            NULL,
            NULL
        
        );
        CreateWindowW(
            L"STATIC",
            L"【使い方】\n"
            L"・フォルダを選択してURLを追加\n"
            L"・左クリック：URLをコピー\n"
            L"・ダブルクリック：サイトを開く\n"
            L"・項目を選択して名前変更(ファイル、フォルダ)\n"
            L"【注意】削除すると子項目も削除",
            WS_VISIBLE | WS_CHILD,
            440,
            380,
            320,
            130,
            hwnd,
            NULL,
            NULL,
            NULL
        );
    }
    case WM_COMMAND:
    {
    if (LOWORD(wParam) == ID_ADD_FOLDER)
    {
        sqlite3_int64 id =
            InsertFolderToDatabase(
                L"新しいフォルダ"
            );

        if (id == -1)
        {
            return 0;
        }

        ItemData *data =
            malloc(sizeof(ItemData));

        if (data == NULL)
        {
            MessageBoxW(
                hwnd,
                L"メモリの確保に失敗しました。",
                L"Error",
                MB_OK | MB_ICONERROR
            );

            return 0;
        }

        data->id = id;
        data->url = NULL;

        TVINSERTSTRUCTW item = {0};

        item.hParent = TVI_ROOT;
        item.hInsertAfter = TVI_LAST;

        item.item.mask =
            TVIF_TEXT |
            TVIF_PARAM;

        item.item.pszText =
            L"新しいフォルダ";

        item.item.lParam =
            (LPARAM)data;


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
                TVITEMW dataItem = {0};

                dataItem.mask =
                    TVIF_PARAM;

                dataItem.hItem =
                    selected;

                TreeView_GetItem(
                    g_tree,
                    &dataItem
                );

                ItemData *data =
                    (ItemData *)dataItem.lParam;

                if (data != NULL &&
                    data->id > 0)
                {
                    if (!UpdateItemNameInDatabase(
                            data->id,
                            newName))
                    {
                        return 0;
                    }
                }


                TVITEMW item = {0};

                item.mask =
                    TVIF_TEXT;

                item.hItem =
                    selected;

                item.pszText =
                    newName;

                TreeView_SetItem(
                    g_tree,
                    &item
                );

                SetWindowTextW(
                    g_nameEdit,
                    L""
                );

                SetFocus(
                    g_nameEdit
                );
            }
        }
    }
    if (LOWORD(wParam) == ID_ADD_FILE)
    {
        HTREEITEM selected =
            TreeView_GetSelection(g_tree);

        if (selected != NULL)
        {
            /*
            * 選択された項目のItemDataを取得
            */
            TVITEMW parentItem = {0};

            parentItem.mask =
                TVIF_PARAM;

            parentItem.hItem =
                selected;

            TreeView_GetItem(
                g_tree,
                &parentItem
            );

            ItemData *parentData =
                (ItemData *)parentItem.lParam;


            /*
            * フォルダ以外にはURLを追加させない
            */
            if (parentData == NULL ||
                parentData->id <= 0 ||
                parentData->url != NULL)
            {
                MessageBoxW(
                    hwnd,
                    L"URLを追加するフォルダを選択してください。",
                    L"URL追加",
                    MB_OK | MB_ICONINFORMATION
                );

                return 0;
            }


            WCHAR fileName[256];

            GetWindowTextW(
                g_fileNameEdit,
                fileName,
                256
            );

            int urlLength =
                GetWindowTextLengthW(
                    g_urlEdit
                );


            if (fileName[0] != L'\0' &&
                urlLength > 0)
            {
                /*
                * URL保存用メモリ
                */
                WCHAR *savedUrl =
                    malloc(
                        (urlLength + 1) *
                        sizeof(WCHAR)
                    );

                if (savedUrl == NULL)
                {
                    return 0;
                }

                GetWindowTextW(
                    g_urlEdit,
                    savedUrl,
                    urlLength + 1
                );


                /*
                * ItemDataも先に確保
                */
                ItemData *data =
                    malloc(sizeof(ItemData));

                if (data == NULL)
                {
                    free(savedUrl);
                    return 0;
                }


                /*
                * SQLiteへURLを保存
                */
                sqlite3_int64 id =
                    InsertUrlToDatabase(
                        parentData->id,
                        fileName,
                        savedUrl
                    );

                if (id == -1)
                {
                    free(savedUrl);
                    free(data);

                    return 0;
                }


                /*
                * DBから取得した本物のidを設定
                */
                data->id = id;
                data->url = savedUrl;


                /*
                * TreeViewへ追加
                */
                TVINSERTSTRUCTW item = {0};

                item.hParent =
                    selected;

                item.hInsertAfter =
                    TVI_LAST;

                item.item.mask =
                    TVIF_TEXT |
                    TVIF_PARAM;

                item.item.pszText =
                    fileName;

                item.item.lParam =
                    (LPARAM)data;


                HTREEITEM newItem =
                    TreeView_InsertItem(
                        g_tree,
                        &item
                    );


                /*
                * TreeViewへの追加に失敗した場合
                */
                if (newItem == NULL)
                {
                    DeleteItemFromDatabase(id);

                    free(savedUrl);
                    free(data);

                    MessageBoxW(
                        hwnd,
                        L"TreeViewへの追加に失敗しました。",
                        L"Error",
                        MB_OK | MB_ICONERROR
                    );

                    return 0;
                }


                /*
                * 入力欄をクリア
                */
                SetWindowTextW(
                    g_fileNameEdit,
                    L""
                );

                SetWindowTextW(
                    g_urlEdit,
                    L""
                );
            }
        }
    }
    if (LOWORD(wParam) == ID_DELETE)
    {
        HTREEITEM selected =
            TreeView_GetSelection(g_tree);

        if (selected != NULL)
        {
            int result = MessageBoxW(
                hwnd,
                L"選択した項目を削除しますか？",
                L"削除確認",
                MB_YESNO | MB_ICONWARNING
            );

            if (result == IDYES)
            {
                TVITEMW item = {0};

                item.mask = TVIF_PARAM;
                item.hItem = selected;

                TreeView_GetItem(
                    g_tree,
                    &item
                );

                ItemData *data =
                    (ItemData *)item.lParam;


                if (data != NULL &&
                    data->id > 0)
                {
                    if (!DeleteItemFromDatabase(
                            data->id))
                    {
                        return 0;
                    }
                }

                FreeTreeItemData(
                    g_tree,
                    selected
                );

                TreeView_DeleteItem(
                    g_tree,
                    selected
                );


                SetWindowTextW(
                    g_nameEdit,
                    L""
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
        header->code == TVN_SELCHANGEDW)
            {
                LPNMTREEVIEWW treeInfo =
                    (LPNMTREEVIEWW)lParam;

                WCHAR itemName[256];

                TVITEMW item = {0};

                item.mask = TVIF_TEXT;
                item.hItem = treeInfo->itemNew.hItem;
                item.pszText = itemName;
                item.cchTextMax = 256;

                TreeView_GetItem(
                    g_tree,
                    &item
                );

                SetWindowTextW(
                    g_nameEdit,
                    itemName
                );
            }
        if (header->idFrom == ID_TREE &&
            header->code == NM_DBLCLK)
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
                    ItemData *data =
                        (ItemData *)selectedItem.lParam;

                    if (data->url != NULL)
                    {
                        ShellExecuteW(
                            hwnd,
                            L"open",
                            data->url,
                            NULL,
                            NULL,
                            SW_SHOWNORMAL
                        );
                    }
                }
            }
        }

        if (header->idFrom == ID_TREE &&
            header->code == NM_CLICK)
        {
            DWORD position =
                GetMessagePos();

            POINT point;

            point.x =
                GET_X_LPARAM(position);

            point.y =
                GET_Y_LPARAM(position);

            ScreenToClient(
                g_tree,
                &point
            );

            TVHITTESTINFO hitInfo = {0};

            hitInfo.pt = point;

            HTREEITEM clicked =
                TreeView_HitTest(
                    g_tree,
                    &hitInfo
                );

            if (clicked != NULL)
            {
                TVITEMW item = {0};

                item.mask = TVIF_PARAM;
                item.hItem = clicked;

                TreeView_GetItem(
                    g_tree,
                    &item
                );

                if (item.lParam != 0)
                {
                    ItemData *data =
                        (ItemData *)item.lParam;

                    if (data != NULL &&
                        data->url != NULL)
                    {
                        CopyTextToClipboard(
                            hwnd,
                            data->url
                        );
                    }
                }
            }
        }

        return 0;
    }
    case WM_DESTROY:
    {
        if (g_db != NULL)
        {
            sqlite3_close(g_db);
            g_db = NULL;
        }

        PostQuitMessage(0);
        return 0;
    }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
{
    if (!InitDatabase())
    {
        return 1;
    }

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
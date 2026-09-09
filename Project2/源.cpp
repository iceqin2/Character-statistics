#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <fstream>
#include <iomanip>
#include <sstream>

using namespace std;


/* 人物信息 */
struct PersonInfo {
    int id;
    const wchar_t* displayName;
    const char* searchName;
};


/* 人物出现位置 */
struct PositionNode {

    long long charPos;
    long long bytePos;

    PositionNode* next;

    PositionNode(
        long long c,
        long long b
    ) {
        charPos = c;
        bytePos = b;
        next = NULL;
    }
};


/* 散列表中的人物节点 */
struct PersonNode {

    int id;

    string searchName;

    wstring name;

    int count;

    PositionNode* positions;

    PositionNode* tail;

    PersonNode* next;


    PersonNode(
        int i,
        const string& s,
        const wstring& n
    ) {

        id = i;

        searchName = s;

        name = n;

        count = 0;

        positions = NULL;

        tail = NULL;

        next = NULL;
    }
};


/* 自定义散列表 */
class HashTable {

private:

    static const int TABLE_SIZE = 1009;

    PersonNode* table[TABLE_SIZE];


    /* BKDR Hash */
    unsigned long long hashFunction(
        const string& str
    ) const {

        unsigned long long hash = 0;

        for (
            size_t i = 0;
            i < str.size();
            i++
            ) {

            unsigned char c =
                (unsigned char)str[i];

            hash =
                hash * 131 + c;
        }

        return hash % TABLE_SIZE;
    }


public:

    /* 构造函数 */
    HashTable() {

        for (
            int i = 0;
            i < TABLE_SIZE;
            i++
            ) {

            table[i] = NULL;
        }
    }


    /* 析构函数 */
    ~HashTable() {

        clear();
    }


    /* 清空散列表 */
    void clear() {

        for (
            int i = 0;
            i < TABLE_SIZE;
            i++
            ) {

            PersonNode* p =
                table[i];

            while (p != NULL) {

                PersonNode* nextPerson =
                    p->next;

                PositionNode* pos =
                    p->positions;

                while (pos != NULL) {

                    PositionNode* nextPos =
                        pos->next;

                    delete pos;

                    pos = nextPos;
                }

                delete p;

                p = nextPerson;
            }

            table[i] = NULL;
        }
    }


    /*
     * 根据UTF-8名称查找人物
     *
     * 平均时间复杂度：O(1)
     *
     * 最坏时间复杂度：O(n)
     */
    PersonNode* find(
        const string& name
    ) {

        unsigned long long index =
            hashFunction(name);

        PersonNode* p =
            table[index];

        while (p != NULL) {

            if (p->searchName == name) {

                return p;
            }

            p = p->next;
        }

        return NULL;
    }


    /*
     * 插入人物
     */
    PersonNode* insert(
        int id,
        const string& searchName,
        const wstring& displayName
    ) {

        unsigned long long index =
            hashFunction(searchName);

        PersonNode* p =
            table[index];

        while (p != NULL) {

            if (p->searchName == searchName) {

                return p;
            }

            p = p->next;
        }


        PersonNode* newNode =
            new PersonNode(
                id,
                searchName,
                displayName
            );


        newNode->next =
            table[index];

        table[index] =
            newNode;


        return newNode;
    }


    /*
     * 添加一次人物出现记录
     */
    void addOccurrence(
        int id,
        const string& searchName,
        const wstring& displayName,
        long long charPos,
        long long bytePos
    ) {

        PersonNode* person =
            find(searchName);


        if (person == NULL) {

            person =
                insert(
                    id,
                    searchName,
                    displayName
                );
        }


        PositionNode* position =
            new PositionNode(
                charPos,
                bytePos
            );


        if (person->positions == NULL) {

            person->positions =
                position;

            person->tail =
                position;
        }
        else {

            person->tail->next =
                position;

            person->tail =
                position;
        }


        person->count++;
    }


    /*
     * 根据人物名称查询
     *
     * 查询过程直接使用散列表。
     */
     // 查询并返回结果字符串（GUI版，不再输出到控制台）
    wstring queryResult(const string& searchName) {
        PersonNode* person = find(searchName);
        if (person == NULL) return L"没有找到该人物！\r\n";

        wostringstream woss;
        woss << L"========================================\r\n";
        woss << L"人物：" << person->name << L"\r\n";
        woss << L"出现次数：" << person->count << L"\r\n";
        woss << L"----------------------------------------\r\n";
        woss << L"出现位置：\r\n";

        PositionNode* p = person->positions;
        int number = 1;
        while (p != NULL) {
            woss << L"第 " << number << L" 次：字符位置 = " << p->charPos
                << L"，字节位置 = " << p->bytePos << L"\r\n";
            p = p->next;
            number++;
        }
        woss << L"========================================\r\n";
        return woss.str();
    }


    /*
     * 显示散列表信息
     */
     // 返回哈希表信息字符串
    wstring getHashInfo() {
        int usedBuckets = 0, maxChain = 0, totalPeople = 0;
        for (int i = 0; i < TABLE_SIZE; i++) {
            int chainLength = 0;
            PersonNode* p = table[i];
            if (p != NULL) usedBuckets++;
            while (p != NULL) { chainLength++; totalPeople++; p = p->next; }
            if (chainLength > maxChain) maxChain = chainLength;
        }
        double loadFactor = (double)totalPeople / TABLE_SIZE;
        wostringstream woss;
        woss << L"散列表大小：" << TABLE_SIZE << L"    ";
        woss << L"人物总数：" << totalPeople << L"    ";
        woss << L"已使用桶数：" << usedBuckets << L"\r\n";
        woss << L"最长冲突链：" << maxChain << L"    ";
        woss << L"装填因子：" << fixed << setprecision(4) << loadFactor;
        return woss.str();
    }

    // 获取所有人的出场次数排行（GUI版新增功能）
    wstring getRanking() {
        struct RankItem { int id; wstring name; int count; };
        RankItem items[15];
        int cnt = 0;
        for (int i = 0; i < TABLE_SIZE; i++) {
            PersonNode* p = table[i];
            while (p != NULL) {
                items[cnt].id = p->id;
                items[cnt].name = p->name;
                items[cnt].count = p->count;
                cnt++;
                p = p->next;
            }
        }
        for (int i = 0; i < cnt - 1; i++)
            for (int j = 0; j < cnt - 1 - i; j++)
                if (items[j].count < items[j + 1].count) {
                    RankItem tmp = items[j]; items[j] = items[j + 1]; items[j + 1] = tmp;
                }
        wostringstream woss;
        woss << L"========== 人物出场次数排行 ==========\r\n";
        for (int i = 0; i < cnt; i++) {
            woss << L"第" << (i + 1) << L"名：" << items[i].name
                << L"  —  " << items[i].count << L" 次\r\n";
        }
        woss << L"======================================\r\n";
        return woss.str();
    }
};


/*
 * 人物库
 *
 * displayName：
 * 用于控制台显示。
 *
 * searchName：
 * UTF-8编码，用于TXT文件搜索。
 */
PersonInfo people[] = {

    {
        1,
        L"刘备",
        "\xE5\x88\x98\xE5\xA4\x87"
    },

    {
        2,
        L"关羽",
        "\xE5\x85\xB3\xE7\xBE\xBD"
    },

    {
        3,
        L"张飞",
        "\xE5\xBC\xA0\xE9\xA3\x9E"
    },

    {
        4,
        L"诸葛亮",
        "\xE8\xAF\xB8\xE8\x91\x9B\xE4\xBA\xAE"
    },

    {
        5,
        L"曹操",
        "\xE6\x9B\xB9\xE6\x93\x8D"
    },

    {
        6,
        L"孙权",
        "\xE5\xAD\x99\xE6\x9D\x83"
    },

    {
        7,
        L"赵云",
        "\xE8\xB5\xB5\xE4\xBA\x91"
    },

    {
        8,
        L"周瑜",
        "\xE5\x91\xA8\xE7\x91\x9C"
    },

    {
        9,
        L"司马懿",
        "\xE5\x8F\xB8\xE9\xA9\xAC\xE6\x87\xBF"
    },

    {
        10,
        L"吕布",
        "\xE5\x90\x95\xE5\xB8\x83"
    },

    {
        11,
        L"黄忠",
        "\xE9\xBB\x84\xE5\xBF\xA0"
    },

    {
        12,
        L"马超",
        "\xE9\xA9\xAC\xE8\xB6\x85"
    },

    {
        13,
        L"魏延",
        "\xE9\xAD\x8F\xE5\xBB\xB6"
    },

    {
        14,
        L"姜维",
        "\xE5\xA7\x9C\xE7\xBB\xB4"
    },

    {
        15,
        L"董卓",
        "\xE8\x91\xA3\xE5\x8D\x93"
    }
};


const int PEOPLE_COUNT =
sizeof(people) /
sizeof(people[0]);


/* 获取文件大小 */
long long getFileSize(
    const string& filename
) {

    ifstream file(
        filename.c_str(),
        ios::binary | ios::ate
    );


    if (!file) {

        return -1;
    }


    return (long long)file.tellg();
}


/* 判断UTF-8字符长度 */
int utf8CharLength(
    unsigned char c
) {

    if ((c & 0x80) == 0) {

        return 1;
    }


    if ((c & 0xE0) == 0xC0) {

        return 2;
    }


    if ((c & 0xF0) == 0xE0) {

        return 3;
    }


    if ((c & 0xF8) == 0xF0) {

        return 4;
    }


    return 1;
}


/*
 * 统计人物
 */
void statistics(
    const string& text,
    HashTable& hashTable
) {

    for (
        int i = 0;
        i < PEOPLE_COUNT;
        i++
        ) {

        const string& searchName =
            people[i].searchName;


        size_t start = 0;


        while (true) {

            size_t position =
                text.find(
                    searchName,
                    start
                );


            if (
                position ==
                string::npos
                ) {

                break;
            }


            /*
             * 计算字符位置
             */
            long long charPosition = 0;


            size_t j = 0;


            while (j < position) {

                unsigned char c =
                    (unsigned char)text[j];


                int len =
                    utf8CharLength(c);


                if (
                    j + len >
                    position
                    ) {

                    len = 1;
                }


                j += len;

                charPosition++;
            }


            /*
             * 保存出现记录
             */
            hashTable.addOccurrence(
                people[i].id,
                people[i].searchName,
                people[i].displayName,
                charPosition,
                (long long)position
            );


            /*
             * 从当前人物后面继续搜索
             */
            start =
                position +
                searchName.size();
        }
    }
}
// ========== 控件ID ==========
#define IDC_FILEEDIT    1001
#define IDC_BROWSE      1002
#define IDC_START       1003
#define IDC_PERSONLIST  1004
#define IDC_QUERYBTN    1005
#define IDC_RESULT      1006
#define IDC_HASHINFO    1007
#define IDC_STATUSBAR   1008

// ========== 编码转换辅助函数（GUI需要宽字符） ==========
wstring utf8ToWstring(const string& utf8) {
    if (utf8.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, NULL, 0);
    wstring result(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &result[0], len);
    if (!result.empty() && result.back() == L'\0') result.pop_back();
    return result;
}

string wstringToAnsi(const wstring& wstr) {
    if (wstr.empty()) return "";
    int len = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
    string result(len, '\0');
    WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, &result[0], len, NULL, NULL);
    if (!result.empty() && result.back() == '\0') result.pop_back();
    return result;
}

// ========== 全局变量 ==========
HashTable g_hashTable;
bool g_statDone = false;
HWND g_hFileEdit, g_hBrowseBtn, g_hStartBtn;
HWND g_hPersonList, g_hQueryBtn, g_hResultEdit, g_hHashInfoStatic;
HWND g_hStatusBar;
HFONT g_hFont;

// ========== 执行统计（对应控制台版main里的统计逻辑） ==========
void doStatistics(HWND hWnd) {
    wchar_t wpath[MAX_PATH];
    GetWindowTextW(g_hFileEdit, wpath, MAX_PATH);
    if (wcslen(wpath) == 0) {
        MessageBoxW(hWnd, L"请先选择或输入剧本TXT文件路径！", L"提示", MB_ICONWARNING);
        return;
    }
    string filename = wstringToAnsi(wpath);
    long long fileSize = getFileSize(filename);
    if (fileSize == -1) {
        MessageBoxW(hWnd, L"文件打开失败！请检查文件路径是否正确。", L"错误", MB_ICONERROR);
        return;
    }
    if (fileSize <= 1024 * 1024) {
        wchar_t msg[256];
        swprintf_s(msg, L"警告：当前文件大小为 %.2f MB，没有超过1MB！\n请使用大于1MB的TXT文本文件。",
            (double)fileSize / 1024.0 / 1024.0);
        MessageBoxW(hWnd, msg, L"文件过小", MB_ICONWARNING);
        return;
    }
    ifstream file(filename.c_str(), ios::binary);
    if (!file) { MessageBoxW(hWnd, L"文件打开失败！", L"错误", MB_ICONERROR); return; }
    string text((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();
    // 去除BOM
    if (text.size() >= 3 && (unsigned char)text[0] == 0xEF &&
        (unsigned char)text[1] == 0xBB && (unsigned char)text[2] == 0xBF) {
        text.erase(0, 3);
    }
    g_hashTable.clear();
    SendMessageW(g_hResultEdit, WM_SETTEXT, 0, (LPARAM)L"正在统计，请稍候...\r\n");
    UpdateWindow(g_hResultEdit);
    statistics(text, g_hashTable);  // 调用你原来的统计函数！
    g_statDone = true;

    // 更新人物列表
    SendMessageW(g_hPersonList, LB_RESETCONTENT, 0, 0);
    for (int i = 0; i < PEOPLE_COUNT; i++) {
        wchar_t item[64];
        PersonNode* p = g_hashTable.find(people[i].searchName);
        int cnt = p ? p->count : 0;
        swprintf_s(item, L"%2d. %s  (%d次)", people[i].id, people[i].displayName, cnt);
        SendMessageW(g_hPersonList, LB_ADDSTRING, 0, (LPARAM)item);
    }
    wstring info = g_hashTable.getHashInfo();
    SetWindowTextW(g_hHashInfoStatic, info.c_str());
    wstring result = L"统计完成！\r\n\r\n";
    result += g_hashTable.getRanking();
    result += L"\r\n";
    result += g_hashTable.getHashInfo();
    result += L"\r\n\r\n提示：在左侧列表双击人物，或选中后点击「查询详情」按钮，查看出场位置。";
    SendMessageW(g_hResultEdit, WM_SETTEXT, 0, (LPARAM)result.c_str());
    SendMessageW(g_hStatusBar, SB_SETTEXTW, 0, (LPARAM)L"统计完成");
}

// ========== 查询选中人物（对应控制台版main里的查询逻辑） ==========
void doQuery() {
    if (!g_statDone) {
        SendMessageW(g_hResultEdit, WM_SETTEXT, 0, (LPARAM)L"请先点击「开始统计」按钮！\r\n");
        return;
    }
    int sel = (int)SendMessageW(g_hPersonList, LB_GETCURSEL, 0, 0);
    if (sel == LB_ERR) {
        SendMessageW(g_hResultEdit, WM_SETTEXT, 0, (LPARAM)L"请先在左侧列表中选择一个人物！\r\n");
        return;
    }
    wstring result = g_hashTable.queryResult(people[sel].searchName);  // 调用你改后的查询方法
    SendMessageW(g_hResultEdit, WM_SETTEXT, 0, (LPARAM)result.c_str());
    SendMessageW(g_hStatusBar, SB_SETTEXTW, 0, (LPARAM)L"查询完成");
}

// ========== 窗口过程（所有消息在这里处理，代替控制台版的菜单循环） ==========
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        g_hFont = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"微软雅黑");
        // 文件选择区
        CreateWindowW(L"STATIC", L"剧本文件：", WS_CHILD | WS_VISIBLE | SS_RIGHT,
            10, 12, 70, 24, hWnd, NULL, NULL, NULL);
        g_hFileEdit = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            85, 10, 500, 26, hWnd, (HMENU)IDC_FILEEDIT, NULL, NULL);
        g_hBrowseBtn = CreateWindowW(L"BUTTON", L"浏览...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            595, 9, 80, 28, hWnd, (HMENU)IDC_BROWSE, NULL, NULL);
        g_hStartBtn = CreateWindowW(L"BUTTON", L"开始统计", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            685, 9, 100, 28, hWnd, (HMENU)IDC_START, NULL, NULL);
        // 人物列表区
        CreateWindowW(L"STATIC", L"人物列表（双击查询）：", WS_CHILD | WS_VISIBLE,
            10, 48, 200, 20, hWnd, NULL, NULL, NULL);
        g_hPersonList = CreateWindowW(L"LISTBOX", L"", WS_CHILD | WS_VISIBLE | WS_BORDER |
            LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | WS_VSCROLL,
            10, 70, 220, 420, hWnd, (HMENU)IDC_PERSONLIST, NULL, NULL);
        g_hQueryBtn = CreateWindowW(L"BUTTON", L"查询详情", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            10, 498, 220, 32, hWnd, (HMENU)IDC_QUERYBTN, NULL, NULL);
        // 结果区
        CreateWindowW(L"STATIC", L"统计结果 / 查询详情：", WS_CHILD | WS_VISIBLE,
            245, 48, 300, 20, hWnd, NULL, NULL, NULL);
        g_hResultEdit = CreateWindowW(L"EDIT", L"欢迎使用剧本人物统计系统（GUI版）！\r\n\r\n"
            L"使用步骤：\r\n"
            L"1. 点击「浏览...」选择大于1MB的UTF-8编码剧本TXT文件\r\n"
            L"2. 点击「开始统计」按钮\r\n"
            L"3. 在左侧人物列表中双击人物，查看出场次数和位置\r\n\r\n"
            L"核心算法：BKDR哈希函数 + 链地址法散列表 + UTF-8字符位置计算",
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE |
            ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL,
            245, 70, 540, 420, hWnd, (HMENU)IDC_RESULT, NULL, NULL);
        // 哈希表信息
        g_hHashInfoStatic = CreateWindowW(L"STATIC", L"散列表信息：（统计后显示）",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            245, 498, 540, 32, hWnd, (HMENU)IDC_HASHINFO, NULL, NULL);
        // 状态栏
        INITCOMMONCONTROLSEX iccx;
        iccx.dwSize = sizeof(iccx);
        iccx.dwICC = ICC_BAR_CLASSES;
        InitCommonControlsEx(&iccx);
        g_hStatusBar = CreateWindowExW(0, STATUSCLASSNAMEW, L"就绪",
            WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
            0, 0, 0, 0, hWnd, (HMENU)IDC_STATUSBAR, NULL, NULL);
        // 设置所有控件字体
        EnumChildWindows(hWnd, [](HWND hChild, LPARAM lParam) -> BOOL {
            SendMessageW(hChild, WM_SETFONT, (WPARAM)lParam, TRUE);
            return TRUE;
            }, (LPARAM)g_hFont);
        break;
    }
    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case IDC_BROWSE: {
            OPENFILENAMEW ofn;
            wchar_t szFile[MAX_PATH] = L"";
            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hWnd;
            ofn.lpstrFilter = L"文本文件 (*.txt)\0*.txt\0所有文件 (*.*)\0*.*\0";
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = MAX_PATH;
            ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
            ofn.lpstrTitle = L"选择剧本TXT文件";
            if (GetOpenFileNameW(&ofn)) SetWindowTextW(g_hFileEdit, szFile);
            break;
        }
        case IDC_START: doStatistics(hWnd); break;
        case IDC_QUERYBTN: doQuery(); break;
        case IDC_PERSONLIST:
            if (HIWORD(wParam) == LBN_DBLCLK) doQuery();
            break;
        }
        break;
    }
    case WM_SIZE:
        SendMessageW(g_hStatusBar, WM_SIZE, 0, 0);
        break;
    case WM_DESTROY:
        if (g_hFont) DeleteObject(g_hFont);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    return 0;
}

// ========== 程序入口（代替main()） ==========
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSEXW wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"ScriptStatGUI";
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    if (!RegisterClassExW(&wc)) {
        MessageBoxW(NULL, L"窗口类注册失败！", L"错误", MB_ICONERROR);
        return 1;
    }
    HWND hWnd = CreateWindowExW(
        WS_EX_WINDOWEDGE, L"ScriptStatGUI",
        L"剧本人物统计系统 — 基于散列表的人物出场统计（GUI版）",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 810, 600,
        NULL, NULL, hInstance, NULL);
    if (!hWnd) {
        MessageBoxW(NULL, L"窗口创建失败！", L"错误", MB_ICONERROR);
        return 1;
    }
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}







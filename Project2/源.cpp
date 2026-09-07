#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <richedit.h>
#include <io.h>
#include <fcntl.h>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

using namespace std;

#pragma comment(lib, "Comctl32.lib")

#define IDC_OPEN        1001
#define IDC_PEOPLE      1002
#define IDC_OCCURRENCES 1003
#define IDC_TEXT        1004
#define IDC_INFO        1005
#define IDC_STATUS      1006
#define IDC_TITLE       1007

#define WM_UPDATE_INFO  (WM_USER + 1)

struct PositionNode {
    long long charPos;
    long long bytePos;
    int line;
    int column;
    PositionNode* next;

    PositionNode(long long c, long long b, int l, int col) {
        charPos = c;
        bytePos = b;
        line = l;
        column = col;
        next = NULL;
    }
};

struct PersonNode {
    int id;
    wstring name;
    string searchName;
    int count;

    PositionNode* positions;
    PositionNode* tail;

    PersonNode* next;

    PersonNode(int i, const wstring& n, const string& s) {
        id = i;
        name = n;
        searchName = s;
        count = 0;
        positions = NULL;
        tail = NULL;
        next = NULL;
    }
};

class HashTable {

private:

    static const int TABLE_SIZE = 1009;

    PersonNode* table[TABLE_SIZE];

    unsigned long long hashFunction(const string& str) const {

        unsigned long long hash = 0;

        for (size_t i = 0; i < str.size(); i++) {

            unsigned char c =
                (unsigned char)str[i];

            hash = hash * 131 + c;
        }

        return hash % TABLE_SIZE;
    }

public:

    HashTable() {

        for (int i = 0; i < TABLE_SIZE; i++) {
            table[i] = NULL;
        }
    }

    ~HashTable() {
        clear();
    }

    void clear() {

        for (int i = 0; i < TABLE_SIZE; i++) {

            PersonNode* p = table[i];

            while (p != NULL) {

                PersonNode* nextPerson = p->next;

                PositionNode* pos = p->positions;

                while (pos != NULL) {

                    PositionNode* nextPos = pos->next;

                    delete pos;

                    pos = nextPos;
                }

                delete p;

                p = nextPerson;
            }

            table[i] = NULL;
        }
    }

    PersonNode* insert(
        int id,
        const wstring& name,
        const string& searchName) {

        unsigned long long index =
            hashFunction(searchName);

        PersonNode* p = table[index];

        while (p != NULL) {

            if (p->id == id) {
                return p;
            }

            p = p->next;
        }

        PersonNode* node =
            new PersonNode(
                id,
                name,
                searchName
            );

        node->next = table[index];

        table[index] = node;

        return node;
    }

    PersonNode* findById(int id) {

        for (int i = 0;
            i < TABLE_SIZE;
            i++) {

            PersonNode* p = table[i];

            while (p != NULL) {

                if (p->id == id) {
                    return p;
                }

                p = p->next;
            }
        }

        return NULL;
    }

    PersonNode* findByName(
        const string& name) {

        unsigned long long index =
            hashFunction(name);

        PersonNode* p = table[index];

        while (p != NULL) {

            if (p->searchName == name) {
                return p;
            }

            p = p->next;
        }

        return NULL;
    }

    void addOccurrence(
        int id,
        long long charPos,
        long long bytePos,
        int line,
        int column) {

        PersonNode* person = findById(id);

        if (person == NULL) {
            return;
        }

        PositionNode* position =
            new PositionNode(
                charPos,
                bytePos,
                line,
                column
            );

        if (person->positions == NULL) {

            person->positions = position;
            person->tail = position;

        }
        else {

            person->tail->next = position;
            person->tail = position;
        }

        person->count++;
    }

    int getTotalPeople() const {

        int total = 0;

        for (int i = 0;
            i < TABLE_SIZE;
            i++) {

            PersonNode* p = table[i];

            while (p != NULL) {

                total++;

                p = p->next;
            }
        }

        return total;
    }

    int getUsedBuckets() const {

        int used = 0;

        for (int i = 0;
            i < TABLE_SIZE;
            i++) {

            if (table[i] != NULL) {
                used++;
            }
        }

        return used;
    }

    int getMaxChain() const {

        int maxChain = 0;

        for (int i = 0;
            i < TABLE_SIZE;
            i++) {

            int length = 0;

            PersonNode* p = table[i];

            while (p != NULL) {

                length++;

                p = p->next;
            }

            if (length > maxChain) {
                maxChain = length;
            }
        }

        return maxChain;
    }

    double getLoadFactor() const {

        return
            (double)getTotalPeople()
            / TABLE_SIZE;
    }

    static int getTableSize() {
        return TABLE_SIZE;
    }
};


/* 全局变量 */

HashTable* g_hashTable = NULL;

string g_textUTF8;
wstring g_textUnicode;

vector<int> g_byteToUTF16;

HWND g_hMain = NULL;
HWND g_hPeople = NULL;
HWND g_hOccurrences = NULL;
HWND g_hText = NULL;
HWND g_hInfo = NULL;
HWND g_hStatus = NULL;
HWND g_hTitle = NULL;
HWND g_hOpen = NULL;


/* UTF-8字符长度 */

int utf8CharLength(unsigned char c) {

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


/* UTF-8转换成Unicode */

bool utf8ToUnicode(
    const string& input,
    wstring& output) {

    if (input.empty()) {

        output.clear();

        return true;
    }

    int len =
        MultiByteToWideChar(
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            input.data(),
            (int)input.size(),
            NULL,
            0
        );

    if (len <= 0) {

        return false;
    }

    output.resize(len);

    int result =
        MultiByteToWideChar(
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            input.data(),
            (int)input.size(),
            &output[0],
            len
        );

    return result > 0;
}


/* 建立UTF-8字节位置到RichEdit UTF-16位置的映射 */

void buildPositionMap() {

    g_byteToUTF16.clear();

    g_byteToUTF16.resize(
        g_textUTF8.size() + 1
    );

    size_t bytePos = 0;

    int utf16Pos = 0;

    while (bytePos < g_textUTF8.size()) {

        g_byteToUTF16[bytePos] = utf16Pos;

        unsigned char c =
            (unsigned char)g_textUTF8[bytePos];

        int len =
            utf8CharLength(c);

        if (bytePos + len >
            g_textUTF8.size()) {

            len = 1;
        }

        string oneChar =
            g_textUTF8.substr(
                bytePos,
                len
            );

        wstring wideChar;

        if (utf8ToUnicode(
            oneChar,
            wideChar)) {

            utf16Pos +=
                (int)wideChar.size();

        }
        else {

            utf16Pos++;
        }

        for (int i = 1;
            i < len &&
            bytePos + i <
            g_byteToUTF16.size();
            i++) {

            g_byteToUTF16[bytePos + i] =
                utf16Pos;
        }

        bytePos += len;
    }

    g_byteToUTF16[g_textUTF8.size()] =
        utf16Pos;
}


/* 获取文件大小 */

long long getFileSize(
    const wstring& filename) {

    HANDLE hFile =
        CreateFileW(
            filename.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );

    if (hFile == INVALID_HANDLE_VALUE) {
        return -1;
    }

    LARGE_INTEGER size;

    if (!GetFileSizeEx(
        hFile,
        &size)) {

        CloseHandle(hFile);

        return -1;
    }

    CloseHandle(hFile);

    return size.QuadPart;
}


/* 读取UTF-8 TXT */

bool loadFile(
    const wstring& filename) {

    ifstream file(
        filename.c_str(),
        ios::binary
    );

    if (!file) {
        return false;
    }

    g_textUTF8.assign(
        (istreambuf_iterator<char>(file)),
        istreambuf_iterator<char>()
    );

    file.close();

    /* 去掉UTF-8 BOM */

    if (g_textUTF8.size() >= 3 &&
        (unsigned char)g_textUTF8[0] == 0xEF &&
        (unsigned char)g_textUTF8[1] == 0xBB &&
        (unsigned char)g_textUTF8[2] == 0xBF) {

        g_textUTF8.erase(0, 3);
    }

    if (!utf8ToUnicode(
        g_textUTF8,
        g_textUnicode)) {

        return false;
    }

    buildPositionMap();

    return true;
}


/* 添加人物 */

void addPeople() {

    g_hashTable->insert(
        1,
        L"刘备",
        "\xE5\x88\x98\xE5\xA4\x87"
    );

    g_hashTable->insert(
        2,
        L"关羽",
        "\xE5\x85\xB3\xE7\xBE\xBD"
    );

    g_hashTable->insert(
        3,
        L"张飞",
        "\xE5\xBC\xA0\xE9\xA3\x9E"
    );

    g_hashTable->insert(
        4,
        L"诸葛亮",
        "\xE8\xAF\xB8\xE8\x91\x9B\xE4\xBA\xAE"
    );

    g_hashTable->insert(
        5,
        L"曹操",
        "\xE6\x9B\xB9\xE6\x93\x8D"
    );

    g_hashTable->insert(
        6,
        L"孙权",
        "\xE5\xAD\x99\xE6\x9D\x83"
    );

    g_hashTable->insert(
        7,
        L"赵云",
        "\xE8\xB5\xB5\xE4\xBA\x91"
    );

    g_hashTable->insert(
        8,
        L"周瑜",
        "\xE5\x91\xA8\xE7\x91\x9C"
    );

    g_hashTable->insert(
        9,
        L"司马懿",
        "\xE5\x8F\xB8\xE9\xA9\xAC\xE6\x87\xBF"
    );

    g_hashTable->insert(
        10,
        L"吕布",
        "\xE5\x90\x95\xE5\xB8\x83"
    );

    g_hashTable->insert(
        11,
        L"黄忠",
        "\xE9\xBB\x84\xE5\xBF\xA0"
    );

    g_hashTable->insert(
        12,
        L"马超",
        "\xE9\xA9\xAC\xE8\xB6\x85"
    );

    g_hashTable->insert(
        13,
        L"魏延",
        "\xE9\xAD\x8F\xE5\xBB\xB6"
    );

    g_hashTable->insert(
        14,
        L"姜维",
        "\xE5\xA7\x9C\xE7\xBB\xB4"
    );

    g_hashTable->insert(
        15,
        L"董卓",
        "\xE8\x91\xA3\xE5\x8D\x93"
    );
}


/* 统计人物 */

void statistics() {

    if (g_hashTable == NULL) {
        return;
    }

    for (int id = 1;
        id <= 15;
        id++) {

        PersonNode* person =
            g_hashTable->findById(id);

        if (person == NULL) {
            continue;
        }

        const string& name =
            person->searchName;

        size_t start = 0;

        while (true) {

            size_t pos =
                g_textUTF8.find(
                    name,
                    start
                );

            if (pos == string::npos) {
                break;
            }

            long long charPosition = 0;

            int line = 1;

            int column = 1;

            size_t j = 0;

            while (j < pos) {

                unsigned char c =
                    (unsigned char)g_textUTF8[j];

                int len =
                    utf8CharLength(c);

                if (j + len >
                    g_textUTF8.size()) {

                    len = 1;
                }

                if (c == '\n') {

                    line++;

                    column = 1;

                }
                else {

                    column++;
                }

                j += len;

                charPosition++;
            }

            g_hashTable->addOccurrence(
                id,
                charPosition,
                (long long)pos,
                line,
                column
            );

            start =
                pos + name.size();
        }
    }
}


/* 获取人物名称 */

wstring getPersonName(
    int id) {

    PersonNode* p =
        g_hashTable->findById(id);

    if (p != NULL) {
        return p->name;
    }

    return L"";
}


/* 显示人物库 */

void refreshPeopleList() {

    SendMessageW(
        g_hPeople,
        LB_RESETCONTENT,
        0,
        0
    );

    for (int id = 1;
        id <= 15;
        id++) {

        PersonNode* p =
            g_hashTable->findById(id);

        if (p == NULL) {
            continue;
        }

        wstring text =
            to_wstring(id) +
            L". " +
            p->name;

        int index =
            (int)SendMessageW(
                g_hPeople,
                LB_ADDSTRING,
                0,
                (LPARAM)text.c_str()
            );

        SendMessageW(
            g_hPeople,
            LB_SETITEMDATA,
            index,
            id
        );
    }
}


/* 显示散列表信息 */

void refreshHashInfo() {

    if (g_hashTable == NULL) {
        return;
    }

    wstringstream ss;

    ss << L"========== 散列表信息 ==========\r\n\r\n";

    ss << L"散列表容量："
        << HashTable::getTableSize()
        << L"\r\n";

    ss << L"人物数量："
        << g_hashTable->getTotalPeople()
        << L"\r\n";

    ss << L"已使用桶数量："
        << g_hashTable->getUsedBuckets()
        << L"\r\n";

    ss << L"最长冲突链长度："
        << g_hashTable->getMaxChain()
        << L"\r\n";

    ss << L"装载因子："
        << fixed
        << setprecision(4)
        << g_hashTable->getLoadFactor()
        << L"\r\n";

    ss << L"\r\n";

    ss << L"查询人物时采用散列表定位，\r\n";
    ss << L"人物出现位置使用链表保存。\r\n\r\n";

    ss << L"平均情况下：O(1)\r\n";
    ss << L"最坏情况下：O(n)\r\n";

    ss << L"================================";

    SetWindowTextW(
        g_hInfo,
        ss.str().c_str()
    );
}


/* 清空查询结果 */

void clearOccurrences() {

    SendMessageW(
        g_hOccurrences,
        LB_RESETCONTENT,
        0,
        0
    );
}


/* 显示某个人物的出现位置 */

void showPersonOccurrences(
    int id) {

    clearOccurrences();

    PersonNode* person =
        g_hashTable->findById(id);

    if (person == NULL) {
        return;
    }

    wstringstream title;

    title << L"人物："
        << person->name
        << L"    出现次数："
        << person->count;

    SetWindowTextW(
        g_hStatus,
        title.str().c_str()
    );

    PositionNode* p =
        person->positions;

    int number = 1;

    while (p != NULL) {

        wstringstream ss;

        ss << L"第 "
            << number
            << L" 次    ";

        ss << L"第 "
            << p->line
            << L" 行，第 "
            << p->column
            << L" 列    ";

        ss << L"字符位置："
            << p->charPos
            << L"    ";

        ss << L"字节位置："
            << p->bytePos;

        int index =
            (int)SendMessageW(
                g_hOccurrences,
                LB_ADDSTRING,
                0,
                (LPARAM)ss.str().c_str()
            );

        SendMessageW(
            g_hOccurrences,
            LB_SETITEMDATA,
            index,
            (LPARAM)p
        );

        p = p->next;

        number++;
    }

    if (person->count > 0) {

        SendMessageW(
            g_hOccurrences,
            LB_SETCURSEL,
            0,
            0
        );

        SendMessageW(
            g_hOccurrences,
            WM_COMMAND,
            MAKEWPARAM(
                IDC_OCCURRENCES,
                LBN_SELCHANGE
            ),
            0
        );
    }
}


/* 定位TXT中的某次出现 */

void locateOccurrence(
    PositionNode* position,
    const string& personName) {

    if (position == NULL) {
        return;
    }

    if (position->bytePos < 0 ||
        position->bytePos >=
        (long long)g_byteToUTF16.size()) {

        return;
    }

    int start =
        g_byteToUTF16[
            (size_t)position->bytePos
        ];

    int length = 0;

    wstring personWide;

    if (utf8ToUnicode(
        personName,
        personWide)) {

        length =
            (int)personWide.size();
    }

    SendMessageW(
        g_hText,
        EM_SETSEL,
        start,
        start + length
    );

    SendMessageW(
        g_hText,
        EM_SCROLLCARET,
        0,
        0
    );

    SetFocus(g_hText);
}


/* 打开文件 */

bool openTextFile(
    const wstring& filename) {

    long long fileSize =
        getFileSize(filename);

    if (fileSize <= 0) {

        MessageBoxW(
            g_hMain,
            L"文件打开失败！",
            L"错误",
            MB_ICONERROR
        );

        return false;
    }

    if (fileSize <= 1024LL * 1024LL) {

        MessageBoxW(
            g_hMain,
            L"当前TXT文件大小没有超过1MB。\n\n"
            L"根据课程设计要求，请使用大于1MB的TXT电子书。",
            L"文件大小不符合要求",
            MB_ICONWARNING
        );

        return false;
    }

    if (!loadFile(filename)) {

        MessageBoxW(
            g_hMain,
            L"TXT读取失败！\n\n"
            L"请确认文件采用UTF-8编码。",
            L"读取失败",
            MB_ICONERROR
        );

        return false;
    }

    delete g_hashTable;

    g_hashTable =
        new HashTable();

    addPeople();

    statistics();

    SetWindowTextW(
        g_hText,
        g_textUnicode.c_str()
    );

    SendMessageW(
        g_hText,
        EM_SETSEL,
        0,
        0
    );

    refreshPeopleList();

    refreshHashInfo();

    clearOccurrences();

    wstringstream ss;

    ss << L"文件读取成功    ";

    ss << fixed
        << setprecision(2)
        << (double)fileSize /
        1024.0 /
        1024.0;

    ss << L" MB    ";

    ss << L"统计完成";

    SetWindowTextW(
        g_hStatus,
        ss.str().c_str()
    );

    return true;
}


/* 打开文件对话框 */

void chooseFile() {

    OPENFILENAMEW ofn;

    ZeroMemory(
        &ofn,
        sizeof(ofn)
    );

    wchar_t filename[MAX_PATH] = L"";

    ofn.lStructSize =
        sizeof(ofn);

    ofn.hwndOwner =
        g_hMain;

    ofn.lpstrFilter =
        L"TXT文本文件 (*.txt)\0*.txt\0"
        L"所有文件 (*.*)\0*.*\0";

    ofn.lpstrFile =
        filename;

    ofn.nMaxFile =
        MAX_PATH;

    ofn.Flags =
        OFN_FILEMUSTEXIST |
        OFN_PATHMUSTEXIST;

    ofn.lpstrTitle =
        L"选择《三国演义》TXT电子书";

    if (GetOpenFileNameW(&ofn)) {

        openTextFile(filename);
    }
}


/* 创建控件 */

void createControls(
    HWND hwnd) {

    g_hTitle =
        CreateWindowW(
            L"STATIC",
            L"剧本人物统计系统",
            WS_CHILD |
            WS_VISIBLE |
            SS_CENTER,
            0,
            0,
            0,
            0,
            hwnd,
            (HMENU)IDC_TITLE,
            GetModuleHandle(NULL),
            NULL
        );

    g_hOpen =
        CreateWindowW(
            L"BUTTON",
            L"打开 TXT 文件",
            WS_CHILD |
            WS_VISIBLE |
            BS_PUSHBUTTON,
            0,
            0,
            0,
            0,
            hwnd,
            (HMENU)IDC_OPEN,
            GetModuleHandle(NULL),
            NULL
        );

    g_hPeople =
        CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"LISTBOX",
            NULL,
            WS_CHILD |
            WS_VISIBLE |
            WS_VSCROLL |
            LBS_NOTIFY |
            LBS_HASSTRINGS,
            0,
            0,
            0,
            0,
            hwnd,
            (HMENU)IDC_PEOPLE,
            GetModuleHandle(NULL),
            NULL
        );

    g_hOccurrences =
        CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"LISTBOX",
            NULL,
            WS_CHILD |
            WS_VISIBLE |
            WS_VSCROLL |
            LBS_NOTIFY |
            LBS_HASSTRINGS,
            0,
            0,
            0,
            0,
            hwnd,
            (HMENU)IDC_OCCURRENCES,
            GetModuleHandle(NULL),
            NULL
        );

    g_hInfo =
        CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"EDIT",
            NULL,
            WS_CHILD |
            WS_VISIBLE |
            WS_VSCROLL |
            ES_MULTILINE |
            ES_READONLY,
            0,
            0,
            0,
            0,
            hwnd,
            (HMENU)IDC_INFO,
            GetModuleHandle(NULL),
            NULL
        );

    g_hText =
        CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"RICHEDIT50W",
            NULL,
            WS_CHILD |
            WS_VISIBLE |
            WS_VSCROLL |
            WS_HSCROLL |
            ES_MULTILINE |
            ES_AUTOVSCROLL |
            ES_AUTOHSCROLL |
            ES_READONLY,
            0,
            0,
            0,
            0,
            hwnd,
            (HMENU)IDC_TEXT,
            GetModuleHandle(NULL),
            NULL
        );

    g_hStatus =
        CreateWindowW(
            L"STATIC",
            L"请点击“打开 TXT 文件”选择电子书",
            WS_CHILD |
            WS_VISIBLE |
            SS_LEFT,
            0,
            0,
            0,
            0,
            hwnd,
            (HMENU)IDC_STATUS,
            GetModuleHandle(NULL),
            NULL
        );

    SendMessageW(
        g_hText,
        EM_EXLIMITTEXT,
        0,
        0x7FFFFFFE
    );
}


/* 设置字体 */

void setFonts() {

    HFONT hFont =
        CreateFontW(
            18,
            0,
            0,
            0,
            FW_NORMAL,
            FALSE,
            FALSE,
            FALSE,
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            L"Microsoft YaHei"
        );

    HFONT hTitleFont =
        CreateFontW(
            26,
            0,
            0,
            0,
            FW_BOLD,
            FALSE,
            FALSE,
            FALSE,
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            L"Microsoft YaHei"
        );

    SendMessageW(
        g_hTitle,
        WM_SETFONT,
        (WPARAM)hTitleFont,
        TRUE
    );

    SendMessageW(
        g_hOpen,
        WM_SETFONT,
        (WPARAM)hFont,
        TRUE
    );

    SendMessageW(
        g_hPeople,
        WM_SETFONT,
        (WPARAM)hFont,
        TRUE
    );

    SendMessageW(
        g_hOccurrences,
        WM_SETFONT,
        (WPARAM)hFont,
        TRUE
    );

    SendMessageW(
        g_hInfo,
        WM_SETFONT,
        (WPARAM)hFont,
        TRUE
    );

    SendMessageW(
        g_hText,
        WM_SETFONT,
        (WPARAM)hFont,
        TRUE
    );

    SendMessageW(
        g_hStatus,
        WM_SETFONT,
        (WPARAM)hFont,
        TRUE
    );
}


/* 窗口布局 */

void resizeControls(
    int width,
    int height) {

    int margin = 15;

    int titleHeight = 45;

    int topHeight = 45;

    int statusHeight = 35;

    int leftWidth = 180;

    int middleWidth = 420;

    int infoHeight = 150;

    int yTop = 10;

    MoveWindow(
        g_hTitle,
        margin,
        yTop,
        width - 2 * margin - 180,
        titleHeight,
        TRUE
    );

    MoveWindow(
        g_hOpen,
        width - 170,
        yTop + 5,
        150,
        35,
        TRUE
    );

    int contentTop =
        titleHeight + topHeight;

    int contentBottom =
        height - statusHeight - margin;

    int contentHeight =
        contentBottom - contentTop;

    int x1 = margin;

    int x2 =
        x1 + leftWidth + margin;

    int x3 =
        x2 + middleWidth + margin;

    int rightWidth =
        width - x3 - margin;

    MoveWindow(
        g_hPeople,
        x1,
        contentTop,
        leftWidth,
        contentHeight,
        TRUE
    );

    MoveWindow(
        g_hOccurrences,
        x2,
        contentTop,
        middleWidth,
        contentHeight,
        TRUE
    );

    MoveWindow(
        g_hText,
        x3,
        contentTop,
        rightWidth,
        contentHeight - infoHeight - margin,
        TRUE
    );

    MoveWindow(
        g_hInfo,
        x3,
        contentTop +
        contentHeight -
        infoHeight,
        rightWidth,
        infoHeight,
        TRUE
    );

    MoveWindow(
        g_hStatus,
        margin,
        height - statusHeight,
        width - 2 * margin,
        statusHeight - 5,
        TRUE
    );
}


/* 窗口过程 */

LRESULT CALLBACK WndProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam) {

    switch (msg) {

    case WM_CREATE:

        g_hMain = hwnd;

        createControls(hwnd);

        setFonts();

        return 0;


    case WM_SIZE:

        resizeControls(
            LOWORD(lParam),
            HIWORD(lParam)
        );

        return 0;


    case WM_COMMAND:

        if (LOWORD(wParam) == IDC_OPEN) {

            chooseFile();

            return 0;
        }


        if (LOWORD(wParam) == IDC_PEOPLE &&
            HIWORD(wParam) == LBN_SELCHANGE) {

            int index =
                (int)SendMessageW(
                    g_hPeople,
                    LB_GETCURSEL,
                    0,
                    0
                );

            if (index != LB_ERR) {

                int id =
                    (int)SendMessageW(
                        g_hPeople,
                        LB_GETITEMDATA,
                        index,
                        0
                    );

                showPersonOccurrences(id);
            }

            return 0;
        }


        if (LOWORD(wParam) == IDC_OCCURRENCES &&
            HIWORD(wParam) == LBN_SELCHANGE) {

            int index =
                (int)SendMessageW(
                    g_hOccurrences,
                    LB_GETCURSEL,
                    0,
                    0
                );

            if (index != LB_ERR) {

                PositionNode* position =
                    (PositionNode*)SendMessageW(
                        g_hOccurrences,
                        LB_GETITEMDATA,
                        index,
                        0
                    );

                int peopleIndex =
                    (int)SendMessageW(
                        g_hPeople,
                        LB_GETCURSEL,
                        0,
                        0
                    );

                if (peopleIndex != LB_ERR) {

                    int id =
                        (int)SendMessageW(
                            g_hPeople,
                            LB_GETITEMDATA,
                            peopleIndex,
                            0
                        );

                    PersonNode* person =
                        g_hashTable->findById(id);

                    if (person != NULL) {

                        locateOccurrence(
                            position,
                            person->searchName
                        );
                    }
                }
            }

            return 0;
        }

        break;


    case WM_DESTROY:

        delete g_hashTable;

        g_hashTable = NULL;

        PostQuitMessage(0);

        return 0;
    }

    return DefWindowProcW(
        hwnd,
        msg,
        wParam,
        lParam
    );
}


/* Windows程序入口 */

int WINAPI wWinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    PWSTR lpCmdLine,
    int nCmdShow) {

    LoadLibraryW(
        L"Msftedit.dll"
    );

    INITCOMMONCONTROLSEX icc;

    icc.dwSize =
        sizeof(icc);

    icc.dwICC =
        ICC_STANDARD_CLASSES;

    InitCommonControlsEx(&icc);

    const wchar_t CLASS_NAME[] =
        L"PersonStatisticsWindow";

    WNDCLASSW wc;

    ZeroMemory(
        &wc,
        sizeof(wc)
    );

    wc.lpfnWndProc =
        WndProc;

    wc.hInstance =
        hInstance;

    wc.lpszClassName =
        CLASS_NAME;

    wc.hCursor =
        LoadCursor(
            NULL,
            IDC_ARROW
        );

    wc.hbrBackground =
        (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClassW(&wc);

    HWND hwnd =
        CreateWindowExW(
            0,
            CLASS_NAME,
            L"剧本人物统计系统",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            1400,
            850,
            NULL,
            NULL,
            hInstance,
            NULL
        );

    if (hwnd == NULL) {
        return 0;
    }

    g_hMain = hwnd;

    ShowWindow(
        hwnd,
        nCmdShow
    );

    UpdateWindow(hwnd);

    MSG msg;

    while (
        GetMessageW(
            &msg,
            NULL,
            0,
            0
        ) > 0) {

        TranslateMessage(&msg);

        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}
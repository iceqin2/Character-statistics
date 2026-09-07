#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <windows.h>
#include <io.h>
#include <fcntl.h>

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
    void query(
        const string& searchName
    ) {

        PersonNode* person =
            find(searchName);


        if (person == NULL) {

            wcout << L"\n没有找到该人物。\n";

            return;
        }


        wcout << L"\n";

        wcout
            << L"========================================\n";


        wcout
            << L"人物："
            << person->name
            << L"\n";


        wcout
            << L"出现次数："
            << person->count
            << L"\n";


        wcout
            << L"----------------------------------------\n";


        wcout
            << L"出现位置：\n";


        PositionNode* p =
            person->positions;


        int number = 1;


        while (p != NULL) {

            wcout
                << L"第 "
                << number
                << L" 次：";

            wcout
                << L"字符位置 = "
                << p->charPos;

            wcout
                << L"，字节位置 = "
                << p->bytePos
                << L"\n";


            p = p->next;

            number++;
        }


        wcout
            << L"========================================\n";
    }


    /*
     * 显示散列表信息
     */
    void showHashInfo() {

        int usedBuckets = 0;

        int maxChain = 0;

        int totalPeople = 0;


        for (
            int i = 0;
            i < TABLE_SIZE;
            i++
            ) {

            int chainLength = 0;

            PersonNode* p =
                table[i];


            if (p != NULL) {

                usedBuckets++;
            }


            while (p != NULL) {

                chainLength++;

                totalPeople++;

                p = p->next;
            }


            if (
                chainLength >
                maxChain
                ) {

                maxChain =
                    chainLength;
            }
        }


        double loadFactor =
            (double)totalPeople /
            TABLE_SIZE;


        wcout << L"\n";

        wcout
            << L"========== 散列表信息 ==========\n";


        wcout
            << L"散列表容量："
            << TABLE_SIZE
            << L"\n";


        wcout
            << L"人物数量："
            << totalPeople
            << L"\n";


        wcout
            << L"已使用桶数量："
            << usedBuckets
            << L"\n";


        wcout
            << L"最长冲突链长度："
            << maxChain
            << L"\n";


        wcout
            << L"装载因子："
            << fixed
            << setprecision(4)
            << loadFactor
            << L"\n";


        wcout
            << L"================================\n";
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


/* 显示人物库 */
void showPeople() {

    wcout << L"\n";

    wcout
        << L"========== 人物库 ==========\n";


    for (
        int i = 0;
        i < PEOPLE_COUNT;
        i++
        ) {

        wcout
            << setw(2)
            << people[i].id
            << L". "
            << people[i].displayName
            << L"\n";
    }


    wcout
        << L"============================\n";
}


/* 显示菜单 */
void showMenu() {

    wcout << L"\n";

    wcout
        << L"========================================\n";

    wcout
        << L"          剧本人物统计系统\n";

    wcout
        << L"========================================\n";

    wcout
        << L"1. 查询人物\n";

    wcout
        << L"2. 显示人物库\n";

    wcout
        << L"3. 显示散列表信息\n";

    wcout
        << L"0. 退出系统\n";

    wcout
        << L"========================================\n";

    wcout
        << L"请选择：";
}


/* 主函数 */
int main() {

    /*
     * Windows Unicode控制台模式
     */
    _setmode(
        _fileno(stdout),
        _O_U16TEXT
    );

    _setmode(
        _fileno(stdin),
        _O_U16TEXT
    );


    ios::sync_with_stdio(false);

    wcin.tie(NULL);

    wcout.tie(NULL);


    wcout
        << L"========================================\n";

    wcout
        << L"          剧本人物统计系统\n";

    wcout
        << L"========================================\n";


    /*
     * 输入TXT文件路径
     */
    wstring wfilename;


    wcout
        << L"请输入电子书TXT文件路径："
        << flush;


    getline(
        wcin,
        wfilename
    );


    /*
     * Windows宽字符串路径转换
     */
    int len =
        WideCharToMultiByte(
            CP_ACP,
            0,
            wfilename.c_str(),
            -1,
            NULL,
            0,
            NULL,
            NULL
        );


    if (len <= 0) {

        wcout
            << L"\n文件路径转换失败！\n";

        return 1;
    }


    string filename(
        len,
        '\0'
    );


    WideCharToMultiByte(
        CP_ACP,
        0,
        wfilename.c_str(),
        -1,
        &filename[0],
        len,
        NULL,
        NULL
    );


    if (
        !filename.empty() &&
        filename.back() == '\0'
        ) {

        filename.pop_back();
    }


    /*
     * 获取文件大小
     */
    long long fileSize =
        getFileSize(filename);


    if (fileSize == -1) {

        wcout
            << L"\n文件打开失败！\n";

        wcout
            << L"请检查文件路径是否正确。\n";

        return 1;
    }


    wcout
        << L"\n文件读取成功！\n";


    wcout
        << L"文件大小："
        << fixed
        << setprecision(2)
        << (double)fileSize /
        1024.0 /
        1024.0
        << L" MB\n";


    /*
     * 检查文件大小是否大于1MB
     */
    if (
        fileSize <=
        1024 * 1024
        ) {

        wcout
            << L"\n警告：当前文件大小没有超过1MB。\n";


        wcout
            << L"根据课程设计要求，"
            << L"请使用大于1MB的TXT电子书。\n";


        return 1;
    }


    /*
     * 打开TXT文件
     */
    ifstream file(
        filename.c_str(),
        ios::binary
    );


    if (!file) {

        wcout
            << L"文件打开失败！\n";

        return 1;
    }


    /*
     * 整个文件读入内存
     */
    string text(
        (istreambuf_iterator<char>(file)),
        istreambuf_iterator<char>()
    );


    file.close();


    wcout
        << L"文本读取完成！\n";


    /*
     * 去除UTF-8 BOM
     */
    if (
        text.size() >= 3 &&
        (unsigned char)text[0] == 0xEF &&
        (unsigned char)text[1] == 0xBB &&
        (unsigned char)text[2] == 0xBF
        ) {

        text.erase(
            0,
            3
        );
    }


    wcout
        << L"正在统计人物，请稍候...\n";


    /*
     * 创建散列表
     */
    HashTable hashTable;


    /*
     * 开始统计
     */
    statistics(
        text,
        hashTable
    );


    wcout
        << L"人物统计完成！\n";


    /*
     * 显示散列表信息
     */
    hashTable.showHashInfo();


    /*
     * 系统菜单
     */
    while (true) {

        showMenu();


        int choice;


        wcin >> choice;


        wcin.ignore(
            10000,
            L'\n'
        );


        /*
         * 退出
         */
        if (choice == 0) {

            wcout
                << L"\n系统退出。\n";

            break;
        }


        /*
         * 查询人物
         */
        else if (choice == 1) {

            showPeople();


            int id;


            wcout
                << L"请输入要查询的人物编号："
                << flush;


            wcin >> id;


            wcin.ignore(
                10000,
                L'\n'
            );


            if (
                id < 1 ||
                id > PEOPLE_COUNT
                ) {

                wcout
                    << L"人物编号错误！\n";
            }
            else {

                /*
                 * 根据编号找到人物
                 * 再把UTF-8名称交给散列表查询
                 */
                hashTable.query(
                    people[id - 1].searchName
                );
            }
        }


        /*
         * 显示人物库
         */
        else if (choice == 2) {

            showPeople();
        }


        /*
         * 显示散列表信息
         */
        else if (choice == 3) {

            hashTable.showHashInfo();
        }


        /*
         * 其他输入
         */
        else {

            wcout
                << L"输入错误，请重新选择。\n";
        }
    }


    return 0;
}
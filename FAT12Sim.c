#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define MAX_BLOCK_LENGTH 512
#define Num_OF_ALL_SEC 2880
#define MAX_ACTIVE_FILES 100
#define MAX_OPEN_FILES 10
#define MAX_FAT_ITEMS 3072
#define true 1
#define false 0
typedef int bool;
typedef unsigned char BYTE1;
typedef unsigned short BYTE2;
typedef unsigned int BYTE4;
typedef unsigned long long BYTE8;

typedef struct AddFileTODO {
    int toSec;
    int fromSec;
    int searchCount;
} AddFileTODO;

typedef struct BPB {
    BYTE1 BS_impBoot[3];
    BYTE1 BS_OEMName[8];
    BYTE2 BPB_BytsPerSec;     // 每扇区字节数 0x200 512
    BYTE1 BPB_SecPerClus;     // 每簇字节数 0x01
    BYTE2 BPB_RsvdSecCnt;     // boot记录占用多少扇区 0x01
    BYTE1 BPB_NumFATs;        // 共有多少FAT表 0x02
    BYTE2 BPB_RootEntCnt;     // 根目录文件数最大值 0xE0 224
    BYTE2 BPB_TotSec16;       // 扇区总数 0xB40 2880
    BYTE1 BPB_Media;          // 介质描述符号 0xF0
    BYTE2 BPB_FATSz16;        // 每FAT扇区数 0x09
    BYTE2 BPB_SecPerTrk;      // 每磁道扇区数 0x12
    BYTE2 BPB_NumHeads;       // 磁头数 0x02
    BYTE4 BPB_HiddSec;        // 隐藏扇区数 0x0
    BYTE4 BPB_TotSec32;       // 扇区总数 0xB40 2880
    BYTE1 BPB_DrvNum;         // 终端13驱动器号 0
    BYTE1 BPB_Reserved1;      // 未使用 0
    BYTE1 BPB_BootSig;        // 扩展引导标记 0x29
    BYTE4 BS_VoID;            // 卷序列号 0
    BYTE1 BS_VolLab[11];      // 卷标 OrangeS0.02
    BYTE1 BS_FileSysType[8];  // 文件系统类型 FAT12
} BPB;

typedef struct FATItem {
    int OriginSec;
    int pointTo;
    bool isDelete;
} FATItem;

typedef struct DirItem {
    BYTE1 szDirName[11];
    BYTE4 dwDirAttr;
    BYTE1 szDirReserve[10];
    BYTE2 dwWrtTime;
    BYTE2 dwWrtDate;
    BYTE2 dwFirstClus;
    BYTE4 dwFileSize;
    BYTE4 *dwClusList;
} DirItem;

typedef struct ACTIVE_FILE {
    bool empty;
    DirItem *f_dir;
    char *f_path;
    int share_counter;
} ACTIVE_FILE;

typedef struct FILE_OPEN {
    bool empty;
    int pos;
    int read_or_write;
    int activefile;
} FILE_OPEN;

typedef struct Date {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int sec;
} Date;

const int month_day_normal[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
const int month_day_special[12] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

unsigned char ramFDD144[Num_OF_ALL_SEC][MAX_BLOCK_LENGTH];
unsigned char ramFDD144_Copy[Num_OF_ALL_SEC][MAX_BLOCK_LENGTH];
ACTIVE_FILE active_file[MAX_ACTIVE_FILES];
FATItem fat_item[MAX_FAT_ITEMS];
int CurrentActiveFileNumber = 0;
int fat_item_num;

unsigned char impBoot[3];
unsigned char OEMName[8];
unsigned int BytsPerSec;       // 每扇区字节数 0x200 512
unsigned int SecPerClus;       // 每簇字节数 0x01
unsigned int RsvdSecCnt;       // boot记录占用多少扇区 0x01
unsigned int NumFATs;          // 共有多少FAT表 0x02
unsigned int RootEntCnt;       // 根目录文件数最大值 0xE0 224
unsigned int TotSec16;         // 扇区总数 0xB40 2880
unsigned int Media;            // 介质描述符号 0xF0
unsigned int FATSz16;          // 每FAT扇区数 0x09
unsigned int SecPerTrk;        // 每磁道扇区数 0x12
unsigned int NumHeads;         // 磁头数 0x02
unsigned int HiddSec;          // 隐藏扇区数 0x0
unsigned int TotSec32;         // 扇区总数 0xB40 2880
unsigned int DrvNum;           // 终端13驱动器号 0
unsigned int Reserved1;        // 未使用 0
unsigned int BootSig;          // 扩展引导标记 0x29
unsigned int VoID;             // 卷序列号 0
unsigned char VolLab[11];      // 卷标 OrangeS0.02
unsigned char FileSysType[8];  // 文件系统类型 FAT12

int Read_ramFDD_to_array(const char *filename, BPB *bpb) {
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL)
        return -1;
    size_t cnt = 0;
    size_t cnt1 = 0;
    size_t all_cnt = 0;
    while (fscanf(fp, "%c", &ramFDD144[cnt][cnt1]) != EOF) {
        all_cnt++;
        cnt1++;
        if (cnt1 == 512) {
            cnt++;
            cnt1 = 0;
        }
    }
    fclose(fp);
    return all_cnt;
}

int Read_ramFDD_to_array_temp(const char *filename, BPB *bpb) {
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL)
        return -1;
    size_t cnt = 0;
    size_t cnt1 = 0;
    size_t all_cnt = 0;
    while (fscanf(fp, "%c", &ramFDD144_Copy[cnt][cnt1]) != EOF) {
        all_cnt++;
        cnt1++;
        if (cnt1 == 512) {
            cnt++;
            cnt1 = 0;
        }
    }
    fclose(fp);
    return all_cnt;
}

int saveToImg(const char *filename) {
    FILE *fp = fopen(filename, "wb");
    if (fp == NULL)
        return -1;
    size_t cnt = 0;
    size_t cnt1 = 0;
    size_t all_cnt = 0;
    while (fprintf(fp, "%c", ramFDD144[cnt][cnt1]) != EOF) {
        all_cnt++;
        if (all_cnt == 512 * 2880)
            break;
        cnt1++;
        if (cnt1 == 512) {
            cnt++;
            cnt1 = 0;
        }
    }
    fclose(fp);
    return all_cnt;
}

int saveToImg_RollBack(const char *filename) {
    FILE *fp = fopen(filename, "wb");
    if (fp == NULL)
        return -1;
    size_t cnt = 0;
    size_t cnt1 = 0;
    size_t all_cnt = 0;
    while (fprintf(fp, "%c", ramFDD144_Copy[cnt][cnt1]) != EOF) {
        all_cnt++;
        if (all_cnt == 512 * 2880)
            break;
        cnt1++;
        if (cnt1 == 512) {
            cnt++;
            cnt1 = 0;
        }
    }
    fclose(fp);
    return all_cnt;
}

unsigned int char_to_uint(int sec_num, int start_index, int len) {
    if (len <= 0 || len > 4) {
        printf("Error: length too short or too long!\n");
        return 0;
    }
    unsigned int temp = 0;
    for (int i = len - 1; i >= 0; --i) {
        temp <<= 8;
        temp += ramFDD144[sec_num][start_index + i];
    }
    return temp;
}

char to_upper_case(char a) {
    if (a >= 'a' && a <= 'z')
        return a - 'a' + 'A';
}

void LoadDirItem(DirItem *di, int RootSecIndex, int offset) {
    for (int i = 0; i < 11; ++i) {
        di->szDirName[i] = ramFDD144[RootSecIndex][offset * 32 + i];
    }
    di->szDirName[11] = '\0';
    unsigned int DirAttr = char_to_uint(RootSecIndex, offset * 32 + 11, 1);
    di->dwDirAttr = DirAttr;
    for (int i = 0; i < 10; ++i) {
        di->szDirReserve[i] = ramFDD144[RootSecIndex][offset * 32 + 12 + i];
    }
    unsigned int DirWrtTime = char_to_uint(RootSecIndex, offset * 32 + 22, 2);
    di->dwWrtTime = DirWrtTime;
    unsigned int DirWrtDate = char_to_uint(RootSecIndex, offset * 32 + 24, 2);
    di->dwWrtDate = DirWrtDate;
    unsigned int DirFstClus = char_to_uint(RootSecIndex, offset * 32 + 26, 2);
    di->dwFirstClus = DirFstClus;
    unsigned int DirFileSize = char_to_uint(RootSecIndex, offset * 32 + 28, 4);
    di->dwFileSize = DirFileSize;
}

void treeSetDeleteInFAT(int RootSecIndex, int level) {
    int offset = 0;
    DirItem dim;
    DirItem *di = &dim;
    int tempSec = 0;
    int addTime = 0;
    while (ramFDD144[RootSecIndex][offset * 32] != 0 && offset < 16 && addTime < 14) {
        LoadDirItem(di, RootSecIndex, offset);
        if (di->szDirName[0] == 0xE5) {
            int DeleteIndex = di->dwFirstClus;
            fat_item[DeleteIndex].OriginSec = RootSecIndex;
            fat_item[DeleteIndex].isDelete = true;
            ++offset;
            continue;
        }
        if (di->dwDirAttr == 0x10) {
            if (di->szDirName[0] != '.') {
                tempSec = di->dwFirstClus + 31;
                treeSetDeleteInFAT(tempSec, level + 1);
            }
        }
        ++offset;
        if (level == 0 && addTime < 14 && offset == 16) {
            offset = 0;
            addTime += 1;
            RootSecIndex += 1;
        }
    }
}

int Read_FAT_Items() {
    treeSetDeleteInFAT(19, 0);
    int count = 0;
    for (int i = 1; i < 10; ++i) {
        for (int j = 0; j < 512; j += 3) {
            if ((j >= 0 && j + 2 < 512 && ramFDD144[i][j] == 0 && ramFDD144[i][j + 1] == 0 && ramFDD144[i][j + 2] == 0) ||
                j + 1 == 512 && i + 1 != 11 && ramFDD144[i + 1][0] == 0 && ramFDD144[i + 1][1] == 0 && ramFDD144[i][j] == 0 ||
                j + 1 == 512 && i + 1 != 11 && ramFDD144[i + 1][0] == 0 && ramFDD144[i + 1][1] == 0 && ramFDD144[i][j] == 0)
                return count;
            if (j >= 0 && j + 2 < 512 && (ramFDD144[i][j] != 0 || ramFDD144[i][j + 1] != 0 || ramFDD144[i][j + 2] != 0)) {
                unsigned char a = ramFDD144[i][j];
                unsigned char b = ramFDD144[i][j + 1];
                unsigned char c = ramFDD144[i][j + 2];
                if (((b & 0x0F) << 8) | a != 0) {
                    fat_item[count].pointTo = ((b & 0x0F) << 8) | a;
                    ++count;
                }
                if ((c << 4) | ((b >> 4) & 0x0F) != 0) {
                    fat_item[count].pointTo = (c << 4) | ((b >> 4) & 0x0F);
                    ++count;
                }
            }
            if (j + 1 == 512 && i + 1 != 11 && (ramFDD144[i + 1][0] != 0 || ramFDD144[i + 1][1] != 0 || ramFDD144[i][j] != 0)) {
                unsigned char a = ramFDD144[i][j];
                unsigned char b = ramFDD144[i + 1][0];
                unsigned char c = ramFDD144[i + 1][1];
                if (((b & 0x0F) << 8) | a != 0) {
                    fat_item[count].pointTo = ((b & 0x0F) << 8) | a;
                    ++count;
                }
                if ((c << 4) | ((b >> 4) & 0x0F) != 0) {
                    fat_item[count].pointTo = (c << 4) | ((b >> 4) & 0x0F);
                    ++count;
                }
                j = -1;
                i += 1;
            }
            if (j + 2 == 512 && i + 1 != 11 && (ramFDD144[i][j] != 0 || ramFDD144[i][j + 1] != 0 || ramFDD144[i + 1][0] != 0)) {
                unsigned char a = ramFDD144[i][j];
                unsigned char b = ramFDD144[i][j + 1];
                unsigned char c = ramFDD144[i + 1][0];
                if (((b & 0x0F) << 8) | a != 0) {
                    fat_item[count].pointTo = ((b & 0x0F) << 8) | a;
                    ++count;
                }
                if ((c << 4) | ((b >> 4) & 0x0F) != 0) {
                    fat_item[count].pointTo = (c << 4) | ((b >> 4) & 0x0F);
                    ++count;
                }
                j = -2;
                i += 1;
            }
        }
    }
    return count;
}

void Read_ramFDD_to_BPB(BPB *bpb) {
    for (int i = 0; i < 3; ++i) {
        bpb->BS_impBoot[i] = ramFDD144[0][i];
    }
    bpb->BS_impBoot[3] = '\0';
    for (int i = 0; i < 8; ++i) {
        bpb->BS_OEMName[i] = ramFDD144[0][i + 3];
    }
    bpb->BS_OEMName[8] = '\0';
    BytsPerSec = char_to_uint(0, 11, 2);
    bpb->BPB_BytsPerSec = BytsPerSec;
    SecPerClus = char_to_uint(0, 13, 1);
    bpb->BPB_SecPerClus = SecPerClus;
    RsvdSecCnt = char_to_uint(0, 14, 2);
    bpb->BPB_RsvdSecCnt = RsvdSecCnt;
    NumFATs = char_to_uint(0, 16, 1);
    bpb->BPB_NumFATs = NumFATs;
    RootEntCnt = char_to_uint(0, 17, 2);
    bpb->BPB_RootEntCnt = RootEntCnt;
    TotSec16 = char_to_uint(0, 19, 2);
    bpb->BPB_TotSec16 = TotSec16;
    Media = char_to_uint(0, 21, 1);
    bpb->BPB_Media = Media;
    FATSz16 = char_to_uint(0, 22, 2);
    bpb->BPB_FATSz16 = FATSz16;
    SecPerTrk = char_to_uint(0, 24, 2);
    bpb->BPB_SecPerTrk = SecPerTrk;
    bpb->BPB_NumHeads = char_to_uint(0, 26, 2);
    bpb->BPB_NumHeads = NumHeads;
    HiddSec = char_to_uint(0, 28, 4);
    bpb->BPB_HiddSec = HiddSec;
    TotSec32 = char_to_uint(0, 32, 4);
    bpb->BPB_TotSec32 = TotSec32;
    DrvNum = char_to_uint(0, 36, 1);
    bpb->BPB_DrvNum = DrvNum;
    Reserved1 = char_to_uint(0, 37, 1);
    bpb->BPB_Reserved1 = Reserved1;
    BootSig = char_to_uint(0, 38, 1);
    bpb->BPB_BootSig = BootSig;
    VoID = char_to_uint(0, 39, 4);
    bpb->BS_VoID = VoID;
    for (int i = 0; i < 11; ++i) {
        bpb->BS_VolLab[i] = ramFDD144[0][i + 43];
    }
    bpb->BS_VolLab[11] = '\0';
    for (int i = 0; i < 8; ++i) {
        bpb->BS_FileSysType[i] = ramFDD144[0][i + 54];
    }
    bpb->BS_FileSysType[8] = '\0';
    // printf("Success to transfer to BPS!\n");
}

void printBPB(BPB *bpb) {
    printf("BS_OEMName: ");
    for (int i = 0; i < 8; ++i) {
        printf("%c", bpb->BS_OEMName[i]);
    }
    printf("\nBPB_BytsPerSec: 0x%02x\n", bpb->BPB_BytsPerSec);
    printf("BPB_SecPerClus: 0x%02x\n", bpb->BPB_SecPerClus);
    printf("BPB_RsvdSecCnt: 0x%02x\n", bpb->BPB_RsvdSecCnt);
    printf("BPB_NumFATs: 0x%02x\n", bpb->BPB_NumFATs);
    printf("BPB_RootEntCnt: 0x%02x\n", bpb->BPB_RootEntCnt);
    printf("BPB_TotSec16: 0x%02x\n", bpb->BPB_TotSec16);
    printf("BPB_Media: 0x%01x\n", bpb->BPB_Media);
    printf("BPB_FATSz16: 0x%02x\n", bpb->BPB_FATSz16);
    printf("BPB_SecPerTrk: 0x%02x\n", bpb->BPB_SecPerTrk);
    printf("BPB_NumHeads: 0x%02x\n", bpb->BPB_NumHeads);
    printf("BPB_HiddSec: 0x%04x\n", bpb->BPB_HiddSec);
    printf("BPB_TotSec32: 0x%04x\n", bpb->BPB_TotSec32);
    printf("BPB_DrvNum: 0x%01x\n", bpb->BPB_DrvNum);
    printf("BPB_Reserved1: 0x%01x\n", bpb->BPB_Reserved1);
    printf("BPB_BootSig: 0x%01x\n", bpb->BPB_BootSig);
    printf("BS_VoID: 0x%04x\n", bpb->BS_VoID);
    printf("BS_VolLab: ");
    for (int i = 0; i < 11; ++i) {
        printf("%c", bpb->BS_VolLab[i]);
    }
    printf("\nBS_FileSysType: ");
    for (int i = 0; i < 8; ++i) {
        printf("%c", bpb->BS_FileSysType[i]);
    }
    printf("\n");
}

void printPerDirItem(DirItem *di) {  // 时间有错误，需要修改
    Date date;
    date.year = (di->dwWrtDate & 0b1111111000000000) >> 10;
    date.month = (di->dwWrtDate & 0b111100000) >> 5;
    date.day = (di->dwWrtDate & 0b11111);
    date.hour = (di->dwWrtTime & 0b1111100000000000) >> 11;
    date.minute = (di->dwWrtTime & 0b11111100000) >> 5;
    date.sec = (di->dwWrtTime & 0b11111);
    if (di->dwDirAttr == 0x10 || di->dwDirAttr == 0x17)
        printf("%s\t<dir>\t20%02d/%02d/%02d %02d:%02d:%02d\t0x%02x\n", di->szDirName, date.year, date.month, date.day,
               date.hour, date.minute, date.sec, di->dwFirstClus);
    if (di->dwDirAttr == 0x20 || di->dwDirAttr == 0x27)
        printf("%s\t\t20%02d/%02d/%02d %02d:%02d:%02d\t0x%02x\t%d B\n", di->szDirName, date.year, date.month, date.day,
               date.hour, date.minute, date.sec, di->dwFirstClus, di->dwFileSize);
}

void printPerDirItemName(int RootSecIndex, int offset, int level) {
    if (level == 0)
        printf("|");
    for (int i = level; i > 0; --i) {
        if (i == 1)
            printf("|-----");
        else
            printf("      ");
    }
    for (int i = 0; i < 11; ++i) {
        printf("%c", ramFDD144[RootSecIndex][offset * 32 + i]);
    }
    printf("\n");
}

int findDirByFileName(DirItem *di, int SecIndex, char *filename) {
    int minlen = strlen(filename);
    if (minlen >= 11)
        minlen = 11;
    for (int i = 0; i < 16; ++i) {
        LoadDirItem(di, SecIndex, i);
        int dotNumber = 0;
        int dotIndex = 0;
        for (int k = 0; k < minlen; ++k) {
            if (filename[0] == '.')
                break;
            if (filename[k] == '.') {
                dotNumber += 1;
                dotIndex = k;
            }
            if (dotNumber > 1) {
                printf("Please enter correct file name!\n");
                return 1;
            }
        }
        if (dotNumber == 0) {
            int sameCount = 0;
            if (strncmp(di->szDirName, filename, minlen) == 0) {
                for (int k = minlen; k < 11; ++k) {
                    if (di->szDirName[k] == 0x20) {
                        sameCount += 1;
                    }
                }
                if (sameCount == 11 - minlen)
                    return i + 1;
            }
        } else {
            int sameCount = 0;
            for (int k = 0; k < dotIndex; ++k) {
                if (di->szDirName[k] == filename[k])
                    sameCount += 1;
            }
            for (int k = 0; k < minlen - 1 - dotIndex; ++k) {
                if (di->szDirName[10 - k] == filename[minlen - 1 - k])
                    sameCount += 1;
            }
            for (int k = dotIndex; k < 11 - (minlen - 1 - dotIndex); ++k) {
                if (di->szDirName[k] == 0x20)
                    sameCount += 1;
            }
            if (sameCount == 11)
                return i + 1;
        }
    }
    printf("No this file!\n");
    return 0;
}

void printRootDir(int RootSecIndex) {
    int offset = 0;
    while (ramFDD144[RootSecIndex][offset * 32] != 0) {
        DirItem *di;
        LoadDirItem(di, RootSecIndex, offset);
        if (di->szDirName[0] != 0xE5 && di->dwDirAttr != 0x17 && di->dwDirAttr != 0x27)
            printPerDirItem(di);
        ++offset;
        if (offset >= 16 && fat_item[RootSecIndex - 31].pointTo != 0xfff) {
            // printf("fat_item[RootSecIndex].pointTo :%03x\n", fat_item[RootSecIndex-31].pointTo);
            RootSecIndex = fat_item[RootSecIndex - 31].pointTo + 31;
            offset = 0;
        }
    }
}

int findDirInCur(char *Dir, char *oldDir, int CurSec) {
    char subDir[100];
    int len = strlen(Dir);
    int oldDirLen = strlen(oldDir);
    if (Dir[0] == '.' && Dir[1] == '\0') {
        return CurSec;
    }
    if (Dir[1] != '.' && Dir[0] != '.') {
        DirItem dim;
        DirItem *di = &dim;
        for (int i = 0; i < 16; ++i) {
            LoadDirItem(di, CurSec, i);
            int sameCount = 0;
            // for(int j = 0; j < len ; ++j) printf("%d[%d] %02x vs. %02x\n",i, j, di->szDirName[j], Dir[j]);
            if (strncmp(di->szDirName, Dir, len) == 0) {
                for (int j = len; j < 11; ++j) {
                    if (di->szDirName[j] == 0x20) {
                        sameCount += 1;
                        // printf("Try: sameCount + len: %d\n", sameCount + len);
                    }
                }
                if (sameCount == 11 - len) {
                    if (di->dwDirAttr != 0x10 && di->dwDirAttr != 0x17) {
                        printf("Error: Not a dir!\n");
                        return CurSec;
                    }
                    if (oldDirLen != 1) {
                        oldDir[oldDirLen] = '/';
                        oldDir[oldDirLen + 1] = '\0';
                    }
                    strcat(oldDir, Dir);
                    CurSec = di->dwFirstClus;
                    return CurSec + 31;
                }
            }
            if (i == 15) {
                printf("Error: findDirInCur can't find this dir!\n");
                return CurSec;
            }
        }
    }
    if (Dir[1] == '.' && Dir[0] == '.' && Dir[2] == '\0') {
        unsigned int newCurSec = char_to_uint(CurSec, 58, 2);
        char temp1 = ramFDD144[CurSec][32];
        char temp2 = ramFDD144[CurSec][33];
        if (temp1 != '.' || temp2 != '.') {
            printf("Root has no father!\n");
            return CurSec;
        }
        for (int i = oldDirLen - 1; i >= 0; --i) {
            if (oldDir[i] == '/' && i != 0) {
                oldDir[i] = '\0';
                break;
            }
            if (i == 0)
                oldDir[1] = '\0';
        }
        if (newCurSec == 0)
            return 19;
        int newCurSec1 = newCurSec + 31;
        return newCurSec1;
    }
}

void treePrint(int RootSecIndex, int level) {
    int offset = 0;
    DirItem dim;
    DirItem *di = &dim;
    int tempSec = 0;
    int addTime = 0;
    while (ramFDD144[RootSecIndex][offset * 32] != 0 && offset < 16 && addTime < 14) {
        LoadDirItem(di, RootSecIndex, offset);
        if (di->szDirName[0] == 0xE5) {
            ++offset;
            continue;
        }
        if (di->dwDirAttr == 0x27 || di->dwDirAttr == 0x20)
            printPerDirItemName(RootSecIndex, offset, level);
        else if (di->dwDirAttr == 0x10) {
            if (di->szDirName[0] != '.') {
                printPerDirItemName(RootSecIndex, offset, level);
                tempSec = di->dwFirstClus + 31;
                treePrint(tempSec, level + 1);
            }
        } else {
            break;
        }
        ++offset;
        if (level == 0 && addTime < 14 && offset == 16) {
            offset = 0;
            addTime += 1;
            RootSecIndex += 1;
        }
    }
}

int setActivativeFile(DirItem *di, char *currentDir) {
    for (int i = 0; i < MAX_ACTIVE_FILES; ++i) {
        if (active_file[i].empty == false && strcmp(active_file[i].f_dir->szDirName, di->szDirName) == 0 && strcmp(active_file[i].f_path, currentDir) == 0) {
            printf("The file has been opened!\n");
            break;
        }
        if (active_file[i].empty == true) {
            active_file[i].empty = false;
            active_file[i].f_dir = di;
            active_file[i].f_path = currentDir;
            active_file[i].share_counter = active_file[i].share_counter + 1;
            CurrentActiveFileNumber++;
            return i;
        }
    }
    return MAX_ACTIVE_FILES;
}

void deleteActivativeFile(int activateFileIndex) {
    active_file[activateFileIndex].empty = true;
    CurrentActiveFileNumber--;
}

void printFileContext(int activateFileIndex) {
    unsigned int newSecIndex = active_file[activateFileIndex].f_dir->dwFirstClus + 31;
    unsigned int fileSize = active_file[activateFileIndex].f_dir->dwFileSize;
    for (int i = 0; i < fileSize; ++i) {
        // if (i != 0 && i % 512 == 0) {
        //     newSecIndex += 1;
        //     i = 0;
        // }
        printf("%c", ramFDD144[newSecIndex][i]);
    }
    printf("\n");
}

int copy_context_to_array(int activateFileIndex, char *originFile) {
    unsigned int newSecIndex = active_file[activateFileIndex].f_dir->dwFirstClus + 31;
    unsigned int fileSize = active_file[activateFileIndex].f_dir->dwFileSize;
    int count = 0;
    for (int i = 0; i < fileSize; ++i) {
        // if (i != 0 && i % 512 == 0) {
        //     newSecIndex += 1;
        //     i = 0;
        // }
        originFile[count++] = ramFDD144[newSecIndex][i];
    }
    return fileSize;
}

void editExistFile(int activateFileIndex, int CurSec, int offset) {
    unsigned int newSecIndex = active_file[activateFileIndex].f_dir->dwFirstClus + 31;
    unsigned int fileSize = active_file[activateFileIndex].f_dir->dwFileSize;
    unsigned int tempnewSecIndex = newSecIndex;
    for (int i = 0; i < fileSize; ++i) {
        if (i != 0 && i % 512 == 0) {
            tempnewSecIndex += 1;
            i = 0;
        }
        ramFDD144[tempnewSecIndex][i] = 0;
    }
    unsigned int c = getchar();
    int Allcount = 0;
    int count = 0;
    while (true) {
        c = getchar();
        if (c == 26 || c == -1)
            break;
        if (count % 512 == 0 && count != 0) {
            count = 0;
            newSecIndex += 1;
        }
        ramFDD144[newSecIndex][count] = c;
        ++Allcount;
        ++count;
    }
    active_file[activateFileIndex].f_dir->dwFileSize = Allcount;
    if (count > 0xFF) {
        ramFDD144[CurSec][offset * 32 + 28] = Allcount % 0x100;
        ramFDD144[CurSec][offset * 32 + 29] = Allcount / 0x100;
    } else {
        ramFDD144[CurSec][offset * 32 + 28] = Allcount;
    }
    time_t timep;
    struct tm *p;
    time(&timep);
    p = gmtime(&timep);
    if (8 + p->tm_hour >= 24) {
        p->tm_hour = -8;
        p->tm_mday += 1;
        if ((((((p->tm_year + 1900) % 4 == 0) && ((p->tm_year + 1900) % 100 != 0)) || ((p->tm_year + 1900) % 400 == 0)) && (p->tm_mday > month_day_special[p->tm_mon])) ||
            (!((((p->tm_year + 1900) % 4 == 0) && ((p->tm_year + 1900) % 100 != 0)) || ((p->tm_year + 1900) % 400 == 0)) && (p->tm_mday > month_day_normal[p->tm_mon]))) {
            p->tm_mday = 1;
            p->tm_mon += 1;
            if (p->tm_mon + 1 > 12) {
                p->tm_mon = 0;
                p->tm_yday += 1;
            }
        }
    }
    unsigned short a = p->tm_mday & 0b11111;
    unsigned short b = ((1 + p->tm_mon) << 5) & 0b111100000;
    unsigned short d = ((p->tm_year - 100) << 10) & 0b1111111000000000;
    unsigned short tempDate = a | b | d;
    active_file[activateFileIndex].f_dir->dwWrtDate = tempDate;
    if (tempDate > 0xFF) {
        ramFDD144[CurSec][offset * 32 + 24] = tempDate % 0x100;
        ramFDD144[CurSec][offset * 32 + 25] = tempDate / 0x100;
    } else {
        ramFDD144[CurSec][offset * 32 + 24] = tempDate;
    }

    a = p->tm_sec & 0b11111;
    b = (p->tm_min << 5) & 0b11111100000;
    d = ((8 + p->tm_hour) << 11) & 0b1111100000000000;
    unsigned short tempTime = a | b | d;
    active_file[activateFileIndex].f_dir->dwWrtTime = tempTime;
    if (tempTime > 0xFF) {
        ramFDD144[CurSec][offset * 32 + 22] = tempTime % 0x100;
        ramFDD144[CurSec][offset * 32 + 23] = tempTime / 0x100;
    } else {
        ramFDD144[CurSec][offset * 32 + 22] = tempTime;
    }
    // 时间还没有改！！
    // active_file[activateFileIndex].f_dir->dwWrtDate = 0;
    // active_file[activateFileIndex].f_dir->dwWrtTime = 0;
}

void addToExistFile(char *originFile, int originFileSize, int activateFileIndex, int CurSec, int offset, const char flag) {
    unsigned int newSecIndex = active_file[activateFileIndex].f_dir->dwFirstClus + 31;
    unsigned int fileSize = active_file[activateFileIndex].f_dir->dwFileSize;
    unsigned int tempnewSecIndex = newSecIndex;
    int Allcount;
    int count;
    if (flag == 'c') {
        Allcount = fileSize;
        count = fileSize;
    }
    if (flag == 'o') {
        Allcount = 0;
        count = 0;
    }
    for (int i = 0; i < originFileSize; ++i) {
        if (count % 512 == 0 && count != 0) {
            break;
        }
        ramFDD144[newSecIndex][count] = originFile[i];
        ++Allcount;
        ++count;
    }
    active_file[activateFileIndex].f_dir->dwFileSize = Allcount;
    if (count > 0xFF) {
        ramFDD144[CurSec][offset * 32 + 28] = Allcount % 0x100;
        ramFDD144[CurSec][offset * 32 + 29] = Allcount / 0x100;
    } else {
        ramFDD144[CurSec][offset * 32 + 28] = Allcount;
    }
    time_t timep;
    struct tm *p;
    time(&timep);
    p = gmtime(&timep);
    if (8 + p->tm_hour >= 24) {
        p->tm_hour = -8;
        p->tm_mday += 1;
        if ((((((p->tm_year + 1900) % 4 == 0) && ((p->tm_year + 1900) % 100 != 0)) || ((p->tm_year + 1900) % 400 == 0)) && (p->tm_mday > month_day_special[p->tm_mon])) ||
            (!((((p->tm_year + 1900) % 4 == 0) && ((p->tm_year + 1900) % 100 != 0)) || ((p->tm_year + 1900) % 400 == 0)) && (p->tm_mday > month_day_normal[p->tm_mon]))) {
            p->tm_mday = 1;
            p->tm_mon += 1;
            if (p->tm_mon + 1 > 12) {
                p->tm_mon = 0;
                p->tm_yday += 1;
            }
        }
    }
    unsigned short a = p->tm_mday & 0b11111;
    unsigned short b = ((1 + p->tm_mon) << 5) & 0b111100000;
    unsigned short d = ((p->tm_year - 100) << 10) & 0b1111111000000000;
    unsigned short tempDate = a | b | d;
    active_file[activateFileIndex].f_dir->dwWrtDate = tempDate;
    if (tempDate > 0xFF) {
        ramFDD144[CurSec][offset * 32 + 24] = tempDate % 0x100;
        ramFDD144[CurSec][offset * 32 + 25] = tempDate / 0x100;
    } else {
        ramFDD144[CurSec][offset * 32 + 24] = tempDate;
    }

    a = p->tm_sec & 0b11111;
    b = (p->tm_min << 5) & 0b11111100000;
    d = ((8 + p->tm_hour) << 11) & 0b1111100000000000;
    unsigned short tempTime = a | b | d;
    active_file[activateFileIndex].f_dir->dwWrtTime = tempTime;
    if (tempTime > 0xFF) {
        ramFDD144[CurSec][offset * 32 + 22] = tempTime % 0x100;
        ramFDD144[CurSec][offset * 32 + 23] = tempTime / 0x100;
    } else {
        ramFDD144[CurSec][offset * 32 + 22] = tempTime;
    }
}

void RemoveFileWithDirItem(int CurSec, char *filename) {
    DirItem dim;
    DirItem *di = &dim;
    int tempFindFile = findDirByFileName(di, CurSec, filename);
    if (di->dwDirAttr != 0x20 && di->dwDirAttr != 0x27) {
        printf("Please enter the currect file name.\n");
        return;
    }
    if (tempFindFile == 0)
        return;
    ramFDD144[CurSec][(tempFindFile - 1) * 32] = 0xE5;
    int listDeleteSec = di->dwFirstClus;
    while (listDeleteSec != 0xfff) {
        fat_item[listDeleteSec].OriginSec = CurSec;
        fat_item[listDeleteSec].isDelete = true;
        listDeleteSec = fat_item[listDeleteSec].pointTo;
    }
    printf("Succeed to delect %s\n", filename);
}

void RemoveEmptyDirWithDirItem(int CurSec, char *filename) {
    DirItem dim;
    DirItem *di = &dim;
    int tempFindFile = findDirByFileName(di, CurSec, filename);
    if (di->dwDirAttr != 0x10 && di->dwDirAttr != 0x17) {
        printf("Please enter the currect folder name.\n");
        return;
    }
    if (tempFindFile == 0)
        return;
    DirItem tempdim;
    DirItem *tempdi = &dim;
    int subCurSec = di->dwFirstClus + 31;
    if (ramFDD144[subCurSec][2 * 32] != 0x00) {
        printf("Can't delete a folder with files.\n");
        return;
    }
    ramFDD144[CurSec][(tempFindFile - 1) * 32] = 0xE5;
    int listDeleteSec = di->dwFirstClus;
    printf("Succeed to delect %s\n", filename);
}

AddFileTODO *SearchRecentDelete(AddFileTODO *result) {
    int mostSmallDate = 65536;
    int mostSmallTime = 65536;
    for (int i = 0; i < MAX_FAT_ITEMS; ++i) {
        // if(fat_item[i].isDelete && )
        if (fat_item[i].isDelete) {
            result->fromSec = fat_item[i].OriginSec;
            result->toSec = i + 31;
            result->searchCount += 1;
            fat_item[i].isDelete = false;
            return result;
        }
    }
    return result;
}

void clearDeletedSec(AddFileTODO *result) {
    DirItem dim;
    DirItem *di = &dim;
    if (result->fromSec == 19) {
        for (int i = 0; i < 14; ++i) {
            for (int k = 0; k < 16; ++k) {
                LoadDirItem(di, result->fromSec + i, k);
                if (di->dwFirstClus == result->toSec - 31) {
                    for (int j = 0; j < 32; ++j)
                        ramFDD144[result->fromSec + i][k * 32 + j] = 0x00;
                }
            }
        }
    } else {
        for (int i = 0; i < 16; ++i) {
            LoadDirItem(di, result->fromSec, i);
            if (di->dwFirstClus == result->toSec - 31) {
                for (int j = 0; j < 32; ++j)
                    ramFDD144[result->fromSec][i * 32 + j] = 0x00;
            }
        }
    }
}

void updateDeletedDirContext(int Sec, int newSec, int newoffset) {
    for (int i = 0; i < 512; ++i) {
        ramFDD144[Sec][i] = 0x00;
    }
    DirItem dim;
    DirItem *di = &dim;
    LoadDirItem(di, newSec, newoffset);
    ramFDD144[Sec][0] = 0X2E;
    ramFDD144[Sec][32] = 0x2E;
    ramFDD144[Sec][33] = 0x2E;
    for (int i = 1; i < 11; ++i)
        ramFDD144[Sec][i] = 0x20;
    for (int i = 34; i < 43; ++i)
        ramFDD144[Sec][i] = 0x20;
    // for(int i = 0; i < 21; ++i) printf("%02x ", ramFDD144[newSec][newoffset * 32 + 11 + i]);
    memcpy(ramFDD144[Sec] + 11, ramFDD144[newSec] + newoffset * 32 + 11, 21);
    if (newSec != 19) {
        memcpy(ramFDD144[Sec] + 32 + 11, ramFDD144[newSec] + 11, 11);
        memcpy(ramFDD144[Sec] + 32 + 22, ramFDD144[newSec] + newoffset * 32 + 22, 4);
        memcpy(ramFDD144[Sec] + 32 + 26, ramFDD144[newSec] + 26, 6);
    } else {
        for (int i = 0; i < 14; ++i) {
            for (int j = 0; j < 16; ++j) {
                LoadDirItem(di, 19 + i, j);
                // printf("%s\n", di->szDirName);
                if ((di->dwDirAttr == 0x10 || di->dwDirAttr == 0x17) && di->szDirName[0] != 0xE5) {
                    i = 15;
                    break;
                }
                if (i == 13 && j == 15) {
                    printf("Root has no dir!");
                    break;
                }
            }
        }
        int keepClus = di->dwFirstClus;
        for (int i = 0; i < 16; ++i) {
            LoadDirItem(di, keepClus + 31, i);
            if (di->szDirName[0] == '.' && di->szDirName[1] == '.') {
                memcpy(ramFDD144[Sec] + 32 + 11, ramFDD144[keepClus + 31] + i * 32 + 11, 11);
                memcpy(ramFDD144[Sec] + 32 + 22, ramFDD144[newSec] + newoffset * 32 + 22, 4);
                memcpy(ramFDD144[Sec] + 32 + 26, ramFDD144[keepClus + 31] + i * 32 + 26, 6);
            }
        }
    }
}

void updateDeletedFileContext(int Sec) {
    for (int i = 0; i < 512; ++i) {
        ramFDD144[Sec][i] = 0x00;
    }
}

void addFATItems(int maxList) {
    maxList = maxList - 1;
    fat_item[maxList].pointTo = 0xFFF;
    // fat_item[maxList].pointTo = 0xFFF;
    int AlreadyOK = 0;
    int Secoffset = 0;
    int Byteoffset = 0;
    if ((maxList + 1) % 2 != 0) {
        AlreadyOK = (maxList + 1) / 2 * 3;
        Secoffset = AlreadyOK / 512;
        Byteoffset = AlreadyOK % 512;
        ramFDD144[1 + Secoffset][Byteoffset] = 0xFF;
        ramFDD144[10 + Secoffset][Byteoffset] = 0xFF;
        if (Byteoffset == 511) {
            ramFDD144[2 + Secoffset][0] = 0x0F;
            ramFDD144[12 + Secoffset][0] = 0x0F;
        } else {
            ramFDD144[1 + Secoffset][Byteoffset + 1] = 0x0F;
            ramFDD144[11 + Secoffset][Byteoffset + 1] = 0x0F;
        }
    } else {
        AlreadyOK = (maxList + 1) / 2 * 3;
        Secoffset = AlreadyOK / 512;
        Byteoffset = AlreadyOK % 512;
        if (Byteoffset == 0) {
            ramFDD144[Secoffset][511] = 0xFF;
            ramFDD144[10 + Secoffset][511] = 0xFF;
            ramFDD144[Secoffset][510] = 0xFF;
            ramFDD144[10 + Secoffset][510] = 0xFF;
        } else if (Byteoffset == 1) {
            ramFDD144[1 + Secoffset][Byteoffset - 1] = 0xFF;
            ramFDD144[11 + Secoffset][Byteoffset - 1] = 0xFF;
            ramFDD144[Secoffset][511] = 0xFF;
            ramFDD144[10 + Secoffset][511] = 0xFF;
        } else {
            ramFDD144[1 + Secoffset][Byteoffset - 1] = 0xFF;
            ramFDD144[11 + Secoffset][Byteoffset - 1] = 0xFF;
            ramFDD144[1 + Secoffset][Byteoffset - 2] = 0xFF;
            ramFDD144[11 + Secoffset][Byteoffset - 2] = 0xFF;
        }
    }
}

void MakeEmptyDir(int CurSec, char *filename) {
    // 找到当前放入空位置，设置名字、大小、时间
    // 检索全树，找到被删除的数据块后，拷贝其数据块位置，进入该位置，修改. 和 ..（上一级的数据块信息）
    DirItem dim;
    DirItem *di = &dim;
    int filenameLen = strlen(filename);
    int sameCount = 0;
    for (int i = 0; i < filenameLen; ++i) {
        if (filename[i] == '.' || filename[i] == ' ' || filename[i] == '\t' || filename[i] == '\n') {
            printf("Dictionary's name has invalid character.\n");
            return;
        }
    }
    if (CurSec == 19) {
        for (int i = 0; i < 14; ++i) {
            for (int j = 0; j < 16; ++j) {
                LoadDirItem(di, CurSec + i, j);
                if (strncmp(filename, di->szDirName, filenameLen) == 0) {
                    for (int k = filenameLen; k < 11; ++k) {
                        if (di->szDirName[k] == 0x20)
                            ++sameCount;
                    }
                    if (sameCount == 11 - filenameLen) {
                        printf("This file has been created in this dictionary!\n");
                        return;
                    }
                }
            }
        }
        for (int i = 0; i < 14; ++i) {
            for (int j = 0; j < 16; ++j) {
                if (ramFDD144[CurSec + i][j * 32] == 0x00 || ramFDD144[CurSec + i][j * 32] == 0xE5) {
                    AddFileTODO addfiletodo;
                    AddFileTODO *addfiletodo_ptr = &addfiletodo;
                    addfiletodo_ptr->searchCount = 0;
                    addfiletodo_ptr = SearchRecentDelete(addfiletodo_ptr);
                    if (addfiletodo_ptr->searchCount == 0) {
                        addfiletodo_ptr->toSec = fat_item_num + 31;
                        fat_item_num += 1;
                        addFATItems(fat_item_num);
                    } else
                        clearDeletedSec(addfiletodo_ptr);
                    memcpy(ramFDD144[CurSec + i] + j * 32, filename, filenameLen);
                    for (int k = 0; k < 11 - filenameLen; ++k)
                        ramFDD144[CurSec + i][k + filenameLen + j * 32] = 0x20;
                    ramFDD144[CurSec + i][11 + j * 32] = 0x10;
                    for (int k = 0; k < 10; ++k)
                        ramFDD144[CurSec + i][k + 12 + j * 32] = 0x00;
                    time_t timep;
                    struct tm *p;
                    time(&timep);
                    p = gmtime(&timep);
                    if (8 + p->tm_hour >= 24) {
                        p->tm_hour = -8;
                        p->tm_mday += 1;
                        if ((((((p->tm_year + 1900) % 4 == 0) && ((p->tm_year + 1900) % 100 != 0)) || ((p->tm_year + 1900) % 400 == 0)) && (p->tm_mday > month_day_special[p->tm_mon])) ||
                            (!((((p->tm_year + 1900) % 4 == 0) && ((p->tm_year + 1900) % 100 != 0)) || ((p->tm_year + 1900) % 400 == 0)) && (p->tm_mday > month_day_normal[p->tm_mon]))) {
                            p->tm_mday = 1;
                            p->tm_mon += 1;
                            if (p->tm_mon + 1 > 12) {
                                p->tm_mon = 0;
                                p->tm_yday += 1;
                            }
                        }
                    }
                    unsigned short a = p->tm_mday & 0b11111;
                    unsigned short b = ((1 + p->tm_mon) << 5) & 0b111100000;
                    unsigned short d = ((p->tm_year - 100) << 10) & 0b1111111000000000;
                    unsigned short tempDate = a | b | d;
                    if (tempDate > 0xFF) {
                        ramFDD144[CurSec + i][j * 32 + 24] = tempDate % 0x100;
                        ramFDD144[CurSec + i][j * 32 + 25] = tempDate / 0x100;
                    } else {
                        ramFDD144[CurSec + i][j * 32 + 24] = tempDate;
                    }

                    a = p->tm_sec & 0b11111;
                    b = (p->tm_min << 5) & 0b11111100000;
                    d = ((8 + p->tm_hour) << 11) & 0b1111100000000000;
                    if (8 + p->tm_hour >= 24) {
                    }
                    unsigned short tempTime = a | b | d;
                    if (tempTime > 0xFF) {
                        ramFDD144[CurSec + i][j * 32 + 22] = tempTime % 0x100;
                        ramFDD144[CurSec + i][j * 32 + 23] = tempTime / 0x100;
                    } else {
                        ramFDD144[CurSec + i][j * 32 + 22] = tempTime;
                    }
                    if (addfiletodo_ptr->toSec - 31 > 0xFF) {
                        ramFDD144[CurSec + i][26 + j * 32] = (addfiletodo_ptr->toSec - 31) % 0x100;
                        ramFDD144[CurSec + i][27 + j * 32] = (addfiletodo_ptr->toSec - 31) / 0x100;
                    } else {
                        ramFDD144[CurSec + i][26 + j * 32] = addfiletodo_ptr->toSec - 31;
                        ramFDD144[CurSec + i][27 + j * 32] = 0x00;
                    }
                    for (int k = 0; k < 4; ++k)
                        ramFDD144[CurSec + i][k + 28 + j * 32] = 0x00;
                    updateDeletedDirContext(addfiletodo_ptr->toSec, CurSec + i, j);
                    i = 15;
                    break;
                }
            }
        }
    } else {
        // 判断满: 当前FAT所对应指向一个新的块（分配已删除或新建），进入块中操作
        int EnableNumber = 0;
        int tempPT = CurSec - 31;
        printf("CurSec : %03x\n", CurSec);
        while (tempPT != 0xfff) {
            for (int j = 0; j < 16; ++j)
                if (ramFDD144[tempPT+31][j * 32] == 0x00 || ramFDD144[tempPT+31][j * 32] == 0xE5) EnableNumber += 1;
            CurSec = tempPT;
            tempPT = fat_item[tempPT].pointTo;
            printf("tempPT : %03x, EnableNumber: %d\n", tempPT, EnableNumber);
        }
        CurSec += 31;
        // CurSec = tempPT;
        if (EnableNumber == 0) {
            AddFileTODO addfiletodo;
            AddFileTODO *addfiletodo_ptr = &addfiletodo;
            addfiletodo_ptr->searchCount = 0;
            addfiletodo_ptr = SearchRecentDelete(addfiletodo_ptr);
            if (addfiletodo_ptr->searchCount == 0) {
                addfiletodo_ptr->toSec = fat_item_num + 31;
                fat_item_num += 1;
                addFATItems(fat_item_num);
            } else
                clearDeletedSec(addfiletodo_ptr);
            // fat_item[CurSec].OriginSec = addfiletodo_ptr->fromSec;
            fat_item[CurSec].pointTo = addfiletodo_ptr->toSec - 31;
            CurSec = addfiletodo_ptr->toSec;
        }
        for (int j = 0; j < 16; ++j) {
            LoadDirItem(di, CurSec, j);
            if (strncmp(filename, di->szDirName, filenameLen) == 0) {
                for (int k = filenameLen; k < 11; ++k) {
                    if (di->szDirName[k] == 0x20)
                        ++sameCount;
                }
                if (sameCount == 11 - filenameLen) {
                    printf("This file has been created in this dictionary!\n");
                    return;
                }
            }
        }
        for (int j = 0; j < 16; ++j) {
            if (ramFDD144[CurSec][j * 32] == 0x00 || ramFDD144[CurSec][j * 32] == 0xE5 || ramFDD144[CurSec][j * 32] == 0xF6) {
                AddFileTODO addfiletodo;
                AddFileTODO *addfiletodo_ptr = &addfiletodo;
                addfiletodo_ptr->searchCount = 0;
                addfiletodo_ptr = SearchRecentDelete(addfiletodo_ptr);
                // addfiletodo_ptr = SearchRecentDelete(19, 0, addfiletodo_ptr);
                if (addfiletodo_ptr->searchCount == 0) {
                    addfiletodo_ptr->toSec = fat_item_num + 31;
                    fat_item_num += 1;
                    addFATItems(fat_item_num);
                } else
                    clearDeletedSec(addfiletodo_ptr);
                memcpy(ramFDD144[CurSec] + j * 32, filename, filenameLen);
                for (int k = 0; k < 11 - filenameLen; ++k)
                    ramFDD144[CurSec][k + filenameLen + j * 32] = 0x20;
                ramFDD144[CurSec][11 + j * 32] = 0x10;
                for (int k = 0; k < 10; ++k)
                    ramFDD144[CurSec][k + 12 + j * 32] = 0x00;
                time_t timep;
                struct tm *p;
                time(&timep);
                p = gmtime(&timep);
                if (8 + p->tm_hour >= 24) {
                    p->tm_hour = -8;
                    p->tm_mday += 1;
                    if ((((((p->tm_year + 1900) % 4 == 0) && ((p->tm_year + 1900) % 100 != 0)) || ((p->tm_year + 1900) % 400 == 0)) && (p->tm_mday > month_day_special[p->tm_mon])) ||
                        (!((((p->tm_year + 1900) % 4 == 0) && ((p->tm_year + 1900) % 100 != 0)) || ((p->tm_year + 1900) % 400 == 0)) && (p->tm_mday > month_day_normal[p->tm_mon]))) {
                        p->tm_mday = 1;
                        p->tm_mon += 1;
                        if (p->tm_mon + 1 > 12) {
                            p->tm_mon = 0;
                            p->tm_yday += 1;
                        }
                    }
                }
                unsigned short a = p->tm_mday & 0b11111;
                unsigned short b = ((1 + p->tm_mon) << 5) & 0b111100000;
                unsigned short d = ((p->tm_year - 100) << 10) & 0b1111111000000000;
                unsigned short tempDate = a | b | d;
                if (tempDate > 0xFF) {
                    ramFDD144[CurSec][j * 32 + 24] = tempDate % 0x100;
                    ramFDD144[CurSec][j * 32 + 25] = tempDate / 0x100;
                } else {
                    ramFDD144[CurSec][j * 32 + 24] = tempDate;
                }

                a = p->tm_sec & 0b11111;
                b = (p->tm_min << 5) & 0b11111100000;
                d = ((8 + p->tm_hour) << 11) & 0b1111100000000000;
                unsigned short tempTime = a | b | d;
                if (tempTime > 0xFF) {
                    ramFDD144[CurSec][j * 32 + 22] = tempTime % 0x100;
                    ramFDD144[CurSec][j * 32 + 23] = tempTime / 0x100;
                } else {
                    ramFDD144[CurSec][j * 32 + 22] = tempTime;
                }
                // ramFDD144[CurSec][22 + j * 32] = 0x3B;
                // ramFDD144[CurSec][23 + j * 32] = 0xA2;
                // ramFDD144[CurSec][24 + j * 32] = 0x6A;
                // ramFDD144[CurSec][25 + j * 32] = 0x50;
                if (addfiletodo_ptr->toSec - 31 > 0xFF) {
                    ramFDD144[CurSec][26 + j * 32] = (addfiletodo_ptr->toSec - 31) % 0x100;
                    ramFDD144[CurSec][27 + j * 32] = (addfiletodo_ptr->toSec - 31) / 0x100;
                } else {
                    ramFDD144[CurSec][26 + j * 32] = addfiletodo_ptr->toSec - 31;
                    ramFDD144[CurSec][27 + j * 32] = 0x00;
                }
                for (int k = 0; k < 4; ++k)
                    ramFDD144[CurSec][k + 28 + j * 32] = 0x00;
                // for (int k = 0; k < 32; ++k)
                //     printf("%02x ", ramFDD144[CurSec][k + j * 32]);
                // printf("\n");
                updateDeletedDirContext(addfiletodo_ptr->toSec, CurSec, j);
                break;
            }
        }
    }
}

int MakeEmptyFile(int CurSec, char *filename) {
    // 找到当前放入空位置，设置名字、大小、时间
    // 检索全树，找到被删除的数据块后，拷贝其数据块位置，进入该位置，修改. 和 ..（上一级的数据块信息）
    DirItem dim;
    DirItem *di = &dim;
    int filenameLen = strlen(filename);
    int sameCount = 0;
    int dotNumber = 0;
    int dotIndex = 0;
    for (int k = 0; k < filenameLen; ++k) {
        if (filename[0] == '.')
            return 1;
        if (filename[k] == '.') {
            dotNumber += 1;
            dotIndex = k;
        }
        if (dotNumber > 1) {
            printf("Please enter correct file name!\n");
            return 1;
        }
    }
    if (CurSec == 19) {
        for (int i = 0; i < 14; ++i) {
            for (int j = 0; j < 16; ++j) {
                LoadDirItem(di, CurSec + i, j);
                if (dotNumber == 0) {
                    int sameCount = 0;
                    if (strncmp(di->szDirName, filename, filenameLen) == 0) {
                        for (int k = filenameLen; k < 11; ++k) {
                            if (di->szDirName[k] == 0x20) {
                                sameCount += 1;
                            }
                        }
                        if (sameCount == 11 - filenameLen) {
                            printf("This file has been created in this dictionary!\n");
                            return 1;
                        }
                    }
                } else {
                    int sameCount = 0;
                    for (int k = 0; k < dotIndex; ++k) {
                        if (di->szDirName[k] == filename[k])
                            sameCount += 1;
                    }
                    for (int k = 0; k < filenameLen - 1 - dotIndex; ++k) {
                        if (di->szDirName[10 - k] == filename[filenameLen - 1 - k])
                            sameCount += 1;
                    }
                    for (int k = dotIndex; k < 11 - (filenameLen - 1 - dotIndex); ++k) {
                        if (di->szDirName[k] == 0x20)
                            sameCount += 1;
                    }
                    if (sameCount == 11) {
                        printf("This file has been created in this dictionary!\n");
                        return 1;
                    }
                }
                // if (strncmp(filename, di->szDirName, filenameLen) == 0)
                // {
                //     for (int k = filenameLen; k < 11; ++k)
                //     {
                //         if (di->szDirName[k] == 0x20)
                //             ++sameCount;
                //     }
                //     if (sameCount == 11 - filenameLen)
                //     {
                //         printf("This file has been created in this dictionary!\n");
                //         return 1;
                //     }
                // }
            }
        }
        for (int i = 0; i < 14; ++i) {
            for (int j = 0; j < 16; ++j) {
                if (ramFDD144[CurSec + i][j * 32] == 0x00 || ramFDD144[CurSec + i][j * 32] == 0xE5) {
                    AddFileTODO addfiletodo;
                    AddFileTODO *addfiletodo_ptr = &addfiletodo;
                    addfiletodo_ptr->searchCount = 0;
                    addfiletodo_ptr = SearchRecentDelete(addfiletodo_ptr);
                    // addfiletodo_ptr = SearchRecentDelete(19, 0, addfiletodo_ptr);
                    if (addfiletodo_ptr->searchCount == 0) {
                        addfiletodo_ptr->toSec = fat_item_num + 31;
                        fat_item_num += 1;
                        addFATItems(fat_item_num);
                        // int fat_item_num = Read_FAT_Items();
                        // for(int i = 0 ; i < fat_item_num; ++i) printf("%03x ", fat_item[i]);
                        // printf("fat_item_num: %d\n",fat_item_num);
                        // printf("DEBUG: %d\n",addfiletodo_ptr->toSec);
                    } else
                        clearDeletedSec(addfiletodo_ptr);
                    if (dotNumber == 0) {
                        memcpy(ramFDD144[CurSec + i] + j * 32, filename, filenameLen);
                        for (int k = 0; k < 11 - filenameLen; ++k)
                            ramFDD144[CurSec + i][k + filenameLen + j * 32] = 0x20;
                    } else {
                        for (int k = 0; k < dotIndex; ++k) {
                            ramFDD144[CurSec + i][k + j * 32] = filename[k];
                        }
                        for (int k = 0; k < filenameLen - 1 - dotIndex; ++k) {
                            ramFDD144[CurSec + i][10 - k + j * 32] = filename[filenameLen - 1 - k];
                        }
                        for (int k = dotIndex; k < 11 - (filenameLen - 1 - dotIndex); ++k) {
                            ramFDD144[CurSec + i][k + j * 32] = 0x20;
                        }
                    }
                    ramFDD144[CurSec + i][11 + j * 32] = 0x20;
                    for (int k = 0; k < 10; ++k)
                        ramFDD144[CurSec + i][k + 12 + j * 32] = 0x00;
                    time_t timep;
                    struct tm *p;
                    time(&timep);
                    p = gmtime(&timep);
                    if (8 + p->tm_hour >= 24) {
                        p->tm_hour = -8;
                        p->tm_mday += 1;
                        if ((((((p->tm_year + 1900) % 4 == 0) && ((p->tm_year + 1900) % 100 != 0)) || ((p->tm_year + 1900) % 400 == 0)) && (p->tm_mday > month_day_special[p->tm_mon])) ||
                            (!((((p->tm_year + 1900) % 4 == 0) && ((p->tm_year + 1900) % 100 != 0)) || ((p->tm_year + 1900) % 400 == 0)) && (p->tm_mday > month_day_normal[p->tm_mon]))) {
                            p->tm_mday = 1;
                            p->tm_mon += 1;
                            if (p->tm_mon + 1 > 12) {
                                p->tm_mon = 0;
                                p->tm_yday += 1;
                            }
                        }
                    }
                    unsigned short a = p->tm_mday & 0b11111;
                    unsigned short b = ((1 + p->tm_mon) << 5) & 0b111100000;
                    unsigned short d = ((p->tm_year - 100) << 10) & 0b1111111000000000;
                    unsigned short tempDate = a | b | d;
                    if (tempDate > 0xFF) {
                        ramFDD144[CurSec + i][j * 32 + 24] = tempDate % 0x100;
                        ramFDD144[CurSec + i][j * 32 + 25] = tempDate / 0x100;
                    } else {
                        ramFDD144[CurSec + i][j * 32 + 24] = tempDate;
                    }

                    a = p->tm_sec & 0b11111;
                    b = (p->tm_min << 5) & 0b11111100000;
                    d = ((8 + p->tm_hour) << 11) & 0b1111100000000000;
                    unsigned short tempTime = a | b | d;
                    if (tempTime > 0xFF) {
                        ramFDD144[CurSec + i][j * 32 + 22] = tempTime % 0x100;
                        ramFDD144[CurSec + i][j * 32 + 23] = tempTime / 0x100;
                    } else {
                        ramFDD144[CurSec + i][j * 32 + 22] = tempTime;
                    }
                    // ramFDD144[CurSec + i][22 + j * 32] = 0x3B;
                    // ramFDD144[CurSec + i][23 + j * 32] = 0xA2;
                    // ramFDD144[CurSec + i][24 + j * 32] = 0x6A;
                    // ramFDD144[CurSec + i][25 + j * 32] = 0x50;
                    if (addfiletodo_ptr->toSec - 31 > 0xFF) {
                        ramFDD144[CurSec + i][26 + j * 32] = (addfiletodo_ptr->toSec - 31) % 0x100;
                        ramFDD144[CurSec + i][27 + j * 32] = (addfiletodo_ptr->toSec - 31) / 0x100;
                    } else {
                        ramFDD144[CurSec + i][26 + j * 32] = addfiletodo_ptr->toSec - 31;
                        ramFDD144[CurSec + i][27 + j * 32] = 0x00;
                    }
                    // EDIT
                    // updateDeletedDirContext(addfiletodo_ptr->toSec, CurSec + i, j);
                    for (int k = 0; k < 4; ++k)
                        ramFDD144[CurSec + i][k + 28 + j * 32] = 0x00;
                    updateDeletedFileContext(addfiletodo_ptr->toSec);
                    printf("Succeed creating file. Please enter bytes:\n");
                    i = 15;
                    break;
                }
            }
        }
    } else {
        for (int j = 0; j < 16; ++j) {
            LoadDirItem(di, CurSec, j);
            if (dotNumber == 0) {
                int sameCount = 0;
                if (strncmp(di->szDirName, filename, filenameLen) == 0) {
                    for (int k = filenameLen; k < 11; ++k) {
                        if (di->szDirName[k] == 0x20) {
                            sameCount += 1;
                        }
                    }
                    if (sameCount == 11 - filenameLen) {
                        printf("This file has been created in this dictionary!\n");
                        return 1;
                    }
                }
            } else {
                int sameCount = 0;
                for (int k = 0; k < dotIndex; ++k) {
                    if (di->szDirName[k] == filename[k])
                        sameCount += 1;
                }
                for (int k = 0; k < filenameLen - 1 - dotIndex; ++k) {
                    if (di->szDirName[10 - k] == filename[filenameLen - 1 - k])
                        sameCount += 1;
                }
                for (int k = dotIndex; k < 11 - (filenameLen - 1 - dotIndex); ++k) {
                    if (di->szDirName[k] == 0x20)
                        sameCount += 1;
                }
                if (sameCount == 11) {
                    printf("This file has been created in this dictionary!\n");
                    return 1;
                }
            }
        }
        for (int j = 0; j < 16; ++j) {
            if (ramFDD144[CurSec][j * 32] == 0x00 || ramFDD144[CurSec][j * 32] == 0xE5) {
                AddFileTODO addfiletodo;
                AddFileTODO *addfiletodo_ptr = &addfiletodo;
                addfiletodo_ptr->searchCount = 0;
                addfiletodo_ptr = SearchRecentDelete(addfiletodo_ptr);
                if (addfiletodo_ptr->searchCount == 0) {
                    addfiletodo_ptr->toSec = fat_item_num + 31;
                    fat_item_num += 1;
                    addFATItems(fat_item_num);
                } else
                    clearDeletedSec(addfiletodo_ptr);
                if (dotNumber == 0) {
                    memcpy(ramFDD144[CurSec] + j * 32, filename, filenameLen);
                    for (int k = 0; k < 11 - filenameLen; ++k)
                        ramFDD144[CurSec][k + filenameLen + j * 32] = 0x20;
                } else {
                    for (int k = 0; k < dotIndex; ++k) {
                        ramFDD144[CurSec][k + j * 32] = filename[k];
                    }
                    for (int k = 0; k < filenameLen - 1 - dotIndex; ++k) {
                        ramFDD144[CurSec][10 - k + j * 32] = filename[filenameLen - 1 - k];
                    }
                    for (int k = dotIndex; k < 11 - (filenameLen - 1 - dotIndex); ++k) {
                        ramFDD144[CurSec][k + j * 32] = 0x20;
                    }
                }
                ramFDD144[CurSec][11 + j * 32] = 0x20;
                for (int k = 0; k < 10; ++k)
                    ramFDD144[CurSec][k + 12 + j * 32] = 0x00;
                time_t timep;
                struct tm *p;
                time(&timep);
                p = gmtime(&timep);
                if (8 + p->tm_hour >= 24) {
                    p->tm_hour = -8;
                    p->tm_mday += 1;
                    if ((((((p->tm_year + 1900) % 4 == 0) && ((p->tm_year + 1900) % 100 != 0)) || ((p->tm_year + 1900) % 400 == 0)) && (p->tm_mday > month_day_special[p->tm_mon])) ||
                        (!((((p->tm_year + 1900) % 4 == 0) && ((p->tm_year + 1900) % 100 != 0)) || ((p->tm_year + 1900) % 400 == 0)) && (p->tm_mday > month_day_normal[p->tm_mon]))) {
                        p->tm_mday = 1;
                        p->tm_mon += 1;
                        if (p->tm_mon + 1 > 12) {
                            p->tm_mon = 0;
                            p->tm_yday += 1;
                        }
                    }
                }
                unsigned short a = p->tm_mday & 0b11111;
                unsigned short b = ((1 + p->tm_mon) << 5) & 0b111100000;
                unsigned short d = ((p->tm_year - 100) << 10) & 0b1111111000000000;
                unsigned short tempDate = a | b | d;
                if (tempDate > 0xFF) {
                    ramFDD144[CurSec][j * 32 + 24] = tempDate % 0x100;
                    ramFDD144[CurSec][j * 32 + 25] = tempDate / 0x100;
                } else {
                    ramFDD144[CurSec][j * 32 + 24] = tempDate;
                }

                a = p->tm_sec & 0b11111;
                b = (p->tm_min << 5) & 0b11111100000;
                d = ((8 + p->tm_hour) << 11) & 0b1111100000000000;
                unsigned short tempTime = a | b | d;
                if (tempTime > 0xFF) {
                    ramFDD144[CurSec][j * 32 + 22] = tempTime % 0x100;
                    ramFDD144[CurSec][j * 32 + 23] = tempTime / 0x100;
                } else {
                    ramFDD144[CurSec][j * 32 + 22] = tempTime;
                }
                if (addfiletodo_ptr->toSec - 31 > 0xFF) {
                    ramFDD144[CurSec][26 + j * 32] = (addfiletodo_ptr->toSec - 31) % 0x100;
                    ramFDD144[CurSec][27 + j * 32] = (addfiletodo_ptr->toSec - 31) / 0x100;
                } else {
                    ramFDD144[CurSec][26 + j * 32] = addfiletodo_ptr->toSec - 31;
                    ramFDD144[CurSec][27 + j * 32] = 0x00;
                }
                for (int k = 0; k < 4; ++k)
                    ramFDD144[CurSec][k + 28 + j * 32] = 0x00;
                updateDeletedFileContext(addfiletodo_ptr->toSec);
                printf("Succeed creating file. Please enter bytes:\n");
                break;
            }
        }
    }
    return 0;
}

int SeperateDirAndFileName(char *fileNametemp, char *currentDir, int CurSec, char *fileName) {
    char dirTemp[15];
    int fileNametemplen = strlen(fileNametemp);
    fileNametemp[fileNametemplen] = '\0';
    int marked = 0;
    for (int i = 0; i < fileNametemplen; ++i) {
        if (fileNametemp[i] == '/') {
            memcpy(dirTemp, fileNametemp + marked, i - marked);
            dirTemp[i - marked] = '\0';
            CurSec = findDirInCur(dirTemp, currentDir, CurSec);
            marked = i + 1;
        }
    }
    memcpy(fileName, fileNametemp + marked, fileNametemplen - marked);
    fileName[fileNametemplen - marked] = '\0';
    return CurSec;
}

void CreateUser(int userId) {
    printf("User %d is created!\n", userId);
    printf("Please enter 'help' to read the introdution of this system.\n");
    FILE_OPEN file_open[MAX_OPEN_FILES];
    for (int i = 0; i < MAX_OPEN_FILES; ++i) {
        file_open[i].empty = true;
    }
    char currentDir[100];
    currentDir[0] = '/';
    currentDir[1] = '\0';
    char operation[100];
    int CurSec = 19;
    int CurOpenFileNumber = 0;
    while (true) {
        fat_item_num = Read_FAT_Items();
        for (int i = 0; i < fat_item_num; ++i) {
            if (fat_item[i].isDelete) {
                printf("%d :originSec: %d pointTo: %03x %d\n", i, fat_item[i].OriginSec - 31, fat_item[i].pointTo, fat_item[i].isDelete);
            }
        }
        printf("fat_item_num: %d\n", fat_item_num);
        // setMaxList(19, 0);
        printf("A:%s> ", currentDir);
        scanf("%s", operation);
        if (strcmp(operation, "exit") == 0 || strcmp(operation, "EXIT") == 0) {
            char yn[2];
            printf("Exit? [y] or [n] ");
            scanf("%s", yn);
            if (yn[0] == 'y' || yn[0] == 'Y')
                break;
        } else if (strcmp(operation, "help") == 0 || strcmp(operation, "HELP") == 0) {
            printf("Please use these sentences:\n");
            printf("Tree All Dir: tree\n");
            printf("List current dir: dir\n");
            printf("Come into a new dir: cd+dir\n");
        } else if (strcmp(operation, "dir") == 0 || strcmp(operation, "DIR") == 0) {
            printRootDir(CurSec);
        } else if (strcmp(operation, "cd") == 0 || strcmp(operation, "CD") == 0) {
            char subDir[100];
            char DirArray[100][100];
            scanf("%s", subDir);
            int subDirLen = strlen(subDir);
            for (int i = 0; i < subDirLen; ++i)
                subDir[i] = to_upper_case(subDir[i]);
            int DirArrayCount = 0;
            int marked = 0;
            for (int i = 0; i < subDirLen; ++i) {
                if (subDir[i] == '/') {
                    memcpy(DirArray[DirArrayCount], subDir + marked, i - marked);
                    DirArray[DirArrayCount][i - marked] = '\0';
                    DirArrayCount++;
                    marked = i + 1;
                }
            }
            memcpy(DirArray[DirArrayCount], subDir + marked, subDirLen - marked);
            DirArray[DirArrayCount][subDirLen - marked] = '\0';
            DirArrayCount++;
            for (int i = 0; i < DirArrayCount; ++i) {
                CurSec = findDirInCur(DirArray[i], currentDir, CurSec);
            }
        } else if (strcmp(operation, "tree") == 0 || strcmp(operation, "TREE") == 0) {
            treePrint(19, 0);
        } else if (strcmp(operation, "active") == 0 || strcmp(operation, "ACTIVE") == 0) {
            printf("Current User %d\n-------------------------------\n", userId);
            for (int i = 0; i < CurOpenFileNumber && file_open[i].empty == false; ++i) {
                int tempactivefileIndex = file_open[i].activefile;
                printf("%s\n", active_file[tempactivefileIndex].f_dir->szDirName);
            }
            printf("==================================\nSystem\n-------------------------------\n");
            for (int i = 0; i < CurrentActiveFileNumber && active_file[i].empty == false; ++i) {
                printf("%s\n", active_file[i].f_dir->szDirName);
            }
        } else if (strcmp(operation, "type") == 0 || strcmp(operation, "TYPE") == 0) {
            char fileNametempwithSpase[50];
            char fileName[15];
            char keepCurDir[50];
            memcpy(keepCurDir, currentDir, strlen(currentDir));
            keepCurDir[strlen(currentDir)] = '\0';
            int keepCurSec = CurSec;
            scanf("%s", fileNametempwithSpase);
            int inputLength = strlen(fileNametempwithSpase);
            for (int i = 0; i < inputLength; ++i)
                fileNametempwithSpase[i] = to_upper_case(fileNametempwithSpase[i]);
            CurSec = SeperateDirAndFileName(fileNametempwithSpase, currentDir, CurSec, fileName);
            DirItem dim;
            DirItem *di = &dim;
            int tempFindFile = findDirByFileName(di, CurSec, fileName);
            if (tempFindFile == 0)
                continue;
            bool has_open = false;
            int activateFileIndex = setActivativeFile(di, currentDir);
            // printf("activateFileIndex: %d\n", activateFileIndex);
            if (activateFileIndex == MAX_ACTIVE_FILES) {
                printf("File has been opened!\n");
                continue;
            }
            for (int i = 0; i < CurOpenFileNumber; ++i) {
                if (file_open[i].empty == false && file_open[i].activefile == activateFileIndex) {
                    has_open = true;
                    break;
                }
            }
            if (has_open == true) {
                printf("File has been opened!\n");
                continue;
            } else {
                file_open[CurOpenFileNumber].empty = false;
                file_open[CurOpenFileNumber].pos = 0;
                file_open[CurOpenFileNumber].read_or_write = 0;
                file_open[CurOpenFileNumber].activefile = activateFileIndex;
                CurOpenFileNumber++;
                // printf("Succeed to open file: %s\n",fileName);
                printFileContext(activateFileIndex);
                CurOpenFileNumber--;
                file_open[CurOpenFileNumber].empty = true;
                deleteActivativeFile(activateFileIndex);
            }
            CurSec = keepCurSec;
            memcpy(currentDir, keepCurDir, strlen(keepCurDir));
            currentDir[strlen(keepCurDir)] = '\0';
        } else if (strcmp(operation, "del") == 0 || strcmp(operation, "DEL") == 0) {
            char fileNametempwithSpase[50];
            char fileName[15];
            char keepCurDir[50];
            memcpy(keepCurDir, currentDir, strlen(currentDir));
            keepCurDir[strlen(currentDir)] = '\0';
            int keepCurSec = CurSec;
            // scanf("%[^\n]", fileNametempwithSpase);
            scanf("%s", fileNametempwithSpase);
            int inputLength = strlen(fileNametempwithSpase);
            for (int i = 0; i < inputLength; ++i)
                fileNametempwithSpase[i] = to_upper_case(fileNametempwithSpase[i]);
            CurSec = SeperateDirAndFileName(fileNametempwithSpase, currentDir, CurSec, fileName);
            RemoveFileWithDirItem(CurSec, fileName);
            CurSec = keepCurSec;
            memcpy(currentDir, keepCurDir, strlen(keepCurDir));
            currentDir[strlen(keepCurDir)] = '\0';
        } else if (strcmp(operation, "rmdir") == 0 || strcmp(operation, "RMDIR") == 0) {
            char fileNametempwithSpase[50];
            char fileName[15];
            char keepCurDir[50];
            memcpy(keepCurDir, currentDir, strlen(currentDir));
            keepCurDir[strlen(currentDir)] = '\0';
            int keepCurSec = CurSec;
            scanf("%s", fileNametempwithSpase);
            int inputLength = strlen(fileNametempwithSpase);
            for (int i = 0; i < inputLength; ++i)
                fileNametempwithSpase[i] = to_upper_case(fileNametempwithSpase[i]);
            CurSec = SeperateDirAndFileName(fileNametempwithSpase, currentDir, CurSec, fileName);
            RemoveEmptyDirWithDirItem(CurSec, fileName);
            CurSec = keepCurSec;
            memcpy(currentDir, keepCurDir, strlen(keepCurDir));
            currentDir[strlen(keepCurDir)] = '\0';
        } else if (strcmp(operation, "edit") == 0 || strcmp(operation, "EDIT") == 0) {
            char fileNametempwithSpase[50];
            char fileName[15];
            char keepCurDir[50];
            memcpy(keepCurDir, currentDir, strlen(currentDir));
            keepCurDir[strlen(currentDir)] = '\0';
            int keepCurSec = CurSec;
            scanf("%s", fileNametempwithSpase);
            int inputLength = strlen(fileNametempwithSpase);
            for (int i = 0; i < inputLength; ++i)
                fileNametempwithSpase[i] = to_upper_case(fileNametempwithSpase[i]);
            CurSec = SeperateDirAndFileName(fileNametempwithSpase, currentDir, CurSec, fileName);
            DirItem dim;
            DirItem *di = &dim;
            int tempFindFile = findDirByFileName(di, CurSec, fileName);
            if (tempFindFile == 0)
                continue;
            bool has_open = false;
            int activateFileIndex = setActivativeFile(di, currentDir);
            // printf("activateFileIndex: %d\n", activateFileIndex);
            if (activateFileIndex == MAX_ACTIVE_FILES) {
                printf("File has been opened!\n");
                continue;
            }
            for (int i = 0; i < CurOpenFileNumber; ++i) {
                if (file_open[i].empty == false && file_open[i].activefile == activateFileIndex) {
                    has_open = true;
                    break;
                }
            }
            if (has_open == true) {
                printf("File has been opened!\n");
                continue;
            } else {
                file_open[CurOpenFileNumber].empty = false;
                file_open[CurOpenFileNumber].pos = 0;
                file_open[CurOpenFileNumber].read_or_write = 1;
                file_open[CurOpenFileNumber].activefile = activateFileIndex;
                CurOpenFileNumber++;
                // printf("Succeed to open file: %s\n", fileName);
                editExistFile(activateFileIndex, CurSec, tempFindFile - 1);
                CurOpenFileNumber--;
                file_open[CurOpenFileNumber].empty = true;
                deleteActivativeFile(activateFileIndex);
            }
            CurSec = keepCurSec;
            memcpy(currentDir, keepCurDir, strlen(keepCurDir));
            currentDir[strlen(keepCurDir)] = '\0';
        } else if (strcmp(operation, "mkdir") == 0 || strcmp(operation, "MKDIR") == 0) {
            char fileNametempwithSpase[50];
            char fileName[15];
            char keepCurDir[50];
            memcpy(keepCurDir, currentDir, strlen(currentDir));
            keepCurDir[strlen(currentDir)] = '\0';
            int keepCurSec = CurSec;
            scanf("%s", fileNametempwithSpase);
            int inputLength = strlen(fileNametempwithSpase);
            for (int i = 0; i < inputLength; ++i)
                fileNametempwithSpase[i] = to_upper_case(fileNametempwithSpase[i]);
            CurSec = SeperateDirAndFileName(fileNametempwithSpase, currentDir, CurSec, fileName);
            MakeEmptyDir(CurSec, fileName);
            CurSec = keepCurSec;
            memcpy(currentDir, keepCurDir, strlen(keepCurDir));
            currentDir[strlen(keepCurDir)] = '\0';
        } else if (strcmp(operation, "copy") == 0 || strcmp(operation, "COPY") == 0) {
            scanf("%s", operation);
            int inputLength = strlen(operation);
            for (int i = 0; i < inputLength; ++i)
                operation[i] = to_upper_case(operation[i]);
            if (strcmp(operation, "con") == 0 || strcmp(operation, "CON") == 0) {
                char fileNametempwithSpase[50];
                char fileName[15];
                char keepCurDir[50];
                memcpy(keepCurDir, currentDir, strlen(currentDir));
                keepCurDir[strlen(currentDir)] = '\0';
                int keepCurSec = CurSec;
                scanf("%s", fileNametempwithSpase);
                inputLength = strlen(fileNametempwithSpase);
                for (int i = 0; i < inputLength; ++i)
                    fileNametempwithSpase[i] = to_upper_case(fileNametempwithSpase[i]);
                CurSec = SeperateDirAndFileName(fileNametempwithSpase, currentDir, CurSec, fileName);
                // todo
                int isExist = MakeEmptyFile(CurSec, fileName);
                if (isExist == 1)
                    continue;
                DirItem dim;
                DirItem *di = &dim;
                int tempFindFile = findDirByFileName(di, CurSec, fileName);
                if (tempFindFile == 0)
                    continue;
                bool has_open = false;
                int activateFileIndex = setActivativeFile(di, currentDir);
                // printf("activateFileIndex: %d\n", activateFileIndex);
                if (activateFileIndex == MAX_ACTIVE_FILES) {
                    printf("File has been opened!\n");
                    continue;
                }
                for (int i = 0; i < CurOpenFileNumber; ++i) {
                    if (file_open[i].empty == false && file_open[i].activefile == activateFileIndex) {
                        has_open = true;
                        break;
                    }
                }
                if (has_open == true) {
                    printf("File has been opened!\n");
                    continue;
                } else {
                    file_open[CurOpenFileNumber].empty = false;
                    file_open[CurOpenFileNumber].pos = 0;
                    file_open[CurOpenFileNumber].read_or_write = 1;
                    file_open[CurOpenFileNumber].activefile = activateFileIndex;
                    CurOpenFileNumber++;
                    // printf("Succeed to open file: %s\n", fileName);
                    editExistFile(activateFileIndex, CurSec, tempFindFile - 1);
                    CurOpenFileNumber--;
                    file_open[CurOpenFileNumber].empty = true;
                    deleteActivativeFile(activateFileIndex);
                }
                CurSec = keepCurSec;
                memcpy(currentDir, keepCurDir, strlen(keepCurDir));
                currentDir[strlen(keepCurDir)] = '\0';
            } else {
                char fileName[15];
                char keepCurDir[50];
                memcpy(keepCurDir, currentDir, strlen(currentDir));
                keepCurDir[strlen(currentDir)] = '\0';
                int keepCurSec = CurSec;

                CurSec = SeperateDirAndFileName(operation, currentDir, CurSec, fileName);
                DirItem dim;
                DirItem *di = &dim;
                int tempFindFile = findDirByFileName(di, CurSec, fileName);
                if (tempFindFile == 0)
                    continue;

                char originFile[512];
                int originFileSize = 0;
                bool has_open = false;
                int activateFileIndex = setActivativeFile(di, currentDir);
                // printf("activateFileIndex: %d\n", activateFileIndex);
                if (activateFileIndex == MAX_ACTIVE_FILES) {
                    printf("File has been opened!\n");
                    continue;
                }
                for (int i = 0; i < CurOpenFileNumber; ++i) {
                    if (file_open[i].empty == false && file_open[i].activefile == activateFileIndex) {
                        has_open = true;
                        break;
                    }
                }
                if (has_open == true) {
                    printf("File has been opened!\n");
                    continue;
                } else {
                    file_open[CurOpenFileNumber].empty = false;
                    file_open[CurOpenFileNumber].pos = 0;
                    file_open[CurOpenFileNumber].read_or_write = 0;
                    file_open[CurOpenFileNumber].activefile = activateFileIndex;
                    CurOpenFileNumber++;
                    // printf("Succeed to open file: %s\n",fileName);
                    originFileSize = copy_context_to_array(activateFileIndex, originFile);
                    CurOpenFileNumber--;
                    file_open[CurOpenFileNumber].empty = true;
                    deleteActivativeFile(activateFileIndex);
                }
                CurSec = keepCurSec;
                memcpy(currentDir, keepCurDir, strlen(keepCurDir));
                currentDir[strlen(keepCurDir)] = '\0';

                char fileNametempwithSpase[50];
                scanf("%s", fileNametempwithSpase);

                char controlSign[2];
                printf("Override / cat  to file? [o] or [c] or [n]  ");
                scanf("%s", controlSign);
                controlSign[1] = '\0';
                if ((controlSign[0] == 'n') || (controlSign[0] != 'n' && controlSign[0] != 'c' && controlSign[0] != 'o')) {
                    continue;
                }

                inputLength = strlen(fileNametempwithSpase);
                for (int i = 0; i < inputLength; ++i)
                    fileNametempwithSpase[i] = to_upper_case(fileNametempwithSpase[i]);
                CurSec = SeperateDirAndFileName(fileNametempwithSpase, currentDir, CurSec, fileName);
                // todo

                tempFindFile = findDirByFileName(di, CurSec, fileName);
                if (tempFindFile == 0)
                    continue;
                has_open = false;
                activateFileIndex = setActivativeFile(di, currentDir);
                // printf("activateFileIndex: %d\n", activateFileIndex);
                if (activateFileIndex == MAX_ACTIVE_FILES) {
                    printf("File has been opened!\n");
                    continue;
                }
                for (int i = 0; i < CurOpenFileNumber; ++i) {
                    if (file_open[i].empty == false && file_open[i].activefile == activateFileIndex) {
                        has_open = true;
                        break;
                    }
                }
                if (has_open == true) {
                    printf("File has been opened!\n");
                    continue;
                } else {
                    file_open[CurOpenFileNumber].empty = false;
                    file_open[CurOpenFileNumber].pos = 0;
                    file_open[CurOpenFileNumber].read_or_write = 1;
                    file_open[CurOpenFileNumber].activefile = activateFileIndex;
                    CurOpenFileNumber++;
                    // printf("Succeed to open file: %s\n", fileName);
                    if (controlSign[0] == 'c' && controlSign[1] == '\0')
                        addToExistFile(originFile, originFileSize, activateFileIndex, CurSec, tempFindFile - 1, 'c');
                    if (controlSign[0] == 'o' && controlSign[1] == '\0')
                        addToExistFile(originFile, originFileSize, activateFileIndex, CurSec, tempFindFile - 1, 'o');
                    CurOpenFileNumber--;
                    file_open[CurOpenFileNumber].empty = true;
                    deleteActivativeFile(activateFileIndex);
                }

                CurSec = keepCurSec;
                memcpy(currentDir, keepCurDir, strlen(keepCurDir));
                currentDir[strlen(keepCurDir)] = '\0';
            }
        } else {
            printf("No this sentence!\n");
            continue;
        }
    }
}

int main() {
    BPB mybpb;
    BPB *bpb_ptr = &mybpb;
    int result = Read_ramFDD_to_array("dossys.img", bpb_ptr);
    // printf("Read_ramFDD_to_array: %d\n", result);
    Read_ramFDD_to_BPB(bpb_ptr);
    printBPB(bpb_ptr);
    for (int i = 0; i < MAX_ACTIVE_FILES; ++i) {
        active_file[i].empty = true;
    }
    for (int i = 0; i < MAX_FAT_ITEMS; ++i) {
        fat_item[i].isDelete = false;
    }
    CreateUser(1);
    printf("Save to img? [y] or [n] ");
    char WantSaving[2];
    scanf("%s", WantSaving);
    if (WantSaving[0] == 'y' || WantSaving[0] == 'Y') {
        printf("Saving...\n");
        int originLength = Read_ramFDD_to_array_temp("dossys.img", bpb_ptr);
        int newLength = saveToImg("dossys.img");
        if (originLength != newLength) {
            printf("Rollback...\n");
            saveToImg_RollBack("dossys.img");
        } else {
            printf("Succeed saving.\n");
        }
    }
    return 0;
}
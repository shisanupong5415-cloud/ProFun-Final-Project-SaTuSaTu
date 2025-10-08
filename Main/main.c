#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// กำหนดค่าสูงสุดค่าตัวแปรที่รับเข้ามา
#define csv_MAX 260
#define name_MAX 100
#define position_MAX 50
#define bonus_MAX 15
#define date_MAX 20
#define id_MAX 50


// ประกาศตัวแปร
char csv[csv_MAX] = "data.csv";
char EmployeeName[name_MAX];
char Position[position_MAX];
char BonusAmount[bonus_MAX];
char PaymentDate[date_MAX];
char ID[id_MAX];


// เคลียข้อความหน้าจอ
void clearScreen(void) {
#ifdef _WIN32
    system("cls");     // Windows
#else
    system("clear");   // Unix/Linux/Mac
#endif  
}

/* ตัด \r/\n ท้ายสตริง (ใช้กับ fgets) */
static void trim_eol(char *s) { s[strcspn(s, "\r\n")] = '\0'; }


/* ล้างบรรทัดใน stdin แบบปลอดภัย (กัน EOF) */
static void flushLine(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { }
}

/* พักหน้าจอ: ให้ผู้ใช้กด Enter เพื่อไปต่อ [Def] */
static void pressEnter(void) {
    printf("\nPress Enter to continue...");
    fflush(stdout);
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}

// เอาแค่ชื่อไฟล์จาก path
static const char* csvBasename(void) {
    const char *p1 = strrchr(csv, '\\');  // Windows
    const char *p2 = strrchr(csv, '/');   // macOS/Linux
    const char *p  = (p1 && p2) ? (p1 > p2 ? p1 : p2) : (p1 ? p1 : p2);
    return p ? p + 1 : csv;
}


// แสดงข้อมูล
void listData(void) {
    FILE *f = fopen(csv, "r");
    if (!f) {
        printf("ไม่พบไฟล์: %s (หรือเปิดอ่านไม่ได้)\n", csv);
        return;
    }

    char line[2048];
    int row = 0;

    printf("\n=== LIST: %s ===\n", csvBasename());

    /* อ่านบรรทัดแรก (ถือเป็น header ถ้ามี) */
    if (fgets(line, sizeof line, f)) {
        trim_eol(line);
        if (line[0] != '\0') {
            printf("%s\n", line);
            size_t len = strlen(line);              // ← คิดครั้งเดียว
            for (size_t i = 0; i < len; ++i) putchar('-');
            putchar('\n');
        }
    } else {
        puts("(ไฟล์ว่าง)");
        fclose(f);
        return;
}

    /* พิมพ์ข้อมูลทีละบรรทัด พร้อมเลขลำดับแบบง่าย ๆ */
    while (fgets(line, sizeof line, f)) {
        trim_eol(line);
        if (line[0] == '\0') continue;          /* ข้ามบรรทัดว่าง */
        printf("%3d) %s\n", ++row, line);
    }

    if (row == 0) puts("(ไม่มีข้อมูลแถว)");

    fclose(f);
}


// เพิ่มข้อมูล
void addData(){
    printf("Add Data function called.\n");
}

// ออกโปรแกรม   
void exitProgram(){
    printf("Exiting program.\n");
}

void editData(){
    printf("Edit Data function called.\n");
}

// menu
int menu(void) {
    for (;;) { //ลูปจนกว่าจะได้ค่าเมนูที่ถูกต้อง
        char buf[64];

        printf("Current CSV: %s\n", csvBasename());
        printf("\n===== Menu =====\n");
        printf("1. Add Data\n");
        printf("2. List\n");
        printf("3. Edit\n");
        printf("4. Exit\n");
        printf("=====================\n");
        printf("Choice (1-4): ");

        /* อ่านทั้งบรรทัด (รองรับ Enter เปล่า ๆ และมีช่องว่าง) */
        if (!fgets(buf, sizeof buf, stdin)) {
            puts("Input error. Try again.");
            clearerr(stdin);          /* รีเซ็ตสถานะ error ของ stdin */
            clearScreen();
            continue;
        }
        trim_eol(buf);                 /* ตัด \r\n ออก */

        /* ข้ามช่องว่างหัวสตริง แล้วปฏิเสธกรอกว่าง */
        char *p = buf;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0') {
            puts("Please enter a number between 1 and 4.");
            getchar();               // รอ Enter
            clearScreen();
            continue;                  /* ← กด Enter เปล่า ๆ จะมาที่นี่ */
        }

        /* แปลงเป็นตัวเลขแบบปลอดภัย และห้ามมีขยะต่อท้าย */
        char *end = NULL;
        long v = strtol(p, &end, 10);
        while (*end == ' ' || *end == '\t') end++;  /* อนุญาตช่องว่างท้าย */
        if (*end != '\0') {
            puts("Invalid input: numbers only (1-4).");
            getchar();               // รอ Enter
            clearScreen();
            continue;                  /* มีอักษรอื่นปน */
        }

        if (v < 1 || v > 4) {
            puts("Choice out of range (1-4).");
            getchar();               // รอ Enter
            clearScreen();
            continue;                  /* นอกช่วง */
        }

        return (int)v;                 /* ถูกต้อง → ออกจากฟังก์ชัน */
    }
}


int main() {
    int choice;

    while (1) {
        clearScreen();
        choice = menu();
        if (choice == 0) continue;   // ข้ามถ้าอินพุตไม่ถูกต้อง

        switch (choice) {
            case 1:
                addData();
                pressEnter();         // พักก่อนลูปใหม่
                break;
            case 2:
                clearScreen();
                listData();
                pressEnter();
                break;
            case 3:
                editData();
                pressEnter();
                break;
            case 4:
                exitProgram();
                return 0;
            default:
                printf("Wrong Menu \n");
                pressEnter();
                break;
        }
    }
    return 0;
}   
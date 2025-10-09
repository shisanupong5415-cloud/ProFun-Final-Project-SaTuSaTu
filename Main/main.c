#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "tests.h"

/* ===== prototypes from tests ===== */
void run_unit_test_list(void);
void run_unit_test_search(void);
void run_e2e_tests(void);

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

/* test mode: 0=ปกติ, 1=ระหว่างวิ่ง unit/E2E (ไม่ clear screen/ไม่รอ Enter) */
static int g_testMode = 0;

/* เพิ่มฟังก์ชันควบคุมโหมดเทสต์ */
void set_test_mode(int on) { g_testMode = on ? 1 : 0; }

// เคลียข้อความหน้าจอ
void clearScreen(void) {
    if (g_testMode) return;   /* โหมดเทสต์: ไม่ต้องเคลียร์จอ */
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif  
}

/* ตัด \r/\n ท้ายสตริง (ใช้กับ fgets) */
void trim_eol(char *s) { s[strcspn(s, "\r\n")] = '\0'; }


/* ล้างบรรทัดใน stdin แบบปลอดภัย (กัน EOF) */
void flushLine(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { }
}

/* พักหน้าจอ: ให้ผู้ใช้กด Enter เพื่อไปต่อ [Def] */
void pressEnter(void){
    if (g_testMode) return;        /* โหมดเทสต์: ไม่ต้องรอ */
    int c;
    printf("Press Enter to continue.");
    while ((c = getchar()) != '\n' && c != EOF) {}
}

// เอาแค่ชื่อไฟล์จาก path
const char* csvBasename(void) {
    const char *p1 = strrchr(csv, '\\');  // Windows
    const char *p2 = strrchr(csv, '/');   // macOS/Linux
    const char *p  = (p1 && p2) ? (p1 > p2 ? p1 : p2) : (p1 ? p1 : p2);
    return p ? p + 1 : csv;
}

//============================================================= Everything About Validator Function ============================================================

// ตรวจสอบสตริงว่างเปล่า (มีแต่ช่องว่าง)
int is_blank(const char *s) {
    while (*s == ' ' || *s == '\t') s++;
    return *s == '\0';
}


// ตรวจสอบสตริงว่ามีแต่ตัวเลขและมีความยาวตามที่กำหนด
int val_digits_len(const char *s, int len) {
    int n = 0;
    for (; s[n]; ++n) if (!isdigit((unsigned char)s[n])) return 0;
    return n == len && n > 0;
}

// ตรวจสอบสตริงว่ามีแต่ตัวเลขและไม่ว่างเปล่า
int val_digits_only(const char *s) {
    int n = 0;
    for (; s[n]; ++n) if (!isdigit((unsigned char)s[n])) return 0;
    return n > 0;
}


// ตรวจสอบสตริงว่ามีแต่ตัวอักษร A-Z, a-z, space และต้องมีตัวอักษรอย่างน้อย 1 ตัว
int val_alpha_space_ascii(const char *s) {
    int ok = 0;
    for (; *s; ++s) {
        unsigned char c = (unsigned char)*s;
        if (c == ' ') continue;
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) { ok = 1; continue; }
        return 0;
    }
    return ok;
}


// ตรวจสอบสตริงว่ามีแต่ตัวเลขกับ '-' และต้องมีตัวอักษรอย่างน้อย 1 ตัว
int val_digits_or_dash(const char *s) {
    int n = 0;
    for (; s[n]; ++n) {
        unsigned char c = (unsigned char)s[n];
        if (!(isdigit(c) || c == '-')) return 0;
    }
    return n > 0;
}
//============================================================= Everything About Validator Function =======================================================

//                                                                           []                                                                          //
//                                                                           []                                                                          //
//                                                                           []                                                                          //
//                                                                           []                                                                          //

//======================================================================= Record ID =======================================================================

static long g_nextRecordId = -1;  /* -1 = ยังไม่โหลด */

static void make_id_path(char *out, size_t n) {  /* ไฟล์ตัวนับชื่อคงที่ */
    snprintf(out, n, "record.txt");
}

static void load_next_id_strict(void) {         /* โหลดเลขจาก record.txt เท่านั้น */
    char idPath[512];
    make_id_path(idPath, sizeof idPath);

    FILE *f = fopen(idPath, "r");
    if (f) {
        long val = 0;
        if (fscanf(f, "%ld", &val) == 1 && val >= 1) g_nextRecordId = val;
        else                                         g_nextRecordId = 1; /* เนื้อหาเสีย → reset */
        fclose(f);
    } else {
        g_nextRecordId = 1;                          /* ไม่มีไฟล์ → เริ่มที่ 1 และสร้าง */
        FILE *w = fopen(idPath, "w");
        if (w) { fprintf(w, "%ld\n", g_nextRecordId); fclose(w); }
    }
}

static void save_next_id(void) {                  /* เขียนเลขถัดไปกลับไปที่ record.txt */
    char idPath[512];
    make_id_path(idPath, sizeof idPath);
    FILE *w = fopen(idPath, "w");
    if (!w) { perror("save record.txt"); return; }
    fprintf(w, "%ld\n", g_nextRecordId);
    fclose(w);
}
//======================================================================= Record ID =======================================================================

//                                                                           []                                                                          //
//                                                                           []                                                                          //
//                                                                           []                                                                          //
//                                                                           []                                                                          //

//============================================================= Everything About Add Function ============================================================

// อ่าน 1 บรรทัด: ล้างส่วนเกินถ้ายาว, ตัด \r\n, ห้ามว่าง, ห้ามมี ',' แล้วต้องผ่าน validator
static void prompt_line_checked(const char *label,
                                char *dst, size_t cap,
                                int (*validator)(const char*),
                                const char *errmsg) {
    for (;;) {
        int longline = 0;

        printf("%s", label);
        if (!fgets(dst, (int)cap, stdin)) { clearerr(stdin); continue; }

        /* ถ้าไม่มี '\n' แสดงว่ายาวเกิน → ล้างส่วนที่เหลือทิ้ง */
        if (strchr(dst, '\n') == NULL) {
            int c; while ((c = getchar()) != '\n' && c != EOF) {}
            longline = 1;
        }
        trim_eol(dst);

        if (is_blank(dst))        { puts("  (Blank is not allowed)"); continue; }
        if (strchr(dst, ','))     { puts("  (Cant have , in the message)"); continue; }
        if (!validator(dst))      { puts(errmsg); continue; }
        if (longline) puts("  (Warning: input too long, truncated to fit)");
        break;
    }
}

/* Wrappers ตามเงื่อนไขของแต่ละฟิลด์ */
static void prompt_id_4digits(char *dst, size_t cap) {
    for (;;) {
        printf("Employee ID (4 digits)    : ");
        if (!fgets(dst, (int)cap, stdin)) { clearerr(stdin); continue; }
        if (strchr(dst, '\n') == NULL) { int c; while ((c=getchar())!='\n'&&c!=EOF){} }
        trim_eol(dst);
        if (is_blank(dst))            { puts("  (Blank is not allowed)"); continue; }
        if (strchr(dst, ','))         { puts("  (Cant have , in the message)"); continue; }
        if (!val_digits_len(dst, 4))  { puts("  (Have to be 4 digits)"); continue; }
        break;
    }
}
static void prompt_name_alpha(char *dst, size_t cap) {
    prompt_line_checked("Employee Name            : ", dst, cap,
                        val_alpha_space_ascii, "  (only A-Z, a-z and space allowed)");
}
static void prompt_position_alpha(char *dst, size_t cap) {
    prompt_line_checked("Position                 : ", dst, cap,
                        val_alpha_space_ascii, "  (only A-Z, a-z and space allowed)");
}
static void prompt_bonus_digits(char *dst, size_t cap) {
    prompt_line_checked("Bonus Amount (digits)    : ", dst, cap,
                        val_digits_only, "  (only digits allowed)");
}
static void prompt_date_digits_dash(char *dst, size_t cap) {
    prompt_line_checked("Payment Date (YYYY-MM-DD): ", dst, cap,
                        val_digits_or_dash, "  (only digits and '-' allowed, e.g., 2025-10-09)");
}

// เพิ่มข้อมูล ================= Main Add Function =================
void addData(void) {
    /* โหลดเลข RecordID ครั้งแรกจาก record.txt เท่านั้น */
    if (g_nextRecordId < 0) {
        load_next_id_strict();
    }

    for (;;) {
        /* อ่านอินพุตตามกติกา */
        prompt_id_4digits(ID, sizeof ID);
        prompt_name_alpha(EmployeeName, sizeof EmployeeName);
        prompt_position_alpha(Position, sizeof Position);
        prompt_bonus_digits(BonusAmount, sizeof BonusAmount);
        prompt_date_digits_dash(PaymentDate, sizeof PaymentDate);

        /* เขียนลง CSV (ไม่มี header อัตโนมัติ) */
        FILE *f = fopen(csv, "a");
        if (!f) { perror("open csv for append"); return; }
        fprintf(f, "%s,%s,%s,%s,%s,%ld\n",
                ID, EmployeeName, Position, BonusAmount, PaymentDate, g_nextRecordId);
        fclose(f);

        printf("Saved. RecordID = %ld\n", g_nextRecordId);

        /* ขยับเลขถัดไป + บันทึกลง record.txt */
        g_nextRecordId++;
        save_next_id();

        /* ถามว่าจะเพิ่มต่อไหม */
        char ans[8];
        printf("Add another? (y/N): ");
        if (!fgets(ans, sizeof ans, stdin)) { clearerr(stdin); break; }
        trim_eol(ans);
        if (!(ans[0] == 'y' || ans[0] == 'Y')) break;

        puts("");
    }
}

//============================================================= Everything About Add Function ============================================================

//                                                                           []                                                                          //
//                                                                           []                                                                          //
//                                                                           []                                                                          //
//                                                                           []                                                                          //

/* ============================================================ Everything About List Function ==============================================================*/

enum { COL_ID, COL_NAME, COL_POS, COL_BONUS, COL_DATE, COL_RID, COLS };  // 6 คอลัมน์

/* หัวคอลัมน์ตามสคีมาในไฟล์: ID,EmployeeName,Position,BonusAmount,PaymentDate,RecordID */
const char *COL_HEADER[COLS] = {
    "ID", "EmployeeName", "Position", "BonusAmount", "PaymentDate", "RecordID"
};

/* กำหนดกว้างขั้นต่ำ/สูงสุดของแต่ละคอลัมน์ (เพื่อความสวยงาม) */
const int COL_MINW[COLS] = { 2, 12,  9, 10, 10,  7 };
const int COL_MAXW[COLS] = {10, 28, 18, 12, 12, 10 };

/* แยกบรรทัด CSV แบบง่าย: แยกด้วย ',', ไม่รองรับค่าที่มีเครื่องหมายคำพูด */
int split_csv_simple(char *line, char *fields[], int max_fields) {
    int n = 0;
    char *p = line;
    while (n < max_fields) {
        fields[n] = p;
        char *comma = strchr(p, ',');
        if (!comma) { n++; break; }   // ฟิลด์สุดท้าย
        *comma = '\0';
        p = comma + 1;
        n++;
    }
    return n;
}

/* แถวนี้คือ header ของไฟล์หรือไม่ (ตรงกับหัว 6 ช่อง) */
int is_header_row(char *f[], int n) {
    if (n < COLS) return 0;
    for (int i = 0; i < COLS; ++i) {
        if (strcmp(f[i], COL_HEADER[i]) != 0) return 0;
    }
    return 1;
}

/* วาดเส้นขอบ: +-----+-------+... */
void print_border(const int w[]) {
    putchar('+');
    for (int i = 0; i < COLS; ++i) {
        for (int k = 0; k < w[i] + 2; ++k) putchar('-');  // +2 = เว้นซ้าย/ขวา
        putchar('+');
    }
    putchar('\n');
}

/* พิมพ์ 1 ช่อง: รองรับตัดข้อความเกินความกว้างด้วย "..." และชิดซ้าย/ขวา */
void print_cell(const char *s, int width, int right_align) {
    int len = (int)strlen(s);
    char buf[1024];
    const char *out = s;

    if (len > width) {
        int keep = (width > 3) ? (width - 3) : width;
        if (keep < 0) keep = 0;
        snprintf(buf, sizeof buf, "%.*s%s", keep, s, (width >= 3 ? "..." : ""));
        out = buf;
    }
    if (right_align) printf(" %*s ", width, out);  // ชิดขวา
    else             printf(" %-*s ", width, out); // ชิดซ้าย
}

void listData(void) { // ================= Main List Function =================
    FILE *f = fopen(csv, "r");
    if (!f) {
        printf("No file found: %s (Or cant open file)\n", csv);
        return;
    }

    char line[4096];

    /* เริ่มความกว้างจากหัวคอลัมน์ (ไม่น้อยกว่า MIN) */
    int w[COLS];
    for (int i = 0; i < COLS; ++i) {
        int base = (int)strlen(COL_HEADER[i]);
        w[i] = base < COL_MINW[i] ? COL_MINW[i] : base;
    }

    /* รอบที่ 1: วัดความกว้างเหมาะสมจากข้อมูลจริง (แต่ไม่เกิน MAX) */
    int header_seen = 0;
    while (fgets(line, sizeof line, f)) {
        trim_eol(line);
        if (line[0] == '\0') continue;

        char *fields[COLS] = {0};
        int n = split_csv_simple(line, fields, COLS);
        if (n < COLS) continue;

        if (!header_seen && is_header_row(fields, n)) {
            header_seen = 1;      // ถ้าแถวแรกของไฟล์คือ header ให้ข้ามตอนวัด
            continue;
        }

        for (int i = 0; i < COLS; ++i) {
            int len = (int)strlen(fields[i]);
            if (len > COL_MAXW[i]) len = COL_MAXW[i];
            if (len > w[i]) w[i] = len;
        }
    }
    for (int i = 0; i < COLS; ++i) if (w[i] > COL_MAXW[i]) w[i] = COL_MAXW[i];

    /* พิมพ์หัวตาราง */
    rewind(f);
    header_seen = 0;

    printf("\n=== LIST: %s ===\n", csvBasename());
    print_border(w);
    printf("|");
    for (int i = 0; i < COLS; ++i) {
        print_cell(COL_HEADER[i], w[i], 0);  // หัวคอลัมน์ชิดซ้าย
        printf("|");
    }
    putchar('\n');
    print_border(w);

    /* รอบที่ 2: พิมพ์ข้อมูลทีละแถว */
    int rows = 0;
    while (fgets(line, sizeof line, f)) {
        trim_eol(line);
        if (line[0] == '\0') continue;

        char *fields[COLS] = {0};
        int n = split_csv_simple(line, fields, COLS);
        if (n < COLS) continue;

        if (!header_seen && is_header_row(fields, n)) {
            header_seen = 1;      // ถ้าไฟล์มี header จริง ให้ข้ามไม่พิมพ์ซ้ำ
            continue;
        }

        printf("|");
        print_cell(fields[COL_ID],   w[COL_ID],   1); // ID ขวา
        printf("|");
        print_cell(fields[COL_NAME], w[COL_NAME], 0); // Name ซ้าย
        printf("|");
        print_cell(fields[COL_POS],  w[COL_POS],  0); // Position ซ้าย
        printf("|");
        print_cell(fields[COL_BONUS],w[COL_BONUS],1); // Bonus ขวา
        printf("|");
        print_cell(fields[COL_DATE], w[COL_DATE], 0); // Date ซ้าย
        printf("|");
        print_cell(fields[COL_RID],  w[COL_RID],  1); // RecordID ขวา
        printf("|\n");
        rows++;
    }
    print_border(w);
    printf("Total rows: %d\n", rows);

    fclose(f);
}

/* ============================================================ Everything About List Function ============================================================*/

//                                                                           []                                                                          //
//                                                                           []                                                                          //
//                                                                           []                                                                          //
//                                                                           []                                                                          //

//=========================================================== Everything About Search Function ============================================================
/* เทียบสตริงแบบไม่สนพิมพ์เล็ก/ใหญ่ (ASCII) */
static int equals_ci(const char *a, const char *b) {
    unsigned char ca, cb;
    while (*a && *b) {
        ca = (unsigned char)*a++;
        cb = (unsigned char)*b++;
        if (tolower(ca) != tolower(cb)) return 0;
    }
    return *a == '\0' && *b == '\0';
}

/* อ่าน 1 บรรทัด + อนุญาตพิมพ์ 'menu' เพื่อยกเลิกและกลับเมนู */
static int prompt_line_checked_or_menu(const char *label,
                                       char *dst, size_t cap,
                                       int (*validator)(const char*),
                                       const char *errmsg) {
    for (;;) {
        int longline = 0;
        printf("%s (type 'menu' to cancel): ", label);
        if (!fgets(dst, (int)cap, stdin)) { clearerr(stdin); continue; }

        if (strchr(dst, '\n') == NULL) { int c; while ((c=getchar())!='\n' && c!=EOF) {} longline = 1; }
        trim_eol(dst);

        if (strcmp(dst, "menu") == 0 || strcmp(dst, "MENU") == 0) return 0; /* ยกเลิก */
        if (is_blank(dst))        { puts("  (ห้ามเว้นว่าง)"); continue; }
        if (strchr(dst, ','))     { puts("  (ห้ามมีเครื่องหมายจุลภาค , )"); continue; }
        if (!validator(dst))      { puts(errmsg); continue; }
        if (longline) puts("  (warning: input too long, truncated to fit)");
        return 1;
    }
}

/* ฟังก์ชันตรวจ ID 4 หลัก เพื่อส่งเป็นพอยน์เตอร์ */
static int validator_id_4digits(const char *s) { return val_digits_len(s, 4); }

/* ถาม keyword ตามคอลัมน์ที่เลือก */
static int prompt_search_value(int col, char *dst, size_t cap) {
    switch (col) {
        case 1: /* ID */
            return prompt_line_checked_or_menu("Search by ID (4 digits)",
                                               dst, cap, validator_id_4digits,
                                               "  (Have to be 4 digits)");
        case 2: /* EmployeeName */
            return prompt_line_checked_or_menu("Search by EmployeeName (A-Za-z & space)",
                                               dst, cap, val_alpha_space_ascii,
                                               "  (only A-Z, a-z and space allowed)");
        case 3: /* Position */
            return prompt_line_checked_or_menu("Search by Position (A-Za-z & space)",
                                               dst, cap, val_alpha_space_ascii,
                                               "  (only A-Z, a-z and space allowed)");
        case 4: /* BonusAmount */
            return prompt_line_checked_or_menu("Search by BonusAmount (digits)",
                                               dst, cap, val_digits_only,
                                               "  (only digits allowed)");
        case 5: /* PaymentDate */
            return prompt_line_checked_or_menu("Search by PaymentDate (YYYY-MM-DD)",
                                               dst, cap, val_digits_or_dash,
                                               "  (only digits and '-' allowed, e.g., 2025-10-09)");
        case 6: /* RecordID */
            return prompt_line_checked_or_menu("Search by RecordID (digits)",
                                               dst, cap, val_digits_only,
                                               "  (only digits allowed)");
        default:
            return 0;
    }
}

void searchData(void){
    clearScreen();
    listData();

    int choice = -1;
    for (;;) {
        char buf[64];

        // เมนูย่อยสำหรับ Search
        puts("\nSearch by:");
        puts("  1) ID");
        puts("  2) EmployeeName");
        puts("  3) Position");
        puts("  4) BonusAmount");
        puts("  5) PaymentDate");
        puts("  6) RecordID");
        puts("  0) Back to Menu");
        printf("Your choice (0-6): ");

        // อ่านอินพุตทั้งบรรทัดด้วย fgets (ทนต่อการพิมพ์ Enter เปล่าหรือมีช่องว่าง)
        if (!fgets(buf, sizeof buf, stdin)) { 
            clearerr(stdin);  // ล้างสถานะผิดพลาดของ stdin (เช่น EOF ชั่วคราว)
            continue; 
        }
        trim_eol(buf);        // ตัด '\r' '\n' ทิ้ง

        // ข้ามช่องว่างหัวสตริง
        char *p = buf; 
        while (*p==' '||*p=='\t') p++;

        // ถ้าผู้ใช้กด Enter เปล่า ๆ → แจ้งเตือนแล้ววนถามใหม่
        if (*p=='\0') { 
            puts("Invalid input. Please type your choice (0-6)."); 
            continue; 
        }

        // แปลงเป็นตัวเลขด้วย strtol (ปลอดภัยกว่า scanf)
        // และเช็คว่าหลังตัวเลขไม่มีขยะตามมา (เช่น '2abc')
        char *end = NULL;
        long v = strtol(p, &end, 10);
        while (*end==' '||*end=='\t') end++;   // อนุญาตช่องว่างท้าย
        if (*end!='\0' || v < 0 || v > 6) {    // นอกช่วงหรือมีอักขระปน
            puts("Invalid input. Please type your choice (0-6).");
            continue;
        }

        choice = (int)v;
        break; // อินพุตถูกต้อง ออกจากลูป
    }

    // 0 = กลับเมนูหลักโดยไม่ทำอะไรต่อ
    if (choice == 0) return;

    // และอนุญาตให้พิมพ์ "menu" เพื่อยกเลิกกลับเมนู
    char key[256];
    if (!prompt_search_value(choice, key, sizeof key)) {
        puts("Search cancelled. Returning to menu...");
        return; // ผู้ใช้ยกเลิก
    }

    FILE *f = fopen(csv, "r");
    if (!f) {
        printf("No file found: %s (Cant open file)\n", csv);
        return;
    }

    // กำหนดความกว้างเริ่มต้นจากหัวคอลัมน์ (อย่างน้อย MIN)
    int w[COLS];
    for (int i = 0; i < COLS; ++i) {
        int base = (int)strlen(COL_HEADER[i]);
        w[i] = base < COL_MINW[i] ? COL_MINW[i] : base;
    }

    char line[4096];
    int header_seen = 0;  // ธงไว้ข้ามบรรทัด header ในไฟล์ CSV
    int matches = 0;      // นับจำนวนแถวที่แมตช์ (เพื่อรายงานผลตอนท้าย)

    // ---------- Pass 1: วัดความกว้างจากแถวที่แมตช์ ----------
    while (fgets(line, sizeof line, f)) {
        trim_eol(line);
        if (line[0] == '\0') continue; // ข้ามบรรทัดว่าง

        // แยกฟิลด์ด้วย split_csv_simple (คาดหวัง 6 ช่องตามสคีมา)
        char *fields[COLS] = {0};
        int n = split_csv_simple(line, fields, COLS);
        if (n < COLS) continue; // แถวไม่ครบช่อง ข้าม

        // บรรทัดแรกเป็น header หรือไม่ (เทียบกับ COL_HEADER)
        if (!header_seen && is_header_row(fields, n)) { 
            header_seen = 1; 
            continue; // ไม่เอา header ไปวัดความกว้าง
        }

        // เทียบว่าตรงเงื่อนไขกับ key หรือไม่
        // * ชื่อ/ตำแหน่ง: เทียบแบบไม่สนพิมพ์เล็กใหญ่ (equals_ci)
        // * ค่าตัวเลข/วันที่: เทียบตรง ๆ
        int ok = 0;
        switch (choice) {
            case 1: ok = (strcmp(fields[COL_ID],   key)==0);  break;
            case 2: ok = equals_ci(fields[COL_NAME], key);    break;
            case 3: ok = equals_ci(fields[COL_POS],  key);    break;
            case 4: ok = (strcmp(fields[COL_BONUS], key)==0); break;
            case 5: ok = (strcmp(fields[COL_DATE],  key)==0); break;
            case 6: ok = (strcmp(fields[COL_RID],   key)==0); break;
        }
        if (!ok) continue;

        // ปรับความกว้างแต่ละคอลัมน์ตามข้อมูล (ไม่เกิน COL_MAXW)
        for (int i = 0; i < COLS; ++i) {
            int len = (int)strlen(fields[i]);
            if (len > COL_MAXW[i]) len = COL_MAXW[i];
            if (len > w[i]) w[i] = len;
        }
        matches++;
    }
    // ตัดความกว้างไม่ให้เกิน MAX ตามที่กำหนด
    for (int i = 0; i < COLS; ++i) if (w[i] > COL_MAXW[i]) w[i] = COL_MAXW[i];

    // พิมพ์หัวตาราง แล้วทำ Pass 2 เพื่อพิมพ์เฉพาะแถวที่ตรงเงื่อนไขจริง
    rewind(f); 
    header_seen = 0;
    clearScreen();
    printf("\n=== SEARCH RESULT (%s) ===\n", csvBasename());
    print_border(w);                  // +----+----+...
    printf("|");
    for (int i = 0; i < COLS; ++i) {  // แสดงหัวคอลัมน์
        print_cell(COL_HEADER[i], w[i], 0);
        printf("|");
    }
    putchar('\n');
    print_border(w);

    int printed = 0;
    while (fgets(line, sizeof line, f)) {
        trim_eol(line);
        if (line[0] == '\0') continue;

        char *fields[COLS] = {0};
        int n = split_csv_simple(line, fields, COLS);
        if (n < COLS) continue;

        // ข้าม header
        if (!header_seen && is_header_row(fields, n)) { 
            header_seen = 1; 
            continue; 
        }

        // เงื่อนไขเดียวกับ Pass 1
        int ok = 0;
        switch (choice) {
            case 1: ok = (strcmp(fields[COL_ID],   key)==0);  break;
            case 2: ok = equals_ci(fields[COL_NAME], key);    break;
            case 3: ok = equals_ci(fields[COL_POS],  key);    break;
            case 4: ok = (strcmp(fields[COL_BONUS], key)==0); break;
            case 5: ok = (strcmp(fields[COL_DATE],  key)==0); break;
            case 6: ok = (strcmp(fields[COL_RID],   key)==0); break;
        }
        if (!ok) continue;

        // แสดงข้อมูล 1 แถว (จัดชิดซ้าย/ขวาให้เหมาะ)
        printf("|");  print_cell(fields[COL_ID],   w[COL_ID],   1); // ID ชิดขวา
        printf("|");  print_cell(fields[COL_NAME], w[COL_NAME], 0); // Name ชิดซ้าย
        printf("|");  print_cell(fields[COL_POS],  w[COL_POS],  0); // Position ชิดซ้าย
        printf("|");  print_cell(fields[COL_BONUS],w[COL_BONUS],1); // Bonus ชิดขวา
        printf("|");  print_cell(fields[COL_DATE], w[COL_DATE], 0); // Date ชิดซ้าย
        printf("|");  print_cell(fields[COL_RID],  w[COL_RID],  1); // RecordID ชิดขวา
        printf("|\n");
        printed++;
    }
    print_border(w);                  // เส้นปิดท้ายตาราง
    printf("Matched rows: %d\n", printed);

    fclose(f); // ปิดไฟล์
}
//=========================================================== Everything About Search Function ============================================================

//                                                                           []                                                                          //
//                                                                           []                                                                          // 
//                                                                           []                                                                          //

/* //=========================================================== CSV row model & helpers (ใช้ใน EDIT) //=========================================================== */
typedef struct {
    char id[id_MAX];
    char name[name_MAX];
    char pos[position_MAX];
    char bonus[bonus_MAX];
    char date[date_MAX];
    char rid[32];
} Row;

/* โหลด CSV เป็นแถว ๆ (ข้าม header ถ้ามี) */
static int load_rows(Row **out_rows, size_t *out_n, int *out_has_header) {
    FILE *f = fopen(csv, "r");
    if (!f) { perror("open csv for read"); return 0; }

    Row *rows=NULL; size_t n=0, cap=0;
    char line[4096]; int header_seen=0;

    while (fgets(line, sizeof line, f)) {
        trim_eol(line); if (line[0]=='\0') continue;
        char *fields[COLS]={0}; int m=split_csv_simple(line, fields, COLS);
        if (m<COLS) continue;
        if (!header_seen && is_header_row(fields,m)) { header_seen=1; continue; }

        if (n==cap){ size_t nc=cap?cap*2:64; Row *tmp=(Row*)realloc(rows, nc*sizeof*rows);
            if(!tmp){fclose(f); free(rows); return 0;} rows=tmp; cap=nc; }
        Row *r=&rows[n++];
        snprintf(r->id,    sizeof r->id,    "%s", fields[COL_ID]);
        snprintf(r->name,  sizeof r->name,  "%s", fields[COL_NAME]);
        snprintf(r->pos,   sizeof r->pos,   "%s", fields[COL_POS]);
        snprintf(r->bonus, sizeof r->bonus, "%s", fields[COL_BONUS]);
        snprintf(r->date,  sizeof r->date,  "%s", fields[COL_DATE]);
        snprintf(r->rid,   sizeof r->rid,   "%s", fields[COL_RID]);
    }
    fclose(f);

    *out_rows = rows; *out_n = n; *out_has_header = header_seen;
    return 1;
}

/* เซฟทั้งไฟล์กลับ (คง header ถ้าเดิมมี) */
static int save_rows(const Row *rows, size_t n, int has_header) {
    FILE *w = fopen(csv, "w");
    if (!w) { perror("open csv for write"); return 0; }

    if (has_header) {
        fprintf(w, "%s,%s,%s,%s,%s,%s\n",
            COL_HEADER[0], COL_HEADER[1], COL_HEADER[2],
            COL_HEADER[3], COL_HEADER[4], COL_HEADER[5]);
    }
    for (size_t i=0;i<n;++i) {
        fprintf(w, "%s,%s,%s,%s,%s,%s\n",
            rows[i].id, rows[i].name, rows[i].pos,
            rows[i].bonus, rows[i].date, rows[i].rid);
    }
    fclose(w);
    return 1;
}

/* หา index ด้วย RecordID */
static int find_index_by_rid(Row *rows, size_t n, const char *rid) {
    for (size_t i=0;i<n;++i) if (strcmp(rows[i].rid, rid)==0) return (int)i;
    return -1;
}

/* พรีวิว 1 แถวแบบตารางเล็ก ๆ */
static void preview_row(const Row *r) {
    int w[COLS];
    for (int i=0;i<COLS;++i){ int base=(int)strlen(COL_HEADER[i]); w[i]= base<COL_MINW[i]?COL_MINW[i]:base; }
    int len;
    len=(int)strlen(r->id);    if(len>COL_MAXW[COL_ID])len=COL_MAXW[COL_ID];     if(len>w[COL_ID])  w[COL_ID]=len;
    len=(int)strlen(r->name);  if(len>COL_MAXW[COL_NAME])len=COL_MAXW[COL_NAME]; if(len>w[COL_NAME])w[COL_NAME]=len;
    len=(int)strlen(r->pos);   if(len>COL_MAXW[COL_POS])len=COL_MAXW[COL_POS];   if(len>w[COL_POS]) w[COL_POS]=len;
    len=(int)strlen(r->bonus); if(len>COL_MAXW[COL_BONUS])len=COL_MAXW[COL_BONUS];if(len>w[COL_BONUS])w[COL_BONUS]=len;
    len=(int)strlen(r->date);  if(len>COL_MAXW[COL_DATE])len=COL_MAXW[COL_DATE]; if(len>w[COL_DATE])w[COL_DATE]=len;
    len=(int)strlen(r->rid);   if(len>COL_MAXW[COL_RID])len=COL_MAXW[COL_RID];   if(len>w[COL_RID]) w[COL_RID]=len;

    print_border(w);
    printf("|"); for (int i=0;i<COLS;++i){ print_cell(COL_HEADER[i],w[i],0); printf("|"); } putchar('\n');
    print_border(w);

    printf("|");  print_cell(r->id,    w[COL_ID],    1);
    printf("|");  print_cell(r->name,  w[COL_NAME],  0);
    printf("|");  print_cell(r->pos,   w[COL_POS],   0);
    printf("|");  print_cell(r->bonus, w[COL_BONUS], 1);
    printf("|");  print_cell(r->date,  w[COL_DATE],  0);
    printf("|");  print_cell(r->rid,   w[COL_RID],   1);
    printf("|\n");
    print_border(w);
}

/* //=========================================================== CSV row model & helpers (ใช้ใน EDIT) //=========================================================== */

//                                                                           []                                                                          //
//                                                                           []                                                                          //
//                                                                           []                                                                          //

//=========================================================== Everything About Edit Functions ============================================================

void editData(void){
    /* 1) โชว์รายการ + ช่วยค้นหาให้ก่อน */
    clearScreen();
    listData();
    pressEnter();           /* ให้ผู้ใช้ดู list จบก่อน */
    clearScreen();
    searchData();           /* narrowing ก่อนแก้ */
    pressEnter();

    /* 2) โหลดทั้งไฟล์เข้าเมมโมรี่ */
    Row *rows=NULL; size_t n=0; int has_header=0;
    if (!load_rows(&rows, &n, &has_header)) { puts("Cannot load CSV. Abort."); return; }
    if (n==0) { puts("No data to edit."); free(rows); return; }

    /* 3) รับ RecordID (พิมพ์ 'menu' เพื่อยกเลิกได้) */
    char rid[64];
    if (!prompt_line_checked_or_menu("Enter RecordID to edit", rid, sizeof rid,
                                     val_digits_only, "  (only digits allowed)")) {
        puts("Edit cancelled.");
        free(rows);
        return;
    }
    int idx = find_index_by_rid(rows, n, rid);
    if (idx < 0) { printf("RecordID %s not found.\n", rid); free(rows); return; }

    /* 4) ทำสำเนาชั่วคราวไว้แก้ */
    Row original = rows[idx];
    Row temp     = rows[idx];

    for (;;) {
        clearScreen();
        puts("Current row (original):");
        preview_row(&original);
        puts("\nPending changes (will be saved):");
        preview_row(&temp);

        puts("\nEdit which field?");
        puts("  1) Employee ID (4 digits)");
        puts("  2) EmployeeName");
        puts("  3) Position");
        puts("  4) BonusAmount");
        puts("  5) PaymentDate");
        puts("  6) Confirm & Save");
        puts("  0) Cancel (discard changes)");
        printf("Your choice (0-6): ");

        char buf[64];
        if (!fgets(buf, sizeof buf, stdin)) { clearerr(stdin); continue; }
        trim_eol(buf);
        char *p=buf; while(*p==' '||*p=='\t') p++;
        if (*p=='\0') continue;

        char *end=NULL; long v=strtol(p,&end,10);
        while(*end==' '||*end=='\t') end++;
        if (*end!='\0' || v<0 || v>6) continue;

        if (v==0) { puts("Cancelled. No changes saved."); break; }
        if (v==6) {
            rows[idx] = temp;
            if (!save_rows(rows, n, has_header)) puts("Error: cannot save CSV.");
            else                                  puts("Saved successfully.");
            break;
        }

        /* 5) ปรับค่าแต่ละฟิลด์ด้วยตัวตรวจเดิม (เหมือน addData) */
        switch (v) {
            case 1: prompt_id_4digits(temp.id, sizeof temp.id);                 break;
            case 2: prompt_name_alpha(temp.name, sizeof temp.name);             break;
            case 3: prompt_position_alpha(temp.pos, sizeof temp.pos);           break;
            case 4: prompt_bonus_digits(temp.bonus, sizeof temp.bonus);         break;
            case 5: prompt_date_digits_dash(temp.date, sizeof temp.date);       break;
        }
        /* ไม่ให้แก้ RecordID เพื่อรักษาความยูนีค */
    }

    free(rows);
}

//=========================================================== Everything About Edit Functions ============================================================

//                                                                           []                                                                          //
//                                                                           []                                                                          //
//                                                                           []                                                                          //

//=========================================================== Everything About Delete Function ============================================================

void deleteData(void){
    /* 0) โชว์รายการ & ช่วยค้นหาให้ผู้ใช้ก่อน */
    clearScreen();
    listData();
    pressEnter();            /* ให้ผู้ใช้ไล่อ่านก่อน */
    clearScreen();
    searchData();            /* narrowing */
    pressEnter();

    /* 1) โหลดทั้งไฟล์มาไว้ในหน่วยความจำ */
    Row *rows = NULL; 
    size_t n = 0; 
    int has_header = 0;
    if (!load_rows(&rows, &n, &has_header)) {
        puts("Cannot load CSV. Abort.");
        return;
    }
    if (n == 0) {
        puts("No data to delete.");
        free(rows);
        return;
    }

    /* 2) ให้ผู้ใช้กรอก RecordID ที่ต้องการลบ (พิมพ์ menu เพื่อยกเลิกได้) */
    char rid[64];
    if (!prompt_line_checked_or_menu("Enter RecordID to delete",
                                     rid, sizeof rid,
                                     val_digits_only,
                                     "  (ใส่ได้เฉพาะตัวเลข)")) {
        puts("Delete cancelled.");
        free(rows);
        return;
    }

    /* 3) หา index ของแถวนั้น */
    int idx = find_index_by_rid(rows, n, rid);
    if (idx < 0) {
        printf("RecordID %s not found.\n", rid);
        free(rows);
        return;
    }

    /* 4) แสดงแถวที่จะลบให้พรีวิว แล้วถามยืนยัน */
    clearScreen();
    puts("You are about to delete this row:");
    preview_row(&rows[idx]);

    char ans[8];
    printf("Confirm delete? (y/N): ");
    if (!fgets(ans, sizeof ans, stdin)) { clearerr(stdin); free(rows); return; }
    trim_eol(ans);
    if (!(ans[0] == 'y' || ans[0] == 'Y')) {
        puts("Deletion cancelled.");
        free(rows);
        return;
    }

    /* 5) ลบออกจากอาเรย์ในหน่วยความจำ (เลื่อนทับ) */
    if ((size_t)idx < n - 1) {
        memmove(&rows[idx], &rows[idx + 1], (n - 1 - idx) * sizeof(Row));
    }
    n--;

    /* 6) เซฟกลับลงไฟล์ (คง header ถ้าไฟล์เดิมมี) */
    if (!save_rows(rows, n, has_header)) {
        puts("Error: cannot write CSV.");
    } else {
        puts("Row deleted.");
    }

    free(rows);
}
//=========================================================== Everything About Delete Function ============================================================

//                                                                           []                                                                          //
//                                                                           []                                                                          //
//                                                                           []                                                                          //

//=========================================================== Everything About Exit Function ============================================================
// ออกโปรแกรม   
int exitProgram(void){
    char buf[32];

    for (;;) {
        printf("Are you sure you want to exit? (y/N): ");
        if (!fgets(buf, sizeof buf, stdin)) {   // กันเคส stdin มีปัญหา
            clearerr(stdin);
            continue;
        }
        trim_eol(buf);

        // แปลงเป็นตัวพิมพ์เล็กทั้งหมดเพื่อเทียบแบบไม่สนเคส
        for (char *p = buf; *p; ++p) {
            *p = (char)tolower((unsigned char)*p);
        }

        if (buf[0] == '\0') {                   // กด Enter เปล่า = ไม่ออก
            puts("Canceled. Returning to menu.");
            return 0;
        }
        if (!strcmp(buf, "y") || !strcmp(buf, "yes")) {
            puts("Goodbye!");
            return 1;                           // ยืนยันให้ออก
        }
        if (!strcmp(buf, "n") || !strcmp(buf, "no")) {
            puts("Canceled. Returning to menu.");
            return 0;                           // ไม่ออก
        }

        puts("Please type 'y' or 'n'.");
    }
}
//=========================================================== Everything About Exit Function ============================================================


void unitTest(void){
    for (;;) {
        clearScreen();
        puts("=== Unit Tests ===");
        puts("1) listData tests");
        puts("2) searchData tests");
        puts("0) Back to menu");
        printf("Choice (0-2): ");

        char buf[32];
        if (!fgets(buf, sizeof buf, stdin)) { clearerr(stdin); continue; }
        trim_eol(buf);
        char *p = buf; while (*p==' '||*p=='\t') p++;
        if (*p=='\0') continue;

        char *end=NULL; long v=strtol(p,&end,10);
        while (*end==' '||*end=='\t') end++;
        if (*end!='\0' || v<0 || v>2) continue;

        if (v==0) return; /* กลับเมนูหลัก */

        g_testMode = 1;   /* ปิด clear/enter ระหว่างรันเทสต์ */
        if (v==1) run_unit_test_list();
        if (v==2) run_unit_test_search();
        g_testMode = 0;

        puts("\n[Unit tests finished]");
        pressEnter();     /* โหมดปกติจะหยุดรอ, โหมดเทสต์ = no-op */
    }
}

void E2Etest(void){
    clearScreen();
    puts("=== E2E Tests ===");

    g_testMode = 1;
    run_e2e_tests();
    g_testMode = 0;

    puts("\n[E2E tests finished]");
    pressEnter();
    /* ไม่ต้อง pressEnter(); เด้งกลับเมนูทันที */
    return;
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
        printf("4. Search\n");
        printf("5. Delete\n");
        printf("6. Unit Test\n");
        printf("7. E2E Test\n");
        printf("8. Exit\n");
        printf("=====================\n");
        printf("Choice (1-8): ");

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
            puts("Please enter a number between 1 and 8.");
            getchar();               // รอ Enter
            clearScreen();
            continue;                  /* ← กด Enter เปล่า ๆ จะมาที่นี่ */
        }

        /* แปลงเป็นตัวเลขแบบปลอดภัย และห้ามมีขยะต่อท้าย */
        char *end = NULL;
        long v = strtol(p, &end, 10);
        while (*end == ' ' || *end == '\t') end++;  /* อนุญาตช่องว่างท้าย */
        if (*end != '\0') {
            puts("Invalid input: numbers only (1-8).");
            getchar();               // รอ Enter
            clearScreen();
            continue;                  /* มีอักษรอื่นปน */
        }

        if (v < 1 || v > 8) {
            puts("Choice out of range (1-8).");
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
                clearScreen();
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
                searchData();
                pressEnter();
                break;
            case 5:
                deleteData();
                pressEnter();
                break;
            case 6:
                unitTest();
                break;
            case 7:
                E2Etest();
                break;
            case 8: {
                if (exitProgram()) {
                    return 0;        // ออกจริง
                }
                pressEnter();        // ถ้ายกเลิก ให้อยู่ต่อและพักหน้าจอ
                break;
            }
            default:
                printf("Wrong Menu \n");
                pressEnter();
                break;
        }
    }
    return 0;
}   
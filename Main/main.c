#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


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
void trim_eol(char *s) { s[strcspn(s, "\r\n")] = '\0'; }


/* ล้างบรรทัดใน stdin แบบปลอดภัย (กัน EOF) */
void flushLine(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { }
}

/* พักหน้าจอ: ให้ผู้ใช้กด Enter เพื่อไปต่อ [Def] */
void pressEnter(void) {
    printf("\nPress Enter to continue...");
    fflush(stdout);
    int c;
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

        if (is_blank(dst))        { puts("  (ห้ามเว้นว่าง)"); continue; }
        if (strchr(dst, ','))     { puts("  (ห้ามมีเครื่องหมายจุลภาค , )"); continue; }
        if (!validator(dst))      { puts(errmsg); continue; }
        if (longline) puts("  (คำเตือน: ข้อความยาวเกิน ถูกตัดให้พอดีกับช่อง)");
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
        if (is_blank(dst))            { puts("  (ห้ามเว้นว่าง)"); continue; }
        if (strchr(dst, ','))         { puts("  (ห้ามมี , )"); continue; }
        if (!val_digits_len(dst, 4))  { puts("  (ต้องเป็นตัวเลข 4 หลัก)"); continue; }
        break;
    }
}
static void prompt_name_alpha(char *dst, size_t cap) {
    prompt_line_checked("Employee Name            : ", dst, cap,
                        val_alpha_space_ascii, "  (ใส่ได้เฉพาะตัวอักษรอังกฤษและเว้นวรรค)");
}
static void prompt_position_alpha(char *dst, size_t cap) {
    prompt_line_checked("Position                 : ", dst, cap,
                        val_alpha_space_ascii, "  (ใส่ได้เฉพาะตัวอักษรอังกฤษและเว้นวรรค)");
}
static void prompt_bonus_digits(char *dst, size_t cap) {
    prompt_line_checked("Bonus Amount (digits)    : ", dst, cap,
                        val_digits_only, "  (ใส่ได้เฉพาะตัวเลข)");
}
static void prompt_date_digits_dash(char *dst, size_t cap) {
    prompt_line_checked("Payment Date (YYYY-MM-DD): ", dst, cap,
                        val_digits_or_dash, "  (ใส่ได้เฉพาะตัวเลขและเครื่องหมาย - เช่น 2025-10-09)");
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
        printf("ไม่พบไฟล์: %s (หรือเปิดอ่านไม่ได้)\n", csv);
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
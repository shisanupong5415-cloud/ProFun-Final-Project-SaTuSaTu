#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tests.h"

/* ====== อ้างอิงของโปรแกรมหลัก ====== */
extern void listData(void);
extern void searchData(void);
extern char csv[260]; /* ต้องตรงกับ csv_MAX */

/* ====== helper (cross-platform) ====== */
#ifdef _WIN32
  #define TTY_PATH "CON"
#else
  #define TTY_PATH "/dev/tty"
#endif

static void write_text(const char *path, const char *text){
    FILE *f = fopen(path, "w");
    if (!f) { perror("write_text"); exit(1); }
    fputs(text, f);
    fclose(f);
}
static void remove_if_exists(const char *path){
    FILE *f = fopen(path, "r");
    if (f) { fclose(f); remove(path); }
}
static int file_contains(const char *path, const char *needle){
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = (char*)malloc((size_t)n+1);
    if (!buf) { fclose(f); return 0; }
    fread(buf, 1, (size_t)n, f);
    buf[n] = '\0';
    fclose(f);
    int ok = strstr(buf, needle) != NULL;
    free(buf);
    return ok;
}

static void set_csv(const char *path){ snprintf(csv, 260, "%s", path); }

static void begin_capture_stdout(const char *outpath){
    fflush(stdout);
    if (!freopen(outpath, "w", stdout)) {
        perror("freopen stdout");
        exit(1);
    }
}
static void end_capture_stdout(void){
    fflush(stdout);
    freopen(TTY_PATH, "w", stdout);
}
static void begin_feed_stdin(const char *inpath){
    if (!freopen(inpath, "r", stdin)) {
        perror("freopen stdin");
        exit(1);
    }
}
static void end_feed_stdin(void){
    freopen(TTY_PATH, "r", stdin);
}

/* ===================== Unit tests: listData ===================== */

/* 1) มี header + 3 แถว */
static void ut_list_with_header(void){
    const char *csvpath = "ut_list_header.csv";
    const char *outpath = "ut_list_header.out";
    write_text(csvpath,
        "ID,EmployeeName,Position,BonusAmount,PaymentDate,RecordID\n"
        "0001,Alice,HR,1000,2025-01-02,1\n"
        "0002,Bob,Dev,2000,2025-02-03,2\n"
        "0003,Carol,HR,3000,2025-03-04,3\n"
    );
    set_csv(csvpath);
    begin_capture_stdout(outpath);
    listData();
    end_capture_stdout();

    puts("UT list: with header");
    printf("Total rows == 3 : %s\n", file_contains(outpath,"Total rows: 3")?"PASS":"FAIL");
    printf("Has Alice       : %s\n", file_contains(outpath,"Alice")?"PASS":"FAIL");
    printf("Has header ID   : %s\n", file_contains(outpath,"| ID ")?"PASS":"FAIL");
}

/* 2) ไม่มี header */
static void ut_list_no_header(void){
    const char *csvpath = "ut_list_nohdr.csv";
    const char *outpath = "ut_list_nohdr.out";
    write_text(csvpath,
        "0001,Alice,HR,1000,2025-01-02,1\n"
        "0002,Bob,Dev,2000,2025-02-03,2\n"
    );
    set_csv(csvpath);
    begin_capture_stdout(outpath);
    listData();
    end_capture_stdout();

    puts("UT list: no header");
    printf("Total rows == 2 : %s\n", file_contains(outpath,"Total rows: 2")?"PASS":"FAIL");
    printf("Has Alice       : %s\n", file_contains(outpath,"Alice")?"PASS":"FAIL");
}

/* 3) ไฟล์ว่าง */
static void ut_list_empty_file(void){
    const char *csvpath = "ut_list_empty.csv";
    const char *outpath = "ut_list_empty.out";
    write_text(csvpath,""); /* empty file */
    set_csv(csvpath);
    begin_capture_stdout(outpath);
    listData();
    end_capture_stdout();

    puts("UT list: empty file");
    printf("Total rows == 0 : %s\n", file_contains(outpath,"Total rows: 0")?"PASS":"FAIL");
}

/* 4) ไม่มีไฟล์ */
static void ut_list_no_file(void){
    const char *csvpath = "ut_list_nofile.csv";
    const char *outpath = "ut_list_nofile.out";
    remove_if_exists(csvpath);
    set_csv(csvpath);
    begin_capture_stdout(outpath);
    listData();
    end_capture_stdout();

    puts("UT list: no file");
    printf("Show 'No file found' : %s\n", file_contains(outpath,"No file found")?"PASS":"FAIL");
}

/* 5) ฟิลด์ยาวมากจนต้องตัดด้วย ... */
static void ut_list_truncation(void){
    const char *csvpath = "ut_list_trunc.csv";
    const char *outpath = "ut_list_trunc.out";
    /* ชื่อยาวกว่า COL_MAXW[COL_NAME] (=28) */
    write_text(csvpath,
        "ID,EmployeeName,Position,BonusAmount,PaymentDate,RecordID\n"
        "0001,ThisIsAReallyReallyLongEmployeeNameBeyondLimit,HR,1000,2025-01-02,1\n"
    );
    set_csv(csvpath);
    begin_capture_stdout(outpath);
    listData();
    end_capture_stdout();

    puts("UT list: truncation");
    printf("Has '...'         : %s\n", file_contains(outpath,"...")?"PASS":"FAIL");
}

/* รวมทั้งหมด */
void run_unit_test_list(void){
    const char *orig = "data.csv";
    ut_list_with_header();
    ut_list_no_header();
    ut_list_empty_file();
    ut_list_no_file();
    ut_list_truncation();
    set_csv(orig);
}

/* ===================== Unit tests: searchData ===================== */

/* เตรียม CSV พื้นฐาน */
static void make_search_csv(const char *csvpath){
    write_text(csvpath,
        "ID,EmployeeName,Position,BonusAmount,PaymentDate,RecordID\n"
        "0001,Alice,HR,1000,2025-01-02,11\n"
        "0002,Bob,Dev,2000,2025-02-03,12\n"
        "0003,Carol,HR,3000,2025-03-04,13\n"
        "0004,dAvE,qa,4000,2025-04-05,14\n"
    );
}

/* 1) ค้นด้วย ID ตรงตัว */
static void ut_search_by_id(void){
    const char *csvpath="ut_s_id.csv", *inpath="ut_s_id.in", *outpath="ut_s_id.out";
    make_search_csv(csvpath); set_csv(csvpath);
    write_text(inpath, "1\n0002\n"); /* 1=ID, key=0002 */
    begin_feed_stdin(inpath); begin_capture_stdout(outpath);
    searchData(); end_capture_stdout(); end_feed_stdin();

    puts("UT search: by ID");
    printf("Match rows == 1 : %s\n", file_contains(outpath,"Matched rows: 1")?"PASS":"FAIL");
    printf("Has Bob         : %s\n", file_contains(outpath,"Bob")?"PASS":"FAIL");
}

/* 2) ชื่อไม่สนพิมพ์เล็กใหญ่ (alice vs Alice) */
static void ut_search_by_name_ci(void){
    const char *csvpath="ut_s_name.csv", *inpath="ut_s_name.in", *outpath="ut_s_name.out";
    make_search_csv(csvpath); set_csv(csvpath);
    write_text(inpath, "2\nalice\n"); /* 2=EmployeeName, 'alice' */
    begin_feed_stdin(inpath); begin_capture_stdout(outpath);
    searchData(); end_capture_stdout(); end_feed_stdin();

    puts("UT search: by name (ci)");
    printf("Match rows == 1 : %s\n", file_contains(outpath,"Matched rows: 1")?"PASS":"FAIL");
    printf("Has Alice       : %s\n", file_contains(outpath,"Alice")?"PASS":"FAIL");
}

/* 3) ตำแหน่งไม่สนพิมพ์เล็กใหญ่ (hr -> HR และ qa -> qa) */
static void ut_search_by_position_ci(void){
    const char *csvpath="ut_s_pos.csv", *inpath="ut_s_pos.in", *outpath="ut_s_pos.out";
    make_search_csv(csvpath); set_csv(csvpath);
    write_text(inpath, "3\nhr\n"); /* Position */
    begin_feed_stdin(inpath); begin_capture_stdout(outpath);
    searchData(); end_capture_stdout(); end_feed_stdin();

    puts("UT search: by position (ci)");
    printf("Match rows == 2 : %s\n", file_contains(outpath,"Matched rows: 2")?"PASS":"FAIL");
    printf("Has HR          : %s\n", file_contains(outpath,"HR")?"PASS":"FAIL");
    /* และลอง qa */
    write_text(inpath, "3\nQA\n");
    begin_feed_stdin(inpath); begin_capture_stdout(outpath);
    searchData(); end_capture_stdout(); end_feed_stdin();
    printf("QA match rows == 1 : %s\n", file_contains(outpath,"Matched rows: 1")?"PASS":"FAIL");
}

/* 4) ค้นด้วย BonusAmount (ตัวเลข) */
static void ut_search_by_bonus(void){
    const char *csvpath="ut_s_bonus.csv", *inpath="ut_s_bonus.in", *outpath="ut_s_bonus.out";
    make_search_csv(csvpath); set_csv(csvpath);
    write_text(inpath, "4\n3000\n"); /* โบนัส 3000 */
    begin_feed_stdin(inpath); begin_capture_stdout(outpath);
    searchData(); end_capture_stdout(); end_feed_stdin();

    puts("UT search: by bonus");
    printf("Match rows == 1 : %s\n", file_contains(outpath,"Matched rows: 1")?"PASS":"FAIL");
    printf("Has Carol       : %s\n", file_contains(outpath,"Carol")?"PASS":"FAIL");
}

/* 5) วันที่: อินพุตผิดก่อน แล้วใส่ใหม่ให้ถูก (วนถาม) */
static void ut_search_by_date_retry(void){
    const char *csvpath="ut_s_date.csv", *inpath="ut_s_date.in", *outpath="ut_s_date.out";
    make_search_csv(csvpath); set_csv(csvpath);
    /* 5=PaymentDate -> ใส่ 2025/01/02 (ผิดรูป) แล้วตามด้วย 2025-01-02 (ถูก) */
    write_text(inpath, "5\n2025/01/02\n2025-01-02\n");
    begin_feed_stdin(inpath); begin_capture_stdout(outpath);
    searchData(); end_capture_stdout(); end_feed_stdin();

    puts("UT search: by date (retry)");
    printf("Match rows == 1 : %s\n", file_contains(outpath,"Matched rows: 1")?"PASS":"FAIL");
    printf("Has Alice       : %s\n", file_contains(outpath,"Alice")?"PASS":"FAIL");
}

/* 6) RecordID */
static void ut_search_by_rid(void){
    const char *csvpath="ut_s_rid.csv", *inpath="ut_s_rid.in", *outpath="ut_s_rid.out";
    make_search_csv(csvpath); set_csv(csvpath);
    write_text(inpath, "6\n14\n"); /* RID 14 -> dAvE */
    begin_feed_stdin(inpath); begin_capture_stdout(outpath);
    searchData(); end_capture_stdout(); end_feed_stdin();

    puts("UT search: by RecordID");
    printf("Match rows == 1 : %s\n", file_contains(outpath,"Matched rows: 1")?"PASS":"FAIL");
    printf("Has dAvE        : %s\n", file_contains(outpath,"dAvE")?"PASS":"FAIL");
}

/* 7) ยกเลิกด้วย 'menu' */
static void ut_search_cancel_menu(void){
    const char *csvpath="ut_s_cancel.csv", *inpath="ut_s_cancel.in", *outpath="ut_s_cancel.out";
    make_search_csv(csvpath); set_csv(csvpath);
    /* เลือก field 2 (name) แล้วพิมพ์ menu เพื่อยกเลิก */
    write_text(inpath, "2\nmenu\n");
    begin_feed_stdin(inpath); begin_capture_stdout(outpath);
    searchData(); end_capture_stdout(); end_feed_stdin();

    puts("UT search: cancel via 'menu'");
    printf("Shows 'Search cancelled' : %s\n", file_contains(outpath,"Search cancelled")?"PASS":"FAIL");
}

/* 8) ไม่มีไฟล์ */
static void ut_search_no_file(void){
    const char *csvpath="ut_s_nofile.csv", *inpath="ut_s_nofile.in", *outpath="ut_s_nofile.out";
    remove_if_exists(csvpath); set_csv(csvpath);
    write_text(inpath, "1\n0001\n");
    begin_feed_stdin(inpath); begin_capture_stdout(outpath);
    searchData(); end_capture_stdout(); end_feed_stdin();

    puts("UT search: no file");
    printf("Show 'No file found' : %s\n", file_contains(outpath,"No file found")?"PASS":"FAIL");
}

/* รวมทั้งหมด */
void run_unit_test_search(void){
    const char *orig="data.csv";
    ut_search_by_id();
    ut_search_by_name_ci();
    ut_search_by_position_ci();
    ut_search_by_bonus();
    ut_search_by_date_retry();
    ut_search_by_rid();
    ut_search_cancel_menu();
    ut_search_no_file();
    set_csv(orig);
}

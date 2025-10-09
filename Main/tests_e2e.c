#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* fd backup/restore (cross-platform) */
#if defined(_WIN32)
  #include <io.h>
  #define MY_DUP     _dup
  #define MY_DUP2    _dup2
  #define MY_CLOSE   _close
  #define MY_FILENO  _fileno
#else
  #include <unistd.h>
  #define MY_DUP     dup
  #define MY_DUP2    dup2
  #define MY_CLOSE   close
  #define MY_FILENO  fileno
#endif

#include "tests.h"

/* ====== อ้างอิงของโปรแกรมหลัก ====== */
extern void listData(void);
extern void searchData(void);
extern void editData(void);
extern void deleteData(void);
extern char csv[260];

/* ====== helper (no TTY needed; use FD backup/restore) ====== */
static void write_text(const char *path, const char *text){
    FILE *f = fopen(path, "w");
    if (!f) { perror("write_text"); exit(1); }
    fputs(text, f);
    fclose(f);
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

/* ====== STDIN/STDOUT redirection with FD backup ====== */
static int saved_stdin_fd  = -1;
static int saved_stdout_fd = -1;

static void begin_capture_stdout(const char *outpath){
    fflush(stdout);
    if (saved_stdout_fd == -1) {
        saved_stdout_fd = MY_DUP(MY_FILENO(stdout));
        if (saved_stdout_fd == -1) { perror("dup stdout"); exit(1); }
    }
    if (!freopen(outpath, "w", stdout)) {
        perror("freopen stdout");
        exit(1);
    }
}
static void end_capture_stdout(void){
    fflush(stdout);
    if (saved_stdout_fd != -1) {
        if (MY_DUP2(saved_stdout_fd, MY_FILENO(stdout)) == -1) {
            perror("dup2 stdout");
            exit(1);
        }
        MY_CLOSE(saved_stdout_fd);
        saved_stdout_fd = -1;
    }
}
static void begin_feed_stdin(const char *inpath){
    if (saved_stdin_fd == -1) {
        saved_stdin_fd = MY_DUP(MY_FILENO(stdin));
        if (saved_stdin_fd == -1) { perror("dup stdin"); exit(1); }
    }
    if (!freopen(inpath, "r", stdin)) {
        perror("freopen stdin");
        exit(1);
    }
}
static void end_feed_stdin(void){
    if (saved_stdin_fd != -1) {
        if (MY_DUP2(saved_stdin_fd, MY_FILENO(stdin)) == -1) {
            perror("dup2 stdin");
            exit(1);
        }
        MY_CLOSE(saved_stdin_fd);
        saved_stdin_fd = -1;
    }
}

/* ตัวช่วย list แล้วจับผล */
static void capture_list(const char *outpath){
    begin_capture_stdout(outpath);
    listData();
    end_capture_stdout();
}

/* ====== E2E scenarios ====== */
void run_e2e_tests(void){
    const char *orig = "data.csv";
    const char *csvpath = "e2e.csv";
    set_csv(csvpath);

    /* Initial data */
    write_text(csvpath,
        "ID,EmployeeName,Position,BonusAmount,PaymentDate,RecordID\n"
        "0001,Alice,HR,1000,2025-01-02,101\n"
        "0002,Bob,Dev,2000,2025-02-03,102\n"
    );

    puts("== E2E #1: Edit cancelled via 'menu' ==");
    write_text("e2e_edit_cancel.in",
        "\n"        /* pressEnter() after listData() */
        "6\n102\n"  /* search by RID=102 */
        "\n"        /* pressEnter() after searchData() */
        "menu\n"    /* cancel at 'Enter RecordID to edit' */
    );
    begin_feed_stdin("e2e_edit_cancel.in");
    begin_capture_stdout("e2e_edit_cancel.out");
    editData();
    end_capture_stdout();
    end_feed_stdin();

    capture_list("e2e_list_after_cancel.out");
    printf("Still has 'Dev' for Bob : %s\n",
        file_contains("e2e_list_after_cancel.out", ",Bob,Dev,") ? "PASS":"FAIL");

    puts("\n== E2E #2: Edit success (RID=102 => Position=QA) ==");
    write_text("e2e_edit_ok.in",
        "\n"        /* listData() -> pressEnter */
        "6\n102\n"  /* search RID=102 */
        "\n"        /* pressEnter after searchData */
        "102\n"     /* RecordID to edit */
        "3\n"       /* field=Position */
        "QA\n"      /* new value */
        "6\n"       /* Confirm & Save */
    );
    begin_feed_stdin("e2e_edit_ok.in");
    begin_capture_stdout("e2e_edit_ok.out");
    editData();
    end_capture_stdout();
    end_feed_stdin();

    capture_list("e2e_list_after_edit.out");
    printf("Bob position => QA     : %s\n",
        file_contains("e2e_list_after_edit.out", ",Bob,QA,") ? "PASS":"FAIL");

    puts("\n== E2E #3: Delete cancelled (RID=101, answer 'n') ==");
    write_text("e2e_del_cancel.in",
        "\n"        /* listData -> pressEnter */
        "6\n101\n"  /* search RID=101 */
        "\n"        /* pressEnter after search */
        "101\n"     /* RID to delete */
        "n\n"       /* cancel */
    );
    begin_feed_stdin("e2e_del_cancel.in");
    begin_capture_stdout("e2e_del_cancel.out");
    deleteData();
    end_capture_stdout();
    end_feed_stdin();

    capture_list("e2e_list_after_del_cancel.out");
    printf("Alice still exists      : %s\n",
        file_contains("e2e_list_after_del_cancel.out", "Alice") ? "PASS":"FAIL");

    puts("\n== E2E #4: Delete success (RID=102) ==");
    write_text("e2e_del_ok.in",
        "\n"        /* listData -> pressEnter */
        "6\n102\n"  /* search RID=102 */
        "\n"        /* pressEnter */
        "102\n"     /* RID to delete */
        "y\n"       /* confirm */
    );
    begin_feed_stdin("e2e_del_ok.in");
    begin_capture_stdout("e2e_del_ok.out");
    deleteData();
    end_capture_stdout();
    end_feed_stdin();

    capture_list("e2e_list_after_del_ok.out");
    printf("RID=102 gone            : %s\n",
        !file_contains("e2e_list_after_del_ok.out", "102") ? "PASS":"FAIL");
    printf("Alice still there       : %s\n",
        file_contains("e2e_list_after_del_ok.out", "Alice") ? "PASS":"FAIL");

    set_csv(orig);
}

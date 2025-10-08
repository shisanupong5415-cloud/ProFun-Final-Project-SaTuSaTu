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

// เอาแค่ชื่อไฟล์จาก path
static const char* csvBasename(void) {
    const char *p1 = strrchr(csv, '\\');  // Windows
    const char *p2 = strrchr(csv, '/');   // macOS/Linux
    const char *p  = (p1 && p2) ? (p1 > p2 ? p1 : p2) : (p1 ? p1 : p2);
    return p ? p + 1 : csv;
}

// แสดงข้อมูล
void listData(){
    printf("List Data function called.\n");
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

int menu() {
    int choice;
    printf("Current CSV: %s\n", csvBasename());
    printf("\n===== Menu =====\n");
    printf("1. Add Data\n");
    printf("2. List\n");
    printf("3. Edit\n");
    printf("4. Exit\n");
    printf("=====================\n");
    printf("Choice (1-4): ");
    scanf("%d", &choice);
    if (choice < 1 || choice > 4) {
        printf("Invalid choice. Please select a number between 1 and 4.\n");
        printf("Press Enter to continue...");
        while (getchar() != '\n'); // Clear input buffer
        getchar(); // Wait for Enter key
    }
    return choice;
}


int main() {
    int choice;

    while (1) {
        clearScreen();
        choice = menu();

        switch (choice) {
            case 1:
                addData();
                break;
            case 2:
                listData();
                break;
            case 3:
                editData();
                break;
            case 4: 
                exitProgram();
                return 0; 
            default:
                printf("Wrong Menu \n");
        }
    }
    return 0;
}
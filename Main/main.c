#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void clearScreen(void) {
#ifdef _WIN32
    system("cls");     // Windows
#else
    system("clear");   // Unix/Linux/Mac
#endif  
}

void addData(){
    printf("Add Data function called.\n");
}
void showData(){
    printf("Show Data function called.\n");
}
void exitProgram(){
    printf("Exiting program.\n");
}


int menu() {
    int choice;
    printf("\n===== Menu =====\n");
    printf("1. Add Data\n");
    printf("2. List\n");
    printf("3. Delete\n");
    printf("4. Exit\n");
    printf("=====================\n");
    printf("Choice (1-4): ");
    scanf("%d", &choice);
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
                showData();
                break;
            case 3:
                exitProgram();
                return 0; 
            default:
                printf("Wrong Menu \n");
        }
    }
    return 0;
}
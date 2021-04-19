#include"list.h"

int main() {
    printf("1 Create List\n");
    printf("2 printf List\n");
    printf("3 insert List\n");
    printf("4 quit\n");
    ListPtr head = NULL;
    while (1) {
        char cmd = getchar();
        switch (cmd)
        {
        case '1':
            head = creatList();
            break;
        case '2':
            printList(head);
            break;
        case '3':
            insertList(head);
            break;
        case '4':
            return 0;
        default:
            break;
        }
    }
    return 0;
}
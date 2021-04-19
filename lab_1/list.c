#include"list.h"

ListPtr creatList() {
    ListPtr head = (ListPtr) malloc(sizeof(ListNode));
    head->next = NULL;
    printf("createList() end! enter cmd... \n");
    return head;
}

void printList(ListPtr head) {
    if (head == NULL) return;
    int cnt = 1;
    for (ListPtr cur = head->next; cur != NULL; cur = cur->next, ++cnt) {
        printf("node %d: stu{name = %s, age = %d}\n", cnt, cur->data.stuName, cur->data.Age);
    }
    printf("printList() end! enter cmd... \n");
}

// 默认就插入在头部
void insertList(ListPtr head) {
    if (head == NULL) return;
    ListPtr ptr = (ListPtr) malloc(sizeof(ListNode));
    printf("Enter stu.name\n");
    scanf("%s", ptr->data.stuName);
    printf("Enter stu.age\n");
    scanf("%d", &(ptr->data.Age));
    ptr->next = head->next;
    head->next = ptr;
    printf("insertList() end! enter cmd... \n");
}
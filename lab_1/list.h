#ifndef LIST_H
#define LIST_H
#include<stdio.h>
#include<stdlib.h>

//预定义数据结构
typedef struct stuInfo
{
    char stuName[10]; /*学生姓名*/
    int Age;          /*年龄*/
} ElemType;

typedef struct node
{
    ElemType data; 
    struct node *next;
} ListNode, *ListPtr;

ListPtr creatList();
void printList(ListPtr head);
void insertList(ListPtr head);

#endif
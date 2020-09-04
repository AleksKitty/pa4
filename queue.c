//
// Created by alex on 31.08.2020.
//
/*
 * C Program to Implement Priority Queue to Add and Delete Elements
 */
#include <stdio.h>
#include "ipc.h"
#include "lamport.h"
#include "log.h"


void tryQueue() {

}

/* Function to insert value into priority queue */
int insert_by_priority(timestamps pri_que[MAX], int rear, local_id id, timestamp_t time) {
    if (rear >= MAX - 1) {
        printf("\nQueue overflow no more elements can be inserted");
        return rear;
    }
    if (rear == -1) {
        rear++;
        pri_que[rear].localId = id;
        pri_que[rear].queue_time = time;
        return rear;
    } else
        check(pri_que, rear, id, time);
    rear++;

    return rear;
}

/* Function to check priority and place element */
void check(timestamps pri_que[MAX], int rear, local_id id, timestamp_t time) {
    int i, j;

    for (i = 0; i <= rear; i++) {
        if (time > pri_que[i].queue_time || (time == pri_que[i].queue_time && id > pri_que[i].localId)) {
            for (j = rear + 1; j > i; j--) {
                pri_que[j] = pri_que[j - 1];
            }
            pri_que[i].localId = id;
            pri_que[i].queue_time = time;
            return;
        }
    }
    pri_que[i].localId = id;
    pri_que[i].queue_time = time;
}

/* Function to delete the smallest element from queue */
int delete_the_smallest(timestamps pri_que[MAX], int rear) {

    if (rear == -1) {
//        printf("\nQueue is empty no elements to delete\n");
        return rear;
    }

    pri_que[rear].localId = -1;
    pri_que[rear].queue_time = -1;
    rear--;

    return rear;
}

int peek_id_with_the_smallest_time(timestamps pri_que[MAX], int rear) {
    if (rear == -1) {
        printf("\nQueue is empty no elements to delete");
        return rear;
    }

    return pri_que[rear].localId;
}



/* Function to delete an element BY ID from queue */
int delete_by_id(timestamps *pri_que, int rear, local_id id) {

    if (rear == -1) {
        printf("\nQueue is empty no elements to delete");
        return -1;
    }

    for (int i = 0; i <= rear; i++) {
        if (id == pri_que[i].localId) {
            for (; i < rear; i++) {
                pri_que[i] = pri_que[i + 1];
            }

            pri_que[i].localId = -1;
            pri_que[i].queue_time = -1;
            rear--;

            return rear;

            if (rear == -1) {
                return -1;
            }
        }
    }
    printf("\n%d not found in queue to delete", id);
    return rear;
}

/* Function to display queue elements */
void display_pqueue(timestamps pri_que[MAX], int rear) {

    if (rear == -1) {
        printf("\nQueue is empty");
        return;
    }

    printf("-------------------\n");
    for (int i = 0; i <= rear; i++) {
        printf("time = %d, id = %d\n", pri_que[i].queue_time, pri_que[i].localId);
    }
}

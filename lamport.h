//
// Created by alex on 28.08.2020.
//

#ifndef PA1_LAMPORT_H
#define PA1_LAMPORT_H

#define MAX MAX_PROCESS_ID * MAX_PROCESS_ID

#include <sys/types.h>
#include "ipc.h"

typedef struct {
    local_id localId; // id from ipc.h
    timestamp_t queue_time; // time in queue
} timestamps;

typedef struct {
    pid_t pid; // special id for processes
    local_id localId; // id from ipc.h
    int *pipe_read; // who we need to READ from
    int *pipe_write; // who we need to WRITE into
    timestamp_t lamport_time; // current time
    timestamps pri_que[MAX]; // queue!
    int rear; // last element in queue
    int finished_processes; // we need to count who is dead, else we are waiting for replies from dead
}  process;

extern int number_of_processes;

void doSecondRule(process * process_receiver, timestamp_t msg_time);


int request(void * self);
int release(void * self);


// for queue
void tryQueue();
int insert_by_priority(timestamps pri_que[MAX], int rear, local_id id, timestamp_t time);
int delete_by_id(timestamps *pri_que, int rear, local_id id);
int delete_the_smallest(timestamps pri_que[MAX], int rear);
void create(timestamps pri_que[MAX]);
void check(timestamps pri_que[MAX], int rear, local_id id, timestamp_t time);
void display_pqueue(timestamps pri_que[MAX], int rear);
int peek_id_with_the_smallest_time(timestamps pri_que[MAX], int rear);


#endif //PA1_LAMPORT_H

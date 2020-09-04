//
// Created by alex on 28.08.2020.
//

#define _XOPEN_SOURCE 500

#include <string.h>
#include <unistd.h>
#include "lamport.h"
#include "log.h"

// time = max of (time_receiver, time_sender)
// time++
//
void doSecondRule(process *process_receiver, timestamp_t msg_time) {

    if (process_receiver->lamport_time < msg_time) {
        process_receiver->lamport_time = msg_time;
    }

    process_receiver->lamport_time++;

//    lg(process_receiver->localId, "doSecondRule", "process %d lamport_time = %d", process_receiver->localId, process_receiver->lamport_time);
}

int request(void *self) {
    process *process = self;

    // send REQUEST to everybody
    // First rule: update time before send or receive (time = time + 1)
    process->lamport_time++;

    // push in queue
    process->rear = insert_by_priority(process->pri_que, process->rear, process->localId, process->lamport_time);
//    display_pqueue(process->pri_que, process->rear);


    Message message = {.s_header = {.s_type = CS_REQUEST, .s_local_time = process->lamport_time, .s_magic = MESSAGE_MAGIC, .s_payload_len = 0},}; // our message, set s_header of Message; set s_type and s_magic of Header
    send_multicast(process, &message); // send CS_REQUEST to everyone

    // print
//    lg(process->localId, "request", "rear = %d", process->rear);


    int in_critical_area = 0;

    int done = 0;
    while (in_critical_area == 0 && process->finished_processes > 0) {

        // First rule: update time before send or receive (time = time + 1)
        process->lamport_time++;

        memset(message.s_payload, 0, message.s_header.s_payload_len);
        local_id id_from = receive_any(process, &message); // receive messages


        // time = max of (time_receiver, time_sender)
        // time++
        doSecondRule(process, message.s_header.s_local_time); // check time


        if (message.s_header.s_type == CS_REPLY) {
            done++;
//            lg(process->localId, "request", "done = %d", done);

        } else if (message.s_header.s_type == CS_REQUEST) { //&& message.s_header.s_local_time <= process->lamport_time

            // print
//            lg(process->localId, "request", "we got a message!");


            // push in queue !other! process's info
            process->rear = insert_by_priority(process->pri_que, process->rear, id_from, message.s_header.s_local_time);

            // print
//            lg(process->localId, "request", "rear = %d", process->rear);
//            display_pqueue(process->pri_que, process->rear);

            // First rule: update time before send or receive (time = time + 1)
            process->lamport_time++;

            // send reply
            Message msg = {.s_header = {.s_type = CS_REPLY, .s_local_time = process->lamport_time, .s_magic = MESSAGE_MAGIC, .s_payload_len = 0},}; // our message, set s_header of Message; set s_type and s_magic of Header
            send(process, id_from, &msg);

            usleep(100);
        } else if (message.s_header.s_type == CS_RELEASE) {

//            lg(process->localId, "request", "in first while deleting smallest!");

            process->rear = delete_by_id(process->pri_que, process->rear, id_from);
        } else if (message.s_header.s_type == DONE) {
//            lg(process->localId, "request", "DONE message");
            process->finished_processes--;
        }



        // try to get in critical area
        if (done == number_of_processes - 2) {

            // in critical area
            if (peek_id_with_the_smallest_time(process->pri_que, process->rear) == process->localId) {
//                display_pqueue(process->pri_que, process->rear);
                in_critical_area = 1;
//                lg(process->localId, "request", "in_critical_area = 1");
            }
        }
    }

//    lg(process->localId, "request", "received all REPLY");


    return 0;
}

int release(void *self) {
    process *process = self;


    // First rule: update time before send or receive (time = time + 1)
    process->lamport_time++;

    Message message = {.s_header = {.s_type = CS_RELEASE, .s_local_time = process->lamport_time, .s_magic = MESSAGE_MAGIC, .s_payload_len = 0},}; // our message, set s_header of Message; set s_type and s_magic of Header

    send_multicast(process, &message);

    process->rear = delete_the_smallest(process->pri_que, process->rear);

    return 0;
}


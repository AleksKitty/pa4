#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "ipc.h"
#include "common.h"
#include "pa2345.h"
#include "log.h"
#include "lamport.h"


static int receive_from_all_children(void *self, Message *msg) {
    process *process = self;

    for (int index_pipe_read = 1; index_pipe_read < number_of_processes; index_pipe_read++) {
        if (index_pipe_read != process->localId) {

            receive(self, index_pipe_read, msg);
            doSecondRule(self, msg->s_header.s_local_time);
        }
    }

    return 0;
}


static void create_pipes(process *array_of_processes) {
    lg(0, "create_pipes", "Creating pipes!");

    FILE *pipe_log = fopen("pipes", "a"); // for writing into file
    for (int i = 0; i < number_of_processes; i++) {
        for (int j = 0; j < number_of_processes; j++) { // making pipes for everyone to everyone
            // try to create new pipe

            if (i != j) {
                // for standard pipe
                int fd[2];

                if (pipe(fd) < 0) { // fail
                    lg(0, "create_pipes", "Can't create new pipe");
                    exit(-1);
                }

                fcntl(fd[0], F_SETFL, fcntl(fd[0], F_GETFL) | O_NONBLOCK); // make pipes not blocking!!!
                fcntl(fd[1], F_SETFL, fcntl(fd[1], F_GETFL) | O_NONBLOCK); // make pipes not blocking!!!

                array_of_processes[j].pipe_read[i] = fd[0]; // j read from i
                array_of_processes[i].pipe_write[j] = fd[1]; // i write into j


                lg(0, "create_pipes", "Pipe (read %d, write %d) has OPENED", fd[0], fd[1]);
                fprintf(pipe_log, "Pipe (read %d, write %d) has OPENED", fd[0], fd[1]);
                fflush(pipe_log);
            } else {
                array_of_processes[j].pipe_read[i] = -1; // can't read from itself
                array_of_processes[i].pipe_write[j] = -1; // can't write into itself
            }
        }

    }
    fclose(pipe_log);
}

static void close_unnecessary_pipes(process array_of_processes[], local_id id) {

    for (int i = 0; i < number_of_processes; i++) {
        for (int j = 0; j < number_of_processes; j++) {
            if (i != id && i != j) {

                close(array_of_processes[i].pipe_write[j]); // i can't write into j

                close(array_of_processes[i].pipe_read[j]); // i can't read from j;
            }
        }
    }
}


static void close_all_pipes(process array_of_processes[]) {
    for (int i = 0; i < number_of_processes; i++) {
        for (int j = 0; j < number_of_processes; j++) {
            if (i != j) {
                if (array_of_processes[j].pipe_read[i] > 0) {
                    close(array_of_processes[j].pipe_read[i]); // j read from i
                }

                if (array_of_processes[i].pipe_read[j] > 0) {
                    close(array_of_processes[i].pipe_read[j]); // j read from i
                }
            }
        }
    }
}


static void create_processes(process *array_of_processes, FILE *event_log, int mutexl) {
//    lg(0, "create_processes", "Creating processes:");

    for (int i = 1; i < number_of_processes; i++) {
        array_of_processes[i].localId = i; // give Local id to the future new process

        pid_t result_of_fork = fork();

        // try to create new processes
        if (result_of_fork == -1) {// fail
            lg(0, "create_processes", "Can't create new process");
            exit(-1);
        } else if (result_of_fork == 0) { // we are in child
            lg(array_of_processes[i].localId, "create_processes", "[son] pid %d from [parent] pid %d", getpid(), getppid());

            array_of_processes[i].pid = getpid(); // give pid

            close_unnecessary_pipes(array_of_processes,
                                    array_of_processes[i].localId); // struct is duplicated, we need to close unnecessary pipes!

            // First rule: update time before send or receive (time = time + 1)
            array_of_processes[i].lamport_time++;


            Message message = {.s_header = {.s_type = STARTED, .s_local_time = array_of_processes[i].lamport_time, .s_magic = MESSAGE_MAGIC},}; // our message, set s_header of Message; set s_type and s_magic of Header
            sprintf(message.s_payload, log_started_fmt, array_of_processes[i].lamport_time,
                    array_of_processes[i].localId, array_of_processes[i].pid, array_of_processes[0].pid,
                    0); // data of our message in a buffer, set s_payload of Message
            message.s_header.s_payload_len = (uint16_t) strlen(message.s_payload) + 1; // set s_payload_len of Header

            send_multicast(&array_of_processes[i], &message); // send STARTED to everyone

            // First rule: update time before send or receive (time = time + 1)
            array_of_processes[i].lamport_time++;


            receive_from_all_children(&array_of_processes[i], &message); // receive all STARTED

            // print
            fprintf(event_log, log_received_all_started_fmt, array_of_processes[i].lamport_time, i);
            fflush(event_log);



            // "useful" work of every child process
            int number_iterations = array_of_processes[i].localId * 5;

            for (int k = 1; k <= number_iterations; k++) {

                // into critical area
                if (mutexl == 1) {

                    request(&array_of_processes[i]);
                }

                // print
                fprintf(event_log, log_loop_operation_fmt, array_of_processes[i].localId, k, number_iterations);
                fflush(event_log);



                char s_payload[64];
                memset(s_payload, 0, sizeof(char) * 64);
                sprintf(s_payload, log_loop_operation_fmt, array_of_processes[i].localId, k, number_iterations);
                print(s_payload);


                usleep(200);

                // out from critical area
                if (mutexl == 1) {
                    release(&array_of_processes[i]);
                }
            }


            // First rule: update time before send or receive (time = time + 1)
            array_of_processes[i].lamport_time++;

            // send DONE
            message.s_header.s_type = DONE;
            message.s_header.s_local_time = array_of_processes[i].lamport_time;
            sprintf(message.s_payload, log_done_fmt, array_of_processes[i].lamport_time, array_of_processes[i].localId,
                    0); // data of our message in a buffer, set s_payload of Message
            message.s_header.s_payload_len = (uint16_t) strlen(message.s_payload) + 1; // set s_payload_len of Header

            send_multicast(&array_of_processes[i], &message); // send all DONE



            while (array_of_processes[i].finished_processes > 0) {

                // First rule: update time before send or receive (time = time + 1)
                array_of_processes[i].lamport_time++;

                memset(message.s_payload, 0, message.s_header.s_payload_len);
                local_id id_from = receive_any(&array_of_processes[i], &message); // receive messages

                // time = max of (time_receiver, time_sender)
                // time++
                doSecondRule(&array_of_processes[i], message.s_header.s_local_time); // check time


                if (message.s_header.s_type == DONE) {
//                    lg(array_of_processes[i].localId, "create_processes", "done++");
                    array_of_processes[i].finished_processes--;
                } else if (message.s_header.s_type == CS_REQUEST) {
                    // First rule: update time before send or receive (time = time + 1)
                    array_of_processes[i].lamport_time++;

                    // send reply
                    Message msg = {.s_header = {.s_type = CS_REPLY, .s_local_time =  array_of_processes[i].lamport_time, .s_magic = MESSAGE_MAGIC, .s_payload_len = 0},}; // our message, set s_header of Message; set s_type and s_magic of Header
                    send(&array_of_processes[i], id_from, &msg);
                }
            }

//            lg(array_of_processes[i].localId, "create_processes", "process %d exit with time = %d!", array_of_processes[i].localId, array_of_processes[i].lamport_time);

            exit(0);
        }
    }
}

int main(int argc, char *argv[]) {
    tryQueue();

    int mutexl = 0;

    if ((argc == 3 || argc == 4) && strcmp("-p", argv[1]) == 0) { // reading input parameters
        number_of_processes = atoi(argv[2]);

    } else {
        lg(0, "main", "Wrong arguments!");
        lg(0, "main", "argc = %d", argc);
        exit(-1);
    }

    if (number_of_processes < 1 || number_of_processes > 10) { // checking number of processes
        lg(0, "main", "Wrong number of processes!");
        exit(-1);
    }

    number_of_processes++; // remember about Parent!

    if (argc == 4 && strcmp("--mutexl", argv[3]) == 0) {
        mutexl = 1;
    }


    process array_of_processes[number_of_processes]; // put in an array, 0 process is a main parent process

    for (int i = 0; i < number_of_processes; i++) {
        array_of_processes[i].pipe_read = (int *) malloc(sizeof(int) * number_of_processes); // initialize our array
        array_of_processes[i].pipe_write = (int *) malloc(sizeof(int) * number_of_processes); // initialize our array
        array_of_processes[i].lamport_time = 0;
        array_of_processes[i].rear = -1;
        array_of_processes[i].finished_processes = number_of_processes - 2; // with out 0 and itself
    }

//    lg(0, "main", "Number of processes = %d", number_of_processes);

    create_pipes(array_of_processes); // our function for creating all pipes

    array_of_processes[0].localId = PARENT_ID; // for parent process
    array_of_processes[0].pid = getpid(); // for parent process, get pid for current process

    FILE *event_log = fopen(events_log, "a"); // for writing into file

    create_processes(array_of_processes, event_log, mutexl);

    close_unnecessary_pipes(array_of_processes, 0);

    // First rule: update time before any event (time = time + 1)
    array_of_processes[0].lamport_time++;

    Message message;
    receive_from_all_children(&array_of_processes[0], &message); // receive STARTED for PARENT GOOD!

    // print
//    lg(0, "main", log_received_all_started_fmt, array_of_processes[0].lamport_time, 0);
    fprintf(event_log, log_received_all_started_fmt, array_of_processes[0].lamport_time, 0);
    fflush(event_log);


    // First rule: update time before send or receive event (time = time + 1)
    array_of_processes[0].lamport_time++;

    receive_from_all_children(&array_of_processes[0], &message); // receive all DONE for PARENT


//    lg(0, "main", "last time = %d", array_of_processes[0].lamport_time);


    fclose(event_log);


    for (local_id j = 1; j < number_of_processes; ++j) {
        wait(NULL);
    }

    sleep(1);

    close_all_pipes(array_of_processes);
}


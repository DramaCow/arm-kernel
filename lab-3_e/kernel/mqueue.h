#ifndef __MQUEUE_H
#define __MQUEUE_H

// Simplified subset of POSIX mqueue

typedef int mqd_t; // message queue descriptor (index for kernel)

typedef struct mqueue {
  int msg_qname;  // non-descriptor name  

  int msg_qnum;   // number messages in queue
  int msg_lspid;  // last send process id
  int msg_lrpid;  // last receive process id

  /*const*/ void* msg_qbuf;   // queue data (only one entry for now) 
} mqueue;

/*
 * Establish connection between a process and a message queue __name and
 * return message queue descriptor or (mqd_t) -1 on error. __oflag determines
 * the type of access used. If O_CREAT is on __oflag, the third argument is
 * taken as a `mode_t', the mode of the created message queue, and the fourth
 * argument is taken as `struct mq_attr *', pointer to message queue
 * attributes. If the fourth argument is NULL, default attributes are used.
 */
extern mqd_t mq_open(int name);

/*
 * Remove the association between message queue descriptor __mqdes and its
 * message queue.
 */
extern int mq_close(mqd_t mqd);

/* 
 * Remove message queue named name 
 */
extern int mq_unlink(int name);

/*
 * Receive the oldest message queue
 */
extern int mq_receive(mqd_t mqd, void* msg_ptr /*, size_t msg_len*/);

/* 
 * Add message pointed by msg_ptr to message queue mqd 
 */
extern int mq_send(mqd_t mqd, void* msg_ptr /*, size_t msg_len*/);

#endif

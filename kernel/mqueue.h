#ifndef __MQUEUE_H
#define __MQUEUE_H

#define MSGCHAN_LIMIT 32 // limit on number of message queues open at once

typedef int mqd_t; // message queue descriptor (index for kernel)

typedef struct mqueue {
  int msg_qname;  // non-descriptor name  

  int msg_qnum;   // number messages in queue
  int msg_qrec;   // receiver signal viewed by sender

  int msg_lspid;  // last send process id
  int msg_lrpid;  // last receive process id

  uint8_t msg_qbuf[64]; // queue data 64 byte limit
} mqueue;

mqd_t mq_open(int name); // channel established using magic number
int mq_close(mqd_t mqd);
int mq_unlink(int name);
int mq_receive(mqd_t mqd, uint8_t *msg_ptr, size_t msg_len);
int mq_send(mqd_t mqd, uint8_t *msg_ptr, size_t msg_len);

#endif

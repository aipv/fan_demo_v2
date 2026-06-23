#ifndef STUDENT_LEARNING_H
#define STUDENT_LEARNING_H

int student_learning_start(void);
int sli_send_message(uint32_t command, uint32_t sub_command, const void *payload, uint32_t payload_size);

#endif // STUDENT_LEARNING_H
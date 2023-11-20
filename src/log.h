/* log Header */

#ifndef HEADER_LOG_H
#define HEADER_LOG_H

int log_init (void);
int log_deinit (void);
void log_port_0 (char *mesg, int port);
void log_port_1 (char *mesg, char *mesg1, int port, int data);
void log_port_2 (char *mesg, char *mesg1, char *mesg2, int port, int data1, int data2);
void log_port_16 (char *mesg, char *mesg1, int port, int data1);
void log_data_1 (char *mesg, char *mesg1, int data);
void log_data_2 (char *mesg, char *mesg1, char *mesg2, int data1, int data2);
void log_data_3 (char *mesg, char *mesg1, char *mesg2, char *mesg3, int data1,
     int data2, int data3);
void log_data_4 (char *mesg, char *mesg1, char *mesg2, char *mesg3, char *mesg4,
     int data1, int data2, int data3, int data4);
void log_data_5 (char *mesg, char *mesg1, char *mesg2, char *mesg3, char *mesg4,
     char *mesg5, int data1, int data2, int data3, int data4, int data5);
void log_mesg (char *mesg);

#endif     /* HEADER_LOG_H */

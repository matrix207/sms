#ifndef SMS_H
#define SMS_H

int sms_init(char *sms_port);

int sms_close(void);

int sms_send(char *sms_text,char *number);

#endif /*  SMS_H */


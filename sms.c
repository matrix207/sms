/* 
 * SMS: send, receive and delete
 *
 * History: 
 *      2014-06-17 v1.1 Matrix207
 *      2010-09-29 v1.0 凌彬严  http://bbs.chinaunix.net/thread-1793815-1-1.html
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <string.h>
#define FALSE         0
#define TRUE          1
#define RECEIPT_OK    3
#define RECEIPT_ERROR 4
#define SMS_MAX       10
#define MAX_SMS_TEXT  121
#define MAX_REV_TEXT  255

struct _sms
{
    char sms_id[7];
    char sms_sender[16];
    char sms_data[24];
    char sms_text[MAX_SMS_TEXT];
};

struct _sms_buf
{
    int sms_count;
    struct _sms sms_arr[SMS_MAX];
};

static int fd;
static struct termios serial_old_attr,serial_new_attr;

static int open_serial(char *dev)	
{
    fd=open(dev,O_RDWR);/* |O_NONBLOCK|O_NOCTTY */

    if (fd==-1)
    {
        printf("Fail to open %s\n", dev);
        return FALSE;
    }

    return TRUE;
}


static int serial_init(char *serial_port)	
{
    if( !(open_serial(serial_port)) )
        return FALSE;

    bzero(&serial_old_attr,sizeof(serial_old_attr));
    if(tcgetattr(fd,&serial_old_attr)<0)
        return FALSE;
    serial_new_attr=serial_old_attr;
    if( cfsetispeed(&serial_new_attr,B9600)<0 )
        return FALSE;
    if( cfsetospeed(&serial_new_attr,B9600)<0 )
        return FALSE;
    serial_new_attr.c_cflag|=(CLOCAL|CREAD);
    serial_new_attr.c_cflag&=~PARENB;
    serial_new_attr.c_cflag&=~CSTOPB;
    serial_new_attr.c_cflag&=~CSIZE;
    serial_new_attr.c_cflag|=CS8;
    serial_new_attr.c_lflag&=~(ICANON|ECHO|ECHOE|ISIG);
    serial_new_attr.c_oflag&=~(OPOST);
    serial_new_attr.c_cc[VTIME]=150;
    serial_new_attr.c_cc[VMIN]=0;
    if( tcflush(fd,TCIFLUSH)<0 )
        return FALSE;
    if( tcsetattr(fd,TCSANOW,&serial_new_attr)<0)
        return FALSE;

    return TRUE;
}

static int write_serial(char *string)
{
    int len=strlen(string);
    if ( len > 0 )
    {
        if( write(fd,string,len)==len )
            return TRUE;
    }		

    return FALSE;
}

static int read_serial(char *buffer,int size,int offset)
{
    int len;
    memset(buffer+offset,0,size-offset);
    if( ( len=read(fd,buffer+offset,size-offset) ) <= 0 )
        return FALSE;

    return len;
}

static int sms_receipt(char *rbuf,int rsize,char *text1,char *text2)
{
    fd_set read_fd;
    struct timeval timeout={20,0};
    int num,i,len=0,buffer_len=0;

    bzero(rbuf,rsize);
    for(i=1;i<=4;i++)
    {
        FD_ZERO(&read_fd);
        FD_SET(fd,&read_fd);
        timeout.tv_sec=20;
        timeout.tv_usec=0;
        if( (num=select(fd+1,&read_fd,NULL,NULL,&timeout))>0 )
        {
            if( FD_ISSET(fd,&read_fd) )
            {

                if( (len=read_serial(rbuf,rsize,buffer_len)) )
                {
                    if( !strstr(rbuf,text1) && !strstr(rbuf,text2) ) 
                    {
                        buffer_len+=len;
                        i--;
                        continue;		
                    }
                    else if( strstr(rbuf,text1)!=0 )
                    {
                        return RECEIPT_OK;	
                    }
                    else if( strstr(rbuf,text2)!=0 )
                    {
                        return RECEIPT_ERROR;	
                    }
                }
                else
                {
                    printf("fail to read serial\n");
                    return FALSE;
                }
            }
            else
            {
                printf("fd not in fd_set\n");
                continue;
            }
        }
        else
        {
            printf("select timeout\n");
            return FALSE;
        }
    }
    return FALSE;
}

static int write_and_check(char *wbuf, char *rbuf, int rsize, char *expect_answer) 
{
    int ret = 0;

    printf("send: %s\n", wbuf);
    if( !write_serial(wbuf) )
        return FALSE;

    ret = sms_receipt(rbuf,rsize,expect_answer,"ERROR");
    printf("recv: %s\n", rbuf);

    if( ret==RECEIPT_ERROR )
        return FALSE;
    else if( ret!=RECEIPT_OK )
    {
        printf("fail to recv\n");
        return FALSE;
    }

    return TRUE;
}

int sms_init(char *sms_port)
{
    char wbuf[20] = {0};
    char rbuf[MAX_REV_TEXT] = {0};

    if(	!serial_init(sms_port) )
    {
        printf("Initialization fail.\n");
        return FALSE;
    }

    /* check connection */	
    bzero(wbuf,sizeof(wbuf));
    sprintf(wbuf,"AT\r");
    if ( !write_and_check(wbuf, rbuf, sizeof(rbuf), "OK") )
        return FALSE;

    /*ATE0 - echo off, ATE1 - echo on*/
    bzero(wbuf,sizeof(wbuf));
    sprintf(wbuf,"ATE0\r");
    if ( !write_and_check(wbuf, rbuf, sizeof(rbuf), "OK") )
        return FALSE;

    /* Set the new message indicators */
    bzero(wbuf,sizeof(wbuf));
    sprintf(wbuf,"AT+CNMI=2,1\r");
    if ( !write_and_check(wbuf, rbuf, sizeof(rbuf), "OK") )
        return FALSE;

    /* SMS format */
    /*0 = PDU Mode, 1 = Text Mode*/
    bzero(wbuf,sizeof(wbuf));
    sprintf(wbuf,"AT+CMGF=1\r");
    if ( !write_and_check(wbuf, rbuf, sizeof(rbuf), "OK") )
        return FALSE;

    return TRUE;
}		

int sms_close(void)
{
    if( tcsetattr(fd,TCSANOW,&serial_old_attr)<0)
        return FALSE;
    printf("close device\n");
    close(fd);

    return TRUE;
}

int sms_send(char *sms_text,char *number)
{
    char wbuf[MAX_SMS_TEXT] = {0};
    char rbuf[MAX_REV_TEXT] = {0};

    bzero(wbuf,sizeof(wbuf));
    sprintf(wbuf,"AT+CMGS=");
    strcat(wbuf,number);
    strcat(wbuf,"\r");

    if ( !write_and_check(wbuf, rbuf, sizeof(rbuf), ">") )
        return FALSE;

    bzero(wbuf,sizeof(wbuf));
    strcat(wbuf,sms_text);
    strcat(wbuf,"\x1a");

    if ( !write_and_check(wbuf, rbuf, sizeof(rbuf), "OK") )
        return FALSE;

    return TRUE;
}

int sms_read_one(char *rbuf,int size,char *sms_id)
{
    char wbuf[MAX_SMS_TEXT] = {0};
    int ret = FALSE;

    sprintf(wbuf,"AT+CMGR=");
    strcat(wbuf,sms_id);
    strcat(wbuf,"\r");

    ret = write_and_check(wbuf, rbuf, size, "OK");

    return ret;
}

/*
 * translate messages
 */
static void sms_trans(struct _sms_buf *sms_buf, char *read_buffer)
{
    int index = 0;
    char *find_start,*find_end;

    find_start=read_buffer;
    bzero(sms_buf,sizeof(sms_buf));
    while( (find_start=strstr(find_start,"+CMGL:"))!=0 && (index<SMS_MAX))
    {
        /* fill ID */
        find_start+=6;
        if( (find_end=strstr(find_start,","))==0 ) return;
        bcopy( find_start+1,&( (sms_buf->sms_arr[index]).sms_id ),find_end-find_start );
        find_start=find_end;
        printf("ID:%s  ",(sms_buf->sms_arr[index]).sms_id);

        /* fill sender */
        if( (find_start=strstr(find_start,"+86")+2)==0 ) return;
        if( (find_end=strstr(find_start,"\""))==0 ) return;
        bcopy( find_start+1,&( (sms_buf->sms_arr[index]).sms_sender ),find_end-find_start );
        find_start=find_end;
        printf("sender:%s  ",(sms_buf->sms_arr[index]).sms_sender);

        /* fill date */
        if( (find_start=strstr(find_start+1,"\""))==0 ) return;
        if( (find_end=strstr(find_start+1,"\""))==0 ) return;
        bcopy( find_start+1,&( (sms_buf->sms_arr[index]).sms_data ),find_end-find_start );
        find_start=find_end;
        printf("date:%s  ",(sms_buf->sms_arr[index]).sms_data);

        /* fill content */
        if( (find_start=strstr(find_start,"\n")+1)==0 ) return;
        if( (find_end=strstr(find_start+1,"\n"))==0 ) return;
        bcopy( find_start+1,&( (sms_buf->sms_arr[index]).sms_text ),find_end-find_start );
        find_start=find_end;
        printf("content:%s  \n",(sms_buf->sms_arr[index]).sms_text);

        index++;
    }
    sms_buf->sms_count = index+1;

    return;
}

/*
 * read sms message
 * Parameters:
 *    sms_buf, point to structu sms_buf
 *    size,    size of sms_buf
 *    id,      0:read all message, other: read the specify id message
 * Return:
 *    0: fail
 *    N: the real count of message had been read
 */
int sms_read(struct _sms_buf *sms_buf, int id)
{
    char wbuf[MAX_SMS_TEXT] = {0};
    char *rbuf = NULL;
    int rsize = SMS_MAX * sizeof(struct _sms);
    int ret = FALSE; 
    char id_s[10] = {0};

    /* TODO: Is there have another characters? if yes, should fix rsize */
    if ( (rbuf = (char*) malloc(rsize)) == NULL)
        return FALSE;
    if (id == 0) 
        sprintf(wbuf,"AT+CMGL=\"ALL\"\r");
    else
    {
        sprintf(wbuf,"AT+CMGR=");
        sprintf(id_s, "%d", id);
        strcat(wbuf,id_s);
        strcat(wbuf,"\r");
    }

    if ( write_and_check(wbuf, rbuf, rsize, "OK") )
    {
        sms_trans(sms_buf,rbuf);
        ret = TRUE;
    }

    free(rbuf);

    return ret;
}

/*
 * delete sms message
 * return:
 *     TRUE, success
 *     FALSE, fail
 */
int sms_del(int all_del,char *sms_id)
{
    char wbuf[MAX_SMS_TEXT] = {0};
    char rbuf[MAX_REV_TEXT] = {0};

    if(all_del) 
    {
        /* delete all messages */
        sprintf(wbuf,"AT+CMGD=1,1\r");
    }
    else
    {
        sprintf(wbuf,"AT+CMGD=");
        strcat(wbuf,sms_id);
        strcat(wbuf,"\r");
    }

    return write_and_check(wbuf, rbuf, sizeof(rbuf), "OK");
}


/* sample */
int main(int argc,char **argv)
{
    char *dev=NULL;
    char *phone_num=NULL;
    char *sms_text=NULL;
    char read_buffer[10240];
    struct _sms_buf sms_buf;

    if (argc < 3) {
        printf("Usage: %s [device_name] [phone_number] [sms_text]\n", argv[0]);
        printf("       %s /dev/ttyUSB0 13800008888 \"Hi,sms TT.\"\n", argv[0]);
        return 0;
    }

    dev = argv[1];
    phone_num = argv[2];
    sms_text = argv[3];

    if( sms_init(dev))
        printf("Initialization OK.\n");
    else
    {
        printf("Initialization fail.\n");
        goto Exit;
    }

    if( sms_send(sms_text, phone_num) )
        printf("Success to send SMS.\n");
#if 0
    if( sms_read_one(read_buffer,sizeof(read_buffer),"1") )
        printf(read_buffer);


    if( sms_read_all(&sms_buf) )
        printf("Success to read SMS text, item count: %d\n",sms_buf.sms_count);

    if( sms_del(TRUE,"1"))
        printf("Success to delete SMS.\n");
#endif

Exit:
    sms_close(); 

    return 0;
}


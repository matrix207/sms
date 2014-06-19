sms
===

send sms by GSM modem

###Introduce

* The origin model is from <http://bbs.chinaunix.net/thread-1793815-1-1.html>

* Send SMS is ok, recv and delete need to check.

* Have been test for Fedora 20, Ubuntu 12.04

###How to use
1. Build
    * Make

2. Check device
    + Plug a GSM modem to your machine with USB line 
      The device I used is wavecom gsm m1306b, and its interface is usb.
    + Use command `lsusb` to check, mostly it will show as "...PL2303 Serial Port"  
        [dennis@localhost sms]$ lsusb  
        Bus 001 Device 011: ID 148f:760b Ralink Technology, Corp.   
        Bus 001 Device 001: ID 1d6b:0002 Linux Foundation 2.0 root hub  
        Bus 005 Device 002: ID 046d:c52f Logitech, Inc. Unifying Receiver  
        Bus 005 Device 001: ID 1d6b:0001 Linux Foundation 1.1 root hub  
        Bus 004 Device 009: ID 067b:2303 Prolific Technology, Inc. PL2303 Serial Port  
        Bus 004 Device 001: ID 1d6b:0001 Linux Foundation 1.1 root hub  
        Bus 003 Device 001: ID 1d6b:0001 Linux Foundation 1.1 root hub  
        Bus 002 Device 003: ID 413c:2105 Dell Computer Corp. Model L100 Keyboard  
        Bus 002 Device 002: ID 0461:4d22 Primax Electronics, Ltd   
        Bus 002 Device 001: ID 1d6b:0001 Linux Foundation 1.1 root hub  

3. Usage, should run the application with root authentication 
    * ./sms [device_name] [phone_number] [sms_text]

4. Sample, test on Fedora 20. Because my device antenna is broken, so most of 
   the time failed to send message.

        [dennis@localhost sms]$ ls
        Makefile  README.md  sms.c  sms.h
        [dennis@localhost sms]$ make
        gcc    -c -o sms.o sms.c
        gcc -Wall -o sms sms.o
        [dennis@localhost sms]$ ls
        Makefile  README.md  sms  sms.c  sms.h  sms.o
        [dennis@localhost sms]$ su -c './sms /dev/ttyUSB0 137******** "Hi, I am GSM modem"'
        Password: 
        send: AT
        recv: AT
        OK
        send: ATE0
        recv: 
        ATE0
        OK
        send: AT+CNMI=2,1
        recv: 

        OK
        send: AT+CMGF=1
        recv: 

        OK
        Initialization OK.
        send: AT+CMGS=137********
        recv: 

        >
        send: Hi, I am GSM modem
        recv:  
        ERROR
        close device

        [dennis@localhost sms]$ su -c './sms /dev/ttyUSB0 137******** "Hi, I am GSM modem"'
        Password: 
        send: AT
        recv: 
        OK
        send: ATE0
        recv: 

        OK
        send: AT+CNMI=2,1
        recv: 

        OK
        send: AT+CMGF=1
        recv: 

        OK
        Initialization OK.
        send: AT+CMGS=137********
        recv: 

        >
        send: Hi, I am GSM modem
        recv:  
        +CMGS: 10

        OK
        Success to send SMS.
        close device
        
###Todo
* Finish receive and delete sms message?
* Implement calling?

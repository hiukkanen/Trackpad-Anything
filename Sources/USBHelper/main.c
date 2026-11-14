// SPDX-License-Identifier: GPL-3.0-or-later
// USB transport only. No gesture recognition or settings run as root.
#include <libusb.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
static volatile sig_atomic_t running=1;
static void stop(int s){(void)s;running=0;}
int main(int argc,char**argv){
 setbuf(stdout,NULL);if(argc!=3||geteuid()!=0){fprintf(stderr,"Usage: usb-helper SOCKET USER_ID (administrator required)\n");return 64;}
 char*end;unsigned long raw=strtoul(argv[2],&end,10);if(!*argv[2]||*end||raw<500||raw>UINT32_MAX)return 64;uid_t uid=(uid_t)raw;
 struct sockaddr_un addr={0};addr.sun_family=AF_UNIX;if(strlen(argv[1])>=sizeof(addr.sun_path))return 64;strcpy(addr.sun_path,argv[1]);
 struct stat st;if(lstat(addr.sun_path,&st)||!S_ISSOCK(st.st_mode)||st.st_uid!=uid||(st.st_mode&0077)){fputs("Unsafe receiver socket\n",stderr);return 65;}
 int fd=socket(AF_UNIX,SOCK_DGRAM,0);if(fd<0||connect(fd,(struct sockaddr*)&addr,sizeof(addr))<0){perror("receiver");return 5;}
 signal(SIGINT,stop);signal(SIGTERM,stop);signal(SIGHUP,stop);printf("USB helper pid=%d\n",getpid());
 libusb_context*ctx=NULL;if(libusb_init(&ctx)){close(fd);return 1;}libusb_device_handle*h=libusb_open_device_with_vid_pid(ctx,0x5ac,0x324);if(!h){puts("USB device not found");close(fd);libusb_exit(ctx);return 1;}
 int r=libusb_detach_kernel_driver(h,1);printf("Temporary USB capture=%s\n",libusb_error_name(r));if(r){libusb_close(h);libusb_exit(ctx);close(fd);return 2;}
 int claimed=0;r=libusb_claim_interface(h,1);printf("Claim=%s\n",libusb_error_name(r));if(!r){claimed=1;unsigned char mode[]={2,1};r=libusb_control_transfer(h,0x21,9,0x0302,1,mode,2,1000);printf("Multitouch mode=%d\n",r);
 if(r==2){send(fd,"READY",5,0);while(running){
 // Exit if GUI removes/replaces its socket, even when the surface is idle.
 struct stat current;if(lstat(addr.sun_path,&current)||current.st_ino!=st.st_ino||current.st_uid!=uid)break;
 unsigned char b[4096];int n=0;r=libusb_interrupt_transfer(h,0x83,b,sizeof(b),&n,100);
 if(!r){if(send(fd,b,n,0)<0)break;}else if(r!=LIBUSB_ERROR_TIMEOUT&&r!=LIBUSB_ERROR_INTERRUPTED){printf("USB error %s\n",libusb_error_name(r));break;}
 }} }
 if(claimed)libusb_release_interface(h,1);r=libusb_attach_kernel_driver(h,1);printf("Restore system driver=%s\n",libusb_error_name(r));send(fd,"END",3,0);close(fd);libusb_close(h);libusb_exit(ctx);return r?3:0;
}

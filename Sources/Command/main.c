#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <spawn.h>
#include <sys/wait.h>
extern char **environ;
int main(int argc,char**argv){if(argc!=2||(strcmp(argv[1],"left")&&strcmp(argv[1],"right"))){fprintf(stderr,"Usage: space-command left|right\n");return 2;}const char*msg=!strcmp(argv[1],"left")?"SPACE_LEFT":"SPACE_RIGHT";
 struct sockaddr_un a={0};a.sun_family=AF_UNIX;const char*home=getenv("HOME");if(!home)return 2;snprintf(a.sun_path,sizeof(a.sun_path),"%s/Library/Application Support/TrackpadCompat/trackpad.sock",home);
 for(int i=0;i<60;i++){int fd=socket(AF_UNIX,SOCK_DGRAM,0);int r=(int)sendto(fd,msg,strlen(msg),0,(struct sockaddr*)&a,sizeof(a));close(fd);if(r==(int)strlen(msg))return 0;if(i==0){pid_t p;char*args[]={"/usr/bin/open","-g","-b","org.trackpadcompat.gui.development",NULL};if(!posix_spawn(&p,args[0],NULL,NULL,args,environ))waitpid(p,NULL,0);}usleep(100000);}fprintf(stderr,"Trackpad Anything is not ready; check Accessibility permission.\n");return 1;}

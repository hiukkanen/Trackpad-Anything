#ifndef TC_RUNTIME_H
#define TC_RUNTIME_H
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
static int tc_paths(uid_t uid,char*dir,size_t ds,char*socketPath,size_t ss){
 if(snprintf(dir,ds,"/tmp/trackpadcompat-%u",(unsigned)uid)>=(int)ds)return -1;
 if(snprintf(socketPath,ss,"%s/input.sock",dir)>=(int)ss)return -1;
 return 0;
}
static int tc_check_dir(const char*dir,uid_t uid,int create){struct stat st;if(create&&mkdir(dir,0700)&&errno!=EEXIST)return -1;if(lstat(dir,&st)||!S_ISDIR(st.st_mode)||st.st_uid!=uid||(st.st_mode&0077))return -1;return 0;}
#endif

// USB-C Magic Trackpad compatibility helper for macOS 14.
// Packet layout documented by Linux hid-magicmouse (GPL-2.0-or-later).
// This file is GPL-2.0-or-later. Gesture fields cross-checked against WebKit TestSPI.
#include <libusb.h>
#include <ApplicationServices/ApplicationServices.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <math.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
static volatile sig_atomic_t running=1;
static CGEventSourceRef source;
static int dragging3=0;
static int previousCount=0, previousMask=0, held=0, gesture=0, fired=0,maxCount=0,hadClick=0;
static double px,py,sx,sy,previousDistance,startDistance,movement,started;
static unsigned long frames=0,scrolls=0,zooms=0,swipes=0;
static void stop(int s){(void)s;running=0;}
void compat_stop(void){running=0;}
static CGPoint location(void){CGEventRef e=CGEventCreate(NULL);CGPoint p=CGEventGetLocation(e);CFRelease(e);return p;}
static void mouse(CGEventType type,CGPoint p,int button){CGEventRef e=CGEventCreateMouseEvent(source,type,p,button);if(e){CGEventSetFlags(e,CGEventSourceFlagsState(kCGEventSourceStateCombinedSessionState));CGEventPost(kCGHIDEventTap,e);CFRelease(e);}}
static void click(int right){CGPoint p=location();mouse(right?kCGEventRightMouseDown:kCGEventLeftMouseDown,p,right?1:0);mouse(right?kCGEventRightMouseUp:kCGEventLeftMouseUp,p,right?1:0);}
static void scroll(double x,double y,int phase){CGEventRef e=CGEventCreateScrollWheelEvent(source,kCGScrollEventUnitPixel,2,(int)llround(y),(int)llround(x));if(!e)return;CGEventSetIntegerValueField(e,kCGScrollWheelEventIsContinuous,1);CGEventSetIntegerValueField(e,kCGScrollWheelEventScrollPhase,phase);CGEventSetDoubleValueField(e,kCGScrollWheelEventPointDeltaAxis1,y);CGEventSetDoubleValueField(e,kCGScrollWheelEventPointDeltaAxis2,x);CGEventPost(kCGHIDEventTap,e);CFRelease(e);scrolls++;}
static void zoom(double delta,int phase){CGEventRef e=CGEventCreate(source);if(!e)return;CGEventSetType(e,(CGEventType)29);CGEventSetLocation(e,location());CGEventSetIntegerValueField(e,(CGEventField)110,8);CGEventSetIntegerValueField(e,(CGEventField)132,phase);CGEventSetDoubleValueField(e,(CGEventField)113,delta);CGEventPost(kCGHIDEventTap,e);CFRelease(e);zooms++;}
#include "dock-gesture.h"
// Dock action API used by LinearMouse DockKit (MIT); see ThirdParty-LinearMouse.txt.
extern CGError CoreDockSendNotification(CFStringRef notification,int unknown);
static int queue[32],qh=0,qt=0,autoActive=0;static double autoStart,autoLast,autoDirection;
static void requestSpace(int direction){if(qt-qh<32){queue[qt++%32]=direction;printf("Keyboard space request %s\n",direction>0?"right":"left");}}
static void automaticTick(void){double now=CFAbsoluteTimeGetCurrent();if(!autoActive&&qh<qt&&!previousCount&&!gesture){autoDirection=queue[qh++%32];autoStart=now;autoLast=0;autoActive=1;dockBegin(1);swipes++;}
 if(autoActive){double t=fmin(1,(now-autoStart)/.25);double progress=autoDirection*1.5*t;dockMove(progress-autoLast);autoLast=progress;if(t>=1){dockEnd();autoActive=0;printf("Keyboard Dock swipe completed progress=%.2f\n",dockProgress);}}}
static int signed13(unsigned v){v&=8191;return (v&4096)?(int)v-8192:(int)v;}
static void finishGesture(void){if(gesture==1)scroll(0,0,4);if(gesture==2)zoom(0,4);if(gesture==3){dockEnd();printf("Dock swipe end axis=%d progress=%.3f\n",dockAxis,dockProgress);}gesture=0;}
static void frame(unsigned char*b,int len){if(autoActive||len<12||b[0]!=2||(len-12)%9)return;int n=0,mask=0;double x=0,y=0,fx[16],fy[16];
 for(int k=12;k+9<=len&&n<16;k+=9){unsigned char*t=b+k;if((t[3]&0xc0)!=0x80||t[4]>200||t[5]>200)continue;fx[n]=signed13(t[0]|t[1]<<8);fy[n]=-signed13((t[1]>>5)|(t[2]<<3)|(t[3]<<11));x+=fx[n];y+=fy[n];mask|=1<<(t[8]&15);n++;}
 if(dragging3&&n!=3){mouse(kCGEventLeftMouseUp,location(),0);dragging3=0;}
 frames++;double now=CFAbsoluteTimeGetCurrent();if(n){x/=n;y/=n;}double distance=n==2?hypot(fx[0]-fx[1],fy[0]-fy[1]):0;int button=b[1]&1;
 if(button&&!held){held=n>=2?2:1;hadClick=1;mouse(held==2?kCGEventRightMouseDown:kCGEventLeftMouseDown,location(),held==2?1:0);}
 if(!button&&held){mouse(held==2?kCGEventRightMouseUp:kCGEventLeftMouseUp,location(),held==2?1:0);held=0;}
 if(!n){finishGesture();if(previousCount&&maxCount<=2&&!hadClick&&now-started<0.22&&movement<65)click(maxCount==2);previousCount=previousMask=maxCount=hadClick=fired=0;movement=0;return;}
 if(maxCount>=4&&n<4){finishGesture();previousCount=n;previousMask=mask;return;}
 if(!previousCount){started=now;sx=x;sy=y;movement=0;maxCount=n;hadClick=button;fired=0;}
 if(n>maxCount)maxCount=n;
 if(n!=previousCount||mask!=previousMask){if(!(gesture==3&&n>=4))finishGesture();px=x;py=y;sx=x;sy=y;previousDistance=startDistance=distance;previousCount=n;previousMask=mask;return;}
 double dx=x-px,dy=y-py;movement+=hypot(dx,dy);
 if(n==1){CGPoint p=location();p.x+=dx*0.30;p.y+=dy*0.30;mouse(held?(held==2?kCGEventRightMouseDragged:kCGEventLeftMouseDragged):kCGEventMouseMoved,p,held==2?1:0);}
 else if(n==2&&!held){if(!gesture){double travel=hypot(x-sx,y-sy);if(fabs(distance-startDistance)>100&&fabs(distance-startDistance)>travel*1.6){gesture=2;zoom(0,1);}else if(travel>30){gesture=1;scroll(0,0,1);}}
 if(gesture==1)scroll(dx*.22,dy*.22,2);if(gesture==2&&previousDistance>100){double delta=(distance-previousDistance)/previousDistance;if(fabs(delta)<.25)zoom(delta,2);}}
 else if(n==3&&!held){if(!dragging3&&hypot(x-sx,y-sy)>25){dragging3=1;hadClick=1;mouse(kCGEventLeftMouseDown,location(),0);}if(dragging3){CGPoint p=location();p.x+=dx*.30;p.y+=dy*.30;mouse(kCGEventLeftMouseDragged,p,0);}}
 else if(n>=4&&!held){double ax=x-sx,ay=y-sy;if(fired){}else if(gesture!=3&&ay<0&&fabs(ay)>fabs(ax)*1.3){if(ay< -180){fired=1;swipes++;CGError result=CoreDockSendNotification(CFSTR("com.apple.expose.awake"),0);printf("Mission Control direct Dock result=%d\n",result);}}else if(gesture!=3&&hypot(ax,ay)>65){int axis=fabs(ax)>fabs(ay)*1.3?1:fabs(ay)>fabs(ax)*1.3?2:0;if(axis){gesture=3;dockBegin(axis);swipes++;printf("Dock swipe begin axis=%d\n",axis);}}if(gesture==3)dockMove((dockAxis==1?-dx:-dy)/1600.0);}
 px=x;py=y;previousDistance=distance;previousCount=n;previousMask=mask;
}

int main(){setbuf(stdout,NULL);printf("Gesture receiver pid=%d accessibility=%d\n",getpid(),AXIsProcessTrusted());if(!AXIsProcessTrusted()){puts("Accessibility required");return 3;}
 signal(SIGINT,stop);signal(SIGTERM,stop);signal(SIGHUP,stop);source=CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
 char path[512];snprintf(path,sizeof(path),"%s/Library/Application Support/TrackpadCompat/trackpad.sock",getenv("HOME")?getenv("HOME"):"");
 int fd=socket(AF_UNIX,SOCK_DGRAM,0);struct sockaddr_un addr={0};addr.sun_family=AF_UNIX;strncpy(addr.sun_path,path,sizeof(addr.sun_path)-1);unlink(path);if(bind(fd,(struct sockaddr*)&addr,sizeof(addr))<0){perror("bind");return 4;}chmod(path,0600);struct timeval timeout={0,16000};setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));
 time_t nextlog=time(NULL)+10;while(running){dockTick();automaticTick();unsigned char b[4096];ssize_t n=recv(fd,b,sizeof(b),0);if(n==10&&!memcmp(b,"SPACE_LEFT",10)){requestSpace(-1);continue;}if(n==11&&!memcmp(b,"SPACE_RIGHT",11)){requestSpace(1);continue;}
 if(n==3&&!memcmp(b,"END",3)){finishGesture();if(dragging3)mouse(kCGEventLeftMouseUp,location(),0);if(held)mouse(held==2?kCGEventRightMouseUp:kCGEventLeftMouseUp,location(),held==2?1:0);held=dragging3=previousCount=previousMask=maxCount=0;puts("USB disconnected; keyboard gestures remain available");continue;}if(n>0)frame(b,(int)n);else if(n<0&&errno!=EAGAIN&&errno!=EWOULDBLOCK&&errno!=EINTR)break;
 if(time(NULL)>=nextlog){printf("frames=%lu scroll=%lu pinch=%lu swipe=%lu\n",frames,scrolls,zooms,swipes);nextlog=time(NULL)+10;}}
 finishGesture();if(dragging3)mouse(kCGEventLeftMouseUp,location(),0);if(held)mouse(held==2?kCGEventRightMouseUp:kCGEventLeftMouseUp,location(),held==2?1:0);close(fd);unlink(path);CFRelease(source);puts("Receiver stopped");return 0;}

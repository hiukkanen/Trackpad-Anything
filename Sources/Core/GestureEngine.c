// SPDX-License-Identifier: GPL-3.0-or-later
#include "GestureEngine.h"
#include "Protocol.h"
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ApplicationServices/ApplicationServices.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>
#include <stdatomic.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>

typedef struct {
#define TC_OPTION(key,field,def,min,max) double field;
#include "Options.def"
#undef TC_OPTION
} Options;
static Options config={
#define TC_OPTION(key,field,def,min,max) .field=def,
#include "Options.def"
#undef TC_OPTION
}, opt;
static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
static _Atomic int running=0;
static void(*systemAction)(const char*);
static unsigned long frames,scrolls,swipes;
static TCFrame live;
static int connected,active;
static char lastAction[96]="Ready",errorText[160]="";
static int requests[32],head,tail;
static CGEventSourceRef source;
static int oldCount,oldMask,maxCount,held,dragging,gesture,fired,hadClick;
static double px,py,sx,sy,pdist,sdist,pangle,sangle,pradius,sradius,movement,startTime,lastFrame;
static double vx,vy,inertiaX,inertiaY,inertiaAt,inertiaNext,pendingTap,tap2Time;
static int inertia,autoActive;
static double autoStart,autoTarget,autoLast;
static int autoAxis;
static double lastClick;static CGPoint lastClickPoint;static int clickCount,clickButton;
static void publishAction(const char*s){pthread_mutex_lock(&lock);snprintf(lastAction,sizeof(lastAction),"%s",s);pthread_mutex_unlock(&lock);}
static void publishError(const char*s){pthread_mutex_lock(&lock);snprintf(errorText,sizeof(errorText),"%s",s);pthread_mutex_unlock(&lock);}
static CGPoint location(void){CGEventRef e=CGEventCreate(NULL);CGPoint p=CGEventGetLocation(e);CFRelease(e);return p;}
static void mouse(CGEventType type,CGPoint p,int button){
 double now=CFAbsoluteTimeGetCurrent();if(type==kCGEventLeftMouseDown||type==kCGEventRightMouseDown){clickCount=(button==clickButton&&now-lastClick<.4&&hypot(p.x-lastClickPoint.x,p.y-lastClickPoint.y)<8)?clickCount%3+1:1;lastClick=now;lastClickPoint=p;clickButton=button;}
 CGEventRef e=CGEventCreateMouseEvent(source,type,p,button);if(!e)return;CGEventSetFlags(e,CGEventSourceFlagsState(kCGEventSourceStateCombinedSessionState));CGEventSetIntegerValueField(e,kCGMouseEventClickState,clickCount);CGEventPost(kCGHIDEventTap,e);CFRelease(e);
}
static void click(int right){CGPoint p=location();mouse(right?kCGEventRightMouseDown:kCGEventLeftMouseDown,p,right?1:0);mouse(right?kCGEventRightMouseUp:kCGEventLeftMouseUp,p,right?1:0);}
static void scrollEvent(double x,double y,int phase,int momentum){CGEventRef e=CGEventCreateScrollWheelEvent(source,kCGScrollEventUnitPixel,2,(int)llround(y),(int)llround(x));if(!e)return;CGEventSetIntegerValueField(e,kCGScrollWheelEventIsContinuous,1);CGEventSetIntegerValueField(e,kCGScrollWheelEventScrollPhase,phase);CGEventSetIntegerValueField(e,kCGScrollWheelEventMomentumPhase,momentum);CGEventSetDoubleValueField(e,kCGScrollWheelEventPointDeltaAxis1,y);CGEventSetDoubleValueField(e,kCGScrollWheelEventPointDeltaAxis2,x);CGEventPost(kCGHIDEventTap,e);CFRelease(e);scrolls++;}
static void appGesture(int subtype,int valueField,double value,int phase){CGEventRef e=CGEventCreate(source);if(!e)return;CGEventSetType(e,(CGEventType)29);CGEventSetLocation(e,location());CGEventSetIntegerValueField(e,(CGEventField)110,subtype);CGEventSetIntegerValueField(e,(CGEventField)132,phase);if(valueField==115)CGEventSetIntegerValueField(e,(CGEventField)valueField,(int64_t)value);else if(valueField)CGEventSetDoubleValueField(e,(CGEventField)valueField,value);CGEventPost(kCGHIDEventTap,e);CFRelease(e);}
#include "DockGesture.h"
static void cancelMomentum(void){if(inertia)scrollEvent(0,0,0,3);inertia=0;vx=vy=0;}
static void finishGesture(int allowMomentum){if(gesture==1){scrollEvent(0,0,4,0);if(allowMomentum&&opt.momentum&&hypot(vx,vy)>1.2){inertia=1;inertiaX=vx;inertiaY=vy;inertiaAt=inertiaNext=CFAbsoluteTimeGetCurrent();}}if(gesture==2)appGesture(8,113,0,4);if(gesture==4)appGesture(5,114,0,4);if(gesture==3){dockEnd();printf("Dock end axis=%d progress=%.3f\n",dockAxis,dockProgress);}gesture=0;}
static void releaseButtons(void){if(held)mouse(held==2?kCGEventRightMouseUp:kCGEventLeftMouseUp,location(),held==2?1:0);if(dragging)mouse(kCGEventLeftMouseUp,location(),0);held=dragging=0;}
static void resetContacts(void){finishGesture(0);cancelMomentum();releaseButtons();oldCount=oldMask=maxCount=hadClick=fired=0;pendingTap=0;movement=0;}
static double angleDiff(double a,double b){double d=a-b;while(d>M_PI)d-=2*M_PI;while(d< -M_PI)d+=2*M_PI;return d;}
void TCSetSystemActionCallback(void(*cb)(const char*)){systemAction=cb;}
int TCSetOption(const char*k,double v){if(!k||!isfinite(v))return 0;int found=0;pthread_mutex_lock(&lock);
#define TC_OPTION(key,field,def,min,max) if(!strcmp(k,#key)&&v>=min&&v<=max&&((min!=0||max!=1)||v==0||v==1)){config.field=v;found=1;}
#include "Options.def"
#undef TC_OPTION
 if(found&&config.drag&&config.lookup){if(!strcmp(k,"lookup"))config.drag=0;else if(!strcmp(k,"threeFingerDrag"))config.lookup=0;}
 pthread_mutex_unlock(&lock);return found;}
const char*TCOptionsJSON(void){static _Thread_local char out[2048];size_t at=0;pthread_mutex_lock(&lock);at+=snprintf(out+at,sizeof(out)-at,"{");
#define TC_OPTION(key,field,def,min,max) at+=snprintf(out+at,sizeof(out)-at,"\"" #key "\":%.6g,",config.field);
#include "Options.def"
#undef TC_OPTION
 out[at-1]='}';out[at]=0;pthread_mutex_unlock(&lock);return out;}
void TCRequestAction(const char*action){int code=!strcmp(action,"left")?-1:!strcmp(action,"right")?1:!strcmp(action,"mission-control")?2:!strcmp(action,"app-expose")?-2:!strcmp(action,"show-desktop")?3:!strcmp(action,"launchpad")?-3:0;if(!code)return;pthread_mutex_lock(&lock);if(tail-head<32)requests[tail++%32]=code;pthread_mutex_unlock(&lock);}
static void ticks(void){pthread_mutex_lock(&lock);if(!oldCount)opt=config;pthread_mutex_unlock(&lock);double now=CFAbsoluteTimeGetCurrent();dockTick();
 if(pendingTap&&now>=pendingTap&&!oldCount){if(opt.secondary)click(1);pendingTap=0;}
 if(inertia&&now>=inertiaNext){double dt=fmin(.05,now-inertiaAt);inertiaAt=now;inertiaNext=now+.016;scrollEvent(inertiaX,inertiaY,0,inertia==1?1:2);inertia=2;double decay=exp(-dt*7);inertiaX*=decay;inertiaY*=decay;if(hypot(inertiaX,inertiaY)<.25)cancelMomentum();}
 if(!autoActive&&!oldCount&&!gesture&&!inertia){int code=0;pthread_mutex_lock(&lock);if(head<tail)code=requests[head++%32];pthread_mutex_unlock(&lock);if(code&&abs(code)>1){if(systemAction)systemAction(code==2?"mission-control":code==-2?"app-expose":code==3?"show-desktop":"launchpad");return;}if(code){autoAxis=abs(code);autoTarget=code>0?1:-1;autoStart=now;autoLast=0;autoActive=1;dockBegin(autoAxis);swipes++;publishAction(autoAxis==1?"Desktop switch":autoAxis==2?(code>0?"Mission Control":"App Exposé"):(code>0?"Show desktop":"Launchpad"));}}
 // Quick constant-speed flick to 60% of a space, released with that speed: the Dock finishes the switch with its
 // own animation. (Ending near zero speed stalled it halfway before snapping; pushing past 100% made it bounce.)
 if(autoActive){double t=fmin(1,(now-autoStart)/.12),p=autoTarget*.6*t;dockMove(p-autoLast);autoLast=p;if(t>=1){dockDelta=autoTarget*.6*(.016/.12);dockEnd();autoActive=0;}}
}
static void handleFrame(const uint8_t*b,int len){TCFrame f;if(!TCDecode(b,len,&f))return;pthread_mutex_lock(&lock);live=f;if(!oldCount)opt=config;connected=1;frames++;pthread_mutex_unlock(&lock);if(autoActive)return;
 double now=CFAbsoluteTimeGetCurrent(),dt=lastFrame?fmin(.05,now-lastFrame):.01;lastFrame=now;int n=f.count;
 if(n&&inertia)cancelMomentum();if(dragging&&n!=3){mouse(kCGEventLeftMouseUp,location(),0);dragging=0;}
 if(f.button&&!held){pendingTap=0;held=n>=2&&opt.secondary?2:1;hadClick=1;mouse(held==2?kCGEventRightMouseDown:kCGEventLeftMouseDown,location(),held==2?1:0);}
 if(!f.button&&held){mouse(held==2?kCGEventRightMouseUp:kCGEventLeftMouseUp,location(),held==2?1:0);held=0;}
 if(!n){int completedGesture=gesture;finishGesture(1);if(oldCount&&!completedGesture&&!hadClick&&now-startTime<.22&&movement<65){if(maxCount==1&&opt.tap){click(0);publishAction("Tap to click");}else if(maxCount==2){if(opt.smart&&pendingTap&&now-tap2Time<.4){pendingTap=0;appGesture(22,0,0,0);publishAction("Smart zoom");}else if(opt.smart){tap2Time=now;pendingTap=now+.32;}else if(opt.secondary)click(1);}else if(maxCount==3&&opt.lookup&&!opt.drag&&systemAction){systemAction("lookup");publishAction("Look up");}}
 oldCount=oldMask=maxCount=hadClick=fired=0;movement=0;return;}
 if(maxCount>=4&&n<4){finishGesture(0);oldCount=n;oldMask=f.mask;return;}
 if(!oldCount){startTime=now;movement=0;maxCount=n;hadClick=f.button;fired=0;}
 maxCount=n>maxCount?n:maxCount;
 if(n!=oldCount||f.mask!=oldMask){if(!(gesture==3&&n>=4))finishGesture(0);px=sx=f.x;py=sy=f.y;pdist=sdist=f.distance;pangle=sangle=f.angle;pradius=sradius=f.radius;oldCount=n;oldMask=f.mask;return;}
 double dx=f.x-px,dy=f.y-py;movement+=hypot(dx,dy);
 if(n==1||(held&&n==2)){CGPoint p=location();p.x+=dx*opt.pointer;p.y+=dy*opt.pointer;mouse(held?(held==2?kCGEventRightMouseDragged:kCGEventLeftMouseDragged):kCGEventMouseMoved,p,held==2?1:0);}
 else if(n==2&&!held&&!fired){double travel=hypot(f.x-sx,f.y-sy),radial=fabs(f.distance-sdist),turn=fabs(angleDiff(f.angle,sangle));
 if(!gesture&&opt.edge&&sx>2750&&f.x-sx< -300&&fabs(f.y-sy)<300){if(systemAction)systemAction("notification-center");fired=1;publishAction("Notification Center");}
 else if(!gesture&&opt.pages&&fabs(f.x-sx)>420&&fabs(f.x-sx)>fabs(f.y-sy)*2){appGesture(16,115,f.x>sx?8:4,1);appGesture(16,115,0,4);fired=1;publishAction("Page navigation");}
 else {if(!gesture){if(opt.rotate&&turn>.10&&turn*f.distance>travel*1.7&&radial<160){gesture=4;appGesture(5,114,0,1);publishAction("Rotate");}else if(opt.zoom&&radial>100&&radial>travel*1.6){gesture=2;appGesture(8,113,0,1);publishAction("Pinch to zoom");}else if(travel>30&&(!opt.pages||fabs(f.y-sy)>fabs(f.x-sx))){gesture=1;vx=vy=0;scrollEvent(0,0,1,0);publishAction("Scroll");}}
 if(gesture){pendingTap=0;}if(gesture==1){double sign=opt.natural?1:-1,x=dx*opt.scroll*sign,y=dy*opt.scroll*sign;scrollEvent(x,y,2,0);double scale=.016/fmax(.004,dt);vx=.6*vx+.4*x*scale;vy=.6*vy+.4*y*scale;}if(gesture==2&&pdist>100){double z=(f.distance-pdist)/pdist;if(fabs(z)<.25)appGesture(8,113,z,2);}if(gesture==4)appGesture(5,114,-angleDiff(f.angle,pangle)*180/M_PI,2);}}
 else if(n==3&&!held&&opt.drag){if(!dragging&&hypot(f.x-sx,f.y-sy)>25){dragging=1;hadClick=1;mouse(kCGEventLeftMouseDown,location(),0);publishAction("Three-finger drag");}if(dragging){CGPoint p=location();p.x+=dx*opt.pointer;p.y+=dy*opt.pointer;mouse(kCGEventLeftMouseDragged,p,0);}}
 else if(n>=4&&!held&&!fired){double ax=f.x-sx,ay=f.y-sy,spread=f.radius-sradius;
 // Preserve the working horizontal Dock stream and direct Mission Control path.
 if(gesture!=3){const char*action=NULL;
 if(fabs(spread)>opt.pinch&&fabs(spread)>hypot(ax,ay)*.75&&((spread>0&&opt.desktop)||(spread<0&&opt.launchpad)))action=spread>0?"show-desktop":"launchpad";
 else if(ay<0&&fabs(ay)>fabs(ax)*1.3){if(opt.mission&&ay< -opt.vertical)action="mission-control";}
 else if(ay>0&&fabs(ay)>fabs(ax)*1.3){if(opt.expose&&ay>opt.vertical)action="app-expose";}
 else if(hypot(ax,ay)>65&&fabs(ax)>fabs(ay)*1.3&&opt.spaces){gesture=3;dockBegin(1);swipes++;publishAction("Desktop swipe");}
 if(action){fired=1;swipes++;if(systemAction)systemAction(action);publishAction(action);}}
 if(gesture==3)dockMove(-dx/opt.horizontal);}

 px=f.x;py=f.y;pdist=f.distance;pangle=f.angle;pradius=f.radius;oldCount=n;oldMask=f.mask;
}
const char*TCStatusJSON(void){static _Thread_local char out[2048];pthread_mutex_lock(&lock);int at=snprintf(out,sizeof(out),"{\"running\":%d,\"connected\":%d,\"frames\":%lu,\"fingers\":%d,\"action\":\"%s\",\"error\":\"%s\",\"contacts\":[",active,connected,frames,live.count,lastAction,errorText);for(int i=0;i<live.count&&i<5;i++){TCFinger*f=&live.fingers[i];at+=snprintf(out+at,sizeof(out)-at,"%s{\"id\":%d,\"x\":%.4f,\"y\":%.4f,\"pressure\":%u}",i?",":"",f->id,(f->x+3678)/7612,(f->y+2587)/5065,f->pressure);}snprintf(out+at,sizeof(out)-at,"]}");pthread_mutex_unlock(&lock);return out;}
void TCStop(void){atomic_store(&running,0);}
static void signalStop(int s){(void)s;TCStop();}
int TCStartAtPath(const char*path){if(atomic_exchange(&running,1))return 0;setbuf(stdout,NULL);if(!AXIsProcessTrusted()){publishError("Accessibility permission required");running=0;return 3;}
 signal(SIGINT,signalStop);signal(SIGTERM,signalStop);source=CGEventSourceCreate(kCGEventSourceStateHIDSystemState);if(!path||strlen(path)>=sizeof(((struct sockaddr_un*)0)->sun_path)){publishError("Invalid socket path");running=0;return 4;}
 struct stat st;if(lstat(path,&st)==0){publishError("Another receiver owns the socket");CFRelease(source);running=0;return 4;}

 int fd=socket(AF_UNIX,SOCK_DGRAM,0);struct sockaddr_un a={0};a.sun_family=AF_UNIX;strncpy(a.sun_path,path,sizeof(a.sun_path)-1);unlink(path);if(bind(fd,(struct sockaddr*)&a,sizeof(a))){publishError("Unable to open local socket");close(fd);running=0;return 5;}chmod(path,0600);struct timeval timeout={0,16000};setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));pthread_mutex_lock(&lock);active=1;opt=config;errorText[0]=0;pthread_mutex_unlock(&lock);
 while(running){ticks();uint8_t b[4096];ssize_t n=recv(fd,b,sizeof(b)-1,0);if(n<0){if(oldCount&&CFAbsoluteTimeGetCurrent()-lastFrame>.75)resetContacts();if(errno!=EAGAIN&&errno!=EWOULDBLOCK&&errno!=EINTR)break;continue;}b[n]=0;
 if(n==3&&!memcmp(b,"END",3)){resetContacts();pthread_mutex_lock(&lock);connected=0;live.count=0;pthread_mutex_unlock(&lock);publishAction("USB disconnected · shortcuts ready");continue;}
 if(n==5&&!memcmp(b,"READY",5)){pthread_mutex_lock(&lock);connected=1;pthread_mutex_unlock(&lock);publishAction("USB connected");continue;}
 if(n==10&&!memcmp(b,"SPACE_LEFT",10)){TCRequestAction("left");continue;}if(n==11&&!memcmp(b,"SPACE_RIGHT",11)){TCRequestAction("right");continue;}
 if(n>7&&!memcmp(b,"ACTION:",7)){TCRequestAction((char*)b+7);continue;}
 if(n>0)handleFrame(b,(int)n);}
 resetContacts();if(autoActive)dockEnd();close(fd);unlink(path);CFRelease(source);pthread_mutex_lock(&lock);active=connected=0;pthread_mutex_unlock(&lock);running=0;return 0;}

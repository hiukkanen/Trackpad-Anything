#include <ApplicationServices/ApplicationServices.h>
#include <assert.h>
#include <string.h>
#include <math.h>
static double clockTime=100;
static CFAbsoluteTime testNow(void){return clockTime;}
static int posts,mouseDown,mouseUp,scrollCount,zoomCount,rotateCount,smartCount,navCount,missionCount,exposeCount,desktopCount,launchCount;
static CGPoint lastPoint;
static double scrollY,scrollX;
static void capture(CGEventTapLocation tap,CGEventRef e){(void)tap;posts++;CGEventType t=CGEventGetType(e);assert(t!=kCGEventKeyDown&&t!=kCGEventKeyUp);if(t==kCGEventLeftMouseDown||t==kCGEventRightMouseDown)mouseDown++;if(t==kCGEventLeftMouseUp||t==kCGEventRightMouseUp)mouseUp++;if(t==kCGEventMouseMoved||t==kCGEventLeftMouseDragged)lastPoint=CGEventGetLocation(e);if(t==kCGEventScrollWheel){scrollCount++;scrollY+=CGEventGetDoubleValueField(e,kCGScrollWheelEventPointDeltaAxis1);scrollX+=CGEventGetDoubleValueField(e,kCGScrollWheelEventPointDeltaAxis2);}if(t==29){int st=(int)CGEventGetIntegerValueField(e,110);if(st==8)zoomCount++;if(st==5)rotateCount++;if(st==22)smartCount++;if(st==16)navCount++;}}
#define CGEventPost capture
#define CFAbsoluteTimeGetCurrent testNow
#include "../Sources/Core/GestureEngine.c"
static void action(const char*s){if(!strcmp(s,"mission-control"))missionCount++;if(!strcmp(s,"app-expose"))exposeCount++;if(!strcmp(s,"show-desktop"))desktopCount++;if(!strcmp(s,"launchpad"))launchCount++;}
static void packet(int n,int x,int y,double spacing,double angle,int button){uint8_t b[156]={2};b[1]=button;for(int i=0;i<n;i++){uint8_t*t=b+12+i*9;double off=(i-(n-1)/2.)*spacing;unsigned X=((int)lround(x+off*cos(angle)))&8191,Y=(-(int)lround(y+off*sin(angle)))&8191;t[0]=X;t[1]=((X>>8)&31)|((Y&7)<<5);t[2]=Y>>3;t[3]=0x80|((Y>>11)&3);t[4]=t[5]=20;t[8]=i;}clockTime+=.01;handleFrame(b,12+n*9);}
static void lift(void){packet(0,0,0,0,0,0);}
int main(void){source=CGEventSourceCreate(kCGEventSourceStateHIDSystemState);TCSetSystemActionCallback(action);opt=config;
 assert(!TCSetOption("pointerSpeed",NAN));assert(!TCSetOption("pointerSpeed",10));assert(!TCSetOption("unknown",1));assert(!TCSetOption("tapToClick",.3));
 // Established horizontal behavior: same threshold, gain, axis and no keyboard events.
 packet(4,0,0,100,0,0);packet(4,800,0,100,0,0);assert(dockAxis==1&&fabs(dockProgress+.5)<1e-9);lift();
 packet(4,0,0,100,0,0);packet(4,0,-800,100,0,0);packet(4,0,-1000,100,0,0);assert(missionCount==1);lift();
 packet(4,0,0,100,0,0);packet(4,0,500,100,0,0);assert(exposeCount==1);lift();
 // Pointer and scroll baseline gains preserved; changes applied next contact.
 CGPoint start=location();packet(1,0,0,0,0,0);packet(1,100,0,0,0,0);assert(fabs(lastPoint.x-start.x-30)<.01);lift();
 assert(TCSetOption("pointerSpeed",.6));start=location();packet(1,0,0,0,0,0);packet(1,100,0,0,0,0);assert(fabs(lastPoint.x-start.x-60)<.01);lift();TCSetOption("pointerSpeed",.3);
 scrollY=0;packet(2,0,0,500,0,0);packet(2,0,100,500,0,0);assert(fabs(scrollY-22)<.01);lift();
 TCSetOption("naturalScroll",0);scrollY=0;packet(2,0,0,500,0,0);packet(2,0,100,500,0,0);assert(fabs(scrollY+22)<.01);lift();TCSetOption("naturalScroll",1);
 packet(2,0,0,500,0,0);packet(2,0,0,620,0,0);lift();assert(zoomCount==3);
 TCSetOption("rotation",1);packet(2,0,0,1000,0,0);packet(2,0,0,1000,.2,0);lift();assert(rotateCount==3);
 TCSetOption("smartZoom",1);packet(2,0,0,500,0,0);lift();packet(2,0,0,500,0,0);lift();assert(smartCount==1);
 TCSetOption("pageSwipe",1);packet(2,0,0,500,0,0);packet(2,600,0,500,0,0);lift();assert(navCount>0);TCSetOption("pageSwipe",0);
 TCSetOption("showDesktop",1);packet(4,0,0,100,0,0);packet(4,0,0,400,0,0);lift();assert(desktopCount==1);
 TCSetOption("launchpad",1);packet(4,0,0,400,0,0);packet(4,0,0,100,0,0);lift();assert(launchCount==1);
 TCSetOption("spaces",0);double progress=dockProgress;packet(4,0,0,100,0,0);packet(4,800,0,100,0,0);lift();assert(dockProgress==progress);TCSetOption("spaces",1);
 TCSetOption("momentum",1);packet(2,0,0,500,0,0);packet(2,0,120,500,0,0);lift();assert(inertia);int before=scrollCount;clockTime+=.02;ticks();assert(scrollCount>before);packet(1,0,0,0,0,0);assert(!inertia);lift();TCSetOption("momentum",0);
 // Drag, disconnect/cancellation and malformed packets must not leave a held button.
 packet(3,0,0,100,0,0);packet(3,200,0,100,0,0);assert(dragging);resetContacts();assert(!dragging&&mouseDown==mouseUp);
 TCSetOption("lookup",1);assert(config.lookup&&!config.drag);TCSetOption("threeFingerDrag",1);assert(config.drag&&!config.lookup);
 TCFrame f;uint8_t bad[13]={2};assert(!TCDecode(bad,sizeof(bad),&f));
 assert(strstr(TCOptionsJSON(),"pointerSpeed"));CFRelease(source);printf("PASS: baseline gains, native Spaces/Mission Control, settings, zoom, rotation, smart zoom, pages, spread/pinch, cancellation and validation (%d captured events; none sent)\n",posts);}

#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>
#import <ApplicationServices/ApplicationServices.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <libproc.h>
#import <IOKit/IOKitLib.h>
#import <IOKit/hid/IOHIDManager.h>
#import <ServiceManagement/ServiceManagement.h>
#import "SettingsStore.h"
#import "SystemActions.h"
#import "SystemMirror.h"
#import "Localization.h"
extern CGError CoreDockSendNotification(CFStringRef,int);
static NSString*const kBTCache=@"/Library/Bluetooth/Library/Preferences/com.apple.MobileBluetooth.devices.plist";
static const int kPIDUSBC=804,kPIDLightning=613;

@interface TPHost:NSObject<NSApplicationDelegate,WKScriptMessageHandler,WKURLSchemeHandler,WKNavigationDelegate>
@property NSWindow*window;
@property WKWebView*web;
@property NSString*notice;
@property BOOL engineActive;
@property BOOL activating;
@property BOOL helperActive;
@property BOOL quitting;
@property NSStatusItem*item;
@property NSNumber*batteryPercent;
@property BOOL batteryReading;
@property NSTimer*batteryTimer;
@property NSTimer*deviceTimer;
@property NSString*lastTransport;
@property BOOL btBusy;
@property NSMenuItem*infoItem;
@property NSArray<NSMenuItem*>*menuItems;
@property CFMachPortRef hotkeyTap;
@property BOOL autoStarted; // engine auto-start happens once per launch (never after a manual revert)
@end
static TPHost*currentHost;
static void systemAction(const char*action){NSString*a=[NSString stringWithUTF8String:action];dispatch_async(dispatch_get_main_queue(),^{BOOL ok=TCSystemAction(a);currentHost.notice=ok?TCLf(@"실행: {0}",@[a]):TCLf(@"동작 실패: {0}",@[a]);});}
@implementation TPHost
-(void)applicationDidFinishLaunching:(NSNotification*)note{
 (void)note;currentHost=self;
 [[NSFileManager defaultManager]createDirectoryAtPath:[self support] withIntermediateDirectories:YES attributes:nil error:nil];
 TCLoadSettings([[self support]stringByAppendingPathComponent:@"settings-v1.plist"]);TCSetSystemActionCallback(systemAction);
 [self refreshBattery];self.batteryTimer=[NSTimer scheduledTimerWithTimeInterval:30 target:self selector:@selector(refreshBattery) userInfo:nil repeats:YES];
 // One menu bar item for the whole product: "TP 100%".
 self.item=[[NSStatusBar systemStatusBar]statusItemWithLength:NSVariableStatusItemLength];self.item.button.title=@"TP";
 NSMenu*tray=[NSMenu new];self.infoItem=[tray addItemWithTitle:TCL(@"Magic Trackpad 확인 중") action:nil keyEquivalent:@""];self.infoItem.enabled=NO;[tray addItem:NSMenuItem.separatorItem];
 NSMenuItem*show=[tray addItemWithTitle:TCL(@"Trackpad Anything 설정 열기") action:@selector(showWindow:) keyEquivalent:@","];show.target=self;
 NSMenuItem*usb=[tray addItemWithTitle:TCL(@"USB 트랙패드 다시 연결") action:@selector(launchUSBFromMenu:) keyEquivalent:@""];usb.target=self;
 NSMenuItem*restore=[tray addItemWithTitle:TCL(@"이전 앱(0.2)으로 복구") action:@selector(restore:) keyEquivalent:@""];restore.target=self;
 [tray addItem:NSMenuItem.separatorItem];NSMenuItem*quit=[tray addItemWithTitle:TCL(@"종료") action:@selector(terminate:) keyEquivalent:@"q"];self.item.menu=tray;
 self.menuItems=@[show,usb,restore,quit];
 self.notice=@"";
 self.lastTransport=[self deviceTransport]?:@"";
 self.deviceTimer=[NSTimer scheduledTimerWithTimeInterval:3 target:self selector:@selector(watchDevice) userInfo:nil repeats:YES];
 // Take over from the 0.2 app so only one menu bar item remains.
 [NSUserDefaults.standardUserDefaults registerDefaults:@{@"hotkeys":@YES}];[self updateHotkeys];
 if(AXIsProcessTrusted()){self.autoStarted=YES;dispatch_async(dispatch_get_main_queue(),^{[self activateEngine];});}
 // Ask once at launch: this registers the app in System Settings > Accessibility so the user only flips the switch.
 else{AXIsProcessTrustedWithOptions((__bridge CFDictionaryRef)@{(__bridge NSString*)kAXTrustedCheckOptionPrompt:@YES});
  if([self engineRunning])self.notice=TCL(@"이전 앱(0.2)이 실행 중입니다. 손쉬운 사용 허용 후 적용 시작을 누르면 하나로 합쳐집니다");}
 WKWebViewConfiguration*cfg=[WKWebViewConfiguration new];[cfg setURLSchemeHandler:self forURLScheme:@"trackpad"];
 [cfg.userContentController addScriptMessageHandler:self name:@"trackpad"];
 self.web=[[WKWebView alloc]initWithFrame:NSMakeRect(0,0,1200,800) configuration:cfg];self.web.navigationDelegate=self;
 self.window=[[NSWindow alloc]initWithContentRect:NSMakeRect(0,0,1200,800) styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable|NSWindowStyleMaskMiniaturizable|NSWindowStyleMaskResizable backing:NSBackingStoreBuffered defer:NO];
 self.window.title=@"Trackpad Anything";self.window.minSize=NSMakeSize(1000,700);self.window.contentView=self.web;[self.window center];[self.window makeKeyAndOrderFront:nil];
 NSMenu*menu=[NSMenu new];NSMenuItem*item=[NSMenuItem new];NSMenu*appMenu=[NSMenu new];[appMenu addItemWithTitle:TCL(@"종료") action:@selector(terminate:) keyEquivalent:@"q"];item.submenu=appMenu;[menu addItem:item];NSApp.mainMenu=menu;
 // TA_START_PAGE (optional): open on a given sidebar page, e.g. for screenshots.
 NSString*start=NSProcessInfo.processInfo.environment[@"TA_START_PAGE"];NSString*url=@"trackpad://app/index.html";if(start.length)url=[url stringByAppendingFormat:@"#%@",[start stringByAddingPercentEncodingWithAllowedCharacters:NSCharacterSet.URLFragmentAllowedCharacterSet]];
 [self.web loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:url]]];[NSApp activateIgnoringOtherApps:YES];
}
-(BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender{(void)sender;return !self.engineActive&&!self.activating;}
-(NSString*)support{return [NSHomeDirectory() stringByAppendingPathComponent:@"Library/Application Support/TrackpadCompat"];}
-(void)showWindow:(id)sender{(void)sender;[self.window makeKeyAndOrderFront:nil];[NSApp activateIgnoringOtherApps:YES];}
-(BOOL)applicationShouldHandleReopen:(NSApplication*)app hasVisibleWindows:(BOOL)flag{(void)app;(void)flag;[self showWindow:nil];return YES;}
-(NSString*)socketPath{return [[self support]stringByAppendingPathComponent:@"trackpad.sock"];}
-(void)stopNewEngine{TCStop();if(self.engineActive)unlink(self.socketPath.fileSystemRepresentation);}
-(void)applicationWillTerminate:(NSNotification*)note{(void)note;[self stopNewEngine];}
-(void)restore:(id)sender{(void)sender;[self stopNewEngine];dispatch_after(dispatch_time(DISPATCH_TIME_NOW,NSEC_PER_SEC),dispatch_get_main_queue(),^{[[NSWorkspace sharedWorkspace]openApplicationAtURL:[NSURL fileURLWithPath:[NSHomeDirectory() stringByAppendingPathComponent:@"Applications/TrackpadCompat Backup/TrackpadCompat.app"]] configuration:[NSWorkspaceOpenConfiguration configuration] completionHandler:nil];self.notice=TCL(@"기존 입력 앱을 다시 열었습니다");[self publish];});}
-(void)activateEngine{
 if(self.activating||self.engineActive)return;
 if(!AXIsProcessTrusted()){self.notice=TCL(@"손쉬운 사용에서 Trackpad Anything을 허용한 뒤 적용 시작을 눌러 주세요");AXIsProcessTrustedWithOptions((__bridge CFDictionaryRef)@{(__bridge NSString*)kAXTrustedCheckOptionPrompt:@YES});[[NSWorkspace sharedWorkspace]openURL:[NSURL URLWithString:@"x-apple.systempreferences:com.apple.preference.security?Privacy_Accessibility"]];[self publish];return;}
 self.activating=YES;self.notice=TCL(@"입력 엔진 전환 중");
 for(NSRunningApplication*a in [NSRunningApplication runningApplicationsWithBundleIdentifier:@"local.souluk.TrackpadCompat"])[a terminate];
 dispatch_after(dispatch_time(DISPATCH_TIME_NOW,NSEC_PER_SEC),dispatch_get_main_queue(),^{
 if([self engineRunning]){self.activating=NO;self.notice=TCL(@"기존 앱이 종료되지 않아 전환을 중단했습니다");[self publish];return;}
 unlink(self.socketPath.fileSystemRepresentation);
 dispatch_after(dispatch_time(DISPATCH_TIME_NOW,NSEC_PER_SEC),dispatch_get_main_queue(),^{[self startReceiver];});});
}
-(void)startReceiver{
 if([self usbRunning]){self.activating=NO;self.notice=TCL(@"기존 USB 연결이 종료되지 않았습니다. 다시 적용하거나 기존 앱으로 복구하세요");[self publish];return;}
 NSString*path=self.socketPath;dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED,0),^{int result=TCStartAtPath(path.fileSystemRepresentation);dispatch_async(dispatch_get_main_queue(),^{self.engineActive=NO;self.activating=NO;self.notice=TCLf(@"입력 엔진 종료 ({0})",@[@(result)]);[self publish];});});
 dispatch_after(dispatch_time(DISPATCH_TIME_NOW,300*NSEC_PER_MSEC),dispatch_get_main_queue(),^{
 NSDictionary*d=[NSJSONSerialization JSONObjectWithData:[[NSString stringWithUTF8String:TCStatusJSON()] dataUsingEncoding:NSUTF8StringEncoding] options:0 error:nil];
 if(![d[@"running"]boolValue]){self.activating=NO;self.notice=d[@"error"]?:TCL(@"입력 엔진을 시작하지 못했습니다");[self publish];return;}
 self.engineActive=YES;self.activating=NO;
 if([[self deviceTransport] isEqualToString:@"USB"])[self launchUSB];
 else self.notice=[self btNative]?TCL(@"블루투스: macOS 기본 드라이버로 동작 중입니다"):TCL(@"입력 엔진 준비 완료 · USB 연결 시 자동으로 연결합니다");
 [self publish];});
}
// Local desktop switch through the engine's native Dock swipe (works inside full-screen remote apps).
-(BOOL)sendSpace:(BOOL)left{
 const char*path=self.socketPath.fileSystemRepresentation;struct sockaddr_un a={0};a.sun_family=AF_UNIX;if(strlen(path)>=sizeof(a.sun_path))return NO;
 strcpy(a.sun_path,path);const char*msg=left?"SPACE_LEFT":"SPACE_RIGHT";int fd=socket(AF_UNIX,SOCK_DGRAM,0);if(fd<0)return NO;
 ssize_t sent=sendto(fd,msg,strlen(msg),0,(struct sockaddr*)&a,sizeof(a));close(fd);return sent==(ssize_t)strlen(msg);
}
// Built-in keyboard shortcut: Control+Option+Command+Left/Right. Needs Accessibility; no Karabiner required.
static CGEventRef TCHotkeyCallback(CGEventTapProxy proxy,CGEventType type,CGEventRef event,void*info){
 (void)proxy;(void)info;
 if(type==kCGEventTapDisabledByTimeout||type==kCGEventTapDisabledByUserInput){if(currentHost.hotkeyTap)CGEventTapEnable(currentHost.hotkeyTap,true);return event;}
 if(type!=kCGEventKeyDown&&type!=kCGEventKeyUp)return event;
 int64_t key=CGEventGetIntegerValueField(event,kCGKeyboardEventKeycode);
 CGEventFlags mods=CGEventGetFlags(event)&(kCGEventFlagMaskControl|kCGEventFlagMaskAlternate|kCGEventFlagMaskCommand|kCGEventFlagMaskShift);
 if((key!=123&&key!=124)||mods!=(kCGEventFlagMaskControl|kCGEventFlagMaskAlternate|kCGEventFlagMaskCommand))return event;
 if(type==kCGEventKeyDown&&!CGEventGetIntegerValueField(event,kCGKeyboardEventAutorepeat)){BOOL left=key==123;dispatch_async(dispatch_get_main_queue(),^{[currentHost sendSpace:left];});}
 return NULL; // consume, so the remote app doesn't also get the keys
}
-(BOOL)hotkeysEnabled{return [NSUserDefaults.standardUserDefaults boolForKey:@"hotkeys"];}
-(void)updateHotkeys{
 BOOL want=[self hotkeysEnabled]&&AXIsProcessTrusted();
 if(want&&!self.hotkeyTap){
  CGEventMask mask=CGEventMaskBit(kCGEventKeyDown)|CGEventMaskBit(kCGEventKeyUp);
  CFMachPortRef tap=CGEventTapCreate(kCGHIDEventTap,kCGHeadInsertEventTap,kCGEventTapOptionDefault,mask,TCHotkeyCallback,NULL);
  if(!tap)tap=CGEventTapCreate(kCGSessionEventTap,kCGHeadInsertEventTap,kCGEventTapOptionDefault,mask,TCHotkeyCallback,NULL);
  if(!tap)return;
  self.hotkeyTap=tap;CFRunLoopSourceRef src=CFMachPortCreateRunLoopSource(NULL,tap,0);CFRunLoopAddSource(CFRunLoopGetMain(),src,kCFRunLoopCommonModes);CFRelease(src);CGEventTapEnable(tap,true);
 }else if(!want&&self.hotkeyTap){CGEventTapEnable(self.hotkeyTap,false);CFMachPortInvalidate(self.hotkeyTap);CFRelease(self.hotkeyTap);self.hotkeyTap=NULL;}
}
-(void)launchUSBFromMenu:(id)sender{(void)sender;if(!self.engineActive)[self activateEngine];else [self launchUSB];}
-(void)watchDevice{
 [self updateHotkeys];
 if(!self.autoStarted&&AXIsProcessTrusted()&&!self.engineActive&&!self.activating){self.autoStarted=YES;[self activateEngine];}
 NSString*t=[self deviceTransport]?:@"";
 if(![t isEqualToString:self.lastTransport]){
  NSString*previous=self.lastTransport;self.lastTransport=t;self.batteryPercent=nil;[self refreshBattery];
  if([t isEqualToString:@"USB"]&&self.engineActive&&!self.helperActive)[self launchUSB];
  if([t isEqualToString:@"Bluetooth"]&&[self btNative]&&[self mirrorEnabled])[self applyMirror];
  if([previous isEqualToString:@"Bluetooth"]&&[t isEqualToString:@"USB"])self.notice=TCL(@"USB로 연결됐습니다. 케이블 연결로 재페어링되면 블루투스 제스처를 다시 켜야 할 수 있습니다");
 }
 [self publish];
}
-(void)launchUSB{
 if(self.helperActive||!self.engineActive)return;self.helperActive=YES;
 NSString*helper=[NSBundle.mainBundle.resourcePath stringByAppendingPathComponent:@"usb-helper"];
 NSString*log=[[self support]stringByAppendingPathComponent:@"helper-gui.log"];
 // Quote both shell and AppleScript layers; all paths are locally derived.
 NSString*(^quote)(NSString*)=^NSString*(NSString*x){return [NSString stringWithFormat:@"'%@'",[x stringByReplacingOccurrencesOfString:@"'" withString:@"'\"'\"'"]];};
 NSString*cmd=[NSString stringWithFormat:@"%@ %@ %u > %@ 2>&1",quote(helper),quote(self.socketPath),getuid(),quote(log)];
 NSString*escaped=[[cmd stringByReplacingOccurrencesOfString:@"\\" withString:@"\\\\"]stringByReplacingOccurrencesOfString:@"\"" withString:@"\\\""];
 dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED,0),^{NSTask*t=[NSTask new];t.executableURL=[NSURL fileURLWithPath:@"/usr/bin/osascript"];t.arguments=@[@"-e",[NSString stringWithFormat:@"do shell script \"%@\" with administrator privileges",escaped]];NSError*error=nil;[t launchAndReturnError:&error];if(!error)[t waitUntilExit];dispatch_async(dispatch_get_main_queue(),^{self.helperActive=NO;self.notice=TCL(@"USB 연결이 종료되었습니다. USB 다시 연결 또는 기존 앱 복구를 선택하세요");[self publish];});});
 self.notice=TCL(@"USB 연결을 위한 관리자 인증을 완료해 주세요");
}
-(BOOL)engineRunning{return [NSRunningApplication runningApplicationsWithBundleIdentifier:@"local.souluk.TrackpadCompat"].count>0;}
-(BOOL)usbRunning{
 NSString*log=[NSString stringWithContentsOfFile:[[self support]stringByAppendingPathComponent:@"helper-app.log"] encoding:NSUTF8StringEncoding error:nil];
 if(!log||[log containsString:@"Restore system driver="])return NO;
 NSScanner*s=[NSScanner scannerWithString:log];[s scanString:@"USB helper pid=" intoString:nil];int pid=0;if(![s scanInt:&pid])return NO;char path[PROC_PIDPATHINFO_MAXSIZE]={0};if(proc_pidpath(pid,path,sizeof(path))<=0)return NO;
 return [[NSString stringWithUTF8String:path]hasSuffix:@"TrackpadCompat.app/Contents/Resources/usb-helper"];
}
// USB interface 0, report 0x90: status bits followed by Battery System RemainingCapacity.
// Descriptor on 05ac:0324 declares 8-bit percentage. No exclusive capture or output report.
-(void)refreshBattery{
 if(self.batteryReading)return;self.batteryReading=YES;
 dispatch_async(dispatch_get_global_queue(QOS_CLASS_UTILITY,0),^{
 // Bluetooth on macOS's own driver: the driver publishes BatteryPercent in the registry.
 NSNumber*percent=[self registryBluetoothBattery];if(percent){dispatch_async(dispatch_get_main_queue(),^{self.batteryPercent=percent;self.batteryReading=NO;[self publish];});return;}
 IOHIDManagerRef manager=IOHIDManagerCreate(kCFAllocatorDefault,0);
 IOHIDManagerSetDeviceMatchingMultiple(manager,(__bridge CFArrayRef)@[@{@kIOHIDVendorIDKey:@1452,@kIOHIDProductIDKey:@804,@kIOHIDPrimaryUsagePageKey:@65280,@kIOHIDPrimaryUsageKey:@11},@{@kIOHIDVendorIDKey:@76,@kIOHIDProductIDKey:@804,@kIOHIDTransportKey:@"Bluetooth"}]);
 CFSetRef devices=IOHIDManagerCopyDevices(manager);
 for(id object in (__bridge NSSet*)devices){IOHIDDeviceRef device=(__bridge IOHIDDeviceRef)object;if(IOHIDDeviceOpen(device,0)!=kIOReturnSuccess)continue;
 uint8_t report[32]={0x90};CFIndex length=sizeof(report);IOReturn result=IOHIDDeviceGetReport(device,kIOHIDReportTypeInput,0x90,report,&length);
 if(result==kIOReturnSuccess&&length==3&&report[0]==0x90&&report[2]<=100)percent=@(report[2]);
 IOHIDDeviceClose(device,0);if(percent)break;}
 if(devices)CFRelease(devices);CFRelease(manager);
 dispatch_async(dispatch_get_main_queue(),^{self.batteryPercent=percent;self.batteryReading=NO;[self publish];});});
}
-(NSString*)deviceTransport{
 io_iterator_t iterator=0;BOOL found=NO;
 if(IOServiceGetMatchingServices(kIOMainPortDefault,IOServiceMatching("IOUSBHostDevice"),&iterator)!=KERN_SUCCESS)return nil;
 io_service_t service;while((service=IOIteratorNext(iterator))){CFTypeRef vendor=IORegistryEntryCreateCFProperty(service,CFSTR("idVendor"),kCFAllocatorDefault,0),product=IORegistryEntryCreateCFProperty(service,CFSTR("idProduct"),kCFAllocatorDefault,0);
 if(vendor&&product&&CFGetTypeID(vendor)==CFNumberGetTypeID()&&CFGetTypeID(product)==CFNumberGetTypeID()){int v=0,p=0;CFNumberGetValue((CFNumberRef)vendor,kCFNumberIntType,&v);CFNumberGetValue((CFNumberRef)product,kCFNumberIntType,&p);found=v==0x5ac&&p==0x324;}
 if(vendor)CFRelease(vendor);if(product)CFRelease(product);IOObjectRelease(service);if(found)break;}
 IOObjectRelease(iterator);if(found)return @"USB";
 return [self bluetoothProductID]?@"Bluetooth":nil;
}
// 804 = USB-C model as itself (gestures off on macOS 14), 613 = presented as Lightning model (native driver).
-(int)bluetoothProductID{
 int found=0;for(NSNumber*pid in @[@(kPIDLightning),@(kPIDUSBC)]){
  IOHIDManagerRef m=IOHIDManagerCreate(kCFAllocatorDefault,0);IOHIDManagerSetDeviceMatching(m,(__bridge CFDictionaryRef)@{@kIOHIDVendorIDKey:@76,@kIOHIDProductIDKey:pid,@kIOHIDTransportKey:@"Bluetooth"});
  CFSetRef devices=IOHIDManagerCopyDevices(m);BOOL any=devices&&CFSetGetCount(devices)>0;if(devices)CFRelease(devices);CFRelease(m);if(any){found=pid.intValue;break;}}
 return found;
}
-(BOOL)btNative{return [self bluetoothProductID]==kPIDLightning;}
-(NSNumber*)registryBluetoothBattery{
 io_iterator_t it=0;NSNumber*result=nil;
 if(IOServiceGetMatchingServices(kIOMainPortDefault,IOServiceMatching("AppleDeviceManagementHIDEventService"),&it)!=KERN_SUCCESS)return nil;
 io_service_t s;while(!result&&(s=IOIteratorNext(it))){
  NSString*t=CFBridgingRelease(IORegistryEntryCreateCFProperty(s,CFSTR("Transport"),kCFAllocatorDefault,0));NSNumber*v=CFBridgingRelease(IORegistryEntryCreateCFProperty(s,CFSTR("VendorID"),kCFAllocatorDefault,0)),*p=CFBridgingRelease(IORegistryEntryCreateCFProperty(s,CFSTR("ProductID"),kCFAllocatorDefault,0)),*b=CFBridgingRelease(IORegistryEntryCreateCFProperty(s,CFSTR("BatteryPercent"),kCFAllocatorDefault,0));
  if([t isKindOfClass:NSString.class]&&[t isEqualToString:@"Bluetooth"]&&[v isKindOfClass:NSNumber.class]&&v.intValue==76&&[p isKindOfClass:NSNumber.class]&&(p.intValue==kPIDLightning||p.intValue==kPIDUSBC)&&[b isKindOfClass:NSNumber.class]&&b.intValue>=0&&b.intValue<=100)result=b;
  IOObjectRelease(s);}
 IOObjectRelease(it);return result;
}
// Menu bar mark: logo mascot + battery outline with the percentage inside. Template image follows light/dark menu bar.
-(NSImage*)statusImage:(NSNumber*)pct{
 NSImage*mascot=[NSImage imageNamed:@"MenuBarMascot"];NSSize ts=mascot?mascot.size:NSMakeSize(22,17);const CGFloat h=18,gap=4,bw=pct?28:0;
 NSImage*img=[NSImage imageWithSize:NSMakeSize(ceil(ts.width)+(pct?gap+bw:0),h) flipped:NO drawingHandler:^BOOL(NSRect r){(void)r;
  if(mascot)[mascot drawInRect:NSMakeRect(0,round((h-ts.height)/2),ts.width,ts.height)];
  else [@"TA" drawAtPoint:NSMakePoint(0,2) withAttributes:@{NSFontAttributeName:[NSFont systemFontOfSize:13 weight:NSFontWeightSemibold]}];
  if(pct){
   [NSColor.blackColor set];
   NSRect body=NSMakeRect(ceil(ts.width)+gap+.5,3.5,bw-4,h-7);
   NSBezierPath*outline=[NSBezierPath bezierPathWithRoundedRect:body xRadius:3 yRadius:3];outline.lineWidth=1;[outline stroke];
   [[NSBezierPath bezierPathWithRoundedRect:NSMakeRect(NSMaxX(body)+1,h/2-2,1.5,4) xRadius:.75 yRadius:.75]fill];
   NSDictionary*na=@{NSFontAttributeName:[NSFont monospacedDigitSystemFontOfSize:(pct.intValue>=100?8.5:9.5) weight:NSFontWeightBold]};
   NSString*n=pct.stringValue;NSSize ns=[n sizeWithAttributes:na];
   [n drawAtPoint:NSMakePoint(NSMidX(body)-ns.width/2,NSMidY(body)-ns.height/2) withAttributes:na];
  }
  return YES;}];
 img.template=YES;img.accessibilityDescription=pct?TCLf(@"Trackpad Anything, 배터리 {0}%",@[pct]):@"Trackpad Anything";
 return img;
}
-(void)updateStatusItem{
 NSString*t=self.lastTransport;BOOL connected=t.length>0;
 NSNumber*pct=connected?self.batteryPercent:nil;
 self.item.button.title=@"";self.item.button.image=[self statusImage:pct];
 self.item.button.toolTip=pct?TCLf(@"Trackpad Anything · 배터리 {0}%",@[pct]):@"Trackpad Anything";
 NSString*mode=[t isEqualToString:@"Bluetooth"]?([self btNative]?TCL(@"Bluetooth · macOS 제스처"):TCL(@"Bluetooth · 제스처 꺼짐")):t;
 NSArray*titles=@[TCL(@"Trackpad Anything 설정 열기"),TCL(@"USB 트랙패드 다시 연결"),TCL(@"이전 앱(0.2)으로 복구"),TCL(@"종료")];
 for(NSUInteger i=0;i<self.menuItems.count;i++)self.menuItems[i].title=titles[i];
 self.infoItem.title=connected?[NSString stringWithFormat:@"Magic Trackpad · %@%@",mode,self.batteryPercent?[NSString stringWithFormat:@" · %@%%",self.batteryPercent]:@""]:TCL(@"Magic Trackpad 연결되지 않음");
}
// ---- Bluetooth setup (root helper, one administrator prompt) ----
-(NSString*)mirrorBackupPath{return [[self support]stringByAppendingPathComponent:@"macos-trackpad-backup.plist"];}
-(BOOL)mirrorEnabled{return [NSUserDefaults.standardUserDefaults boolForKey:@"mirrorSystem"];}
-(void)applyMirror{
 if(!TCMirrorSnapshot([self mirrorBackupPath])){self.notice=TCL(@"macOS 설정 백업에 실패해 미러를 적용하지 않았습니다");return;}
 int n=TCMirrorApply(TCReadOptions());self.notice=n?TCLf(@"macOS 트랙패드 설정 {0}개 항목에 반영했습니다",@[@(n)]):TCL(@"macOS 트랙패드 설정이 이미 같습니다");
}
-(void)runBluetoothSetup:(NSString*)mode{
 if(self.btBusy)return;self.btBusy=YES;self.notice=TCL(@"관리자 인증을 완료해 주세요 · 블루투스 기기가 몇 초간 끊깁니다");[self publish];
 NSString*tool=[NSBundle.mainBundle.resourcePath stringByAppendingPathComponent:@"bt-spoof"];
 NSString*cmd=[NSString stringWithFormat:@"'%@' %@ 2>&1; echo EXIT=$?",[tool stringByReplacingOccurrencesOfString:@"'" withString:@"'\"'\"'"],mode];
 NSString*escaped=[[cmd stringByReplacingOccurrencesOfString:@"\\" withString:@"\\\\"]stringByReplacingOccurrencesOfString:@"\"" withString:@"\\\""];
 dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED,0),^{
  NSTask*t=[NSTask new];t.executableURL=[NSURL fileURLWithPath:@"/usr/bin/osascript"];t.arguments=@[@"-e",[NSString stringWithFormat:@"do shell script \"%@\" with administrator privileges",escaped]];
  NSPipe*pipe=[NSPipe pipe];t.standardOutput=pipe;t.standardError=pipe;NSError*error=nil;[t launchAndReturnError:&error];
  NSString*out=@"";if(!error){NSData*d=[pipe.fileHandleForReading readDataToEndOfFile];[t waitUntilExit];out=[[NSString alloc]initWithData:d encoding:NSUTF8StringEncoding]?:@"";}
  dispatch_async(dispatch_get_main_queue(),^{
   self.btBusy=NO;BOOL apply=[mode isEqualToString:@"apply"];
   if([out containsString:@"RESULT applied"])self.notice=TCL(@"블루투스 제스처를 켰습니다. 트랙패드가 다시 연결되면 macOS 기본 제스처가 동작합니다");
   else if([out containsString:@"RESULT restored"])self.notice=TCL(@"원래 상태로 되돌렸습니다. 블루투스에서는 기본 마우스로만 동작합니다");
   else if([out containsString:@"state: already enabled"])self.notice=TCL(@"이미 블루투스 제스처가 켜져 있습니다");
   else if([out containsString:@"state: already original"])self.notice=TCL(@"이미 원래 상태입니다");
   else if([out containsString:@"device:"])self.notice=TCL(@"블루투스 기록에서 트랙패드를 찾지 못했습니다. 블루투스로 한 번 연결한 뒤 다시 시도하세요");
   else if([out containsString:@"layout:"])self.notice=TCL(@"이 macOS의 블루투스 기록 형식이 예상과 달라 아무것도 바꾸지 않았습니다");
   else if([out containsString:@"verify:"])self.notice=TCL(@"macOS가 기록을 되돌렸습니다. 트랙패드 전원을 껐다 켠 뒤 다시 시도하세요");
   else if([out containsString:@"-128"]||!out.length)self.notice=TCL(@"관리자 인증이 취소되었습니다");
   else self.notice=apply?TCL(@"블루투스 제스처 켜기에 실패했습니다"):TCL(@"되돌리기에 실패했습니다");
   if(apply&&[out containsString:@"RESULT applied"]&&[self mirrorEnabled])dispatch_after(dispatch_time(DISPATCH_TIME_NOW,4*NSEC_PER_SEC),dispatch_get_main_queue(),^{if([self btNative]){[self applyMirror];[self publish];}});
   [self publish];});
 });
}
-(void)publish{
 NSDictionary*engine=[NSJSONSerialization JSONObjectWithData:[[NSString stringWithUTF8String:TCStatusJSON()] dataUsingEncoding:NSUTF8StringEncoding] options:0 error:nil];
 NSString*transport=[self deviceTransport];BOOL physicalConnected=transport!=nil;if(!physicalConnected)self.batteryPercent=nil;
 self.lastTransport=transport?:@"";[self updateStatusItem];
 int btPID=[self bluetoothProductID];
 NSDictionary*bluetooth=@{@"productID":@(btPID),@"native":@(btPID==kPIDLightning),@"busy":@(self.btBusy),@"mirror":@([self mirrorEnabled]),@"mirrorBackup":[NSFileManager.defaultManager fileExistsAtPath:[self mirrorBackupPath]]?[self mirrorBackupPath]:@"",@"cacheBackup":[kBTCache stringByAppendingString:@".tc-before-pid-spoof"],@"cache":kBTCache};
 NSString*version=NSBundle.mainBundle.infoDictionary[@"CFBundleShortVersionString"]?:@"1.0.0";
 NSDictionary*d=@{@"lang":TCLanguage(),@"languagePref":TCLanguagePref(),@"transport":transport?:@"",@"bluetooth":bluetooth,@"version":version,@"batteryPercent":self.batteryPercent?:NSNull.null,@"deviceConnected":@(physicalConnected),@"running":@(self.engineActive||[self engineRunning]),@"usb":@(self.engineActive?[engine[@"connected"]boolValue]:[self usbRunning]),@"native":@YES,@"message":(self.engineActive&&[engine[@"connected"]boolValue]&&[self.notice isEqualToString:TCL(@"USB 연결을 위한 관리자 인증을 완료해 주세요")])?TCL(@"연결됨 · 설정이 입력에 적용됩니다"):self.notice?:@"",@"settings":TCReadOptions(),@"hotkeys":@([self hotkeysEnabled]),@"hotkeysActive":@(self.hotkeyTap!=NULL),@"loginItem":@(SMAppService.mainAppService.status==SMAppServiceStatusEnabled),@"engineActive":@(self.engineActive),@"activating":@(self.activating),@"trusted":@(AXIsProcessTrusted()),@"engine":engine?:@{}};NSData*j=[NSJSONSerialization dataWithJSONObject:d options:0 error:nil];NSString*json=[[NSString alloc]initWithData:j encoding:NSUTF8StringEncoding];[self.web evaluateJavaScript:[NSString stringWithFormat:@"window.tcReceive && window.tcReceive(%@)",json] completionHandler:nil];
}
-(void)userContentController:(WKUserContentController*)controller didReceiveScriptMessage:(WKScriptMessage*)message{
 (void)controller;if(!message.frameInfo.isMainFrame||![message.frameInfo.request.URL.scheme isEqualToString:@"trackpad"]||![message.body isKindOfClass:NSDictionary.class])return;
 NSString*c=message.body[@"command"];if(![c isKindOfClass:NSString.class])return;
 if([c isEqualToString:@"reset-options"]){
#define TC_OPTION(key,field,def,min,max) TCSetOption(#key,def);
#include "../Core/Options.def"
#undef TC_OPTION
 self.notice=TCSaveSettings([[self support]stringByAppendingPathComponent:@"settings-v1.plist"])?TCL(@"기존 사용감의 기본값으로 복원했습니다"):TCL(@"기본값 저장에 실패했습니다");if([self mirrorEnabled]&&[self btNative])[self applyMirror];[self publish];return;}
 if([c isEqualToString:@"set-option"]){NSString*k=message.body[@"key"];id v=message.body[@"value"];NSDictionary*before=TCReadOptions();if([k isKindOfClass:NSString.class]&&[v isKindOfClass:NSNumber.class]&&TCSetOption(k.UTF8String,[v doubleValue])){if(TCSaveSettings([[self support]stringByAppendingPathComponent:@"settings-v1.plist"])){self.notice=self.engineActive?TCL(@"설정을 저장했습니다. 다음 손가락 동작부터 적용됩니다"):TCL(@"설정을 저장했습니다. 적용 시작 후 입력에 반영됩니다");if([self mirrorEnabled]&&[self btNative])[self applyMirror];}else{for(NSString*key in before)TCSetOption(key.UTF8String,[before[key]doubleValue]);self.notice=TCL(@"설정 저장에 실패해 이전 값으로 되돌렸습니다");}}else self.notice=TCL(@"허용되지 않은 설정값입니다");[self publish];return;}
 if([c isEqualToString:@"status"]){[self publish];return;}
 if([c isEqualToString:@"set-language"]){NSString*v=message.body[@"value"];if([@[@"system",@"ko",@"en",@"ja",@"zh"] containsObject:v]){[NSUserDefaults.standardUserDefaults setObject:v forKey:@"language"];self.notice=@"";}[self publish];return;}
 if([c isEqualToString:@"bt-apply"]){[self runBluetoothSetup:@"apply"];return;}
 if([c isEqualToString:@"bt-restore"]){[self runBluetoothSetup:@"restore"];return;}
 if([c isEqualToString:@"set-mirror"]){BOOL on=[message.body[@"value"] isKindOfClass:NSNumber.class]&&[message.body[@"value"] boolValue];[NSUserDefaults.standardUserDefaults setBool:on forKey:@"mirrorSystem"];
  if(on&&[self btNative])[self applyMirror];else self.notice=on?TCL(@"설정 미러를 켰습니다. 블루투스 제스처가 동작할 때 macOS 설정에 반영됩니다"):TCL(@"설정 미러를 껐습니다. macOS 설정은 지금 상태로 유지됩니다");[self publish];return;}
 if([c isEqualToString:@"mirror-restore"]){[NSUserDefaults.standardUserDefaults setBool:NO forKey:@"mirrorSystem"];self.notice=TCMirrorRestore([self mirrorBackupPath])?TCL(@"macOS 트랙패드 설정을 미러 사용 전 값으로 되돌렸습니다"):TCL(@"되돌릴 macOS 설정 백업이 없습니다");[self publish];return;}
 if([c isEqualToString:@"reveal"]){NSString*p=message.body[@"path"];if([p isKindOfClass:NSString.class]&&([p isEqualToString:[self mirrorBackupPath]]||[p hasPrefix:@"/Library/Bluetooth/"]))[[NSWorkspace sharedWorkspace]selectFile:nil inFileViewerRootedAtPath:[p stringByDeletingLastPathComponent]];return;}
 if([c isEqualToString:@"set-login"]){BOOL on=[message.body[@"value"] isKindOfClass:NSNumber.class]&&[message.body[@"value"] boolValue];NSError*e=nil;
  BOOL ok=on?[SMAppService.mainAppService registerAndReturnError:&e]:[SMAppService.mainAppService unregisterAndReturnError:&e];
  self.notice=ok?(on?TCL(@"로그인할 때 Trackpad Anything을 자동으로 실행합니다"):TCL(@"로그인 시 자동 실행을 껐습니다")):(SMAppService.mainAppService.status==SMAppServiceStatusRequiresApproval?TCL(@"시스템 설정 > 일반 > 로그인 항목에서 Trackpad Anything을 허용해 주세요"):TCL(@"로그인 항목 설정에 실패했습니다"));[self publish];return;}
 if([c isEqualToString:@"activate"]){[self activateEngine];return;}
 if([c isEqualToString:@"restore"]){[self restore:nil];return;}
 if([c isEqualToString:@"reconnect"]){[self launchUSB];return;}
 if([c isEqualToString:@"set-hotkeys"]){BOOL on=[message.body[@"value"] isKindOfClass:NSNumber.class]&&[message.body[@"value"] boolValue];[NSUserDefaults.standardUserDefaults setBool:on forKey:@"hotkeys"];[self updateHotkeys];self.notice=on?TCL(@"키보드 단축키를 켰습니다"):TCL(@"키보드 단축키를 껐습니다");[self publish];return;}
 if([c isEqualToString:@"left"]||[c isEqualToString:@"right"]){self.notice=[self sendSpace:[c isEqualToString:@"left"]]?TCL(@"화면 전환 명령을 기존 앱에 전달했습니다"):TCL(@"입력 앱에 연결하지 못했습니다");
 }else if([c isEqualToString:@"mission"]){CGError e=CoreDockSendNotification(CFSTR("com.apple.expose.awake"),0);self.notice=e==0?TCL(@"Mission Control 실행 명령을 보냈습니다"):TCL(@"Mission Control 실행에 실패했습니다");}
 [self publish];
}
-(void)webView:(WKWebView*)web startURLSchemeTask:(id<WKURLSchemeTask>)task{
 (void)web;NSURL*u=task.request.URL;NSString*relative=[u.path stringByRemovingPercentEncoding];if(![u.host isEqualToString:@"app"]||[relative.pathComponents containsObject:@".."]){[task didFailWithError:[NSError errorWithDomain:NSURLErrorDomain code:NSURLErrorNoPermissionsToReadFile userInfo:nil]];return;}
 NSString*root=[NSBundle.mainBundle.resourcePath stringByAppendingPathComponent:@"GUI"];NSString*p=[[root stringByAppendingPathComponent:relative]stringByStandardizingPath];if(![p hasPrefix:[root stringByAppendingString:@"/"]]){[task didFailWithError:[NSError errorWithDomain:NSURLErrorDomain code:NSURLErrorNoPermissionsToReadFile userInfo:nil]];return;}
 NSData*data=[NSData dataWithContentsOfFile:p];if(!data){[task didFailWithError:[NSError errorWithDomain:NSURLErrorDomain code:NSURLErrorFileDoesNotExist userInfo:nil]];return;}
 NSDictionary*mimes=@{@"html":@"text/html",@"js":@"application/javascript",@"css":@"text/css",@"woff2":@"font/woff2",@"svg":@"image/svg+xml",@"png":@"image/png"};NSURLResponse*r=[[NSURLResponse alloc]initWithURL:u MIMEType:mimes[p.pathExtension]?:@"application/octet-stream" expectedContentLength:data.length textEncodingName:@"UTF-8"];[task didReceiveResponse:r];[task didReceiveData:data];[task didFinish];
}
-(void)webView:(WKWebView*)web stopURLSchemeTask:(id<WKURLSchemeTask>)task{(void)web;(void)task;}
-(void)webView:(WKWebView*)web decidePolicyForNavigationAction:(WKNavigationAction*)action decisionHandler:(void(^)(WKNavigationActionPolicy))handler{(void)web;handler([action.request.URL.scheme isEqualToString:@"trackpad"]?WKNavigationActionPolicyAllow:WKNavigationActionPolicyCancel);}
@end
int main(){@autoreleasepool{NSApplication*app=NSApplication.sharedApplication;[app setActivationPolicy:NSApplicationActivationPolicyRegular];static TPHost*host;host=[TPHost new];app.delegate=host;[app run];}}

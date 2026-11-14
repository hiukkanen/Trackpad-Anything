// Settings mirror: while the trackpad runs on macOS's own Bluetooth driver, TrackpadCompat
// writes its options into the standard macOS trackpad preferences. These preferences are shared
// by every trackpad on this Mac (including the built-in one); the first write snapshots the
// previous values so they can be restored exactly.
#import <Foundation/Foundation.h>

#define TC_MT @"com.apple.AppleMultitouchTrackpad"
#define TC_BT @"com.apple.driver.AppleBluetoothMultitouch.trackpad"
#define TC_DOCK @"com.apple.dock"
#define TC_GLOBAL @"kCFPreferencesAnyApplication"

typedef struct{NSString*domain;BOOL currentHost;NSString*key;} TCPref;

static CFStringRef TCApp(NSString*d){return [d isEqualToString:TC_GLOBAL]?kCFPreferencesAnyApplication:(__bridge CFStringRef)d;}
static id TCPrefGet(TCPref p){return CFBridgingRelease(CFPreferencesCopyValue((__bridge CFStringRef)p.key,TCApp(p.domain),kCFPreferencesCurrentUser,p.currentHost?kCFPreferencesCurrentHost:kCFPreferencesAnyHost));}
static void TCPrefSet(TCPref p,id value){CFPreferencesSetValue((__bridge CFStringRef)p.key,(__bridge CFPropertyListRef)value,TCApp(p.domain),kCFPreferencesCurrentUser,p.currentHost?kCFPreferencesCurrentHost:kCFPreferencesAnyHost);}
static NSString*TCPrefID(TCPref p){return [NSString stringWithFormat:@"%@|%d|%@",p.domain,p.currentHost,p.key];}

// Both trackpad domains carry the same keys; System Settings writes both.
static NSArray<NSDictionary*>*TCMirrorValues(NSDictionary*o){
 int(^b)(NSString*)=^int(NSString*k){return [o[k] doubleValue]>=.5;};
 NSMutableArray*rows=[NSMutableArray array];
 void(^pad)(NSString*,id)=^(NSString*key,id value){for(NSString*d in @[TC_MT,TC_BT])[rows addObject:@{@"d":d,@"h":@NO,@"k":key,@"v":value}];};
 void(^global)(NSString*,id,BOOL)=^(NSString*key,id value,BOOL host){[rows addObject:@{@"d":TC_GLOBAL,@"h":@(host),@"k":key,@"v":value}];};
 void(^dock)(NSString*,BOOL)=^(NSString*key,BOOL on){[rows addObject:@{@"d":TC_DOCK,@"h":@NO,@"k":key,@"v":@(on)}];};
 pad(@"Clicking",@(b(@"tapToClick")));global(@"com.apple.mouse.tapBehavior",@(b(@"tapToClick")),YES);
 pad(@"TrackpadRightClick",@(b(@"secondaryClick")));
 pad(@"TrackpadThreeFingerDrag",@(b(@"threeFingerDrag")));
 pad(@"TrackpadThreeFingerTapGesture",@(b(@"lookup")&&!b(@"threeFingerDrag")?2:0));
 pad(@"TrackpadPinch",@(b(@"pinchZoom")));pad(@"TrackpadRotate",@(b(@"rotation")));
 pad(@"TrackpadTwoFingerDoubleTapGesture",@(b(@"smartZoom")));pad(@"TrackpadMomentumScroll",@(b(@"momentum")));
 pad(@"TrackpadFourFingerHorizSwipeGesture",@(b(@"spaces")?2:0));
 pad(@"TrackpadFourFingerVertSwipeGesture",@(b(@"missionControl")||b(@"appExpose")?2:0));
 pad(@"TrackpadFourFingerPinchGesture",@(b(@"launchpad")?2:0));pad(@"TrackpadFiveFingerPinchGesture",@(b(@"showDesktop")?2:0));
 pad(@"TrackpadTwoFingerFromRightEdgeSwipeGesture",@(b(@"notificationCenter")?3:0));
 global(@"com.apple.swipescrolldirection",@(b(@"naturalScroll")),NO);
 global(@"AppleEnableSwipeNavigateWithScrolls",@(b(@"pageSwipe")),NO);
 // App pointer speed .05..1 -> macOS tracking speed 0..3.
 double speed=MIN(MAX(([o[@"pointerSpeed"] doubleValue]-.05)/.95*3,0),3);global(@"com.apple.trackpad.scaling",@(round(speed*100)/100),NO);
 dock(@"showMissionControlGestureEnabled",b(@"missionControl"));dock(@"showAppExposeGestureEnabled",b(@"appExpose"));
 dock(@"showLaunchpadGestureEnabled",b(@"launchpad"));dock(@"showDesktopGestureEnabled",b(@"showDesktop"));
 return rows;
}
static TCPref TCRowPref(NSDictionary*r){return (TCPref){r[@"d"],[r[@"h"] boolValue],r[@"k"]};}

static void TCMirrorActivate(BOOL dockChanged){
 for(NSString*d in @[TC_MT,TC_BT,TC_DOCK,TC_GLOBAL])CFPreferencesSynchronize(TCApp(d),kCFPreferencesCurrentUser,kCFPreferencesAnyHost);
 CFPreferencesSynchronize(kCFPreferencesAnyApplication,kCFPreferencesCurrentUser,kCFPreferencesCurrentHost);
 // Same tool System Settings relies on to apply pointer preferences without logging out.
 NSTask*t=[NSTask new];t.executableURL=[NSURL fileURLWithPath:@"/System/Library/PrivateFrameworks/SystemAdministration.framework/Resources/activateSettings"];t.arguments=@[@"-u"];[t launchAndReturnError:nil];
 if(dockChanged){NSTask*k=[NSTask new];k.executableURL=[NSURL fileURLWithPath:@"/usr/bin/killall"];k.arguments=@[@"Dock"];[k launchAndReturnError:nil];}
}

// Snapshot previous macOS values once (NSNull = key was absent).
static BOOL TCMirrorSnapshot(NSString*path){
 if([NSFileManager.defaultManager fileExistsAtPath:path])return YES;
 NSMutableDictionary*snap=[NSMutableDictionary dictionary];
 for(NSDictionary*r in TCMirrorValues(@{})){TCPref p=TCRowPref(r);snap[TCPrefID(p)]=TCPrefGet(p)?:@"__absent__";}
 return [snap writeToFile:path atomically:YES];
}

// Returns number of preferences changed.
static int TCMirrorApply(NSDictionary*options){
 int changed=0;BOOL dock=NO;
 for(NSDictionary*r in TCMirrorValues(options)){TCPref p=TCRowPref(r);id old=TCPrefGet(p);if([old isEqual:r[@"v"]])continue;TCPrefSet(p,r[@"v"]);changed++;if([p.domain isEqualToString:TC_DOCK])dock=YES;}
 if(changed)TCMirrorActivate(dock);
 return changed;
}

static BOOL TCMirrorRestore(NSString*path){
 NSDictionary*snap=[NSDictionary dictionaryWithContentsOfFile:path];if(!snap)return NO;
 for(NSDictionary*r in TCMirrorValues(@{})){TCPref p=TCRowPref(r);id v=snap[TCPrefID(p)];if(!v)continue;TCPrefSet(p,[v isEqual:@"__absent__"]?nil:v);}
 TCMirrorActivate(YES);
 return [NSFileManager.defaultManager removeItemAtPath:path error:nil];
}

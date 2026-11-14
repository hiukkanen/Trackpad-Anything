#import <Cocoa/Cocoa.h>
#import <ApplicationServices/ApplicationServices.h>
extern CGError CoreDockSendNotification(CFStringRef,int);
// Notification names verified against LinearMouse DockKit (MIT).
static BOOL TCSystemAction(NSString*action){
 NSDictionary*names=@{@"mission-control":@"com.apple.expose.awake",@"app-expose":@"com.apple.expose.front.awake",@"show-desktop":@"com.apple.showdesktop.awake",@"launchpad":@"com.apple.launchpad.toggle"};
 if(names[action])return CoreDockSendNotification((__bridge CFStringRef)names[action],0)==0;
 if([action isEqualToString:@"lookup"]){ // Standard macOS Look Up shortcut at pointer position.
  CGEventRef down=CGEventCreateKeyboardEvent(NULL,2,true),up=CGEventCreateKeyboardEvent(NULL,2,false);
  CGEventSetFlags(down,kCGEventFlagMaskCommand|kCGEventFlagMaskControl);CGEventSetFlags(up,kCGEventFlagMaskCommand|kCGEventFlagMaskControl);CGEventPost(kCGHIDEventTap,down);CGEventPost(kCGHIDEventTap,up);CFRelease(down);CFRelease(up);return YES;
 }
 if([action isEqualToString:@"notification-center"]){
  NSRunningApplication*cc=[NSRunningApplication runningApplicationsWithBundleIdentifier:@"com.apple.controlcenter"].firstObject;if(!cc)return NO;
  AXUIElementRef app=AXUIElementCreateApplication(cc.processIdentifier);CFTypeRef bar=NULL;BOOL result=NO;
  if(AXUIElementCopyAttributeValue(app,kAXMenuBarAttribute,&bar)==kAXErrorSuccess){CFTypeRef children=NULL;
   if(AXUIElementCopyAttributeValue((AXUIElementRef)bar,kAXChildrenAttribute,&children)==kAXErrorSuccess){for(id child in (__bridge NSArray*)children){CFTypeRef value=NULL;AXUIElementRef item=(__bridge AXUIElementRef)child;if(AXUIElementCopyAttributeValue(item,CFSTR("AXIdentifier"),&value)==kAXErrorSuccess){NSString*identifier=(__bridge NSString*)value;if([identifier isKindOfClass:NSString.class]&&[identifier.lowercaseString containsString:@"clock"]){result=AXUIElementPerformAction(item,kAXPressAction)==kAXErrorSuccess;CFRelease(value);break;}CFRelease(value);}}CFRelease(children);}CFRelease(bar);}CFRelease(app);return result;
 }
 return NO;
}

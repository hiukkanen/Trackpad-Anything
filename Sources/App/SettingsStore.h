#import <Foundation/Foundation.h>
#import "../Core/GestureEngine.h"
static NSDictionary*TCReadOptions(void){return [NSJSONSerialization JSONObjectWithData:[[NSString stringWithUTF8String:TCOptionsJSON()] dataUsingEncoding:NSUTF8StringEncoding] options:0 error:nil];}
static BOOL TCLoadSettings(NSString*path){NSDictionary*d=[NSDictionary dictionaryWithContentsOfFile:path];if(!d)return NO;for(NSString*k in d)if([d[k]isKindOfClass:NSNumber.class])TCSetOption(k.UTF8String,[d[k]doubleValue]);return YES;}
static BOOL TCSaveSettings(NSString*path){return [TCReadOptions() writeToFile:path atomically:YES];}

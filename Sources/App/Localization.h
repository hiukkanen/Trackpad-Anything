// UI language for native strings (menu bar, notices). Shares GUI/src/locales.json with the web UI:
// Korean source strings are keys; {0}, {1}… are placeholders.
#import <Foundation/Foundation.h>

static NSString*TCLanguagePref(void){NSString*p=[NSUserDefaults.standardUserDefaults stringForKey:@"language"];return p.length?p:@"system";}
static NSString*TCLanguage(void){
 NSArray*supported=@[@"ko",@"en",@"ja",@"zh"];NSString*p=TCLanguagePref();
 if([supported containsObject:p])return p;
 for(NSString*l in NSLocale.preferredLanguages){NSString*code=[l substringToIndex:MIN((NSUInteger)2,l.length)];if([supported containsObject:code])return code;}
 return @"en";
}
static NSDictionary*TCLocaleTable(void){
 static NSDictionary*table;static dispatch_once_t once;
 dispatch_once(&once,^{NSData*d=[NSData dataWithContentsOfFile:[NSBundle.mainBundle pathForResource:@"locales" ofType:@"json"]];table=d?[NSJSONSerialization JSONObjectWithData:d options:0 error:nil]:@{};});
 return table;
}
static NSString*TCLf(NSString*ko,NSArray*args){
 NSString*lang=TCLanguage();NSString*s=ko;
 if(![lang isEqualToString:@"ko"]){NSDictionary*t=TCLocaleTable();s=t[lang][ko]?:t[@"en"][ko]?:ko;}
 for(NSUInteger i=0;i<args.count;i++)s=[s stringByReplacingOccurrencesOfString:[NSString stringWithFormat:@"{%lu}",(unsigned long)i] withString:[args[i] description]];
 return s;
}
static NSString*TCL(NSString*ko){return TCLf(ko,@[]);}

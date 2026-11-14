#ifndef TC_PROTOCOL_H
#define TC_PROTOCOL_H
#include <stddef.h>
#include <stdint.h>
#include <math.h>
typedef struct {int id;double x,y;unsigned pressure,major,minor;} TCFinger;
typedef struct {int count,mask,button;TCFinger fingers[16];double x,y,radius,angle,distance;} TCFrame;
static int tc_signed13(unsigned n){n&=8191;return(n&4096)?(int)n-8192:(int)n;}
// Linux hid-magicmouse documents 12-byte USB header + 9 bytes per contact.
static int TCDecode(const uint8_t*b,size_t n,TCFrame*f){
 if(!b||!f||n<12||b[0]!=2||(n-12)%9||n>12+16*9)return 0;
 *f=(TCFrame){0};f->button=b[1]&1;
 for(size_t k=12;k+9<=n;k+=9){const uint8_t*t=b+k;if((t[3]&0xc0)!=0x80||t[4]>200||t[5]>200)continue;int id=t[8]&15;if(f->mask&(1<<id))return 0;
 TCFinger*o=&f->fingers[f->count++];o->id=id;o->x=tc_signed13(t[0]|t[1]<<8);o->y=-tc_signed13((t[1]>>5)|(t[2]<<3)|(t[3]<<11));o->pressure=t[7];o->major=t[4];o->minor=t[5];f->mask|=1<<id;f->x+=o->x;f->y+=o->y;}
 if(f->count){f->x/=f->count;f->y/=f->count;for(int i=0;i<f->count;i++)f->radius+=hypot(f->fingers[i].x-f->x,f->fingers[i].y-f->y);f->radius/=f->count;}
 if(f->count==2){TCFinger a=f->fingers[0],c=f->fingers[1];if(a.id>c.id){TCFinger t=a;a=c;c=t;}f->distance=hypot(a.x-c.x,a.y-c.y);f->angle=atan2(c.y-a.y,c.x-a.x);}return 1;
}
#endif

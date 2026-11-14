#ifndef TC_ENGINE_H
#define TC_ENGINE_H
#include <stdint.h>
int TCStartAtPath(const char*path);
void TCStop(void);
int TCSetOption(const char*key,double value);
const char*TCStatusJSON(void);
const char*TCOptionsJSON(void);
void TCRequestAction(const char*action);
void TCSetSystemActionCallback(void (*callback)(const char*));
#endif

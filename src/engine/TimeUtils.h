#ifndef TimeUtils_H
#define TimeUtils_H

#include <string>
#include <functional>

unsigned long long
GetTickCount();

void TimeAction(std::string message, std::function<void()>& runAction);

#endif

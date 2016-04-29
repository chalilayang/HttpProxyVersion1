#ifndef UTIL_H
#define UTIL_H

#pragma once
#include <string>
#include "p2pbase\udp_def.h"
using namespace std;

std::wstring  NatTypeDesc(SHNatType type);

wstring GetIpStr(int ip);

wstring GetSizeDesc(int size);
#endif
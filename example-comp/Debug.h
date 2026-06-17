#pragma once

#ifndef RHYTHM_CDC_LOG
#define RHYTHM_CDC_LOG 0
#endif

namespace RhythmDebug
{
void Print(const char* format, ...);

template <typename... Args>
int IgnoreArgs(const Args&... args);
}

#if RHYTHM_CDC_LOG
#define DBGPRINT(hw, format, ...) \
    do { RhythmDebug::Print((format), ##__VA_ARGS__); } while(0)
#else
#define DBGPRINT(hw, format, ...) \
    do { (void)sizeof(RhythmDebug::IgnoreArgs((format), ##__VA_ARGS__)); } while(0)
#endif

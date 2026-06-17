#include "Debug.h"

#include <cstdarg>
#include <cstdio>

#if RHYTHM_CDC_LOG
#include "usb_comp_cdc_if.h"
#endif

namespace RhythmDebug
{
void Print(const char* format, ...)
{
#if RHYTHM_CDC_LOG
    static char buffer[256];

    va_list args;
    va_start(args, format);
    int written = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if(written < 0)
        return;

    int len = written;
    if(len > static_cast<int>(sizeof(buffer) - 3U))
        len = static_cast<int>(sizeof(buffer) - 3U);

    buffer[len++] = '\r';
    buffer[len++] = '\n';
    buffer[len] = '\0';

    (void)CDC_Transmit_HS(reinterpret_cast<uint8_t*>(buffer),
                          static_cast<uint16_t>(len));
#else
    (void)format;
#endif
}
}

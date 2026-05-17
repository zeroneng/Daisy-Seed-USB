#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t AudioFifo_GetWritePtrLast(void);
float AudioFifo_GetLeft(uint32_t index);
float AudioFifo_GetRight(uint32_t index);
uint32_t AudioFifo_GetRingSize(void);
uint32_t AudioFifo_GetRingMask(void);

#ifdef __cplusplus
}
#endif

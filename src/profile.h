#pragma once

#define PRF_NO_INSTRUMENT __attribute__((no_instrument_function))

PRF_NO_INSTRUMENT void Prf_StartFrame(void);
PRF_NO_INSTRUMENT void Prf_EndFrame(void);

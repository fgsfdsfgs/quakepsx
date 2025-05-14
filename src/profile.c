#include "profile.h"

#ifdef INSTRUMENT_FUNCTIONS

PRF_NO_INSTRUMENT static inline void PCSX_ExecSlot(const unsigned char slot) {
  *((volatile unsigned char* const)0x1f802081) = slot;
}

PRF_NO_INSTRUMENT void __cyg_profile_func_enter(void *this_fn, void *call_site) {
  register void *a0 asm("a0") = this_fn;
  register void *a1 asm("a1") = call_site;
  __asm__ volatile("" : : "r"(a0), "r"(a1));
  PCSX_ExecSlot(0xfe);
}

PRF_NO_INSTRUMENT void __cyg_profile_func_exit(void *this_fn, void *call_site) {
  register void *a0 asm("a0") = this_fn;
  register void *a1 asm("a1") = call_site;
  __asm__ volatile("" : : "r"(a0), "r"(a1));
  PCSX_ExecSlot(0xff);
}

#else

#define PCSX_ExecSlot(a)

#endif

PRF_NO_INSTRUMENT void Prf_StartFrame(void) {
  PCSX_ExecSlot(0xfd);
}

PRF_NO_INSTRUMENT void Prf_EndFrame(void) {
  PCSX_ExecSlot(0xfc);
}

#include <psxcd.h>

#include "common.h"
#include "system.h"
#include "spu.h"
#include "cd.h"

static struct {
  cdmode_t mode;
  u32 numtracks;
  u32 curtrack; // 0 if data
  qboolean paused;
  volatile qboolean endoftrack;
} cd;

static void AutoPauseCallback(CdlIntrResult result, u8 *data) {
  if (cd.curtrack && cd.mode == CDMODE_AUDIO) {
    // signal to CD_Update that we need to restart the CD track
    cd.endoftrack = true;
    cd.paused = true;
  }
}

void CD_Init(void) {
  // see if there was a CD in the drive during init, die if there wasn't
  if (!CdInit())
    Sys_Error("CD_Init: bad CD or no CD in drive");

  // update status
  CdControl(CdlNop, 0, 0);
  CdStatus();

  // set hispeed mode
  CD_SetMode(CDMODE_DATA);

  // TODO: is this wait necessary? many examples seem to do it
  Sys_Wait(3);

  // set the CD mixer to the default settings (L -> L, R -> R)
  // the actual volume is set via the Snd_ API
  const CdlATV atv = { 128, 0, 128, 0 };
  CdMix(&atv);

  // detect audio tracks, if any
  CdlLOC tracks[100];
  cd.numtracks = CdGetToc(tracks);
  Sys_Printf("CD_Init: %d tracks\n", cd.numtracks);
}

u32 CD_GetCurrentTrack(void) {
  return cd.curtrack;
}

qboolean CD_IsPlaybackPaused(void) {
  return cd.paused;
}

void CD_PlayAudio(const u32 track) {
  // track 1 is data
  if (track < 2 || track > cd.numtracks) {
    Sys_Printf("CD_PlayTrack: track %d does not exist\n", track);
    return;
  }

  CD_SetMode(CDMODE_AUDIO);

  const u32 param = track;
  if (CdControl(CdlPlay, &param, NULL) != 1) {
    Sys_Printf("CD_PlayTrack: CdlPlay failed\n");
    return;
  }

  cd.curtrack = track;
  cd.paused = false;
}

void CD_Pause(const qboolean pause) {
  if (cd.paused == pause)
    return;

  if (cd.mode != CDMODE_AUDIO)
    return;

  if (CdControl(pause ? CdlPause : CdlPlay, NULL, NULL) != 1) {
    Sys_Printf("CD_Pause: failed to set pause=%d\n", pause);
    return;
  }

  cd.paused = pause;
}

void CD_Stop(void) {
  // disable audio mode
  CD_SetMode(CDMODE_DATA);
  // stop the drive motor
  CdControl(CdlStop, NULL, NULL);
}

void CD_SetMode(const cdmode_t mode) {
  if (mode == cd.mode)
    return;

  // set the CDDA flag if we're going into audio mode
  SPU_EnableCDDA(mode == CDMODE_AUDIO);

  // if we were playing something, pause it and reset state
  if (cd.mode == CDMODE_AUDIO) {
    CdControlB(CdlPause, NULL, NULL);
    cd.curtrack = 0;
    cd.paused = false;
  }

  const u32 param = (mode == CDMODE_AUDIO) ? (CdlModeDA | CdlModeAP) : CdlModeSpeed;
  if (CdControlB(CdlSetmode, &param, NULL) != 1) {
    Sys_Printf("CD_EnableCDDA: CdlSetmode failed\n");
    return;
  }

  CdAutoPauseCallback((mode == CDMODE_AUDIO) ? AutoPauseCallback : NULL);

  cd.mode = mode;
}

void CD_Update(void) {
  if (cd.mode != CDMODE_AUDIO)
    return;

  if (cd.curtrack && cd.endoftrack) {
    if (CdControl(CdlPlay, &cd.curtrack, NULL) != 1) {
      Sys_Printf("CD_Update: failed to restart track\n");
      return;
    }
    cd.endoftrack = false;
    cd.paused = false;
  }
}

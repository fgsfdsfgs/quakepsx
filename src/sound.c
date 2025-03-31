#include "common.h"
#include "system.h"
#include "spu.h"
#include "sound.h"

static const sfx_t *snd_sfx = NULL;
static int snd_num_sfx = 0;
static x32vec3_t *listener_origin = NULL;
static x16vec3_t *listener_right = NULL;

static struct sndchan {
  const sfx_t *sfx;
  s16 looping;
  s16 entchannel;
  s16 entnum;
  s16 vol;
  x16 attn;
  s32 endframe;
  x32vec3_t origin;
} snd_chan[SND_NUM_CH];

void Snd_Init(void) {
  SPU_Init();
}

void Snd_SetBank(const sfx_t *sfx, const int num_sfx, const u8 *spudata, const u32 spudata_size) {
  snd_sfx = sfx;
  snd_num_sfx = num_sfx;

  SPU_StartUpload(SPU_RAM_BASE, spudata, spudata_size);
  SPU_WaitForTransfer();

  Sys_Printf("new sound bank loaded: %d sfx, %u bytes\n", num_sfx, spudata_size);
}

void Snd_ResetBank(void) {
  snd_sfx = NULL;
  snd_num_sfx = 0;
}

static struct sndchan *Snd_PickChannel(const s16 entnum, const s16 entch) {
  s16 ch_idx = 0;
  s16 first_to_die = -1;
  s32 life_left = 0x7fffffff;
  const s32 frame_now = Sys_Frames();

  for (ch_idx = SND_CH_DYNAMIC0; ch_idx < SND_NUM_CH; ++ch_idx) {
    if (entch && snd_chan[ch_idx].entnum == entnum && (entch == -1 || snd_chan[ch_idx].entchannel == entch)) {
      // always override sound from same entity
      first_to_die = ch_idx;
      break;
    }

    // don't let monster sounds override player sounds
    if (snd_chan[ch_idx].entnum == 1 && entnum != 1 && snd_chan[ch_idx].sfx)
      continue;

    const s32 dt = snd_chan[ch_idx].endframe - frame_now;
    if (dt < life_left) {
      life_left = dt;
      first_to_die = ch_idx;
    }
  }

  if (first_to_die == -1)
    return NULL;

  if (snd_chan[first_to_die].sfx)
    snd_chan[first_to_die].sfx = NULL;

  return &snd_chan[first_to_die];
}

static qboolean Snd_Spatialize(struct sndchan* ch) {
  const s16 voice = ch - snd_chan;
  s16 lvol = 0;
  s16 rvol = 0;

  // anything coming from the view entity will allways be full volume
  if (ch->entnum == 1) {
    lvol = ch->vol;
    rvol = ch->vol;
  } else {
    x32vec3_t source_vec;
    x16vec3_t dir_vec;
    x32 dist = 0;
    XVecSub(&ch->origin, listener_origin, &source_vec);
    XVecNormLS(&source_vec, &dir_vec, &dist); // this returns squared distance
    // if out of clip distance, don't bother
    if (dist < SND_CLIPDIST * SND_CLIPDIST) {
      dist = SquareRoot0(dist) * ch->attn;
      dist = ONE - dist;
      const x16 dot = XVecDotSS(listener_right, &dir_vec);
      const x32 rscale = xmul32(dist, ONE + dot);
      const x32 lscale = xmul32(dist, ONE - dot);
      rvol = (rscale <= 0) ? 0 : xmul32(rscale, ch->vol);
      lvol = (lscale <= 0) ? 0 : xmul32(lscale, ch->vol);
    }
  }

  SPU_SetVoiceVolume(voice, lvol, rvol);

  return (rvol || lvol);
}

const sfx_t *Snd_FindSound(const s16 id) {
  if (snd_sfx) {
    for (s32 i = 0; i < snd_num_sfx; ++i) {
      if (snd_sfx[i].id == id)
        return &snd_sfx[i];
    }
  }

  return NULL;
}

void Snd_StartSound(const s16 entnum, const s16 entch, const sfx_t *sfx, const x32vec3_t *origin, s16 vol, x32 attn) {
  if (!sfx)
    return;

  // pick a channel to play on
  struct sndchan *target_chan = Snd_PickChannel(entnum, entch);
  if (!target_chan)
    return;

  // spatialize
  target_chan->sfx = NULL;
  target_chan->endframe = 0;
  target_chan->origin = *origin;
  target_chan->entnum = entnum;
  target_chan->entchannel = entch;
  target_chan->vol = vol;
  target_chan->attn = XMUL16(attn, SND_INV_CLIPDIST);
  if (!Snd_Spatialize(target_chan))
    return; // sound is inaudible

  target_chan->sfx = sfx;
  target_chan->endframe = Sys_Frames() + sfx->frames;

  const s16 voice = target_chan - snd_chan;
  SPU_PlaySample(voice, sfx->spuaddr, SND_FREQ);
}

void Snd_Update(x32vec3_t *lorigin, x16vec3_t *lright) {
  const s32 frame_now = Sys_Frames();

  listener_origin = lorigin;
  listener_right = lright;

  struct sndchan *ch = &snd_chan[SND_CH_DYNAMIC0];
  for (int i = SND_CH_DYNAMIC0; i < SND_NUM_CH; ++i, ++ch) {
    if (!ch->sfx)
      continue;
    if (!Snd_Spatialize(ch))
      continue;
  }
}

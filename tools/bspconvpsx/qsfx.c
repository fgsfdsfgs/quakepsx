#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define DR_WAV_IMPLEMENTATION 1
#include "../common/dr_wav.h"
#include "../common/libpsxav/libpsxav.h"
#include "../common/psxtypes.h"
#include "qsfx.h"
#include "util.h"

static char qsfxmap[MAX_QSFX][MAX_QSFX_NAME];
static int num_qsfxmap = 0;

static qsfx_t qsfx[MAX_QSFX];

int qsfxmap_init(const char *mapfile) {
  num_qsfxmap = resmap_parse(mapfile, (char *)qsfxmap, MAX_QSFX, sizeof(qsfxmap[0]), sizeof(qsfxmap[0]));
  printf("qsfxmap_init(): indexed %d sounds from %s\n", num_qsfxmap, mapfile);
  return num_qsfxmap;
}

int qsfxmap_id_for_name(const char *name) {
  for (s32 i = 0; i < num_qsfxmap; ++i) {
    if (!strcmp(name, qsfxmap[i])) {
      return i;
      break;
    }
  }
  return 0;
}

const char *qsfxmap_name_for_id(const int id) {
  if (id <= 0 || id >= num_qsfxmap)
    return NULL;
  return qsfxmap[id];
}

qsfx_t *qsfx_add(const int in_id, const char *name, u8 *start, const size_t size) {
  const int id = in_id ? in_id : qsfxmap_id_for_name(name);
  if (id <= 0) {
    fprintf(stderr, "qsfx_add(): unknown sfx '%s'\n", name);
    return NULL;
  }

  drwav wav;
  if (!drwav_init_memory(&wav, start, size, NULL)) {
    fprintf(stderr, "qsfx_add(): could not open wav '%s'\n", name);
    return NULL;
  }

  if (wav.channels != 1) {
    fprintf(stderr, "qsfx_add(): '%s' is not mono\n", name);
    drwav_uninit(&wav);
    return NULL;
  }

  drwav_int16 *srcsamples = malloc(wav.totalPCMFrameCount * sizeof(drwav_int16));
  assert(srcsamples);

  const int srate = wav.sampleRate;
  const double stime = (double)wav.totalPCMFrameCount / (double)srate;
  int numsrcsamples = drwav_read_pcm_frames_s16(&wav, wav.totalPCMFrameCount, srcsamples);
  drwav_uninit(&wav);

  if (numsrcsamples <= 0) {
    fprintf(stderr, "qsfx_add(): could not read samples from '%s'\n", name);
    return NULL;
  }

  if (srate == 22050 && numsrcsamples > 1) {
    // downsample to 11025 by throwing out every other sample
    numsrcsamples /= 2;
    drwav_int16 *dst = malloc(numsrcsamples * sizeof(drwav_int16));
    assert(dst);
    for (int i = 0, j = 0; j < numsrcsamples; i += 2, ++j)
      dst[j] = srcsamples[i];
    free(srcsamples);
    srcsamples = dst;
  }

  qsfx_t *sfx = &qsfx[id];
  sfx->id = id;
  sfx->samplerate = 11025;
  sfx->samples = srcsamples;
  sfx->numframes = stime * 60.0;
  sfx->numsamples = numsrcsamples;
  sfx->looped = (strstr(name, "ambience/") != NULL); // TODO

  return sfx;
}

qsfx_t *qsfx_find(const int id) {
  if (id <= 0 || id >= MAX_QSFX || !qsfx[id].samples)
    return NULL;
  return &qsfx[id];
}

void qsfx_free(qsfx_t *sfx) {
  free(sfx->samples);
  sfx->samples = NULL;
}

int qsfx_convert(qsfx_t *sfx, u8 *outptr, const int maxlen) {
  // HACK: the lib apparently has no way to set output buffer size, so
  // 4MB ought to be enough for everybody
  static u8 tmp[4 * 1024 * 1024];

  const int loop_start = sfx->looped ? 0 : -1;
  const int adpcm_len = psx_audio_spu_encode_simple(sfx->samples, sfx->numsamples, tmp, loop_start);
  if (adpcm_len <= 0) {
    fprintf(stderr, "could not encode sound '%s'\n", qsfxmap[sfx->id]);
    return -1;
  }

  if (adpcm_len + 16 > maxlen) {
    fprintf(stderr, "could not fit sound '%s' (pcm len %d, adpcm len %d)\n", qsfxmap[sfx->id], sfx->numsamples * 2, adpcm_len + 16);
    return -2;
  }

  // 16 null bytes of lead-in to avoid pops
  memcpy(outptr + 16, tmp, adpcm_len);
  return ALIGN(adpcm_len + 16, 8);
}

#include "prcommon.h"

void spawn_player(edict_t *self) {
  player_state_t *plr = gs.player + EDICT_NUM(self) - 1;
  self->v.extra = plr;
  plr->ent = self;

  self->v.solid = SOLID_SLIDEBOX;
  self->v.movetype = MOVETYPE_WALK;
  self->v.mins = (x32vec3_t){ TO_FIX32(-16), TO_FIX32(-16), TO_FIX32(-24) };
  self->v.maxs = (x32vec3_t){ TO_FIX32(16), TO_FIX32(16), TO_FIX32(32) };

  plr->viewofs.z = TO_FIX32(22);
  plr->viewangles.y = gs.edicts[1].v.angles.y;
  plr->stats.health = PLAYER_HEALTH;
  plr->stats.weaponnum = IT_SHOTGUN;
  plr->stats.ammonum = AMMO_SHELLS;
  plr->stats.ammo[AMMO_SHELLS] = 25;
  plr->stats.items = IT_SHOTGUN | IT_AXE;
}

void Player_PreThink(edict_t *ent) {
  player_state_t *plr = ent->v.extra;

  plr->flags &= ~PFL_JUMPED;

  if ((ent->v.watertype == CONTENTS_WATER) || (ent->v.watertype == CONTENTS_SLIME) || (ent->v.watertype == CONTENTS_LAVA))
    plr->flags |= PFL_INWATER;
  else
    plr->flags &= ~PFL_INWATER;
}

void Player_PostThink(edict_t *ent) {
  player_state_t *plr = ent->v.extra;

  const u16 oldwater = (plr->flags & PFL_INWATER);
  const u16 newwater = (ent->v.watertype == CONTENTS_WATER) || (ent->v.watertype == CONTENTS_SLIME)
    || (ent->v.watertype == CONTENTS_LAVA);

  if (!oldwater && newwater) {
    Snd_StartSoundId(EDICT_NUM(ent), CHAN_BODY, SFXID_PLAYER_INH2O, &ent->v.origin, SND_MAXVOL, ATTN_NORM);
  } else if (!newwater && plr->fallspeed < -TO_FIX32(300) && (ent->v.flags & FL_ONGROUND)) {
    if (plr->fallspeed < -TO_FIX32(650)) {
      // TODO: fall damage
      Snd_StartSoundId(EDICT_NUM(ent), CHAN_BODY, SFXID_PLAYER_LAND2, &ent->v.origin, SND_MAXVOL, ATTN_NORM);
    } else {
      Snd_StartSoundId(EDICT_NUM(ent), CHAN_BODY, SFXID_PLAYER_LAND, &ent->v.origin, SND_MAXVOL, ATTN_NORM);
    }
    plr->fallspeed = 0;
  }

  if (plr->flags & PFL_JUMPED)
    Snd_StartSoundId(EDICT_NUM(ent), CHAN_VOICE, SFXID_PLAYER_PLYRJMP8, &ent->v.origin, SND_MAXVOL, ATTN_NORM);
  else if (!(ent->v.flags & FL_ONGROUND))
    plr->fallspeed = ent->v.velocity.z;
}

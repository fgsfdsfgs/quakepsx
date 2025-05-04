#include "prcommon.h"

static void player_fire(edict_t *self) {
  player_state_t *plr = self->v.player;

  Snd_StartSoundId(EDICT_NUM(self), CHAN_WEAPON, weap_table[plr->stats.weaponnum].noise,
    &self->v.origin, SND_MAXVOL, ATTN_NORM);

  if (plr->stats.weaponnum == WEAP_AXE)
    return;

  if (plr->stats.ammonum >= 0)
    plr->stats.ammo[plr->stats.ammonum]--;

  self->v.angles = plr->viewangles;
  utl_firebullets(self, 4, &rs.vforward, FTOX(0.1), FTOX(0.1));

  self->v.effects |= EF_MUZZLEFLASH;
}

static void player_think(edict_t *self) {
  player_state_t *plr = self->v.player;

  // test gun
  if (!plr->vmodelframe && (plr->buttons & BTN_FIRE)) {
    if (plr->stats.ammonum < 0 || plr->stats.ammo[plr->stats.ammonum] > 0) {
      plr->vmodelframe++;
      player_fire(self);
    }
  } else if (plr->vmodelframe) {
    plr->vmodelframe++;
    if (plr->vmodelframe >= (plr->stats.weaponnum == WEAP_SHOTGUN ? 6 : 5))
      plr->vmodelframe = 0;
  }

  self->v.nextthink = gs.time + PR_FRAMETIME;
}

void player_pain(edict_t *self, edict_t *attacker, s16 damage) {
  if (self->v.health <= 0)
    return;

  Snd_StartSoundId(EDICT_NUM(self), CHAN_VOICE, SFXID_PLAYER_PAIN1, &self->v.origin, SND_MAXVOL, ATTN_NORM);

  Sbar_IndicateDamage(damage);
}

void player_die(edict_t *self) {
  Snd_StartSoundId(EDICT_NUM(self), CHAN_VOICE, SFXID_PLAYER_DEATH1, &self->v.origin, SND_MAXVOL, ATTN_NORM);
  self->v.flags &= ~FL_TAKEDAMAGE;
}

void spawn_player(edict_t *self) {
  player_state_t *plr = gs.player + EDICT_NUM(self) - 1;
  self->v.player = plr;
  plr->ent = self;

  self->v.solid = SOLID_SLIDEBOX;
  self->v.movetype = MOVETYPE_WALK;
  self->v.mins = (x32vec3_t){ TO_FIX32(-16), TO_FIX32(-16), TO_FIX32(-24) };
  self->v.maxs = (x32vec3_t){ TO_FIX32(16), TO_FIX32(16), TO_FIX32(32) };
  self->v.viewheight = TO_FIX32(22);
  self->v.think = player_think;
  self->v.nextthink = 1;
  self->v.health = PLAYER_HEALTH;
  self->v.max_health = self->v.health;
  self->v.flags |= FL_TAKEDAMAGE;
  self->v.th_die = player_die;

  plr->viewofs.z = self->v.viewheight;
  plr->viewangles.y = gs.edicts[1].v.angles.y;
  plr->stats.items = IT_SHOTGUN | IT_AXE;
  plr->stats.ammo[AMMO_SHELLS] = 25;
  plr->stats.weaponnum = WEAP_SHOTGUN;
  plr->stats.ammonum = AMMO_SHELLS;
  plr->vmodel = G_FindAliasModel(MDLID_V_SHOT);

  G_SetSize(self, &self->v.mins, &self->v.maxs);
}

void Player_PreThink(edict_t *ent) {
  player_state_t *plr = ent->v.player;

  plr->flags &= ~PFL_JUMPED;

  if ((ent->v.watertype == CONTENTS_WATER) || (ent->v.watertype == CONTENTS_SLIME) || (ent->v.watertype == CONTENTS_LAVA))
    plr->flags |= PFL_INWATER;
  else
    plr->flags &= ~PFL_INWATER;
}

void Player_PostThink(edict_t *ent) {
  player_state_t *plr = ent->v.player;

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

void Player_SetWeapon(edict_t *ent, const s16 num) {
  player_state_t *plr = ent->v.player;

  if ((plr->stats.items & weap_table[num].item) == 0)
    return;

  plr->stats.weaponnum = num;

  plr->vmodel = G_FindAliasModel(weap_table[plr->stats.weaponnum].vmdl);
  plr->vmodelframe = 0;

  plr->stats.ammonum = weap_table[plr->stats.weaponnum].ammo;
}

void Player_NextWeapon(edict_t *ent) {
  player_state_t *plr = ent->v.player;

  s16 next = plr->stats.weaponnum + 1;
  for (; next < WEAP_COUNT && (plr->stats.items & weap_table[next].item) == 0; ++next);
  if (next == WEAP_COUNT)
    next = WEAP_AXE;

  Player_SetWeapon(ent, next);
}

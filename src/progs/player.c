#include "prcommon.h"

static void player_fire(edict_t *self) {
  player_state_t *plr = self->v.player;

  if (gs.time < plr->attack_finished)
    return;

  if (plr->stats.ammonum >= 0 && plr->stats.ammo[plr->stats.ammonum] <= 0)
    return;

  Snd_StartSoundId(EDICT_NUM(self), CHAN_WEAPON, weap_table[plr->stats.weaponnum].noise,
    &self->v.origin, SND_MAXVOL, ATTN_NORM);

  weap_table[plr->stats.weaponnum].attack(self);
}

static void player_think(edict_t *self) {
  player_state_t *plr = self->v.player;

  if (self->v.health < 1) {
    // big think
    self->v.nextthink = gs.time + PR_FRAMETIME;
    return;
  }

  if (plr->vmodel_think)
    plr->vmodel_think(self);

  if (plr->buttons & BTN_FIRE)
    player_fire(self);

  self->v.nextthink = gs.time + PR_FRAMETIME;
}

void player_pain(edict_t *self, edict_t *attacker, s16 damage) {
  if (self->v.health <= 0)
    return;

  Snd_StartSoundId(EDICT_NUM(self), CHAN_VOICE, SFXID_PLAYER_PAIN1, &self->v.origin, SND_MAXVOL, ATTN_NORM);

  Sbar_IndicateDamage(damage);
}

void player_die(edict_t *self, edict_t *killer) {
  player_state_t *plr = self->v.player;

  plr->vmodel = NULL;
  plr->vmodel_think = NULL;
  plr->vmodel_frame = 0;
  plr->viewangles.x = 0;
  plr->viewangles.z = 0;
  plr->viewofs.x = 0;
  plr->viewofs.y = 0;
  plr->viewofs.z = TO_FIX32(-4);
  plr->stats.items &= ~IT_INVISIBILITY;

  self->v.flags &= ~(FL_TAKEDAMAGE | FL_ONGROUND);
  self->v.solid = SOLID_NOT;
  self->v.movetype = MOVETYPE_TOSS;
  self->v.viewheight = TO_FIX32(-4);
  self->v.angles.x = 0;
  self->v.angles.z = 0;

  if (self->v.velocity.z < TO_FIX32(10))
    self->v.velocity.z += TO_FIX32(rand() % 300);

  if (self->v.health < -40) {
    Snd_StartSoundId(EDICT_NUM(self), CHAN_VOICE, SFXID_PLAYER_GIB, &self->v.origin, SND_MAXVOL, ATTN_NORM);
    fx_throw_head(self, MDLID_GIB1, self->v.health);
    fx_throw_gib(&self->v.origin, MDLID_GIB2, self->v.health);
    fx_throw_gib(&self->v.origin, MDLID_GIB3, self->v.health);
    return;
  }

  Snd_StartSoundId(EDICT_NUM(self), CHAN_VOICE, SFXID_PLAYER_DEATH1, &self->v.origin, SND_MAXVOL, ATTN_NORM);
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
  plr->stats.items = IT_SHOTGUN | IT_AXE | IT_SUPER_SHOTGUN | IT_NAILGUN | IT_SUPER_NAILGUN | IT_GRENADE_LAUNCHER | IT_ROCKET_LAUNCHER;
  plr->stats.ammo[AMMO_SHELLS] = 50;
  plr->stats.ammo[AMMO_NAILS] = 200;
  plr->stats.ammo[AMMO_ROCKETS] = 50;
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
  plr->vmodel_frame = 0;
  plr->vmodel_think = NULL;

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

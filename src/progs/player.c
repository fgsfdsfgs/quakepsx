#include <string.h>
#include "cd.h"
#include "prcommon.h"

#define IT_ALL_SIGILS (IT_SIGIL1 | IT_SIGIL2 | IT_SIGIL3 | IT_SIGIL4)

static const u32 powerup_table[] = {
  IT_SUPERHEALTH,
  IT_INVISIBILITY,
  IT_INVULNERABILITY,
  IT_SUIT,
  IT_QUAD
};

static const char *powerup_warnings[] = {
  "",
  "Ring of Shadows magic is fading",
  "Protection is almost burned out",
  "Air supply in Biosuit expiring",
  "Quad Damage is wearing off"
};

static void player_fire(edict_t *self) {
  player_state_t *plr = self->v.player;

  if (gs.time < plr->attack_finished)
    return;

  if (plr->stats.ammonum >= 0 && plr->stats.ammo[plr->stats.ammonum] <= 0)
    return;

  plr->show_hostile = gs.time + ONE;

  Snd_StartSoundId(EDICT_NUM(self), CHAN_WEAPON, weap_table[plr->stats.weaponnum].noise,
    &self->v.origin, SND_MAXVOL, ATTN_NORM);

  weap_table[plr->stats.weaponnum].attack(self);
}

static void player_tick_powerups(edict_t *self) {
  player_state_t *plr = self->v.player;

  // mega health rot
  if (plr->stats.items & IT_SUPERHEALTH) {
    if (plr->power_time[POWER_MEGAHEALTH] <= gs.time) {
      if (self->v.health > self->v.max_health) {
        self->v.health--;
        plr->power_time[POWER_MEGAHEALTH] = gs.time + ONE;
      } else {
        plr->stats.items &= ~IT_SUPERHEALTH;
        plr->power_time[POWER_MEGAHEALTH] = -1;
      }
    }
  }

  // powerups expiring
  for (int i = 1; i < MAX_POWERUPS; ++i) {
    if (!(plr->stats.items & powerup_table[i]))
      continue;

    // sound and screen flash when item starts to run out
    if (plr->power_time[i] < gs.time + TO_FIX32(3)) {
      if (plr->power_warn[i] == 1) {
        Scr_SetTopMsg(powerup_warnings[i]);
        Scr_SetBlend(C_YELLOW, SCR_FLASH_TIME);
        // TODO: sound
        plr->power_warn[i] = gs.time + ONE;
      } else if (plr->power_warn[i] < gs.time) {
        Scr_SetBlend(C_YELLOW, SCR_FLASH_TIME);
        // TODO: sound
        plr->power_warn[i] = gs.time + ONE;
      }

      if (plr->power_time[i] < gs.time) {
        // just stopped
        plr->stats.items &= ~powerup_table[i];
        plr->power_time[i] = 0;
        plr->power_warn[i] = 0;
      }
    }
  }

  // don't drown with biosuit on
  if (plr->power_time[POWER_SUIT])
    plr->air_finished = gs.time + TO_FIX32(12);
}

static void player_exit_intermission(const player_state_t *plr) {
  pr.intermission_state++;
  pr.intermission_time = gs.time + ONE;

  // run some text if at the end of an episode
  if (pr.intermission_state == 2) {
    if (pr.nextmap && !strcmp(pr.nextmap, "START")) {
      CD_PlayAudio(2);
      // TODO: episode win text
      Scr_SetCenterMsg("gg");
      return;
    }
  } else if (pr.intermission_state == 3) {
    // TODO: all runes text
    if ((plr->stats.items & IT_ALL_SIGILS) == IT_ALL_SIGILS) {
      Scr_SetCenterMsg("You got all the runes!");
      return;
    }
  }

  G_RequestMap(pr.nextmap);
}

static void player_think_intermission(edict_t *self) {
  player_state_t *plr = self->v.player;

  self->v.nextthink = gs.time + PR_FRAMETIME;

  if (gs.time < pr.intermission_time)
    return;

  if (plr->buttons & (BTN_FIRE | BTN_JUMP))
    player_exit_intermission(plr);
}

static void player_think(edict_t *self) {
  if (pr.intermission_state) {
    player_think_intermission(self);
    return;
  }

  player_state_t *plr = self->v.player;

  if (self->v.health < 1) {
    // big think
    self->v.nextthink = gs.time + PR_FRAMETIME;
    return;
  }

  player_tick_powerups(self);

  if (plr->vmodel_think)
    plr->vmodel_think(self);

  if (plr->buttons & BTN_FIRE)
    player_fire(self);

  self->v.nextthink = gs.time + PR_FRAMETIME;
}

void player_pain(edict_t *self, edict_t *attacker, s16 damage) {
  if (self->v.health <= 0)
    return;

  if (self->v.player->pain_finished > gs.time)
    return;

  self->v.player->pain_finished = gs.time + HALF;

  static const s16 painsfx[3] = { SFXID_PLAYER_PAIN1, SFXID_PLAYER_PAIN3, SFXID_PLAYER_PAIN6 };

  s16 sfx;
  if (self->v.waterlevel > 2) {
    sfx = SFXID_PLAYER_DROWN2;
  } else if (self->v.waterlevel && self->v.watertype == CONTENTS_LAVA) {
    sfx = SFXID_PLAYER_LBURN1;
  } else {
    sfx = painsfx[rand() & 3];
  }

  utl_sound(self, CHAN_VOICE, sfx, SND_MAXVOL, ATTN_NORM);

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
     utl_sound(self, CHAN_VOICE, SFXID_PLAYER_GIB, SND_MAXVOL, ATTN_NORM);
    fx_throw_head(self, MDLID_GIB1, self->v.health);
    fx_throw_gib(&self->v.origin, MDLID_GIB2, self->v.health);
    fx_throw_gib(&self->v.origin, MDLID_GIB3, self->v.health);
    return;
  }

  if (self->v.waterlevel > 2)
    utl_sound(self, CHAN_VOICE, SFXID_PLAYER_H2ODEATH, SND_MAXVOL, ATTN_NORM);
  else
    utl_sound(self, CHAN_VOICE, SFXID_PLAYER_DEATH1, SND_MAXVOL, ATTN_NORM);
}

void player_set_weapon(edict_t *ent, const s16 num) {
  player_state_t *plr = ent->v.player;

  if ((plr->stats.items & weap_table[num].item) == 0)
    return;

  plr->stats.weaponnum = num;

  plr->vmodel = G_FindAliasModel(weap_table[plr->stats.weaponnum].vmdl);
  plr->vmodel_frame = 0;
  plr->vmodel_think = NULL;

  plr->stats.ammonum = weap_table[plr->stats.weaponnum].ammo;
}

void player_next_weapon(edict_t *ent) {
  player_state_t *plr = ent->v.player;

  s16 next = plr->stats.weaponnum + 1;
  for (; next < WEAP_COUNT && (plr->stats.items & weap_table[next].item) == 0; ++next);
  if (next == WEAP_COUNT)
    next = WEAP_AXE;

  player_set_weapon(ent, next);
}

void player_prev_weapon(edict_t *ent) {
  player_state_t *plr = ent->v.player;

  s16 next = plr->stats.weaponnum - 1;
  if (next < 0)
    next = WEAP_COUNT - 1;
  for (; next > WEAP_AXE && (plr->stats.items & weap_table[next].item) == 0; --next);

  player_set_weapon(ent, next);
}

void spawn_player(edict_t *self) {
  player_state_t *plr = gs.player + EDICT_NUM(self) - 1;
  self->v.player = plr;
  plr->ent = self;

  self->v.solid = SOLID_SLIDEBOX;
  self->v.movetype = MOVETYPE_WALK;
  self->v.mins = (x32vec3_t){{ TO_FIX32(-16), TO_FIX32(-16), TO_FIX32(-24) }};
  self->v.maxs = (x32vec3_t){{ TO_FIX32(+16), TO_FIX32(+16), TO_FIX32(+32) }};
  self->v.viewheight = TO_FIX32(22);
  self->v.think = player_think;
  self->v.nextthink = 1;
  self->v.health = PLAYER_HEALTH;
  self->v.max_health = self->v.health;
  self->v.flags |= FL_TAKEDAMAGE | FL_AUTOAIM;
  self->v.th_die = player_die;

  plr->viewofs.z = self->v.viewheight;
  plr->viewangles.y = gs.edicts[1].v.angles.y;

  if (plr->stats.items == 0) {
    // spawning for the first time
    plr->stats.items = IT_SHOTGUN | IT_AXE;
    plr->stats.ammo[AMMO_SHELLS] = 25;
    player_set_weapon(self, WEAP_SHOTGUN);
  } else {
    // level transition
    // TODO: a better way to detect this
    player_set_weapon(self, plr->stats.weaponnum);
    plr->stats.items &= ~(IT_KEY1 | IT_KEY2 | IT_INVISIBILITY | IT_INVULNERABILITY | IT_SUIT | IT_QUAD | IT_SUPERHEALTH);
  }

  plr->air_finished = gs.time + TO_FIX32(12);

  G_SetSize(self, &self->v.mins, &self->v.maxs);
}

// progs <-> engine interface

void Player_PreThink(edict_t *ent) {
  player_state_t *plr = ent->v.player;

  plr->flags &= ~PFL_JUMPED;

  if ((ent->v.watertype == CONTENTS_WATER) || (ent->v.watertype == CONTENTS_SLIME) || (ent->v.watertype == CONTENTS_LAVA))
    plr->flags |= PFL_INWATER;
  else
    plr->flags &= ~PFL_INWATER;

  if (ent->v.waterlevel > 2)
    plr->flags |= PFL_NOAIR;
  else
    plr->flags &= ~PFL_NOAIR;
}

void Player_PostThink(edict_t *ent) {
  player_state_t *plr = ent->v.player;

  if (ent->v.health <= 0) {
    plr->fallspeed = 0;
    plr->flags = 0;
    return;
  }

  const u16 oldwater = (plr->flags & PFL_INWATER);
  const u16 newwater = (ent->v.watertype == CONTENTS_WATER) || (ent->v.watertype == CONTENTS_SLIME)
    || (ent->v.watertype == CONTENTS_LAVA);

  if (!oldwater && newwater) {
    utl_sound(ent, CHAN_BODY, SFXID_PLAYER_INH2O, SND_MAXVOL, ATTN_NORM);
  } else if (!newwater && plr->fallspeed < -TO_FIX32(300) && (ent->v.flags & FL_ONGROUND)) {
    if (plr->fallspeed < -TO_FIX32(650)) {
      utl_damage(ent, gs.edicts, gs.edicts, 5);
      utl_sound(ent, CHAN_BODY, SFXID_PLAYER_LAND2, SND_MAXVOL, ATTN_NORM);
    } else {
      utl_sound(ent, CHAN_BODY, SFXID_PLAYER_LAND, SND_MAXVOL, ATTN_NORM);
    }
    plr->fallspeed = 0;
  }

  // drowning
  if (ent->v.waterlevel == 3) {
    if (plr->air_finished < gs.time) {
      if (plr->pain_finished < gs.time) {
        ent->v.dmg += 2;
        if (ent->v.dmg > 15)
          ent->v.dmg = 10; // ???
        utl_damage(ent, gs.edicts, gs.edicts, ent->v.dmg);
        plr->pain_finished = gs.time + ONE;
      }
    }
  } else {
    // only play the gasp if we were in deep water last tick
    if ((plr->air_finished < gs.time + TO_FIX32(9)) && (plr->flags & PFL_NOAIR))
      utl_sound(ent, CHAN_VOICE, SFXID_PLAYER_GASP1, SND_MAXVOL, ATTN_NORM);
    plr->air_finished = gs.time + TO_FIX32(12);
    ent->v.dmg = 2;
  }

  // slime/lava damage
  if (ent->v.waterlevel) {
    if (ent->v.watertype == CONTENTS_LAVA) {
      // do damage
      if (plr->dmg_time < gs.time) {
        if (plr->power_time[POWER_SUIT] > gs.time)
          plr->dmg_time = gs.time + ONE;
        else
          plr->dmg_time = gs.time + HALF;
        utl_damage(ent, gs.edicts, gs.edicts, 10 * ent->v.waterlevel);
      }
    } else if (ent->v.watertype == CONTENTS_SLIME) {
      // do damage
      if (plr->dmg_time < gs.time && plr->power_time[POWER_SUIT] < gs.time) {
        plr->dmg_time = gs.time + ONE;
        utl_damage(ent, gs.edicts, gs.edicts, 4 * ent->v.waterlevel);
      }
    }
  }

  // weapon changing
  if (plr->buttons & BTN_NEXTWEAPON) {
    player_next_weapon(ent);
    plr->buttons &= ~BTN_NEXTWEAPON;
  } else if (plr->buttons & BTN_PREVWEAPON) {
    player_prev_weapon(ent);
    plr->buttons &= ~BTN_PREVWEAPON;
  }

  if (plr->flags & PFL_JUMPED)
    utl_sound(ent, CHAN_VOICE, SFXID_PLAYER_PLYRJMP8, SND_MAXVOL, ATTN_NORM);
  else if (!(ent->v.flags & FL_ONGROUND))
    plr->fallspeed = ent->v.velocity.z;
}

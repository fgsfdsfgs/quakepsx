#include "prcommon.h"

static void axe_attack(edict_t *self);
static void shotgun_attack(edict_t *self);
static void sshotgun_attack(edict_t *self);
static void nailgun_attack(edict_t *self);
static void snailgun_attack(edict_t *self);
static void grenade_attack(edict_t *self);
static void rocket_attack(edict_t *self);
static void thunder_attack(edict_t *self);

const weapdesc_t weap_table[WEAP_COUNT] = {
  { "Axe",                      IT_AXE,              axe_attack,      MDLID_V_AXE,   -1,           SFXID_WEAPONS_AX1      },
  { "Shotgun",                  IT_SHOTGUN,          shotgun_attack,  MDLID_V_SHOT,  AMMO_SHELLS,  SFXID_WEAPONS_GUNCOCK  },
  { "Double-barrelled Shotgun", IT_SUPER_SHOTGUN,    sshotgun_attack, MDLID_V_SHOT2, AMMO_SHELLS,  SFXID_WEAPONS_SHOTGN2  },
  { "Nailgun",                  IT_NAILGUN,          nailgun_attack,  MDLID_V_NAIL,  AMMO_NAILS,   SFXID_WEAPONS_ROCKET1I },
  { "Super Nailgun",            IT_SUPER_NAILGUN,    snailgun_attack, MDLID_V_NAIL2, AMMO_NAILS,   SFXID_WEAPONS_SPIKE2   },
  { "Grenade Launcher",         IT_GRENADE_LAUNCHER, grenade_attack,  MDLID_V_ROCK,  AMMO_ROCKETS, SFXID_WEAPONS_GRENADE  },
  { "Rocket Launcher",          IT_ROCKET_LAUNCHER,  rocket_attack,   MDLID_V_ROCK2, AMMO_ROCKETS, SFXID_WEAPONS_SGUN1    },
  { "Thunderbolt",              IT_LIGHTNING,        thunder_attack,  MDLID_V_LIGHT, AMMO_CELLS,   SFXID_WEAPONS_LSTART   },
};

static void attack_cycle(edict_t *self) {
  player_state_t *plr = self->v.player;
  plr->vmodel_frame++;
  if (plr->vmodel_frame > plr->vmodel_end_frame) {
    plr->vmodel_frame = 0;
    plr->vmodel_think = NULL;
  }
}

static void axe_attack_cycle(edict_t *self) {
  player_state_t *plr = self->v.player;

  const qboolean do_attack = plr->vmodel_frame == 3;

  attack_cycle(self);

  if (!do_attack)
    return;

  utl_makevectors(&plr->viewangles);

  x32vec3_t src = {{ self->v.origin.x, self->v.origin.y, self->v.origin.z + TO_FIX32(16) }};
  x32vec3_t dst = {{
    src.x + (x32)pr.v_forward.x * 64,
    src.y + (x32)pr.v_forward.y * 64,
    src.z + (x32)pr.v_forward.z * 64
  }};

  const trace_t *trace = utl_traceline(&src, &dst, false, self);
  if (trace->fraction == ONE)
    return;

  if (trace->ent->v.flags & FL_TAKEDAMAGE) {
    fx_spawn_blood(&trace->endpos, 20);
    utl_damage(trace->ent, self, self, 20);
  } else {
    // hit wall
    Snd_StartSoundId(EDICT_NUM(self), CHAN_WEAPON, SFXID_PLAYER_AXHIT2, &self->v.origin, SND_MAXVOL, ATTN_NORM);
    fx_spawn_gunshot(&trace->endpos);
  }
}

static void axe_attack(edict_t *self) {
  player_state_t *plr = self->v.player;
  plr->attack_finished = gs.time + HALF;
  plr->vmodel_frame = 1;
  plr->vmodel_end_frame = 4;
  plr->vmodel_think = axe_attack_cycle;
}

static void shotgun_attack(edict_t *self) {
  player_state_t *plr = self->v.player;

  plr->stats.ammo[AMMO_SHELLS]--;

  x16vec3_t dir;
  utl_makevectors(&plr->viewangles);
  utl_aim(self, &dir);
  utl_firebullets(self, 6, &dir, FTOX(0.04), FTOX(0.04));

  self->v.effects |= EF_MUZZLEFLASH;

  plr->attack_finished = gs.time + HALF;
  plr->punchangle = -TO_DEG16(2);
  plr->vmodel_frame = 1;
  plr->vmodel_end_frame = plr->vmodel->numframes - 1;
  plr->vmodel_think = attack_cycle;
}

static void sshotgun_attack(edict_t *self) {
  player_state_t *plr = self->v.player;

  if (plr->stats.ammo[AMMO_SHELLS] == 1) {
    Player_SetWeapon(self, WEAP_SHOTGUN);
    shotgun_attack(self);
    return;
  }

  plr->stats.ammo[AMMO_SHELLS] -= 2;

  x16vec3_t dir;
  utl_makevectors(&plr->viewangles);
  utl_aim(self, &dir);
  utl_firebullets(self, 14, &dir, FTOX(0.14), FTOX(0.08));

  self->v.effects |= EF_MUZZLEFLASH;

  plr->attack_finished = gs.time + FTOX(0.7);
  plr->punchangle = -TO_DEG16(4);
  plr->vmodel_frame = 1;
  plr->vmodel_end_frame = plr->vmodel->numframes - 1;
  plr->vmodel_think = attack_cycle;
}

static edict_t *fire_spikes(edict_t *self, const x16vec3_t *angles, const s16 dx) {
  x16vec3_t dir;
  utl_makevectors(angles);
  utl_aim(self, &dir);
  x32vec3_t org;
  org.x = self->v.origin.x + pr.v_right.x * dx;
  org.y = self->v.origin.y + pr.v_right.y * dx;
  org.z = self->v.origin.z + pr.v_right.z * dx + TO_FIX32(16);
  return utl_launch_spike(self, &org, angles, &dir, 1000);
}

static void nailgun_attack_cycle(edict_t *self) {
  player_state_t *plr = self->v.player;

  if ((plr->buttons & BTN_FIRE) == 0 || plr->stats.ammo[AMMO_NAILS] <= 0) {
    plr->vmodel_frame = 0;
    plr->vmodel_think = NULL;
    return;
  }

  plr->stats.ammo[AMMO_NAILS]--;

  Snd_StartSoundId(EDICT_NUM(self), CHAN_WEAPON, weap_table[plr->stats.weaponnum].noise,
    &self->v.origin, SND_MAXVOL, ATTN_NORM);

  const qboolean is_super = plr->stats.weaponnum == WEAP_SUPER_NAILGUN;
  const s16 offset = is_super ? 0 : ((plr->vmodel_frame & 1) ? -4 : +4);
  edict_t *newmis = fire_spikes(self, &plr->viewangles, offset);
  self->v.effects |= EF_MUZZLEFLASH;
  plr->attack_finished = gs.time + FTOX(0.1);
  plr->punchangle = -TO_DEG16(2);

  if (is_super && plr->stats.ammo[AMMO_NAILS]) {
    plr->stats.ammo[AMMO_NAILS]--;
    newmis->v.dmg = 18;
  }

  attack_cycle(self);
}

static void nailgun_attack(edict_t *self) {
  player_state_t *plr = self->v.player;

  plr->stats.ammo[AMMO_NAILS]--;

  fire_spikes(self, &plr->viewangles, +4);

  self->v.effects |= EF_MUZZLEFLASH;

  plr->attack_finished = gs.time + FTOX(0.1);
  plr->punchangle = -TO_DEG16(2);
  plr->vmodel_frame = 1;
  plr->vmodel_end_frame = plr->vmodel->numframes - 1;
  plr->vmodel_think = nailgun_attack_cycle;
}

static void snailgun_attack(edict_t *self) {
  player_state_t *plr = self->v.player;

  if (plr->stats.ammo[AMMO_NAILS] == 1) {
    nailgun_attack(self);
    return;
  }

  plr->stats.ammo[AMMO_NAILS] -= 2;

  fire_spikes(self, &plr->viewangles, 0);

  self->v.effects |= EF_MUZZLEFLASH;

  plr->attack_finished = gs.time + FTOX(0.1);
  plr->punchangle = -TO_DEG16(2);
  plr->vmodel_frame = 1;
  plr->vmodel_end_frame = plr->vmodel->numframes - 1;
  plr->vmodel_think = nailgun_attack_cycle;
}

static void grenade_attack(edict_t *self) {
  player_state_t *plr = self->v.player;

  plr->stats.ammo[AMMO_ROCKETS]--;

  self->v.effects |= EF_MUZZLEFLASH;

  plr->attack_finished = gs.time + FTOX(0.6);
  plr->punchangle = -TO_DEG16(2);
  plr->vmodel_frame = 1;
  plr->vmodel_end_frame = plr->vmodel->numframes - 1;
  plr->vmodel_think = attack_cycle;

  utl_makevectors(&plr->viewangles);

  edict_t *missile = utl_launch_grenade(self, &plr->viewangles);
  missile->v.dmg = 120;
  if (plr->viewangles.x) {
    const s16 rrnd = XMUL16(10, trace_random());
    const s16 urnd = XMUL16(10, trace_random());
    missile->v.velocity.x = pr.v_forward.x * 600 + pr.v_up.x * 200 + pr.v_right.x * rrnd + pr.v_up.x * urnd;
    missile->v.velocity.y = pr.v_forward.y * 600 + pr.v_up.y * 200 + pr.v_right.y * rrnd + pr.v_up.y * urnd;
    missile->v.velocity.z = pr.v_forward.z * 600 + pr.v_up.z * 200 + pr.v_right.z * rrnd + pr.v_up.z * urnd;
  } else {
    x16vec3_t dir;
    utl_aim(self, &dir);
    missile->v.velocity.x = dir.x * 600;
    missile->v.velocity.y = dir.y * 600;
    missile->v.velocity.z = dir.z * 600 + TO_FIX32(200);
  }
}

static void rocket_attack(edict_t *self) {
  player_state_t *plr = self->v.player;

  plr->stats.ammo[AMMO_ROCKETS]--;

  self->v.effects |= EF_MUZZLEFLASH;

  plr->attack_finished = gs.time + FTOX(0.8);
  plr->punchangle = -TO_DEG16(2);
  plr->vmodel_frame = 1;
  plr->vmodel_end_frame = plr->vmodel->numframes - 1;
  plr->vmodel_think = attack_cycle;

  x16vec3_t dir;
  utl_makevectors(&plr->viewangles);
  utl_aim(self, &dir);

  x32vec3_t org;
  org.x = self->v.origin.x + (x32)dir.x * 8;
  org.y = self->v.origin.y + (x32)dir.y * 8;
  org.z = self->v.origin.z + (x32)dir.z * 8 + TO_FIX32(16);

  utl_launch_rocket(self, &org, &plr->viewangles, &dir, 1000);
}

static void thunder_attack(edict_t *self) {
  // TODO
}

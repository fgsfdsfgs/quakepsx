#pragma once

#include "../common.h"
#include "../game.h"
#include "../move.h"
#include "../entity.h"
#include "../progs.h"
#include "../sound.h"
#include "../screen.h"
#include "../system.h"
#include "entclasses.h"
#include "mdlids.h"
#include "picids.h"
#include "sfxids.h"

// player stuff
#define PLAYER_HEALTH      100
#define PLAYER_AIM_MINDIST FTOX(0.95)
#define PLAYER_AIM_ENABLED 1

// weapons
#define WEAP_AXE              0
#define WEAP_SHOTGUN          1
#define WEAP_SUPER_SHOTGUN    2
#define WEAP_NAILGUN          3
#define WEAP_SUPER_NAILGUN    4
#define WEAP_GRENADE_LAUNCHER 5
#define WEAP_ROCKET_LAUNCHER  6
#define WEAP_LIGHTNING        7
#define WEAP_COUNT            8

// powerups
#define POWER_MEGAHEALTH 0
#define POWER_INVIS      1
#define POWER_INVULN     2
#define POWER_SUIT       3
#define POWER_QUAD       4

// items
#define IT_SHOTGUN          1u
#define IT_SUPER_SHOTGUN    2u
#define IT_NAILGUN          4u
#define IT_SUPER_NAILGUN    8u
#define IT_GRENADE_LAUNCHER 16u
#define IT_ROCKET_LAUNCHER  32u
#define IT_LIGHTNING        64u
#define IT_SUPER_LIGHTNING  128u
#define IT_SHELLS           256u
#define IT_NAILS            512u
#define IT_ROCKETS          1024u
#define IT_CELLS            2048u
#define IT_AXE              4096u
#define IT_ARMOR1           8192u
#define IT_ARMOR2           16384u
#define IT_ARMOR3           32768u
#define IT_SUPERHEALTH      65536u
#define IT_KEY1             131072u
#define IT_KEY2             262144u
#define IT_INVISIBILITY     524288u
#define IT_INVULNERABILITY  1048576u
#define IT_SUIT             2097152u
#define IT_QUAD             4194304u
#define IT_SIGIL1           (1u << 28)
#define IT_SIGIL2           (1u << 29)
#define IT_SIGIL3           (1u << 30)
#define IT_SIGIL4           (1u << 31)

// ammo
#define AMMO_SHELLS 0
#define AMMO_NAILS 1
#define AMMO_ROCKETS 2
#define AMMO_CELLS 3

// sight range
#define RANGE_MELEE 0
#define RANGE_NEAR 1
#define RANGE_MID 2
#define RANGE_FAR 3

// weapon table
typedef struct {
  const char *name;
  u32 item;
  think_fn_t attack;
  s16 vmdl;
  s16 ammo;
  s16 noise;
} weapdesc_t;

extern const weapdesc_t weap_table[WEAP_COUNT];

// prog globals

typedef struct {
  // last result of a utl_makevectors
  x16vec3_t v_forward;
  x16vec3_t v_right;
  x16vec3_t v_up;

  // last result of a utl_vectoangles
  x16vec3_t v_angles;

  // last trace
  const trace_t *trace;

  // when a monster becomes angry at a player, that monster will be used
  // as the sight target the next frame so that monsters near that one
  // will wake up even if they wouldn't have noticed the player
  edict_t *sight_entity;
  x32 sight_entity_time;

  // combat globals
  qboolean enemy_vis;
  qboolean enemy_infront;
  s16 enemy_range;
  x16 enemy_yaw;

  // multidamage
  edict_t *multi_ent;
  s16 multi_damage;

  // stats
  s16 total_monsters;
  s16 killed_monsters;
  s16 total_secrets;
  s16 found_secrets;
} pr_globals_t;

extern pr_globals_t pr;

// extra data for monsters

enum monsterstate_e {
  MSTATE_STAND,
  MSTATE_WALK,
  MSTATE_RUN,
  MSTATE_MISSILE,
  MSTATE_MELEE,
  MSTATE_DIE_A,
  MSTATE_DIE_B,
  MSTATE_PAIN_A,
  MSTATE_PAIN_B,
  MSTATE_PAIN_C,
  MSTATE_PAIN_D,
  MSTATE_PAIN_E,
  MSTATE_EXTRA,
  MSTATE_COUNT
};

enum attackstate_e {
  AS_NONE,
  AS_SLIDING,
  AS_STRAIGHT,
  AS_MELEE,
  AS_MISSILE
};

typedef struct {
  think_fn_t think_fn;
  s16 first_frame;
  s16 last_frame;
} monster_state_t;

typedef struct {
  const monster_state_t *state_table;
  damage_fn_t fn_start_pain;
  interact_fn_t fn_start_die;
  check_fn_t fn_check_attack;
  s16 sight_sound;
} monster_class_t;

typedef struct monster_fields_s {
  const monster_class_t *class;
  const monster_state_t *state;
  edict_t *enemy;
  edict_t *oldenemy;
  edict_t *movetarget;
  edict_t *goalentity;
  x32 hunt_time;
  x32 pause_time;
  x32 search_time;
  x32 pain_finished;
  x32 attack_finished;
  x32 show_hostile;
  x16 ideal_yaw;
  x16 yaw_speed;
  s16 state_num;
  s16 attack_state;
  s16 lefty;
} monster_fields_t;

// extra data for doors, buttons and plats

enum door_state_e {
  STATE_TOP,
  STATE_BOTTOM,
  STATE_UP,
  STATE_DOWN
};

typedef struct door_fields_s {
  x32vec3_t pos1;
  x32vec3_t pos2;
  x32vec3_t dest;
  x32vec3_t start;
  edict_t *linked;
  edict_t *field;
  think_fn_t reached;
  x32 touch_finished;
  x32 wait;
  s16 state;
} door_fields_t;

// common thinkers
void null_think(edict_t *self);
void null_touch(edict_t *self, edict_t *other);
void cycler_think(edict_t *self, const s16 idle_start, const s16 idle_end);

// effects
void fx_spawn_blood(const x32vec3_t *org, const s16 damage);
void fx_spawn_gunshot(const x32vec3_t *org);
void fx_spawn_explosion(const x32vec3_t *org);
void fx_spawn_telefog(const x32vec3_t *org);
void fx_throw_gib(const x32vec3_t *org, const s16 mdlid, const s16 damage);
void fx_throw_head(edict_t *self, const s16 mdlid, const s16 damage);

// tracing and other utilities
const trace_t *utl_traceline(const x32vec3_t *v1, const x32vec3_t *v2, const qboolean nomonsters, edict_t *ent);
void utl_makevectors(const x16vec3_t *angles);
void utl_vectoangles(const x16vec3_t *dir);
void utl_remove(edict_t *self);
void utl_remove_delayed(edict_t *self);
void utl_set_movedir(edict_t *self, x16vec3_t *movedir);
void utl_calc_move(edict_t *self, const x32vec3_t *tdest, const s16 tspeed, think_fn_t func);
void utl_usetargets(edict_t *self, edict_t *activator);
void utl_sound(edict_t *self, const s16 chan, const s16 sndid, const u8 vol, const x32 attn);
void utl_spawn_backpack(edict_t *owner, const u32 items, const s16 ammo_type, const s16 ammo_count);

// combat
void utl_damage(edict_t *targ, edict_t *inflictor, edict_t *attacker, s16 damage);
void utl_killed(edict_t *self, edict_t *attacker);
void utl_traceattack(edict_t *self, s16 damage, const x32vec3_t *dir);
void utl_firebullets(edict_t *self, int shotcount, const x16vec3_t *dir, const x16 spread_x, const x16 spread_y);
void utl_become_explosion(edict_t *self);
qboolean utl_can_damage(edict_t *self, edict_t *targ, edict_t *inflictor);
void utl_radius_damage(edict_t *inflictor, edict_t *attacker, const s16 damage, edict_t *ignore);
void utl_aim(edict_t *self, x16vec3_t *result);
qboolean utl_heal(edict_t *e, const s16 amount, const qboolean ignore_max);
edict_t *utl_launch_spike(edict_t *self, const x32vec3_t *org, const x16vec3_t *angles, const x16vec3_t *dir, const s16 speed);
edict_t *utl_launch_grenade(edict_t *self, const x16vec3_t *angles);

// monster initialization
void monster_set_state(edict_t *self, const s16 state);
void monster_loop_state(edict_t *self, const s16 state);
void monster_end_state(edict_t *self, const s16 state, const s16 next_state);
void walkmonster_start(edict_t *self, const monster_class_t *class);
void flymonster_start(edict_t *self, const monster_class_t *class);
void swimmonster_start(edict_t *self, const monster_class_t *class);

// monster physics and ai
void monster_death_use(edict_t *self);
qboolean ai_movestep(edict_t *ent, const x32vec3_t *move, const qboolean relink);
qboolean ai_walkmove(edict_t *ent, x16 yaw, const x32 dist);
void ai_stand(edict_t *self);
void ai_face(edict_t *self);
void ai_walk(edict_t *self, const x32 dist);
void ai_run(edict_t *self, const x32 dist);
void ai_forward(edict_t *self, const x32 dist);
void ai_back(edict_t *self, const x32 dist);
void ai_pain(edict_t *self, const x32 dist);
void ai_painforward(edict_t *self, const x32 dist);
void ai_charge(edict_t *self, const x32 dist);
void ai_checkrefire(edict_t *self, const s16 state);
void ai_foundtarget(edict_t *self);
void ai_attack_finished(edict_t *self, const x32 dt);
void ai_gib(edict_t *self);

// player functions
void player_pain(edict_t *self, edict_t *attacker, s16 damage);
void player_die(edict_t *self, edict_t *killer);

// doors and plats
void door_init(edict_t *self);

// status bar
void Sbar_IndicateDamage(const s16 damage);

// combat random
static inline x32 trace_random(void) {
  return 2 * (xrand32() - HALF);
}

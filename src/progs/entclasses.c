#include "entity.h"
#include "entclasses.h"

extern void spawn_air_bubbles(edict_t *self) { }
extern void spawn_ambient_comp_hum(edict_t *self) { }
extern void spawn_ambient_drip(edict_t *self) { }
extern void spawn_ambient_drone(edict_t *self) { }
extern void spawn_ambient_suck_wind(edict_t *self) { }
extern void spawn_ambient_swamp1(edict_t *self) { }
extern void spawn_ambient_swamp2(edict_t *self) { }
extern void spawn_event_lightning(edict_t *self) { }
extern void spawn_func_bossgate(edict_t *self) { }
extern void spawn_func_button(edict_t *self) { }
extern void spawn_func_door(edict_t *self) { }
extern void spawn_func_door_secret(edict_t *self) { }
extern void spawn_func_episodegate(edict_t *self) { }
extern void spawn_func_illusionary(edict_t *self) { }
extern void spawn_func_plat(edict_t *self) { }
extern void spawn_func_train(edict_t *self) { }
extern void spawn_func_wall(edict_t *self) { }
extern void spawn_info_intermission(edict_t *self) { }
extern void spawn_info_null(edict_t *self) { }
extern void spawn_info_player_coop(edict_t *self) { }
extern void spawn_info_player_deathmatch(edict_t *self) { }
extern void spawn_info_player_start(edict_t *self) { }
extern void spawn_info_player_start2(edict_t *self) { }
extern void spawn_info_teleport_destination(edict_t *self) { }
extern void spawn_item_armor1(edict_t *self) { }
extern void spawn_item_armor2(edict_t *self) { }
extern void spawn_item_armorInv(edict_t *self) { }
extern void spawn_item_artifact_envirosuit(edict_t *self) { }
extern void spawn_item_artifact_invisibility(edict_t *self) { }
extern void spawn_item_artifact_invulnerability(edict_t *self) { }
extern void spawn_item_artifact_super_damage(edict_t *self) { }
extern void spawn_item_cells(edict_t *self) { }
extern void spawn_item_health(edict_t *self) { }
extern void spawn_item_key1(edict_t *self) { }
extern void spawn_item_key2(edict_t *self) { }
extern void spawn_item_rockets(edict_t *self) { }
extern void spawn_item_shells(edict_t *self) { }
extern void spawn_item_sigil(edict_t *self) { }
extern void spawn_item_spikes(edict_t *self) { }
extern void spawn_item_weapon(edict_t *self) { }
extern void spawn_light(edict_t *self) { }
extern void spawn_light_flame_large_yellow(edict_t *self) { }
extern void spawn_light_flame_small_white(edict_t *self) { }
extern void spawn_light_flame_small_yellow(edict_t *self) { }
extern void spawn_light_fluoro(edict_t *self) { }
extern void spawn_light_fluorospark(edict_t *self) { }
extern void spawn_light_globe(edict_t *self) { }
extern void spawn_light_torch_small_walltorch(edict_t *self) { }
extern void spawn_misc_explobox(edict_t *self) { }
extern void spawn_misc_explobox2(edict_t *self) { }
extern void spawn_misc_fireball(edict_t *self) { }
extern void spawn_misc_teleporttrain(edict_t *self) { }
extern void spawn_monster_army(edict_t *self);
extern void spawn_monster_boss(edict_t *self) { }
extern void spawn_monster_demon1(edict_t *self) { }
extern void spawn_monster_dog(edict_t *self);
extern void spawn_monster_enforcer(edict_t *self) { }
extern void spawn_monster_fish(edict_t *self) { }
extern void spawn_monster_hell_knight(edict_t *self) { }
extern void spawn_monster_knight(edict_t *self) { }
extern void spawn_monster_ogre(edict_t *self) { }
extern void spawn_monster_oldone(edict_t *self) { }
extern void spawn_monster_shalrath(edict_t *self) { }
extern void spawn_monster_shambler(edict_t *self) { }
extern void spawn_monster_tarbaby(edict_t *self) { }
extern void spawn_monster_wizard(edict_t *self) { }
extern void spawn_monster_zombie(edict_t *self) { }
extern void spawn_path_corner(edict_t *self) { }
extern void spawn_trap_spikeshooter(edict_t *self) { }
extern void spawn_trigger_changelevel(edict_t *self) { }
extern void spawn_trigger_counter(edict_t *self) { }
extern void spawn_trigger_hurt(edict_t *self) { }
extern void spawn_trigger_monsterjump(edict_t *self) { }
extern void spawn_trigger_multiple(edict_t *self) { }
extern void spawn_trigger_once(edict_t *self) { }
extern void spawn_trigger_onlyregistered(edict_t *self) { }
extern void spawn_trigger_push(edict_t *self) { }
extern void spawn_trigger_relay(edict_t *self) { }
extern void spawn_trigger_secret(edict_t *self) { }
extern void spawn_trigger_setskill(edict_t *self) { }
extern void spawn_trigger_teleport(edict_t *self) { }
extern void spawn_weapon_grenadelauncher(edict_t *self) { }
extern void spawn_weapon_lightning(edict_t *self) { }
extern void spawn_weapon_nailgun(edict_t *self) { }
extern void spawn_weapon_rocketlauncher(edict_t *self) { }
extern void spawn_weapon_supernailgun(edict_t *self) { }
extern void spawn_weapon_supershotgun(edict_t *self) { }

think_fn_t ent_spawnfuncs[] = {
  [ENT_AIR_BUBBLES] = spawn_air_bubbles,
  [ENT_AMBIENT_COMP_HUM] = spawn_ambient_comp_hum,
  [ENT_AMBIENT_DRIP] = spawn_ambient_drip,
  [ENT_AMBIENT_DRONE] = spawn_ambient_drone,
  [ENT_AMBIENT_SUCK_WIND] = spawn_ambient_suck_wind,
  [ENT_AMBIENT_SWAMP1] = spawn_ambient_swamp1,
  [ENT_AMBIENT_SWAMP2] = spawn_ambient_swamp2,
  [ENT_EVENT_LIGHTNING] = spawn_event_lightning,
  [ENT_FUNC_BOSSGATE] = spawn_func_bossgate,
  [ENT_FUNC_BUTTON] = spawn_func_button,
  [ENT_FUNC_DOOR] = spawn_func_door,
  [ENT_FUNC_DOOR_SECRET] = spawn_func_door_secret,
  [ENT_FUNC_EPISODEGATE] = spawn_func_episodegate,
  [ENT_FUNC_ILLUSIONARY] = spawn_func_illusionary,
  [ENT_FUNC_PLAT] = spawn_func_plat,
  [ENT_FUNC_TRAIN] = spawn_func_train,
  [ENT_FUNC_WALL] = spawn_func_wall,
  [ENT_INFO_INTERMISSION] = spawn_info_intermission,
  [ENT_INFO_NULL] = spawn_info_null,
  [ENT_INFO_PLAYER_COOP] = spawn_info_player_coop,
  [ENT_INFO_PLAYER_DEATHMATCH] = spawn_info_player_deathmatch,
  [ENT_INFO_PLAYER_START] = spawn_info_player_start,
  [ENT_INFO_PLAYER_START2] = spawn_info_player_start2,
  [ENT_INFO_TELEPORT_DESTINATION] = spawn_info_teleport_destination,
  [ENT_ITEM_ARMOR1] = spawn_item_armor1,
  [ENT_ITEM_ARMOR2] = spawn_item_armor2,
  [ENT_ITEM_ARMORINV] = spawn_item_armorInv,
  [ENT_ITEM_ARTIFACT_ENVIROSUIT] = spawn_item_artifact_envirosuit,
  [ENT_ITEM_ARTIFACT_INVISIBILITY] = spawn_item_artifact_invisibility,
  [ENT_ITEM_ARTIFACT_INVULNERABILITY] = spawn_item_artifact_invulnerability,
  [ENT_ITEM_ARTIFACT_SUPER_DAMAGE] = spawn_item_artifact_super_damage,
  [ENT_ITEM_CELLS] = spawn_item_cells,
  [ENT_ITEM_HEALTH] = spawn_item_health,
  [ENT_ITEM_KEY1] = spawn_item_key1,
  [ENT_ITEM_KEY2] = spawn_item_key2,
  [ENT_ITEM_ROCKETS] = spawn_item_rockets,
  [ENT_ITEM_SHELLS] = spawn_item_shells,
  [ENT_ITEM_SIGIL] = spawn_item_sigil,
  [ENT_ITEM_SPIKES] = spawn_item_spikes,
  [ENT_ITEM_WEAPON] = spawn_item_weapon,
  [ENT_LIGHT] = spawn_light,
  [ENT_LIGHT_FLAME_LARGE_YELLOW] = spawn_light_flame_large_yellow,
  [ENT_LIGHT_FLAME_SMALL_WHITE] = spawn_light_flame_small_white,
  [ENT_LIGHT_FLAME_SMALL_YELLOW] = spawn_light_flame_small_yellow,
  [ENT_LIGHT_FLUORO] = spawn_light_fluoro,
  [ENT_LIGHT_FLUOROSPARK] = spawn_light_fluorospark,
  [ENT_LIGHT_GLOBE] = spawn_light_globe,
  [ENT_LIGHT_TORCH_SMALL_WALLTORCH] = spawn_light_torch_small_walltorch,
  [ENT_MISC_EXPLOBOX] = spawn_misc_explobox,
  [ENT_MISC_EXPLOBOX2] = spawn_misc_explobox2,
  [ENT_MISC_FIREBALL] = spawn_misc_fireball,
  [ENT_MISC_TELEPORTTRAIN] = spawn_misc_teleporttrain,
  [ENT_MONSTER_ARMY] = spawn_monster_army,
  [ENT_MONSTER_BOSS] = spawn_monster_boss,
  [ENT_MONSTER_DEMON1] = spawn_monster_demon1,
  [ENT_MONSTER_DOG] = spawn_monster_dog,
  [ENT_MONSTER_ENFORCER] = spawn_monster_enforcer,
  [ENT_MONSTER_FISH] = spawn_monster_fish,
  [ENT_MONSTER_HELL_KNIGHT] = spawn_monster_hell_knight,
  [ENT_MONSTER_KNIGHT] = spawn_monster_knight,
  [ENT_MONSTER_OGRE] = spawn_monster_ogre,
  [ENT_MONSTER_OLDONE] = spawn_monster_oldone,
  [ENT_MONSTER_SHALRATH] = spawn_monster_shalrath,
  [ENT_MONSTER_SHAMBLER] = spawn_monster_shambler,
  [ENT_MONSTER_TARBABY] = spawn_monster_tarbaby,
  [ENT_MONSTER_WIZARD] = spawn_monster_wizard,
  [ENT_MONSTER_ZOMBIE] = spawn_monster_zombie,
  [ENT_PATH_CORNER] = spawn_path_corner,
  [ENT_TRAP_SPIKESHOOTER] = spawn_trap_spikeshooter,
  [ENT_TRIGGER_CHANGELEVEL] = spawn_trigger_changelevel,
  [ENT_TRIGGER_COUNTER] = spawn_trigger_counter,
  [ENT_TRIGGER_HURT] = spawn_trigger_hurt,
  [ENT_TRIGGER_MONSTERJUMP] = spawn_trigger_monsterjump,
  [ENT_TRIGGER_MULTIPLE] = spawn_trigger_multiple,
  [ENT_TRIGGER_ONCE] = spawn_trigger_once,
  [ENT_TRIGGER_ONLYREGISTERED] = spawn_trigger_onlyregistered,
  [ENT_TRIGGER_PUSH] = spawn_trigger_push,
  [ENT_TRIGGER_RELAY] = spawn_trigger_relay,
  [ENT_TRIGGER_SECRET] = spawn_trigger_secret,
  [ENT_TRIGGER_SETSKILL] = spawn_trigger_setskill,
  [ENT_TRIGGER_TELEPORT] = spawn_trigger_teleport,
  [ENT_WEAPON_GRENADELAUNCHER] = spawn_weapon_grenadelauncher,
  [ENT_WEAPON_LIGHTNING] = spawn_weapon_lightning,
  [ENT_WEAPON_NAILGUN] = spawn_weapon_nailgun,
  [ENT_WEAPON_ROCKETLAUNCHER] = spawn_weapon_rocketlauncher,
  [ENT_WEAPON_SUPERNAILGUN] = spawn_weapon_supernailgun,
  [ENT_WEAPON_SUPERSHOTGUN] = spawn_weapon_supershotgun,
};

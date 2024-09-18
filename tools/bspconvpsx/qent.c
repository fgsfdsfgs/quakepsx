#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "util.h"
#include "qent.h"

// TODO: move these into an external file or some shit
const qentmap_t qentmap[] = {
  { 0x00, "worldspawn" },
  { 0x01, "player" },
  { 0x02, "air_bubbles" },
  { 0x03, "ambient_comp_hum" },
  { 0x04, "ambient_drip" },
  { 0x05, "ambient_drone" },
  { 0x06, "ambient_suck_wind" },
  { 0x07, "ambient_swamp1" },
  { 0x08, "ambient_swamp2" },
  { 0x09, "event_lightning" },
  { 0x0a, "func_bossgate" },
  { 0x0b, "func_button" },
  { 0x0c, "func_door" },
  { 0x0d, "func_door_secret" },
  { 0x0e, "func_episodegate" },
  { 0x0f, "func_illusionary" },
  { 0x10, "func_plat" },
  { 0x11, "func_train" },
  { 0x12, "func_wall" },
  { 0x13, "info_intermission" },
  { 0x14, "info_null" },
  { 0x15, "info_player_coop" },
  { 0x16, "info_player_deathmatch" },
  { 0x17, "info_player_start", { "progs/v_axe.mdl", "progs/v_shot.mdl" } },
  { 0x18, "info_player_start2" },
  { 0x19, "info_teleport_destination" },
  { 0x1a, "item_armor1", { "progs/armor.mdl" } },
  { 0x1b, "item_armor2", { "progs/armor.mdl" } },
  { 0x1c, "item_armorInv", { "progs/armor.mdl" } },
  { 0x1d, "item_artifact_envirosuit", { "progs/suit.mdl" } },
  { 0x1e, "item_artifact_invisibility", { "progs/invisibl.mdl" } },
  { 0x1f, "item_artifact_invulnerability", { "progs/invulner.mdl" } },
  { 0x20, "item_artifact_super_damage", { "progs/quaddama.mdl" } },
  { 0x21, "item_cells" },
  { 0x22, "item_health" },
  { 0x23, "item_key1" },
  { 0x24, "item_key2" },
  { 0x25, "item_rockets" },
  { 0x26, "item_shells" },
  { 0x27, "item_sigil" },
  { 0x28, "item_spikes" },
  { 0x29, "item_weapon" },
  { 0x2a, "light" },
  { 0x2b, "light_flame_large_yellow" },
  { 0x2c, "light_flame_small_white" },
  { 0x2d, "light_flame_small_yellow" },
  { 0x2e, "light_fluoro" },
  { 0x2f, "light_fluorospark" },
  { 0x30, "light_globe" },
  { 0x31, "light_torch_small_walltorch" },
  { 0x32, "misc_explobox" },
  { 0x33, "misc_explobox2" },
  { 0x34, "misc_fireball" },
  { 0x35, "misc_teleporttrain" },
  { 0x36, "monster_army", { "progs/soldier.mdl", "progs/backpack.mdl" } },
  { 0x37, "monster_boss" },
  { 0x38, "monster_demon1", { "progs/demon.mdl" } },
  { 0x39, "monster_dog", { "progs/dog.mdl" } },
  { 0x3a, "monster_enforcer" },
  { 0x3b, "monster_fish", { "progs/fish.mdl" } },
  { 0x3c, "monster_hell_knight", { "progs/knight.mdl" } },
  { 0x3d, "monster_knight", { "progs/knight.mdl" } }, 
  { 0x3e, "monster_ogre", { "progs/ogre.mdl", "progs/backpack.mdl", "progs/grenade.mdl" } },
  { 0x3f, "monster_oldone" },
  { 0x40, "monster_shalrath" },
  { 0x41, "monster_shambler", { "progs/shambler.mdl" } },
  { 0x42, "monster_tarbaby" },
  { 0x43, "monster_wizard", { "progs/wizard.mdl" } },
  { 0x44, "monster_zombie", { "progs/zombie.mdl" } },
  { 0x45, "path_corner" },
  { 0x46, "trap_spikeshooter" },
  { 0x47, "trigger_changelevel" },
  { 0x48, "trigger_counter" },
  { 0x49, "trigger_hurt" },
  { 0x4a, "trigger_monsterjump" },
  { 0x4b, "trigger_multiple" },
  { 0x4c, "trigger_once" },
  { 0x4d, "trigger_onlyregistered" },
  { 0x4e, "trigger_push" },
  { 0x4f, "trigger_relay" },
  { 0x50, "trigger_secret" },
  { 0x51, "trigger_setskill" },
  { 0x52, "trigger_teleport" },
  { 0x53, "weapon_grenadelauncher", { "progs/g_rock.mdl", "progs/v_rock.mdl" } },
  { 0x54, "weapon_lightning", { "progs/g_light.mdl", "progs/v_light.mdl" } },
  { 0x55, "weapon_nailgun", { "progs/g_nail.mdl", "progs/v_nail.mdl" } },
  { 0x56, "weapon_rocketlauncher", { "progs/g_rock2.mdl", "progs/v_rock2.mdl" } },
  { 0x57, "weapon_supernailgun", { "progs/g_nail2.mdl", "progs/v_nail2.mdl" } },
  { 0x58, "weapon_supershotgun", { "progs/g_shot.mdl", "progs/v_shot2.mdl" } },
};

const int num_qentmap = sizeof(qentmap) / sizeof(*qentmap);

int num_qents = 0;
qent_t qents[MAX_ENTITIES];

const qentmap_t *qentmap_find(const char *classname) {
  for (int i = 0; i < num_qentmap; ++i) {
    if (!strcmp(qentmap[i].classname, classname))
      return &qentmap[i];
  }
  return NULL;
}

const char *qent_parse(qent_t *ent, const char *data) {
  char key[MAX_TOKEN] = { 0 };
  char value[MAX_TOKEN] = { 0 };

  ent->numfields = 0;

  while (1) {
    // parse key
    data = com_parse(data, key);
    if (key[0] == '}')
      break;

    assert(data);

    // parse value
    data = com_parse(data, value);
    assert(value[0] != '}');
    assert(data);

    // save keyvalue
    assert(ent->numfields < MAX_ENT_FIELDS);
    const int keylen = strlen(key);
    const int valuelen = strlen(value);
    qentfield_t *field = calloc(1, sizeof(qentfield_t) + keylen + valuelen + 2);
    assert(field);
    field->keylen = keylen;
    field->valuelen = valuelen;
    field->key = field->strdata;
    field->value = field->strdata + keylen + 1;
    memcpy(field->key, key, keylen);
    memcpy(field->value, value, valuelen);
    ent->fields[ent->numfields++] = field;

    // save pointer to classname if that's what it is
    if (!strcmp(key, "classname"))
      ent->classname = field->value;
  }

  assert(ent->classname);

  ent->info = qentmap_find(ent->classname);

  assert(ent->info);

  return data;
}

void qent_load(const char *data, int datalen) {
  char token[MAX_TOKEN] = { 0 };
  const char *end = data + datalen;
  qent_t *ent = NULL;

  while (1) {
    // parse opening bracket
    data = com_parse(data, token);
    if (!data || data >= end)
      break;
    assert(token[0] == '{');

    // parse entity block
    assert(num_qents < MAX_ENTITIES);
    ent = &qents[num_qents++];
    data = qent_parse(ent, data);
  }
}

const char *qent_get_string(qent_t *ent, const char *key) {
  for (int i = 0; i < ent->numfields; ++i) {
    if (!strcmp(ent->fields[i]->key, key))
      return ent->fields[i]->value;
  }
  return NULL;
}

const char *qent_get_int(qent_t *ent, const char *key, int *out) {
  const char *value = qent_get_string(ent, key);
  if (!value) return NULL;
  *out = atoi(value);
  return value;
}

const char *qent_get_float(qent_t *ent, const char *key, float *out) {
  const char *value = qent_get_string(ent, key);
  if (!value) return NULL;
  *out = atof(value);
  return value;
}

const char *qent_get_vector(qent_t *ent, const char *key, qvec3_t out) {
  const char *value = qent_get_string(ent, key);
  if (!value) return NULL;
  sscanf(value, "%f %f %f", &out[0], &out[1], &out[2]);
  return value;
}

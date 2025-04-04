#include "prcommon.h"

const weapdesc_t weap_table[WEAP_COUNT] = {
  { "Axe",                      IT_AXE,              MDLID_V_AXE,   -1,           SFXID_WEAPONS_AX1      },
  { "Shotgun",                  IT_SHOTGUN,          MDLID_V_SHOT,  AMMO_SHELLS,  SFXID_WEAPONS_GUNCOCK  },
  { "Double-barrelled Shotgun", IT_SUPER_SHOTGUN,    MDLID_V_SHOT2, AMMO_SHELLS,  SFXID_WEAPONS_SHOTGN2  },
  { "Nailgun",                  IT_NAILGUN,          MDLID_V_NAIL,  AMMO_NAILS,   SFXID_WEAPONS_ROCKET1I },
  { "Super Nailgun",            IT_SUPER_NAILGUN,    MDLID_V_NAIL2, AMMO_NAILS,   SFXID_WEAPONS_SPIKE2   },
  { "Grenade Launcher",         IT_GRENADE_LAUNCHER, MDLID_V_ROCK,  AMMO_ROCKETS, SFXID_WEAPONS_GRENADE  },
  { "Rocket Launcher",          IT_ROCKET_LAUNCHER,  MDLID_V_ROCK2, AMMO_ROCKETS, SFXID_WEAPONS_SGUN1    },
  { "Thunderbolt",              IT_LIGHTNING,        MDLID_V_LIGHT, AMMO_CELLS,   SFXID_WEAPONS_LSTART   },
};

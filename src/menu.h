#pragma once

#include "common.h"

typedef struct menuoption_s menuoption_t;
typedef struct menu_s menu_t;
typedef void (*menu_fn_t)(menuoption_t *self);

enum optiontype_e {
  OPT_LABEL,
  OPT_BUTTON,
  OPT_CHOICE,
  OPT_SLIDER
};

struct menuoption_s {
  u32 type;
  const char *title;
  void *value;
  menu_fn_t callback;
  union {
    struct {
      menu_t *nextmenu;
    } button;
    struct {
      const char **choices;
      s32 numchoices;
    } choice;
    struct {
      s32 min;
      s32 max;
      s32 step;
    } slider;
    struct {
      const char *path;
      const char *init;
    } file;
  };
};

struct menu_s {
  const char *title;
  menuoption_t *options;
  s32 numoptions;
  s32 selection;
  struct menu_s *prev;
};

void Menu_Toggle(void);
void Menu_Close(void);
void Menu_Draw(void);
void Menu_Update(void);

qboolean Menu_IsOpen(void);

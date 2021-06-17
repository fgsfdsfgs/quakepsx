#pragma once

void SCR_Init(void);
void SCR_UpdateScreen(void);

void SCR_SizeUp(void);
void SCR_SizeDown(void);

void SCR_CenterPrint(const char *str);
int SCR_ModalMessage(const char *text);

void SCR_BeginLoadingPlaque(void);
void SCR_EndLoadingPlaque(void);

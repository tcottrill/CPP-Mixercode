#pragma once


#include "framework.h"

extern int Font_Init(int sizept);
extern int GetCharFontWidth(const char cCharacter);
extern void StartTextMode(void);
void Font_Print(int x, int y, const char * string, ...);
void EndTextMode(void);
void KillFont();





#ifndef INI_H
#define INI_H

// C style naming conventions to be compatible with old allegro code
// NOT unicode compatible
#include <stdbool.h>

void set_config_file(char* szFileName);

int get_config_int(char* szSection, char* szKey, int iDefaultValue);
float get_config_float(char* szSection, char* szKey, float fltDefaultValue);
bool get_config_bool(char* szSection, char* szKey, bool bolDefaultValue);
char* get_config_string(char* szSection, char* szKey, const char* szDefaultValue);

void set_config_int(char* szSection, char* szKey, int iValue);
void set_config_float(char* szSection, char* szKey, float fltValue);
void set_config_bool(char* szSection, char* szKey, bool bolValue);
void set_config_string(char* szSection, char* szKey, char* szValue);


#endif//INI_H
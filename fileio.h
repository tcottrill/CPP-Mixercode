#pragma once

#include "log.h"
#include <string>

#ifndef FILEIO_H
#define FILEIO_H


//Get Current Directory
std::wstring getCurrentDirectoryW();
std::string getCurrentDirectory();

//Get information on last file operation
int get_last_file_size();

//Get Information on last Compression Operation
size_t get_last_zip_file_size();

//Load\Save File
unsigned char* load_file(const char *filename);
int save_file(const char *filename, unsigned char *buf, int size);

//Load\Save Zip
unsigned char* load_generic_zip(const char *archname, const char *filename);
bool saveGenericZip(const char* archname, const char* filename, unsigned char* data);

//Replace File Extension
void replaceExtension(std::string& str, std::string rep);


#endif
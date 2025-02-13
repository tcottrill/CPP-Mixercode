#include "fileio.h"
#include "stdlib.h"
#include "stdio.h"
#include "miniz.h"
#include <stdbool.h> 
#include <string.h>

// Basic file and zip handling.

long filesz = 0;
size_t uncomp_size = 0;

#pragma warning ( disable:4996 )

int get_last_file_size()
{
	return filesz;
}

size_t get_last_zip_file_size()
{
	return  uncomp_size;
}

static int getFileSize(FILE *input)
{
	int fileSizeBytes = 0;

	fseek(input, 0, SEEK_END);
	fileSizeBytes = ftell(input);
	fseek(input, 0, SEEK_SET);
	return fileSizeBytes;
}

unsigned char* load_file(const char *filename)
{
	unsigned char *buf = nullptr;

	FILE *fd = fopen(filename, "rb");
	if (!fd)
	{
		wrlog("Filed to find file! %s", filename);
		return (0);
	}

	filesz = getFileSize(fd);
	buf = (unsigned char *)malloc(filesz);
	fread(buf, 1, filesz, fd);
	fclose(fd);
	return buf;
}

int save_file(const char *filename, unsigned char *buf, int size)
{
	FILE *fd = fopen(filename, "wb");
	if (!fd) { wrlog("Filed to save file %s.", filename); return 0; }

	fwrite(buf, size, 1, fd);
	fclose(fd);
	return 1;
}

// ToDo: Add a debug clause in front of the logging to disable it
unsigned char* load_generic_zip(const char *archname, const char *filename)
{
	mz_bool status;
	mz_uint file_index = -1;
	mz_zip_archive zip_archive;
	mz_zip_archive_file_stat file_stat;

	unsigned char *buf = NULL;
	int ret = 1; // Zero Means the file didn't load, 1 Means everything is hunky dory. We start with one. 

	wrlog("Opening Archive %s", archname);
	// Now try to open the archive.
	memset(&zip_archive, 0, sizeof(zip_archive));
	status = mz_zip_reader_init_file(&zip_archive, archname, 0);
	if (!status) { wrlog("Zip Archive %s not found. (Check your path?)", archname); ret = 0; goto end; }

	// Find the requested file, ignore case 
	file_index = mz_zip_reader_locate_file(&zip_archive, filename, 0, 0);
	if (file_index == -1) { wrlog("Error: File %s not found in Zip Archive %s", filename, archname); ret = 0; goto end; }

	// Get information on the current file
	status = mz_zip_reader_file_stat(&zip_archive, file_index, &file_stat);
	if (status != MZ_TRUE) { wrlog("Error reading Zip File Info, it's probably corrupt"); ret = 0; goto end; }

	//Fill in the size in case we need to get it later
	uncomp_size = (size_t)file_stat.m_uncomp_size;

	//Create the unsigned char buffer
	buf = (unsigned char *)malloc(uncomp_size);
	if (!buf) { wrlog("Failed to create char buffer, mem error?"); ret = 0; goto end; }

	// Read (decompress) the file 
	status = mz_zip_reader_extract_to_mem(&zip_archive, file_index, buf, uncomp_size, 0);
	if (status != MZ_TRUE) { wrlog("Failed to extract zip file to mem for some weird reason"); ret = 0; goto end; }

end:
	// Close the archive 
	wrlog("Closing Archive");
	mz_zip_reader_end(&zip_archive);

	if (ret) { wrlog("Zip file loaded Successfully: %d from archive %s", filename, archname); }
	else { wrlog("Zip file %d in archive %s failed to load!", filename, archname); return 0; }

	return buf;
}

// ToDo: Add a debug clause in front of the logging to disable it
bool saveGenericZip(const char *archname, const char *filename, unsigned char *data)
{
	bool status;

	status = mz_zip_add_mem_to_archive_file_in_place(archname, filename, data, strlen((char *)data) + 1, 0, 0, MZ_BEST_COMPRESSION);
	if (!status)
	{
		wrlog("mz_zip_add_mem_to_archive_file_in_place failed!");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}


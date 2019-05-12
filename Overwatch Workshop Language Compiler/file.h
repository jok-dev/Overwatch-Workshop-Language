#pragma once

#include "general.h"

struct file_data {
	void* data;
	u32 length;
};

file_data* file_load(char* path);
void file_destroy(file_data* file);

char* file_get_extension(char* path);
u32 file_get_length_without_extension(char* path);
u32 file_get_filename_start(char* path);
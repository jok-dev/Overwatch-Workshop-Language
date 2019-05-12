#include "file.h"

#include <stdio.h>

file_data* file_load(char* path)
{
	FILE* file;
	fopen_s(&file, path, "r");

	if(!file) return NULL;

	fseek(file, 0, SEEK_END);
	u32 file_size = ftell(file);
	fseek(file, 0, SEEK_SET);

	// pack the struct and file data into one memory allocation
	file_data* data = (file_data*) malloc(sizeof(file_data) + file_size + 1);

	// the data pointer is after the struct data
	void* file_buffer = (void*) ((char*) data + sizeof(file_data));
	data->length = file_size;
	data->data = file_buffer;

	fread_s(file_buffer, file_size, file_size, 1, file);

	((char*) data->data)[file_size - 2] = 0;

	fclose(file);

	return data;
}

void file_destroy(file_data* file)
{
	free(file);
}

char* file_get_extension(char* path)
{
	u32 last_dot_loc = -1;

	for(u32 i = 0; true; i++)
	{
		char c = path[i];

		if(c == 0) break;

		if(c == '.') last_dot_loc = i;
	}

	if(last_dot_loc >= 0) return path + last_dot_loc;

	return NULL;
}

u32 get_last_character_in_string(char* str, char chr)
{
	u32 last = 0;

	u32 i = 0;
	for(;; i++)
	{
		char c = str[i];

		if(c == 0) break;

		if(c == chr) last = i;
	}

	return last;
}

u32 file_get_filename_start(char* path)
{
	return get_last_character_in_string(path, '/') + 1;
}

u32 file_get_length_without_extension(char* path)
{
	return get_last_character_in_string(path, '.');
}
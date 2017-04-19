#ifndef FILE_H
#define FILE_H

typedef struct File File;

struct File{
	char file_name[20];
	time_t time_created;
	time_t time_accessed;
	time_t time_modified;
	time_t user_name;
};

typedef struct User User;

struct User{
	char user_name[20];
	char password[20];
	time_t last_login;
	int num_files;
	File *file_list[5];
};

int num_user;
int num_file;

#endif
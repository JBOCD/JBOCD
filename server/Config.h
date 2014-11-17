#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <json/json.h>

#ifndef CONFIG_H
#define CONFIG_H

#define CONFIG_MAX_KEY 20

class Config{
	protected:
		json_object *root, *nul;

	private:
		char tmpStr[CONFIG_MAX_KEY+1]; //config file key name should not exceed 20 char
	public:
		Config();
		json_object *getFrom(char *path, json_object *curRoot);
		json_object *get(char *path);
};

#include "Config.cpp"

#endif
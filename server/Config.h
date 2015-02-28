#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <json/json.h>

#ifndef CONFIG_H
#define CONFIG_H

#define CONFIG_MAX_KEY 20

class Config{
	protected:
		static json_object *root, *nul;

	private:
		static char *tmpStr; //config file key name should not exceed 20 char
	public:
		static void init();
		static json_object *getFrom(const char *path, json_object *curRoot);
		static json_object *get(const char *path);
};

json_object *Config::root = NULL;
json_object *Config::nul = NULL;
char *Config::tmpStr = (char*) malloc(CONFIG_MAX_KEY+1);

#include "Config.cpp"

#endif
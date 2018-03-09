#include <glib-2.0/glib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <iostream>
using namespace std;
#include "confoperation.h"

int GetPrivateProfileInt(const char *profile, const char *section, const char *key)
{
	GKeyFile *keyfile;
	GKeyFileFlags flags;
	GError *error = NULL;
	gsize length;
	int ret = 0;
	keyfile = g_key_file_new ();
	flags = (GKeyFileFlags)(G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS);
	if (!g_key_file_load_from_file (keyfile, profile, flags, &error))
	{
		g_error (error->message);
		g_key_file_free(keyfile);
		return ret;
	}
	ret = g_key_file_get_integer(keyfile, section,  
		key, &error);
	g_key_file_free(keyfile);
	return ret;
}

char *GetPrivateProfileString(const char *profile, const char *section, const char *key)
{
	GKeyFile *keyfile;
	GKeyFileFlags flags;
	GError *error = NULL;
	gsize length;
	int ret = 0;
	keyfile = g_key_file_new ();
	flags = (GKeyFileFlags)(G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS);
	if (!g_key_file_load_from_file (keyfile, profile, flags, &error))
	{
		g_error (error->message);
		g_key_file_free(keyfile);
		return NULL;
	}
	gchar *value =  g_key_file_get_string(keyfile, section,  
		key, &error);
	g_key_file_free(keyfile);
	return value;
}

int  SetPrivateProfileInt(const char *profile, const char *section, const char *key, int value)
{
	GKeyFile *keyfile;
	GKeyFileFlags flags;
	GError *error = NULL;
	gsize length;
	int ret = 0;
	gchar *content = NULL;
	FILE *fd = NULL;

	keyfile = g_key_file_new ();
	flags = (GKeyFileFlags)(G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS);
	if (!g_key_file_load_from_file (keyfile, profile, flags, &error))
	{
		g_error (error->message);
		ret =1;
		goto exit;
	}
	g_key_file_set_integer(keyfile, section,  
		key, value);

	content = g_key_file_to_data(keyfile, &length, &error);
	if(content == NULL)
	{
		g_error (error->message);
		ret =1;
		goto exit;
	}

	if(!g_file_set_contents(profile, content, length, &error))
	{
		g_error (error->message);
		ret = 1;
	}
	else
	{
		ret = 0;
	}
	g_free(content);
exit:
	g_key_file_free(keyfile);
	return ret;
}

int SetPrivateProfileString(const char *profile, const char *section, const char *key, char *value)
{
	GKeyFile *keyfile;
	GKeyFileFlags flags;
	GError *error = NULL;
	gsize length;
	int ret = 0;
	gchar *content = NULL;
	FILE *fd = NULL;

	keyfile = g_key_file_new ();
	flags = (GKeyFileFlags)(G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS);
	if (!g_key_file_load_from_file (keyfile, profile, flags, &error))
	{
		g_error (error->message);
		ret =1;
		goto exit;
	}
	g_key_file_set_string(keyfile, section,  
		key, value);

	content = g_key_file_to_data(keyfile, &length, &error);
	if(content == NULL)
	{
		g_error (error->message);
		ret =1;
		goto exit;
	}
	if(!g_file_set_contents(profile, content, length, &error))
	{
		g_error (error->message);
		ret =1;
	}
	else
	{
		ret = 0;
	}
	g_free(content);
	ret = 1;
exit:
	g_key_file_free(keyfile);
	return ret;
}

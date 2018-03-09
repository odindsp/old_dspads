#include <stdlib.h>
#include <string.h>
#include "json_util.h"

void jsonInsertString(json_t *entry, const char *label, const char *value)
{
	json_t *jLabel = json_new_string(label);
	json_t *jValue = json_new_string(value);

	json_insert_child(jLabel, jValue);
	json_insert_child(entry, jLabel);
}

void jsonInsertBool(json_t *entry, const char *label, bool value)
{
	json_t *jLabel, *jValue;

	jLabel = json_new_string(label);
	if(value)
	{
		jValue = json_new_true();
	}
	else
	{
		jValue = json_new_false();
	}
	json_insert_child(jLabel, jValue);
	json_insert_child(entry, jLabel);
}

void jsonInsertDouble(json_t *entry, const char *label, double value)
{
	json_t *jLabel, *jValue;
	char str[128];

	jLabel = json_new_string(label);
	sprintf(str, "%f", value);
	jValue = json_new_number(str);
	json_insert_child(jLabel, jValue);
	json_insert_child(entry, jLabel);
}

void jsonInsertInt(json_t *entry, const char *label, int value)
{
	json_t *jLabel, *jValue;
	char str[64];

	jLabel = json_new_string(label);
	sprintf(str, "%d", value);
	jValue = json_new_number(str);
	json_insert_child(jLabel, jValue);
	json_insert_child(entry, jLabel);
}

void jsonInsertInt64(json_t *entry, const char *label, long long value)
{
	json_t *jLabel, *jValue;
	char str[128];

	jLabel = json_new_string(label);
	sprintf(str, "%ld", value);
	jValue = json_new_number(str);
	json_insert_child(jLabel, jValue);
	json_insert_child(entry, jLabel);
}

void jsonGetIntegerArray(json_t *label, vector<int> &v)
{
	if (label == NULL || label->child == NULL || label->child->type != JSON_ARRAY)
		return;

	json_t *temp = label->child->child;

	while (temp != NULL)
	{
		if(temp->type == JSON_NUMBER)
			v.push_back(atoi(temp->text));
		temp = temp->next;
	}
}

void jsonGetStringArray(json_t *label, vector<string> &v)
{
	if (label == NULL || label->child == NULL || label->child->type != JSON_ARRAY)
		return;

	json_t *temp = label->child->child;

	while (temp != NULL)
	{
		if(temp->type == JSON_STRING && strlen(temp->text) > 0)
			v.push_back(temp->text);
		temp = temp->next;
	}
}

void jsonGetStringSet(json_t *label, set<string> &v)
{
	if (label == NULL || label->child == NULL || label->child->type != JSON_ARRAY)
		return;

	json_t *temp = label->child->child;

	while (temp != NULL )
	{
		if(temp->type == JSON_STRING && strlen(temp->text) > 0 )
			v.insert(temp->text);
		temp = temp->next;
	}
}


void jsonInsertFloat(json_t *entry, const char *label, double value, uint8_t precision)
{
	json_t *jLabel, *jValue;
	char str[64];

	jLabel = json_new_string(label);
	if (precision <= 2)
	{
		sprintf(str, "%.2lf", value);
	}
	else
	{
		char format[64];

		sprintf(format, "%%.%dlf", precision);

		sprintf(str, format, value);
	}
	
	jValue = json_new_number(str);
	json_insert_child(jLabel, jValue);
	json_insert_child(entry, jLabel);
}

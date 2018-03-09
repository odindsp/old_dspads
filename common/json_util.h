#ifndef JSON_UTIL_H_
#define JSON_UTIL_H_

#include <vector>
#include <string>
#include <set>
#include "json.h"
using namespace std;

#define JSON_CHILD_TYPE_CHECK(label, child_type) (label->child->type == child_type) 
#define JSON_FIND_LABEL(root, label, labelname)  (label = json_find_first_label(root, labelname))
#define JSON_FIND_LABEL_AND_CHECK(root, label, labelname, child_type) (((JSON_FIND_LABEL(root, label, labelname)) != NULL) && JSON_CHILD_TYPE_CHECK(label, child_type))

#define JSON_TYPE_CHECK(label, child_type) (label->type == child_type) 

void jsonInsertString(json_t *entry, const char *label, const char *value);
void jsonInsertInt(json_t *entry, const char *label, int value);
void jsonInsertInt64(json_t *entry, const char *label, long long value);
void jsonInsertBool(json_t *entry, const char *label, bool value);
void jsonInsertDouble(json_t *entry, const char *label, double value);
void jsonGetIntegerArray(json_t *label, vector<int> &v);
void jsonGetStringArray(json_t *label, vector<string> &v);
void jsonGetStringSet(json_t *label, set<string> &v);
void jsonInsertFloat(json_t *entry, const char *label, double value, uint8_t precision = 2);

#endif	/* JSON_UTIL_H_ */

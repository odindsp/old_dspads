#include "type.h"
#include "util.h"

uint64_t openIPB(char *fname);
void closeIPB(uint64_t addr);
int getRegionCode(uint64_t addr, string &IPstr);

#include "hex.h"

int hex2num(unsigned char c)
{
    if (c>='0' && c<='9') return c - '0';
    if (c>='a' && c<='z') return c - 'a' + 10;
    if (c>='A' && c<='Z') return c - 'A' + 10;
    
    return '0';
}

char* hex(const char* str,int *n)
{
    if (str == NULL) 
	{
        return NULL;
    }
	
	char *result = NULL;
	unsigned char a,b;
    int i;
	unsigned int strSize;

    strSize = strlen(str);
    if (strSize % 2 == 1 || strSize == 0){
        goto exit;
    }
	result = malloc(strlen(str)/2 + 1);
	if (result == NULL)
		goto exit;

    for ( i=0; (i<strSize/2); i++) {
        a = hex2num(str[i*2]);
        b = hex2num(str[i*2+1]);
        result[i] = (a << 4) | b;
    }
    *n = strSize/2;
    result[i]='\0';
 
exit:  
	return result;
}

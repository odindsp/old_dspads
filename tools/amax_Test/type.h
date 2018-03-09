#ifndef TYPE
#define TYPE

#define ERROR_STRING_URL "输入的URL格式有错误，请检查URL格式是否正确！"
#define ERROR_STRING_JSON "输入的JSON格式有错误，请检查JSON格式是否正确！"
#define EMPTY_STRING_URL "发送到的URL不能为空，请输入正确的URL地址！"
#define EMPTY_STRING_JSON "需要发送的JSON请求不能为空，请输入正确的JSON！"

#define HEADER_JSON "application/json"
#define IFREE(p)  if(p != NULL) { delete p;  p = NULL; }
#define MAX_LENGTH 10000

#endif // TYPE


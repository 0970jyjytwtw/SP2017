
#include<stdint.h>
#define LEFTROTATE(x, c) (((x) << (c)) | ((x) >> (32 - (c))))
void makemd5(uint8_t a1[],uint8_t *initial_msg,uint32_t initial_len);


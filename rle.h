// rle.h

#ifndef _RLE_H_
#define _RLE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


extern int rle_enc(void *data, int len, void *buf);
extern int rle_dec(void *data, int len, void *buf);


#ifdef __cplusplus
}
#endif

#endif // _RLE_H_

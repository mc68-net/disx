// rle.c

#include "rle.h"

#include <string.h>


// RLE (Run-Length Encoding) is used to reduce the size of the
// mostly unchanging values of the attr and type blocks.
//
// RLE encoding algorithm
//
// 0xxx xxxx bb... =  1 + 0-127 random bytes
// 10xx xxxx    bb =  2 + 0-63 identical bytes
// 110x xxxx    bb = 66 + 0-31 identical bytes
// 111x xxxx nn bb = 98 + 0-8191 identical bytes (nn = low byte of length)


// =====================================================
// encode RLE, returns size of encoded data
//      source data is in 'data' with size 'len'
//      destination data is in 'buf' with size in return value
//      if buf is NULL, size is returned without writing anything
int rle_enc(void *data, int len, void *buf)
{
    uint8_t *s = (uint8_t*)data;// source data
    uint8_t *e = data + len;    // end of source data
    uint8_t *d = (uint8_t*)buf; // destination data
    int size = 0;               // size of destination data

    while (s < e) {
        if (s == e - 1 || s[0] != s[1]) {
            // find a run of random data
            uint8_t* p = s + 1; // pointer into run
            int n = 1;          // length of run

            // search until p == first byte after run
            while (p < e && n < 128) {
                // If current byte is same as previous byte,
                // check the next byte. If next byte is also same,
                // there is a run of at least 3 identical bytes.
                // If there are only two identical bytes, it is
                // more efficient to not break the random run.

                // Mote that if there are 3 identical bytes after
                // 127 random bytes, the first identical byte will
                // become part of the random run. This is okay
                // because it should not make the encoded data larger.

                if (p[0] == p[-1] && (p+1 < e && p[0] == p[1])) {
                    // back off of the first identical byte
                    p--;
                    n--;
                    break;
                }
                p++;
                n++;
            }

            // 0xxx xxxx bb... =  1 + 0-127 random bytes
            if (buf) {
                // copy encoded bytes to destination buffer
                *d++ = n - 1;
                memcpy(d, s, n);
                d += n;         // move up destination pointer
            }
            s = p; // look for next run at p
            size += n + 1;      // add to generated length
        } else {
            // find a run of identical data

            uint8_t* p = s + 1; // pointer into run
            int n = 1;          // length of run

            // search until p == first byte after run
            while (p < e && n < 8192 && p[0] == p[-1]) {
                p++;
                n++;
            }

            if (n < 98) {
                // 10xx xxxx bb =  2 + 0-63 identical bytes
                // 110x xxxx bb = 66 + 0-31 identical bytes

                if (buf) {
                    // copy encoded bytes to destination buffer
                    *d++ = (n - 2) | 0x80;
                    *d++ = *s;
                }
                s = p; // look for next run at p
                size += 2;      // add to generated length
            } else {
                // 111x xxxx nn bb = 98 + 0-8191 identical bytes

                if (buf) {
                    // copy encoded bytes to destination buffer
                    *d++ = ((n - 98) >> 8) | 0xE0;
                    *d++ =  (n - 98) & 0xFF;
                    *d++ = *s;
                }
                s = p; // look for next run at p
                size += 3;      // add to generated length
            }
        }
    }

    return size;
}


// =====================================================
// decode RLE, returns size of decoded data
//      source data is in 'data' with size 'len'
//      destination data is in 'buf' with size in return value
//      if buf is NULL, size is returned without writing anything
int rle_dec(void *data, int len, void *buf)
{
    uint8_t *s = (uint8_t*)data;// source data
    uint8_t *e = data + len;    // end of source data
    uint8_t *d = (uint8_t*)buf; // destination data
    int size = 0;               // size of destination data

    while (s < e) {
        int n = *s++;           // length of run

        if (n < 0x80) {         // 0xxx xxxx bb...
            n = n + 1;
            if (buf) {
                // block copy n bytes
                memcpy(d, s, n);
                d += n;
            }
            s += n;
        } else {
            if (n < 0xE0) {     // 10xx xxxx bb and 110x xxxx bb
                n = (n & 0x7F) + 2;
            } else {            // 111x xxxx nn bb
                n = (n & 0x1F) << 8;
                n += *s++ + 98;
            }
            int b = *s++;
            if (buf) {
                // fill n bytes of bb
                memset(d, b, n);
                d += n;
            }
        }
        size += n;
    }

    return size;
}


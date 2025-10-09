#pragma once

#define DIGEST_SIZE (512/8)

#ifdef __cplusplus
extern "C" {
#endif

void sha512(unsigned char* inp, int len, unsigned char* digest);

#ifdef __cplusplus
}
#endif

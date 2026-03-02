#pragma once

#define DIGEST_SIZE (512/8)

#ifdef __cplusplus
extern "C" {
#endif

void sha512(unsigned char* inp, int len, unsigned char* digest);

void* sha512_init();
void sha512_update(void *state_ptr, unsigned char* inp, int len);
void sha512_finalize(void *state_ptr, unsigned char* digest);

#ifdef __cplusplus
}
#endif

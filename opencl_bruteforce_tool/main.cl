#include "inc_vendor.h"
#include "inc_types.h"
#include "inc_platform.h"
#include "inc_common.h"
#include "inc_hash_md5.h"
#include "inc_hash_sha512.h"
// #include "inc_hash_sha256.h"
#include "main.cl.h"


__kernel void simple_md5(__global uint* input,
                        __global uint* in_size,
                        __global uint* output,
                        __global uint* out_size)
{
    const uint idx = get_global_id(0);
    uint size = in_size[0];

    md5_ctx_t ctx;
    md5_init(&ctx);
    md5_update(&ctx, input + idx*BUFFER_SIZE, size);
    md5_final(&ctx);
 
    for (int i = 0; i < 4; i++) {
        output[idx * BUFFER_SIZE + i] = ctx.h[i];
    }
    out_size[0] = MD5_HASH_LEN;
}

__kernel void simple_xor(__global uint* input1,
                        __global uint* in1_size,
                        __global uint* input2,
                        __global uint* in2_size,
                        __global uint* output,
                        __global uint* out_size)
{
    const uint idx = get_global_id(0);
    uint size = max(in1_size[0], in2_size[0]);
    out_size[0] = size;
    size = size/4;

    for (int i = 0; i < size; i++) {
        output[idx * BUFFER_SIZE + i] = input1[idx * BUFFER_SIZE + i] ^ input2[idx * BUFFER_SIZE + i];
    }
}

__kernel void simple_sum4(__global uint* input1,
                        __global uint* in1_size,
                        __global uint* input2,
                        __global uint* in2_size,
                        __global uint* output,
                        __global uint* out_size)
{
    const uint idx = get_global_id(0);
    uint size = max(in1_size[0], in2_size[0]);
    out_size[0] = size;
    size = size/4;

    for (int i = 0; i < size; i++) {
        output[idx * BUFFER_SIZE + i] = input1[idx * BUFFER_SIZE + i] + input2[idx * BUFFER_SIZE + i];
    }
}

__kernel void simple_not(__global uint* input1,
                        __global uint* in1_size,
                        __global uint* output,
                        __global uint* out_size)
{
    const uint idx = get_global_id(0);
    uint size = in1_size[0];
    out_size[0] = size;
    size = size/4;

    for (int i = 0; i < size; i++) {
        output[idx * BUFFER_SIZE + i] = input1[idx * BUFFER_SIZE + i] ^ 0xffffffff;
    }
}

__kernel void simple_sha512(__global uint* input,
                        __global uint* in_size,
                        __global uint* output,
                        __global uint* out_size)
{
    const uint idx = get_global_id(0);
    uint size = in_size[0];

    sha512_ctx_t ctx;
    sha512_init(&ctx);
    sha512_update_swap(&ctx, input + idx*BUFFER_SIZE, size);
    sha512_final(&ctx);
    for (int i = 0; i < 8; i++) {
        ullong out = ctx.h[i];
        uint h1 = out&0xffffffff;
        uint h2 = out >> 32;

        h1 = hc_swap32(h1);
        h2 = hc_swap32(h2);

        output[idx * BUFFER_SIZE + i*2 + 0] = h2;
        output[idx * BUFFER_SIZE + i*2 + 1] = h1;
    }
    out_size[0] = SHA512_HASH_LEN;
}

// __kernel void simple_sha256(__global const uint* input,
//                         __global uint* output)
// {
//     const uint idx = get_global_id(0);
//     sha256_ctx_t ctx;
//     sha256_init(&ctx);
//     sha256_update_swap(&ctx, input + idx*BUFFER_SIZE, HASH_BYTE_SIZE);
//     sha256_final(&ctx);
 
//     for (int i = 0; i < 8; i++) {
//         output[idx * BUFFER_SIZE + i] = hc_swap32(ctx.h[i]);
//     }
   
// }
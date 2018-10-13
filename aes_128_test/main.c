/* Copyright 2018 Canaan Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <string.h>
#include "aes.h"
#include "aes_cbc.h"
#include "gcm.h"
#include "sysctl.h"
#include "encoding.h"

//#define AES_DBUG
#ifdef AES_DBUG
#define AES_DBG(fmt, args...) printf(fmt, ##args)
#else
#define AES_DBG(fmt, args...)
#endif

#define CIPHER_MAX 3
#define AES_TEST_DATA_LEN  1024

typedef enum _check_result
{
    AES_CHECK_PASS = 0,
    AES_CHECK_FAIL = 1,
    AES_CHECK_MAX,
} check_result_t;

char *cipher_name[CIPHER_MAX] =
{
    "aes-ecb-128",
    "aes-cbc-128",
    "aes-gcm-128",
};

uint8_t aes_key[] = {0xfe, 0xff, 0xe9, 0x92, 0x86, 0x65, 0x73, 0x1c, 0x6d, 0x6a, 0x8f, 0x94, 0x67, 0x30, 0x83, 0x08};
size_t key_length = AES_128;

uint8_t aes_iv[] = {0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce, 0xdb, 0xad, 0xde, 0xca, 0xf8, 0x88, 0x67, 0x30, 0x83, 0x08};
uint8_t iv_length = 16;
uint8_t iv_gcm_length = 12;

uint8_t aes_aad[] = {0xfe, 0xed, 0xfa, 0xce, 0xde, 0xad, 0xbe, 0xef, 0xfe, 0xed, 0xfa, 0xce, 0xde, 0xad, 0xbe, 0xef,
                    0xab, 0xad, 0xda, 0xd2};
uint8_t aad_length = 20;

uint8_t gcm_soft_tag[16];
uint8_t gcm_hard_tag[16];
uint8_t aes_hard_in_data[AES_TEST_DATA_LEN];
uint8_t aes_soft_in_data[AES_TEST_DATA_LEN];
uint8_t aes_hard_out_data[AES_TEST_DATA_LEN];
uint8_t aes_soft_out_data[AES_TEST_DATA_LEN];
uint64_t cycle[CIPHER_MAX][2];
uint8_t get_time_flag;

check_result_t aes_check(uint8_t* key_addr,
    size_t key_length,
    uint8_t* gcm_iv,
    size_t iv_length,
    uint8_t* aes_aad,
    size_t aad_size,
    aes_cipher_mode_t cipher_mod,
    uint8_t* aes_data,
    size_t data_size)
{
    uint32_t i, temp_size;
    uint8_t total_check_tag = 0;
    cbc_context_t cbc_context;
    gcm_context_t gcm_context;
    cbc_context.input_key = key_addr;
    cbc_context.iv = gcm_iv;
    gcm_context.gcm_aad = aes_aad;
    gcm_context.gcm_aad_len = aad_size;
    gcm_context.input_key = key_addr;
    gcm_context.iv = gcm_iv;

    if (cipher_mod == AES_CBC)
    {
        if(get_time_flag)
            cycle[AES_CBC][0] = read_cycle();
        aes_cbc128_hard_encrypt(&cbc_context, aes_data, data_size, aes_hard_out_data);
        if(get_time_flag)
            cycle[AES_CBC][0] = read_cycle() - cycle[AES_CBC][0];
    }
    else if (cipher_mod == AES_ECB)
    {
        if(get_time_flag)
            cycle[AES_ECB][0] = read_cycle();
        aes_ecb128_hard_encrypt(key_addr, aes_data, data_size, aes_hard_out_data);
        if(get_time_flag)
            cycle[AES_ECB][0] = read_cycle() - cycle[AES_ECB][0];
    }
    else
    {
        if(get_time_flag)
            cycle[AES_GCM][0] = read_cycle();
        aes_gcm128_hard_encrypt(&gcm_context, aes_data, data_size, aes_hard_out_data, gcm_hard_tag);
        if(get_time_flag)
            cycle[AES_GCM][0] = read_cycle() - cycle[AES_GCM][0];
    }

    memset(aes_soft_in_data, 0, AES_TEST_DATA_LEN);
    memcpy(aes_soft_in_data, aes_data, data_size);
    memset(aes_soft_out_data, 0, AES_TEST_DATA_LEN);
    if (cipher_mod == AES_ECB)
    {
        if(get_time_flag)
            cycle[AES_ECB][1] = read_cycle();
        i = 0;
        if (data_size >= 16)
        {
            for (i = 0; i < (data_size / 16); i++)
                AES_ECB_encrypt(&aes_soft_in_data[i * 16], key_addr, &aes_soft_out_data[i * 16], 16);
        }
        temp_size = data_size % 16;
        if (temp_size)
            AES_ECB_encrypt(&aes_soft_in_data[i * 16], key_addr, &aes_soft_out_data[i * 16], temp_size);
        if(get_time_flag)
            cycle[AES_ECB][1] = read_cycle() - cycle[AES_ECB][1];
    }
    else if (cipher_mod == AES_CBC)
    {
        if(get_time_flag)
            cycle[AES_CBC][1] = read_cycle();
        AES_CBC_encrypt_buffer(aes_soft_out_data, aes_soft_in_data, data_size, key_addr, gcm_iv);
        if(get_time_flag)
            cycle[AES_CBC][1] = read_cycle() - cycle[AES_CBC][1];
    }
    else if (cipher_mod == AES_GCM)
    {
        if(get_time_flag)
            cycle[AES_GCM][1] = read_cycle();
        mbedtls_gcm_context ctx;
        mbedtls_cipher_id_t cipher = MBEDTLS_CIPHER_ID_AES;
        mbedtls_gcm_init(&ctx);
        // 128 bits, not bytes!
        mbedtls_gcm_setkey(&ctx, cipher, key_addr, key_length*8);
        mbedtls_gcm_crypt_and_tag(&ctx, MBEDTLS_GCM_ENCRYPT, data_size, gcm_iv, iv_length, aes_aad, aad_size, aes_data,
            aes_soft_out_data, 16, gcm_soft_tag);
        mbedtls_gcm_free(&ctx);
        if(get_time_flag)
            cycle[AES_GCM][1] = read_cycle() - cycle[AES_GCM][1];
    }
    else
    {
        printf("\n error cipher \n");
    }
    for (i = 0; i < data_size; i++)
    {
        if (aes_hard_out_data[i] != aes_soft_out_data[i])
        {
            AES_DBG("out_data[%d]:0x%02x  aes_soft_out_data:0x%02x\n", i, aes_hard_out_data[i], aes_soft_out_data[i]);
            total_check_tag = 1;
        }
    }
    if (total_check_tag == 1)
    {
        AES_DBG("\nciphertext error\n");
    }
    else
    {
        AES_DBG("ciphertext pass\n");
    }
    if (cipher_mod == AES_GCM)
    {
        total_check_tag = 0;
        for (i = 0; i < 16; i++)
        {
            if (gcm_soft_tag[i] != gcm_hard_tag[i])
            {
                AES_DBG("error tag : gcm_soft_tag:0x%02x    gcm_hard_tag:0x%02x\n", gcm_soft_tag[i], gcm_hard_tag[i]);
                total_check_tag = 1;
            }
        }
        if (total_check_tag == 1)
        {
            AES_DBG("tag error\n");
        }
        else
        {
            AES_DBG("tag OK\n");
        }
    }

    if (cipher_mod == AES_CBC)
        aes_cbc128_hard_decrypt(&cbc_context, aes_hard_out_data, data_size, aes_soft_out_data);
    else if (cipher_mod == AES_ECB)
        aes_ecb128_hard_decrypt(key_addr, aes_hard_out_data, data_size, aes_soft_out_data);
    else
        aes_gcm128_hard_decrypt(&gcm_context, aes_hard_out_data, data_size, aes_soft_out_data, gcm_hard_tag);
    total_check_tag = 0;
    for (i = 0; i < data_size; i++)
    {
        if (aes_data[i] != aes_soft_out_data[i])
        {
            AES_DBG("aes_data[%d]:0x%02x  aes_soft_out_data:0x%02x\n", i, aes_data[i], aes_soft_out_data[i]);
            total_check_tag = 1;
        }
    }
    if (total_check_tag == 1)
    {
        AES_DBG("\nplaintext error\n");
        return AES_CHECK_FAIL;
    }
    else
    {
        AES_DBG("plaintext OK\n");
        return AES_CHECK_PASS;
    }
}

check_result_t aes_check_all_byte(aes_cipher_mode_t cipher)
{
    uint32_t total_check_tag = 0;
    uint32_t index = 0;
    size_t data_length = 0;

    if (cipher == AES_GCM)
        iv_length = iv_gcm_length;
    for (index = 0; index < 256; index++)
    {
        aes_hard_in_data[index] = index;
        data_length++;
        AES_DBG("[%s] test num: %ld \n", cipher_name[cipher], data_length);
        if (aes_check(aes_key, key_length, aes_iv, iv_length, aes_aad, aad_length, cipher, aes_hard_in_data, data_length)
            == AES_CHECK_FAIL)
            total_check_tag = 1;
    }
    get_time_flag = 1;
    data_length = AES_TEST_DATA_LEN;
    AES_DBG("[%s] test num: %ld \n", cipher_name[cipher], data_length);
    for (index = 0; index < data_length; index++)
        aes_hard_in_data[index] = index % 256;
    if (aes_check(aes_key, key_length, aes_iv, iv_length, aes_aad, aad_length, cipher, aes_hard_in_data, data_length)
        == AES_CHECK_FAIL)
        total_check_tag = 1;
    get_time_flag = 0;
    if(total_check_tag)
        return AES_CHECK_FAIL;
    else
        return AES_CHECK_PASS;
}

check_result_t aes_check_all_key(aes_cipher_mode_t cipher)
{
    size_t data_length = 0;
    uint32_t index = 0;
    uint32_t i = 0;
    uint32_t total_check_tag = 0;

    if (cipher == AES_GCM)
        iv_length = iv_gcm_length;
    data_length = AES_TEST_DATA_LEN;
    for (index = 0; index < data_length; index++)
        aes_hard_in_data[index] = index;
    for (i = 0; i < (256 / key_length); i++)
    {
        for (index = i * key_length; index < (i * key_length) + key_length; index++)
            aes_key[index - (i * key_length)] = index;
        if (aes_check(aes_key, key_length, aes_iv, iv_length, aes_aad, aad_length, cipher, aes_hard_in_data, data_length)
            == AES_CHECK_FAIL)
            total_check_tag = 1;
    }
    if(total_check_tag)
        return AES_CHECK_FAIL;
    else
        return AES_CHECK_PASS;
}

check_result_t aes_check_all_iv(aes_cipher_mode_t cipher)
{
    size_t data_length = 0;
    uint32_t index = 0;
    uint32_t i = 0;
    uint8_t total_check_tag = 0;

    if (cipher == AES_GCM)
        iv_length = iv_gcm_length;
    data_length = AES_TEST_DATA_LEN;
    for (index = 0; index < data_length; index++)
        aes_hard_in_data[index] = index;
    for (i = 0; i < (256 / iv_length); i++)
    {
        for (index = i * iv_length; index < (i * iv_length) + iv_length; index++)
            aes_iv[index - (i * iv_length)] = index;
        if (aes_check(aes_key, key_length, aes_iv, iv_length, aes_aad, aad_length, cipher, aes_hard_in_data, data_length)
            == AES_CHECK_FAIL)
            total_check_tag = 1;
    }
    if(total_check_tag)
        return AES_CHECK_FAIL;
    else
        return AES_CHECK_PASS;
}

int main(void)
{
    aes_cipher_mode_t cipher;
    printf("begin test %d\n", get_time_flag);

    for (cipher = AES_ECB; cipher < AES_CIPHER_MAX; cipher++)
    {
        printf("[%s] test all byte ... \n", cipher_name[cipher]);
        if (AES_CHECK_FAIL == aes_check_all_byte(cipher))
        {
            printf("aes %s check_all_byte fail\n", cipher_name[cipher]);
            return -1;
        }
        printf("[%s] test all key ... \n", cipher_name[cipher]);
        if (AES_CHECK_FAIL == aes_check_all_key(cipher))
        {
            printf("aes %s check_all_key fail\n", cipher_name[cipher]);
            return -1;
        }
        printf("[%s] test all iv ... \n", cipher_name[cipher]);
        if (AES_CHECK_FAIL == aes_check_all_iv(cipher))
        {
            printf("aes %s check_all_iv fail\n", cipher_name[cipher]);
            return -1;
        }
        printf("[%s] [%d bytes] hard time = %ld us, soft time = %ld us\n", cipher_name[cipher],
                AES_TEST_DATA_LEN,
                cycle[cipher][0]/(sysctl_clock_get_freq(SYSCTL_CLOCK_CPU)/1000000),
                cycle[cipher][1]/(sysctl_clock_get_freq(SYSCTL_CLOCK_CPU)/1000000));
    }
    printf("aes-128 test pass\n");
    while (1)
        ;
    return 0;
}


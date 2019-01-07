#ifndef MINIMP3_H
#define MINIMP3_H
/*
    https://github.com/lieff/minimp3
    To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide.
    This software is distributed without any warranty.
    See <http://creativecommons.org/publicdomain/zero/1.0/>.
*/

#define MINIMP3_MAX_SAMPLES_PER_FRAME (1152*2)

typedef struct
{
    int frame_bytes;
    int channels;
    int hz;
    int layer;
    int bitrate_kbps;
} mp3dec_frame_info_t;

typedef struct
{
    float mdct_overlap[2][9*32];
    float qmf_state[15*2*32];
    int reserv;
    int free_format_bytes;
    unsigned char header[4];
    unsigned char reserv_buf[511];
} mp3dec_t;


void mp3dec_init(mp3dec_t *dec);
int mp3dec_decode_frame(mp3dec_t *dec, const unsigned char *mp3, int mp3_bytes, short *pcm, mp3dec_frame_info_t *info);


#endif /*MINIMP3_H*/



//MP3解码实现开始
#include <string.h>// memcpy memset memmove
#include <stdint.h>// (u)int(8|16|32)_t



#define MAX_FREE_FORMAT_FRAME_SIZE  1152*2    /* more than ISO spec's */
#define MAX_FRAME_SYNC_MATCHES      10

#define MAX_L3_FRAME_PAYLOAD_BYTES  MAX_FREE_FORMAT_FRAME_SIZE /* MUST be >= 320000/8/32000*1152 = 1440 */

#define MAX_BITRESERVOIR_BYTES      511
#define SHORT_BLOCK_TYPE            2
#define STOP_BLOCK_TYPE             3
#define MODE_MONO                   3
#define MODE_JOINT_STEREO           1
#define HDR_SIZE                    4
#define HDR_IS_MONO(h)              (((h[3]) & 0xC0) == 0xC0)
#define HDR_IS_MS_STEREO(h)         (((h[3]) & 0xE0) == 0x60)
#define HDR_IS_FREE_FORMAT(h)       (((h[2]) & 0xF0) == 0)
#define HDR_IS_CRC(h)               (!((h[1]) & 1))
#define HDR_TEST_PADDING(h)         ((h[2]) & 0x2)
#define HDR_TEST_MPEG1(h)           ((h[1]) & 0x8)
#define HDR_TEST_NOT_MPEG25(h)      ((h[1]) & 0x10)
#define HDR_TEST_I_STEREO(h)        ((h[3]) & 0x10)
#define HDR_TEST_MS_STEREO(h)       ((h[3]) & 0x20)
#define HDR_GET_STEREO_MODE(h)      (((h[3]) >> 6) & 3)
#define HDR_GET_STEREO_MODE_EXT(h)  (((h[3]) >> 4) & 3)
#define HDR_GET_LAYER(h)            (((h[1]) >> 1) & 3)
#define HDR_GET_BITRATE(h)          ((h[2]) >> 4)
#define HDR_GET_SAMPLE_RATE(h)      (((h[2]) >> 2) & 3)
#define HDR_GET_MY_SAMPLE_RATE(h)   (HDR_GET_SAMPLE_RATE(h) + (((h[1] >> 3) & 1) + ((h[1] >> 4) & 1))*3)
#define HDR_IS_FRAME_576(h)         ((h[1] & 14) == 2)
#define HDR_IS_LAYER_1(h)           ((h[1] & 6) == 6)

#define BITS_DEQUANTIZER_OUT        -1
#define MAX_SCF                     (255 + BITS_DEQUANTIZER_OUT*4 - 210)
#define MAX_SCFI                    ((MAX_SCF + 3) & ~3)

#define MINIMP3_MIN(a, b)           ((a) > (b) ? (b) : (a))
#define MINIMP3_MAX(a, b)           ((a) < (b) ? (b) : (a))


//bits结构体
typedef struct
{
    const uint8_t *buf;//字节数据
    int pos;//当前位位置
    int limit;
} bs_t;

//gr信息结构体
typedef struct
{
    const uint8_t *sfbtab;
    uint16_t part_23_length;
    uint16_t big_values;
    uint16_t scalefac_compress;
    uint8_t global_gain;
    uint8_t block_type;
    uint8_t mixed_block_flag;
    uint8_t n_long_sfb;
    uint8_t n_short_sfb;
    uint8_t table_select[3];
    uint8_t region_count[3];
    uint8_t subblock_gain[3];
    uint8_t preflag;
    uint8_t scalefac_scale;
    uint8_t count1_table;
    uint8_t scfsi;
} L3_gr_info_t;

//
typedef struct
{
    bs_t bs;
    uint8_t maindata[MAX_BITRESERVOIR_BYTES + MAX_L3_FRAME_PAYLOAD_BYTES];
    L3_gr_info_t gr_info[4];
    float grbuf[2][576];
    float scf[40];
    uint8_t ist_pos[2][39];
    float syn[18 + 15][2*32];
} mp3dec_scratch_t;

void mp3_to_wav(const uint8_t *mp3_buf,size_t mp3_size, uint8_t *wav, size_t wav_size);


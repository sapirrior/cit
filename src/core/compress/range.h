#ifndef RANGE_H
#define RANGE_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint32_t low;
    uint32_t range;
    uint8_t *out;
    size_t pos;
} RangeEncoder;

typedef struct {
    uint32_t low;
    uint32_t range;
    uint32_t code;
    const uint8_t *in;
    size_t pos;
    size_t size;
} RangeDecoder;

typedef struct {
    uint16_t freq[257];
    uint16_t total;
} AdaptiveModel;

void model_init(AdaptiveModel *m);
void model_update(AdaptiveModel *m, int symbol);
void range_encoder_init(RangeEncoder *e, uint8_t *out);
void range_encode(RangeEncoder *e, uint32_t cum_freq, uint32_t freq, uint32_t total);
void range_encoder_flush(RangeEncoder *e);
void range_decoder_init(RangeDecoder *d, const uint8_t *in, size_t size);
uint32_t range_decode_cum(RangeDecoder *d, uint32_t total);
void range_decode_update(RangeDecoder *d, uint32_t cum_freq, uint32_t freq, uint32_t total);

#endif // RANGE_H

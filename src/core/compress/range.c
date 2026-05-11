#include "range.h"
#include "internal.h"

void model_init(AdaptiveModel *m) {
    for (int i = 0; i < 257; i++) m->freq[i] = 1;
    m->total = 257;
}

void model_update(AdaptiveModel *m, int symbol) {
    m->freq[symbol]++;
    m->total++;
    // Rescale to prevent overflow
    if (m->total > 32000) {
        m->total = 0;
        for (int i = 0; i < 257; i++) {
            m->freq[i] = (m->freq[i] >> 1) | 1;
            m->total += m->freq[i];
        }
    }
}

void range_encoder_init(RangeEncoder *e, uint8_t *out) {
    e->low = 0;
    e->range = 0xFFFFFFFF;
    e->out = out;
    e->pos = 0;
}

void range_encode(RangeEncoder *e, uint32_t cum_freq, uint32_t freq, uint32_t total) {
    e->low += cum_freq * (e->range /= total);
    e->range *= freq;
    while ((e->low ^ (e->low + e->range)) < RANGE_TOP || e->range < RANGE_BOT) {
        e->out[e->pos++] = (uint8_t)(e->low >> 24);
        e->low <<= 8;
        e->range <<= 8;
    }
}

void range_encoder_flush(RangeEncoder *e) {
    for (int i = 0; i < 4; i++) {
        e->out[e->pos++] = (uint8_t)(e->low >> 24);
        e->low <<= 8;
    }
}

void range_decoder_init(RangeDecoder *d, const uint8_t *in, size_t size) {
    d->low = 0;
    d->range = 0xFFFFFFFF;
    d->code = 0;
    d->in = in;
    d->pos = 0;
    d->size = size;
    for (int i = 0; i < 4; i++) {
        d->code = (d->code << 8) | d->in[d->pos++];
    }
}

uint32_t range_decode_cum(RangeDecoder *d, uint32_t total) {
    return d->code / (d->range /= total);
}

void range_decode_update(RangeDecoder *d, uint32_t cum_freq, uint32_t freq, uint32_t total) {
    d->low = cum_freq * d->range;
    d->code -= d->low;
    d->range *= freq;
    while ((d->low ^ (d->low + d->range)) < RANGE_TOP || d->range < RANGE_BOT) {
        d->code = (d->code << 8) | d->in[d->pos++];
        d->range <<= 8;
    }
}

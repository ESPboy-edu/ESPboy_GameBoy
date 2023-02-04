/* Audio Support */

#pragma GCC optimize ("-O3")

#include "sound.h"

#include <stdio.h>
#include <math.h>


// Audio output buffer

//s16 AUDIO_BUFFER_L[AUDIO_BUFFER_SIZE];
//s16 AUDIO_BUFFER_R[AUDIO_BUFFER_SIZE];

// Audio output buffer indices

//u32 buffer_start = 0;
//u32 buffer_end = 0;
//u32 fill_amt = 0;
//u32 fill_start = 0;
//u32 fill_end = 0;
//u32 fill_idx = 0;

//u32 audio_frame = 0;
u32 sample_count = 0;
u8 audio_cycle=0;

// Debug

//FILE* raw = NULL;

// APU registers, state, and RAM

CH1_t CH1;
CH2_t CH2;
CH3_t CH3;
CH4_t CH4;
SO_t  SO;

u8 AUDIO_RAM[AUDIO_RAM_SIZE] = 
{ 
    0x84, 0x40, 0x43, 0xAA, 0x2D, 0x78, 0x92, 0x3C, 0x60, 0x59, 0x59, 0xB0, 0x34, 0xB8, 0x2E, 0xDA 
};

// APU output settings

u8 READ_SO_50()
    {
    return 0;
    }

void WRITE_SO_50(u8 value)
    {
    }

u8 READ_SO_51()
    {
    return 0;
    }

void WRITE_SO_51(u8 value)
    {
    }

u8 READ_SO_52()
    {
    return (
        (SO.power << NR52_POWER_OFFS) |
        (CH1.channel.enable) |
        (CH2.channel.enable << 1) |
        (CH3.channel.enable << 2) |
        (CH4.channel.enable << 3)
    );
    }

void WRITE_SO_52(u8 value)
    {
    if (value & NR52_POWER_BIT)
        {
        SO.power = 1;
        }
    else
        {
        SO.power = 0;
        CH1.channel.enable = CH1.channel.initset = 0;
        CH2.channel.enable = CH2.channel.initset = 0;
        CH3.channel.enable = CH3.channel.initset = 0;
        CH4.channel.enable = CH4.channel.initset = 0;
        }
    }

// frequency sweep function

u8 READ_SWEEP(SWEEP* sweep)
    {
    return (
        ((sweep->time << SWEEP_TIME_OFFS) & SWEEP_TIME_BITS) |
        ((sweep->dir << SWEEP_DIR_OFFS) & SWEEP_DIR_BIT) |
        ((sweep->shift << SWEEP_SHIFT_OFFS) & SWEEP_SHIFT_BITS)
    );
    }

void WRITE_SWEEP(SWEEP* sweep, u8 value)
    {
    sweep->time = (value & SWEEP_TIME_BITS) >> SWEEP_TIME_OFFS;
    sweep->dir = (value & SWEEP_DIR_BIT) >> SWEEP_DIR_OFFS;
    sweep->shift = (value & SWEEP_SHIFT_BITS) >> SWEEP_SHIFT_OFFS;
    }

// duty cycle / sound length function

u8 READ_DUTY_LEN(DUTY_LEN* duty_len)
    {
    return (
        ((duty_len->duty << DUTY_OFFS) & DUTY_BITS) |
        ((duty_len->len << SOUND_LEN_OFFS) & SOUND_LEN_BITS)
    );
    }

void WRITE_DUTY_LEN(DUTY_LEN* duty_len, u8 value)
    {
    duty_len->duty = (value & DUTY_BITS) >> DUTY_OFFS;
    duty_len->len = (value & SOUND_LEN_BITS) >> SOUND_LEN_OFFS;
    }

// volume envelope function

u8 READ_ENVELOPE(ENVELOPE* env)
    {
    return (
        ((env->volume << INIT_VOLUME_OFFS) & INIT_VOLUME_BITS) |
        ((env->dir << ENV_DIR_OFFS) & ENV_DIR_BIT) |
        ((env->period << ENV_SWEEP_OFFS) & ENV_SWEEP_BITS)
    );
    }

void WRITE_ENVELOPE(ENVELOPE* env, u8 value)
    {
    env->volume = (value & INIT_VOLUME_BITS) >> INIT_VOLUME_OFFS;
    env->dir = (value & ENV_DIR_BIT) >> ENV_DIR_OFFS;
    env->period = (value & ENV_SWEEP_BITS) >> ENV_SWEEP_OFFS;
    env->timer = 0;
    }

// channel frequency, init, count enable function

u8 READ_CHANNEL_LO(CHANNEL* channel)
    {
    return (channel->freq & FREQ_LO_MASK);
    }

void WRITE_CHANNEL_LO(CHANNEL* channel, u8 value)
    {
    channel->freq = (channel->freq & FREQ_HI_MASK) | (value & FREQ_LO_MASK);
    }

u8 READ_CHANNEL_HI(CHANNEL* channel)
    {
    return (
        (channel->counterset << COUNTER_OFFS) & COUNTER_BIT
    );
    }

void WRITE_CHANNEL_HI(CHANNEL* channel, u8 value, u8 channel_num)
    {
    channel->initset = (value & INIT_BIT);
    channel->counterset = (value & COUNTER_BIT) >> COUNTER_OFFS;
    channel->freq = (channel->freq & FREQ_LO_MASK) | ((value & FREQ_HI_BITS) << 8);
    if (!channel->enable && channel->initset)
        {
        channel->enable = 1;
        channel->timer = 0;
        switch (channel_num)
            {
            case 1:
                CH1.envelope.disabled = 0;
                CH1.envelope.timer = 0;
            break;
            case 2:
                CH2.envelope.disabled = 0;
                CH2.envelope.timer = 0;
            break;
            case 3:
                CH3.pos_counter = 0;
            break;
            case 4:
                CH4.envelope.disabled = 0;
                CH4.envelope.timer = 0;
            break;
            }
        }
    }


/*
    Handle reads from sound controller.
*/
u8 audio_read(u8 addr)
    {
    // check audio unit power
    if (!SO.power) return 0;

    // read sound registers / data
    switch (addr)
        {
        /* NR10 - Ch1 Sweep */
        case 0x10: return READ_SWEEP(&CH1.sweep);
        /* NR11 - Ch1 Duty / Len */
        case 0x11: return READ_DUTY_LEN(&CH1.duty_len);
        /* NR12 - Ch1 Envelope */
        case 0x12: return READ_ENVELOPE(&CH1.envelope);
        /* NR13 - Ch1 Lo Freq */
        case 0x13: return READ_CHANNEL_LO(&CH1.channel);
        /* NR14 - CH1 Init, Counter, Hi Freq */
        case 0x14: return READ_CHANNEL_HI(&CH1.channel);
        /* NR20 - Not Used */
        case 0x15: return 0;
        /* NR21 - Ch2 Duty / Len */
        case 0x16: return READ_DUTY_LEN(&CH2.duty_len);
        /* NR22 - Ch2 Envelope */
        case 0x17: return READ_ENVELOPE(&CH2.envelope);
        /* NR23 - Ch2 Lo Freq */
        case 0x18: return READ_CHANNEL_LO(&CH2.channel);
        /* NR24 - Ch2 Init, Counter, Hi Freq */
        case 0x19: return READ_CHANNEL_HI(&CH2.channel);
        /* NR30 - Ch3 Sound On */
        case 0x1A: return (CH3.enable << NR30_SOUND_ON_OFFS) & NR30_SOUND_ON_BIT;
        /* NR31 - Ch3 Sound Len */
        case 0x1B: return CH3.sound_len;
        /* NR32 - Ch3 Volume Out Level */
        case 0x1C: return (CH3.out_level << NR32_OUT_LEVEL_OFFS) & NR32_OUT_LEVEL_BITS;
        /* NR33 - Ch3 Lo Freq */
        case 0x1D: return READ_CHANNEL_LO(&CH3.channel);
        /* NR34 - Ch3 Init, Counter, Hi Freq */
        case 0x1E: return READ_CHANNEL_HI(&CH3.channel);
        /* NR40 - Not Used */
        case 0x1F: return 0;
        /* NR41 - Ch4 Len Register */
        case 0x20: return READ_DUTY_LEN(&CH4.len);
        /* NR42 - Ch4 Envelope */
        case 0x21: return READ_ENVELOPE(&CH4.envelope);
        /* NR43 - Ch4 Clock, Step, Ratio */
        case 0x22: return CH4.NR43;
        /* NR44 - Ch4 Init, Counter */
        case 0x23: return READ_CHANNEL_HI(&CH4.channel);
        /* NR50 */
        case 0x24: return READ_SO_50();
        /* NR51 */
        case 0x25: return READ_SO_51();
        /* NR52 */
        case 0x26: return READ_SO_52();
        // wave pattern RAM
        case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37: 
        case 0x38: case 0x39: case 0x3A: case 0x3B: case 0x3C: case 0x3D: case 0x3E: case 0x3F: 
            return AUDIO_RAM[(addr - 0x30)];
        }
    return 0;
    }

/*
    Handle writes to sound controller.
*/
void audio_write(u8 addr, u8 val)
    {
    // check audio unit power
    if (!(SO.power || addr == NR52_ADDR)) return;

    switch (addr)
        {
        /* NR10 - Ch1 Sweep */
        case 0x10: WRITE_SWEEP(&CH1.sweep, val); return;
        /* NR11 - Ch1 Duty / Len */
        case 0x11: WRITE_DUTY_LEN(&CH1.duty_len, val); return;
        /* NR12 - Ch1 Envelope */
        case 0x12: WRITE_ENVELOPE(&CH1.envelope, val); return;
        /* NR13 - Ch1 Lo Freq */
        case 0x13: WRITE_CHANNEL_LO(&CH1.channel, val); return;
        /* NR14 - CH1 Init, Counter, Hi Freq */
        case 0x14: WRITE_CHANNEL_HI(&CH1.channel, val, 1); return;
        /* NR20 - Not Used */
        case 0x15: return;
        /* NR21 - Ch2 Duty / Len */
        case 0x16: WRITE_DUTY_LEN(&CH2.duty_len, val); return;
        /* NR22 - Ch2 Envelope */
        case 0x17: WRITE_ENVELOPE(&CH2.envelope, val); return;
        /* NR23 - Ch2 Lo Freq */
        case 0x18: WRITE_CHANNEL_LO(&CH2.channel, val); return;
        /* NR24 - Ch2 Init, Counter, Hi Freq */
        case 0x19: WRITE_CHANNEL_HI(&CH2.channel, val, 2); return;
        /* NR30 - Ch3 Sound On */
        case 0x1A: CH3.enable = (val & NR30_SOUND_ON_BIT) >> NR30_SOUND_ON_OFFS; return;
        /* NR31 - Ch3 Sound Len */
        case 0x1B: CH3.sound_len = val; return;
        /* NR32 - Ch3 Volume Out Level */
        case 0x1C: CH3.out_level = (val & NR32_OUT_LEVEL_BITS) >> NR32_OUT_LEVEL_OFFS; return;
        /* NR33 - Ch3 Lo Freq */
        case 0x1D: WRITE_CHANNEL_LO(&CH3.channel, val); return;
        /* NR34 - Ch3 Init, Counter, Hi Freq */
        case 0x1E: WRITE_CHANNEL_HI(&CH3.channel, val, 3); return;
        /* NR40 - Not Used */
        case 0x1F: return;
        /* NR41 - Ch4 Len Register */
        case 0x20: WRITE_DUTY_LEN(&CH4.len, val); return;
        /* NR42 - Ch4 Envelope */
        case 0x21: WRITE_ENVELOPE(&CH4.envelope, val); return;
        /* NR43 - Ch4 Clock, Step, Ratio */
        case 0x22: CH4.NR43 = val; return;
        /* NR44 - Ch4 Init, Counter */
        case 0x23: WRITE_CHANNEL_HI(&CH4.channel, val, 4); return;
        /* NR50 */
        case 0x24: WRITE_SO_50(val); return;
        /* NR51 */
        case 0x25: WRITE_SO_51(val); return;
        /* NR52 */
        case 0x26: WRITE_SO_52(val); return;
        // wave pattern RAM
        case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37: 
        case 0x38: case 0x39: case 0x3A: case 0x3B: case 0x3C: case 0x3D: case 0x3E: case 0x3F: 
            AUDIO_RAM[(addr - 0x30)] = val;
            return;
        }
    }

void CHANNEL_UPDATE(
    CHANNEL* channel,
    u8 channel_num,
    u8 sound_len,
    u8 audio_cycle
    )
    {
    if (channel->enable && channel->counterset)
        {
        channel->timer++;
        if (channel->timer == sound_len)
            {
            channel->enable = 0;
            }
        }

    return;
    }

void SWEEP_UPDATE(CHANNEL* channel, SWEEP* sweep, u8 audio_cycle)
    {
    if (audio_cycle % 2) return;

    if (sweep->time && sweep->shift)
        {
        sweep->timer = (sweep->timer + 1) % sweep->time;
        if (sweep->timer == 0)
            {
            u16 freq, offset;
            sweep->freq = channel->freq;
            offset = (sweep->freq >> sweep->shift);

            if (sweep->dir)
                {
                freq = sweep->freq - offset;
                }
            else
                {
                freq = sweep->freq + offset;
                }

            if (freq < MAX_FREQ)
                {
                channel->freq = freq;
                }
            else
                {
                channel->enable = 0;
                }
            }
        }
    }

void ENVELOPE_UPDATE(ENVELOPE* envelope, u8 audio_cycle)
    {
    if (audio_cycle == 3 &&
        envelope->period != 0 && 
        !envelope->disabled)
        {
        envelope->timer = (envelope->timer + 1) % envelope->period;
        if (envelope->timer == 0)
            {
            u8 new_vol = envelope->volume;
            if (envelope->dir)
                {
                new_vol++;
                }
            else
                {
                new_vol--;
                }
            if (new_vol <= 16)
                {
                envelope->volume = new_vol;
                }
            else
                {
                envelope->disabled = 1;
                }
            }
        }
    }


/* Generate 'noise' */
s16 NOISE(u32 tick, u8 vol, u8 tone)
    {
    s16 svol = vol / 0x02;
    static u32 rand_state = 0xF390439F;
    rand_state = 137 * rand_state + ((rand_state / 0x08) | (rand_state << 0x18)) + 1;

    return svol * (rand_state & 0xFF - 0x80);
    }

/* Process GB frequency value. */
u32 PERIOD(u16 xfreq)
    {
    //u32 fp_conversion = (0x100 * SAMPLING_RATE / AUDIO_SAMPLING_RATE);
    return (0x10 * SAMPLING_RATE * (2048 - xfreq) / AUDIO_SAMPLING_RATE);
    }

/* Signal generator */
s16 RESAMPLE(u32 tick, u32 fp_period, u8 vol, u8 duty)
    {
    s16 svol = vol;
    //u8 fp_phase, phase;
    u32 subtick;

    //fp_phase = ((0x10 * tick / fp_period) % 0x80);
    //phase = fp_phase / 0x10;
    subtick = ((0x10 * tick) % fp_period) * 0x80 / fp_period;

#ifdef SQUARE_WAVE
    u8 duties[4] = { 0x01, 0x81, 0x83, 0x7E };
    if (duties[duty] & (0x80 >> (subtick / 0x10)))
        {
        return 0x20 * svol;
        }
    else
        {
        return -0x20 * svol;
        }
#endif
#ifdef TRIANGLE_WAVE
        subtick = (subtick + 0x20) % 0x80;
        if (subtick > 0x40)
            {
            subtick = 0x80 - subtick;
            }
        return svol * ((s16)subtick - 0x20);
#endif
    }


// modify soundwave
s16 GENERATE_WAVE(u16 freq, u8 vol, u8 duty)
    {
            s16 sample=RESAMPLE(sample_count, PERIOD(freq), vol, duty);

            return sample;
    }

s16 GENERATE_CH3(u16 freq, u8 vol)
    {
    s16 sample=0;
    u32 fp_period;
    u32 tick, index;
    u8 byte, nibble;

    if (vol)
        {
        fp_period = PERIOD(freq);
             
            tick = CH3.pos_counter ;
            index = ((0x08 * tick) / fp_period);
            byte = (index / 2) % AUDIO_RAM_SIZE;
            nibble = 1 - (index % 2);

            sample = (AUDIO_RAM[byte] >> (4 * nibble)) & 0x0F; 
            sample = (sample) * (1 << (9 - vol));
            
        }
    //CH3.pos_counter += fill_amt;

    return sample;
    }

s16 GENERATE_NOISE(u8 vol, u8 tone)
    {
    s16 sample=0;
    u32 i;
    if (vol > 0)
        {
            sample = NOISE(sample_count, vol, tone);
        }

        return sample;
    }



    extern int32_t maxout;
    extern uint8_t divider;
    extern int32_t minch1, minch2, minch3, minch4;
    int32_t ch;
    
/*
    Handle audio logic and generate sound at 256hz
*/
u8 audio_update()
    {
    /*
        Audio buffer housekeeping.
    */
    int32_t out=0;

    // check APU power
    if (SO.power) 
        {
    /*
        Channel logic and sound generation.
    */

    // Channel 1

    CHANNEL_UPDATE(
        &CH1.channel, 1,
        64 - CH2.duty_len.len,
        audio_cycle
    );

    //if (CH1.channel.enable && 1)
    //    {
        SWEEP_UPDATE(
            &CH1.channel,
            &CH1.sweep,
            audio_cycle
        );

        ENVELOPE_UPDATE(
            &CH1.envelope,
            audio_cycle
        );

        ch=GENERATE_WAVE(
            CH1.channel.freq,
            CH1.envelope.volume,
            CH1.duty_len.duty
        );
        if(ch<minch1)minch1=ch;
        out+=ch-minch1;
      //  }

    // Channel 2

    CHANNEL_UPDATE(
        &CH2.channel, 2,
        64 - CH2.duty_len.len,
        audio_cycle
    );

    //if (CH2.channel.enable && 1)
    //    {
        ENVELOPE_UPDATE(
            &CH2.envelope,
            audio_cycle
        );

        ch=GENERATE_WAVE(
            CH2.channel.freq,
            CH2.envelope.volume,
            CH2.duty_len.duty
        );
        if(ch<minch2)minch2=ch;
        out+=ch-minch2;      
      //  }


    // Channel 3

    CHANNEL_UPDATE(
        &CH3.channel, 3,
        64 - CH3.sound_len,
        audio_cycle);

    //if (CH3.channel.enable && 1)
    //    {
        ch=GENERATE_CH3(CH3.channel.freq, CH3.out_level);
        if(ch<minch3)minch3=ch;
        out+=ch-minch3;
    //    }


    // Channel 4

    CHANNEL_UPDATE(
        &CH4.channel, 4,
        64 - CH4.len.len,
        audio_cycle
        );

    //if (CH4.channel.enable && 1)
    //    {
        ENVELOPE_UPDATE(
            &CH4.envelope,
            audio_cycle
        );
    //out+=(GENERATE_NOISE(CH4.envelope.volume, CH4.NR43 & NR43_DIV_RATIO_BITS));
    ch=(GENERATE_NOISE(CH4.envelope.volume, CH4.NR43 & NR43_DIV_RATIO_BITS))>>1;//make noise silent dev by 2
    if(ch<minch3)minch3=ch;
    out+=ch-minch3;
     //   }
    
        }

++audio_cycle;
++sample_count;

    if (out>maxout)maxout=out;

    out = out/divider;
    return (out);
    }

#pragma once
/*
 *  File: synth.h
 *
 *  synth unit for braids
 *
 */

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cmath>

#include <arm_neon.h>

#include "unit.h"  // Note: Include common definitions for all units

#include "braids/envelope.h"
#include "braids/macro_oscillator.h"

enum Params {
    Note = 0,
    Shape,
    Param1,
    Param2,
    Attack,
    Decay,
    AD_Timbre,
    AD_Color,
    Resolution,
};

inline float note2freq(float note) {
    return (440.f / 32) * powf(2, (note - 9.0) / 12);
}

const float x7fff_recipf = 1.f / 32767;

const uint16_t bit_reduction_masks[] = {
    0xc000,
    0xe000,
    0xf000,
    0xf800,
    0xff00,
    0xfff0,
    0xffff
};

class Synth {
public:
    Synth(void) {}
    ~Synth(void) {}

    inline int8_t Init(const unit_runtime_desc_t * desc) {
        if (desc->samplerate != 48000)
            return k_unit_err_samplerate;

        if (desc->output_channels != 2)  // should be stereo output
            return k_unit_err_geometry;

        std::memset(&osc_, 0, sizeof(osc_));
        osc_.Init();
        envelope_.Init();

        return k_unit_err_none;
    }

    inline void Teardown() {}

    inline void Reset() {
        gate_ = 0;
    }

    inline void Resume() {}

    inline void Suspend() {}

    fast_inline void Render(float * out, size_t frames) {
        float * __restrict out_p = out;
        const int bufsize = 24; // size of temp_buffer in macro_oscillator.h

        osc_.set_pitch(pitch_);

        int16_t buf[bufsize] = {};
        const uint8_t sync[bufsize] = {};
        uint16_t bit_mask = bit_reduction_masks[p_[Resolution]];
        for(uint32_t p = 0; p < frames; p += bufsize) {
            uint32_t env = envelope_.Render();

            // Set timbre and color: parameter value + internal modulation.
            int32_t timbre = timbre_;
            timbre += env * p_[AD_Timbre] >> 5;
            CONSTRAIN(timbre, 0, 32767);
            int32_t color = color_;
            color += env * p_[AD_Color] >> 5;
            CONSTRAIN(color, 0, 32767);
            osc_.set_parameters(timbre, color);

            size_t r_size = (bufsize < (frames - p)) ? bufsize : frames - p;
            osc_.Render(sync, buf, r_size);

            // Copy to the buffer with sample rate and bit reduction applied.
            int32_t gain = env;
            int16_t sample = 0;
            for(uint32_t i = 0; i < r_size ; i++, out_p += 2) {
                sample = buf[i] & bit_mask;
                sample = sample * gain_lp_ >> 16;
                gain_lp_ += (gain - gain_lp_) >> 4;
                vst1_f32(out_p, vdup_n_f32(amp_ * sample / 32768.f));
            }
        }
    }

    inline void setParameter(uint8_t index, int32_t value) {
        p_[index] = value;
        switch (index) {
        case Note:    // 0..127
            pitch_ = value << 7;
            break;
        case Shape:   // 0..46
            osc_.set_shape(static_cast<braids::MacroOscillatorShape>(value));
            break;
        case Param1:  // -256..255
            // timbre and color must be 0..32767
            timbre_ = (value + 256) << 6;
            break;
        case Param2:
            color_ = (value + 256) << 6;
            break;
            break;
        case Attack:
        case Decay:
            envelope_.Update(p_[Attack], p_[Decay]);
            break;
        default:
            break;
        }
    }

    inline int32_t getParameterValue(uint8_t index) const {
        return p_[index];
    }

    inline const char * getParameterStrValue(uint8_t index, int32_t value) const {
        (void)value;
        switch (index) {
        case Shape:
            if (value < 47) {
                return ShapeStr[value];
            } else {
                return nullptr;
            }

        case Resolution:
            if (value < 7) {
                return BitsStr[value];
            } else {
                return nullptr;
            }

        default:
            break;
        }
        return nullptr;
    }

    inline const uint8_t * getParameterBmpValue(uint8_t index,
                                              int32_t value) const {
        (void)value;
        switch (index) {
        default:
            break;
        }
        return nullptr;
    }

    inline void NoteOn(uint8_t note, uint8_t velocity) {
        pitch_ = note << 7;
        GateOn(velocity);
    }

    inline void NoteOff(uint8_t note) {
        (void)note;
        GateOff();
    }

    inline void GateOn(uint8_t velocity) {
        amp_ = 1. / 127 * velocity;
        gate_ += 1;
        osc_.Strike();
        envelope_.Trigger(braids::EnvelopeSegment::ENV_SEGMENT_ATTACK);
    }

    inline void GateOff() {
        if (gate_ > 0 ) {
            gate_ -= 1;
        }
    }

    inline void AllNoteOff() {}

    inline void PitchBend(uint16_t bend) { (void)bend; }

    inline void ChannelPressure(uint8_t pressure) { (void)pressure; }

    inline void Aftertouch(uint8_t note, uint8_t aftertouch) {
        (void)note;
        (void)aftertouch;
    }

    inline void LoadPreset(uint8_t idx) { (void)idx; }

    inline uint8_t getPresetIndex() const { return 0; }

    /* Static Members. */

    static inline const char * getPresetName(uint8_t idx) {
        (void)idx;
        return nullptr;
    }

private:
    std::atomic_uint_fast32_t flags_;

    int32_t p_[24];
    braids::MacroOscillator osc_;
    braids::Envelope envelope_;

    int16_t pitch_;
    int16_t timbre_;
    int16_t color_;
    float amp_;
    uint32_t gate_;

    uint16_t gain_lp_;

    /* Private Methods. */
    /* Constants. */
    const char *ShapeStr[47] = {
        "CSAW",
        "/\\-_",
        "//-_",
        "FOLD",
        "uuuu",
        "SUB-",
        "SUB/",
        "SYN-",
        "SYN/",
        "//x3",
        "-_x3",
        "/\\x3",
        "SIx3",
        "RING",
        "////",
        "//uu",
        "TOY*",
        "ZLPF",
        "ZPKF",
        "ZBPF",
        "ZHPF",
        "VOSM",
        "VOWL",
        "VFOF",
        "HARM",
        "FM  ",
        "FBFM",
        "WTFM",
        "PLUK",
        "BOWD",
        "BLOW",
        "FLUT",
        "BELL",
        "DRUM",
        "KICK",
        "CYMB",
        "SNAR",
        "WTBL",
        "WMAP",
        "WLIN",
        "WTx4",
        "NOIS",
        "TWNQ",
        "CLKN",
        "CLOU",
        "PRTC",
        "QPSK",
    };

    const char *BitsStr[7] = {
        " 2",
        " 3",
        " 4",
        " 6",
        " 8",
        "12",
        "16",
    };
};

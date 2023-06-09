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
#include <cstdio>

#include <algorithm>

#include <arm_neon.h>

#include "unit.h"  // Note: Include common definitions for all units

#include "stmlib/utils/dsp.h"
//#include "braids/envelope.h"
#include "braids/macro_oscillator.h"
#include "braids/signature_waveshaper.h"
#include "braids/vco_jitter_source.h"
#include "linenvelope.h"

using namespace stmlib;

constexpr size_t PRESET_COUNT = 1;
const char *PresetNameStr[PRESET_COUNT] = {
    "Init",
};

enum Params {
    Note = 0,
    Shape,
    Param1,
    Param2,
    EG1Curve,
    EG1Trigger,
    Attack,
    Decay,
    ModSrcVCA,
    ModIntVCA,
    ModSrcFM,
    ModIntFM,
    EG2Curve,
    EG2Trigger,
    Attack2,
    Decay2,
    ModSrcTimbre,
    ModIntTimbre,
    ModSrcColor,
    ModIntColor,
    Octave,
    Pitch,
    Resolution,
    SampleRate,
    ModSrcShape,
    ModIntShape,
    Signature,
    VCO_Flatten,
    VCO_Drift,
    PARAMCOUNT,
};

enum ModulationSrc {
    SRC_EG1,
    SRC_EG2,
    SRC_SUM,
    SRC_MUL,
    SRC_A_MINUS_B,
    SRC_B_MINUS_A,
    SRC_A_MINUS_AB,
    SRC_B_MINUS_AB,
    SRC_A_PLUS_B_MINUS_AB,
    MODSRCCOUNT
};
/*
enum EGType {
    EG_EXP_1SHOT_ALWAYS,
    EG_EXP_LOOP_ALWAYS,
    EG_EXP_LOOP_GATE,
    EG_EXP_LOOP_NOTGATE,
    EG_EXP_LOOP_ATTACK,
    EG_EXP_LOOP_DECAY,
    EG_LIN_1SHOT_ALWAYS,
    EG_LIN_LOOP_ALWAYS,
    EG_LIN_LOOP_GATE,
    EG_LIN_LOOP_NOTGATE,
    EG_LIN_LOOP_ATTACK,
    EG_LIN_LOOP_DECAY,
    EGTYPECOUNT
};
*/
enum EGTrigger {
    EG_GATEON,
    EG_A_END,
    EG_A_ATTACK_END,
    EG_B_END,
    EG_B_ATTACK_END,
    EG_A_DECY_B_END,
    EG_B_DECY_A_END,
    EGTRIGGERCOUNT
};

inline float note2freq(float note) {
    return (440.f / 32) * powf(2, (note - 9.0) / 12);
}

inline int32_t clipminmax(int32_t a, int32_t x, int32_t b) {
    return x > b ? b : (x < a ? a : x);
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

const uint16_t decimation_factors[] = { 12, 8, 6, 3, 2, 1 };

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
        envelope2_.Init();
        ws_.Init(0x42636877U); // in the original src, MPU's unique id is used 
        jitter_source_.Init();

        return k_unit_err_none;
    }

    inline void Teardown() {}

    inline void Reset() {
        gate_ = 0;
        trigger_ = 0;
    }

    inline void Resume() {}

    inline void Suspend() {}

    fast_inline void Render(float * out, size_t frames) {
        float * __restrict out_p = out;
        const int bufsize = 24; // size of temp_buffer in macro_oscillator.h

        int16_t buf[bufsize] = {};
        const uint8_t sync[bufsize] = {};
        int16_t env1trigger = getTrigger(p_[EG1Trigger]);
        int16_t env2trigger = getTrigger(p_[EG2Trigger]);
        size_t decimation_factor = decimation_factors[p_[SampleRate]];
        uint16_t bit_mask = bit_reduction_masks[p_[Resolution]];
        uint16_t signature = p_[Signature] * p_[Signature] * 4095;
        static uint32_t n = 0;
        static int16_t current_sample = 0;
        for(uint32_t p = 0; p < frames; p += bufsize) {
            uint32_t env1 = envelope_.Render(env1trigger);
            uint32_t env2 = envelope2_.Render(env2trigger);
            uint32_t env_val;
            uint32_t env_int;

            // Set timbre and color: parameter value + internal modulation.
            int32_t timbre = timbre_;
            env_val = getModVal(p_[ModSrcTimbre], env1, env2);
            env_int = clipminmax(0, p_[ModIntTimbre], 15);
            timbre += env_val * env_int >> 5;
            CONSTRAIN(timbre, 0, 32767);

            int32_t color = color_;
            env_val = getModVal(p_[ModSrcColor], env1, env2);
            env_int = clipminmax(0, p_[ModIntColor], 15);
            color += env_val * env_int >> 5;
            CONSTRAIN(color, 0, 32767);
            osc_.set_parameters(timbre, color);

            int32_t pitch = pitch_;
            env_val = getModVal(p_[ModSrcFM], env1, env2);
            env_int = clipminmax(0, p_[ModIntFM], 15);
            pitch += jitter_source_.Render(p_[VCO_Drift]);
            pitch += p_[Pitch] + p_[Octave] * 12 * 128;
            pitch += env_val * env_int >> 7;

            if (pitch > 16383) {
                pitch = 16383;
            } else if (pitch < 0) {
                pitch = 0;
            }

            osc_.set_pitch(pitch);

            size_t r_size = (bufsize < (frames - p)) ? bufsize : frames - p;
            osc_.Render(sync, buf, r_size);

            env_val = getModVal(p_[ModSrcVCA], env1, env2);
            int32_t gain = (env_val * p_[ModIntVCA] >> 4);
            gain += (15 - p_[ModIntVCA]) * (gate_ > 0) << 11;
            
            // Copy to the buffer with sample rate and bit reduction applied.
            for(uint32_t i = 0; i < r_size ; i++, n++, out_p += 2) {
                if ((n % decimation_factor) == 0) {
                    current_sample = buf[i] & bit_mask;
                }
                int16_t sample = current_sample * gain_lp_ >> 16;
                gain_lp_ += (gain - gain_lp_) >> 4;
                int16_t warped = ws_.Transform(sample);
                vst1_f32(out_p, vdup_n_f32(amp_ * Mix(sample, warped, signature) / 32768.f));
            }
        }
        trigger_ = 0;
    }

    inline void setParameter(uint8_t index, int32_t value) {
        p_[index] = value;
        switch (index) {
        case Note:    // 0..127
            CONSTRAIN(value, 0, 127);
            pitch_ = value << 7;
            break;
        case Shape:   // 0..46
            CONSTRAIN(value, 0, 46);
            osc_.set_shape(static_cast<braids::MacroOscillatorShape>(value));
            break;
        case Param1:  // -256..255
            // timbre and color must be 0..32767
            CONSTRAIN(value, -2560, 256);
            timbre_ = (value + 256) << 6;
            break;
        case Param2:
            CONSTRAIN(value, -2560, 256);
            color_ = (value + 256) << 6;
            break;
            break;
        case Attack:
        case Decay:
            envelope_.Update(p_[Attack], p_[Decay]);
            break;
        case Attack2:
        case Decay2:
            envelope2_.Update(p_[Attack2], p_[Decay2]);
            break;
        case EG1Curve:
            envelope_.SetCurve(value << 9);
            break;
        case EG2Curve:
            envelope2_.SetCurve(value << 9);
            break;
        case EG1Trigger:
            envelope_.Reset();
            break;
        case EG2Trigger:
            envelope2_.Reset();
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

        case SampleRate:
            if (value < 6) {
                return RateStr[value];
            } else {
                return nullptr;
            }

        case Signature:
            if (value < 5) {
                return IntensityStr[value];
            } else {
                return nullptr;
            }

        case VCO_Flatten:
            if (value < 2) {
                return OffOnStr[value];
            } else {
                return nullptr;
            }

        case VCO_Drift:
        case ModIntFM:
            if (value < 5) {
                return IntensityStr[value];
            } else {
                return nullptr;
            }

        case ModSrcTimbre:
        case ModSrcColor:
        case ModSrcVCA:
        case ModSrcFM:
        case ModSrcShape:
            if (value < MODSRCCOUNT) {
                return ModSrcStr[value];
            } else {
                return nullptr;
            }
        case EG1Trigger:
        case EG2Trigger:
            if (value < EGTRIGGERCOUNT) {
                return EGTriggerStr[value];
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

    inline void SetTempo(uint32_t tempo) {
        // const float t = (tempo >> 16) + (tempo & 0xFFFF) /
        // static_cast<float>(0x10000);
        (void) tempo;
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
        trigger_ = 1;
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

    inline void LoadPreset(uint8_t idx) {
	if (idx < PRESET_COUNT) {
	    preset_ = idx;
	    for(int i = 0 ; i < 24 ; i++) {
		setParameter(i, Presets[idx][i]);
	    }
	}
    }

    inline uint8_t getPresetIndex() const {
	return preset_;
    }

    static inline const char * getPresetName(uint8_t idx) {
	if (idx < PRESET_COUNT) {
	    return PresetNameStr[idx];
	} else {
	    return nullptr;
	}
    }

private:
    inline int16_t getTrigger(int32_t type) {
        int16_t trigger;
        switch(type) {
        case EG_A_END:
            trigger = (envelope_.segment() == braids::EnvelopeSegment::ENV_SEGMENT_DEAD);
            break;
        case EG_A_ATTACK_END:
            trigger = (envelope_.segment() > braids::EnvelopeSegment::ENV_SEGMENT_ATTACK);
            break;
        case EG_B_END:
            trigger = (envelope2_.segment() == braids::EnvelopeSegment::ENV_SEGMENT_DEAD);
            break;
        case EG_B_ATTACK_END:
            trigger = (envelope2_.segment() > braids::EnvelopeSegment::ENV_SEGMENT_ATTACK);
            break;
        case EG_A_DECY_B_END:
            trigger = (envelope_.segment() == braids::EnvelopeSegment::ENV_SEGMENT_DECAY) * (envelope2_.segment() == braids::EnvelopeSegment::ENV_SEGMENT_DEAD);
            break;
        case EG_B_DECY_A_END:
            trigger = (envelope2_.segment() == braids::EnvelopeSegment::ENV_SEGMENT_DECAY) * (envelope_.segment() == braids::EnvelopeSegment::ENV_SEGMENT_DEAD);
            break;
        default:
            trigger = trigger_;
        }
        return trigger;
    }

    inline uint32_t getModVal(int32_t src, uint32_t env, uint32_t env2) {
        int32_t env_val;
        switch(src) {
        case SRC_EG1:
            env_val = env;
            break;
        case SRC_EG2:
            env_val = env2;
            break;
        case SRC_SUM:
            env_val = (env + env2) / 2;
            break;
        case SRC_MUL:
            env_val = (env * env2) >> 16;
            break;
        case SRC_A_MINUS_B:
            env_val = env - env2;
            break;
        case SRC_B_MINUS_A:
            env_val = env2 - env;
            break;
        case SRC_A_MINUS_AB:
            env_val = env - (env * env2 >> 16);
            break;
        case SRC_B_MINUS_AB:
            env_val = env2 - (env * env2 >> 16);
            break;
        case SRC_A_PLUS_B_MINUS_AB:
            env_val = env + env2 - (env * env2 >> 16);
            break;
        default:
            break;
        }
        CONSTRAIN(env_val, 0, 0xffff);
        return env_val;
    }

    std::atomic_uint_fast32_t flags_;

    int32_t p_[PARAMCOUNT];
    uint8_t preset_;

    braids::MacroOscillator osc_;
    braids::LinEnvelope envelope_;
    braids::LinEnvelope envelope2_;
    braids::SignatureWaveshaper ws_;
    braids::VcoJitterSource jitter_source_;

    int16_t pitch_;
    int16_t timbre_;
    int16_t color_;
    float amp_;
    uint32_t gate_;
    int16_t trigger_;

    uint16_t gain_lp_;

    /* Private Methods. */
    /* Constants. */
    const char *ShapeStr[47] = {
        "CS SAW",
        "Morph",
        "Saw Sqr",
        "Fold",
        "Buzz",
        "Sqr Sub",
        "Saw Sub",
        "SqrSync",
        "SawSync",
        "Saw x 3",
        "Sqr x 3",
        "Tri x 3",
        "Sin x 3",
        "Ring",
        "SprSaws",
        "SawComb",
        "Toy*",
        "Phz LPF",
        "Phz PkF",
        "Phz BPF",
        "Phz HPF",
        "Vo sim",
        "Vowel",
        "Vwl FOF",
        "AddHarm",
        "FM  ",
        "FB FM",
        "Chaotic",
        "Plucked",
        "Bowed",
        "Blown",
        "Fluted",
        "Bell",
        "Drum",
        "Kick",
        "Cymbal",
        "Snare",
        "WaveTbl",
        "WaveMap",
        "WavLine",
        "WvTblx4",
        "FltNois",
        "Twin Q",
        "Clocked",
        "GrnlrCld",
        "PartclNz",
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

    const char *RateStr[6] = {
        " 4K",
        " 6K",
        " 8K",
        "16K",
        "24K",
        "48K",
    };

    const char* const OffOnStr[2] = { "OFF ", "ON " };

    const char *IntensityStr[5] = {
        "OFF ",
        "   1",
        "   2",
        "   3",
        "FULL"
    };

    const char *ModSrcStr[MODSRCCOUNT] ={
        " EG A",
        " EG B",
        " A + B",
        " A * B",
        " A - B",
        " B - A",
        "A - A*B",
        "B - A*B",
        "A+B-A*B",
    };

    const char *EGTriggerStr[EGTRIGGERCOUNT] = {
        "G:ON",
        "A:EODCY",
        "A:EOATK",
        "B:EODCY",
        "B:EOATK",
        "A:D&B:E",
        "B:D&A:E",
    };

    const int16_t Presets[PRESET_COUNT][24] = {
	// Note, Shape, Timbre, Color,
        // EG1 Curve, Trigger, Attack, Decay,
        // Mod VCA src, int, FM src, int,
        // EG2 Curve, Trigger, Attack, Decay,
        // Mod Timbre src, int, Color src, int,
        // Octave, Pitch, Resolution, SampleRate

	// "Init"
	{60, 0, 0, 0,
	 127, EG_GATEON, 0, 45,
	 SRC_EG1, 15, SRC_EG1, 0,
	 0, EG_GATEON, 30, 45,
	 SRC_EG2, 0, SRC_EG2, 0,
         0, 0, 6, 5},

    };
};

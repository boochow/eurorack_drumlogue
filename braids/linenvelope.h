#ifndef BRAIDS_ENVELOPE_MOD_H_
#define BRAIDS_ENVELOPE_MOD_H_

#include "stmlib/stmlib.h"

#include "stmlib/utils/dsp.h"

#include "braids/resources.h"

namespace braids {

    using namespace stmlib;

    enum EnvelopeSegment {
        ENV_SEGMENT_ATTACK = 0,
        ENV_SEGMENT_DECAY = 1,
        ENV_SEGMENT_DEAD = 2,
        ENV_NUM_SEGMENTS,
    };

    enum EnvelopeCurve {
        ENVELOPE_EXP,
        ENVELOPE_LINEAR,
    };

    class LinEnvelope {
    public:
        void Init() {
            target_[ENV_SEGMENT_ATTACK] = 65535;
            target_[ENV_SEGMENT_DECAY] = 0;
            target_[ENV_SEGMENT_DEAD] = 0;
            increment_[ENV_SEGMENT_DEAD] = 0;
            loop_ = 0;
            curve_ = ENVELOPE_EXP;
            prev_trigger_ = 0;
        }

        inline EnvelopeSegment segment() const {
            return static_cast<EnvelopeSegment>(segment_);
        }

        inline void Update(int32_t a, int32_t d) {
            increment_[ENV_SEGMENT_ATTACK] = lut_env_portamento_increments[a];
            increment_[ENV_SEGMENT_DECAY] = lut_env_portamento_increments[d];
        }
  
        inline void Trigger(EnvelopeSegment segment) {
            if (segment == ENV_SEGMENT_DEAD) {
                value_ = 0;
            }
            a_ = value_;
            b_ = target_[segment];
            segment_ = segment;
            phase_ = 0;
        }

        inline uint16_t Render(int trigger) {
            uint32_t increment = increment_[segment_];
            phase_ += increment;
            if (phase_ < increment) {
                value_ = Mix(a_, b_, 65535);
                Trigger(static_cast<EnvelopeSegment>(segment_ + 1));
            }
            if (increment_[segment_]) {
                if (curve_ == ENVELOPE_LINEAR) {
                    value_ = Mix(a_, b_, phase_ >> 16);
                } else {
                    value_ = Mix(a_, b_, Interpolate824(lut_env_expo, phase_));
                }
            } else if (loop_) {
                Trigger(ENV_SEGMENT_ATTACK);
            }
            if (trigger - prev_trigger_ > 0) {
                Trigger(ENV_SEGMENT_ATTACK);
            }
            prev_trigger_ = trigger;
            return value_;
        }

        inline void Reset() {
            prev_trigger_ = 0;
        }

        inline void SetCurve(EnvelopeCurve value) {
            curve_ = value;
        }

        inline void SetLoop(uint16_t value) {
            loop_ = value;
        }

    private:
        // Phase increments for each segment.
        uint32_t increment_[ENV_NUM_SEGMENTS];
  
        // Value that needs to be reached at the end of each segment.
        uint16_t target_[ENV_NUM_SEGMENTS];
  
        // Trigger detector
        uint16_t prev_trigger_;

        // Current segment.
        size_t segment_;
  
        // Start and end value of the current segment.
        uint16_t a_;
        uint16_t b_;
        uint16_t value_;
        uint32_t phase_;

        // Variation
        uint16_t loop_;
        EnvelopeCurve curve_;
    };

}  // namespace braids

#endif  // BRAIDS_ENVELOPE_MOD_H_

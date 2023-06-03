#ifndef BRAIDS_LINENVELOPE_H_
#define BRAIDS_LINENVELOPE_H_

#include "stmlib/stmlib.h"

#include "stmlib/utils/dsp.h"

#include "braids/resources.h"

#include "braids/envelope.h"

namespace braids {

using namespace stmlib;

class LinEnvelope : public Envelope {
 public:
    void Init() {
        target_[ENV_SEGMENT_ATTACK] = 65535;
        target_[ENV_SEGMENT_DECAY] = 0;
        target_[ENV_SEGMENT_DEAD] = 0;
        increment_[ENV_SEGMENT_DEAD] = 0;
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

    inline uint16_t Render() {
        uint32_t increment = increment_[segment_];
        phase_ += increment;
        if (phase_ < increment) {
            value_ = Mix(a_, b_, 65535);
            Trigger(static_cast<EnvelopeSegment>(segment_ + 1));
        }
        if (increment_[segment_]) {
            value_ = Mix(a_, b_, phase_ >> 16);
        }
        return value_;
    }

 private:
  // Phase increments for each segment.
  uint32_t increment_[ENV_NUM_SEGMENTS];
  
  // Value that needs to be reached at the end of each segment.
  uint16_t target_[ENV_NUM_SEGMENTS];
  
  // Current segment.
  size_t segment_;
  
  // Start and end value of the current segment.
  uint16_t a_;
  uint16_t b_;
  uint16_t value_;
  uint32_t phase_;

};

}  // namespace braids

#endif  // BRAIDS_LINENVELOPE_H_

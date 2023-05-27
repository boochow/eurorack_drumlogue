/**
 *  @file header.c
 *  @brief drumlogue SDK unit header
 *
 *  Copyright (c) 2020-2022 KORG Inc. All rights reserved.
 *
 */

#include "unit.h"  // Note: Include common definitions for all units

// ---- Unit header definition  --------------------------------------------------------------------

const __unit_header unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),  // leave as is, size of this header
    .target = UNIT_TARGET_PLATFORM | k_unit_module_synth,  // target platform and module for this unit
    .api = UNIT_API_VERSION,// logue sdk API version against which unit was built
    .dev_id = 0x42636877U,  // "Bchw"
    .unit_id = 0x02010000U, // Product number(02),Unit type(01=Synth),reserved
    .version = 0x00010000U, // major.minor.patch (major<<16 minor<<8 patch).
    .name = "MacroOsc1",
    .num_presets = 0,       // Number of internal presets this unit has
    .num_params = 16,        // Number of parameters for this unit, max 24
    .params = {
        // Format: min, max, center, default, type, fractional, frac. type, <reserved>, name
        // See common/runtime.h for type enum and unit_param_t structure

        // Page 1
        {0, 127, 60, 60, k_unit_param_type_midi_note, 0, 0, 0, {"Note"}},
        {0, 46, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"Shape"}},
        {-256, 255, 0, 0, k_unit_param_type_none, 0, 0, 0, {"Timbre"}},
        {-256, 255, 0, 0, k_unit_param_type_none, 0, 0, 0, {"Color"}},

        // Page 2
        {0, 127, 0, 0, k_unit_param_type_none, 0, 0, 0, {"Attack"}},
        {0, 127, 0, 48, k_unit_param_type_none, 0, 0, 0, {"Decay"}},
        {0, 15, 0, 0, k_unit_param_type_none, 0, 0, 0, {">Timbre"}},
        {0, 15, 0, 0, k_unit_param_type_none, 0, 0, 0, {">Color"}},

        // Page 3
        {0, 6, 0, 6, k_unit_param_type_strings, 0, 0, 0, {"Bits"}},
        {0, 5, 0, 5, k_unit_param_type_strings, 0, 0, 0, {"Rate"}},
        {0, 4, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"Sign."}},
        {0, 15, 0, 15, k_unit_param_type_none, 0, 0, 0, {">VCA"}},

        // Page 4
        {-2, 2, 0, 0, k_unit_param_type_none, 0, 0, 0, {"Octave"}},
        {-127, 127, 0, 0, k_unit_param_type_none, 0, 0, 0, {"Pitch"}},
        {0, 1, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"Flatten"}},
        {0, 4, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"Drift"}},

        // Page 5
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},

        // Page 6
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}}}};

#ifndef DFRAMETRACKERGPFBASEC_H
#define DFRAMETRACKERGPFBASEC_H

/*****************************************************************************
**
**  Definition of the frame tracker Gpf base interface
**
**  Creation date:  2014/12/16
**  Created by:     Haakon Larsson
**
**
**  Copyright (c) 2014 Piql AS. All rights reserved.
**
**  This file is part of the unboxing library
**
*****************************************************************************/

//  PROJECT INCLUDES
//
#include "boxing/frame/frametracker.h"
#include "boxing/graphics/genericframegpf_b1.h"
#include "boxing/unboxer/sampler.h"
#include "boxing/unboxer/cornermark.h"

struct boxing_tracker_gpf_s;

typedef int (*boxing_tracker_gpf_simulated_mode_callback)(struct boxing_tracker_gpf_s * tracker, const boxing_image8 * frame);
typedef int (*boxing_tracker_gpf_analog_mode_callback)(struct boxing_tracker_gpf_s * tracker, const boxing_image8 * frame);


//============================================================================
//  STRUCT:  boxing_tracker_gpf

typedef struct boxing_tracker_gpf_s
{
    boxing_tracker          base;

    boxing_tracker_gpf_simulated_mode_callback track_frame_simulated_mode;
    boxing_tracker_gpf_analog_mode_callback    track_frame_analog_mode;

    boxing_frame *          generic_frame;
    boxing_sampler *        metadata_sampler;
    boxing_sampler *        content_sampler;
    boxing_sampler *        calibration_bar_sampler;

} boxing_tracker_gpf;


//============================================================================

void    boxing_frame_tracker_gpf_base_init(boxing_tracker_gpf * tracker, boxing_frame * generic_frame);
int     boxing_frame_tracker_gpf_base_track_frame(boxing_tracker * tracker, const boxing_image8 * frame);
void    boxing_frame_tracker_gpf_base_free(boxing_tracker * tracker);

#endif // DFRAMETRACKERGPFBASEC_H

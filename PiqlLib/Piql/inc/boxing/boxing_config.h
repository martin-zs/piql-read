#ifndef BOXING_CONFIG_H
#define BOXING_CONFIG_H

/*****************************************************************************
**
**  Definition of the boxing config interface
**
**  Creation date:  2017/06/22
**  Created by:     Oleksandr Ivanov
**
**
**  Copyright (c) 2017 Piql AS. All rights reserved.
**
**  This file is part of the boxingdata library
**
*****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#include "boxing/config.h"

boxing_config *   boxing_get_boxing_config(const char* format_name);
int               boxing_get_format_count();
const char*       boxing_get_configuration_name(int index);
int               boxing_get_control_frame_format_count();
const char*       boxing_get_control_frame_configuration_name(int index);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // BOXING_CONFIG_H

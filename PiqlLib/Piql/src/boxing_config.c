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

//  PROJECT INCLUDES
//

#include "config.h"
#include "boxing_config.h"
#include "boxing/log.h"
#include <stdio.h>
#include "../tests/unboxer/config_source_4kv6.h"
#include "../tests/unboxer/config_source_4kv7.h"
#include "../tests/unboxer/config_source_4kv8.h"
#include "../tests/unboxer/config_source_4kv9.h"
#include "../tests/unboxer/config_source_4kv10.h"
#include "../tests/unboxer/config_source_4k_controlframe_v1.h"
#include "../tests/unboxer/config_source_4k_controlframe_v2.h"
#include "../tests/unboxer/config_source_4k_controlframe_v3.h"
#include "../tests/unboxer/config_source_4k_controlframe_v4.h"
#include "../tests/unboxer/config_source_4k_controlframe_v5.h"
#include "../tests/unboxer/config_source_4k_controlframe_v6.h"
#include "../tests/unboxer/config_source_4k_controlframe_v7.h"

//  DEFINES
//

static config_structure* boxing_formats[] = { &config_source_4kv6, &config_source_4kv7, &config_source_4kv8, &config_source_4kv9, &config_source_4kv10 };
static config_structure* boxing_control_frame_formats[] = { &config_source_v1, &config_source_v2, &config_source_v3, &config_source_v4, &config_source_v5, &config_source_v6, &config_source_v7 };

// PRIVATE INTERFACE
//

static config_structure* find_boxing_format(const char* boxing_format_name);
static DBOOL             format_setting(boxing_config * config, config_structure * source_config_data);
static const char *      get_format_info(config_structure* config);

// PUBLIC BOXING CONFIG FUNCTIONS
//

//----------------------------------------------------------------------------
/*!
 *  \brief The function returns an boxing_config instance by its name.
 *
 *  The function returns an boxing_config instance by its name.
 *  If the input format name is NULL or there is no configuration with this name then function return NULL.
 *
 *  \ingroup unbox
 *  \param[in]   format_name  format name.
 *  \return boxing_config instance.
 */

boxing_config * boxing_get_boxing_config(const char* format_name)
{
    if (format_name == NULL)
    {
        return NULL;
    }

    // Find boxing format by its name
    config_structure* boxing_format = find_boxing_format(format_name);

    // Check the boxing format
    if (boxing_format == NULL)
    {
        DLOG_INFO1("Unsupported boxing format: %s\n", format_name);
        return NULL;
    }

    // Create an instance of a configuration
    boxing_config * config = boxing_config_create();

    // Populate boxing format configuration
    if (!format_setting(config, boxing_format))
    {
        DLOG_INFO("Failed to setting the config data from source!\n");

        boxing_config_free(config);

        return NULL;
    }

    return config;
}


//----------------------------------------------------------------------------
/*!
*  \brief The function provides the number of boxing formats.
*
*  The function provides the number of boxing formats.
*
*  \ingroup unbox
*  \return the number of boxing formats.
*/

int boxing_get_format_count()
{
    return CONFIG_ARRAY_SIZE(boxing_formats);
}


//----------------------------------------------------------------------------
/*!
*  \brief The function provides the name of boxing format.
*
*  The function provides the name of boxing format by its number in the boxing formats array.
*
*  \ingroup unbox
*  \return the name of boxing format.
*/

const char* boxing_get_configuration_name(int number)
{
    return get_format_info(boxing_formats[number]);
}


//----------------------------------------------------------------------------
/*!
 *  \brief The function provides the number of control frame formats.
 *
 *  The function provides the number of control frame formats.
 *
 *  \ingroup unbox
 *  \return the number of control frame formats.
 */

int boxing_get_control_frame_format_count()
{
    return CONFIG_ARRAY_SIZE(boxing_control_frame_formats);
}


//----------------------------------------------------------------------------
/*!
 *  \brief The function provides the name of control frame format.
 *
 *  The function provides the name of control frame format by its number in the control frame formats array.
 *
 *  \ingroup unbox
 *  \return the name of control frame format.
 */

const char* boxing_get_control_frame_configuration_name(int number)
{
    return get_format_info(boxing_control_frame_formats[number]);
}


// PRIVATE BOXING CONFIG FUNCTIONS
//

//----------------------------------------------------------------------------
/*! \ingroup unbox
 *
 *  Searching the boxing format by its name.
 *
 *  \param[in] boxing_format_name   Name of the boxing format.
 *  \param[in] is_control_frame     Sign of the control frame format.
 *  \return pointer to the boxing format config_structure or NULL if the format with the specified name was not found.
 */

static config_structure* find_boxing_format(const char* boxing_format_name)
{
    unsigned int count = CONFIG_ARRAY_SIZE(boxing_formats); // : CONFIG_ARRAY_SIZE(boxing_control_frame_formats);

    // Looking for formats for data frames
    for (unsigned int i = 0; i < count; i++)
    {
        const char* format_info = get_format_info(boxing_formats[i]); // Get the name of the configuration from the configuration structure
        if (format_info && boxing_string_equal(format_info, boxing_format_name) == DTRUE)
        {
            // If we have found the right name, saves the current configuration structure in the pointer boxing_format
            return boxing_formats[i];
        }
    }

    count = CONFIG_ARRAY_SIZE(boxing_control_frame_formats);

    // Looking for formats for control frames
    for (unsigned int i = 0; i < count; i++)
    {
        const char* format_info = get_format_info(boxing_control_frame_formats[i]); // Get the name of the configuration from the configuration structure
        if (format_info && boxing_string_equal(format_info, boxing_format_name) == DTRUE)
        {
            // If we have found the right name, saves the current configuration structure in the pointer boxing_format
            return boxing_control_frame_formats[i];
        }
    }

    return NULL;
}


//----------------------------------------------------------------------------
/*! \ingroup unbox
 *
 *  Reading the configuration data from the source.
 *
 *  \param config               Configuration data
 *  \param source_config_data   Source.
 *  \return DTRUE if reading the configuration data from the source was successful.
 */

static DBOOL format_setting(boxing_config * config, config_structure * source_config_data)
{
    // If the input data is incorrect, then exit with an error
    if (config == NULL || source_config_data == NULL || source_config_data->count == 0)
    {
        return DFALSE;
    }

    config_class * classes = source_config_data->classes;

    // Pass on all classes
    for (unsigned int i = 0; i < source_config_data->count; i++)
    {
        if (classes->version != NULL && classes->version[0] != '\0')
        {
            boxing_config_set_property(config, classes->name, "version", classes->version);
        }

        config_pair * pairs = classes->pairs;

        // Pass on all properties
        for (unsigned int j = 0; j < classes->count; j++)
        {
            boxing_config_set_property(config, classes->name, pairs->key, pairs->value);
            pairs++;
        }

        classes++;
    }

    return DTRUE;
}


//----------------------------------------------------------------------------
/*! \ingroup unbox
 *
 * Get the name of the configuration from the configuration structure.
 *
 *  \param[in] config   Input configuration structure.
 *  \return name of the input configuration structure or NULL if the name does not exist.
 */

static const char* get_format_info(config_structure* config)
{
    config_class * temp_classes = config->classes;
    for (unsigned int i = 0; i < config->count; i++)
    {
        if (boxing_string_equal("FormatInfo", temp_classes->name) == DTRUE)
        {
            config_pair* temp_pairs = temp_classes->pairs;
            for (unsigned int j = 0; j < temp_classes->count; j++)
            {
                if (boxing_string_equal("name", temp_pairs->key) == DTRUE)
                {
                    return temp_pairs->value;
                }
                temp_pairs++;
            }
        }
        temp_classes++;
    }

    return NULL;
}

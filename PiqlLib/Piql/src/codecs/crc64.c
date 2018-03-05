/*****************************************************************************
**
**  Implementation of the crc64 codec interface
**
**  Creation date:  2016/06/28
**  Created by:     Piql
**
**
**  Copyright (c) 2016 Piql AS. All rights reserved.
**
**  This file is part of the boxingdata library
**
*****************************************************************************/

//  PROJECT INCLUDES
//
#include "config.h"
#include "boxing/math/crc32.h"
#include "boxing/codecs/crc64.h"
#include "boxing/log.h"
#include "boxing/math/bitutils.h"
#include "boxing/platform/memory.h"

//  DEFINES
//

#define CODEC_MEMBER(name) (((boxing_codec_crc64 *)codec)->name)
#define CODEC_BASE_MEMBER(name) (((boxing_codec_crc64 *)codec)->base.name)
#define CRC_SIZE sizeof(unsigned long long)

//  PRIVATE INTERFACE
//

static DBOOL decode(void * codec, gvector * data, gvector * erasures, boxing_stats_decode * stats, void* user_data);
static DBOOL encode(void * codec, gvector * data);
static DBOOL set_property(void * codec, const char * name, const g_variant * value);
static DBOOL init_capacity(struct boxing_codec_s *codec, int size);

// PUBLIC CRC64 FUNCTIONS
//

boxing_codec * boxing_crc64_create(GHashTable * properties, const boxing_config * config)
{
    BOXING_UNUSED_PARAMETER( config );
    g_variant * parameter;
    boxing_codec_crc64 * codec = BOXING_MEMORY_ALLOCATE_TYPE(boxing_codec_crc64);
    boxing_codec_init_base((boxing_codec *)codec);
    codec->base.free = boxing_crc64_free;
    codec->base.is_error_correcting = DFALSE;
    codec->base.name = codec_crc64_name;
    codec->base.init_capacity = init_capacity;
   
    codec->base.decode = decode;
    codec->base.encode = encode;
    codec->size = 0;
    codec->polynom = 0;
    codec->seed = 0;
    codec->base.set_property = set_property;

    parameter = g_hash_table_lookup(properties, PARAM_NAME_POLYNOM);
    if (parameter == NULL)
    {
        DLOG_ERROR1("Requires property '%s' to be set", PARAM_NAME_POLYNOM);
        boxing_crc64_free((boxing_codec *)codec);
        return NULL;
    }
    CODEC_MEMBER(polynom) = g_variant_to_ulonglong(parameter);

    parameter = g_hash_table_lookup(properties, PARAM_NAME_SEED);
    if (parameter == NULL)
    {
        DLOG_ERROR1("Requires property '%s' to be set", PARAM_NAME_SEED);
        boxing_crc64_free((boxing_codec *)codec);
        return NULL;
    }
    CODEC_MEMBER(seed) = g_variant_to_ulonglong(parameter);

    codec->crc_calculator = boxing_math_crc64_create(codec->seed, codec->polynom);

    (void)init_capacity((boxing_codec *)codec, CRC_SIZE);
    return (boxing_codec *)codec;
}

void boxing_crc64_free(boxing_codec *codec)
{
    boxing_math_crc64_free(CODEC_MEMBER(crc_calculator));
    boxing_codec_release_base(codec);
    boxing_memory_free(codec);
}

// PRIVATE CRC64 FUNCTIONS
//

static DBOOL decode(void * codec, gvector * data, gvector * erasures, boxing_stats_decode * stats, void* user_data)
{
    BOXING_UNUSED_PARAMETER(erasures);
    BOXING_UNUSED_PARAMETER(user_data);

    unsigned long long crc = boxing_math_crc64_calc_crc_re(CODEC_MEMBER(crc_calculator), CODEC_MEMBER(seed), data->buffer, (unsigned int)data->size);
    gvector_resize(data, (data->size > CRC_SIZE ? (unsigned int)data->size - CRC_SIZE : 0));

    stats->fec_accumulated_amount = 0.0f;
    stats->fec_accumulated_weight = 0.0f;
    stats->resolved_errors = 0;
    stats->unresolved_errors = 0;

    return (crc == 0);
}

static DBOOL encode(void * codec, gvector * data)
{
    unsigned char *ptr;
    unsigned long long crc = boxing_math_crc64_calc_crc_re(CODEC_MEMBER(crc_calculator), CODEC_MEMBER(seed), data->buffer, (unsigned int)data->size);
    gvector_resize(data, (unsigned int)(data->size + CRC_SIZE));
    ptr = ((unsigned char *)data->buffer) + data->size - CRC_SIZE;

    for (unsigned int i = 1; i < CRC_SIZE + 1; i++, ptr++)
    {
        *ptr = (unsigned char)((crc >> (8 * (CRC_SIZE - i))) & 0xff);
    }
    return DTRUE;
}

static DBOOL set_property(void * codec, const char * name, const g_variant * value)
{
    BOXING_UNUSED_PARAMETER(codec);
    BOXING_UNUSED_PARAMETER(name);
    BOXING_UNUSED_PARAMETER(value);

    return DTRUE;
}

static DBOOL init_capacity(struct boxing_codec_s *codec, int size)
{
    if (size < CRC_SIZE)
    {
        return DFALSE;
    }
    codec->encoded_block_size = size;
    codec->encoded_data_size = size;
    codec->decoded_block_size = size - CRC_SIZE;
    codec->decoded_data_size = size - CRC_SIZE;
    return DTRUE;
}
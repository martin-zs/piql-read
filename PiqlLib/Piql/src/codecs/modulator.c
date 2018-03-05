/*****************************************************************************
**
**  Implementation of the modulator codec interface
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
#include "boxing/codecs/modulator.h"
#include "boxing/log.h"
#include "boxing/platform/memory.h"

//  DEFINES
//

#define CODEC_MEMBER(name) (((boxing_codec_modulator *)codec)->name)
#define CODEC_BASE_MEMBER(name) (((boxing_codec_modulator *)codec)->base.name)

//  CONSTANTS
//

static const char codec_name[] = "ModulatorTechnicolor";
static const char *property_name_num_bits_per_pixel_s = "NumBitsPerPixel";


//  PRIVATE INTERFACE
//

static DBOOL codec_decode(void * codec, gvector * data, gvector * erasures, boxing_stats_decode * stats, void* user_data);
static DBOOL codec_encode(void * codec, gvector * data);

// PUBLIC MODULATOR FUNCTIONS
//

boxing_codec * boxing_codec_modulator_create(GHashTable * properties, const boxing_config * config)
{
    BOXING_UNUSED_PARAMETER( config );
    boxing_codec_modulator * codec = BOXING_MEMORY_ALLOCATE_TYPE(boxing_codec_modulator);
    boxing_codec_init_base((boxing_codec *)codec);
    codec->base.free = boxing_codec_modulator_free;
    codec->base.is_error_correcting = DFALSE;
    codec->base.name = codec_name;
    codec->base.decode = codec_decode;
    codec->base.encode = codec_encode;

    g_variant * key = g_hash_table_lookup(properties, property_name_num_bits_per_pixel_s);
    if(key == NULL)
    {
        DLOG_ERROR1( "Requires property '%s' to be set", property_name_num_bits_per_pixel_s);
        boxing_codec_modulator_free((boxing_codec *)codec);
        return NULL;
    }

    unsigned int uvalue = g_variant_to_uint(key);
    if (uvalue == 0 || uvalue > 8)
    {
        DLOG_ERROR2( "Modulator::setProperty() unsupported value for property %s : %u", property_name_num_bits_per_pixel_s, uvalue);
        boxing_codec_modulator_free((boxing_codec *)codec);
        return NULL;
    }
    codec->num_bits_per_pixel = uvalue;
    codec->base.encoded_symbol_size = codec->num_bits_per_pixel;

    codec->base.decoded_block_size = codec->base.encoded_symbol_size;
    codec->base.encoded_block_size = codec->base.decoded_symbol_size;

    return (boxing_codec *)codec;
}

void boxing_codec_modulator_free(boxing_codec *codec)
{
    boxing_codec_release_base(codec);
    boxing_memory_free(codec);
}

// PRIVATE MODULATOR FUNCTIONS
//

static DBOOL codec_decode(void * codec, gvector * data, gvector * erasures, boxing_stats_decode * stats, void* user_data)
{
    BOXING_UNUSED_PARAMETER(erasures);
    BOXING_UNUSED_PARAMETER(stats);
    BOXING_UNUSED_PARAMETER(user_data);

    gvector * demodulated;
    unsigned char *data_source = (unsigned char *)data->buffer;
    if (CODEC_MEMBER(num_bits_per_pixel) == 1) {
        demodulated = gvector_create(1, (data->size + 7) / 8);
        unsigned int counter = 0;
        unsigned int to = ((unsigned int)data->size / 8) * 8;
        for (unsigned int i_byte = 0; i_byte<to;) {
            unsigned int byte_value = 0;
            for (int i_bit = 0; i_bit<8; i_bit++, data_source++, i_byte++) {
                byte_value <<= 1;
                byte_value |= 0x01 & *data_source;
            }
            GVECTORN(demodulated, unsigned char, counter++) = (unsigned char)byte_value;
        }
        if (counter < demodulated->size)
        {
            unsigned int byte_value = 0;
            for (int i_bit = 0; i_bit < 8 && data_source != (unsigned char *)data->buffer + data->size; i_bit++, data_source++) {
                byte_value <<= 1;
                byte_value |= 0x01 & *data_source;
            }

            GVECTORN(demodulated, unsigned char, counter++) = (unsigned char)byte_value;
        }
    }
    else if (CODEC_MEMBER(num_bits_per_pixel) == 2) {
        const unsigned char lut[4] =
        {
            0,
            1,
            3,
            2,
        };

        // ensure data alignment
        int new_size = (int)data->size - ((int)data->size % (1 << CODEC_MEMBER(num_bits_per_pixel)));
        gvector_resize(data, new_size);

        demodulated = gvector_create(1, (data->size + 3) / 4);
        unsigned int counter = 0;

        char * data_source = (char *)data->buffer;
        const char * data_end = data_source + data->size;

        while (data_source != data_end)
        {
            unsigned int byte_value = 0;

            for (int i_bit = 0; i_bit<4; i_bit++)
            {
                byte_value <<= 2;
                byte_value |= lut[0x03 & *data_source++];
            }

            GVECTORN(demodulated, unsigned char, counter++) = (unsigned char)byte_value;
        }

    }
    else {
        return DFALSE;
    }

    gvector_swap(data, demodulated);
    gvector_free(demodulated);
    return DTRUE;
}

static DBOOL codec_encode(void * codec, gvector * data)
{
    gvector * modulated;
    unsigned int counter = 0;
    if (CODEC_MEMBER(num_bits_per_pixel) == 1)
    {
        modulated = gvector_create(1, data->size * 8);
        for (unsigned int i_byte = 0; i_byte<data->size; i_byte++)
        {
            unsigned int byte_value = (unsigned int)((256 + (int)GVECTORN(data, char, i_byte)) % 256);
            for (int i_bit = 0; i_bit<8; i_bit++)
            {
                GVECTORN(modulated, char, counter++) = (byte_value >> (7 - i_bit)) & 0x01;
            }
        }
    }
    else if (CODEC_MEMBER(num_bits_per_pixel) == 2)
    {
        modulated = gvector_create(1, data->size * 4);
        char* buffer = (char *)modulated->buffer;
        for (unsigned int i_byte = 0; i_byte<data->size; i_byte++)
        {
            unsigned int byte_value = (unsigned int)((256 + (int)GVECTORN(data, char, i_byte)) % 256);
            for (int i_tuple = 0; i_tuple<4; i_tuple++)
            {
                unsigned int tuple_value = (byte_value >> ((3 - i_tuple) * 2)) & 0x03;

                char mod[4] = { 0, 1, 3, 2 };
                *buffer = mod[tuple_value];
                buffer++;
            }
        }
    }
    else if (CODEC_MEMBER(num_bits_per_pixel) == 8)
    {
        modulated = gvector_create(1, data->size);
        memcpy(modulated->buffer, data->buffer, data->size);
    }
    else
    {
        return DFALSE;
    }

    gvector_swap(data, modulated);
    gvector_free(modulated);
    return DTRUE;
}

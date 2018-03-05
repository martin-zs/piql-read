/*****************************************************************************
**
**  Implementation of the synchronization point inserter interface
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
#include "boxing/codecs/syncpointinserter.h"
#include "boxing/log.h"
#include "boxing/graphics/genericframefactory.h"
#include "boxing/graphics/genericframe.h"
#include "boxing/platform/memory.h"

//  CONSTANTS
//

static const char codec_name[] = "SyncPointInserter";
static const int  Horizontal = 1;
static const int  Vertical = 2;

// Definition of property names
static const char *property_name_sync_point_radius_s = "SyncPointRadiusPixel";
static const char *property_name_sync_point_distance_s = "SyncPointDistancePixel";    // IN -  QVariant(int)
static const char *property_name_sync_point_h_distance_s = "SyncPointHDistancePixel";    // IN -  QVariant(int)
static const char *property_name_sync_point_v_distance_s = "SyncPointVDistancePixel";
static const char *property_name_sync_point_h_offset_s = "SyncPointHOffsetPixel";
static const char *property_name_sync_point_v_offset_s = "SyncPointVOffsetPixel";
static const char *property_name_image_size_s = "ImageSizePixel";              // IN -  QVariant(QSize)
static const char *property_name_data_orientation_s = "DataOrientation";             // IN -  QVariant(int = Qt::Orientation)
static const char *property_name_value_sync_point_background_s = "ValueSyncPointBackground";    // IN -  QVariant(int)
static const char *property_name_value_sync_point_foreground_s = "ValueSyncPointForeground";    // IN -  QVariant(int)
static const char *property_name_num_bits_per_pixel_s = "NumBitsPerPixel";         // IN -  QVariant(int)

//  DEFINES
//

#define CODEC_MEMBER(name) (((boxing_codec_syncpointinserter *)codec)->name)
#define CODEC_BASE_MEMBER(name) (((boxing_codec_syncpointinserter *)codec)->base.name)

//  PRIVATE INTERFACE
//

static void  update_syncpointmasks(boxing_codec_syncpointinserter * codec);
static DBOOL codec_decode(void * codec, gvector * data, gvector * erasures, boxing_stats_decode * stats, void* user_data);
static DBOOL codec_encode(void * codec, gvector * data);
static DBOOL init_capacity(struct boxing_codec_s *codec, int size);

// PUBLIC SYNC POINT INSERTER FUNCTIONS
//

boxing_codec * boxing_codec_syncpointinserter_create(GHashTable * properties, const boxing_config * config)
{
    boxing_codec_syncpointinserter * codec = BOXING_MEMORY_ALLOCATE_TYPE(boxing_codec_syncpointinserter);
    boxing_codec_init_base((boxing_codec *)codec);
    codec->base.free = boxing_codec_syncpointinserter_free;
    codec->base.is_error_correcting = DFALSE;
    codec->base.name = codec_name;
    codec->base.decode = codec_decode;
    codec->base.encode = codec_encode;
    codec->base.init_capacity = init_capacity;

    codec->property_sync_point_centers_m.buffer = NULL;
    codec->property_sync_point_areas_m.buffer = NULL;
    codec->bitarray_sync_point_background = NULL;
    codec->bitarray_sync_point_foreground = NULL;
    codec->is_valid = DFALSE;

    g_variant * image_size_pixel = (g_variant *)g_hash_table_lookup(properties, property_name_image_size_s);
    if(image_size_pixel == NULL)
    {
        boxing_pointi dimension;
        boxing_pointi tilesize;

        boxing_frame * frame = boxing_generic_frame_factory_create(config);
        boxing_generic_container * container = frame->container(frame);
        dimension = container->dimension(container);
        tilesize = container->get_tile_size_enum(container, BOXING_CONTENT_TYPE_DIGITAL);
        boxing_generic_frame_factory_free(frame);

        boxing_pointi pointi;
        pointi.x = dimension.x/tilesize.x;
        pointi.y = dimension.y/tilesize.y;
        g_hash_table_replace(properties, boxing_string_clone(property_name_image_size_s), g_variant_create_pointi(pointi));
    }

    g_variant * key = (g_variant *)g_hash_table_lookup(properties, property_name_image_size_s);
    if(key == NULL)
    {
        DLOG_ERROR1( "Requires property '%s' to be set", property_name_image_size_s);
        boxing_codec_syncpointinserter_free((boxing_codec *)codec);
        return NULL;
    }
    codec->property_image_size_m = g_variant_to_pointi(key, NULL);


    g_variant * property_sync_point_h_distance = (g_variant *)g_hash_table_lookup(properties, property_name_sync_point_h_distance_s);
    g_variant * property_sync_point_v_distance = (g_variant *)g_hash_table_lookup(properties, property_name_sync_point_v_distance_s);

    if(!property_sync_point_h_distance && !property_sync_point_v_distance)
    {
        key = (g_variant *)g_hash_table_lookup(properties, property_name_sync_point_distance_s);
        if(key == NULL)
        {
            DLOG_ERROR1( "Requires property '%s' to be set", property_name_sync_point_distance_s);
            boxing_codec_syncpointinserter_free((boxing_codec *)codec);
            return NULL;
        }
        codec->property_sync_point_h_distance_m = g_variant_to_int(key);
        codec->property_sync_point_v_distance_m = codec->property_sync_point_h_distance_m;
    }
    else if(property_sync_point_h_distance && property_sync_point_v_distance)
    {
        codec->property_sync_point_h_distance_m = g_variant_to_int(property_sync_point_h_distance);
        codec->property_sync_point_v_distance_m = g_variant_to_int(property_sync_point_v_distance);
    }
    else
    {
        DLOG_ERROR2( "Requires properties '%s' and '%s' to be set", property_name_sync_point_h_distance_s, property_name_sync_point_v_distance_s);
        boxing_codec_syncpointinserter_free((boxing_codec *)codec);
        return NULL;
    }

    codec->property_sync_point_h_offset_m = -1;
    key = (g_variant *)g_hash_table_lookup(properties, property_name_sync_point_h_offset_s);
    if(key)
    {
        codec->property_sync_point_h_offset_m = g_variant_to_int(key);
    }
    
    codec->property_sync_point_v_offset_m = -1;
    key = (g_variant *)g_hash_table_lookup(properties, property_name_sync_point_v_offset_s);
    if(key)
    {
        codec->property_sync_point_v_offset_m = g_variant_to_int(key);
    }

    key = (g_variant *)g_hash_table_lookup(properties, property_name_sync_point_radius_s);
    if(key == NULL)
    {
        DLOG_ERROR1( "Requires property '%s' to be set", property_name_sync_point_radius_s);
        boxing_codec_syncpointinserter_free((boxing_codec *)codec);
        return NULL;
    }
    codec->property_sync_point_radius_m = g_variant_to_int(key);

    key = (g_variant *)g_hash_table_lookup(properties, property_name_num_bits_per_pixel_s);
    if(key == NULL)
    {
        DLOG_ERROR1( "Requires property '%s' to be set", property_name_num_bits_per_pixel_s);
        boxing_codec_syncpointinserter_free((boxing_codec *)codec);
        return NULL;
    }
    codec->num_bits_per_pixel_m = g_variant_to_int(key);
    codec->property_value_sync_point_background_m = 0;
    codec->property_value_sync_point_foreground_m = (1 << codec->num_bits_per_pixel_m) - 1;

    key = (g_variant *)g_hash_table_lookup(properties, property_name_data_orientation_s);
    if(key == NULL)
    {
        DLOG_ERROR1( "Requires property '%s' to be set", property_name_data_orientation_s);
        boxing_codec_syncpointinserter_free((boxing_codec *)codec);
        return NULL;
    }
    codec->property_data_orientation_m = g_variant_to_int(key);
    if (codec->property_data_orientation_m != Horizontal && codec->property_data_orientation_m != Vertical)
    {
        DLOG_ERROR1( "property '%s' has invalid value", property_name_data_orientation_s);
        boxing_codec_syncpointinserter_free((boxing_codec *)codec);
        return NULL;
    }

    key = (g_variant *)g_hash_table_lookup(properties, property_name_value_sync_point_background_s);
    if(key != NULL)
    {
        codec->property_value_sync_point_background_m = g_variant_to_int(key);
    }
    

    key = (g_variant *)g_hash_table_lookup(properties, property_name_value_sync_point_foreground_s);
    if(key != NULL)
    {
        codec->property_value_sync_point_foreground_m = g_variant_to_int(key);
    }

    update_syncpointmasks(codec);

    init_capacity((boxing_codec* )codec, codec->property_image_size_m.x * codec->property_image_size_m.y);

    codec->base.decoded_symbol_size = codec->num_bits_per_pixel_m;
    codec->base.encoded_symbol_size = codec->num_bits_per_pixel_m;

    return (boxing_codec *)codec;
}

void boxing_codec_syncpointinserter_free(boxing_codec* codec)
{
    boxing_codec_release_base((boxing_codec *)codec);
    boxing_memory_free(CODEC_MEMBER(bitarray_sync_point_background));
    boxing_memory_free(CODEC_MEMBER(bitarray_sync_point_foreground));
    boxing_memory_free(CODEC_MEMBER(property_sync_point_areas_m).buffer);
    boxing_memory_free(CODEC_MEMBER(property_sync_point_centers_m).buffer);
    boxing_memory_free(codec);
}

// PRIVATE SYNC POINT INSERTER FUNCTIONS
//

static DBOOL codec_decode(void * codec, gvector * data, gvector * erasures, boxing_stats_decode * stats, void* user_data)
{
    BOXING_UNUSED_PARAMETER(erasures);
    BOXING_UNUSED_PARAMETER(stats);
    BOXING_UNUSED_PARAMETER(user_data);

    if (CODEC_MEMBER(is_valid) == DFALSE)
    {
        return DFALSE;
    }

    // Check if all properties are set

    if ((size_t)CODEC_MEMBER(property_image_size_m).x*CODEC_MEMBER(property_image_size_m).y > data->size)
    {
        return DFALSE;
    }

    int image_width = CODEC_MEMBER(property_image_size_m).x;
    int image_height = CODEC_MEMBER(property_image_size_m).y;

    gvector * result = gvector_create(1, image_height * image_width);

    unsigned int counter = 0;

    if (CODEC_MEMBER(property_data_orientation_m) == Horizontal) {

        char * data_source = (char *)data->buffer;
        char * sync_pointer_background = CODEC_MEMBER(bitarray_sync_point_background);
        char * sync_pointer_foreground = CODEC_MEMBER(bitarray_sync_point_foreground);
        for (int iy = 0; iy<image_height; iy++)
        {
            for (int ix = 0; ix<image_width; ix++)
            {
                if ((*sync_pointer_background == 0) &&
                    (*sync_pointer_foreground == 0))
                {
                    GVECTORN(result, char, counter++) = *data_source;
                }
                data_source++;
                sync_pointer_background++;
                sync_pointer_foreground++;
            }
        }

    }
    else if (CODEC_MEMBER(property_data_orientation_m) == Vertical) {

        char * data_source = (char *)data->buffer;
        char * sync_pointer_background = CODEC_MEMBER(bitarray_sync_point_background);
        char * sync_pointer_foreground = CODEC_MEMBER(bitarray_sync_point_foreground);
        for (int ix = 0; ix<image_width; ix++)
        {
            for (int iy = 0; iy<image_height; iy++)
            {
                if ((sync_pointer_background[ix + iy * image_width] == 0) &&
                    (sync_pointer_foreground[ix + iy * image_width] == 0))
                {
                    GVECTORN(result, char, counter++) = *data_source;
                }
                data_source++;
            }
        }
    }
    else
    {
        gvector_free(result);
        return DFALSE;
    }

    gvector_resize(result, counter);
    gvector_swap(data, result);
    gvector_free(result);
    return DTRUE;
}

static DBOOL codec_encode(void * codec, gvector * data)
{
    if (CODEC_MEMBER(is_valid) == DFALSE)
    {
        return DFALSE;
    }

    // Check if all properties are set

    gvector * result = gvector_create(1, CODEC_MEMBER(property_image_size_m).x*CODEC_MEMBER(property_image_size_m).y);

    char sync_point_background = (char)CODEC_MEMBER(property_value_sync_point_background_m);
    char sync_point_foreground = (char)CODEC_MEMBER(property_value_sync_point_foreground_m);

    char * sync_pointer_background = CODEC_MEMBER(bitarray_sync_point_background);
    char * sync_pointer_foreground = CODEC_MEMBER(bitarray_sync_point_foreground);

    if (CODEC_MEMBER(property_data_orientation_m) == Horizontal)
    {
        char* result_buffer = (char *)result->buffer;

        unsigned int index_data = 0;
        const char * result_buffer_end = result_buffer + CODEC_MEMBER(property_image_size_m).x * CODEC_MEMBER(property_image_size_m).y;
        char *data_buffer = (char *)data->buffer;
        while (result_buffer != result_buffer_end)
        {
            if (*sync_pointer_background == 1)
            {
                *result_buffer = sync_point_background;
            }
            else if (*sync_pointer_foreground == 1)
            {
                *result_buffer = sync_point_foreground;
            }
            else
            {
                if (index_data < data->size)
                {
                    *result_buffer = data_buffer[index_data++];
                }
                else
                {
                    *result_buffer = (char)0;
                }
            }
            result_buffer++;
            sync_pointer_background++;
            sync_pointer_foreground++;
        }
    }
    else if (CODEC_MEMBER(property_data_orientation_m) == Vertical)
    {
        unsigned int index_data = 0;
        int image_width = CODEC_MEMBER(property_image_size_m).x;
        int image_height = CODEC_MEMBER(property_image_size_m).y;
        unsigned int counter = 0;
        for (int ix = 0; ix<image_width; ix++)
        {
            for (int iy = 0; iy<image_height; iy++)
            {
                if (sync_pointer_background[iy*image_width + ix] == 1)
                {
                    GVECTORN(result, char, counter++) = sync_point_background;
                }
                else if (sync_pointer_foreground[iy*image_width + ix] == 1)
                {
                    GVECTORN(result, char, counter++) = sync_point_foreground;
                }
                else
                {
                    if (index_data < data->size)
                    {
                        GVECTORN(result, char, counter++) = GVECTORN(data, char, index_data++);
                    }
                    else
                    {
                        GVECTORN(result, char, counter++) = 0;
                    }
                }
            }
        }
    }
    else
    {
        return DFALSE;
    }

    gvector_swap(data, result);
    gvector_free(result);
    return DTRUE;
}

static void update_syncpointmasks(boxing_codec_syncpointinserter* codec)
{
    // Check if all necessary properties are set
    {

        int sync_point_offset_h;
        int num_sync_points_h;
        if (codec->property_sync_point_h_offset_m < 0)
        {
            num_sync_points_h = (codec->property_image_size_m.x - 2 * codec->property_sync_point_radius_m - 1) /
                codec->property_sync_point_h_distance_m + 1;
            sync_point_offset_h = (codec->property_image_size_m.x - ((num_sync_points_h - 1)*codec->property_sync_point_h_distance_m + 1)) / 2;
        }
        else
        {
            num_sync_points_h = ((codec->property_image_size_m.x - codec->property_sync_point_h_offset_m) - 2 * codec->property_sync_point_radius_m - 1) /
                codec->property_sync_point_h_distance_m + 1;
            sync_point_offset_h = codec->property_sync_point_h_offset_m;
        }


        int sync_point_offset_v;
        int num_sync_points_v;
        if (codec->property_sync_point_v_offset_m < 0)
        {
            num_sync_points_v = ((codec->property_image_size_m.y) - 2 * codec->property_sync_point_radius_m - 1) /
                codec->property_sync_point_v_distance_m + 1;
            sync_point_offset_v = (codec->property_image_size_m.y - ((num_sync_points_v - 1)*codec->property_sync_point_v_distance_m + 1)) / 2;
        }
        else
        {
            num_sync_points_v = ((codec->property_image_size_m.y - codec->property_sync_point_v_offset_m) - 2 * codec->property_sync_point_radius_m - 1) /
                codec->property_sync_point_v_distance_m + 1;
            sync_point_offset_v = codec->property_sync_point_v_offset_m;
        }

        codec->sync_point_pixel_count_m = (codec->property_sync_point_radius_m*2+1)*(codec->property_sync_point_radius_m*2+1)*
                num_sync_points_h*num_sync_points_v;

        boxing_memory_free(codec->property_sync_point_centers_m.buffer);
        boxing_memory_free(codec->property_sync_point_areas_m.buffer);
        gvector_create_inplace(&codec->property_sync_point_centers_m, sizeof(boxing_pointi), num_sync_points_v * num_sync_points_h);
        gvector_create_inplace(&codec->property_sync_point_areas_m, sizeof(boxing_recti), num_sync_points_v * num_sync_points_h);
        int counter = 0;
        int sync_point_radius = codec->property_sync_point_radius_m;
        int sync_point_h_distance = codec->property_sync_point_h_distance_m;
        int sync_point_v_distance = codec->property_sync_point_v_distance_m;
        for (int iy=0; iy<num_sync_points_v; iy++) {
            for (int ix=0; ix<num_sync_points_h; ix++) {
                int posx = sync_point_offset_h + ix*sync_point_h_distance;
                int posy = sync_point_offset_v + iy*sync_point_v_distance;

                GVECTORN(&codec->property_sync_point_centers_m, boxing_pointi, counter).x = posx;
                GVECTORN(&codec->property_sync_point_centers_m, boxing_pointi, counter).y = posy;

                GVECTORN(&codec->property_sync_point_areas_m, boxing_recti, counter).x = posx-sync_point_radius;
                GVECTORN(&codec->property_sync_point_areas_m, boxing_recti, counter).y = posy-sync_point_radius;
                GVECTORN(&codec->property_sync_point_areas_m, boxing_recti, counter).width = 2*sync_point_radius+1;
                GVECTORN(&codec->property_sync_point_areas_m, boxing_recti, counter).height = 2*sync_point_radius+1;
                ++counter;
            }
        }
    }
 
    // Create the mask bitmaps
    char * mask_background;
    char * mask_foreground;

    // Delete old buffers
    boxing_memory_free(codec->bitarray_sync_point_background);
    boxing_memory_free(codec->bitarray_sync_point_foreground);

    mask_background = BOXING_MEMORY_ALLOCATE_TYPE_ARRAY_CLEAR(char, codec->property_image_size_m.x * codec->property_image_size_m.y);
    mask_foreground = BOXING_MEMORY_ALLOCATE_TYPE_ARRAY_CLEAR(char, codec->property_image_size_m.x * codec->property_image_size_m.y);
    int image_width = codec->property_image_size_m.x;
#if !defined NDEBUG
    int image_height = codec->property_image_size_m.y;
#endif
    {
        for (unsigned int i=0; i<codec->property_sync_point_areas_m.size; i++)
        {
            boxing_recti sync_point_area = GVECTORN(&codec->property_sync_point_areas_m, boxing_recti, i);
            assert(sync_point_area.y >= 0 && sync_point_area.x >= 0);

            for (int iy=0;iy<sync_point_area.height;iy++)
            {
                assert(sync_point_area.y+iy < image_height);

                char * y_position = mask_background + (sync_point_area.y+iy) * image_width;
                for (int ix=0;ix<sync_point_area.width;ix++)
                {
                    assert(sync_point_area.x+ix < image_width);
                    y_position[sync_point_area.x+ix] = 1;
                }
            }
        }
    }
    {
        for (unsigned int i=0; i<codec->property_sync_point_centers_m.size; i++)
        {
            boxing_pointi sync_point_center = GVECTORN(&codec->property_sync_point_centers_m, boxing_pointi, i);
            assert(sync_point_center.y >= 0 && sync_point_center.x >= 0);
            assert(sync_point_center.y < image_height && sync_point_center.x < image_width);

            mask_background[sync_point_center.y * image_width + sync_point_center.x] = 0;
            mask_foreground[sync_point_center.y * image_width + sync_point_center.x] = 1;
        }
    }

    // Store the masks
    codec->bitarray_sync_point_background = mask_background;
    codec->bitarray_sync_point_foreground = mask_foreground;
    codec->is_valid = DTRUE;
}

static DBOOL init_capacity(struct boxing_codec_s *codec, int size)
{
    boxing_codec_syncpointinserter * syncpointinserter = (boxing_codec_syncpointinserter *)codec;
    if (size != (syncpointinserter->property_image_size_m.x * syncpointinserter->property_image_size_m.y))
    {
        return DFALSE;
    }
    codec->encoded_data_size = size;
    codec->encoded_block_size = codec->encoded_data_size;
    codec->decoded_data_size = size - syncpointinserter->sync_point_pixel_count_m;;
    codec->decoded_block_size = codec->decoded_data_size;
    return DTRUE;
}
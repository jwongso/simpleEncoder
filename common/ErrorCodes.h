// -------------------------------------------------------------------------------------------------
//
// Copyright (C) Juniarto Saputra (juniarto.wongso@gmail.com). All rights reserved.
//
// This software, including documentation, is protected by copyright controlled by
// me. All rights are reserved. Copying, including reproducing, storing,
// adapting or translating, any or all of this material requires the prior written
// consent of Juniarto Saputra.
//
// -------------------------------------------------------------------------------------------------

#ifndef ERRORCODES_H
#define ERRORCODES_H

#define RETURN_ERR_IF_ERROR( func, err ) \
do \
{ \
    int ret = ( func ); \
    if ( ret != 0 ) \
    { \
        fprintf(stderr, "Error: %s returned %d at %s:%d", #func, ret, __FILE__, __LINE__ ); \
        return err; \
    } \
} while (0)

#define RETURN_IF_ERROR( func ) \
do \
{ \
    int ret = ( func ); \
    if ( ret != 0 ) \
    { \
        fprintf(stderr, "Error: %s returned %d at %s:%d", #func, ret, __FILE__, __LINE__ ); \
        return ret; \
    } \
} while (0)

namespace common
{

/**
 * Error codes that can be returned by encoder components.
 */
enum ErrorCode
{
    ERROR_NONE = 0,
    ERROR_NOT_FOUND,
    ERROR_READ_FILE,
    /// utils
    ERROR_WAVE_INVALID,
    /// core specific error codes
    ERROR_NOT_IMPLEMENTED,
    ERROR_PTHREAD_CREATE,
    ERROR_PTHREAD_JOIN,
    ERROR_LAME,
    ERROR_BUSY,
    ERROR_IO
};

} // core

#endif // ENCODER_ERRORCODES_H

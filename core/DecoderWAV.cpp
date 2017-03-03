// -------------------------------------------------------------------------------------------------
//
// Copyright (C) all of the contributors. All rights reserved.
//
// This software, including documentation, is protected by copyright controlled by
// contributors. All rights are reserved. Copying, including reproducing, storing,
// adapting or translating, any or all of this material requires the prior written
// consent of all contributors.
//
// -------------------------------------------------------------------------------------------------

#include "DecoderWAV.h"
#include "utils/Mp3FileWrapper.h"
#include "utils/Helper.h"

#include <lame/lame.h>
#include <iostream>
#include <sstream>
#include <fstream>

namespace core
{

namespace
{
const std::string LAME = "Lame ";
const std::string OUTPUT_EXT = ".wav";
static pthread_mutex_t process_mutex = PTHREAD_MUTEX_INITIALIZER;
} // namespace

// -------------------------------------------------------------------------------------------------

DecoderWAV::DecoderWAV( common::AudioFormatType input_type, uint16_t thread_number )
    : Decoder( input_type, common::AudioFormatType::WAV )
    , m_decoder_version( LAME + get_lame_version( ) )
    , m_thread_number( thread_number )
    , m_cancelled( false )
{
}

// -------------------------------------------------------------------------------------------------

DecoderWAV::~DecoderWAV( )
{
}

// -------------------------------------------------------------------------------------------------

const std::string&
DecoderWAV::get_decoder_version( ) const
{
    return m_decoder_version;
}

// -------------------------------------------------------------------------------------------------

void*
DecoderWAV::processing_files( void* arg )
{
    auto error = common::ErrorCode::ERROR_NONE;
    core::DecoderWAV::DecoderThreadArg* thread_arg = ( core::DecoderWAV::DecoderThreadArg* )arg;
    const uint32_t thread_id = thread_arg->thread_id;
    const auto& callback = thread_arg->callback;

    while ( true )
    {
        std::string input_file;

        pthread_mutex_lock( &process_mutex );

        for ( auto it = thread_arg->input_files->begin( );
              it != thread_arg->input_files->end( ); it++ )
        {
            if ( !it->second )
            {
                input_file = it->first;
                it->second = true;

                break;
            }
        }

        if ( *thread_arg->cancelled )
        {
            error = common::ErrorCode::ERROR_CANCELLED;
            fprintf( stderr, "Cancel running operations at %s:%d\n", __FILE__, __LINE__ );
            utils::Helper::log( callback, thread_id,
                 "Cancelled " + input_file );

            break;
        }

        pthread_mutex_unlock( &process_mutex );

        if ( input_file.empty( ) )
        {
            break;
        }

        utils::Helper::log( callback, thread_id, "Processing " + input_file );

        std::string output_file = utils::Helper::generate_output_file( input_file, OUTPUT_EXT );

        // TODO DECODING


        utils::Helper::log( callback, thread_id, "Process done, output file: " + output_file );
    }

    pthread_exit( ( void* )error );
}

// -------------------------------------------------------------------------------------------------

common::ErrorCode
DecoderWAV::start_decoding( )
{
    if ( m_input_files.empty( ) )
    {
        return common::ERROR_NOT_FOUND;
    }

    m_to_be_decoded_files.clear( );

    for( const auto& file : m_input_files )
    {
        m_to_be_decoded_files[ file ] = false;
    }

    pthread_t threads[ m_thread_number ];
    pthread_attr_t thread_attr;
    pthread_attr_init( &thread_attr );
    pthread_attr_setdetachstate( &thread_attr, PTHREAD_CREATE_JOINABLE );

    for ( int i = 0; i < m_thread_number; i++ )
    {
        DecoderThreadArg thread_arg;
        thread_arg.thread_id = ( i + 1 );
        thread_arg.input_files = &m_to_be_decoded_files;
        thread_arg.cancelled = &m_cancelled;

        auto callback = [ this ] ( const std::string& key, const std::string& value )
        {
            on_decoding_status( key, value );
        };

        thread_arg.callback = std::move( callback );

        RETURN_ERR_IF_ERROR( pthread_create( &threads[ i ],
                                             &thread_attr,
                                             DecoderWAV::processing_files,
                                             ( void* )&thread_arg ),
                             common::ErrorCode::ERROR_PTHREAD_CREATE );
    }

    pthread_attr_destroy( &thread_attr );

    for ( int i = 0; i < m_thread_number; i++ )
    {
        RETURN_ERR_IF_ERROR( pthread_join( threads[ i ], NULL ),
                             common::ErrorCode::ERROR_PTHREAD_JOIN );
    }

#ifdef ENABLE_LOG
    std::ofstream ofs( DECODER_LOG_FILE );
    if ( ofs.is_open( ) )
    {
        for ( const auto& status : m_status )
        {
            ofs << status << std::endl;
        }
    }
    ofs.close( );
#endif

    m_cancelled = false;

    return common::ErrorCode::ERROR_NONE;
}

// -------------------------------------------------------------------------------------------------

common::ErrorCode
DecoderWAV::cancel_decoding( )
{
    m_cancelled = true;
}

// -------------------------------------------------------------------------------------------------

void
DecoderWAV::on_decoding_status( const std::string& key, const std::string& value )
{
    std::lock_guard< std::mutex > guard( m_mutex );

    std::string log = key + " " + value;
    m_status.emplace_back( log );

    std::cout << log << std::endl;
}

// -------------------------------------------------------------------------------------------------

} // core

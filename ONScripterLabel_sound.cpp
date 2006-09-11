/* -*- C++ -*-
 *
 *  ONScripterLabel_sound.cpp - Methods for playing sound
 *
 *  Copyright (c) 2001-2006 Ogapee. All rights reserved.
 *
 *  ogapee@aqua.dti2.ne.jp
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ONScripterLabel.h"
#if defined(LINUX)
#include <signal.h>
#endif

#if defined(USE_AVIFILE)
#include "AVIWrapper.h"
#endif

struct WAVE_HEADER{
    char chunk_riff[4];
    char riff_length[4];
    char fmt_id[8];
    char fmt_size[4];
    char data_fmt[2];
    char channels[2];
    char frequency[4];
    char byte_size[4];
    char sample_byte_size[2];
    char sample_bit_size[2];

    char chunk_id[4];
    char data_length[4];
} header;

extern bool ext_music_play_once_flag;

extern "C"{
    extern void mp3callback( void *userdata, Uint8 *stream, int len );
    extern void oggcallback( void *userdata, Uint8 *stream, int len );
    extern Uint32 SDLCALL cdaudioCallback( Uint32 interval, void *param );
#if defined(MACOSX) && defined(INSANI)
	extern Uint32 SDLCALL midiSDLCallback( Uint32 interval, void *param );
#endif
}
extern void midiCallback( int sig );
extern void musicCallback( int sig );
extern SDL_TimerID timer_cdaudio_id;

#if defined(MACOSX) && defined(INSANI)
extern SDL_TimerID timer_midi_id;
#endif

#define TMP_MIDI_FILE "tmp.mid"
#define TMP_MUSIC_FILE "tmp.mus"

extern long decodeOggVorbis(OVInfo *ovi, unsigned char *buf_dst, long len, bool do_rate_conversion)
{
    int current_section;
    long total_len = 0;

    char *buf = (char*)buf_dst;
    if (do_rate_conversion && ovi->cvt.needed){
        len = len * ovi->mult1 / ovi->mult2;
        if (ovi->cvt_len < len*ovi->cvt.len_mult){
            if (ovi->cvt.buf) delete[] ovi->cvt.buf;
            ovi->cvt.buf = new unsigned char[len*ovi->cvt.len_mult];
            ovi->cvt_len = len*ovi->cvt.len_mult;
        }
        buf = (char*)ovi->cvt.buf;
    }

#if defined(USE_OGG_VORBIS)
    while(1){
#if defined(INTEGER_OGG_VORBIS)
        long src_len = ov_read( &ovi->ovf, buf, len, &current_section);
#else
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
        long src_len = ov_read( &ovi->ovf, buf, len, 0, 2, 1, &current_section);
#else
        /*
		 * In practice, on Mac OS X PPC, flipping the endian flag here to 1
		 * actually causes the endianness to be *wrong* -- causing static
		 * as output whenever ogg vorbis is used.  Apparently you're doing
		 * this endian flipping already somewhere else; therefore, the one here
		 * is not only unnecessary, it causes absolutely undesired behavior.
		 * Recommend for integration. [Seung Park, 20060707]
		 */
        long src_len = ov_read( &ovi->ovf, buf, len, 0, 2, 1, &current_section);
#endif
#endif
        if (src_len <= 0) break;

        long dst_len = src_len;
        if (do_rate_conversion && ovi->cvt.needed){
            ovi->cvt.len = src_len;
            SDL_ConvertAudio(&ovi->cvt);
            memcpy(buf_dst, ovi->cvt.buf, ovi->cvt.len_cvt);
            dst_len = ovi->cvt.len_cvt;
            buf_dst += ovi->cvt.len_cvt;
        }
        else{
            buf += dst_len;
        }

        total_len += dst_len;
        if (src_len == len) break;
        len -= src_len;
    }
#endif

    return total_len;
}

int ONScripterLabel::playSound(const char *filename, int format, bool loop_flag, int channel)
{
    if ( !audio_open_flag ) return SOUND_NONE;

    long length = script_h.cBR->getFileLength( filename );
    if (length == 0) return SOUND_NONE;

    unsigned char *buffer = new unsigned char[length];
    script_h.cBR->getFile( filename, buffer );

    if (format & (SOUND_OGG | SOUND_OGG_STREAMING)){
        int ret = playOGG(format, buffer, length, loop_flag, channel);
        if (ret & (SOUND_OGG | SOUND_OGG_STREAMING)) return ret;
    }

    if (format & SOUND_WAVE){
        Mix_Chunk *chunk = Mix_LoadWAV_RW(SDL_RWFromMem(buffer, length), 1);
        if (playWave(chunk, format, loop_flag, channel) == 0){
            delete[] buffer;
            return SOUND_WAVE;
        }
    }

    if (format & SOUND_MP3){
        if (music_cmd){
            FILE *fp;
            if ( (fp = fopen(TMP_MUSIC_FILE, "wb")) == NULL){
                fprintf(stderr, "can't open temporary Music file %s\n", TMP_MUSIC_FILE);
            }
            else{
                fwrite(buffer, 1, length, fp);
                fclose( fp );
                ext_music_play_once_flag = !loop_flag;
                if (playExternalMusic(loop_flag) == 0){
                    delete[] buffer;
                    return SOUND_MP3;
                }
            }
        }

        mp3_sample = SMPEG_new_rwops( SDL_RWFromMem( buffer, length ), NULL, 0 );
 #if defined(INSANI)
        /* music_volume = 100; */
 #endif
        if (playMP3() == 0){
            mp3_buffer = buffer;
            return SOUND_MP3;
        }
    }

    /* check WMA */
    if ( buffer[0] == 0x30 && buffer[1] == 0x26 &&
         buffer[2] == 0xb2 && buffer[3] == 0x75 ){
        delete[] buffer;
        return SOUND_OTHER;
    }

    if (format & SOUND_MIDI){
        FILE *fp;
        if ( (fp = fopen(TMP_MIDI_FILE, "wb")) == NULL){
            fprintf(stderr, "can't open temporaly MIDI file %s\n", TMP_MIDI_FILE);
        }
        else{
            fwrite(buffer, 1, length, fp);
            fclose( fp );
            ext_music_play_once_flag = !loop_flag;
            if (playMIDI(loop_flag) == 0){
                delete[] buffer;
                return SOUND_MIDI;
            }
        }
    }

    delete[] buffer;

    return SOUND_OTHER;
}

void ONScripterLabel::playCDAudio()
{
    if ( cdaudio_flag ){
        if ( cdrom_info ){
            int length = cdrom_info->track[current_cd_track - 1].length / 75;
            SDL_CDPlayTracks( cdrom_info, current_cd_track - 1, 0, 1, 0 );
            timer_cdaudio_id = SDL_AddTimer( length * 1000, cdaudioCallback, NULL );
        }
    }
    else{
        char filename[256];
        sprintf( filename, "cd\\track%2.2d.mp3", current_cd_track );
 #if defined(INSANI)
        /* music_volume = 100; */
 #endif
        int ret = playSound( filename, SOUND_MP3, cd_play_loop_flag );
        if (ret == SOUND_MP3) return;

        sprintf( filename, "cd\\track%2.2d.ogg", current_cd_track );
        ret = playSound( filename, SOUND_OGG_STREAMING, cd_play_loop_flag );
        if (ret == SOUND_OGG_STREAMING) return;

        sprintf( filename, "cd\\track%2.2d.wav", current_cd_track );
        ret = playSound( filename, SOUND_WAVE, cd_play_loop_flag, MIX_BGM_CHANNEL );
    }
}

int ONScripterLabel::playWave(Mix_Chunk *chunk, int format, bool loop_flag, int channel)
{
    if (!chunk) return -1;

    Mix_Pause( channel );
    if ( wave_sample[channel] ) Mix_FreeChunk( wave_sample[channel] );
    wave_sample[channel] = chunk;

    if      (channel == 0)               Mix_Volume( channel, voice_volume * 128 / 100 );
    else if (channel == MIX_BGM_CHANNEL) Mix_Volume( channel, music_volume * 128 / 100 );
    else                                 Mix_Volume( channel, se_volume * 128 / 100 );

    if ( !(format & SOUND_PRELOAD) )
        Mix_PlayChannel( channel, wave_sample[channel], loop_flag?-1:0 );

    return 0;
}

int ONScripterLabel::playMP3()
{
    if ( SMPEG_error( mp3_sample ) ){
        //printf(" failed. [%s]\n",SMPEG_error( mp3_sample ));
        // The line below fails. ?????
        //SMPEG_delete( mp3_sample );
        mp3_sample = NULL;
        return -1;
    }

#ifndef MP3_MAD
    SMPEG_enableaudio( mp3_sample, 0 );
    SMPEG_actualSpec( mp3_sample, &audio_format );
    SMPEG_enableaudio( mp3_sample, 1 );
#endif
    SMPEG_setvolume( mp3_sample, music_volume );
    Mix_HookMusic( mp3callback, mp3_sample );
    SMPEG_play( mp3_sample );

    return 0;
}

int ONScripterLabel::playOGG(int format, unsigned char *buffer, long length, bool loop_flag, int channel)
{
    int channels, rate;
    OVInfo *ovi = openOggVorbis(buffer, length, channels, rate);
    if (ovi == NULL) return SOUND_OTHER;

    if (format & SOUND_OGG){
        unsigned char *buffer2 = new unsigned char[sizeof(WAVE_HEADER)+ovi->decoded_length];
        decodeOggVorbis(ovi, buffer2+sizeof(WAVE_HEADER), ovi->decoded_length, false);
        setupWaveHeader(buffer2, channels, rate, ovi->decoded_length);
        Mix_Chunk *chunk = Mix_LoadWAV_RW(SDL_RWFromMem(buffer2, sizeof(WAVE_HEADER)+ovi->decoded_length), 1);
        delete[] buffer2;
        closeOggVorbis(ovi);

        playWave(chunk, format, loop_flag, channel);

        return SOUND_OGG;
    }

    music_ovi = ovi;
    Mix_VolumeMusic(music_volume * 128 / 100);
    Mix_HookMusic(oggcallback, music_ovi);

    return SOUND_OGG_STREAMING;
}

int ONScripterLabel::playExternalMusic(bool loop_flag)
{
    int music_looping = loop_flag ? -1 : 0;
#if defined(LINUX)
    signal(SIGCHLD, musicCallback);
    if (music_cmd) music_looping = 0;
#endif

    Mix_SetMusicCMD(music_cmd);

    char music_filename[256];
    sprintf(music_filename, "%s%s", archive_path, TMP_MUSIC_FILE);
    if ((music_info = Mix_LoadMUS(music_filename)) == NULL){
        fprintf( stderr, "can't load Music file %s\n", music_filename );
        return -1;
    }

    // Mix_VolumeMusic( music_volume );
    Mix_PlayMusic(music_info, music_looping);

    return 0;
}

int ONScripterLabel::playMIDI(bool loop_flag)
{
    Mix_SetMusicCMD(midi_cmd);

    char midi_filename[256];
    sprintf(midi_filename, "%s%s", archive_path, TMP_MIDI_FILE);
    if ((midi_info = Mix_LoadMUS(midi_filename)) == NULL) return -1;

    int midi_looping = loop_flag ? -1 : 0;

#if defined(EXTERNAL_MIDI_PROGRAM)
    FILE *com_file;
    if ( midi_play_loop_flag ){
        if( (com_file = fopen("play_midi", "wb")) != NULL )
            fclose(com_file);
    }
    else{
        if( (com_file = fopen("playonce_midi", "wb")) != NULL )
            fclose(com_file);
    }
#endif

#if defined(LINUX)
    signal(SIGCHLD, midiCallback);
    if (midi_cmd) midi_looping = 0;
#endif

    Mix_VolumeMusic(music_volume);
#if defined(MACOSX) && defined(INSANI)
	// Emulate looping on MacOS ourselves to work around bug in SDL_Mixer
	Mix_PlayMusic(midi_info, false);
	timer_midi_id = SDL_AddTimer(1000, midiSDLCallback, NULL);
#else
    Mix_PlayMusic(midi_info, midi_looping);
#endif
    current_cd_track = -2;

    return 0;
}

int ONScripterLabel::playMPEG( const char *filename, bool click_flag )
{
    int ret = 0;
#ifndef MP3_MAD
    unsigned long length = script_h.cBR->getFileLength( filename );
    unsigned char *mpeg_buffer = new unsigned char[length];
    script_h.cBR->getFile( filename, mpeg_buffer );
    SMPEG *mpeg_sample = SMPEG_new_rwops( SDL_RWFromMem( mpeg_buffer, length ), NULL, 0 );

    if ( !SMPEG_error( mpeg_sample ) ){
        SMPEG_enableaudio( mpeg_sample, 0 );

        if ( audio_open_flag ){
            SMPEG_actualSpec( mpeg_sample, &audio_format );
            SMPEG_enableaudio( mpeg_sample, 1 );
        }
        SMPEG_enablevideo( mpeg_sample, 1 );
        SMPEG_setdisplay( mpeg_sample, screen_surface, NULL, NULL );
        SMPEG_setvolume( mpeg_sample, music_volume );

        Mix_HookMusic( mp3callback, mpeg_sample );
        SMPEG_play( mpeg_sample );

        bool done_flag = false;
        while( !(done_flag & click_flag) && SMPEG_status(mpeg_sample) == SMPEG_PLAYING ){
            SDL_Event event;

            while( SDL_PollEvent( &event ) ){
                switch (event.type){
                  case SDL_KEYDOWN:
                    if ( ((SDL_KeyboardEvent *)&event)->keysym.sym == SDLK_RETURN ||
                         ((SDL_KeyboardEvent *)&event)->keysym.sym == SDLK_SPACE ||
                         ((SDL_KeyboardEvent *)&event)->keysym.sym == SDLK_ESCAPE )
                        done_flag = true;
                    break;
                  case SDL_QUIT:
                    ret = 1;
                  case SDL_MOUSEBUTTONDOWN:
                    done_flag = true;
                    break;
                  default:
                    break;
                }
            }
            SDL_Delay( 10 );
        }

        SMPEG_stop( mpeg_sample );
        Mix_HookMusic( NULL, NULL );
        SMPEG_delete( mpeg_sample );

    }
    delete[] mpeg_buffer;
#else
    fprintf( stderr, "mpegplay command is disabled.\n" );
#endif

    return ret;
}

void ONScripterLabel::playAVI( const char *filename, bool click_flag )
{
#if defined(USE_AVIFILE)
    char *absolute_filename = new char[ strlen(archive_path) + strlen(filename) + 1 ];
    sprintf( absolute_filename, "%s%s", archive_path, filename );
    for ( unsigned int i=0 ; i<strlen( absolute_filename ) ; i++ )
        if ( absolute_filename[i] == '/' ||
             absolute_filename[i] == '\\' )
            absolute_filename[i] = DELIMITER;

    if ( audio_open_flag ) Mix_CloseAudio();

    AVIWrapper *avi = new AVIWrapper();
    if ( avi->init( absolute_filename, false ) == 0 &&
         avi->initAV( screen_surface, audio_open_flag ) == 0 ){
        if (avi->play( click_flag )) endCommand();
    }
    delete avi;
    delete[] absolute_filename;

    if ( audio_open_flag ){
        Mix_CloseAudio();
        openAudio();
    }
#else
    fprintf( stderr, "avi command is disabled.\n" );
#endif
}

void ONScripterLabel::stopBGM( bool continue_flag )
{
#if defined(EXTERNAL_MIDI_PROGRAM)
    FILE *com_file;
    if( (com_file = fopen("stop_bgm", "wb")) != NULL )
        fclose(com_file);
#endif

    if ( cdaudio_flag && cdrom_info ){
        extern SDL_TimerID timer_cdaudio_id;

        if ( timer_cdaudio_id ){
            SDL_RemoveTimer( timer_cdaudio_id );
            timer_cdaudio_id = NULL;
        }
        if (SDL_CDStatus( cdrom_info ) >= CD_PLAYING )
            SDL_CDStop( cdrom_info );
    }

    if ( mp3_sample ){
        SMPEG_stop( mp3_sample );
        Mix_HookMusic( NULL, NULL );
        SMPEG_delete( mp3_sample );
        mp3_sample = NULL;

        if ( mp3_buffer ){
            delete[] mp3_buffer;
            mp3_buffer = NULL;
        }
    }

    if (music_ovi){
        Mix_HaltMusic();
        Mix_HookMusic( NULL, NULL );
        closeOggVorbis(music_ovi);
        music_ovi = NULL;
    }

    if ( wave_sample[MIX_BGM_CHANNEL] ){
        Mix_Pause( MIX_BGM_CHANNEL );
        Mix_FreeChunk( wave_sample[MIX_BGM_CHANNEL] );
        wave_sample[MIX_BGM_CHANNEL] = NULL;
    }

    if ( !continue_flag ){
        setStr( &music_file_name, NULL );
        music_play_loop_flag = false;
    }

    if ( midi_info ){

#if defined(MACOSX) && defined(INSANI)
        if ( timer_midi_id ){
            SDL_RemoveTimer( timer_midi_id );
            timer_midi_id = NULL;
        }
#endif

        ext_music_play_once_flag = true;
        Mix_HaltMusic();
        Mix_FreeMusic( midi_info );
        midi_info = NULL;
    }
    if ( !continue_flag ){
        setStr( &midi_file_name, NULL );
        midi_play_loop_flag = false;
    }

    if ( music_info ){
        ext_music_play_once_flag = true;
        Mix_HaltMusic();
        Mix_FreeMusic( music_info );
        music_info = NULL;
    }

    if ( !continue_flag ) current_cd_track = -1;
}

void ONScripterLabel::playClickVoice()
{
    if      ( clickstr_state == CLICK_NEWPAGE ){
        if ( clickvoice_file_name[CLICKVOICE_NEWPAGE] )
            playSound(clickvoice_file_name[CLICKVOICE_NEWPAGE],
                      SOUND_WAVE|SOUND_OGG, false, MIX_WAVE_CHANNEL);
    }
    else if ( clickstr_state == CLICK_WAIT ){
        if ( clickvoice_file_name[CLICKVOICE_NORMAL] )
            playSound(clickvoice_file_name[CLICKVOICE_NORMAL],
                      SOUND_WAVE|SOUND_OGG, false, MIX_WAVE_CHANNEL);
    }
}

void ONScripterLabel::setupWaveHeader( unsigned char *buffer, int channels, int rate, unsigned long data_length )
{
    memcpy( header.chunk_riff, "RIFF", 4 );
    int riff_length = sizeof(WAVE_HEADER) + data_length - 8;
    header.riff_length[0] = riff_length & 0xff;
    header.riff_length[1] = (riff_length >> 8) & 0xff;
    header.riff_length[2] = (riff_length >> 16) & 0xff;
    header.riff_length[3] = (riff_length >> 24) & 0xff;
    memcpy( header.fmt_id, "WAVEfmt ", 8 );
    header.fmt_size[0] = 0x10;
    header.fmt_size[1] = header.fmt_size[2] = header.fmt_size[3] = 0;
    header.data_fmt[0] = 1; header.data_fmt[1] = 0; // PCM format
    header.channels[0] = channels; header.channels[1] = 0;
    header.frequency[0] = rate & 0xff;
    header.frequency[1] = (rate >> 8) & 0xff;
    header.frequency[2] = (rate >> 16) & 0xff;
    header.frequency[3] = (rate >> 24) & 0xff;

    int sample_byte_size = 2 * channels; // 16bit * channels
    int byte_size = sample_byte_size * rate;
    header.byte_size[0] = byte_size & 0xff;
    header.byte_size[1] = (byte_size >> 8) & 0xff;
    header.byte_size[2] = (byte_size >> 16) & 0xff;
    header.byte_size[3] = (byte_size >> 24) & 0xff;
    header.sample_byte_size[0] = sample_byte_size;
    header.sample_byte_size[1] = 0;
    header.sample_bit_size[0] = 16; // 16bit
    header.sample_bit_size[1] = 0;

    memcpy( header.chunk_id, "data", 4 );
    header.data_length[0] = (char)(data_length & 0xff);
    header.data_length[1] = (char)((data_length >> 8) & 0xff);
    header.data_length[2] = (char)((data_length >> 16) & 0xff);
    header.data_length[3] = (char)((data_length >> 24) & 0xff);

    memcpy( buffer, &header, sizeof(header) );
}
#if defined(USE_OGG_VORBIS)
static size_t oc_read_func(void *ptr, size_t size, size_t nmemb, void *datasource)
{
    OVInfo *ogg_vorbis_info = (OVInfo*)datasource;

    size_t len = size*nmemb;
    if (ogg_vorbis_info->pos+len > ogg_vorbis_info->length)
        len = ogg_vorbis_info->length - ogg_vorbis_info->pos;
    memcpy(ptr, ogg_vorbis_info->buf+ogg_vorbis_info->pos, len);
    ogg_vorbis_info->pos += len;

    return len;
}

static int oc_seek_func(void *datasource, ogg_int64_t offset, int whence)
{
    OVInfo *ogg_vorbis_info = (OVInfo*)datasource;

    ogg_int64_t pos = 0;
    if (whence == 0)
        pos = offset;
    else if (whence == 1)
        pos = ogg_vorbis_info->pos + offset;
    else if (whence == 2)
        pos = ogg_vorbis_info->length + offset;

    if (pos < 0 || pos > ogg_vorbis_info->length) return -1;

    ogg_vorbis_info->pos = pos;

    return 0;
}

static int oc_close_func(void *datasource)
{
    return 0;
}

static long oc_tell_func(void *datasource)
{
    OVInfo *ogg_vorbis_info = (OVInfo*)datasource;

    return ogg_vorbis_info->pos;
}
#endif
OVInfo *ONScripterLabel::openOggVorbis( unsigned char *buf, long len, int &channels, int &rate )
{
    OVInfo *ovi = NULL;

#if defined(USE_OGG_VORBIS)
    ovi = new OVInfo();

    ovi->buf = buf;
    ovi->decoded_length = 0;
    ovi->length = len;
    ovi->pos = 0;

    ov_callbacks oc;
    oc.read_func  = oc_read_func;
    oc.seek_func  = oc_seek_func;
    oc.close_func = oc_close_func;
    oc.tell_func  = oc_tell_func;
    if (ov_open_callbacks(ovi, &ovi->ovf, NULL, 0, oc) < 0){
        delete ovi;
        return NULL;
    }

    vorbis_info *vi = ov_info( &ovi->ovf, -1 );
    if (vi == NULL){
        ov_clear(&ovi->ovf);
        delete ovi;
        return NULL;
    }

    channels = vi->channels;
    rate = vi->rate;

    ovi->cvt.buf = NULL;
    ovi->cvt_len = 0;
    SDL_BuildAudioCVT(&ovi->cvt,
                      AUDIO_S16, channels, rate,
                      audio_format.format, audio_format.channels, audio_format.freq);
    ovi->mult1 = 10;
    ovi->mult2 = (int)(ovi->cvt.len_ratio*10.0);

    ovi->decoded_length = ov_pcm_total(&ovi->ovf, -1) * channels * 2;
#endif

    return ovi;
}

int ONScripterLabel::closeOggVorbis(OVInfo *ovi)
{
    if (ovi->buf){
        delete[] ovi->buf;
        ovi->buf = NULL;
#if defined(USE_OGG_VORBIS)
        ovi->length = 0;
        ovi->pos = 0;
        ov_clear(&ovi->ovf);
#endif
    }
    if (ovi->cvt.buf){
        delete[] ovi->cvt.buf;
        ovi->cvt.buf = NULL;
        ovi->cvt_len = 0;
    }
    delete ovi;

    return 0;
}

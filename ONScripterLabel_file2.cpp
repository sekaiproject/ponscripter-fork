/* -*- C++ -*-
 *
 *  ONScripterLabel_file2.cpp - FILE I/O of ONScripter
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

int ONScripterLabel::loadSaveFile2( int file_version )
{
    deleteNestInfo();
    
    int i, j;
    
    readInt(); // 1
    if ( readInt() == 1 ) sentence_font.is_bold = true;
    else                  sentence_font.is_bold = false;
    if ( readInt() == 1 ) sentence_font.is_shadow = true;
    else                  sentence_font.is_shadow = false;

    readInt(); // 0
    rmode_flag = (readInt()==1)?true:false;
    sentence_font.color[0] = readInt();
    sentence_font.color[1] = readInt();
    sentence_font.color[2] = readInt();
    cursor_info[0].remove();
    readStr( &cursor_info[0].image_name );
    if ( cursor_info[0].image_name ){
        parseTaggedString( &cursor_info[0] );
        setupAnimationInfo( &cursor_info[0] );
        if ( cursor_info[0].image_surface )
            cursor_info[0 ].visible = true;
    }
    cursor_info[1].remove();
    readStr( &cursor_info[1].image_name );
    if ( cursor_info[1].image_name ){
        parseTaggedString( &cursor_info[1] );
        setupAnimationInfo( &cursor_info[1] );
        if ( cursor_info[1].image_surface )
            cursor_info[1 ].visible = true;
    }

    window_effect.effect = readInt();
    window_effect.duration = readInt();
    readStr( &window_effect.anim.image_name ); // probably

    sentence_font.clear();
    sentence_font.ttf_font  = NULL;
    sentence_font.top_xy[0] = readInt();
    sentence_font.top_xy[1] = readInt();
    sentence_font.num_xy[0] = readInt();
    sentence_font.num_xy[1] = readInt();
    sentence_font.font_size_xy[0] = readInt();
    sentence_font.font_size_xy[1] = readInt();
    sentence_font.pitch_xy[0] = readInt();
    sentence_font.pitch_xy[1] = readInt();
    for ( i=0 ; i<3 ; i++ )
        sentence_font.window_color[2-i] = readChar();
    if ( readChar() == 0x00 ) sentence_font.is_transparent = true;
    else                      sentence_font.is_transparent = false;
    sentence_font.wait_time = readInt();
    sentence_font_info.pos.x = readInt() * screen_ratio1 / screen_ratio2;
    sentence_font_info.pos.y = readInt() * screen_ratio1 / screen_ratio2;
    sentence_font_info.pos.w = (readInt() + 1 - sentence_font_info.pos.x * screen_ratio1 / screen_ratio2) * screen_ratio1 / screen_ratio2;
    sentence_font_info.pos.h = (readInt() + 1 - sentence_font_info.pos.y * screen_ratio1 / screen_ratio2) * screen_ratio1 / screen_ratio2;
    readStr( &sentence_font_info.image_name );
    if ( !sentence_font.is_transparent && sentence_font_info.image_name ){
        parseTaggedString( &sentence_font_info );
        setupAnimationInfo( &sentence_font_info );
    }

    if ( readInt() == 1 ) cursor_info[0].abs_flag = false;
    else                  cursor_info[0].abs_flag = true;
    if ( readInt() == 1 ) cursor_info[1].abs_flag = false;
    else                  cursor_info[1].abs_flag = true;
    cursor_info[0].pos.x = readInt() * screen_ratio1 / screen_ratio2;
    cursor_info[1].pos.x = readInt() * screen_ratio1 / screen_ratio2;
    cursor_info[0].pos.y = readInt() * screen_ratio1 / screen_ratio2;
    cursor_info[1].pos.y = readInt() * screen_ratio1 / screen_ratio2;

    // load background surface
    bg_info.remove();
    readStr( &bg_info.file_name );
    createBackground();

    for ( i=0 ; i<3 ; i++ ){
        tachi_info[i].remove();
        readStr( &tachi_info[i].image_name );
        if ( tachi_info[i].image_name ){
            parseTaggedString( &tachi_info[i] );
            setupAnimationInfo( &tachi_info[i] );
        }
    }

    for ( i=0 ; i<3 ; i++ )
        tachi_info[i].pos.x = readInt() * screen_ratio1 / screen_ratio2;

    for ( i=0 ; i<3 ; i++ )
        tachi_info[i].pos.y = readInt() * screen_ratio1 / screen_ratio2;

    readInt(); // 0
    readInt(); // 0
    readInt(); // 0
    
    for ( i=0 ; i<MAX_SPRITE_NUM ; i++ ){
        sprite_info[i].remove();
        readStr( &sprite_info[i].image_name );
        if ( sprite_info[i].image_name ){
            parseTaggedString( &sprite_info[i] );
            setupAnimationInfo( &sprite_info[i] );
        }
        sprite_info[i].pos.x = readInt() * screen_ratio1 / screen_ratio2;
        sprite_info[i].pos.y = readInt() * screen_ratio1 / screen_ratio2;
        if ( readInt() == 1 ) sprite_info[i].visible = true;
        else                  sprite_info[i].visible = false;
        sprite_info[i].current_cell = readInt();
    }

    readVariables( 0, script_h.global_variable_border );

    // nested info
    int num_nest =readInt();
    last_nest_info = &root_nest_info;
    if (num_nest > 0){
        file_io_buf_ptr += (num_nest-1)*4;
        while( num_nest > 0 ){
            NestInfo *info = new NestInfo();
            if (last_nest_info == &root_nest_info) last_nest_info = info;
        
            i = readInt();
            if (i > 0){
                info->nest_mode = NestInfo::LABEL;
                info->next_script = script_h.getAddress( i );
                file_io_buf_ptr -= 8;
                num_nest--;
            }
            else{
                info->nest_mode = NestInfo::FOR;
                info->next_script = script_h.getAddress( -i );
                file_io_buf_ptr -= 16;
                info->var_no = readInt();
                info->to = readInt();
                info->step = readInt();
                file_io_buf_ptr -= 16;
                num_nest -= 4;
            }
            info->next = root_nest_info.next;
            if (root_nest_info.next) root_nest_info.next->previous = info;
            root_nest_info.next = info;
            info->previous = &root_nest_info;
        }
        num_nest = readInt();
        file_io_buf_ptr += num_nest*4;
    }

    if (readInt() == 1) monocro_flag = true;
    else                monocro_flag = false;
    for ( i=0 ; i<3 ; i++ )
        monocro_color[2-i] = readInt();
    for ( i=0 ; i<256 ; i++ ){
        monocro_color_lut[i][0] = (monocro_color[0] * i) >> 8;
        monocro_color_lut[i][1] = (monocro_color[1] * i) >> 8;
        monocro_color_lut[i][2] = (monocro_color[2] * i) >> 8;
    }
    nega_mode = readInt();
    
    // ----------------------------------------
    // Sound
    stopCommand();
    loopbgmstopCommand();

    readStr( &midi_file_name ); // MIDI file
    readStr( &wave_file_name ); // wave, waveloop
    i = readInt();
    if ( i >= 0 ) current_cd_track = i;

    // play, playonce MIDI
    if ( readInt() == 1 ){
        midi_play_loop_flag = true;
        current_cd_track = -2;
        playSound(midi_file_name, SOUND_MIDI, midi_play_loop_flag);
    }
    else
        midi_play_loop_flag = false;
    
    // wave, waveloop
    if ( readInt() == 1 ) wave_play_loop_flag = true;
    else                  wave_play_loop_flag = false;
    if ( wave_file_name && wave_play_loop_flag )
        playSound(wave_file_name, SOUND_WAVE|SOUND_OGG, wave_play_loop_flag, MIX_WAVE_CHANNEL);

    // play, playonce
    if ( readInt() == 1 ) cd_play_loop_flag = true;
    else                  cd_play_loop_flag = false;
    if ( current_cd_track >= 0 ) playCDAudio();

    // bgm, mp3, mp3loop
    if ( readInt() == 1 ) music_play_loop_flag = true;
    else                  music_play_loop_flag = false;
    if ( readInt() == 1 ) mp3save_flag = true;
    else                  mp3save_flag = false;
    readStr( &music_file_name );
    if ( music_file_name ){
        playSound(music_file_name,
                  SOUND_WAVE | SOUND_OGG_STREAMING | SOUND_MP3 | SOUND_MIDI,
                  music_play_loop_flag, MIX_BGM_CHANNEL);
    }

    erase_text_window_mode = readInt();
    readInt(); // 1

    barclearCommand();
    for ( i=0 ; i<MAX_PARAM_NUM ; i++ ){
        j = readInt();
        if ( j != 0 ){
            bar_info[i] = new AnimationInfo();
            bar_info[i]->trans_mode = AnimationInfo::TRANS_COPY;
            bar_info[i]->num_of_cells = 1;

            bar_info[i]->param = j;
            bar_info[i]->pos.x = readInt() * screen_ratio1 / screen_ratio2;
            bar_info[i]->pos.y = readInt() * screen_ratio1 / screen_ratio2;
            bar_info[i]->max_width = readInt() * screen_ratio1 / screen_ratio2;
            bar_info[i]->pos.h = readInt() * screen_ratio1 / screen_ratio2;
            bar_info[i]->max_param = readInt();
            for ( j=0 ; j<3 ; j++ )
                bar_info[i]->color[2-j] = readChar();
            readChar(); // 0x00

            int w = bar_info[i]->max_width * bar_info[i]->param / bar_info[i]->max_param;
            if ( bar_info[i]->max_width > 0 && w > 0 ){
                bar_info[i]->pos.w = w;
                bar_info[i]->allocImage( bar_info[i]->pos.w, bar_info[i]->pos.h );
                bar_info[i]->fill( bar_info[i]->color[0], bar_info[i]->color[1], bar_info[i]->color[2], 0xff );
            }
        }
        else{
            readInt(); // -1
            readInt(); // 0
            readInt(); // 0
            readInt(); // 0
            readInt(); // 0
            readInt(); // 0
        }
    }

    prnumclearCommand();
    for ( i=0 ; i<MAX_PARAM_NUM ; i++ ){
        j = readInt();
        if ( prnum_info[i] ){
            prnum_info[i] = new AnimationInfo();
            prnum_info[i]->trans_mode = AnimationInfo::TRANS_STRING;
            prnum_info[i]->num_of_cells = 1;
            prnum_info[i]->color_list = new uchar3[1];

            prnum_info[i]->param = j;
            prnum_info[i]->pos.x = readInt() * screen_ratio1 / screen_ratio2;
            prnum_info[i]->pos.y = readInt() * screen_ratio1 / screen_ratio2;
            prnum_info[i]->font_size_xy[0] = readInt();
            prnum_info[i]->font_size_xy[1] = readInt();
            for ( j=0 ; j<3 ; j++ )
                prnum_info[i]->color_list[0][2-j] = readChar();
            readChar(); // 0x00

            char num_buf[7];
            script_h.getStringFromInteger( num_buf, prnum_info[i]->param, 3 );
            setStr( &prnum_info[i]->file_name, num_buf );

            setupAnimationInfo( prnum_info[i] );
        }
        else{
            readInt(); // -1
            readInt(); // 0
            readInt(); // 0
            readInt(); // 0
            readInt(); // 0
        }
    }

    readInt(); // 1
    readInt(); // 0
    readInt(); // 1
    btndef_info.remove();
    readStr( &btndef_info.image_name );
    if ( btndef_info.image_name && btndef_info.image_name[0] != '\0' ){
        parseTaggedString( &btndef_info );
        setupAnimationInfo( &btndef_info );
        SDL_SetAlpha( btndef_info.image_surface, DEFAULT_BLIT_FLAG, SDL_ALPHA_OPAQUE );
    }

    if ( file_version >= 202 )
        readArrayVariable();
    
    readInt(); // 0
    if ( readChar() == 1 ) erase_text_window_mode = 2;
    readChar(); // 0
    readChar(); // 0
    readChar(); // 0
    readStr( &loop_bgm_name[0] );
    readStr( &loop_bgm_name[1] );
    if ( loop_bgm_name[0] ) {
        if ( loop_bgm_name[1] )
            playSound(loop_bgm_name[1],
                      SOUND_PRELOAD|SOUND_WAVE|SOUND_OGG, false, MIX_LOOPBGM_CHANNEL1);
        playSound(loop_bgm_name[0],
                  SOUND_WAVE|SOUND_OGG, false, MIX_LOOPBGM_CHANNEL0);
    }

    if ( file_version >= 201 ){
        if ( readInt() == 1 ) rubyon_flag = true;
        else                  rubyon_flag = false;
        ruby_struct.font_size_xy[0] = readInt();
        ruby_struct.font_size_xy[1] = readInt();
        readStr( &ruby_struct.font_name );
        sentence_font.setRubyOnFlag(rubyon_flag);
    }
    
    int text_num = readInt();
    start_text_buffer = current_text_buffer;
    for ( i=0 ; i<text_num ; i++ ){
        clearCurrentTextBuffer();
        do{
            current_text_buffer->buffer2[current_text_buffer->buffer2_count] = readChar();
        }
        while( current_text_buffer->buffer2[current_text_buffer->buffer2_count++] );
        current_text_buffer->buffer2_count--;
        current_text_buffer = current_text_buffer->next;
    }
    clearCurrentTextBuffer();

    i = readInt();
    current_label_info = script_h.getLabelByLine( i );
    current_line = i - current_label_info.start_line;
    //printf("load %d:%d(%d-%d)\n", current_label_info.start_line, current_line, i, current_label_info.start_line);
    char *buf = script_h.getAddressByLine( i );
    
    j = readInt();
    for ( i=0 ; i<j ; i++ ){
        while( *buf != ':' ) buf++;
        buf++;
    }
    script_h.setCurrent( buf );

    display_mode = next_display_mode = NORMAL_DISPLAY_MODE;
    current_refresh_mode = REFRESH_NORMAL_MODE;

    clickstr_state = CLICK_NONE;
    event_mode = 0;//WAIT_SLEEP_MODE;
    draw_cursor_flag = false;
    
    return 0;
}

void ONScripterLabel::saveSaveFile2( bool output_flag )
{
    int i, j;
    
    writeInt( 1, output_flag );
    writeInt( (sentence_font.is_bold?1:0), output_flag );
    writeInt( (sentence_font.is_shadow?1:0), output_flag );

    writeInt( 0, output_flag );
    writeInt( (rmode_flag)?1:0, output_flag );
    writeInt( sentence_font.color[0], output_flag );
    writeInt( sentence_font.color[1], output_flag );
    writeInt( sentence_font.color[2], output_flag );
    writeStr( cursor_info[0].image_name, output_flag );
    writeStr( cursor_info[1].image_name, output_flag );

    writeInt( window_effect.effect, output_flag );
    writeInt( window_effect.duration, output_flag );
    writeStr( window_effect.anim.image_name, output_flag ); // probably
    
    writeInt( sentence_font.top_xy[0], output_flag );
    writeInt( sentence_font.top_xy[1], output_flag );
    writeInt( sentence_font.num_xy[0], output_flag );
    writeInt( sentence_font.num_xy[1], output_flag );
    writeInt( sentence_font.font_size_xy[0], output_flag );
    writeInt( sentence_font.font_size_xy[1], output_flag );
    writeInt( sentence_font.pitch_xy[0], output_flag );
    writeInt( sentence_font.pitch_xy[1], output_flag );
    for ( i=0 ; i<3 ; i++ )
        writeChar( sentence_font.window_color[2-i], output_flag );
    writeChar( ( sentence_font.is_transparent )?0x00:0xff, output_flag ); 
    writeInt( sentence_font.wait_time, output_flag );
    writeInt( sentence_font_info.pos.x * screen_ratio2 / screen_ratio1, output_flag );
    writeInt( sentence_font_info.pos.y * screen_ratio2 / screen_ratio1, output_flag );
    writeInt( sentence_font_info.pos.w * screen_ratio2 / screen_ratio1 + sentence_font_info.pos.x * screen_ratio2 / screen_ratio1 - 1, output_flag );
    writeInt( sentence_font_info.pos.h * screen_ratio2 / screen_ratio1 + sentence_font_info.pos.y * screen_ratio2 / screen_ratio1 - 1, output_flag );
    writeStr( sentence_font_info.image_name, output_flag );

    writeInt( (cursor_info[0].abs_flag)?0:1, output_flag );
    writeInt( (cursor_info[1].abs_flag)?0:1, output_flag );
    writeInt( cursor_info[0].pos.x * screen_ratio2 / screen_ratio1, output_flag );
    writeInt( cursor_info[1].pos.x * screen_ratio2 / screen_ratio1, output_flag );
    writeInt( cursor_info[0].pos.y * screen_ratio2 / screen_ratio1, output_flag );
    writeInt( cursor_info[1].pos.y * screen_ratio2 / screen_ratio1, output_flag );
    
    writeStr( bg_info.file_name, output_flag );
    for ( i=0 ; i<3 ; i++ )
        writeStr( tachi_info[i].image_name, output_flag );

    for ( i=0 ; i<3 ; i++ )
        writeInt( tachi_info[i].pos.x * screen_ratio2 / screen_ratio1, output_flag );

    for ( i=0 ; i<3 ; i++ )
        writeInt( tachi_info[i].pos.y * screen_ratio2 / screen_ratio1, output_flag );

    writeInt( 0, output_flag );
    writeInt( 0, output_flag );
    writeInt( 0, output_flag );
    
    for ( i=0 ; i<MAX_SPRITE_NUM ; i++ ){
        writeStr( sprite_info[i].image_name, output_flag );
        writeInt( sprite_info[i].pos.x * screen_ratio2 / screen_ratio1, output_flag );
        writeInt( sprite_info[i].pos.y * screen_ratio2 / screen_ratio1, output_flag );
        writeInt( sprite_info[i].visible?1:0, output_flag );
        writeInt( sprite_info[i].current_cell, output_flag );
    }

    writeVariables( 0, script_h.global_variable_border, output_flag );

    // nested info
    int num_nest = 0;
    NestInfo *info = root_nest_info.next;
    while( info ){
        if      (info->nest_mode == NestInfo::LABEL) num_nest++;
        else if (info->nest_mode == NestInfo::FOR)   num_nest+=4;
        info = info->next;
    }
    writeInt( num_nest, output_flag );
    info = root_nest_info.next;
    while( info ){
        if  (info->nest_mode == NestInfo::LABEL){
            writeInt( script_h.getOffset( info->next_script ), output_flag );
        }
        else if (info->nest_mode == NestInfo::FOR){
            writeInt( info->var_no, output_flag );
            writeInt( info->to, output_flag );
            writeInt( info->step, output_flag );
            writeInt( -script_h.getOffset( info->next_script ), output_flag );
        }
        info = info->next;
    }
    
    writeInt( (monocro_flag)?1:0, output_flag );
    for ( i=0 ; i<3 ; i++ )
        writeInt( monocro_color[2-i], output_flag );
    writeInt( nega_mode, output_flag );

    // sound
    writeStr( midi_file_name, output_flag ); // MIDI file

    writeStr( wave_file_name, output_flag ); // wave, waveloop

    if ( current_cd_track >= 0 ) // play CD
        writeInt( current_cd_track, output_flag );
    else
        writeInt( -1, output_flag );

    writeInt( (midi_play_loop_flag)?1:0, output_flag ); // play, playonce MIDI
    writeInt( (wave_play_loop_flag)?1:0, output_flag ); // wave, waveloop
    writeInt( (cd_play_loop_flag)?1:0, output_flag ); // play, playonce
    writeInt( (music_play_loop_flag)?1:0, output_flag ); // bgm, mp3, mp3loop
    writeInt( (mp3save_flag)?1:0, output_flag );
    if (mp3save_flag)
        writeStr( music_file_name, output_flag );
    else
        writeStr( NULL, output_flag );
    
    writeInt( (erase_text_window_mode>0)?1:0, output_flag );
    writeInt( 1, output_flag );
    
    for ( i=0 ; i<MAX_PARAM_NUM ; i++ ){
        if ( bar_info[i] ){
            writeInt( bar_info[i]->param, output_flag );
            writeInt( bar_info[i]->pos.x * screen_ratio2 / screen_ratio1, output_flag );
            writeInt( bar_info[i]->pos.y * screen_ratio2 / screen_ratio1, output_flag );
            writeInt( bar_info[i]->max_width * screen_ratio2 / screen_ratio1, output_flag );
            writeInt( bar_info[i]->pos.h * screen_ratio2 / screen_ratio1, output_flag );
            writeInt( bar_info[i]->max_param, output_flag );
            for ( j=0 ; j<3 ; j++ )
                writeChar( bar_info[i]->color[2-j], output_flag );
            writeChar( 0x00, output_flag );
        }
        else{
            writeInt( 0, output_flag );
            writeInt( -1, output_flag );
            writeInt( 0, output_flag );
            writeInt( 0, output_flag );
            writeInt( 0, output_flag );
            writeInt( 0, output_flag );
            writeInt( 0, output_flag );
        }
    }
    
    for ( i=0 ; i<MAX_PARAM_NUM ; i++ ){
        if ( prnum_info[i] ){
            writeInt( prnum_info[i]->param, output_flag );
            writeInt( prnum_info[i]->pos.x * screen_ratio2 / screen_ratio1, output_flag );
            writeInt( prnum_info[i]->pos.y * screen_ratio2 / screen_ratio1, output_flag );
            writeInt( prnum_info[i]->font_size_xy[0], output_flag );
            writeInt( prnum_info[i]->font_size_xy[1], output_flag );
            for ( j=0 ; j<3 ; j++ )
                writeChar( prnum_info[i]->color_list[0][2-j], output_flag );
            writeChar( 0x00, output_flag );
        }
        else{
            writeInt( 0, output_flag );
            writeInt( -1, output_flag );
            writeInt( 0, output_flag );
            writeInt( 0, output_flag );
            writeInt( 0, output_flag );
            writeInt( 0, output_flag );
        }
    }

    writeInt( 1, output_flag );
    writeInt( 0, output_flag );
    writeInt( 1, output_flag );
    writeStr( btndef_info.image_name, output_flag );

    writeArrayVariable(output_flag);
    
    writeInt( 0, output_flag );
    writeChar( (erase_text_window_mode==2)?1:0, output_flag );
    writeChar( 0, output_flag );
    writeChar( 0, output_flag );
    writeChar( 0, output_flag );
    writeStr( loop_bgm_name[0], output_flag );
    writeStr( loop_bgm_name[1], output_flag );

    writeInt( (rubyon_flag)?1:0, output_flag );
    writeInt( ruby_struct.font_size_xy[0], output_flag );
    writeInt( ruby_struct.font_size_xy[1], output_flag );
    writeStr( ruby_struct.font_name, output_flag );
    
    TextBuffer *tb = current_text_buffer;
    int text_num = 0;
    while( tb != start_text_buffer ){
        tb = tb->previous;
        text_num++;
    }
    writeInt( text_num, output_flag );

    for ( i=0 ; i<text_num ; i++ ){
        for ( j=0 ; j<tb->buffer2_count ; j++ )
            writeChar( tb->buffer2[j], output_flag );
        writeChar( 0, output_flag );
        tb = tb->next;
    }

    writeInt( current_label_info.start_line + current_line, output_flag );
    char *buf = script_h.getAddressByLine( current_label_info.start_line + current_line );
    //printf("save %d:%d\n", current_label_info.start_line, current_line);

    i = 0;
    if (!script_h.isText()){
        while( buf != script_h.getCurrent() ){
            if ( *buf == ':' ) i++;
            buf++;
        }
    }
    writeInt( i, output_flag );
}

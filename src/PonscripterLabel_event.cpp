/* -*- C++ -*-
 *
 *  PonscripterLabel_event.cpp - Event handler of Ponscripter
 *
 *  Copyright (c) 2001-2007 Ogapee (original ONScripter, of which this
 *  is a fork).
 *
 *  ogapee@aqua.dti2.ne.jp
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307 USA
 */

#include "PonscripterLabel.h"
#ifdef LINUX
#include <sys/types.h>
#include <sys/wait.h>
#endif

#include "PonscripterUserEvents.h"

#define EDIT_MODE_PREFIX "[EDIT MODE]  "
#define EDIT_SELECT_STRING "MP3 vol (m)  SE vol (s)  Voice vol (v)  Numeric variable (n)"

static SDL_TimerID timer_id  = 0;

// This block does two things: it sets up the timer id for mp3 fadeout, and it also sets up a timer id for midi looping --
// the reason we have a separate midi loop timer id here is that on Mac OS X, looping midis via SDL will cause SDL itself
// to hard crash after the first play.  So, we work around that by manually causing the midis to loop.  This OS X midi
// workaround is the work of Ben Carter.  Recommend for integration.  [Seung Park, 20060621]
SDL_TimerID timer_mp3fadeout_id = 0;
#ifdef MACOSX
SDL_TimerID timer_midi_id = 0;
#endif
bool ext_music_play_once_flag = false;

extern long decodeOggVorbis(PonscripterLabel::MusicStruct *music_struct, Uint8 *buf_dst, long len, bool do_rate_conversion);

/* **************************************** *
* Callback functions
* **************************************** */
extern "C" void mp3callback(void* userdata, Uint8* stream, int len)
{
    if (SMPEG_playAudio((SMPEG*) userdata, stream, len) == 0) {
        SDL_Event event;
        event.type = ONS_SOUND_EVENT;
        SDL_PushEvent(&event);
    }
}


extern "C" void oggcallback(void* userdata, Uint8* stream, int len)
{
    if (decodeOggVorbis((PonscripterLabel::MusicStruct*)userdata, stream, len, true) == 0){
        SDL_Event event;
        event.type = ONS_SOUND_EVENT;
        SDL_PushEvent(&event);
    }
}


// Pushes the mp3 fadeout event onto the stack.  Part of our mp3
// fadeout enabling patch.  Recommend for integration.
// [Seung Park, 20060621]
extern "C" Uint32 SDLCALL mp3fadeoutCallback(Uint32 interval, void* param)
{
    SDL_Event event;
    event.type = ONS_FADE_EVENT;
    SDL_PushEvent(&event);

    return interval;
}


SDL_Keycode transKey(SDL_Keycode key)
{
#ifdef IPODLINUX
    switch (key) {
    case SDLK_m:      key = SDLK_UP;      break; /* Menu                   */
    case SDLK_d:      key = SDLK_DOWN;    break; /* Play/Pause             */
    case SDLK_f:      key = SDLK_RIGHT;   break; /* Fast forward           */
    case SDLK_w:      key = SDLK_LEFT;    break; /* Rewind                 */
    case SDLK_RETURN: key = SDLK_RETURN;  break; /* Action                 */
    case SDLK_h:      key = SDLK_ESCAPE;  break; /* Hold                   */
    case SDLK_r:      key = SDLK_UNKNOWN; break; /* Wheel clockwise        */
    case SDLK_l:      key = SDLK_UNKNOWN; break; /* Wheel counterclockwise */
    default: break;
    }

#endif
    return key;
}

SDL_Keycode transControllerButton(Uint8 button)
{
    SDL_Keycode button_map[] = {
        SDLK_SPACE,
        SDLK_RETURN,
        SDLK_RCTRL,
        SDLK_ESCAPE,
        SDLK_0,
        SDLK_UNKNOWN,
        SDLK_a,
        SDLK_UNKNOWN,
        SDLK_UNKNOWN,
        SDLK_o,
        SDLK_s,
        SDLK_UP,
        SDLK_DOWN,
        SDLK_LEFT,
        SDLK_RIGHT,
        SDLK_UNKNOWN };

    return button_map[button];
}


SDL_Keycode transJoystickButton(Uint8 button)
{
#ifdef PSP
    SDL_Keycode button_map[] = { SDLK_ESCAPE, /* TRIANGLE */
                            SDLK_RETURN, /* CIRCLE   */
                            SDLK_SPACE,  /* CROSS    */
                            SDLK_RCTRL,  /* SQUARE   */
                            SDLK_o,      /* LTRIGGER */
                            SDLK_s,      /* RTRIGGER */
                            SDLK_DOWN,   /* DOWN     */
                            SDLK_LEFT,   /* LEFT     */
                            SDLK_UP,     /* UP       */
                            SDLK_RIGHT,  /* RIGHT    */
                            SDLK_0,      /* SELECT   */
                            SDLK_a,      /* START    */
                            SDLK_UNKNOWN, /* HOME     */ /* kernel mode only */
                            SDLK_UNKNOWN, /* HOLD     */ };
    return button_map[button];
#endif
    return SDLK_UNKNOWN;
}


SDL_KeyboardEvent transJoystickAxis(SDL_JoyAxisEvent &jaxis)
{
    static int old_axis = -1;

    SDL_KeyboardEvent event;

    SDL_Keycode axis_map[] = { SDLK_LEFT,  /* AL-LEFT  */
                          SDLK_RIGHT,/* AL-RIGHT */
                          SDLK_UP,   /* AL-UP    */
                          SDLK_DOWN /* AL-DOWN  */ };

    int axis = ((3200 > jaxis.value) && (jaxis.value > -3200) ? -1 :
                (jaxis.axis * 2 + (jaxis.value > 0 ? 1 : 0)));

    if (axis != old_axis) {
        if (axis == -1) {
            event.type = SDL_KEYUP;
            event.keysym.sym = axis_map[old_axis];
        }
        else {
            event.type = SDL_KEYDOWN;
            event.keysym.sym = axis_map[axis];
        }

        old_axis = axis;
    }
    else {
        event.keysym.sym = SDLK_UNKNOWN;
    }

    return event;
}


void PonscripterLabel::flushEventSub(SDL_Event &event)
{
    if (event.type == ONS_SOUND_EVENT) {
        if (music_play_loop_flag) {
            stopBGM(true);
            if (music_file_name)
                playSound(music_file_name, SOUND_OGG_STREAMING | SOUND_MP3, true);
        }
        else {
            stopBGM(false);
        }
    }
// The event handler for the mp3 fadeout event itself.  Simply sets the volume of the mp3 being played lower and lower until it's 0,
// and until the requisite mp3 fadeout time has passed.  Recommend for integration.  [Seung Park, 20060621]
    else if (event.type == ONS_FADE_EVENT) {
        if (skip_flag || draw_one_page_flag ||
            ctrl_pressed_status || skip_to_wait)
        {
            mp3fadeout_duration = 0;
            if (mp3_sample) SMPEG_setvolume(mp3_sample, 0);
        }

        Uint32 tmp = SDL_GetTicks() - mp3fadeout_start;
        if (tmp < mp3fadeout_duration) {
            tmp  = mp3fadeout_duration - tmp;
            tmp *= music_volume;
            tmp /= mp3fadeout_duration;

            if (mp3_sample) SMPEG_setvolume(mp3_sample, tmp);
        }
        else {
            SDL_RemoveTimer(timer_mp3fadeout_id);
            timer_mp3fadeout_id = 0;

            event_mode &= ~WAIT_TIMER_MODE;
            stopBGM(false);
            advancePhase();
        }
    }
    else if (event.type == ONS_MIDI_EVENT) {
#ifdef MACOSX
        if (!Mix_PlayingMusic()) {
            ext_music_play_once_flag = !midi_play_loop_flag;
            Mix_FreeMusic(midi_info);
            playMIDI(midi_play_loop_flag);
        }

#else
        ext_music_play_once_flag = !midi_play_loop_flag;
        Mix_FreeMusic(midi_info);
        playMIDI(midi_play_loop_flag);
#endif
    }
    else if (event.type == ONS_MUSIC_EVENT) {
        ext_music_play_once_flag = !music_play_loop_flag;
        Mix_FreeMusic(music_info);
        playExternalMusic(music_play_loop_flag);
    }
    else if (event.type == ONS_WAVE_EVENT) { // for processing btntim2 and automode correctly
        if (wave_sample[event.user.code]) {
            Mix_FreeChunk(wave_sample[event.user.code]);
            wave_sample[event.user.code] = NULL;
            if (event.user.code == MIX_LOOPBGM_CHANNEL0
                && loop_bgm_name[1]
                && wave_sample[MIX_LOOPBGM_CHANNEL1])
                Mix_PlayChannel(MIX_LOOPBGM_CHANNEL1,
                    wave_sample[MIX_LOOPBGM_CHANNEL1], -1);
        }
    }
}


void PonscripterLabel::flushEvent()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
        flushEventSub(event);
}


void PonscripterLabel::startTimer(int count)
{
    int duration = proceedAnimation();

    if (duration > 0 && duration < count) {
        resetRemainingTime(duration);
        advancePhase(duration);
        remaining_time = count;
    }
    else {
        advancePhase(count);
        remaining_time = 0;
    }

    event_mode |= WAIT_TIMER_MODE;
}


void PonscripterLabel::advancePhase(int count) {
    timer_event_time = SDL_GetTicks() + count;
    timer_event_flag = true;

    SDL_Event event;
    event.type = INTERNAL_REDRAW_EVENT;
    SDL_PushEvent(&event);
}

void PonscripterLabel::queueRerender() {
    SDL_Event rerender_event;
    rerender_event.type = INTERNAL_REDRAW_EVENT;
    SDL_PushEvent(&rerender_event);
}


void midiCallback(int sig)
{
#ifdef LINUX
    int status;
    wait(&status);
#endif
    if (!ext_music_play_once_flag) {
        SDL_Event event;
        event.type = ONS_MIDI_EVENT;
        SDL_PushEvent(&event);
    }
}


// Pushes the midi loop event onto the stack.  Part of a workaround for Ponscripter
// crashing in Mac OS X after a midi is looped for the first time.  Recommend for
// integration.  This is the work of Ben Carter.  [Seung Park, 20060621]
#ifdef MACOSX
extern "C" Uint32 midiSDLCallback(Uint32 interval, void* param)
{
    SDL_Event event;
    event.type = ONS_MIDI_EVENT;
    SDL_PushEvent(&event);
    return interval;
}


#endif

extern "C" void waveCallback(int channel)
{
    SDL_Event event;
    event.type = ONS_WAVE_EVENT;
    event.user.code = channel;
    SDL_PushEvent(&event);
}


void musicCallback(int sig)
{
#ifdef LINUX
    int status;
    wait(&status);
#endif
    if (!ext_music_play_once_flag) {
        SDL_Event event;
        event.type = ONS_MUSIC_EVENT;
        SDL_PushEvent(&event);
    }
}


void PonscripterLabel::trapHandler()
{
    trap_mode = TRAP_NONE;
    setCurrentLabel(trap_dist);
    readToken();
    stopAnimation(clickstr_state);
    event_mode = IDLE_EVENT_MODE;
    advancePhase();
}


/* **************************************** *
* Event handlers
* **************************************** */
void PonscripterLabel::mouseMoveEvent(SDL_MouseMotionEvent* event)
{
    current_button_state.x = event->x;
    current_button_state.y = event->y;

    if (event_mode & WAIT_BUTTON_MODE)
        mouseOverCheck(current_button_state.x, current_button_state.y);
}


void PonscripterLabel::mousePressEvent(SDL_MouseButtonEvent* event)
{
    if (variable_edit_mode) return;

    if (automode_flag) {
        remaining_time = -1;
        setAutoMode(false);
        return;
    }

    if (event->button == SDL_BUTTON_RIGHT
        && trap_mode & TRAP_RIGHT_CLICK) {
        trapHandler();
        return;
    }
    else if (event->button == SDL_BUTTON_LEFT
             && trap_mode & TRAP_LEFT_CLICK) {
        trapHandler();
        return;
    }

    //Mouse didn't have a mouse-down event
    if(current_button_state.down_x == -1 && current_button_state.down_y == -1) {
        current_button_state.ignore_mouseup = true;
    }

    /* Use both = -1 to indicate we haven't received a mousedown yet */
    if(event->type == SDL_MOUSEBUTTONUP){
        current_button_state.down_x = -1;
        current_button_state.down_y = -1;
    }

    current_button_state.x = event->x;
    current_button_state.y = event->y;
    current_button_state.down_flag = false;
    setSkipMode(false);

    if (event->button == SDL_BUTTON_RIGHT
        && event->type == SDL_MOUSEBUTTONUP
        && !current_button_state.ignore_mouseup
        && ((rmode_flag && (event_mode & WAIT_TEXT_MODE))
            || (event_mode & WAIT_BUTTON_MODE))) {
        current_button_state.button  = -1;
        volatile_button_state.button = -1;
        if (event_mode & WAIT_TEXT_MODE) {
            if (!rmenu.empty())
                system_menu_mode = SYSTEM_MENU;
            else
                system_menu_mode = SYSTEM_WINDOWERASE;
        }
    }
    else if (event->button == SDL_BUTTON_LEFT
             && ((!current_button_state.ignore_mouseup && event->type == SDL_MOUSEBUTTONUP) || btndown_flag)) {
        current_button_state.button  = current_over_button;
        volatile_button_state.button = current_over_button;
//#ifdef SKIP_TO_WAIT
        if (event_mode & WAIT_SLEEP_MODE) skip_to_wait = 1;
//#endif

        if (event->type == SDL_MOUSEBUTTONDOWN) {
            current_button_state.down_flag = true;
        }
    } else {
      return;
    }

    if (event_mode & (WAIT_INPUT_MODE | WAIT_BUTTON_MODE)) {
        playClickVoice();
        stopAnimation(clickstr_state);
        advancePhase();
    }
}

void PonscripterLabel::mouseWheelEvent(SDL_MouseWheelEvent* event) {
    if (variable_edit_mode) return;


    if (automode_flag) {
        remaining_time = -1;
        setAutoMode(false);
        return;
    }

    setSkipMode(false);

    if (event->y > 0 //Scroll up
             && ((event_mode & WAIT_TEXT_MODE)
                 || (usewheel_flag && (event_mode & WAIT_BUTTON_MODE))
                 || system_menu_mode == SYSTEM_LOOKBACK)) {
        current_button_state.button  = -2;
        volatile_button_state.button = -2;
        if (event_mode & WAIT_TEXT_MODE) system_menu_mode = SYSTEM_LOOKBACK;
    }
    else if (event->y < 0 //Scroll down
             && ((enable_wheeldown_advance_flag &&
                  (event_mode & WAIT_TEXT_MODE))
                 || (usewheel_flag && (event_mode & WAIT_BUTTON_MODE))
                 || system_menu_mode == SYSTEM_LOOKBACK)) {
        if (event_mode & WAIT_TEXT_MODE) {
            current_button_state.button  = 0;
            volatile_button_state.button = 0;
        }
        else {
            current_button_state.button  = -3;
            volatile_button_state.button = -3;
        }
    } else {
      return;
    }
    /* Perhaps handle x too in case they have a trackpad sideways scroll? */

    if (event_mode & (WAIT_INPUT_MODE | WAIT_BUTTON_MODE)) {
      playClickVoice();
      stopAnimation(clickstr_state);
      advancePhase();
    }

}


void PonscripterLabel::variableEditMode(SDL_KeyboardEvent* event)
{
    switch (event->keysym.sym) {
    case SDLK_m:
        if (variable_edit_mode != EDIT_SELECT_MODE) return;

        variable_edit_mode = EDIT_MP3_VOLUME_MODE;
        variable_edit_num  = music_volume;
        break;

    case SDLK_s:
        if (variable_edit_mode != EDIT_SELECT_MODE) return;

        variable_edit_mode = EDIT_SE_VOLUME_MODE;
        variable_edit_num  = se_volume;
        break;

    case SDLK_v:
        if (variable_edit_mode != EDIT_SELECT_MODE) return;

        variable_edit_mode = EDIT_VOICE_VOLUME_MODE;
        variable_edit_num  = voice_volume;
        break;

    case SDLK_n:
        if (variable_edit_mode != EDIT_SELECT_MODE) return;

        variable_edit_mode = EDIT_VARIABLE_INDEX_MODE;
        variable_edit_num  = 0;
        break;

    case SDLK_9: case SDLK_KP_9:
	variable_edit_num = variable_edit_num * 10 + 9; break;
    case SDLK_8: case SDLK_KP_8:
	variable_edit_num = variable_edit_num * 10 + 8; break;
    case SDLK_7: case SDLK_KP_7:
	variable_edit_num = variable_edit_num * 10 + 7; break;
    case SDLK_6: case SDLK_KP_6:
	variable_edit_num = variable_edit_num * 10 + 6; break;
    case SDLK_5: case SDLK_KP_5:
	variable_edit_num = variable_edit_num * 10 + 5; break;
    case SDLK_4: case SDLK_KP_4:
	variable_edit_num = variable_edit_num * 10 + 4; break;
    case SDLK_3: case SDLK_KP_3:
	variable_edit_num = variable_edit_num * 10 + 3; break;
    case SDLK_2: case SDLK_KP_2:
	variable_edit_num = variable_edit_num * 10 + 2; break;
    case SDLK_1: case SDLK_KP_1:
	variable_edit_num = variable_edit_num * 10 + 1; break;
    case SDLK_0: case SDLK_KP_0:
	variable_edit_num = variable_edit_num * 10 + 0; break;

    case SDLK_MINUS: case SDLK_KP_MINUS:
        if (variable_edit_mode == EDIT_VARIABLE_NUM_MODE &&
	    variable_edit_num == 0)
	{
	    variable_edit_sign = -1;
	}
        break;

    case SDLK_BACKSPACE:
        if (variable_edit_num) variable_edit_num /= 10;
        else if (variable_edit_sign == -1) variable_edit_sign = 1;

        break;

    case SDLK_RETURN: case SDLK_KP_ENTER:
        switch (variable_edit_mode) {
        case EDIT_VARIABLE_INDEX_MODE:
            variable_edit_index = variable_edit_num;
            variable_edit_num =
                script_h.getVariableData(variable_edit_index).get_num();
            if (variable_edit_num < 0) {
                variable_edit_num  = -variable_edit_num;
                variable_edit_sign = -1;
            }
            else {
                variable_edit_sign = 1;
            }

            break;

        case EDIT_VARIABLE_NUM_MODE:
            script_h.setNumVariable(variable_edit_index, variable_edit_sign * variable_edit_num);
            break;

        case EDIT_MP3_VOLUME_MODE:
            music_volume = variable_edit_num;
            if (mp3_sample)
                SMPEG_setvolume(mp3_sample, !volume_on_flag? 0 : music_volume);
            break;

        case EDIT_SE_VOLUME_MODE:
            se_volume = variable_edit_num;
            for (int i = 1; i < ONS_MIX_CHANNELS; i++)
                if (wave_sample[i])
                    Mix_Volume(i, !volume_on_flag? 0 : se_volume * 128 / 100);

            if (wave_sample[MIX_LOOPBGM_CHANNEL0])
                Mix_Volume(MIX_LOOPBGM_CHANNEL0, !volume_on_flag? 0 : se_volume * 128 / 100);

            if (wave_sample[MIX_LOOPBGM_CHANNEL1])
                Mix_Volume(MIX_LOOPBGM_CHANNEL1, !volume_on_flag? 0 : se_volume * 128 / 100);

            break;

        case EDIT_VOICE_VOLUME_MODE:
            voice_volume = variable_edit_num;
            if (wave_sample[0])
                Mix_Volume(0, !volume_on_flag? 0 : se_volume * 128 / 100);

        default:
            break;
        }

        if (variable_edit_mode == EDIT_VARIABLE_INDEX_MODE)
            variable_edit_mode = EDIT_VARIABLE_NUM_MODE;
        else
            variable_edit_mode = EDIT_SELECT_MODE;

        break;

    case SDLK_ESCAPE:
        if (variable_edit_mode == EDIT_SELECT_MODE) {
            variable_edit_mode = NOT_EDIT_MODE;
            SDL_SetWindowTitle(screen, DEFAULT_WM_TITLE);
            SDL_Delay(100);
            SDL_SetWindowTitle(screen, wm_title_string);
            return;
        }

        variable_edit_mode = EDIT_SELECT_MODE;

    default:
        break;
    }

    if (variable_edit_mode == EDIT_SELECT_MODE) {
        wm_edit_string = EDIT_MODE_PREFIX EDIT_SELECT_STRING;
    }
    else if (variable_edit_mode == EDIT_VARIABLE_INDEX_MODE) {
        wm_edit_string.format(EDIT_MODE_PREFIX "Variable Index?  %%%d",
			      variable_edit_sign * variable_edit_num);
    }
    else if (variable_edit_mode >= EDIT_VARIABLE_NUM_MODE) {
        int p = 0;
	pstring var_name;

        switch (variable_edit_mode) {
        case EDIT_VARIABLE_NUM_MODE:
	    var_name.format("%%%d", variable_edit_index);
	    p = script_h.getVariableData(variable_edit_index).get_num();
            break;

        case EDIT_MP3_VOLUME_MODE:
            var_name = "MP3 Volume"; p = music_volume; break;

        case EDIT_VOICE_VOLUME_MODE:
            var_name = "Voice Volume"; p = voice_volume; break;

        case EDIT_SE_VOLUME_MODE:
            var_name = "Sound effect Volume"; p = se_volume; break;

        default:
            var_name = "";
        }

	wm_edit_string.format(EDIT_MODE_PREFIX "Current %s=%d  New value? %d",
			      (const char*) var_name, p,
			      variable_edit_num * variable_edit_sign);
    }

    SDL_SetWindowTitle(screen, wm_title_string);
    //TODO
    //SDL_SetWindowIcon(screen, wm_icon_string);
}


void PonscripterLabel::shiftCursorOnButton(int diff)
{
    if (buttons.size() < 2)
	shortcut_mouse_line = buttons.begin();
    else if (diff > 0) {
	while (diff--) {
	    do {
		if (shortcut_mouse_line == buttons.begin())
		    shortcut_mouse_line = buttons.end();
		--shortcut_mouse_line;
	    } while (shortcut_mouse_line->first == 0);
	}
    }
    else {
	while (diff++) {
	    do {
		++shortcut_mouse_line;
		if (shortcut_mouse_line == buttons.end())
		    shortcut_mouse_line = buttons.begin();
	    } while (shortcut_mouse_line->first == 0);
	}
    }
    ButtonElt& e = shortcut_mouse_line->second;

    int x = e.select_rect.x + e.select_rect.w / 2;
    int y = e.select_rect.y + e.select_rect.h / 2;
    if (x < 0) x = 0; else if (x >= screen_width) x = screen_width - 1;
    if (y < 0) y = 0; else if (y >= screen_height) y = screen_height - 1;
    warpMouse(x, y);
}


void PonscripterLabel::keyDownEvent(SDL_KeyboardEvent* event)
{
    switch (event->keysym.sym) {
    case SDLK_RCTRL:
        ctrl_pressed_status |= 0x01;
        goto ctrl_pressed;
    case SDLK_LCTRL:
        ctrl_pressed_status |= 0x02;
        goto ctrl_pressed;
    case SDLK_RSHIFT:
        shift_pressed_status |= 0x01;
        break;
    case SDLK_LSHIFT:
        shift_pressed_status |= 0x02;
        break;
    default:
        break;
    }

    return;

    ctrl_pressed:
    current_button_state.button  = 0;
    volatile_button_state.button = 0;
    playClickVoice();
    stopAnimation(clickstr_state);
    advancePhase();
    return;
}


void PonscripterLabel::keyUpEvent(SDL_KeyboardEvent* event)
{
    switch (event->keysym.sym) {
    case SDLK_RCTRL:
        ctrl_pressed_status &= ~0x01;
        break;
    case SDLK_LCTRL:
        ctrl_pressed_status &= ~0x02;
        break;
    case SDLK_RSHIFT:
        shift_pressed_status &= ~0x01;
        break;
    case SDLK_LSHIFT:
        shift_pressed_status &= ~0x02;
        break;
    default:
        break;
    }
}


void PonscripterLabel::keyPressEvent(SDL_KeyboardEvent* event)
{
    current_button_state.button = 0;
    current_button_state.down_flag = false;

    // This flag is set by anything before autmode that would like to not interrupt automode.
    // At present, this is volume mute and fullscreen. The commands don't simply return in case
    // the keypresses are handled below (e.g. telling the script the key 'a' was pressed)
    bool automode_ignore = false;

    if (event->type == SDL_KEYUP) {
        if (variable_edit_mode) {
            variableEditMode(event);
            return;
        }

        if (event->keysym.sym == SDLK_m) {
            volume_on_flag = !volume_on_flag;
            setVolumeMute(!volume_on_flag);
            printf("turned %s volume mute\n", !volume_on_flag?"on":"off");
            automode_ignore = true;
        }

        if (event->keysym.sym == SDLK_f) {
            if (fullscreen_mode) menu_windowCommand("menu_window");
            else menu_fullCommand("menu_full");
            return;
            automode_ignore = true;
        }

        if (edit_flag && event->keysym.sym == SDLK_z) {
            variable_edit_mode = EDIT_SELECT_MODE;
            variable_edit_sign = 1;
            variable_edit_num  = 0;
            wm_edit_string = EDIT_MODE_PREFIX EDIT_SELECT_STRING;
            SDL_SetWindowTitle(screen, wm_title_string);
        }
    }

    if (automode_flag && !automode_ignore) {
        remaining_time = -1;
        setAutoMode(false);
        return;
    }

    if (event->type == SDL_KEYUP
        && (event->keysym.sym == SDLK_RETURN
            || event->keysym.sym == SDLK_KP_ENTER
            || event->keysym.sym == SDLK_SPACE
            || event->keysym.sym == SDLK_s))
        setSkipMode(false);

    if (shift_pressed_status && event->keysym.sym == SDLK_q &&
	current_mode == NORMAL_MODE) {
        endCommand("end");
    }

    if ((trap_mode & TRAP_LEFT_CLICK)
        && (event->keysym.sym == SDLK_RETURN
            || event->keysym.sym == SDLK_KP_ENTER
            || event->keysym.sym == SDLK_SPACE)) {
        trapHandler();
        return;
    }
    else if ((trap_mode & TRAP_RIGHT_CLICK)
             && (event->keysym.sym == SDLK_ESCAPE)) {
        trapHandler();
        return;
    }

    const bool wait_button_mode = event_mode & WAIT_BUTTON_MODE;
    const bool key_or_btn = event->type == SDL_KEYUP || btndown_flag;
    const bool enter_key = !getenter_flag &&
	                   (event->keysym.sym == SDLK_RETURN ||
			    event->keysym.sym == SDLK_KP_ENTER);
    const bool space_lclick = spclclk_flag || !useescspc_flag;
    const bool space_key = event->keysym.sym == SDLK_SPACE;
    if (wait_button_mode && ((key_or_btn && enter_key) ||
			     (space_lclick && space_key))) {
	if (event->keysym.sym == SDLK_RETURN   ||
	    event->keysym.sym == SDLK_KP_ENTER ||
	    (spclclk_flag && event->keysym.sym == SDLK_SPACE))
	{
      current_button_state.button =
        volatile_button_state.button = current_over_button;
      if (event->type == SDL_KEYDOWN) {
          current_button_state.down_flag = true;
      }
  }
  else {
	    current_button_state.button =
	    volatile_button_state.button = 0;
	}
	playClickVoice();
	stopAnimation(clickstr_state);
	advancePhase();
	return;
    }

    if (event->type == SDL_KEYDOWN) return;

    if ((event_mode & (WAIT_INPUT_MODE | WAIT_BUTTON_MODE))
        && (autoclick_time == 0 || (event_mode & WAIT_BUTTON_MODE))) {
        if (!useescspc_flag && event->keysym.sym == SDLK_ESCAPE) {
            current_button_state.button = -1;
            if (rmode_flag && event_mode & WAIT_TEXT_MODE) {
                if (!rmenu.empty())
                    system_menu_mode = SYSTEM_MENU;
                else
                    system_menu_mode = SYSTEM_WINDOWERASE;
            }
        }
        else if (useescspc_flag && event->keysym.sym == SDLK_ESCAPE) {
            current_button_state.button = -10;
        }
        else if (!spclclk_flag && useescspc_flag && event->keysym.sym == SDLK_SPACE) {
            current_button_state.button = -11;
        }
        else if (((!getcursor_flag && event->keysym.sym == SDLK_LEFT) ||
                  event->keysym.sym == SDLK_h) &&
                 (event_mode & WAIT_TEXT_MODE ||
                  (usewheel_flag && !getcursor_flag &&
                   event_mode & WAIT_BUTTON_MODE) ||
                  system_menu_mode == SYSTEM_LOOKBACK))
	{
	    current_button_state.button  = -2;
            volatile_button_state.button = -2;
            if (event_mode & WAIT_TEXT_MODE) system_menu_mode = SYSTEM_LOOKBACK;
        }
        else if (((!getcursor_flag && event->keysym.sym == SDLK_RIGHT) ||
                  event->keysym.sym == SDLK_l) &&
                 ((enable_wheeldown_advance_flag &&
                   event_mode & WAIT_TEXT_MODE) ||
		  (usewheel_flag && event_mode & WAIT_BUTTON_MODE) ||
                  system_menu_mode == SYSTEM_LOOKBACK))
	{
	    if (event_mode & WAIT_TEXT_MODE) {
                current_button_state.button  = 0;
                volatile_button_state.button = 0;
            }
            else {
                current_button_state.button  = -3;
                volatile_button_state.button = -3;
            }
        }
	else if (((!getcursor_flag && event->keysym.sym == SDLK_UP) ||
                  event->keysym.sym == SDLK_k ||
                  event->keysym.sym == SDLK_p) &&
                 event_mode & WAIT_BUTTON_MODE){
            shiftCursorOnButton(1);
            return;
        }
        else if (((!getcursor_flag && event->keysym.sym == SDLK_DOWN) ||
                  event->keysym.sym == SDLK_j ||
                  event->keysym.sym == SDLK_n) &&
                 event_mode & WAIT_BUTTON_MODE){
            shiftCursorOnButton(-1);
            return;
        }
	else if (getpageup_flag && event->keysym.sym == SDLK_PAGEUP) {
            current_button_state.button = -12;
        }
        else if (getpagedown_flag && event->keysym.sym == SDLK_PAGEDOWN) {
            current_button_state.button = -13;
        }
        else if ((getenter_flag && event->keysym.sym == SDLK_RETURN)
                 || (getenter_flag && event->keysym.sym == SDLK_KP_ENTER)) {
            current_button_state.button = -19;
        }
        else if (gettab_flag && event->keysym.sym == SDLK_TAB) {
            current_button_state.button = -20;
        }
        else if (getcursor_flag && event->keysym.sym == SDLK_UP) {
            current_button_state.button = -40;
        }
        else if (getcursor_flag && event->keysym.sym == SDLK_RIGHT) {
            current_button_state.button = -41;
        }
        else if (getcursor_flag && event->keysym.sym == SDLK_DOWN) {
            current_button_state.button = -42;
        }
        else if (getcursor_flag && event->keysym.sym == SDLK_LEFT) {
            current_button_state.button = -43;
        }
        else if (getinsert_flag && event->keysym.sym == SDLK_INSERT) {
            current_button_state.button = -50;
        }
        else if (getzxc_flag && event->keysym.sym == SDLK_z) {
            current_button_state.button = -51;
        }
        else if (getzxc_flag && event->keysym.sym == SDLK_x) {
            current_button_state.button = -52;
        }
        else if (getzxc_flag && event->keysym.sym == SDLK_c) {
            current_button_state.button = -53;
        }
        else if (getfunction_flag) {
            if (event->keysym.sym == SDLK_F1)
                current_button_state.button = -21;
            else if (event->keysym.sym == SDLK_F2)
                current_button_state.button = -22;
            else if (event->keysym.sym == SDLK_F3)
                current_button_state.button = -23;
            else if (event->keysym.sym == SDLK_F4)
                current_button_state.button = -24;
            else if (event->keysym.sym == SDLK_F5)
                current_button_state.button = -25;
            else if (event->keysym.sym == SDLK_F6)
                current_button_state.button = -26;
            else if (event->keysym.sym == SDLK_F7)
                current_button_state.button = -27;
            else if (event->keysym.sym == SDLK_F8)
                current_button_state.button = -28;
            else if (event->keysym.sym == SDLK_F9)
                current_button_state.button = -29;
            else if (event->keysym.sym == SDLK_F10)
                current_button_state.button = -30;
            else if (event->keysym.sym == SDLK_F11)
                current_button_state.button = -31;
            else if (event->keysym.sym == SDLK_F12)
                current_button_state.button = -32;
        }

        if (current_button_state.button != 0) {
            volatile_button_state.button = current_button_state.button;
            stopAnimation(clickstr_state);
            advancePhase();
            return;
        }
    }

    if (event_mode & WAIT_INPUT_MODE && !key_pressed_flag
        && (autoclick_time == 0 || (event_mode & WAIT_BUTTON_MODE))) {
        if (event->keysym.sym == SDLK_RETURN
            || event->keysym.sym == SDLK_KP_ENTER
            || event->keysym.sym == SDLK_SPACE) {
            key_pressed_flag = true;
            playClickVoice();
            stopAnimation(clickstr_state);
            advancePhase();
        }
    }

    if (event_mode & (WAIT_INPUT_MODE | WAIT_TEXTBTN_MODE)
        && !key_pressed_flag) {
        if (event->keysym.sym == SDLK_s && !automode_flag) {
            setSkipMode(true);
            //printf("toggle skip to true\n");
            key_pressed_flag = true;
            stopAnimation(clickstr_state);
            advancePhase();
        }
        else if (event->keysym.sym == SDLK_o) {
            draw_one_page_flag = !draw_one_page_flag;
            //printf("toggle draw one page flag to %s\n", (draw_one_page_flag ? "true" : "false"));
            if (draw_one_page_flag) {
                stopAnimation(clickstr_state);
                advancePhase();
            }
        }
        else if (event->keysym.sym == SDLK_a && mode_ext_flag &&
                 !automode_flag)
        {
            setAutoMode(true);
            //printf("change to automode\n");
            key_pressed_flag = true;
            stopAnimation(clickstr_state);
            advancePhase();
        }
        else if (event->keysym.sym == SDLK_0) {
            if (++text_speed_no > 2) text_speed_no = 0;

            sentence_font.wait_time = -1;
        }
        else if (event->keysym.sym == SDLK_1) {
            text_speed_no = 0;
            sentence_font.wait_time = -1;
        }
        else if (event->keysym.sym == SDLK_2) {
            text_speed_no = 1;
            sentence_font.wait_time = -1;
        }
        else if (event->keysym.sym == SDLK_3) {
            text_speed_no = 2;
            sentence_font.wait_time = -1;
        }
    }

    if (event_mode & WAIT_SLEEP_MODE) {
      if (event->keysym.sym == SDLK_RETURN ||
          event->keysym.sym == SDLK_KP_ENTER ||
          event->keysym.sym == SDLK_SPACE) {
        skip_to_wait = 1;
      }
    }
}


void PonscripterLabel::timerEvent(void)
{
    timerEventTop:

    int ret;

    if (event_mode & WAIT_TIMER_MODE) {
        int duration = proceedAnimation();

        if (duration == 0
            || (remaining_time >= 0
                && remaining_time - duration <= 0)) {
            bool end_flag  = true;
            bool loop_flag = false;
            if (remaining_time >= 0) {
                remaining_time = -1;
                if (event_mode & WAIT_VOICE_MODE && wave_sample[0]) {
                    end_flag = false;
                    if (duration > 0) {
                        resetRemainingTime(duration);
                        advancePhase(duration);
                    }
                }
                else {
                    loop_flag = true;
                    if (automode_flag || autoclick_time > 0)
                        current_button_state.button = 0;
                    else if (usewheel_flag)
                        current_button_state.button = -5;
                    else
                        current_button_state.button = -2;
                }
            }

            if (end_flag
                && event_mode & (WAIT_INPUT_MODE | WAIT_BUTTON_MODE)
                && (clickstr_state == CLICK_WAIT
                    || clickstr_state == CLICK_NEWPAGE)) {
                playClickVoice();
                stopAnimation(clickstr_state);
            }

            if (end_flag || duration == 0)
                event_mode &= ~WAIT_TIMER_MODE;

            if (loop_flag) goto timerEventTop;
        }
        else {
            if (remaining_time > 0)
                remaining_time -= duration;

            resetRemainingTime(duration);
            advancePhase(duration);
        }
    }
    else if (event_mode & EFFECT_EVENT_MODE) {
        const char* current = script_h.getCurrent();
        ret = this->parseLine();

        if (ret & RET_CONTINUE) {
            if (ret == RET_CONTINUE) {
                readToken(); // skip trailing \0 and mark kidoku
            }

            if (effect_blank == 0 || effect_counter == 0) goto timerEventTop;

            startTimer(effect_blank);
        }
        else {
            script_h.setCurrent(current);
            readToken();
            advancePhase();
        }

        return;
    }
    else {
        if (system_menu_mode != SYSTEM_NULL
            || (event_mode & WAIT_INPUT_MODE
                && volatile_button_state.button == -1)) {
            if (!system_menu_enter_flag)
                event_mode |= WAIT_TIMER_MODE;

            executeSystemCall();
        }
        else
            executeLabel();
    }

    volatile_button_state.button = 0;
}

Uint32 PonscripterLabel::getRefreshRateDelay() {
    SDL_DisplayMode mode;
    SDL_GetWindowDisplayMode(screen, &mode);
    if(mode.refresh_rate == 0) return 16; //~60 hz

    return 1000 / mode.refresh_rate;
}


/* **************************************** *
* Event loop
* **************************************** */
int PonscripterLabel::eventLoop()
{
    SDL_Event event, tmp_event;

    /* Note, this rate can change if the window is dragged to a new
       screen or the monitor settings are changed while running.
       We do not handle either of these cases */
    Uint32 refresh_delay = getRefreshRateDelay();
    Uint32 last_refresh = 0, current_time;
    timer_event_flag = false;
#ifdef WIN32
    Uint32 win_flags;
#endif

    queueRerender();

    advancePhase();

    // when we're on the first of a button-waiting frame (menu, etc), we snap mouse cursor to button when
    //   using keyboard/gamecontroller to vastly improve the experience when using not using a mouse directly
    bool using_buttonbased_movement = true;  // true to snap to main menu when it loads
    first_buttonwait_mode_frame = false;  // if it's the first frame of a buttonwait (menu/choice), snap to default button
    SDL_GetMouseState(&last_mouse_x, &last_mouse_y);

    while (SDL_WaitEvent(&event)) {
        // ignore continous SDL_MOUSEMOTION
        while (event.type == SDL_MOUSEMOTION) {
            if (SDL_PeepEvents(&tmp_event, 1, SDL_PEEKEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT) == 0) break;

            // improve using keyboard/gamecontroller controls
            if ((last_mouse_x != ( (SDL_MouseButtonEvent *) &event)->x) || (last_mouse_y != ( (SDL_MouseButtonEvent *) &event)->y)) {
                using_buttonbased_movement = false;

                last_mouse_x = ( (SDL_MouseButtonEvent *) &event)->x;
                last_mouse_y = ( (SDL_MouseButtonEvent *) &event)->y;
            }

            if (tmp_event.type != SDL_MOUSEMOTION) break;

            SDL_PeepEvents(&tmp_event, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT);
            event = tmp_event;
        }

        switch (event.type) {
        case SDL_MOUSEMOTION:
            mouseMoveEvent((SDL_MouseMotionEvent*) &event);
            break;

        case SDL_MOUSEBUTTONDOWN:
            current_button_state.down_x = ( (SDL_MouseButtonEvent *) &event)->x;
            current_button_state.down_y = ( (SDL_MouseButtonEvent *) &event)->y;
            current_button_state.ignore_mouseup = false;
            if (!btndown_flag) break;

        case SDL_MOUSEBUTTONUP:
            mousePressEvent((SDL_MouseButtonEvent*) &event);
            break;

        case SDL_MOUSEWHEEL:
            mouseWheelEvent(&event.wheel);
            break;

        // NOTE: we reverse KEYUP and KEYDOWN for controller presses, because otherwise it feels really slow and junky
        // If necessary, we can make keyPressEvent actually interpret controller keys but this works fine for now
        case SDL_CONTROLLERBUTTONDOWN:
            using_buttonbased_movement = true;
            event.key.type = SDL_KEYUP;
            // printf("Controller button press: %s\n", SDL_GameControllerGetStringForButton((SDL_GameControllerButton)event.cbutton.button));
            event.key.keysym.sym = transControllerButton(event.cbutton.button);
            if (event.key.keysym.sym == SDLK_UNKNOWN)
                break;

            event.key.keysym.sym = transKey(event.key.keysym.sym);

            keyDownEvent((SDL_KeyboardEvent*) &event);
            keyPressEvent((SDL_KeyboardEvent*) &event);

            break;

        case SDL_CONTROLLERBUTTONUP:
            using_buttonbased_movement = true;
            event.key.type = SDL_KEYDOWN;
            // printf("Controller button release: %s\n", SDL_GameControllerGetStringForButton((SDL_GameControllerButton)event.cbutton.button));
            event.key.keysym.sym = transControllerButton(event.cbutton.button);
            if (event.key.keysym.sym == SDLK_UNKNOWN)
                break;

            event.key.keysym.sym = transKey(event.key.keysym.sym);

            keyUpEvent((SDL_KeyboardEvent*) &event);
            if (btndown_flag)
                keyPressEvent((SDL_KeyboardEvent*) &event);

            break;

        case SDL_JOYBUTTONDOWN:
            using_buttonbased_movement = true;
            event.key.type = SDL_KEYDOWN;
            event.key.keysym.sym = transJoystickButton(event.jbutton.button);
            if (event.key.keysym.sym == SDLK_UNKNOWN)
                break;

        case SDL_KEYDOWN:
            if ((event.key.keysym.sym == SDLK_UP) || (event.key.keysym.sym == SDLK_DOWN) ||
                (event.key.keysym.sym == SDLK_LEFT) || (event.key.keysym.sym == SDLK_RIGHT))
                using_buttonbased_movement = true;
            event.key.keysym.sym = transKey(event.key.keysym.sym);
            keyDownEvent((SDL_KeyboardEvent*) &event);
            if (btndown_flag)
                keyPressEvent((SDL_KeyboardEvent*) &event);

            break;

        case SDL_JOYBUTTONUP:
            using_buttonbased_movement = true;
            event.key.type = SDL_KEYUP;
            event.key.keysym.sym = transJoystickButton(event.jbutton.button);
            if (event.key.keysym.sym == SDLK_UNKNOWN)
                break;

        case SDL_KEYUP:
            event.key.keysym.sym = transKey(event.key.keysym.sym);
            keyUpEvent((SDL_KeyboardEvent*) &event);
            keyPressEvent((SDL_KeyboardEvent*) &event);
            break;

        case SDL_JOYAXISMOTION:
        {
            SDL_KeyboardEvent ke = transJoystickAxis(event.jaxis);
            if (ke.keysym.sym != SDLK_UNKNOWN) {
                if (ke.type == SDL_KEYDOWN) {
                    keyDownEvent(&ke);
                    if (btndown_flag)
                        keyPressEvent(&ke);
                }
                else if (ke.type == SDL_KEYUP) {
                    keyUpEvent(&ke);
                    keyPressEvent(&ke);
                }
            }

            break;
        }

        case ONS_SOUND_EVENT:
        case ONS_FADE_EVENT:
        case ONS_MIDI_EVENT:
        case ONS_MUSIC_EVENT:
            flushEventSub(event);
            break;

        case INTERNAL_REDRAW_EVENT:
            /* Handle cursor shifting for controller/keyboard button-based movement */
            if (first_buttonwait_mode_frame && using_buttonbased_movement && buttons.size() > 1) {
                shiftCursorOnButton(0);
            }

            if (event_mode & WAIT_BUTTON_MODE)
                first_buttonwait_mode_frame = true;
            else if (first_buttonwait_mode_frame)
                first_buttonwait_mode_frame = false;

            /* Stop rerendering while minimized; wait for the restore event + queueRerender */
            if(minimized_flag) {
                break;
            }

            current_time = SDL_GetTicks();
            if((current_time - last_refresh) >= refresh_delay || last_refresh == 0) {
                /* It has been longer than the refresh delay since we last started a refresh. Start another */

                last_refresh = current_time;
                rerender();

                /* Refresh time since rerender does take some odd ms */
                current_time = SDL_GetTicks();
            }

            SDL_PumpEvents();
            /* Remove all pending redraw events on the queue */
            while(SDL_PeepEvents(&tmp_event, 1, SDL_GETEVENT, INTERNAL_REDRAW_EVENT, INTERNAL_REDRAW_EVENT) == 1)
                ;

            /* If there are any events on the queue, re-add us and let it get those events asap.
             * It'll then come back to us with no events and we'll just sleep until it's time to redraw again.
             * If there are no events, sleep right away
             */
            if(SDL_PeepEvents(&tmp_event, 1, SDL_PEEKEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT) == 0) {
                if(timer_event_flag && timer_event_time <= current_time) {
                    timer_event_flag = false;

                    timerEvent();
                } else if(last_refresh <= current_time && refresh_delay >= (current_time - last_refresh)) {
                    SDL_Delay(std::min(refresh_delay / 3, refresh_delay - (current_time - last_refresh)));
                }
            }
            tmp_event.type = INTERNAL_REDRAW_EVENT;
            SDL_PushEvent(&tmp_event);

            break;

        case ONS_WAVE_EVENT:
            flushEventSub(event);
            //printf("ONS_WAVE_EVENT %d: %x %d %x\n", event.user.code, wave_sample[0], automode_flag, event_mode);
            if (event.user.code != 0
                || !(event_mode & WAIT_VOICE_MODE)) break;

            if (remaining_time <= 0) {
                event_mode &= ~WAIT_VOICE_MODE;
                if (automode_flag)
                    current_button_state.button = 0;
                else if (usewheel_flag)
                    current_button_state.button = -5;
                else
                    current_button_state.button = -2;

                stopAnimation(clickstr_state);
                advancePhase();
            }

            break;

        case SDL_WINDOWEVENT:
            switch(event.window.event) {
              case SDL_WINDOWEVENT_FOCUS_LOST:
                break;
              case SDL_WINDOWEVENT_FOCUS_GAINED:
                /* See comment below under RESIZED */
                SDL_PumpEvents();
                SDL_PeepEvents(&tmp_event, 1, SDL_GETEVENT, SDL_MOUSEBUTTONDOWN, SDL_MOUSEBUTTONDOWN);
                current_button_state.ignore_mouseup = true;
#ifdef WIN32
                win_flags = SDL_GetWindowFlags(screen);
                /* Work around: https://bugzilla.libsdl.org/show_bug.cgi?id=2510
                 *
                 * On windows, the RESTORED event does not occur when you restore a
                 * maximized event. The only events you get are a ton of exposes and
                 * this one. The screen also remains black if it was maximized until
                 * the window is "restored".
                 */
                SDL_RestoreWindow(screen);
                if(win_flags & SDL_WINDOW_FULLSCREEN_DESKTOP) {
                    SDL_SetWindowFullscreen(screen, SDL_WINDOW_FULLSCREEN_DESKTOP);
                } else if(win_flags & SDL_WINDOW_FULLSCREEN) {
                    SDL_SetWindowFullscreen(screen, SDL_WINDOW_FULLSCREEN);
                } else if(win_flags & SDL_WINDOW_MAXIMIZED) {
                    SDL_MaximizeWindow(screen);
                }
#endif
                break;
              case SDL_WINDOWEVENT_MAXIMIZED:
              case SDL_WINDOWEVENT_RESIZED:
                /* Due to what I suspect is an SDL bug, you get a mosuedown +
                 * mouseup event when you maximize the window by double
                 * clicking the titlebar in windows. These events both have
                 * coordinates inside of the screen, and I can't see any way
                 * to tell them apart from legitimate clicks. (it even triggers
                 * a mouse move event).
                 * To fix this bug, we kill any mousedown events when we maximize.
                 * Note, we do this under RESIZED too because if you do a
                 * "vertical maximize" (double click with the upper resize arrow)
                 * that doesn't trigger a maximized event
                 */
                SDL_PumpEvents();
                SDL_PeepEvents(&tmp_event, 1, SDL_GETEVENT, SDL_MOUSEBUTTONDOWN, SDL_MOUSEBUTTONDOWN);
                current_button_state.ignore_mouseup = true;
              case SDL_WINDOWEVENT_RESTORED:
              case SDL_WINDOWEVENT_SHOWN:
              case SDL_WINDOWEVENT_EXPOSED:
                /* If we weren't minimized, a rerender is already queued */
                if(minimized_flag) {
                    minimized_flag = false;
                    queueRerender();
                }
                break;
              case SDL_WINDOWEVENT_MINIMIZED:
              case SDL_WINDOWEVENT_HIDDEN:
                minimized_flag = true;
                break;
            }
            break;

        case SDL_QUIT:
            endCommand("end");
            break;

        default:
            break;
        }
    }
    return -1;
}


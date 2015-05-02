#ifndef ACCESSIBILITY_H
#define ACCESSIBILITY_H

#include <fstream>
#include "pstring.h"
#include "pugixml.hpp"

#ifdef WIN32
#include <windows.h>

typedef void(__cdecl *TOLKVOIDTYPE)();
typedef bool(__cdecl *TOLKBOOLTYPE)();
typedef bool(__cdecl *TOLKOUTPUTTYPE)(LPWSTR, BOOL);
#endif

class Accessibility
{
public:
    pugi::xml_node d_translation;
    pstring lang_prefix;

    Accessibility();
    Accessibility(const pstring l_prefix, bool f_output);
    ~Accessibility();
    const pstring get_accessible(
        pstring t, const int sprite_id, const int button_id, const pstring cmd);
    void output(const pstring &text, const int num);
    void set_draw_one_page(const bool draw_one_page_flag);
    int get_bar_no();
    void set_config_buttons_flags(const bool flag, const int type);

private:
    int bar_no;
    pugi::xml_document doc;
    void push_output(const pstring text);
    std::fstream fs;
    pstring last_str;
    pstring last_str_buffer;
    pstring heading_text;
    pstring footnote_text;
    pstring last_heading;
    pstring last_output;
    pstring sentence;
    pstring last_input;
    bool file_output;
    pstring background_text;
    bool subtitles_on_flag;
    bool footnotes_on_flag;
    bool fullscreen_on_flag;
    bool footnote_flag;
    bool footnote;
    bool draw_one_page;
    bool need_outputbg;
    bool clipboard_output;
#ifdef WIN32
    HINSTANCE tolk_lib;
    TOLKVOIDTYPE tolk_load;
    TOLKVOIDTYPE tolk_unload;
    TOLKBOOLTYPE tolk_isloaded;
    TOLKOUTPUTTYPE tolk_output;
#endif

    const pugi::xml_node find_menu(const pstring &c);
    const pstring get_bg_text(const pugi::xml_node &background);
    const bool is_bg_inline(const pugi::xml_node &background);
    const pugi::xml_node find_bg(const pstring &c);
    const pugi::xml_node find_sprite(const pugi::xml_node &menu, const int sprite_id);
    const pstring get_button_text(const pugi::xml_node &menu, const int button_id);
    bool is_ok(const pstring &t, const int l, const int w);
    const pstring process_text_n(pstring t);
    const pstring process_text(pstring t);
    const pstring process_csel(const pstring &csel_text);
    const pstring process_saveload(const pstring &t);
    const pstring strip_ponscripter_tags(const pstring &t);
    const pstring process_history(const pstring &t);
};

#endif

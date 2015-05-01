#ifndef ACCESSIBILITY_H
#define ACCESSIBILITY_H

#include <fstream>
#include <algorithm>
#include "pstring.h"
#include "pugixml.hpp"

class Accessibility
{
public:
    Accessibility();
    Accessibility(const pstring l_prefix, bool f_output);
    ~Accessibility();
    const pstring make_accessible(pstring t, const int line_num, const int what, const pstring cmd);
    const pstring get_accessible(
        pstring t, const int sprite_id, const int button_id, const pstring cmd);
    // const pstring& make_accessible();
    void output(const pstring &text, const int num);
    void history_output(bool direction);
    void reset_currenthistoryline();
    bool is_footnote() const;
    void reset_footnote();
    pugi::xml_node d_translation;
    pstring lang_prefix;

private:
    void push_output(const pstring text);
    std::fstream fs;
    bool file_output;
    pstring last_t;
    int last_what;
    int last_linenum;
    pstring last_str;
    pstring last_str_buffer;
    pstring last_output;
    pstring sentence;
    pstring last_input;
    pstring background_text;
    std::vector<pstring> history;
    int current_historyline;
    bool subtitles_on;
    bool footnote_flag;
    bool footnote;
    bool ag_translation;
    bool need_outputbg;
    pugi::xml_document doc;
    pugi::xml_node find_menu(const pstring &c);
    const pstring get_bg_text(const pugi::xml_node &background) const;
    const bool is_bg_inline(const pugi::xml_node &background) const;
    const pugi::xml_node find_bg(const pstring &c) const;
    const pstring get_subtitle_text(const pugi::xml_node &subtitle) const;
    const pugi::xml_node find_subtitles(const pstring &c) const;
    const pugi::xml_node find_sprite(const pugi::xml_node &menu, const int sprite_id) const;
    const pstring get_button_text(const pugi::xml_node &menu, const int button_id) const;
    bool is_ok(const pstring &t, const int l, const int w) const;
    const pstring process_text_n(pstring t);
    const pstring process_text(pstring t);
    const pstring process_csel(const pstring &csel_text) const;
    pstring process_saveload(pstring t) const;
    void clear_history();
};

#endif

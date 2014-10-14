#include "Accessibility.h"
#include "bstrlib.h"

#ifdef MACOSX
#include <AppKit/NSSpeechSynthesizer.h>
#include <Foundation/NSString.h>

NSSpeechSynthesizer *ns;
#endif

Accessibility::Accessibility() : file_output(false)
{
    lang_prefix = "en";
}

Accessibility::Accessibility(const pstring l_prefix, bool f_output) : file_output(f_output)
{
    lang_prefix = l_prefix;

    doc.load_file("translations/trans.xml");

    // grab the default translation for menus/etc.
    pugi::xpath_variable_set vars;
    vars.add("default_translation_name", pugi::xpath_type_string);
    vars.add("default_translation_language", pugi::xpath_type_string);
    pugi::xpath_query query_default_translation("/translations/translation[@name = string($default_translation_name) and @lang = string($default_translation_language)]", &vars);
    vars.set("default_translation_name", "default");
    vars.set("default_translation_language", "en");
    pugi::xpath_node_set default_translations = query_default_translation.evaluate_node_set(doc);
    if(!default_translations.empty()){
        d_translation = default_translations[0].node();
    }

    // initializing
    last_str = "";
    last_str_buffer = "";

#ifdef MACOSX
    ns = [[NSSpeechSynthesizer alloc] init];
#endif
}

Accessibility::~Accessibility()
{
    // pugixml destroys itself while it's not in the buffer?
}

const pstring Accessibility::get_accessible(pstring t, const int sprite_id, const int button_id, const pstring cmd)
{
    // printf("get_accessible called: %d %d [%s] [%s]\n", sprite_id, button_id, (const char*)cmd, (const char*)t);
    if(cmd == ""){
        if(is_ok(t, sprite_id, button_id)){
            pugi::xml_node menu = find_menu(t);
            if(!menu.empty()){
                if(sprite_id != 255){
                    pugi::xml_node sprite = find_sprite(menu, sprite_id);
                    if(!sprite.empty()){
                        return get_button_text(sprite, button_id);
                    }
                }else{
                    return get_button_text(menu, button_id);
                }
            }
        }
    }else if(cmd == "lsp"){
        pugi::xml_node subtitles = find_subtitles(t);
        if(!subtitles.empty()){
                subtitles_on = true;
                return get_subtitle_text(subtitles);
        }
    }else if(cmd == "csel"){
        //clear_history();
        //current_historyline = 0;
        return process_csel(t);
    }else if(cmd == "bg"){
        if(last_input == t){
            return "";
        }
        last_input = t;
        pugi::xml_node background = find_bg(t);
        if(!background.empty()){
            if(is_bg_inline(background)){
                need_outputbg = true;
            }else{
                need_outputbg = false;
            }
            pstring temp = get_bg_text(background);
            if(!need_outputbg){
                background_text = background_text + '\n' + temp;
                return "";
            }
            return temp;
        }
    } else if(cmd == "text") {
        if (last_input == t) {
            return "";
        }
        last_input = t;
        if (!sentence) {
            //fs << "ingoing!!! ->" << t << '\n';
            pstring temp = process_text_n(t);
            //fs << "outgoing!!! <-" << temp << '\n';
            if (temp) {
                history.push_back(temp);
                ++current_historyline;
            }
            return temp;
        }
        pstring temp = process_text_n(t);
        //fs << "ingoing ->" << t << '\n';
        if (temp) {
            sentence = "";
            //fs << "outgoing_inside <-" << temp << '\n';
            history.push_back(temp);
            ++current_historyline;
            return temp;
        }
        //fs << "outgoing_outside <-" << temp << '\n';
        return "";
    }

    return "";
}

const pstring Accessibility::get_bg_text(const pugi::xml_node& background) const
{
    pugi::xpath_node_set background_texts = background.select_nodes("content");
    if(!background_texts.empty()){
        return background_texts[0].node().text().get();
    }

    return "";
}

const bool Accessibility::is_bg_inline(const pugi::xml_node& background) const
{
    pugi::xpath_node_set inline_bg = background.select_nodes("inline");
    if(!inline_bg.empty()){
        return true;
    }

    return false;
}

const pugi::xml_node Accessibility::find_bg(const pstring& c) const
{
    pugi::xpath_node_set backgrounds = doc.select_nodes("//backgrounds/background/src");
    if(!backgrounds.empty()){
        pugi::xml_node parent;
        for (pugi::xpath_node_set::const_iterator it = backgrounds.begin(); it != backgrounds.end(); ++it){
            pugi::xpath_node node = *it;
            if(!strcmp(node.node().text().get(), c)){
                parent = node.node().parent();
                return parent;
            }
        }
    }

    return pugi::xml_node();
}

const pstring Accessibility::get_subtitle_text(const pugi::xml_node& subtitle) const
{
    pugi::xpath_node_set subtitle_text = subtitle.select_nodes("content");
    if(!subtitle_text.empty()){
        return subtitle_text[0].node().text().get();
    }

    return "";
}

const pugi::xml_node Accessibility::find_subtitles(const pstring& c) const
{
    pugi::xpath_node_set subtitles = doc.select_nodes("//subtitles/subtitle/src");
    if(!subtitles.empty()){
        pugi::xml_node parent;
        for (pugi::xpath_node_set::const_iterator it = subtitles.begin(); it != subtitles.end(); ++it){
            pugi::xpath_node node = *it;
            if(!strcmp(node.node().text().get(), c)){
                parent = node.node().parent();
                return parent;
            }
        }
    }

    return pugi::xml_node();
}

const pugi::xml_node Accessibility::find_sprite(const pugi::xml_node& menu, const int sprite_id) const
{
    pugi::xpath_variable_set vars;
    vars.add("sprite_id", pugi::xpath_type_number);
    pugi::xpath_query query_sprite("sprites/sprite[@id = $sprite_id]", &vars);
    vars.set("sprite_id", (double)sprite_id);
    pugi::xpath_node_set sprites = menu.select_nodes(query_sprite);
    if(!sprites.empty()){
        return sprites[0].node();
    }
    return pugi::xml_node();
}

const pstring Accessibility::get_button_text(const pugi::xml_node& menu, const int button_id) const
{
    pugi::xpath_variable_set vars;
    vars.add("button_id", pugi::xpath_type_number);
    pugi::xpath_query query_button("buttons/button[@id = $button_id]", &vars);
    vars.set("button_id", (double)button_id);
    pugi::xpath_node_set buttons = menu.select_nodes(query_button);
    if(!buttons.empty()){
        return buttons[0].node().text().get();
    }
    return "";
}

pugi::xml_node Accessibility::find_menu(const pstring& c)
{
    pugi::xpath_node_set menus = d_translation.select_nodes("menus/menu/src");
    pugi::xml_node parent;
    for (pugi::xpath_node_set::const_iterator it = menus.begin(); it != menus.end(); ++it){
        pugi::xpath_node node = *it;
        if(!strcmp(node.node().text().get(), c)){
            parent = node.node().parent();
            break;
        }
    }

    // TO-DO lang prefix dependance
    if(parent.empty()){
        pugi::xpath_node_set translations = doc.select_nodes("/translations/translation[@id]");
        if(!translations.empty()){
            bool need_brake = false;
            for (pugi::xpath_node_set::const_iterator itt = translations.begin(); itt != translations.end(); ++itt){
                if(need_brake) break;
                pugi::xpath_node node_t = *itt;
                pugi::xpath_node_set menus = node_t.node().select_nodes("menus/menu/src");
                for (pugi::xpath_node_set::const_iterator it = menus.begin(); it != menus.end(); ++it){
                    pugi::xpath_node node = *it;
                    if(!strcmp(node.node().text().get(), c)){
                        parent = node.node().parent();
                        need_brake = true;
                        break;
                    }
                }
            }
        }
    }

    return parent;
}

const pstring Accessibility::make_accessible(pstring t, const int line_num, const int what, const pstring cmd)
{
    /*if(cmd == ""){
        //fs << "ingoing empty ->" << t << " line ->" << line_num << " what->" << what;
        if(is_ok(t, line_num, what)){
            pstring temp;
            if((temp = process_saveload(t))){
                return temp;
            }
            if((p = find_page(t))){
                if(p->title == "title_on2" || p->title == "title_on22" || p->title == "title_nar1lim"){
                    if(history.size() > 0){
                        clear_history();
                        current_historyline = 0;
                    }
                }
                last_t = t;
                last_what = what;
                pstring tempo = locale->translate(p->title, line_num, what);
                //fs << " outgoing empty ->" << tempo;
                return tempo;
                //return locale->translate(p->title, line_num, what);
            }
        }
    }else if(cmd == "text"){
        if(last_input == t){
            return "";
        }
        last_input = t;
        if(!sentence){
            //fs << "ingoing!!! ->" << t << '\n';
            pstring temp = process_text(t);
            //fs << "outgoing!!! <-" << temp << '\n';
            if(temp){
                history.push_back(temp);
                ++current_historyline;
            }
            return temp;
        }
        pstring temp = process_text(t);
        //fs << "ingoing ->" << t << '\n';
        if(temp){
            sentence = "";
            //fs << "outgoing_inside <-" << temp << '\n';
            history.push_back(temp);
            ++current_historyline;
            return temp;
        }
        //fs << "outgoing_outside <-" << temp << '\n';
        return "";
    }else if(cmd == "csel"){
        clear_history();
        current_historyline = 0;
        return process_csel(t);
    }else if(cmd == "bg"){
        if(last_input == t){
            return "";
        }
        last_input = t;
        pstring temp = "";
        if((temp = process_bg(t))){
            if(temp[temp.length() - 1] == '1'){             //1 at the end symbolize need of output a bg text
                need_outputbg = true;
                pstring str = "";
                for(int i = 0; i < temp.length() - 1; ++i){
                    str += temp[i];
                }
                temp = str;
            }else{
                need_outputbg = false;
            }
            if(!need_outputbg){
                background_text = background_text + '\n' + temp;
                return "";
            }
            return temp;
        }
        return "";
    }else if(cmd == "lsp"){
        if(locale->has_agskey(t)){
                subtitles_on = true;
                return locale->agilis_subtitles(t);
        }
        return "";
    }
    */
    return "";
}

void Accessibility::history_output(bool direction)
{
    if(history.size() > 0){
        if(direction){                                                                              // down in history
            --current_historyline;
            if((unsigned int)current_historyline == history.size() - 1){    //we don't need last element
                --current_historyline;
            }
            if(current_historyline > 0 && (unsigned int)current_historyline < history.size() -1){
                output(history[current_historyline], 888);
            }else{
                current_historyline = 0;
                output(history[current_historyline], 888);
            }
        }
        if(!direction){                                                                             //up in history
            ++current_historyline;
            if(current_historyline >= 0 && (unsigned int)current_historyline < history.size() - 1){
                output(history[current_historyline], 888);
            }else{
                current_historyline = history.size();
            }
        }
    }
}

void Accessibility::reset_currenthistoryline()
{
    current_historyline = history.size();
}

void Accessibility::clear_history()
{
    history.clear();
}

pstring Accessibility::process_saveload(pstring t) const
{
    if(t.length() > 34){
        if(t[11] == '#' && t[24] == 'B' && t[25] != '^'){           // :s/14,14,1;#EEFCFD#99CCFB
            pstring accessible_text = "";
            accessible_text = accessible_text + t[25] + t[26] + ": ";                   // day
            for(int i = 29; i < t.length(); ++i){                           // skip space after day number
                if(t[i] != '^'){                                                        // read month name
                    accessible_text += t[i];
                    continue;
                }
                return accessible_text + ' ' + t[i+2] + t[i+3] + ':' + t[i+5] + t[i+6]; // adding time in ex. 13:54
            }
        }
    }

    return "";
}

bool Accessibility::is_ok(const pstring& t, const int l, const int w) const
{
    if(t){
        if(l == 255){
            return true;
        }else if(l < 151 || l > 215){   // min num in Page.line_nums is 171, max - 215 // 151-170 for save/load
            return false;
        }else if(w < 1 || w > 41){      // there is no item with num = 0 or with num > 41
            return false;
        }
        return true;
    }

    return false;
}

bool Accessibility::is_footnote() const
{
    return footnote;
}

void Accessibility::reset_footnote()
{
    footnote_flag = false;
    footnote = false;
}

// given the input line, output a decently screen-readable string
const pstring Accessibility::process_text_n(pstring t)
{
    char start_char = (unsigned char)t.data[0];

    // standard lines of text
    if (start_char != '!') {
        unsigned int proper_i = t.length(); // index of the last proper character (not whitespace)
        unsigned char proper_char = ' ';

        while (proper_i > 0) {
            proper_char = (unsigned char)t.data[proper_i];

            if ((proper_char == 0) || (proper_char == ' ') || (proper_char == '\n') || (proper_char == '\t')) {
                proper_i--;
            } else {
                break;
            }
        }

        if ((proper_char == '\\') || (proper_char == '@')) {
            last_str_buffer += t.midstr(0, proper_i);

            pstring output_text = last_str_buffer;
            last_str_buffer = "";
            return (const char*)output_text;
        } else if (proper_i == 0) {
            if (last_str_buffer.length() > 0) {
                pstring output_text = last_str_buffer;
                last_str_buffer = "";
                return (const char*)output_text;
            }
        } else {
            proper_i++;

            last_str_buffer += t.midstr(0, proper_i);
            last_str_buffer += ' ';
        }
    }

    // commands!
    else {
        printf("cmd: %s\n", (const char*)t);
    }

    return "";
}

const pstring Accessibility::process_text(pstring t)
{
    if(t == " \n"){ // gp32
        return "";
    }
    pstring accessible_text = "";
    for(int i = 0; i < t.length(); ++i){
        if(t[i] == '!' && t[i+1] == 's' && t[i+2] == 'd'){  // !sd
            i += 3;
            continue;
        }
        if(t[i] == '!' && t[i+1] == 's' && t[i+2] == '0'){  // !s0~i %120 x-20 y-40~ (including space after) (csel heading begins here)
            i += 13;
            continue;
        }
        if(t[i] == 0x12 && t[i+1] == 0x17 && t[i+2] == 0xFF && t[i+3] == 0xFF && t[i+4] == '!'){    // ~i =0~!sd\n (csel heading ends here)
            i += 8;
            accessible_text += ".\n";                           // separate headings into sentences
            if(!sentence){
                sentence = accessible_text;
                return "";
            }                                                                   // next time we're here on a second heading
            sentence += accessible_text;
            return "";
        }
        if(t[i] == 0x19 && t[i+3] == '\n'){                     //~%90~\n
            i += 3;
            continue;
        }
        if(t[i] == 0x19 && t[i+3] == '*' && t[i+7] == '/'){     // ~%90~*~=0~/
            i +=7;
            footnote_flag = true;
            continue;
        }
        if(t[i] == 0x19 && t[i+3] == '*'){                      //~%70~* ~n~ and ~%90~*~=0~
            i +=6;
            continue;
        }
        // # chars
        if(t[i] == '#'){
            if(t[i+2] == '.'){                      // #1. etc 1digit
                accessible_text += '#';
                continue;
            }
            if(t[i+2] == ' ' || t[i+3] == ' '){ // #1 etc 1-2digits
                accessible_text += '#';
                continue;
            }
            if(t[i+7] == '/'){
                i += 7;
                if(footnote_flag){
                    footnote = true;
                }
                continue;
            }
            i += 6;                                 //ffaaff
            continue;
        }

        // ~=0~^@^  //TO-DO
        if(t[i] == 0x2E && t[i+1] == 0x2E && t[i+2] == 0x16 && t[i+3] == '\n'){                     // ~s~^\n   gp32 on runtime look here
            if(!sentence){
                sentence = accessible_text + ". \n";
                return "";
            }
            sentence += accessible_text;
            return "";
        }
        if(t[i] >= 0x10 && t[i] < 0x20){                        // ~i~ etc.
            continue;
        }

        // ! commands
        if(t[i] == '!'){
            // !s commands
            if(t[i+1] == 's'){
                if(t[i+2] == '8'){              // !s80 !s85
                    i += 4;
                    continue;
                }
                if(t[i+2] == '9'){              // !s90
                    i += 4;
                    continue;
                }
                if(t[i+2] == '7'){              // !s75  !s70
                    i +=4;
                    continue;
                }
                if(t[i+2] == '1'){              // !s100 !s120 !s140
                    i += 5;
                    continue;
                }
                if(t[i+2] == '6'){              //!s65
                    i += 4;
                    continue;
                }
                if(t[i+2] == '2'){              // !s20
                    i+=4;
                    continue;
                }
                if(t[i+2] == '3'){              // !s35
                    i+=4;
                    continue;
                }
            }
            // !w commands
            if(t[i+1] == 'w'){
                if(t[i+2] == '1' && t[i+3] == '5'){ // !w1500
                    i += 6;
                    continue;
                }
                if(t[i+2] == '1' && t[i+3] == '0' && t[i+4] == '0' && t[i+5] != '0'){       // !w100
                    i += 5;
                    continue;
                }
                if(t[i+2] == '1' && t[i+3] == '0' &&  t[i+4] == '0' && t[i+5] == '0'){  // !w1000
                    i += 6;
                    continue;
                }
                if(t[i+2] == '2' && t[i+3] == '5'){ // !w2500
                    i += 6;
                    continue;
                }
                if(t[i+2] == '2' && t[i+3] == '0' && t[i+4] == '0' && t[i+5] != '0'){       // !w200
                    i += 5;
                    continue;
                }
                if(t[i+2] == '2' && t[i+3] == '0' && t[i+4] == '0' && t[i+5] == '0'){       // !w2000
                    i+=6;
                    continue;
                }
                if(t[i+2] == '3' && t[i+3] == '5'){ // !w3500
                    i += 6;
                    continue;
                }
                if(t[i+2] == '3' && t[i+3] == '0'){ // !w300
                    i += 5;
                    continue;
                }
                if(t[i+2] == '4' && t[i+3] == '0'){ // !w400
                    i += 5;
                    continue;
                }
                if(t[i+2] == '5' && t[i+3] == '0'){ // !w500
                    i += 5;
                    continue;
                }
                if(t[i+2] == '6' && t[i+3] == '0'){ // !w600
                    i += 5;
                    continue;
                }
                if(t[i+2] == '7' && t[i+3] == '0'){ // !w700
                    i += 5;
                    continue;
                }
                if(t[i+2] == '8' && t[i+3] == '0'){ // !w800
                    i += 5;
                    continue;
                }
            }
        }
        if(t[i] == '.' && t[i+1] == '\n'){                              // .\n
            if(!sentence){
                sentence = accessible_text + ". \n";
                return "";
            }
            sentence = sentence + accessible_text + ". ";
            return "";
        }
        if(t[i] == '.' && t[i+1] == ' ' && t[i+2] == '\n'){     // . \n
            if(!sentence){
                sentence = accessible_text + ". \n";
                return "";
            }
            sentence = sentence + accessible_text + ". ";
            return "";
        }
        if(t[i] == '`' && t[i+1] == '|' && t[i+2] == '`'){      // `|`
            i +=3;
            accessible_text += '\'';
            continue;
        }
        if(t[i] == '\'' && t[i+1] == '|' && t[i+2] == '\''){        // '|'
            i +=3;
            accessible_text += '\'';
            continue;
        }
        if(t[i] == '\'' && t[i+1] == '\'' && t[i+2] == '\n'){       // ''\n
            if(!sentence){
                sentence = accessible_text + ". \n";
                return "";
            }
            sentence += accessible_text;
            return "";
        }
        if(t[i] == '\'' && t[i+1] == '\n'){                             // '\n
            if(!sentence){
                sentence = accessible_text + ". \n";
                return "";
            }
            sentence += accessible_text;
            return "";
        }
        if(t[i] == '*' && t[i+1] == '\n'){                              // *\n
            if(!sentence){
                sentence = accessible_text + ". \n";
                return "";
            }
            sentence += accessible_text;
            return "";
        }
        if(t[i] == '?' && t[i+1] == '\n'){                              // ?\n
            if(!sentence){
                sentence = accessible_text + ". \n";
                return "";
            }
            sentence += accessible_text;
            return "";
        }

        if(t[i] == 0xE2 && t[i+1] == 0x96 && t[i+2] == 0xA0){                       // black square ? vertical line ? gp32
            i += 1;
            accessible_text += ". ";
            continue;
        }

        if(t[i] == ',' && t[i+1] == '\n'){                              // ,\n  gp32
            if(!sentence){
                sentence = accessible_text + ". \n";
                return "";
            }
            sentence += accessible_text;
            return "";
        }

        if(t[i] == ':' && t[i+1] == '\n'){                              // :\n  gp32
            if(!sentence){
                sentence = accessible_text + ". \n";
                return "";
            }
            sentence += accessible_text;
            return "";
        }

        if(t[i] == 'f' && t[i+1] == '\n'){                          // f\n      going down///////// gp32
            if(!sentence){
                sentence = accessible_text + " \n";
                return "";
            }
            sentence += accessible_text;
            return "";
        }
        if(t[i] == 't' && t[i+1] == '\n'){                          // t\n      going down///////// gp32
            if(!sentence){
                sentence = accessible_text + " \n";
                return "";
            }
            sentence += accessible_text;
            return "";
        }
        if(t[i] == 'd' && t[i+1] == '\n'){                          // d\n      going down///////// gp32
            if(!sentence){
                sentence = accessible_text + ", \n";
                return "";
            }
            sentence += accessible_text;
            return "";
        }
        if(t[i] == 'e' && t[i+1] == '\n'){                          // e\n      going down///////// gp32
            if(!sentence){
                sentence = accessible_text + ", \n";
                return "";
            }
            sentence += accessible_text;
            return "";
        }

        //TO_DO: add consistent double quotes check??????//

        switch(t[i]){
            case '\\':                                                                                  // new page and output for background_text
                if(sentence){
                    sentence += accessible_text;
                    return sentence;
                }
                if(background_text){
                    accessible_text += background_text;
                    background_text = "";
                    return accessible_text;
                }
                continue;
            case '/':
                if(t[i+1] == '\n') continue;
                accessible_text += " / ";
                break;
            case '`':                                                                                   // consistent single quotes
                accessible_text += '\'';
                break;
            case '@':                                                                               // looking where line stops outputing
                if(sentence){
                    return sentence;
                }
                continue;
            case '\n':
                continue;
            default:
                accessible_text += t[i];
                break;
        }
    }

    if (!sentence)
        return accessible_text;
    sentence += accessible_text;
    return "";
}

const pstring Accessibility::process_csel(const pstring& csel_text) const
{
    pstring accessible_text = "";
    for (int i = 0; i < csel_text.length(); ++i) {
        if (csel_text[i] == '^' && csel_text[i+1] == '*') {               // ^**%.
            i += 4;
            continue;
        }
        if (csel_text[i] >= 0x10 && csel_text[i] < 0x20)             // ~i~ etc.
            continue;
        accessible_text += csel_text[i];
    }
    return accessible_text;
}

void Accessibility::output(const pstring& text, const int num)
{
    // don't duplicate output
    if (text != last_output) {
        last_output = text;

        if (text) {
            // do this replacing for wide characters and the like
            //   that stuff up with os text to speech
            pstring modded_for_output = text;

            modded_for_output.findreplace("０", "0", 0);
            modded_for_output.findreplace("１", "1", 0);
            modded_for_output.findreplace("２", "2", 0);
            modded_for_output.findreplace("３", "3", 0);
            modded_for_output.findreplace("４", "4", 0);
            modded_for_output.findreplace("５", "5", 0);
            modded_for_output.findreplace("６", "6", 0);
            modded_for_output.findreplace("７", "7", 0);
            modded_for_output.findreplace("８", "8", 0);
            modded_for_output.findreplace("９", "9", 0);

            push_output(modded_for_output);
        }
    }
}

#ifdef MACOSX
void Accessibility::push_output(const pstring text)
{
    [ns stopSpeaking];
    [ns startSpeakingString:[NSString stringWithUTF8String:text]];
    printf("acc: [%s]\n", (const char *)text);
}
#else
void Accessibility::push_output(const pstring text)
{
    printf("acc: [%s]\n", (const char *)text);
}
#endif // !MACOSX

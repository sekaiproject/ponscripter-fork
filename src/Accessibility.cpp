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
    pugi::xpath_query query_default_translation(
        "/translations/translation[@name = string($default_translation_name) and @lang = "
        "string($default_translation_language)]",
        &vars);
    vars.set("default_translation_name", "default");
    vars.set("default_translation_language", "en");
    pugi::xpath_node_set default_translations = query_default_translation.evaluate_node_set(doc);
    if (!default_translations.empty()) {
        d_translation = default_translations[0].node();
    }

    // initializing
    last_str = "";
    last_str_buffer = "";

#ifdef MACOSX
    ns = [[NSSpeechSynthesizer alloc] init];
    loaded = true;
#elif WIN32
    // load sapi
    if (FAILED(::CoInitialize(NULL)))
        loaded = false;
    else
        // load voices
        HRESULT hr = CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void **)&pVoice;);
        if (SUCCEEDED(hr)) {
            loaded = true;
        }
        else {
            ::CoUninitialize();
            loaded = false;
        }
#else
    loaded = false;
#endif
}

Accessibility::~Accessibility()
{
    // pugixml destroys itself while it's not in the buffer?

#ifdef WIN32
    if (loaded) {
        pVoice->Release();
        pVoice = NULL;

        ::CoUninitialize();
    }
#endif
}

/**
 * Main handling interface.
 * @param pstring t text string, image name, filename
 * @param int sprite_id ID-number of sprite-image or, e.g., 255 if none
 * @param int button_id ID-number of button, or, e.g., 25 if none and there is no sprite_id
 * @param pstring cmd command to handle
 * @return sptring processed text or empty string
 */
const pstring Accessibility::get_accessible(
    pstring t, const int sprite_id, const int button_id, const pstring cmd)
{
    // printf("get_accessible called: %d %d [%s] [%s]\n", sprite_id, button_id, (const
    // char*)cmd, (const char*)t);
    if (cmd == "") {
        if (is_ok(t, sprite_id, button_id)) {
            pugi::xml_node menu = find_menu(t);
            if (!menu.empty()) {
                if (sprite_id != 255) {
                    pugi::xml_node sprite = find_sprite(menu, sprite_id);
                    if (!sprite.empty()) {
                        return get_button_text(sprite, button_id);
                    }
                } else {
                    return get_button_text(menu, button_id);
                }
            }
        }
        // } else if (cmd == "lsp") {
        //     pugi::xml_node subtitles = find_subtitles(t);
        //     if (!subtitles.empty()) {
        //         subtitles_on = true;
        //         return get_subtitle_text(subtitles);
        //     }
    } else if (cmd == "lsph") {
        return strip_ponscripter_tags(t);
    } else if (cmd == "csel") {
        return process_csel(t);
    } else if (cmd == "bg") {
        if (last_input == t) {
            return "";
        }
        last_input = t;
        pugi::xml_node background = find_bg(t);
        if (!background.empty()) {
            if (is_bg_inline(background)) {
                need_outputbg = true;
            } else {
                need_outputbg = false;
            }
            pstring temp = get_bg_text(background);
            if (!need_outputbg) {
                background_text = background_text + '\n' + temp;
                return "";
            }
            return temp;
        }
    } else if (cmd == "text") {
        if (last_input == t) {
            return "";
        }
        last_input = t;
        return process_text(t);
    } else if (cmd == "history") {
        if (t)
            return process_history(t);
    } else if (cmd == "saveload") {
        if (is_ok(t, sprite_id, button_id))
            return process_saveload(t);
	}

    return "";
}

/*
 * Deletes simple formatting tags
 * @param pstring t input string
 * @see Accessibility::get_accessible()
 * @see PonscripterLabel::lspCommand()
 * @return pstring
 */
const pstring Accessibility::strip_ponscripter_tags(const pstring &t)
{
    pstring temp = "";
    pstrIter it(t);

    if (it.get() == file_encoding->TextMarker())
        it.next();

    if (it.get() == '~') {
        it.next();

        // delete opening formatting tag
        while (it.get() >= 0 && it.get() != '~')
            it.next();

        if (it.get() == '~')
            it.next();

        // get current text
        while (it.get() >= 0 && it.get() != '~') {
            temp += it.getstr();
            it.next();
        }
    } else {
        while (it.get() >= 0) {
            temp += it.getstr();
            it.next();
        }
    }

    return temp;
}

/*
 * Processes and outputs the history log.
 * @param pstring t input string
 * @see Accessibility::get_accessible()
 * @see PonscripterLabel::getlogCommand()
 * @return pstring
 */
const pstring Accessibility::process_history(const pstring &t)
{
    pstrIter it(t);
    pstring current_text = "";

    while (it.get() >= 0) {
        // consistent single quotes
        if (it.get() == '`') {
            while (it.get() == '`') {
                it.next();
                current_text += '\'';
            }
        }

        // add " " for readability
        if (it.get() == '*') {
            it.next();
            current_text += " * ";

            continue;
        }

        if (it.get() >= 0x10 && it.get() <= 0x16) {
            it.forward(1);

            continue;
        }

        if (it.get() == 0x17 || it.get() == 0x18) {
            it.forward(3);

            continue;
        }

        if (it.get() == 0x19) {
            it.forward(3);

            continue;
        }

        if (it.get() >= 0x1A && it.get() <= 0x1E) {
            it.forward(3);
            while (it.get() >= 0x1A && it.get() <= 0x1E) {
                it.forward(3);
            }

            continue;
        }

        if (it.get() == 0x1F) {
            it.forward(2);

            continue;
        }

        current_text += it.getstr();

        it.next();
    }

    return current_text;
}

/*
 * Returns text for corresponding background-sprite.
 * @param pugi::xml_node background the background-node to be searched
 * @see Accessibility::get_accessible()
 * @return pstring text string or empty string
 */
const pstring Accessibility::get_bg_text(const pugi::xml_node &background)
{
    pugi::xpath_node_set background_texts = background.select_nodes("content");
    if (!background_texts.empty()) {
        return background_texts[0].node().text().get();
    }

    return "";
}

/*
 * Checks whether the background is displayed along with the text or separately.
 * @param pugi::xml_node background the background-node to be searched
 * @see Accessibility::get_accessible()
 * @return bool
 */
const bool Accessibility::is_bg_inline(const pugi::xml_node &background)
{
    pugi::xpath_node_set inline_bg = background.select_nodes("inline");
    if (!inline_bg.empty()) {
        return true;
    }

    return false;
}

/*
 * Finds background-node by given filename of the sprite.
 * @param pstring c filename of the sprite
 * @see Accessibility::get_accessible()
 * @see PonscripterLabel::bgCommand()
 * @return pugi:xml_node found background-node or empty-node
 */
const pugi::xml_node Accessibility::find_bg(const pstring &c)
{
    pugi::xpath_node_set backgrounds = doc.select_nodes("//backgrounds/background/src");
    if (!backgrounds.empty()) {
        pugi::xml_node parent;
        for (pugi::xpath_node_set::const_iterator it = backgrounds.begin(); it != backgrounds.end();
             ++it) {
            pugi::xpath_node node = *it;
            if (!strcmp(node.node().text().get(), c)) {
                parent = node.node().parent();
                return parent;
            }
        }
    }

    return pugi::xml_node();
}

/*
 * Finds sprite-node inside given menu-node by it's ID-number.
 * @param pugi::xml_node menu given menu-node
 * @param int sprite_id ID-number of the sprite, that we're searching.
 * @see Accessibility::get_accessible()
 * @see PonscripterLabel::btnwaitCommand()
 * @return pugi::xml_node sprite-node or empty-node
 */
const pugi::xml_node Accessibility::find_sprite(
    const pugi::xml_node &menu, const int sprite_id)
{
    pugi::xpath_variable_set vars;
    vars.add("sprite_id", pugi::xpath_type_number);
    pugi::xpath_query query_sprite("sprites/sprite[@id = $sprite_id]", &vars);
    vars.set("sprite_id", (double)sprite_id);
    pugi::xpath_node_set sprites = menu.select_nodes(query_sprite);
    if (!sprites.empty()) {
        return sprites[0].node();
    }
    return pugi::xml_node();
}

/**
 * Returns text for specified sprite button, if available.
 * Sets the number for bar.
 * Returns states (checked or empty string) for buttons: footnotes, subtitles, display mode.
 * @param pugi::xml_node menu menu node
 * @param int button_id ID-number for button
 * @see Accessibility::get_accessible()
 * @see Accessibility::find_menu()
 * @see PonscripterLabel::btnwaitCommand()
 * @see PonscripterLabel::btnCommand()
 * @return pstring text for corresponding button or empty string
 */
const pstring Accessibility::get_button_text(const pugi::xml_node &menu, const int button_id)
{
    pugi::xpath_variable_set vars;
    vars.add("button_id", pugi::xpath_type_number);
    pugi::xpath_query query_button("buttons/button[@id = $button_id]", &vars);
    vars.set("button_id", (double)button_id);
    pugi::xpath_node_set buttons = menu.select_nodes(query_button);
    if (!buttons.empty()) {
        if (!buttons[0].node().attribute("bar").empty()) {
            bar_no = buttons[0].node().attribute("bar").as_int();
        } else {
            bar_no = -1;
        }
        if (!buttons[0].node().attribute("type").empty()) {
            // footnotes
            if (footnotes_on_flag && buttons[0].node().attribute("type").as_int() == 1) {
                if (!buttons[0].node().attribute("button_state_text").empty()) {
                    return (pstring)buttons[0].node().text().get() + ' '
                        + (pstring)buttons[0].node().attribute("button_state_text").value();
                }
            }
            // subtitles
            else if (subtitles_on_flag && buttons[0].node().attribute("type").as_int() == 2) {
                if (!buttons[0].node().attribute("button_state_text").empty()) {
                    return (pstring)buttons[0].node().text().get() + ' '
                        + (pstring)buttons[0].node().attribute("button_state_text").value();
                }
            }
            // display mode
            else if (fullscreen_on_flag && buttons[0].node().attribute("type").as_int() == 3) {
                if (!buttons[0].node().attribute("button_state_text").empty()) {
                    return (pstring)buttons[0].node().text().get() + ' '
                        + (pstring)buttons[0].node().attribute("button_state_text").value();
                }
            }
        }

        return buttons[0].node().text().get();
    }
    return "";
}

/**
 * Finds menu-node by it's image name.
 * @param pstring c image name
 * @see Accessibility::get_accessible()
 * @see PonscripterLabel::btnwaitCommand()
 * @see PonscripterLabel::btnCommand()
 * @return pugi::xml_node xml-node of the menu
 */
const pugi::xml_node Accessibility::find_menu(const pstring &c)
{
    pugi::xpath_node_set menus = d_translation.select_nodes("menus/menu/src");
    pugi::xml_node parent;
    for (pugi::xpath_node_set::const_iterator it = menus.begin(); it != menus.end(); ++it) {
        pugi::xpath_node node = *it;
        if (!strcmp(node.node().text().get(), c)) {
            parent = node.node().parent();
            break;
        }
    }

    // XXX: lang prefix dependence?
    if (parent.empty()) {
        pugi::xpath_node_set translations = doc.select_nodes("/translations/translation[@id]");
        if (!translations.empty()) {
            bool need_brake = false;
            for (pugi::xpath_node_set::const_iterator itt = translations.begin();
                 itt != translations.end(); ++itt) {
                if (need_brake)
                    break;
                pugi::xpath_node node_t = *itt;
                pugi::xpath_node_set menus = node_t.node().select_nodes("menus/menu/src");
                for (pugi::xpath_node_set::const_iterator it = menus.begin(); it != menus.end();
                     ++it) {
                    pugi::xpath_node node = *it;
                    if (!strcmp(node.node().text().get(), c)) {
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

/*
 * sets flags for buttons inside config menu
 * @param int type 1 - footnotes, 2 - subtitles, 3 - display mode
 * @see PonscripterLabel::btnwaitCommand()
 * @return void
 */
void Accessibility::set_config_buttons_flags(const bool flag, const int type)
{
    switch (type) {
    case 1:
        footnotes_on_flag = flag;
        break;
    case 2:
        subtitles_on_flag = flag;
        break;
    case 3:
        fullscreen_on_flag = flag;
    }
}

/**
 * returns the number of bar from xml-file
 * @param none
 * @see PonscripterLabel::btnwaitCommand()
 * @return int
 */
int Accessibility::get_bar_no()
{
    return bar_no;
}

/**
 * processes the text for save / load menus
 * @param pstring t text string
 * @see PonscripterLabel::btnwaitCommand()
 * @see Accessibility::get_accessible()
 * @return pstring processed string
 * Template: 32^ Dec^ 00:00
 */
const pstring Accessibility::process_saveload(const pstring &t)
{
    pstrIter it(t);
    // if the next symbol isn't the day number, then the slot is empty
    if (it.get() == file_encoding->TextMarker())
        return "Empty.";	// default return for now

    pstring current_text = "";
    while(it.get() >= 0) {
    if (it.get() == file_encoding->TextMarker())
        it.next();

        current_text += it.getstr();

        it.next();
    }
	
    return current_text;
}

/**
 * Checks whether the sprite in the xml-file or not.
 * @param pstring t text string
 * @param int sprite_id ID-number for sprite
 * @param int button_id ID-number for button
 * @see Accessibility::get_accessible()
 * @return bool
 */
bool Accessibility::is_ok(const pstring &t, const int sprite_id, const int button_id)
{
    if (t) {
        if (sprite_id == 255) {
            return true;
        } else if (sprite_id < 151
            || sprite_id > 215) {  // min num in xml is 171, max - 215 // 151-170 for save/load
            return false;
        } else if (button_id < 1
            || button_id > 41) {  // there is no item with num = 0 or with num > 41
            return false;
        }
        return true;
    }

    return false;
}

/**
 * Sets the current displaying mode for ingame text.
 * @param bool draw_one_page_flag flag for displaying mode (text is a whole page or not)
 * @see PonscripterLabel::loadEnvData()
 * @see PonscripterLabel::keyPressEvent()
 * @return void
 */
void Accessibility::set_draw_one_page(const bool draw_one_page_flag)
{
    draw_one_page = draw_one_page_flag;
}

/**
 * given the input line, output a decently screen-readable string
 * @param pstring t text string
 * @see Accessibility::get_accessible()
 * @return pstring processed text string
 */
const pstring Accessibility::process_text_n(pstring t)
{
    char start_char = (unsigned char)t.data[0];

    // standard lines of text
    if (start_char != '!') {
        unsigned int proper_i = t.length();  // index of the last proper character (not whitespace)
        unsigned char proper_char = ' ';

        while (proper_i > 0) {
            proper_char = (unsigned char)t.data[proper_i];

            if ((proper_char == 0) || (proper_char == ' ') || (proper_char == '\n')
                || (proper_char == '\t')) {
                proper_i--;
            } else {
                break;
            }
        }

        if ((proper_char == '\\') || (proper_char == '@')) {
            last_str_buffer += t.midstr(0, proper_i);

            pstring output_text = last_str_buffer;
            last_str_buffer = "";
            return (const char *)output_text;
        } else if (proper_i == 0) {
            if (last_str_buffer.length() > 0) {
                pstring output_text = last_str_buffer;
                last_str_buffer = "";
                return (const char *)output_text;
            }
        } else {
            proper_i++;

            last_str_buffer += t.midstr(0, proper_i);
            last_str_buffer += ' ';
        }
    }

    // commands!
    else {
        printf("cmd: %s\n", (const char *)t);
    }

    return "";
}

/**
 * processes ingame text
 * @param pstring t text string
 * @see Accessibility::get_accessible()
 * @return pstring processed text string
 */
const pstring Accessibility::process_text(pstring t)
{
    if (!t || t == "\n")
        return "";
    pstrIter it(t);
    while (it.get() == ' ') {
        it.next();
    }
    if (it.get() == '\n')
        return "";
    pstring accessible_text = "";
    while (it.get() >= 0) {
        // processing !sNUM, !sd, !wNUM, !dNUM
        if (it.get() == '!') {
            it.forward(1);
            // deleting "character display speed" commands
            // and processing headings
            if (it.get() == 's') {
                it.forward(1);
                // delete !sNUM
                if (it.get() >= '0' && it.get() <= '9') {
                    while (it.get() >= '0' && it.get() <= '9') {
                        it.forward(1);
                    }
                    // save heading text for later use
                } else if (it.get() == 'd') {
                    it.forward(1);
                    if (accessible_text) {
                        if (heading_text) {
                            heading_text += accessible_text;
                        } else {
                            heading_text = accessible_text;
                        }

                        return "";
                    }
                }
                // delete "wait" commands
            } else if (it.get() == 'w' || it.get() == 'd') {
                it.forward(1);
                while (it.get() >= '0' && it.get() <= '9') {
                    it.forward(1);
                }
            }

            continue;
        }

        // processing ponscripter "formatting tag value":
        // d, r, i, t, b, f, s
        if (it.get() >= 0x10 && it.get() <= 0x16) {
            it.forward(1);

            continue;
        }

        // processing ponscripter "formatting tag value":
        // =NUM, +NUM, -NUM
        if (it.get() == 0x17 || it.get() == 0x18) {
            it.forward(3);

            continue;
        }

        // footnote sign check
        // and processing ponscripter "formatting tag value":
        // %NUM
        if (it.get() == 0x19) {
            it.forward(3);
            while (it.get() == ' ') {
                it.forward(1);
            }
            if (it.get() == '*') {
                accessible_text += it.getstr();
                it.forward(1);
                while (it.get() == ' ') {
                    it.forward(1);
                }
                if (it.get() == 0x17) {
                    it.forward(3);
                    footnote = true;
                }
            }

            continue;
        }

        // processing ponscripter "formatting tag value":
        // x+NUM, x-NUM, xNUM, y+NUM, y-NUM, yNUM, cNUM
        if (it.get() >= 0x1A && it.get() <= 0x1E) {
            it.forward(3);
            while (it.get() >= 0x1A && it.get() <= 0x1E) {
                it.forward(3);
            }

            continue;
        }

        // check if current text is a footnote.
        // processing ponscripter "formatting tag value":
        // n, u
        if (it.get() == 0x1F) {
            it.forward(1);
            if (it.get() == 0x10)
                it.forward(1);
            else if (it.get() == 0x11) {
                if (footnote) {
                    footnote = false;
                    footnote_text = accessible_text;

                    return "";
                }

                it.forward(1);
            }

            continue;
        }

        // consistent single quotes for readability
        if (it.get() == '`') {
            while (it.get() == '`') {
                it.forward(1);
                accessible_text += '\'';
            }

            continue;
        }

        // delete ZWNJ builtin shortcut
        if (it.get() == '|') {
            it.forward(2);

            continue;
        }

        // delete black square for readability
        if (it.getstr() == "\u25A0") {
            it.forward(3);

            continue;
        }

        // delete "hexadecimal color codes"
        if (it.get() == '#') {
            it.forward(1);
            int i = 1;
            pstring temp = "#";
            while ((it.get() >= '0' && it.get() <= '9') || (it.get() >= 'a' && it.get() <= 'f')
                || (it.get() >= 'A' && it.get() <= 'F')) {
                temp += it.getstr();
                it.forward(1);
                ++i;
            }

            if (i != 7)
                accessible_text += temp;

            continue;
        }

        // delete "ignore new line" command
        // and process forward slash for readability
        if (it.get() == '/') {
            it.forward(1);
            while (it.get() == ' ') {
                it.forward(1);
            }
            if (it.get() != '\n') {
                accessible_text += " / ";
            } else {
                if (accessible_text) {
                    // add " " for readability
                    if (sentence) {
                        sentence = sentence + ' ' + accessible_text;
                    } else {
                        sentence = accessible_text + ' ';
                    }
                }

                return "";
            }

            continue;
        }

        // process "click wait state"
        if (it.get() == '@') {
            // If we're drawing the entire page,
            // then there is no need to process "click wait state"
            if (!draw_one_page) {
                if (heading_text) {
                    // add ".\n" for readability
                    if (sentence) {
                        accessible_text = heading_text + ".\n" + sentence + accessible_text;
                        sentence = "";
                    } else {
                        accessible_text = heading_text + ".\n" + accessible_text;
                    }

                    heading_text = "";
                } else {
                    // add " " for readability
                    if (sentence) {
                        accessible_text = sentence + ' ' + accessible_text;
                        sentence = "";
                    }
                }

                return accessible_text;
            } else {
                // for "click wait state" different from ^@^
                // (e.g. sentence.@)
                if (accessible_text) {
                    if (sentence) {
                        // add " " for readability
                        sentence = sentence + ' ' + accessible_text;
                    } else {
                        sentence = accessible_text + ' ';
                    }
                }

                return "";
            }
        }

        // process "end-of-page wait state"
        if (it.get() == '\\') {
            if (heading_text) {
                // add ".\n" for readability
                if (sentence) {
                    accessible_text = heading_text + ".\n" + sentence + ' ' + accessible_text;
                    sentence = "";
                } else {
                    accessible_text = heading_text + ".\n" + accessible_text;
                }

                heading_text = "";
            } else if (footnote_text) {
                // add ".\n" for readability
                if (sentence) {
                    accessible_text = sentence + accessible_text + ".\n" + footnote_text;
                    sentence = "";
                } else {
                    accessible_text = accessible_text + ".\n" + footnote_text;
                }

                footnote_text = "";
            } else {
                // add " " for readability
                if (sentence) {
                    accessible_text = sentence + ' ' + accessible_text;
                    sentence = "";
                }
            }

            // Output for background text
            if (background_text) {
                // add ".\n" for readability
                accessible_text = background_text + ".\n" + accessible_text;
                background_text = "";
            }

            return accessible_text;
        }

        accessible_text += it.getstr();

        it.next();
    }

    if (accessible_text) {
        if (sentence) {
            // add " " for readability
            sentence = sentence + ' ' + accessible_text;
        } else {
            sentence = accessible_text + ' ';
        }
    }

    return "";
}

/*
 * Processes lists text.
 * @param pstring csel_text text string
 * @see Accessibility::get_accessible()
 * @see PonscripterLabel::cselbtnCommand()
 * @return pstring processed text string
 */
const pstring Accessibility::process_csel(const pstring &csel_text)
{
    pstrIter it(csel_text);
    if (it.get() == file_encoding->TextMarker())
        it.next();

    pstring accessible_text = "";
    while (it.get() >= 0) {
        if (it.get() == '*') {
            while (it.get() == '*')
                it.next();

            if (it.get() == '%') {
                it.next();
                if (it.get() == '.') {
                    it.next();
                }
            }

            continue;
            // delete "black circle"
        } else if (it.getstr() == "\u25CF") {
            it.next();

            continue;
        }

        if (it.get() >= 0x10 && it.get() <= 0x16) {
            it.forward(1);

            continue;
        }

        if (it.get() == 0x17 || it.get() == 0x18) {
            it.forward(3);

            continue;
        }

        if (it.get() == 0x19) {
            it.forward(3);

            continue;
        }

        if (it.get() >= 0x1A && it.get() <= 0x1E) {
            it.forward(3);
            while (it.get() >= 0x1A && it.get() <= 0x1E) {
                it.forward(3);
            }

            continue;
        }

        if (it.get() == 0x1F) {
            it.forward(2);

            continue;
        }

        accessible_text += it.getstr();

        it.next();
    }

    return accessible_text;
}

/**
 * Output text to our screen-reader
 * @param pstring what text string to output
 * @param int num some integer to output
 */
void Accessibility::output(const pstring &text, const int num)
{
    // don't duplicate output
    if (text != last_output) {
        last_output = text;

        if (text) {
            // do this replacing for wide characters and the like
            //   that stuff up with text to speech
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

#ifdef MACOSX
            // these cause problems for VoiceOver
            // so only replace them for OSX (unless they also cause problems for other OSes)
            modded_for_output.findreplace("''...", "...");
            modded_for_output.findreplace("(...", "...");
#endif

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
#elif WIN32
void Accessibility::push_output(const pstring text)
{
    if (loaded) {
        wchar_t *buffer;
        int buffer_size = MultiByteToWideChar(CP_UTF8, 0, (const char *)last_output, -1, NULL, 0);
        buffer = new wchar_t[buffer_size];
        MultiByteToWideChar(CP_UTF8, 0, (const char *)last_output, -1, buffer, buffer_size);

        pVoice->Speak(buffer, 0, NULL);
		
        delete buffer;
    }
    printf("acc: [%s]\n", (const char *)text);
}
#else
void Accessibility::push_output(const pstring text)
{
    printf("acc: [%s]\n", (const char *)text);
}
#endif // !MACOSX

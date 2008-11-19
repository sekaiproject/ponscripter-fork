/* -*- C++ -*-
 *
 *  ScriptHandler.cpp - Script manipulation class
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

#include "ScriptHandler.h"
#include "Fontinfo.h"
#include <ctype.h>

#ifdef MACOSX
#include <Carbon/Carbon.h>
#endif

#define TMP_SCRIPT_BUF_LEN 4096
#define STRING_BUFFER_LENGTH 2048

#define SKIP_SPACE(p) while (*(p) == ' ' || *(p) == '\t') (p)++

BaseReader* ScriptHandler::cBR = NULL;

FILE* cout = stdout;
FILE* cerr = stderr;

ScriptHandler::ScriptHandler()
    : variable_data(VARIABLE_RANGE + 1),
      game_identifier()
{
    script_buffer = NULL;
    kidoku_buffer = NULL;
    label_log.filename = "NScrllog.dat";
    file_log.filename  = "NScrflog.dat";
    clickstr_list.clear();

    arrays.clear();

    screen_size = SCREEN_SIZE_640x480;
    global_variable_border = 200;

    // Prefer Ponscripter files over NScripter files, and prefer
    // unencoded files over encoded files.
    script_filenames.push_back(ScriptFilename("0.utf",        0, UTF8));
    script_filenames.push_back(ScriptFilename("0.txt",        0, CP932));
    script_filenames.push_back(ScriptFilename("00.utf",       0, UTF8));
    script_filenames.push_back(ScriptFilename("00.txt",       0, CP932));
    script_filenames.push_back(ScriptFilename("pscr_sec.dat", 2, UTF8));
    script_filenames.push_back(ScriptFilename("nscr_sec.dat", 2, CP932));
    script_filenames.push_back(ScriptFilename("pscript.___",  3, UTF8));
    script_filenames.push_back(ScriptFilename("nscript.___",  3, CP932));
    script_filenames.push_back(ScriptFilename("pscript.dat",  1, UTF8));
    script_filenames.push_back(ScriptFilename("nscript.dat",  1, CP932));
}


ScriptHandler::~ScriptHandler()
{
    reset();
    if (script_buffer) delete[] script_buffer;
    if (kidoku_buffer) delete[] kidoku_buffer;
}


void ScriptHandler::reset()
{
    for (int i = 0; i < VARIABLE_RANGE; i++)
        variable_data[i].reset(true);

    arrays.clear();

    // reset log info
    label_log.clear();
    file_log.clear();

    // reset aliases
    num_aliases.clear();
    str_aliases.clear();

    // reset misc. variables
    end_status = END_NONE;
    kidokuskip_flag = false;
    text_flag = true;
    linepage_flag  = false;
    textgosub_flag = false;
    skip_enabled = false;
    clickstr_list.clear();

    Fontinfo::default_encoding = 0;
}


void ScriptHandler::setKeyTable(const unsigned char* key_table)
{
    int i;
    if (key_table) {
        key_table_flag = true;
        for (i = 0; i < 256; i++) this->key_table[i] = key_table[i];
    }
    else {
        key_table_flag = false;
        for (i = 0; i < 256; i++) this->key_table[i] = i;
    }
}


// basic parser function
const char* ScriptHandler::readToken(bool no_kidoku)
{
    current_script = next_script;
    const char* buf = current_script;
    end_status = END_NONE;
    current_variable.type = VAR_NONE;

    text_flag = false;

    SKIP_SPACE(buf);
    if (!no_kidoku) markAsKidoku(buf);
    
readTokenTop:
    string_buffer.trunc(0);
    char ch = *buf;
    if (ch == ';') { // comment
        addStrBuf(ch);
        do {
            ch = *++buf;
            addStrBuf(ch);
        } while (ch != 0x0a && ch != '\0');
    }
    else if (ch & 0x80
             || (ch >= '0' && ch <= '9')
             || ch == '@' || ch == '\\' || ch == '/'
             || ch == '%' || ch == '?' || ch == '$'
             || ch == '[' || ch == '('
             || ch == '!' || ch == '#' || ch == ',' || ch == '"') {
        // text
//	if (ch != '!')
//	    errorWarning("unmarked text found - this may be deprecated soon");
        bool loop_flag = true;
        bool ignore_click_flag = false;
        do {
            char bytes = encoding->NextCharSize(buf);
            if (bytes > 1) {
                if (textgosub_flag && !ignore_click_flag && checkClickstr(buf))
		    loop_flag = false;
		string_buffer.add(buf, bytes);
		buf += bytes;
                SKIP_SPACE(buf);
                ch = *buf;
            }
            else {
                if (ch == '%' || ch == '?') {
                    addIntVariable(&buf);
                }
                else if (ch == '$') {
                    addStrVariable(&buf);
                }
                else {
                    if (textgosub_flag && !ignore_click_flag &&
			checkClickstr(buf))
                        loop_flag = false;

                    string_buffer += ch;
                    buf++;
                    ignore_click_flag = false;
                    if (ch == '_') ignore_click_flag = true;
                }

		// CHECKME: why do we ignore text markers here?
                if (ch >= '0' && ch <= '9' &&
		    (*buf == ' ' || *buf == '\t' ||
		     *buf == encoding->TextMarker()) &&
		    string_buffer.length() % 2)
		{
		    string_buffer += ' ';
		}

                ch = *buf;
                if (ch == 0x0a || ch == '\0' || !loop_flag ||
		    ch == encoding->TextMarker())
		{
		    break;
		}

                SKIP_SPACE(buf);
                ch = *buf;
            }
        }
        while (ch != 0x0a && ch != '\0' && loop_flag && ch != encoding->TextMarker()) /*nop*/;
        if (loop_flag && ch == 0x0a && !(textgosub_flag && linepage_flag)) {
            string_buffer += ch;
            if (!no_kidoku) markAsKidoku(buf++);
        }

        text_flag = true;
    }
    else if (ch == encoding->TextMarker()) {
//if (buf[1] == '~') {
//    cerr << "Text: " + string(buf, 20) << eol;
//}
        ch = *++buf;
        while (ch != encoding->TextMarker() && ch != 0x0a && ch != '\0') {
            if ((ch == '\\' || ch == '@') && (buf[1] == 0x0a || buf[1] == 0)) {
                string_buffer += *buf++;
                ch = *buf;
                break;
            }
   
            if (encoding->UseTags() && ch == '~' && (ch = *++buf) != '~') {
                while (ch != '~') {
                    int l;
		    string_buffer += encoding->TranslateTag(buf, l);
                    buf += l;
                    ch = *buf;
                }
                ch = *++buf;
                continue;
            }

	    int bytes;
	    // NOTE: we don't substitute ligatures at this stage.
            string_buffer += encoding->Encode(encoding->DecodeChar(buf, bytes));
            buf += bytes;
            ch = *buf;
        }
        if (ch == encoding->TextMarker()) ++buf;

        if (ch == 0x0a && !(textgosub_flag && linepage_flag)) {
            string_buffer += ch;
            if (!no_kidoku) markAsKidoku(buf++);
        }

        text_flag   = true;
        end_status |= END_1BYTE_CHAR;
    }
    else if ((ch >= 'a' && ch <= 'z')
             || (ch >= 'A' && ch <= 'Z')
             || ch == '_') { // command
        do {
            if (ch >= 'A' && ch <= 'Z') ch += 'a' - 'A';

            string_buffer += ch;
            ch = *++buf;
        }
        while ((ch >= 'a' && ch <= 'z')
               || (ch >= 'A' && ch <= 'Z')
               || (ch >= '0' && ch <= '9')
               || ch == '_');
    }
    else if (ch == '*') { // label
        return readLabel();
    }
    else if (ch == '~' || ch == 0x0a || ch == ':') {
        string_buffer += ch;
        if (!no_kidoku) markAsKidoku(buf++);
    }
    else if (ch != '\0') {
        fprintf(stderr, "readToken: skip unknown heading character %c (%x)\n",
		ch, ch);
        buf++;
        goto readTokenTop;
    }

    next_script = checkComma(buf);

    return string_buffer;
}


const char* ScriptHandler::readLabel()
{
    end_status = END_NONE;
    current_variable.type = VAR_NONE;

    current_script = next_script;
    SKIP_SPACE(current_script);
    const char* buf = current_script;

    string_buffer.trunc(0);
    char ch = *buf;
    if (ch == '$') {
        addStrVariable(&buf);
    }
    else if ((ch >= 'a' && ch <= 'z')
             || (ch >= 'A' && ch <= 'Z')
             || ch == '_' || ch == '*') {
        if (ch >= 'A' && ch <= 'Z') ch += 'a' - 'A';

        string_buffer += ch;
        buf++;
        if (ch == '*') SKIP_SPACE(buf);

        ch = *buf;
        while ((ch >= 'a' && ch <= 'z')
               || (ch >= 'A' && ch <= 'Z')
               || (ch >= '0' && ch <= '9')
               || ch == '_') {
            if (ch >= 'A' && ch <= 'Z') ch += 'a' - 'A';

            string_buffer += ch;
            ch = *++buf;
        }
    }

    next_script = checkComma(buf);

    return string_buffer;
}


const char* ScriptHandler::readStr()
{
    end_status = END_NONE;
    current_variable.type = VAR_NONE;

    current_script = next_script;
    SKIP_SPACE(current_script);
    const char* buf = current_script;

    string_buffer.trunc(0);

    while (1) {
        string_buffer += parseStr(&buf);
        buf = checkComma(buf);
        if (buf[0] != '+') break;

        buf++;
    }
    next_script = buf;

    return string_buffer;
}


int ScriptHandler::readInt()
{
    string_buffer.trunc(0);

    end_status = END_NONE;
    current_variable.type = VAR_NONE;

    current_script = next_script;
    SKIP_SPACE(current_script);
    const char* buf = current_script;

    int ret = parseIntExpression(&buf);

    next_script = checkComma(buf);

    return ret;
}


void ScriptHandler::skipToken()
{
    SKIP_SPACE(current_script);
    const char* buf = current_script;

    bool quat_flag = false;
    bool text_flag = false;
    while (1) {
        if (*buf == 0x0a ||
            (!quat_flag && !text_flag && (*buf == ':' || *buf == ';')))
	  break;

        if (*buf == '"') quat_flag = !quat_flag;

        const char bytes = encoding->NextCharSize(buf);

	// CHECKME: what exactly does this do?
        if (bytes > 1 && !quat_flag) text_flag = true;

        buf += bytes;
    }

    if (text_flag && *buf == 0x0a) ++buf;
    
    next_script = buf;
}


// script address direct manipulation function
void ScriptHandler::setCurrent(const char* pos)
{
    current_script = next_script = pos;
}


void ScriptHandler::pushCurrent(const char* pos)
{
    pushed_current_script = current_script;
    pushed_next_script = next_script;

    current_script = pos;
    next_script = pos;
}


void ScriptHandler::popCurrent()
{
    current_script = pushed_current_script;
    next_script = pushed_next_script;
}


int ScriptHandler::getOffset(const char* pos)
{
    return pos - script_buffer;
}


const char* ScriptHandler::getAddress(int offset)
{
    return script_buffer + offset;
}


int ScriptHandler::getLineByAddress(const char* address, bool absolute)
{
    LabelInfo label = getLabelByAddress(address);

    const char* addr = label.label_header;
    int line = absolute ? label.start_line + 1 : 0;
    while (address > addr) {
        if (*addr == 0x0a) line++;

        addr++;
    }
    return line;
}


const char* ScriptHandler::getAddressByLine(int line)
{
    LabelInfo label = getLabelByLine(line);

    int l = line - label.start_line;
    const char* addr = label.label_header;
    while (l > 0) {
        while (*addr != 0x0a) addr++;
        addr++;
        l--;
    }
    return addr;
}


ScriptHandler::LabelInfo ScriptHandler::getLabelByAddress(const char* address)
{
    LabelInfo::vec::size_type i;
    for (i = 0; i < label_info.size() - 1; i++) {
        if (label_info[i + 1].start_address > address)
            return label_info[i];
    }
    return label_info[i];
}


ScriptHandler::LabelInfo ScriptHandler::getLabelByLine(int line)
{
    LabelInfo::vec::size_type i;
    for (i = 0; i < label_info.size() - 1; i++) {
        if (label_info[i + 1].start_line > line)
            return label_info[i];
    }

    return label_info[i];
}


bool ScriptHandler::isText()
{
    return text_flag;
}


bool ScriptHandler::compareString(const char* buf)
{
    SKIP_SPACE(next_script);
    unsigned int i, num = strlen(buf);
    for (i = 0; i < num; i++) {
        unsigned char ch = next_script[i];
        if ('A' <= ch && 'Z' >= ch) ch += 'a' - 'A';

        if (ch != buf[i]) break;
    }
    return i == num;
}


void ScriptHandler::skipLine(int no)
{
    for (int i = 0; i < no; i++) {
        while (*current_script != 0x0a) current_script++;
        current_script++;
    }

    next_script = current_script;
}


void ScriptHandler::setLinepage(bool val)
{
    linepage_flag = val;
}


// function for kidoku history
bool ScriptHandler::isKidoku()
{
    return skip_enabled;
}


void ScriptHandler::markAsKidoku(const char* address)
{
    if (!kidokuskip_flag) return;

    int offset = current_script - script_buffer;
    if (address) offset = address - script_buffer;

    //printf("mark (%c)%x:%x = %d\n", *current_script, offset /8, offset%8, kidoku_buffer[ offset/8 ] & ((char)1 << (offset % 8)));
    if (kidoku_buffer[offset / 8] & ((char) 1 << (offset % 8)))
        skip_enabled = true;
    else
        skip_enabled = false;

    kidoku_buffer[offset / 8] |= ((char) 1 << (offset % 8));
}


void ScriptHandler::setKidokuskip(bool kidokuskip_flag)
{
    this->kidokuskip_flag = kidokuskip_flag;
}


void ScriptHandler::saveKidokuData()
{
    FILE* fp;
    pstring fnam = save_path + "kidoku.dat";
    if ((fp = fopen(fnam, "wb")) == NULL) {
        fprintf(stderr, "can't write kidoku.dat\n");
        return;
    }

    fwrite(kidoku_buffer, 1, script_buffer_length / 8, fp);
    fclose(fp);
}


void ScriptHandler::loadKidokuData()
{
    FILE* fp;
    pstring fnam = save_path + "kidoku.dat";
    setKidokuskip(true);
    kidoku_buffer = new char[script_buffer_length / 8 + 1];
    memset(kidoku_buffer, 0, script_buffer_length / 8 + 1);

    if ((fp = fopen(fnam, "rb")) != NULL) {
        fread(kidoku_buffer, 1, script_buffer_length / 8, fp);
        fclose(fp);
    }
}


void ScriptHandler::addIntVariable(const char** buf)
{
    string_buffer += stringFromInteger(parseInt(buf), -1);
}


void ScriptHandler::addStrVariable(const char** buf)
{
    (*buf)++;
    string_buffer += variable_data[parseInt(buf)].str;
}


void ScriptHandler::enableTextgosub(bool val)
{
    textgosub_flag = val;
}


void ScriptHandler::setClickstr(pstring values)
{
    clickstr_list.clear();
    pstrIter it(values);
    if (it.get() == encoding->TextMarker()) it.next();
    while (it.get() >= 0) {
	clickstr_list.insert(it.get());
	it.next();
    }
}


int ScriptHandler::checkClickstr(const char* buf, bool recursive_flag)
{
    if (buf[0] == '@' || buf[0] == '\\') return 1;
    int bytes;
    wchar c = encoding->DecodeChar(buf, bytes);
    if (clickstr_list.find(c) != clickstr_list.end()) {
	if (!recursive_flag && checkClickstr(buf + bytes, true)) return 0;
	return bytes;
    }
    return 0;
}


void ScriptHandler::setNumVariable(int no, int val)
{
    if (no < 0 || no >= VARIABLE_RANGE)
        no = VARIABLE_RANGE;

    VariableData &vd = variable_data[no];
    if (vd.num_limit_flag) {
        if (val < vd.num_limit_lower) val = vd.num;
        else if (val > vd.num_limit_upper) val = vd.num;
    }

    vd.num = val;
}


pstring ScriptHandler::stringFromInteger(int no, int num_column,
					 bool is_zero_inserted)
{
    pstring s;
    s.format(is_zero_inserted ? "%0*d" : "%*d", num_column, no);
    if (num_column > 0) s.trunc(num_column);
    if (s == "-" || !s) s = "0";
    return s;
}


int ScriptHandler::readScriptSub(FILE* fp, char** buf, int encrypt_mode)
{
    char magic[5] = { 0x79, 0x57, 0x0d, 0x80, 0x04 };
    int  magic_counter = 0;
    bool newline_flag  = true;
    bool cr_flag = false;

    if (encrypt_mode == 3 && !key_table_flag)
        errorAndExit("readScriptSub: the EXE file must be specified with --key-exe option.");

    size_t len = 0, count = 0;
    while (1) {
        if (len == count) {
            len = fread(tmp_script_buf, 1, TMP_SCRIPT_BUF_LEN, fp);
            if (len == 0) {
                if (cr_flag) *(*buf)++ = 0x0a;

                break;
            }

            count = 0;
        }

        char ch = tmp_script_buf[count++];
        if (encrypt_mode == 1) ch ^= 0x84;
        else if (encrypt_mode == 2) {
            ch = (ch ^ magic[magic_counter++]) & 0xff;
            if (magic_counter == 5) magic_counter = 0;
        }
        else if (encrypt_mode == 3) {
            ch = key_table[(unsigned char) ch] ^ 0x84;
        }

        if (cr_flag && ch != 0x0a) {
            *(*buf)++    = 0x0a;
            newline_flag = true;
            cr_flag = false;
        }

        if (ch == 0x0d) {
            cr_flag = true;
            continue;
        }

        if (ch == 0x0a) {
            *(*buf)++    = 0x0a;
            newline_flag = true;
            cr_flag = false;
        }
        else {
            *(*buf)++ = ch;
            if (ch != ' ' && ch != '\t')
                newline_flag = false;
        }
    }

    *(*buf)++ = 0x0a;
    return 0;
}


int ScriptHandler::readScript(const pstring& path)
{
    archive_path = path;

    FILE* fp = NULL;
    int encrypt_mode = 0;
    encoding_t enc = UTF8;

    for (ScriptFilename::iterator ft = script_filenames.begin();
	 ft != script_filenames.end(); ++ft) {
	if ((fp = fopen(path + ft->filename, "rb")) != NULL) {
	    encrypt_mode = ft->encryption;
	    enc = ft->encoding;
	    if (enc == UTF8) {
		encoding = new UTF8Encoding;
		is_ponscripter = true;
	    }
	    else {
		encoding = new CP932Encoding;
		is_ponscripter = false;
	    }
	    break;
	}
    }

    if (fp == NULL) {
#ifdef MACOSX
        // Note: \p Pascal strings require compilation with -fpascal-strings
        StandardAlert(kAlertStopAlert, "\pMissing game data",
		      "\pNo game data found. This application must be run "
		      "from a directory containing NScripter, ONScripter, "
		      "or Ponscripter game data.", NULL, NULL);
#else
	fprintf(stderr, "Can't find a Ponscripter game script.\n");
#endif
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    int estimated_buffer_length = ftell(fp) + 1;

    if (encrypt_mode == 0) {
        fclose(fp);
        for (int i = 1; i < 100; i++) {
	    pstring filename;
	    filename.format("%s%d.%s", (const char*) archive_path, i,
			    enc == UTF8 ? "utf" : "txt");
            if ((fp = fopen(filename, "rb")) == NULL) {
		filename.format("%s%02d.%s", (const char*) archive_path, i,
				enc == UTF8 ? "utf" : "txt");
                fp = fopen(filename, "rb");
            }

            if (fp) {
                fseek(fp, 0, SEEK_END);
                estimated_buffer_length += ftell(fp) + 1;
                fclose(fp);
            }
        }
    }

    if (script_buffer) delete[] script_buffer;

    char* p_script_buffer = new char[estimated_buffer_length];

    current_script = script_buffer = p_script_buffer;
    tmp_script_buf = new char[TMP_SCRIPT_BUF_LEN];
    if (encrypt_mode > 0) {
        fseek(fp, 0, SEEK_SET);
        readScriptSub(fp, &p_script_buffer, encrypt_mode);
        fclose(fp);
    }
    else {
        for (int i = 0; i < 100; i++) {
	    pstring filename;
	    filename.format("%s%d.%s", (const char*) archive_path, i,
			    enc == UTF8 ? "utf" : "txt");
            if ((fp = fopen(filename, "rb")) == NULL) {
		filename.format("%s%02d.%s", (const char*) archive_path, i,
				enc == UTF8 ? "utf" : "txt");
                fp = fopen(filename, "rb");
            }

            if (fp) {
                readScriptSub(fp, &p_script_buffer, 0);
                fclose(fp);
            }
        }
    }

    delete[] tmp_script_buf;

    script_buffer_length = p_script_buffer - script_buffer;

    /* ---------------------------------------- */
    /* screen size and value check */
    const char* buf = script_buffer + 1;
    while (script_buffer[0] == ';') {
        if (!strncmp(buf, "mode", 4)) {
            buf += 4;
            if (!strncmp(buf, "800", 3))
                screen_size = SCREEN_SIZE_800x600;
            else if (!strncmp(buf, "400", 3))
                screen_size = SCREEN_SIZE_400x300;
            else if (!strncmp(buf, "320", 3))
                screen_size = SCREEN_SIZE_320x240;
            else
                screen_size = SCREEN_SIZE_640x480;

            buf += 3;
        }
        else if (!strncmp(buf, "value", 5)) {
            buf += 5;
            global_variable_border = 0;
            while (*buf >= '0' && *buf <= '9')
                global_variable_border = global_variable_border * 10
		                       + *buf++ - '0';
        }
        else if (!strncmp(buf, "-*-", 3)) {
            buf++;
            while (*buf != '\n' &&
                   !(*buf == '-' && buf[1] == '*' && buf[2] == '-'))
                buf++;
            if (*buf != '\n') buf += 3;
        }
        else {
            break;
        }

        if (*buf != ',') {
            while (*buf++ != '\n') ;
            break;
        }

        buf++;
        while (*buf == ' ' || *buf == '\t') ++buf;
    }

    // game ID check
    if (*buf++ == ';') {
        while (*buf == ' ' || *buf == '\t') ++buf;
        if (!strncmp(buf, "gameid ", 7)) {
            buf += 7;
            int i = 0;
            while (buf[i++] >= ' ') ;
	    game_identifier = pstring(buf, i - 1);
            buf += i;
        }
    }

    return labelScript();
}


int ScriptHandler::labelScript()
{
    int current_line = 0;
    const char* buf = script_buffer;
    label_info.clear();

    while (buf < script_buffer + script_buffer_length) {
        SKIP_SPACE(buf);
        if (*buf == '*') {
            setCurrent(buf);
            readLabel();
	    LabelInfo new_label;
	    new_label.name
		= string_buffer.midstr(1, string_buffer.length() - 1);
            new_label.label_header = buf;
            new_label.num_of_lines = 1;
            new_label.start_line = current_line;
	    buf = getNext();
            if (*buf == 0x0a) {
                buf++;
                SKIP_SPACE(buf);
                current_line++;
            }
	    new_label.start_address = buf;
	    label_info.push_back(new_label);
        }
        else {
	    if (label_info.size())
		label_info.back().num_of_lines++;

            while (*buf != 0x0a) buf++;
            buf++;
            current_line++;
        }
    }

    // Index label names.
    for (LabelInfo::iterator i = label_info.begin(); i != label_info.end(); ++i)
	label_names[i->name] = i;
    
    return 0;
}


ScriptHandler::LabelInfo ScriptHandler::lookupLabel(const pstring& label)
{
    LabelInfo::iterator i = findLabel(label);
    label_log.add(label);
    return *i;
}


ScriptHandler::LabelInfo ScriptHandler::lookupLabelNext(const pstring& label)
{
    LabelInfo::iterator i = findLabel(label);
    if (++i != label_info.end()) {
	label_log.add(label);
        return *i;
    }
    return LabelInfo();
}


void ScriptHandler::errorAndExit(pstring s)
{
    fprintf(stderr, "Script error (line %d): %s\n(String buffer: [%s])\n",
            getLineByAddress(getCurrent(), true),
            (const char*) s, (const char*) string_buffer);
    exit(-1);
}

void ScriptHandler::errorWarning(pstring s)
{
    fprintf(stderr, "Warning (line %d): %s\n",
            getLineByAddress(getCurrent(), true),
            (const char*) s);
}


// ----------------------------------------
// Private methods

ScriptHandler::LabelInfo::iterator ScriptHandler::findLabel(pstring label)
{
    if (label[0] == '*') label.remove(0, 1);
    label.tolower();

    LabelInfo::dic::iterator e = label_names.find(label);
    if (e != label_names.end())
	return e->second;

    errorAndExit("Label \"" + label + "\" is not found.");
    return label_info.end(); // dummy
}


const char* ScriptHandler::checkComma(const char* buf)
{
    SKIP_SPACE(buf);
    if (*buf == ',') {
        end_status |= END_COMMA;
        buf++;
        SKIP_SPACE(buf);
    }

    return buf;
}


pstring ScriptHandler::parseStr(const char** buf)
{
    SKIP_SPACE(*buf);

    if (**buf == '(') {
	// (foo) bar baz : apparently returns bar if foo has been
	// viewed, baz otherwise.
	
        (*buf)++;
        pstring s = parseStr(buf);
        SKIP_SPACE(*buf);
        if ((*buf)[0] != ')') errorAndExit("parseStr: ) is not found.");

        (*buf)++;

        if (file_log.find(s)) {
            s = parseStr(buf);
            parseStr(buf);
        }
        else {
            parseStr(buf);
            s = parseStr(buf);
        }

        current_variable.type |= VAR_CONST;
	return s;
    }
    else if (**buf == '$') {
        (*buf)++;
        int no = parseInt(buf);
        current_variable.type = VAR_STR;
        current_variable.var_no = no;
        if (no < 0 || no >= VARIABLE_RANGE)
            current_variable.var_no = VARIABLE_RANGE;
	return variable_data[no].str;
    }
    else if (**buf == '"') {
        (*buf)++;
	const char* const start = *buf;
	int len = 0;
	while (**buf != '"' && **buf != 0x0a) {
	    ++len;
            *(*buf)++;
	}
        if (**buf == '"') (*buf)++;

        current_variable.type |= VAR_CONST;

	return pstring(start, len);
    }
    else if (**buf == encoding->TextMarker()) {
        pstring s(encoding->TextMarker());
	(*buf)++;

        char ch = **buf;
        while (ch != encoding->TextMarker() && ch != 0x0a && ch != '\0') {
	    if (encoding->UseTags() && ch == '~' && (ch = *++ (*buf)) != '~') {
                while (ch != '~') {
                    int l;
                    s += encoding->TranslateTag(*buf, l);
                    *buf += l;
                    ch = **buf;
                }
                ch = *++ (*buf);
                continue;
            }

	    int bytes;
            s += encoding->Encode(encoding->DecodeChar(*buf, bytes));
            *buf += bytes;
            ch = **buf;
        }

        if (**buf == encoding->TextMarker()) (*buf)++;

        current_variable.type |= VAR_CONST;
        end_status |= END_1BYTE_CHAR;
	return s;
    }
    else if (**buf == '#') { // for color
	pstring s(*buf, 7);
	*buf += 7;
        current_variable.type = VAR_NONE;
	return s;
    }
    else if (**buf == '*') { // label
        pstring s(*(*buf)++);
        SKIP_SPACE(*buf);
        char ch = **buf;
        while((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
              (ch >= '0' && ch <= '9') || ch == '_')
	{
            if (ch >= 'A' && ch <= 'Z') ch += 'a' - 'A';
            s += ch;
            ch = *++(*buf);
        }
        current_variable.type |= VAR_CONST | VAR_LABEL;
	return s;
    }   
    else { // bareword
        char ch;
	pstring alias_buf;
        bool first_flag = true;

        while (1) {
            ch = **buf;

            if ((ch >= 'a' && ch <= 'z')
                || (ch >= 'A' && ch <= 'Z')
                || ch == '_') {
                if (ch >= 'A' && ch <= 'Z') ch += 'a' - 'A';

                first_flag = false;
                alias_buf += ch;
            }
            else if (ch >= '0' && ch <= '9') {
                if (first_flag)
		    errorAndExit("parseStr: number is not allowed for the "
				 "first letter of str alias.");

                first_flag = false;
                alias_buf += ch;
            }
            else break;

            (*buf)++;
        }

        if (!alias_buf) {
            current_variable.type = VAR_NONE;
            return "";
        }

	stralias_t::iterator a = str_aliases.find(alias_buf);
	if (a == str_aliases.end()) {
            current_variable.type = VAR_NONE;
	    return alias_buf;
	}

        current_variable.type |= VAR_CONST;
	return a->second;
    }
}


int ScriptHandler::parseInt(const char** buf)
{
    int ret = 0;

    SKIP_SPACE(*buf);

    if (**buf == '%') {
        (*buf)++;
        current_variable.var_no = parseInt(buf);
        if (current_variable.var_no < 0
            || current_variable.var_no >= VARIABLE_RANGE)
            current_variable.var_no = VARIABLE_RANGE;

        current_variable.type = VAR_INT;
        return variable_data[current_variable.var_no].num;
    }
    else if (**buf == '?') {
	array_ref arr = parseArray(buf);
	current_variable.var_no = arr.first;
	current_variable.array = arr.second;
        current_variable.type  = VAR_ARRAY;
	ArrayVariable::iterator i = arrays.find(arr.first);
	if (i != arrays.end()) {
	    if (arr.second.size() < i->second.dimensions())
		arr.second.push_back(0);
	    return i->second.getValue(arr.second);
	}
	return 0;
    }
    else {
        char ch;
	pstring alias_buf;
        int alias_no = 0;
        bool direct_num_flag = false;
        bool num_alias_flag  = false;
        bool hex_num_flag = (*buf)[0] == '0' && (*buf)[1] == 'x';
        if (hex_num_flag) *buf += 2;

        const char* buf_start = *buf;
        while (1) {
            ch = **buf;

            if (hex_num_flag && isxdigit(ch)) {
                alias_no *= 16;
                if (isdigit(ch)) alias_no += ch - '0';
                else if (isupper(ch)) alias_no += ch - 'A' + 10;
                else alias_no += ch - 'a' + 10;
            }
            else if (isdigit(ch)) {
                if (!num_alias_flag) direct_num_flag = true;

                if (direct_num_flag)
                    alias_no = alias_no * 10 + ch - '0';
                else
                    alias_buf += ch;
            }
            else if (isalpha(ch) || ch == '_') {
                if (ch >= 'A' && ch <= 'Z') ch += 'a' - 'A';

                if (hex_num_flag || direct_num_flag) break;

                num_alias_flag = true;
                alias_buf += ch;
            }
            else break;

            (*buf)++;
        }

        if (*buf - buf_start == 0) {
            current_variable.type = VAR_NONE;
            return 0;
        }

        /* ---------------------------------------- */
        /* Solve num aliases */
        if (num_alias_flag) {

	    numalias_t::iterator a = num_aliases.find(alias_buf);
	    if (a == num_aliases.end()) {
                printf("can't find num alias for %s... assume 0.\n",
		       (const char*) alias_buf);
                current_variable.type = VAR_NONE;
                *buf = buf_start;
                return 0;
	    }
	    else {
		alias_no = a->second;
	    }
        }

        current_variable.type = VAR_INT | VAR_CONST;
        ret = alias_no;
    }

    SKIP_SPACE(*buf);

    return ret;
}


int ScriptHandler::parseIntExpression(const char** buf)
{
    int num[3], op[2]; // internal buffer

    SKIP_SPACE(*buf);

    readNextOp(buf, NULL, &num[0]);

    readNextOp(buf, &op[0], &num[1]);
    if (op[0] == OP_INVALID)
        return num[0];

    while (1) {
        readNextOp(buf, &op[1], &num[2]);
        if (op[1] == OP_INVALID) break;

        if (!(op[0] & 0x04) && (op[1] & 0x04)) { // if priority of op[1] is higher than op[0]
            num[1] = calcArithmetic(num[1], op[1], num[2]);
        }
        else {
            num[0] = calcArithmetic(num[0], op[0], num[1]);
            op[0]  = op[1];
            num[1] = num[2];
        }
    }
    return calcArithmetic(num[0], op[0], num[1]);
}


/*
 * Internal buffer looks like this.
 *   num[0] op[0] num[1] op[1] num[2]
 * If priority of op[0] is higher than op[1], (num[0] op[0] num[1]) is computed,
 * otherwise (num[1] op[1] num[2]) is computed.
 * Then, the next op and num is read from the script.
 * Num is an immediate value, a variable or a bracketed expression.
 */
void ScriptHandler::readNextOp(const char** buf, int* op, int* num)
{
    bool minus_flag = false;
    SKIP_SPACE(*buf);
    const char* buf_start = *buf;

    if (op) {
        if ((*buf)[0] == '+') *op = OP_PLUS;
        else if ((*buf)[0] == '-') *op = OP_MINUS;
        else if ((*buf)[0] == '*') *op = OP_MULT;
        else if ((*buf)[0] == '/') *op = OP_DIV;
        else if ((*buf)[0] == 'm'
                 && (*buf)[1] == 'o'
                 && (*buf)[2] == 'd'
                 && ((*buf)[3] == ' '
                     || (*buf)[3] == '\t'
                     || (*buf)[3] == '$'
                     || (*buf)[3] == '%'
                     || (*buf)[3] == '?'
                     || ((*buf)[3] >= '0' && (*buf)[3] <= '9')))
            *op = OP_MOD;
        else {
            *op = OP_INVALID;
            return;
        }

        if (*op == OP_MOD) *buf += 3;
        else (*buf)++;

        SKIP_SPACE(*buf);
    }
    else {
        if ((*buf)[0] == '-') {
            minus_flag = true;
            (*buf)++;
            SKIP_SPACE(*buf);
        }
    }

    if ((*buf)[0] == '(') {
        (*buf)++;
        *num = parseIntExpression(buf);
        if (minus_flag) *num = -*num;

        SKIP_SPACE(*buf);
        if ((*buf)[0] != ')') errorAndExit(") is not found.");

        (*buf)++;
    }
    else {
        *num = parseInt(buf);
        if (minus_flag) *num = -*num;

        if (current_variable.type == VAR_NONE) {
            if (op) *op = OP_INVALID;

            *buf = buf_start;
        }
    }
}


int ScriptHandler::calcArithmetic(int num1, int op, int num2)
{
    int ret = 0;

    if (op == OP_PLUS) ret = num1 + num2;
    else if (op == OP_MINUS) ret = num1 - num2;
    else if (op == OP_MULT) ret = num1 * num2;
    else if (op == OP_DIV) ret = num1 / num2;
    else if (op == OP_MOD) ret = num1 % num2;

    current_variable.type = VAR_INT | VAR_CONST;

    return ret;
}


ScriptHandler::array_ref ScriptHandler::parseArray(const char** buf)
{
    SKIP_SPACE(*buf);

    (*buf)++; // skip '?'
    int no = parseInt(buf);

    SKIP_SPACE(*buf);
    
    h_index_t indices;
    
    while (**buf == '[') {
        (*buf)++;
	indices.push_back(parseIntExpression(buf));
        SKIP_SPACE(*buf);
        if (**buf != ']') errorAndExit("parseArray: no ']' is found.");
        (*buf)++;
    }
    return std::make_pair(no, indices);
}

void ScriptHandler::declareDim()
{
    current_script = next_script;
    const char* buf = current_script;
    array_ref arr = parseArray(&buf);
    arrays.insert(std::make_pair(arr.first, ArrayVariable(this, arr.second)));
    next_script = buf;
}

bool ScriptHandler::LogInfo::find(pstring what)
{
    if (what[0] == '*') what.remove(0, 1);
    what.toupper();
    replace_ascii(what, '/', '\\');
    return logged.find(what) != logged.end();
}

void ScriptHandler::LogInfo::add(pstring what)
{
    if (what[0] == '*') what.remove(0, 1);
    what.toupper();
    replace_ascii(what, '/', '\\');    
    if (logged.find(what) == logged.end()) {
	logged.insert(what);
	ordered.push_back(&(*logged.find(what)));
    }
}

void ScriptHandler::LogInfo::write(ScriptHandler& h)
{
    pstring buf;
    buf.format("%d\n", ordered.size());
    for (ordered_t::const_iterator it = ordered.begin();
	 it != ordered.end(); ++it) {
	buf += '"';
	const char* si = **it;
	const char* ei = si + (*it)->length();
	while (si < ei) buf += char(*si++ ^ 0x84);
	buf += '"';
    }
    FILE* f = fopen(h.save_path + filename, "wb");
    if (f) {
	fwrite((const char*) buf, 1, buf.length(), f);
	fclose(f);
    }
    else {
        fprintf(stderr, "can't write %s\n", (const char*) filename);
        exit(-1);
    }
}

void ScriptHandler::LogInfo::read(ScriptHandler& h)
{
    clear();
    FILE* f = fopen(h.save_path + filename, "rb");
    size_t len = 1, ret = 0;
    char* buf = 0;
    if (f) {
	fseek(f, 0, SEEK_END);
	len = ftell(f);
	buf = new char[len];
	fseek(f, 0, SEEK_SET);
	ret = fread(buf, 1, len, f);
	fclose(f);
    }
    if (ret == len) {
	int count = 0;
	char *it = buf;
	while (*it != '\n') {
	    count = count * 10 + *it++ - '0';
	}
	++it; // \n
	while (count--) {
	    pstring item;
	    ++it; // "
	    while (*it != '"') {
		item += char(*it++ ^ 0x84);
	    }
	    ++it; // "
	    add(item);
	}
    }
    if (buf) delete[] buf;
}


int&
ScriptHandler::ArrayVariable::getoffs(const h_index_t& indices)
{
    if (indices.size() != dim.size()) {
	pstring msg;
	msg.format("Indexed %d deep into %d-dimensional array",
		   indices.size(), dim.size());
	owner->errorAndExit(msg);
    }
    int offs_idx = 0;
    for (h_index_t::size_type i = 0; i < indices.size(); ++i) {
	if (indices[i] > dim[i])
	    owner->errorAndExit("array index out of range");
	offs_idx *= dim[i];
	offs_idx += indices[i];
    }
    return data[offs_idx];
}

ScriptHandler::ArrayVariable::ArrayVariable(ScriptHandler* o, h_index_t sizes)
    : owner(o), dim(sizes)
{
    int sz = 1;
    for (h_index_t::iterator i = dim.begin(); i != dim.end(); ++i) sz *= ++(*i);
    data.assign(sz, 0);
}


struct aliases_t {
    set<pstring>::t aliases;
    aliases_t();    
} dodgy;
aliases_t::aliases_t() {
    aliases.insert("black");
    aliases.insert("white");
    aliases.insert("clear");
    aliases.insert("none");
    aliases.insert("fchk");
    aliases.insert("lchk");
    aliases.insert("on");
    aliases.insert("off");
    aliases.insert("remove");
    aliases.insert("step");
    aliases.insert("to");
}

void ScriptHandler::checkalias(const pstring& alias)
{
    if (dodgy.aliases.find(alias) != dodgy.aliases.end())
	fprintf(stderr,
		"Warning: alias `%s' may conflict with some barewords\n",
		(const char*) alias);
}

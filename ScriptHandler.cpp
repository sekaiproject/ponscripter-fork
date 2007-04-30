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
#include "FontInfo.h"
#include <ctype.h>

#define TMP_SCRIPT_BUF_LEN 4096
#define STRING_BUFFER_LENGTH 2048

#define SKIP_SPACE(p) while (*(p) == ' ' || *(p) == '\t') (p)++

BaseReader * ScriptHandler::cBR = NULL;

ScriptHandler::ScriptHandler()
    : game_identifier("Ponscripter")
{
    script_buffer = NULL;
    kidoku_buffer = NULL;
    label_log.filename = "NScrllog.dat";
    file_log.filename  = "NScrflog.dat";
    clickstr_list.clear();

    // the last one is a sink:
    variable_data = new VariableData[VARIABLE_RANGE + 1];

    arrays.clear();

    screen_size = SCREEN_SIZE_640x480;
    global_variable_border = 200;
}


ScriptHandler::~ScriptHandler()
{
    reset();

    if (script_buffer) delete[] script_buffer;

    if (kidoku_buffer) delete[] kidoku_buffer;

    delete[] variable_data;
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

    FontInfo::default_encoding = 0;
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
const char* ScriptHandler::readToken()
{
    current_script = next_script;
    char* buf = current_script;
    end_status = END_NONE;
    current_variable.type = VAR_NONE;

    text_flag = false;

    SKIP_SPACE(buf);
    markAsKidoku(buf);

readTokenTop:
    string_buffer.clear();
    char ch = *buf;
    if (ch == ';') { // comment
        addStringBuffer(ch);
        do {
            ch = *++buf;
            addStringBuffer(ch);
        } while (ch != 0x0a && ch != '\0');
    }
    else if (ch & 0x80
             || (ch >= '0' && ch <= '9')
             || ch == '@' || ch == '\\' || ch == '/'
             || ch == '%' || ch == '?' || ch == '$'
             || ch == '[' || ch == '('
             || ch == '!' || ch == '#' || ch == ',' || ch == '"') { // text
        bool loop_flag = true;
        bool ignore_click_flag = false;
        do {
            char bytes = encoding->CharacterBytes(buf);
            if (bytes > 1) {
                if (textgosub_flag && !ignore_click_flag && checkClickstr(buf))
		    loop_flag = false;
		string_buffer.append(buf, bytes);
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

                if (ch >= '0' && ch <= '9' &&
		    (*buf == ' ' || *buf == '\t' || *buf == encoding->TextMarker()) &&
		    string_buffer.size() % 2)
		    string_buffer += ' ';

                ch = *buf;
                if (ch == 0x0a || ch == '\0' || !loop_flag || ch == encoding->TextMarker())
		    break;

                SKIP_SPACE(buf);
                ch = *buf;
            }
        }
        while (ch != 0x0a && ch != '\0' && loop_flag && ch != encoding->TextMarker()) /*nop*/;
        if (loop_flag && ch == 0x0a && !(textgosub_flag && linepage_flag)) {
            string_buffer += ch;
            markAsKidoku(buf++);
        }

        text_flag = true;
    }
    else if (ch == encoding->TextMarker()) {
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

            const wchar uc = encoding->Decode(buf);
            buf += encoding->CharacterBytes(buf);
            string_buffer += encoding->Encode(uc);
            ch = *buf;
        }
        if (ch == encoding->TextMarker()) ++buf;

        if (ch == 0x0a && !(textgosub_flag && linepage_flag)) {
            string_buffer += ch;
            markAsKidoku(buf++);
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
        markAsKidoku(buf++);
    }
    else if (ch != '\0') {
        fprintf(stderr, "readToken: skip unknown heading character %c (%x)\n",
		ch, ch);
        buf++;
        goto readTokenTop;
    }

    next_script = checkComma(buf);

    return string_buffer.c_str();
}


const char* ScriptHandler::readLabel()
{
    end_status = END_NONE;
    current_variable.type = VAR_NONE;

    current_script = next_script;
    SKIP_SPACE(current_script);
    char* buf = current_script;

    string_buffer.clear();
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

    return string_buffer.c_str();
}


const char* ScriptHandler::readStr()
{
    end_status = END_NONE;
    current_variable.type = VAR_NONE;

    current_script = next_script;
    SKIP_SPACE(current_script);
    char* buf = current_script;

    string_buffer.clear();

    while (1) {
        string_buffer += parseStr(&buf);
        buf = checkComma(buf);
        if (buf[0] != '+') break;

        buf++;
    }
    next_script = buf;

    return string_buffer.c_str();
}


int ScriptHandler::readInt()
{
    string_buffer.clear();

    end_status = END_NONE;
    current_variable.type = VAR_NONE;

    current_script = next_script;
    SKIP_SPACE(current_script);
    char* buf = current_script;

    int ret = parseIntExpression(&buf);

    next_script = checkComma(buf);

    return ret;
}


void ScriptHandler::skipToken()
{
    SKIP_SPACE(current_script);
    char* buf = current_script;

    bool quat_flag = false;
    bool text_flag = false;
    while (1) {
        if (*buf == 0x0a ||
            (!quat_flag && !text_flag && (*buf == ':' || *buf == ';')))
	  break;

        if (*buf == '"') quat_flag = !quat_flag;

        const char bytes = encoding->CharacterBytes(buf);
        if (bytes > 1 && !quat_flag) text_flag = true;

        buf += bytes;
    }

    if (text_flag && *buf == 0x0a) ++buf;
    
    next_script = buf;
}


// script address direct manipulation function
void ScriptHandler::setCurrent(char* pos)
{
    current_script = next_script = pos;
}


void ScriptHandler::pushCurrent(char* pos)
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


int ScriptHandler::getOffset(char* pos)
{
    return pos - script_buffer;
}


char* ScriptHandler::getAddress(int offset)
{
    return script_buffer + offset;
}


int ScriptHandler::getLineByAddress(char* address)
{
    LabelInfo label = getLabelByAddress(address);

    char* addr = label.label_header;
    int   line = 0;
    while (address > addr) {
        if (*addr == 0x0a) line++;

        addr++;
    }
    return line;
}


char* ScriptHandler::getAddressByLine(int line)
{
    LabelInfo label = getLabelByLine(line);

    int   l    = line - label.start_line;
    char* addr = label.label_header;
    while (l > 0) {
        while (*addr != 0x0a) addr++;
        addr++;
        l--;
    }
    return addr;
}


ScriptHandler::LabelInfo ScriptHandler::getLabelByAddress(char* address)
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

    return (i == num) ? true : false;
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


void ScriptHandler::markAsKidoku(char* address)
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
    string fnam = save_path + "kidoku.dat";
    if ((fp = fopen(fnam.c_str(), "wb")) == NULL) {
        fprintf(stderr, "can't write kidoku.dat\n");
        return;
    }

    fwrite(kidoku_buffer, 1, script_buffer_length / 8, fp);
    fclose(fp);
}


void ScriptHandler::loadKidokuData()
{
    FILE* fp;
    string fnam = save_path + "kidoku.dat";
    setKidokuskip(true);
    kidoku_buffer = new char[script_buffer_length / 8 + 1];
    memset(kidoku_buffer, 0, script_buffer_length / 8 + 1);

    if ((fp = fopen(fnam.c_str(), "rb")) != NULL) {
        fread(kidoku_buffer, 1, script_buffer_length / 8, fp);
        fclose(fp);
    }
}


void ScriptHandler::addIntVariable(char** buf)
{
    string_buffer += stringFromInteger(parseInt(buf), -1);
}


void ScriptHandler::addStrVariable(char** buf)
{
    (*buf)++;
    string_buffer += variable_data[parseInt(buf)].str;
}


void ScriptHandler::enableTextgosub(bool val)
{
    textgosub_flag = val;
}


void ScriptHandler::setClickstr(string values)
{
    clickstr_list.clear();
    string::witerator it = values.wbegin();
    if (*it == encoding->TextMarker()) ++it;
    while (it != values.wend())
	clickstr_list.insert(*it++);
}


int ScriptHandler::checkClickstr(const char* buf, bool recursive_flag)
{
    if (buf[0] == '@' || buf[0] == '\\') return 1;
    wchar c = encoding->Decode(buf);
    int bytes = encoding->CharacterBytes(buf);
    if (clickstr_list.find(c) != clickstr_list.end()) {
	if (!recursive_flag && checkClickstr(buf + bytes, true)) return 0;
	return bytes;
    }
    return 0;
}


int ScriptHandler::getIntVariable(VariableInfo* var_info)
{
    if (var_info == NULL) var_info = &current_variable;

    if (var_info->type == VAR_INT)
        return variable_data[var_info->var_no].num;
    else if (var_info->type == VAR_ARRAY)
        return * getArrayPtr(var_info->var_no, var_info->array, 0);

    return 0;
}


void ScriptHandler::readVariable(bool reread_flag)
{
    end_status = END_NONE;
    current_variable.type = VAR_NONE;
    if (reread_flag) next_script = current_script;

    current_script = next_script;
    char* buf = current_script;

    SKIP_SPACE(buf);

    bool ptr_flag = false;
    if (*buf == 'i' || *buf == 'f') {
        ptr_flag = true;
        buf++;
    }

    if (*buf == '%') {
        buf++;
        current_variable.var_no = parseInt(&buf);
        if (current_variable.var_no < 0
            || current_variable.var_no >= VARIABLE_RANGE)
            current_variable.var_no = VARIABLE_RANGE;

        current_variable.type = VAR_INT;
    }
    else if (*buf == '?') {
        current_variable.var_no = parseArray(&buf, current_variable.array);
        current_variable.type = VAR_ARRAY;
    }
    else if (*buf == '$') {
        buf++;
        current_variable.var_no = parseInt(&buf);
        if (current_variable.var_no < 0
            || current_variable.var_no >= VARIABLE_RANGE)
            current_variable.var_no = VARIABLE_RANGE;

        current_variable.type = VAR_STR;
    }

    if (ptr_flag) current_variable.type |= VAR_PTR;

    next_script = checkComma(buf);
}


void ScriptHandler::setInt(VariableInfo* var_info, int val, int offset)
{
    if (var_info->type & VAR_INT) {
        setNumVariable(var_info->var_no + offset, val);
    }
    else if (var_info->type & VAR_ARRAY) {
        * getArrayPtr(var_info->var_no, var_info->array, offset) = val;
    }
    else {
        errorAndExit("setInt: no variables.");
    }
}


void ScriptHandler::pushVariable()
{
    pushed_variable = current_variable;
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


string ScriptHandler::stringFromInteger(int no, int num_column,
					bool is_zero_inserted)
{
    string n = nstr(no, num_column, is_zero_inserted);
    if (num_column > 0 && (int)n.size() > num_column) n.resize(num_column);
    if (n == "-" || n == "") n = "0";
    return n;
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


enum encodings { UTF8, CP932 };
static struct filetypes_t {
    char* filename;
    int encryption;
    encodings encoding;
} filetypes[] = {
    { "0.txt",        0, CP932 }, { "0.utf",        0, UTF8 },
    { "00.txt",       0, CP932 }, { "00.utf",       0, UTF8 },
    { "nscr_sec.dat", 2, CP932 }, { "pscr_sec.dat", 2, UTF8 },
    { "nscript.___",  3, CP932 }, { "pscript.___",  3, UTF8 },
    { "nscript.dat",  1, CP932 }, { "pscript.dat",  1, UTF8 },
    { 0, 0, UTF8 }
};

int ScriptHandler::readScript(const char* path)
{
    archive_path = path;

    FILE* fp = NULL;
    int encrypt_mode;
    encodings enc;

    for (filetypes_t* ft = filetypes; ft->filename; ++ft) {
	if ((fp = fopen(ft->filename, "rb")) != NULL) {
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
        fprintf(stderr, "can't open any of 0.txt, 0.utf, 00.txt, 00.utf, nscript.dat, pscript.dat, nscript.___, or pscript.___\n");
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    int estimated_buffer_length = ftell(fp) + 1;

    if (encrypt_mode == 0) {
        fclose(fp);
        for (int i = 1; i < 100; i++) {
	    string filename = str(i) + (enc == UTF8 ? ".utf" : ".txt");
            if ((fp = fopen(filename.c_str(), "rb")) == NULL) {
		filename = "0" + filename;
                fp = fopen(filename.c_str(), "rb");
            }

            if (fp) {
                fseek(fp, 0, SEEK_END);
                estimated_buffer_length += ftell(fp) + 1;
                fclose(fp);
            }
        }
    }

    if (script_buffer) delete[] script_buffer;

    script_buffer = new char[estimated_buffer_length];

    char* p_script_buffer;
    current_script = p_script_buffer = script_buffer;

    tmp_script_buf = new char[TMP_SCRIPT_BUF_LEN];
    if (encrypt_mode > 0) {
        fseek(fp, 0, SEEK_SET);
        readScriptSub(fp, &p_script_buffer, encrypt_mode);
        fclose(fp);
    }
    else {
        for (int i = 0; i < 100; i++) {
	    string filename = str(i) + (enc == UTF8 ? ".utf" : ".txt");
            if ((fp = fopen(filename.c_str(), "rb")) == NULL) {
		filename = "0" + filename;
                fp = fopen(filename.c_str(), "rb");
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
    char* buf = script_buffer + 1;
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
        else {
            break;
        }

        if (*buf != ',') {
            while (*buf++ != '\n') ;
            break;
        }

        buf++;
    }
    // game ID check
    if (*buf++ == ';') {
        while (*buf == ' ' || *buf == '\t') ++buf;
        if (!strncmp(buf, "gameid ", 7)) {
            buf += 7;
            int i = 0;
            while (buf[i++] >= ' ') ;
	    game_identifier.assign(buf, i - 1);
            buf += i;
        }
    }

    return labelScript();
}


int ScriptHandler::labelScript()
{
    int current_line = 0;
    char* buf = script_buffer;
    label_info.clear();

    while (buf < script_buffer + script_buffer_length) {
        SKIP_SPACE(buf);
        if (*buf == '*') {
            setCurrent(buf);
            readLabel();
	    LabelInfo new_label;
	    new_label.name.assign(string_buffer, 1, string_buffer.size() - 1);
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


ScriptHandler::LabelInfo ScriptHandler::lookupLabel(const string& label)
{
    LabelInfo::iterator i = findLabel(label);
    label_log.add(label);
    return *i;
}


ScriptHandler::LabelInfo ScriptHandler::lookupLabelNext(const string& label)
{
    LabelInfo::iterator i = findLabel(label);
    if (++i != label_info.end()) {
	label_log.add(label);
        return *i;
    }
    return LabelInfo();
}


void ScriptHandler::errorAndExit(const char* str)
{
    fprintf(stderr, " **** Script error, %s [%s] ***\n", str, string_buffer.c_str());
    exit(-1);
}


// ----------------------------------------
// Private methods

ScriptHandler::LabelInfo::iterator ScriptHandler::findLabel(string label)
{
    for (string::size_type i = 0; i < label.size(); ++i)
        if ('A' <= label[i] && label[i] <= 'Z')
	    label[i] += 'a' - 'A';

    LabelInfo::dic::iterator e = label_names.find(label);
    if (e != label_names.end())
	return e->second;

    label = "Label \"" + label + "\" is not found.";
    errorAndExit(label.c_str());
    return label_info.end(); // dummy
}


char* ScriptHandler::checkComma(char* buf)
{
    SKIP_SPACE(buf);
    if (*buf == ',') {
        end_status |= END_COMMA;
        buf++;
        SKIP_SPACE(buf);
    }

    return buf;
}


string ScriptHandler::parseStr(char** buf)
{
    SKIP_SPACE(*buf);

    if (**buf == '(') {
	// (foo) bar baz : apparently returns bar if foo has been
	// viewed, baz otherwise.
	
        (*buf)++;
        string s = parseStr(buf);
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
	string s;
        (*buf)++;
	while (**buf != '"' && **buf != 0x0a)
            s.push_back(*(*buf)++);
        if (**buf == '"') (*buf)++;

        current_variable.type |= VAR_CONST;
	return s;
    }
    else if (**buf == encoding->TextMarker()) {
        string s;
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

            const wchar uc = encoding->Decode(*buf);
            *buf += encoding->CharacterBytes(*buf);
            s += encoding->Encode(uc);
            ch = **buf;
        }

        if (**buf == encoding->TextMarker()) (*buf)++;

        current_variable.type |= VAR_CONST;
        end_status |= END_1BYTE_CHAR;
	return s;
    }
    else if (**buf == '#') { // for color
	string s(*buf, 7);
	*buf += 7;
        current_variable.type = VAR_NONE;
	return s;
    }
    else if (**buf == '*') { // label
        string s(1, *(*buf)++);
        SKIP_SPACE(*buf);
        char ch = **buf;
        while((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
              (ch >= '0' && ch <= '9') || ch == '_')
	{
            if (ch >= 'A' && ch <= 'Z') ch += 'a' - 'A';
            s += ch;
            ch = *++(*buf);
        }
        current_variable.type |= VAR_CONST;
	return s;
    }   
    else { // str alias
        char ch;
	string alias_buf;
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
            printf("can't find str alias for %s...\n", alias_buf.c_str());
            exit(-1);
	}
        current_variable.type |= VAR_CONST;
	return a->second;
    }
}


int ScriptHandler::parseInt(char** buf)
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
        current_variable.var_no = parseArray(buf, current_variable.array);
        current_variable.type  = VAR_ARRAY;
        return *getArrayPtr(current_variable.var_no, current_variable.array, 0);
    }
    else {
        char ch;
	string alias_buf;
        int alias_no = 0;
        bool direct_num_flag = false;
        bool num_alias_flag  = false;
        bool hex_num_flag = (*buf)[0] == '0' & (*buf)[1] == 'x';
        if (hex_num_flag) *buf += 2;

        char* buf_start = *buf;
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
                printf("can't find num alias for %s... assume 0.\n", alias_buf.c_str());
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


int ScriptHandler::parseIntExpression(char** buf)
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
void ScriptHandler::readNextOp(char** buf, int* op, int* num)
{
    bool minus_flag = false;
    SKIP_SPACE(*buf);
    char* buf_start = *buf;

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


int ScriptHandler::parseArray(char** buf, struct ArrayVariable &array)
{
    SKIP_SPACE(*buf);

    (*buf)++; // skip '?'
    int no = parseInt(buf);

    SKIP_SPACE(*buf);
    array.num_dim = 0;
    while (**buf == '[') {
        (*buf)++;
        array.dim[array.num_dim] = parseIntExpression(buf);
        array.num_dim++;
        SKIP_SPACE(*buf);
        if (**buf != ']') errorAndExit("parseArray: no ']' is found.");

        (*buf)++;
    }
    for (int i = array.num_dim; i < 20; i++) array.dim[i] = 0;

    return no;
}


int* ScriptHandler::getArrayPtr(int no, ArrayVariable &array, int offset)
{
    ArrayVariable::iterator it = arrays.find(no);
    if (it == arrays.end()) errorAndExit("Array No. is not declared.");
    ArrayVariable& av = it->second;

    int dim = 0, i;
    for (i = 0; i < av.num_dim; i++) {
        if (av.dim[i] <= array.dim[i])
	    errorAndExit("dim[i] <= array.dim[i].");

        dim = dim * av.dim[i] + array.dim[i];
    }

    if (av.dim[i - 1] <= array.dim[i - 1] + offset)
	errorAndExit("dim[i-1] <= array.dim[i-1] + offset.");

    return &av.data[dim + offset];
}


void ScriptHandler::declareDim()
{
    current_script = next_script;
    char* buf = current_script;

    ArrayVariable array;
    int no = parseArray(&buf, array);
    ArrayVariable* newarr = &arrays[no];

    int dim = 1;
    newarr->num_dim = array.num_dim;
    for (int i = 0; i < array.num_dim; i++) {
        newarr->dim[i] = array.dim[i] + 1;
        dim *= array.dim[i] + 1;
    }
    newarr->data.assign(dim, 0);
    next_script = buf;
}

bool ScriptHandler::LogInfo::find(string what)
{
    if (what[0] == '*') what.shift();
    what.uppercase();
    what.replace('/', '\\');
    return logged.find(what) != logged.end();
}

void ScriptHandler::LogInfo::add(string what)
{
    if (what[0] == '*') what.shift();
    what.uppercase();
    what.replace('/', '\\');    
    if (logged.find(what) == logged.end()) {
	logged.insert(what);
	ordered.push_back(&(*logged.find(what)));
    }
}

void ScriptHandler::LogInfo::write(ScriptHandler& h)
{
    string buf = str(ordered.size()) + '\n';
    for (ordered_t::const_iterator it = ordered.begin();
	 it != ordered.end(); ++it) {
	buf += '"';
	for (string::const_iterator si = (*it)->begin(); si != (*it)->end();
	     ++si)
	    buf += char(*si ^ 0x84);
	buf += '"';
    }
    string fnam = h.save_path + filename;
    FILE* f = fopen(fnam.c_str(), "wb");
    if (f) {
	fwrite(buf.data(), 1, buf.size(), f);
	fclose(f);
    }
    else {
        fprintf(stderr, "can't write %s\n", filename.c_str());
        exit(-1);
    }
}

void ScriptHandler::LogInfo::read(ScriptHandler& h)
{
    clear();
    string fnam = h.save_path + filename;
    FILE* f = fopen(fnam.c_str(), "rb");
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
	while (*it != '\n') count = count * 10 + *it++ - '0';
	++it; // \n
	while (count--) {
	    string item;
	    ++it; // "
	    while (*it != '"') item += char(*it++ ^ 0x84);
	    ++it; // "
	    add(item);
	}
    }
    if (buf) delete[] buf;
}

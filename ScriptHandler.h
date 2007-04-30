/* -*- C++ -*-
 *
 *  ScriptHandler.h - Script manipulation class
 *
 *  Copyright (c) 2001-2005 Ogapee (original ONScripter, of which this
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

#ifndef __SCRIPT_HANDLER_H__
#define __SCRIPT_HANDLER_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "defs.h"
#include "BaseReader.h"

const int VARIABLE_RANGE = 4096;

class ScriptHandler {
public:
    bool is_ponscripter;
    
    enum { END_NONE  = 0,
           END_COMMA = 1,
           END_1BYTE_CHAR = 2 };
    struct LabelInfo {
	typedef std::vector<LabelInfo> vec;
	typedef vec::iterator iterator;
	typedef dictionary<string, iterator>::t dic;
	string name;
        char* label_header;
        char* start_address;
        int start_line;
        int num_of_lines;
	LabelInfo() : start_address(NULL) {}
    };

    struct ArrayVariable {
        ArrayVariable* next;
        int no;
        int num_dim;
        int dim[20];
        int* data;
        ArrayVariable() {
            next = NULL;
            data = NULL;
        };
        ~ArrayVariable() {
            if (data) delete[] data;
        };
        ArrayVariable& operator=(const ArrayVariable& av)
        {
	    if (&av == this) return *this;

            no = av.no;
            num_dim = av.num_dim;

            int total_dim = 1;
            for (int i = 0; i < 20; i++) {
                dim[i]     = av.dim[i];
                total_dim *= dim[i];
            }

            if (data) delete[] data;

            data = NULL;
            if (av.data) {
                data = new int[total_dim];
                memcpy(data, av.data, sizeof(int) * total_dim);
            }

            return *this;
        };
    };

    enum { VAR_NONE  = 0,
           VAR_INT   = 1,  // integer
           VAR_ARRAY = 2,  // array
           VAR_STR   = 4,  // string
           VAR_CONST = 8,  // direct value or alias, not variable
           VAR_PTR   = 16  // poiter to a variable, e.g. i%0, s%0
    };
    struct VariableInfo {
        int type;
        int var_no;   // for integer(%), array(?), string($) variable
        ArrayVariable array; // for array(?)
    };

    ScriptHandler();
    ~ScriptHandler();

    void reset();
    void setKeyTable(const unsigned char* key_table);

    // basic parser function
    const char* readToken();
    const char* readLabel();
    void readVariable(bool reread_flag = false);
    const char* readStr();
    int  readInt();
    int  parseInt(char** buf);
    void skipToken();

    // function for string access
    inline string& getStringBuffer() { return string_buffer; }
    void addStringBuffer(char ch) { string_buffer += ch; }

    // function for direct manipulation of script address
    inline char* getCurrent() { return current_script; };
    inline char* getNext() { return next_script; };
    void setCurrent(char* pos);
    void pushCurrent(char* pos);
    void popCurrent();

    int getScriptBufferLength() const { return script_buffer_length; }
    
    int  getOffset(char* pos);
    char* getAddress(int offset);
    int  getLineByAddress(char* address);
    char* getAddressByLine(int line);
    LabelInfo getLabelByAddress(char* address);
    LabelInfo getLabelByLine(int line);

    bool isText();
    bool compareString(const char* buf);

    inline int getEndStatus() { return end_status; };
    void skipLine(int no = 1);
    void setLinepage(bool val);

    // function for kidoku history
    bool isKidoku();
    void markAsKidoku(char* address = NULL);
    void setKidokuskip(bool kidokuskip_flag);
    void saveKidokuData();
    void loadKidokuData();

    void addStrVariable(char** buf);
    void addIntVariable(char** buf);
    void declareDim();

    void enableTextgosub(bool val);
    void setClickstr(string values);
    int  checkClickstr(const char* buf, bool recursive_flag = false);

    void setInt(VariableInfo* var_info, int val, int offset = 0);
    void setNumVariable(int no, int val);
    void pushVariable();
    int getIntVariable(VariableInfo* var_info = NULL);

    string stringFromInteger(int no, int num_column,
			     bool is_zero_inserted = false);
    
    int  readScriptSub(FILE* fp, char** buf, int encrypt_mode);
    int  readScript(const char* path);
    int  labelScript();

    LabelInfo lookupLabel(const string& label);
    LabelInfo lookupLabelNext(const string& label);
    void errorAndExit(const char* str);

    ArrayVariable* getRootArrayVariable();
    void loadArrayVariable(FILE* fp);

    void addNumAlias(const string& str, int val)
	{ num_aliases[str] = val; }
    void addStrAlias(const string& str, const string& val)
	{ str_aliases[str] = val; }


    class LogInfo {
	typedef set<string>::t logged_t;
	logged_t logged;
	typedef std::vector<const string*> ordered_t;
	ordered_t ordered;
    public:
	string filename;
	bool find(string what);
	void add(string what);
	void clear() { ordered.clear(); logged.clear(); }
	void write(ScriptHandler& h);
	void read(ScriptHandler& h);
    } label_log, file_log;

    /* ---------------------------------------- */
    /* Variable */
    struct VariableData {
        int num;
        bool num_limit_flag;
        int num_limit_upper;
        int num_limit_lower;
        string str;

        VariableData() {
            reset(true);
        };
        void reset(bool limit_reset_flag)
        {
            num = 0;
            if (limit_reset_flag)
                num_limit_flag = false;

            if (str) str.clear();
        };
    }* variable_data;

    VariableInfo current_variable, pushed_variable;

    int screen_size;
    enum { SCREEN_SIZE_640x480 = 0,
           SCREEN_SIZE_800x600 = 1,
           SCREEN_SIZE_400x300 = 2,
           SCREEN_SIZE_320x240 = 3 };
    int global_variable_border;

    string game_identifier;
    string save_path;

    static BaseReader* cBR;

private:
    enum { OP_INVALID = 0, // 000
           OP_PLUS  = 2,   // 010
           OP_MINUS = 3, // 011
           OP_MULT  = 4,   // 100
           OP_DIV   = 5,   // 101
           OP_MOD   = 6    // 110
    };

    LabelInfo::iterator findLabel(string label);

    char* checkComma(char* buf);
    string parseStr(char** buf);
    int  parseIntExpression(char** buf);
    void readNextOp(char** buf, int* op, int* num);
    int  calcArithmetic(int num1, int op, int num2);
    int  parseArray(char** buf, ArrayVariable &array);
    int* getArrayPtr(int no, ArrayVariable &array, int offset);

    /* ---------------------------------------- */
    /* Variable */
    typedef dictionary<string, int>::t    numalias_t;
    typedef dictionary<string, string>::t stralias_t;
    numalias_t num_aliases;
    stralias_t str_aliases;

    ArrayVariable *root_array_variable, *current_array_variable;

    string archive_path;
    int   script_buffer_length;
    char* script_buffer;
    char* tmp_script_buf;

    string string_buffer; // updated only by readToken (is this true?)

    LabelInfo::vec label_info;
    LabelInfo::dic label_names;
    
    bool  skip_enabled;
    bool  kidokuskip_flag;
    char* kidoku_buffer;

    bool  text_flag; // true if the current token is text
    int   end_status;
    bool  linepage_flag;
    bool  textgosub_flag;
    std::set<wchar> clickstr_list;

    char* current_script;
    char* next_script;

    char* pushed_current_script;
    char* pushed_next_script;

    unsigned char key_table[256];
    bool key_table_flag;
};

#endif // __SCRIPT_HANDLER_H__

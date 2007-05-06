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
#include "expression.h"

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
	typedef std::map<int, ArrayVariable> map;
	typedef map::iterator iterator;

	int  getValue(const index_t& i)          { return getoffs(i); }
	void setValue(const index_t& i, int val) { getoffs(i) = val; }

	int  getValue(const int* indices, int num_idx);
	void setValue(const int* indices, int num_idx, int val);

	int getDimensionSize(int depth) { return dim[depth]; }
	
	ArrayVariable(ScriptHandler* o) : owner(o) {}

	// TODO: make these private
        index_t dim;
	index_t data;
    private:
	ScriptHandler* owner;
	int& getoffs(const index_t& indices);
    };
    ArrayVariable::map arrays;

    enum { VAR_NONE  = 0,
           VAR_INT   = 1,  // integer
           VAR_ARRAY = 2,  // array
           VAR_STR   = 4,  // string
           VAR_CONST = 8,  // direct value or alias, not variable
           VAR_PTR   = 16, // pointer to a variable, e.g. i%0, s%0
	   VAR_LABEL = 32  // label, not string constant
    };
    struct VariableInfo {
        int type;
        int var_no;   // for integer(%), array(?), string($) variable
        index_t array; // for array(?)
	VariableInfo() {}
    };

    ScriptHandler();
    ~ScriptHandler();

    void reset();
    void setKeyTable(const unsigned char* key_table);

    // basic parser function
    const char* readToken();
private:
    const char* readLabel();
    const char* readStr();
    int  readInt();
public:    
    int  parseInt(char** buf);
    void skipToken();

    // saner parser functions :)
    // Implementations in expression.cpp
    string readStrValue();
    string readBareword();
    int readIntValue();
    Expression readStrExpr();
    Expression readIntExpr();
    Expression readExpr();
    char checkPtr();

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
    int  getLineByAddress(char* address, bool absolute = false);
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

    void setNumVariable(int no, int val);

    string stringFromInteger(int no, int num_column,
			     bool is_zero_inserted = false);
    
    int readScriptSub(FILE* fp, char** buf, int encrypt_mode);
    int readScript(const char* path);
    int labelScript();

    LabelInfo lookupLabel(const string& label);
    LabelInfo lookupLabelNext(const string& label);
    void errorAndExit(string str);

    void loadArrayVariable(FILE* fp);

    void addNumAlias(const string& str, int val)
	{ checkalias(str); num_aliases[str] = val; }
    void addStrAlias(const string& str, const string& val)
	{ checkalias(str); str_aliases[str] = val; }


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
    };
    std::vector<VariableData> variable_data;

    VariableInfo current_variable;

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
    typedef std::pair<int, index_t> array_ref;
    array_ref parseArray(char** buf);
    int* getArrayPtr(int no, index_t indices, int offset);
    
    /* ---------------------------------------- */
    /* Variable */
    typedef dictionary<string, int>::t    numalias_t;
    typedef dictionary<string, string>::t stralias_t;
    void checkalias(const string& alias); // warns if an alias may cause trouble
    numalias_t num_aliases;
    stralias_t str_aliases;

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

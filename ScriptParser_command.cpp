/* -*- C++ -*-
 *
 *  ScriptParser_command.cpp - Define command executer of Ponscripter
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ScriptParser.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int ScriptParser::zenkakkoCommand(const pstring& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit(cmd, "not in the define section");

    zenkakko_flag = true;

    return RET_CONTINUE;
}


int ScriptParser::windowbackCommand(const pstring& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit(cmd, "not in the define section");

    windowback_flag = true;

    return RET_CONTINUE;
}


int ScriptParser::versionstrCommand(const pstring& cmd)
{
    version_str = script_h.readStrValue() + '\n';
    version_str += script_h.readStrValue() + '\n';
    return RET_CONTINUE;
}


int ScriptParser::usewheelCommand(const pstring& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("usewheel: not in the define section");

    usewheel_flag = true;

    return RET_CONTINUE;
}


int ScriptParser::useescspcCommand(const pstring& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("useescspc: not in the define section");

    if (!force_button_shortcut_flag)
        useescspc_flag = true;

    return RET_CONTINUE;
}


int ScriptParser::underlineCommand(const pstring& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("underline: not in the define section");

    underline_value = script_h.readIntValue() * screen_ratio1 / screen_ratio2;

    return RET_CONTINUE;
}


int ScriptParser::transmodeCommand(const pstring& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("transmode: not in the define section");

    pstring l = script_h.readStrValue();
    
    if (l == "leftup")
	trans_mode = AnimationInfo::TRANS_TOPLEFT;
    else if (l == "copy")
	trans_mode = AnimationInfo::TRANS_COPY;
    else if (l == "alpha")
	trans_mode = AnimationInfo::TRANS_ALPHA;
    else if (l == "rightup")
	trans_mode = AnimationInfo::TRANS_TOPRIGHT;

    return RET_CONTINUE;
}


int ScriptParser::timeCommand(const pstring& cmd)
{
    time_t t = time(NULL);
    struct tm* tm = localtime(&t);
    script_h.readIntExpr().mutate(tm->tm_hour);
    script_h.readIntExpr().mutate(tm->tm_min);
    script_h.readIntExpr().mutate(tm->tm_sec);
    return RET_CONTINUE;
}


int ScriptParser::textgosubCommand(const pstring& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("textgosub: not in the define section");

    Expression e = script_h.readStrExpr();
    e.require_label();
    textgosub_label = e.as_string();
    script_h.enableTextgosub(true);

    return RET_CONTINUE;
}


int ScriptParser::tanCommand(const pstring& cmd)
{
    Expression e = script_h.readIntExpr();
    e.mutate(int(tan(M_PI * script_h.readIntValue() / 180.0) * 1000.0));
    return RET_CONTINUE;
}


int ScriptParser::subCommand(const pstring& cmd)
{
    Expression e = script_h.readIntExpr();
    e.mutate(e.as_int() - script_h.readIntValue());
    return RET_CONTINUE;
}


int ScriptParser::straliasCommand(const pstring& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("stralias: not in the define section");
    pstring label = script_h.readStrValue();
    script_h.addStrAlias(label, script_h.readStrValue());
    return RET_CONTINUE;
}


int ScriptParser::soundpressplginCommand(const pstring& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("soundpressplgin: not in the define section");

    CBStringList opts = script_h.readStrValue().split('|', 2);
    
    // only nbzplgin.dll is supported
    opts[0].tolower();
    if (opts[0] != "nbzplgin.dll")
        fprintf(stderr, " *** plugin %s is not available, ignored. ***\n",
		(const char*) opts[0]);
    else
	ScriptHandler::cBR->
	    registerCompressionType(opts[1], BaseReader::NBZ_COMPRESSION);
	
    return RET_CONTINUE;
}


int ScriptParser::skipCommand(const pstring& cmd)
{
    int line = current_label_info.start_line + current_line
	     + script_h.readIntValue();

    const char* buf = script_h.getAddressByLine(line);
    current_label_info = script_h.getLabelByAddress(buf);
    current_line = script_h.getLineByAddress(buf);

    script_h.setCurrent(buf);

    return RET_CONTINUE;
}


int ScriptParser::sinCommand(const pstring& cmd)
{
    Expression e = script_h.readIntExpr();
    e.mutate(int(sin(M_PI * script_h.readIntValue() / 180.0) * 1000.0));
    return RET_CONTINUE;
}


int ScriptParser::shadedistanceCommand(const pstring& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("shadedistance: not in the define section");

    shade_distance[0] = script_h.readIntValue();
    shade_distance[1] = script_h.readIntValue();

    return RET_CONTINUE;
}


int ScriptParser::selectvoiceCommand(const pstring& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("selectvoice: not in the define section");

    for (int i = 0; i < SELECTVOICE_NUM; i++)
        selectvoice_file_name[i] = script_h.readStrValue();

    return RET_CONTINUE;
}


int ScriptParser::selectcolorCommand(const pstring& cmd)
{
    sentence_font.on_color = readColour(script_h.readStrValue());
    sentence_font.off_color = readColour(script_h.readStrValue());

    return RET_CONTINUE;
}


int ScriptParser::savenumberCommand(const pstring& cmd)
{
    num_save_file = script_h.readIntValue();

    return RET_CONTINUE;
}


int ScriptParser::savenameCommand(const pstring& cmd)
{
    save_menu_name = script_h.readStrValue();
    load_menu_name = script_h.readStrValue();
    save_item_name = script_h.readStrValue();
    return RET_CONTINUE;
}


int ScriptParser::rubyonCommand(const pstring& cmd)
{
    // disabled
    const char* buf = script_h.getNext();
    if (buf[0] == 0x0a || buf[0] == ':' || buf[0] == ';') { }
    else {
        script_h.readIntValue();
        script_h.readIntValue();

        if (script_h.hasMoreArgs()) {
            script_h.readStrExpr();
        }
    }

    return RET_CONTINUE;
}


int ScriptParser::rubyoffCommand(const pstring& cmd)
{
    // disabled
    return RET_CONTINUE;
}


int ScriptParser::roffCommand(const pstring& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("roff: not in the define section");

    rmode_flag = false;

    return RET_CONTINUE;
}


int ScriptParser::rmenuCommand(const pstring& cmd)
{
    rmenu.clear();
    rmenu_link_width = 0;

    do {
	pstring s = script_h.readStrValue();
	rmenu.push_back(RMenuElt(s, getSystemCallNo(script_h.readStrValue())));
    } while (script_h.hasMoreArgs());

    return RET_CONTINUE;
}


int ScriptParser::returnCommand(const pstring& cmd)
{
    if (nest_infos.empty() ||
        (nest_infos.back().nest_mode != NestInfo::LABEL &&
         nest_infos.back().nest_mode != NestInfo::TEXTGOSUB))
        errorAndExit("return: not in gosub");

    const bool is_text = nest_infos.back().nest_mode == NestInfo::TEXTGOSUB;
    const char* const next_script = nest_infos.back().next_script;
    
    current_label_info = script_h.getLabelByAddress(next_script);
    current_line       = script_h.getLineByAddress(next_script);

    if (is_text) {
        script_h.setCurrent(next_script);
        string_buffer_restore = nest_infos.back().to;
    }
    else {
        const char *buf = script_h.getNext();
        if (buf[0] == 0x0a || buf[0] == ':' || buf[0] == ';')
            script_h.setCurrent(next_script);
        else
            setCurrentLabel(script_h.readStrValue());
    }

    nest_infos.pop_back();

    return RET_CONTINUE;
}


int ScriptParser::pretextgosubCommand(const pstring& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("pretextgosub: not in the define section");

    Expression e = script_h.readStrExpr();
    e.require_label();
    pretextgosub_label = e.as_string();

    return RET_CONTINUE;
}


int ScriptParser::numaliasCommand(const pstring& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("numalias: numalias: not in the define section");
    pstring label = script_h.readStrValue();
    label.tolower();
    int no = script_h.readIntValue();
    // Extension: allow detection of Ponscripter in compatibility mode
    // by allowing the user to define an alias "ponscripter, 0" that
    // Ponscripter actually defines as 1.
    if (!script_h.is_ponscripter && label == "ponscripter") no = 1;
    script_h.addNumAlias(label, no);
    return RET_CONTINUE;
}


int ScriptParser::nsadirCommand(const pstring& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("nsadir: not in the define section");

    nsa_path = script_h.readStrValue() + DELIMITER;
    return RET_CONTINUE;
}


int ScriptParser::nsaCommand(const pstring& cmd)
{
    int archive_type = NsaReader::ARCHIVE_TYPE_NSA;

    if (cmd == "ns2") {
        archive_type = NsaReader::ARCHIVE_TYPE_NS2;
    }
    else if (cmd == "ns3") {
        archive_type = NsaReader::ARCHIVE_TYPE_NS3;
    }

    delete ScriptHandler::cBR;
    ScriptHandler::cBR = new NsaReader(archive_path, key_table);
    if (ScriptHandler::cBR->open(nsa_path, archive_type))
        fprintf(stderr, " *** failed to open Nsa archive, ignored.  ***\n");

    return RET_CONTINUE;
}


int ScriptParser::nextCommand(const pstring& cmd)
{
    if (nest_infos.empty() || nest_infos.back().nest_mode != NestInfo::FOR)
        errorAndExit("next: not in for loop\n");

    Expression& e = nest_infos.back().var;

    if (!break_flag)
	e.mutate(e.as_int() + nest_infos.back().step);

    int val = e.as_int();

    if (break_flag
        || (nest_infos.back().step > 0 && val > nest_infos.back().to)
        || (nest_infos.back().step < 0 && val < nest_infos.back().to)) {
        break_flag = false;
	nest_infos.pop_back();
    }
    else {
        script_h.setCurrent(nest_infos.back().next_script);
        current_label_info =
	    script_h.getLabelByAddress(nest_infos.back().next_script);
        current_line =
	    script_h.getLineByAddress(nest_infos.back().next_script);
    }

    return RET_CONTINUE;
}


int ScriptParser::mulCommand(const pstring& cmd)
{
    Expression e = script_h.readIntExpr();
    e.mutate(e.as_int() * script_h.readIntValue());
    return RET_CONTINUE;
}


int ScriptParser::movCommand(const pstring& cmd)
{
    // Haeleth extension: movz is like movl, but if not enough
    // arguments are provided, fills remaining spaces with zeroes.
    Expression e = script_h.readExpr();
    int limit = cmd == "mov" ? 1
	      : cmd == "movl" || cmd == "movz" ? e.dim()
	      : atoi(((const char*) cmd) + 3);

    // ONScripter has been a bit permissive in the past.
    if (!script_h.is_ponscripter && e.is_array() &&
	cmd != "movl" && cmd != "movz" && cmd != "mov")
	errorAndCont("NScripter does not permit `" + cmd + " " +
		     e.debug_string() + ", ...': for portability, use "
		     "`movl' or a series of `mov' calls instead.");
    
    if (e.is_textual()) {
	if (limit != 1)
	    errorAndExit(cmd + " is not valid with string variables (use mov)");
	e.mutate(script_h.readStrValue());
    }
    else {
	bool movl = cmd == "movl" || cmd == "movz";
	for (int i = 0; i < limit; ++i) {
	    int val;
	    if (script_h.hasMoreArgs())
		val = script_h.readIntValue();
	    else if (cmd == "movz")
		val = 0;
	    else
		errorAndExit("Not enough arguments to " + cmd);
	    e.mutate(val, i, movl);
	}
	if (script_h.hasMoreArgs())
	    errorAndCont("Too many arguments to " + cmd);
    }
    return RET_CONTINUE;
}


int ScriptParser::mode_sayaCommand(const pstring& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("mode_saya: not in the define section");

    mode_saya_flag = true;

    return RET_CONTINUE;
}


int ScriptParser::mode_extCommand(const pstring& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("mode_ext: not in the define section");

    mode_ext_flag = true;

    return RET_CONTINUE;
}


int ScriptParser::modCommand(const pstring& cmd)
{
    Expression e = script_h.readIntExpr();
    e.mutate(e.as_int() % script_h.readIntValue());
    return RET_CONTINUE;
}


int ScriptParser::midCommand(const pstring& cmd)
{
    Expression e       = script_h.readStrExpr();
    pstring src        = script_h.readStrValue();
    unsigned int start = script_h.readIntValue();
    unsigned int len   = script_h.readIntValue();
    e.mutate(src.midstr(start, len));
    return RET_CONTINUE;
}


int ScriptParser::menusetwindowCommand(const pstring& cmd)
{
    int s1 = script_h.readIntValue();
    int s2 = script_h.readIntValue();
    menu_font.set_size(s1 > s2 ? s1 : s2);
    menu_font.set_mod_size(0);
    menu_font.pitch_x   = script_h.readIntValue();
    menu_font.pitch_y   = script_h.readIntValue();
    menu_font.is_bold   = script_h.readIntValue();
    menu_font.is_shadow = script_h.readIntValue();

    pstring buf = script_h.readStrValue();
    if (buf) { // Comma may or may not appear in this case.
        menu_font.window_color = readColour(buf);
    }
    else {
        menu_font.window_color.set(0x99);
    }

    return RET_CONTINUE;
}


int ScriptParser::menuselectvoiceCommand(const pstring& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("menuselectvoice: not in the define section");

    for (int i = 0; i < MENUSELECTVOICE_NUM; i++)
        menuselectvoice_file_name[i] = script_h.readStrValue();

    return RET_CONTINUE;
}


int ScriptParser::menuselectcolorCommand(const pstring& cmd)
{
    menu_font.on_color = readColour(script_h.readStrValue());
    menu_font.off_color = readColour(script_h.readStrValue());    
    menu_font.nofile_color = readColour(script_h.readStrValue());

    return RET_CONTINUE;
}


int ScriptParser::maxkaisoupageCommand(const pstring& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("maxkaisoupage: not in the define section");

    max_text_buffer = script_h.readIntValue();

    return RET_CONTINUE;
}


int ScriptParser::lookbackspCommand(const pstring& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("lookbacksp: not in the define section");

    for (int i = 0; i < 2; i++)
        lookback_sp[i] = script_h.readIntValue();

    if (filelog_flag) {
	script_h.file_log.add(DEFAULT_LOOKBACK_NAME0);
	script_h.file_log.add(DEFAULT_LOOKBACK_NAME1);
	script_h.file_log.add(DEFAULT_LOOKBACK_NAME2);
	script_h.file_log.add(DEFAULT_LOOKBACK_NAME3);
    }

    return RET_CONTINUE;
}


int ScriptParser::lookbackcolorCommand(const pstring& cmd)
{
    lookback_color = readColour(script_h.readStrValue());

    return RET_CONTINUE;
}


int ScriptParser::loadgosubCommand(const pstring& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("loadgosub: not in the define section");

    Expression e = script_h.readStrExpr();
    e.require_label();
    loadgosub_label = e.as_string();

    return RET_CONTINUE;
}


int ScriptParser::linepageCommand(const pstring& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("linepage: not in the define section");

    if (cmd == "linepage2") {
        linepage_mode = 2;
        clickstr_line = script_h.readIntValue();
    }
    else
        linepage_mode = 1;

    script_h.setLinepage(true);

    return RET_CONTINUE;
}


int ScriptParser::lenCommand(const pstring& cmd)
{
    Expression e = script_h.readIntExpr();
    e.mutate(script_h.readStrValue().length());
    return RET_CONTINUE;
}


int ScriptParser::labellogCommand(const pstring& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("labellog: not in the define section");

    labellog_flag = true;

    return RET_CONTINUE;
}


int ScriptParser::kidokuskipCommand(const pstring& cmd)
{
    kidokuskip_flag = true;
    kidokumode_flag = true;
    script_h.loadKidokuData();

    return RET_CONTINUE;
}


int ScriptParser::kidokumodeCommand(const pstring& cmd)
{
    kidokumode_flag = script_h.readIntValue() == 1;
    return RET_CONTINUE;
}


int ScriptParser::itoaCommand(const pstring& cmd)
{
    Expression e = script_h.readStrExpr();
    pstring v;
    v.format("%d", script_h.readIntValue());
    if (!script_h.is_ponscripter && cmd == "itoa2") {
	// Handle zenkaku output in compatibility mode
	e.mutate("");
	for (pstrIter it(v); it.get() >= 0; it.next())
	    e.append(it.get() - 0x0030 + 0xff10);
    }
    else
	e.mutate(v);

    return RET_CONTINUE;
}


int ScriptParser::intlimitCommand(const pstring& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("intlimit: not in the define section");

    int no = script_h.readIntValue();

    script_h.variable_data[no].num_limit_flag  = true;
    script_h.variable_data[no].num_limit_lower = script_h.readIntValue();
    script_h.variable_data[no].num_limit_upper = script_h.readIntValue();

    return RET_CONTINUE;
}


int ScriptParser::incCommand(const pstring& cmd)
{
    Expression e = script_h.readIntExpr();
    e.mutate(e.as_int() + 1);
    return RET_CONTINUE;
}


int ScriptParser::ifCommand(const pstring& cmd)
{
    bool condition_flag = true, f = false;
    bool if_flag = cmd != "notif";

    while (1) {
        if (script_h.compareString("fchk")) {
            script_h.readStrExpr();
	    f = script_h.file_log.find(script_h.readStrValue());
        }
        else if (script_h.compareString("lchk")) {
            script_h.readStrExpr();
            f = script_h.label_log.find(script_h.readStrValue());
        }
        else {
	    Expression left = script_h.readExpr();

	    const char* op_buf = script_h.getNext();
	    if ((op_buf[0] == '>' && op_buf[1] == '=') ||
		(op_buf[0] == '<' && op_buf[1] == '=') ||
		(op_buf[0] == '=' && op_buf[1] == '=') ||
		(op_buf[0] == '!' && op_buf[1] == '=') ||
		(op_buf[0] == '<' && op_buf[1] == '>'))
		script_h.setCurrent(op_buf + 2);
	    else if (op_buf[0] == '<' || op_buf[0] == '>' || op_buf[0] == '=')
		script_h.setCurrent(op_buf + 1);

	    Expression right = left.is_textual()
		             ? script_h.readStrExpr()
		             : script_h.readIntExpr();
	    
	    int comp_val = 0;
	    if (left.is_numeric() && right.is_numeric())
		comp_val = left.as_int() < right.as_int() ? -1
		         : left.as_int() > right.as_int() ? 1
		         : 0;
	    else if (left.is_textual() && right.is_textual())
		comp_val = left.as_string().cmp(right.as_string());
	    else
		errorAndExit("comparison operands are different types");
	    
	    if (op_buf[0] == '>' && op_buf[1] == '=') f = comp_val >= 0;
	    else if (op_buf[0] == '<' && op_buf[1] == '=') f = comp_val <= 0;
	    else if (op_buf[0] == '=' && op_buf[1] == '=') f = comp_val == 0;
	    else if (op_buf[0] == '!' && op_buf[1] == '=') f = comp_val != 0;
	    else if (op_buf[0] == '<' && op_buf[1] == '>') f = comp_val != 0;
	    else if (op_buf[0] == '<') f = comp_val < 0;
	    else if (op_buf[0] == '>') f = comp_val > 0;
	    else if (op_buf[0] == '=') f = comp_val == 0;
        }

        condition_flag &= if_flag ? f : !f;

        const char* op_buf = script_h.getNext();
        if (op_buf[0] == '&') {
            while (*op_buf == '&') op_buf++;
            script_h.setCurrent(op_buf);
            continue;
        }

        break;
    }
    ;

    /* Execute command */
    if (condition_flag) return RET_CONTINUE;
    else return RET_SKIP_LINE;
}


int ScriptParser::humanzCommand(const pstring& cmd)
{
    z_order = script_h.readIntValue();

    return RET_CONTINUE;
}


int ScriptParser::gotoCommand(const pstring& cmd)
{
    setCurrentLabel(script_h.readStrValue());

    return RET_CONTINUE;
}


void ScriptParser::gosubReal(const pstring& label, const char* next_script)
{
    nest_infos.push_back(NestInfo(script_h, next_script));
    setCurrentLabel(label);
}

void ScriptParser::gosubDoTextgosub()
{
    nest_infos.push_back(NestInfo(script_h, script_h.getCurrent(),
                                  string_buffer_offset + 1));
    setCurrentLabel(textgosub_label);
}


int ScriptParser::gosubCommand(const pstring& cmd)
{
    pstring buf = script_h.readStrValue();
    gosubReal(buf, script_h.getNext());

    return RET_CONTINUE;
}


int ScriptParser::globalonCommand(const pstring& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("globalon: not in the define section");

    globalon_flag = true;

    return RET_CONTINUE;
}


int ScriptParser::getparamCommand(const pstring& cmd)
{
    if (nest_infos.empty() ||
        (nest_infos.back().nest_mode != NestInfo::LABEL &&
         nest_infos.back().nest_mode != NestInfo::TEXTGOSUB))
        errorAndExit("getparam: not in a subroutine");

    bool more_args;

    do {
	char ptr = script_h.checkPtr();
	Expression e = script_h.readExpr();

        script_h.pushCurrent(nest_infos.back().next_script);

	if (ptr == 'i')
	    e.mutate(script_h.readIntExpr().var_no());
	else if (ptr == 's')
	    e.mutate(script_h.readStrExpr().var_no());
	else if (e.is_numeric())
	    e.mutate(script_h.readIntValue());
	else
	    e.mutate(script_h.readStrValue());

        more_args = script_h.hasMoreArgs();

        nest_infos.back().next_script = script_h.getNext();
        script_h.popCurrent();
    }
    while (more_args);

    return RET_CONTINUE;
}


int ScriptParser::forCommand(const pstring& cmd)
{
    NestInfo ni(script_h.readIntExpr());

    if (!script_h.compareString("="))
        errorAndExit("for: no =");

    script_h.setCurrent(script_h.getNext() + 1);
    ni.var.mutate(script_h.readIntValue());

    if (script_h.readBareword() != "to")
        errorAndExit("for: no `to'");

    ni.to = script_h.readIntValue();

    if (script_h.compareString("step")) {
        script_h.readBareword();
        ni.step = script_h.readIntValue();
    }
    else {
        ni.step = 1;
    }

    break_flag = (ni.step > 0 && ni.var.as_int() > ni.to)
              || (ni.step < 0 && ni.var.as_int() < ni.to);
    
    /* ---------------------------------------- */
    /* Step forward callee's label info */
    ni.next_script = script_h.getNext();

    nest_infos.push_back(ni);
    return RET_CONTINUE;
}


int ScriptParser::filelogCommand(const pstring& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("filelog: not in the define section");

    filelog_flag = true;
    script_h.file_log.read(script_h);

    return RET_CONTINUE;
}


int ScriptParser::effectcutCommand(const pstring& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("effectcut: not in the define section.");

    effect_cut_flag = true;

    return RET_CONTINUE;
}


int ScriptParser::effectblankCommand(const pstring& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("effectblank: not in the define section");

    effect_blank = script_h.readIntValue();

    return RET_CONTINUE;
}


int ScriptParser::effectCommand(const pstring& cmd)
{
    if (cmd == "windoweffect") {
        readEffect(window_effect);
	return RET_CONTINUE;
    }

    if (current_mode != DEFINE_MODE) errorAndExit("effect: not in the define section");

    Effect e;
    e.no = script_h.readIntValue();
    if (e.no < 2 || e.no > 255) errorAndExit("Effect No. is out of range");
    readEffect(e);
    effects.push_back(e);
    
    return RET_CONTINUE;
}


int ScriptParser::divCommand(const pstring& cmd)
{
    Expression e = script_h.readIntExpr();
    e.mutate(e.as_int() / script_h.readIntValue());
    return RET_CONTINUE;
}


int ScriptParser::dimCommand(const pstring& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("dim: not in the define section");
    script_h.declareDim();
    return RET_CONTINUE;
}


int ScriptParser::defvoicevolCommand(const pstring& cmd)
{
    voice_volume = script_h.readIntValue();
    return RET_CONTINUE;
}


int ScriptParser::defsubCommand(const pstring& cmd)
{
    user_func_lut.insert(script_h.readBareword());
    return RET_CONTINUE;
}


int ScriptParser::defsevolCommand(const pstring& cmd)
{
    se_volume = script_h.readIntValue();
    return RET_CONTINUE;
}


int ScriptParser::defmp3volCommand(const pstring& cmd)
{
    music_volume = script_h.readIntValue();
    return RET_CONTINUE;
}


int ScriptParser::defaultspeedCommand(const pstring& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("defaultspeed: not in the define section");
    for (int i = 0; i < 3; i++)
	default_text_speed[i] = script_h.readIntValue();
    return RET_CONTINUE;
}


int ScriptParser::defaultfontCommand(const pstring& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("defaultfont: not in the define section");
    default_env_font = script_h.readStrValue();
    return RET_CONTINUE;
}


int ScriptParser::decCommand(const pstring& cmd)
{
    Expression e = script_h.readIntExpr();
    e.mutate(e.as_int() - 1);
    return RET_CONTINUE;
}


int ScriptParser::dateCommand(const pstring& cmd)
{
    time_t t = time(NULL);
    struct tm* tm = localtime(&t);
    script_h.readIntExpr().mutate(tm->tm_year % 100);
    script_h.readIntExpr().mutate(tm->tm_mon + 1);
    script_h.readIntExpr().mutate(tm->tm_mday);
    return RET_CONTINUE;
}


int ScriptParser::cosCommand(const pstring& cmd)
{
    Expression e = script_h.readIntExpr();
    e.mutate(int(cos(M_PI * script_h.readIntValue() / 180.0) * 1000.0));
    return RET_CONTINUE;
}


int ScriptParser::cmpCommand(const pstring& cmd)
{
    Expression e = script_h.readIntExpr();
    pstring buf1 = script_h.readStrValue();
    pstring buf2 = script_h.readStrValue();
    int val = buf1.cmp(buf2);
    if (val > 0) val = 1;
    else if (val < 0) val = -1;
    e.mutate(val);
    return RET_CONTINUE;
}


int ScriptParser::clickvoiceCommand(const pstring& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("clickvoice: not in the define section");
    for (int i = 0; i < CLICKVOICE_NUM; i++)
        clickvoice_file_name[i] = script_h.readStrValue();
    return RET_CONTINUE;
}


int ScriptParser::clickstrCommand(const pstring& cmd)
{
    script_h.setClickstr(script_h.readStrValue());
    clickstr_line = script_h.readIntValue();
    return RET_CONTINUE;
}


int ScriptParser::breakCommand(const pstring& cmd)
{
    if (nest_infos.empty() || nest_infos.back().nest_mode != NestInfo::FOR)
        errorAndExit("break: not in for loop\n");

    const char* buf = script_h.getNext();
    if (buf[0] == '*') {
        nest_infos.pop_back();
        setCurrentLabel(script_h.readStrValue());
    }
    else {
        break_flag = true;
    }

    return RET_CONTINUE;
}


int ScriptParser::atoiCommand(const pstring& cmd)
{
    Expression e = script_h.readIntExpr();
    e.mutate(atoi(script_h.readStrValue()));
    return RET_CONTINUE;
}


int ScriptParser::arcCommand(const pstring& cmd)
{
    // arc "filename|archive reader DLL"
    // We ignore the DLL, and assume the archive is SAR.
    pstring buf = script_h.readStrValue();
    buf.trunc(buf.find('|', 0)); // TODO: check this removes the |
    if (ScriptHandler::cBR->getArchiveName() == "direct") {
        delete ScriptHandler::cBR;
        ScriptHandler::cBR = new SarReader(archive_path, key_table);
        if (ScriptHandler::cBR->open(buf))
            fprintf(stderr, " *** failed to open archive %s, ignored.  ***\n",
		    (const char*) buf);
    }
    else if (ScriptHandler::cBR->getArchiveName() == "sar") {
        if (ScriptHandler::cBR->open(buf)) {
            fprintf(stderr, " *** failed to open archive %s, ignored.  ***\n",
		    (const char*) buf);
        }
    }
    // skip "arc" commands after "ns?" command
    return RET_CONTINUE;
}


int ScriptParser::addCommand(const pstring& cmd)
{
    Expression e = script_h.readExpr();
    if (!script_h.is_ponscripter && e.is_array())
	errorAndCont("NScripter does not permit `add ?array, val': for "
		     "portability, use `mov ?array,?array + val' instead.");

    if (e.is_numeric())
	e.mutate(e.as_int() + script_h.readIntValue());
    else
	e.append(script_h.readStrValue());

    return RET_CONTINUE;
}

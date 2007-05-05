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

int ScriptParser::zenkakkoCommand(const string& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit(script_h.getStringBuffer(), "not in the define section");

    zenkakko_flag = true;

    return RET_CONTINUE;
}


int ScriptParser::windowbackCommand(const string& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit(script_h.getStringBuffer(), "not in the define section");

    windowback_flag = true;

    return RET_CONTINUE;
}


int ScriptParser::versionstrCommand(const string& cmd)
{
    version_str = script_h.readStrValue() + '\n';
    version_str += script_h.readStrValue() + '\n';
    return RET_CONTINUE;
}


int ScriptParser::usewheelCommand(const string& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("usewheel: not in the define section");

    usewheel_flag = true;

    return RET_CONTINUE;
}


int ScriptParser::useescspcCommand(const string& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("useescspc: not in the define section");

    if (!force_button_shortcut_flag)
        useescspc_flag = true;

    return RET_CONTINUE;
}


int ScriptParser::underlineCommand(const string& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("underline: not in the define section");

    underline_value = script_h.readIntValue() * screen_ratio1 / screen_ratio2;

    return RET_CONTINUE;
}


int ScriptParser::transmodeCommand(const string& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("transmode: not in the define section");

    if (script_h.compareString("leftup"))
	trans_mode = AnimationInfo::TRANS_TOPLEFT;
    else if (script_h.compareString("copy"))
	trans_mode = AnimationInfo::TRANS_COPY;
    else if (script_h.compareString("alpha"))
	trans_mode = AnimationInfo::TRANS_ALPHA;
    else if (script_h.compareString("righttup"))
	trans_mode = AnimationInfo::TRANS_TOPRIGHT;

    script_h.readLabel();

    return RET_CONTINUE;
}


int ScriptParser::timeCommand(const string& cmd)
{
    time_t t = time(NULL);
    struct tm* tm = localtime(&t);

    script_h.readVariable();
    script_h.setInt(&script_h.current_variable, tm->tm_hour);

    script_h.readVariable();
    script_h.setInt(&script_h.current_variable, tm->tm_min);

    script_h.readVariable();
    script_h.setInt(&script_h.current_variable, tm->tm_sec);

    return RET_CONTINUE;
}


int ScriptParser::textgosubCommand(const string& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("textgosub: not in the define section");

    Expression e = script_h.readStrExpr();
    e.require_label();
    textgosub_label = e.as_string();
    script_h.enableTextgosub(true);

    return RET_CONTINUE;
}


int ScriptParser::tanCommand(const string& cmd)
{
    Expression e = script_h.readIntExpr();
    e.mutate(int(tan(M_PI * script_h.readIntValue() / 180.0) * 1000.0));
    return RET_CONTINUE;
}


int ScriptParser::subCommand(const string& cmd)
{
    Expression e = script_h.readIntExpr();
    e.mutate(e.as_int() - script_h.readIntValue());
    return RET_CONTINUE;
}


int ScriptParser::straliasCommand(const string& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("stralias: not in the define section");
    string label = script_h.readLabel();
    script_h.addStrAlias(label, script_h.readStrValue());
    return RET_CONTINUE;
}


int ScriptParser::soundpressplginCommand(const string& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("soundpressplgin: not in the define section");

    string buf = script_h.readStrValue();
    string buf2(buf, 12);

    // only nbzplgin.dll is supported
    buf2.lowercase();
    if (buf2 != "nbzplgin.dll")
        fprintf(stderr, " *** plugin %s is not available, ignored. ***\n", buf.c_str());
    else
	ScriptHandler::cBR->registerCompressionType(buf.c_str() + buf.find('|'),
						   BaseReader::NBZ_COMPRESSION);
    return RET_CONTINUE;
}


int ScriptParser::skipCommand(const string& cmd)
{
    int line = current_label_info.start_line + current_line
	     + script_h.readIntValue();

    char* buf = script_h.getAddressByLine(line);
    current_label_info = script_h.getLabelByAddress(buf);
    current_line = script_h.getLineByAddress(buf);

    script_h.setCurrent(buf);

    return RET_CONTINUE;
}


int ScriptParser::sinCommand(const string& cmd)
{
    Expression e = script_h.readIntExpr();
    e.mutate(int(sin(M_PI * script_h.readIntValue() / 180.0) * 1000.0));
    return RET_CONTINUE;
}


int ScriptParser::shadedistanceCommand(const string& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("shadedistance: not in the define section");

    shade_distance[0] = script_h.readIntValue();
    shade_distance[1] = script_h.readIntValue();

    return RET_CONTINUE;
}


int ScriptParser::selectvoiceCommand(const string& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("selectvoice: not in the define section");

    for (int i = 0; i < SELECTVOICE_NUM; i++)
        selectvoice_file_name[i] = script_h.readStrValue();

    return RET_CONTINUE;
}


int ScriptParser::selectcolorCommand(const string& cmd)
{
    sentence_font.on_color = readColour(script_h.readStrValue());
    sentence_font.off_color = readColour(script_h.readStrValue());

    return RET_CONTINUE;
}


int ScriptParser::savenumberCommand(const string& cmd)
{
    num_save_file = script_h.readIntValue();

    return RET_CONTINUE;
}


int ScriptParser::savenameCommand(const string& cmd)
{
    save_menu_name = script_h.readStrValue();
    load_menu_name = script_h.readStrValue();
    save_item_name = script_h.readStrValue();
    return RET_CONTINUE;
}


int ScriptParser::rubyonCommand(const string& cmd)
{
    // disabled
    char* buf = script_h.getNext();
    if (buf[0] == 0x0a || buf[0] == ':' || buf[0] == ';') { }
    else {
        script_h.readIntValue();
        script_h.readIntValue();

        if (script_h.getEndStatus() & ScriptHandler::END_COMMA) {
            script_h.readStrExpr();
        }
    }

    return RET_CONTINUE;
}


int ScriptParser::rubyoffCommand(const string& cmd)
{
    // disabled
    return RET_CONTINUE;
}


int ScriptParser::roffCommand(const string& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("roff: not in the define section");

    rmode_flag = false;

    return RET_CONTINUE;
}


int ScriptParser::rmenuCommand(const string& cmd)
{
    rmenu.clear();
    rmenu_link_width = 0;

    bool comma_flag = true;
    while (comma_flag) {
	string s = script_h.readStrValue();
	rmenu.push_back(RMenuElt(s, getSystemCallNo(script_h.readLabel())));
        comma_flag = script_h.getEndStatus() & ScriptHandler::END_COMMA;
    }

    return RET_CONTINUE;
}


int ScriptParser::returnCommand(const string& cmd)
{
    if (nest_infos.empty() || nest_infos.back().nest_mode != NestInfo::LABEL)
        errorAndExit("return: not in gosub");

    current_label_info =
	script_h.getLabelByAddress(nest_infos.back().next_script);
    current_line =
	script_h.getLineByAddress(nest_infos.back().next_script);

    char *buf = script_h.getNext();
    if (buf[0] == 0x0a || buf[0] == ':' || buf[0] == ';')
	script_h.setCurrent(nest_infos.back().next_script);
    else
	setCurrentLabel(script_h.readStrValue());

    nest_infos.pop_back();

    return RET_CONTINUE;
}


int ScriptParser::pretextgosubCommand(const string& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("pretextgosub: not in the define section");

    Expression e = script_h.readStrExpr();
    e.require_label();
    pretextgosub_label = e.as_string();

    return RET_CONTINUE;
}


int ScriptParser::numaliasCommand(const string& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("numalias: numalias: not in the define section");

    string label = script_h.readLabel();
    int no = script_h.readIntValue();
    script_h.addNumAlias(label, no);

    return RET_CONTINUE;
}


int ScriptParser::nsadirCommand(const string& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("nsadir: not in the define section");

    nsa_path = script_h.readStrValue() + DELIMITER;
    return RET_CONTINUE;
}


int ScriptParser::nsaCommand(const string& cmd)
{
    int archive_type = NsaReader::ARCHIVE_TYPE_NSA;

    if (cmd == "ns2") {
        archive_type = NsaReader::ARCHIVE_TYPE_NS2;
    }
    else if (cmd == "ns3") {
        archive_type = NsaReader::ARCHIVE_TYPE_NS3;
    }

    delete ScriptHandler::cBR;
    ScriptHandler::cBR = new NsaReader(archive_path.c_str(), key_table);
    if (ScriptHandler::cBR->open(nsa_path.c_str(), archive_type))
        fprintf(stderr, " *** failed to open Nsa archive, ignored.  ***\n");

    return RET_CONTINUE;
}


int ScriptParser::nextCommand(const string& cmd)
{
    if (nest_infos.empty() || nest_infos.back().nest_mode != NestInfo::FOR)
        errorAndExit("next: not in for loop\n");

    int val;
    if (!break_flag) {
        val = script_h.variable_data[nest_infos.back().var_no].num;
        script_h.setNumVariable(nest_infos.back().var_no,
				val + nest_infos.back().step);
    }

    val = script_h.variable_data[nest_infos.back().var_no].num;

    if (break_flag
        || nest_infos.back().step > 0 && val > nest_infos.back().to
        || nest_infos.back().step < 0 && val < nest_infos.back().to) {
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


int ScriptParser::mulCommand(const string& cmd)
{
    Expression e = script_h.readIntExpr();
    e.mutate(e.as_int() * script_h.readIntValue());
    return RET_CONTINUE;
}


int ScriptParser::movCommand(const string& cmd)
{
    int limit = 1;
    if (cmd == "mov10")
        limit = 10;
    else if (cmd == "movl")
        limit = MAX_INT;
    else if (cmd[3] >= '3' && cmd[3] <= '9')
        limit = cmd[3] - '0';

    Expression e = script_h.readExpr();
    if (e.is_textual()) {
	if (limit != 1)
	    errorAndExit(cmd + " is not valid with string variables (use mov)");
	e.mutate(script_h.readStrValue());
    }
    else {
        bool loop_flag = script_h.getEndStatus() & ScriptHandler::END_COMMA;
	for (int i = 0; loop_flag && i < limit; ++i) {
	    int no = script_h.readIntValue();
	    loop_flag = script_h.getEndStatus() & ScriptHandler::END_COMMA;
	    e.mutate(no, i, limit == MAX_INT);
	}
    }
    return RET_CONTINUE;
}


int ScriptParser::mode_sayaCommand(const string& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("mode_saya: not in the define section");

    mode_saya_flag = true;

    return RET_CONTINUE;
}


int ScriptParser::mode_extCommand(const string& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("mode_ext: not in the define section");

    mode_ext_flag = true;

    return RET_CONTINUE;
}


int ScriptParser::modCommand(const string& cmd)
{
    Expression e = script_h.readIntExpr();
    e.mutate(e.as_int() % script_h.readIntValue());
    return RET_CONTINUE;
}


int ScriptParser::midCommand(const string& cmd)
{
    Expression e = script_h.readStrExpr();
    string src = script_h.readStrValue();
    unsigned int start = script_h.readIntValue();
    unsigned int len = script_h.readIntValue();
    if (start >= src.size()) {
        e.mutate("");
    }
    else {
        if (start + len > src.size()) len = src.size() - start;
	e.mutate(src.substr(start, len));
    }
    return RET_CONTINUE;
}


int ScriptParser::menusetwindowCommand(const string& cmd)
{
    int s1 = script_h.readIntValue();
    int s2 = script_h.readIntValue();
    menu_font.set_size(s1 > s2 ? s1 : s2);
    menu_font.set_mod_size(0);
    menu_font.pitch_x   = script_h.readIntValue();
    menu_font.pitch_y   = script_h.readIntValue();
    menu_font.is_bold   = script_h.readIntValue();
    menu_font.is_shadow = script_h.readIntValue();

    string buf = script_h.readStrValue();
    if (buf) { // Comma may or may not appear in this case.
        menu_font.window_color = readColour(buf);
    }
    else {
        menu_font.window_color.set(0x99);
    }

    return RET_CONTINUE;
}


int ScriptParser::menuselectvoiceCommand(const string& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("menuselectvoice: not in the define section");

    for (int i = 0; i < MENUSELECTVOICE_NUM; i++)
        menuselectvoice_file_name[i] = script_h.readStrValue();

    return RET_CONTINUE;
}


int ScriptParser::menuselectcolorCommand(const string& cmd)
{
    menu_font.on_color = readColour(script_h.readStrValue());
    menu_font.off_color = readColour(script_h.readStrValue());    
    menu_font.nofile_color = readColour(script_h.readStrValue());

    return RET_CONTINUE;
}


int ScriptParser::maxkaisoupageCommand(const string& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("maxkaisoupage: not in the define section");

    max_text_buffer = script_h.readIntValue();

    return RET_CONTINUE;
}


int ScriptParser::lookbackspCommand(const string& cmd)
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


int ScriptParser::lookbackcolorCommand(const string& cmd)
{
    lookback_color = readColour(script_h.readStrValue());

    return RET_CONTINUE;
}


int ScriptParser::loadgosubCommand(const string& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("loadgosub: not in the define section");

    Expression e = script_h.readStrExpr();
    e.require_label();
    loadgosub_label = e.as_string();

    return RET_CONTINUE;
}


int ScriptParser::linepageCommand(const string& cmd)
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


int ScriptParser::lenCommand(const string& cmd)
{
    Expression e = script_h.readIntExpr();
    e.mutate(script_h.readStrValue().size());
    return RET_CONTINUE;
}


int ScriptParser::labellogCommand(const string& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("labellog: not in the define section");

    labellog_flag = true;

    return RET_CONTINUE;
}


int ScriptParser::kidokuskipCommand(const string& cmd)
{
    kidokuskip_flag = true;
    kidokumode_flag = true;
    script_h.loadKidokuData();

    return RET_CONTINUE;
}


int ScriptParser::kidokumodeCommand(const string& cmd)
{
    kidokumode_flag = script_h.readIntValue() == 1;
    return RET_CONTINUE;
}


int ScriptParser::itoaCommand(const string& cmd)
{
    Expression e = script_h.readStrExpr();
    e.mutate(str(script_h.readIntValue()));
    if (cmd == "itoa2") {
	printf("itoa2: zenkaku conversion not implemented (returning hankaku moji)\n");
	// TODO: zenkaku-fy string
    }

    return RET_CONTINUE;
}


int ScriptParser::intlimitCommand(const string& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("intlimit: not in the define section");

    int no = script_h.readIntValue();

    script_h.variable_data[no].num_limit_flag  = true;
    script_h.variable_data[no].num_limit_lower = script_h.readIntValue();
    script_h.variable_data[no].num_limit_upper = script_h.readIntValue();

    return RET_CONTINUE;
}


int ScriptParser::incCommand(const string& cmd)
{
    Expression e = script_h.readIntExpr();
    e.mutate(e.as_int() + 1);
    return RET_CONTINUE;
}


int ScriptParser::ifCommand(const string& cmd)
{
    //printf("ifCommand\n");
    bool condition_flag = true, f = false;
    bool if_flag = cmd != "notif";

    while (1) {
        if (script_h.compareString("fchk")) {
            script_h.readLabel();
	    f = script_h.file_log.find(script_h.readStrValue());
        }
        else if (script_h.compareString("lchk")) {
            script_h.readLabel();
            f = script_h.label_log.find(script_h.readStrValue());
        }
        else {
	    Expression left = script_h.readExpr();

	    char* op_buf = script_h.getNext();
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
		comp_val = left.as_string().compare(right.as_string());
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

        char* op_buf = script_h.getNext();
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


int ScriptParser::humanzCommand(const string& cmd)
{
    z_order = script_h.readIntValue();

    return RET_CONTINUE;
}


int ScriptParser::gotoCommand(const string& cmd)
{
    setCurrentLabel(script_h.readStrValue());

    return RET_CONTINUE;
}


void ScriptParser::gosubReal(const string& label, char* next_script)
{
    nest_infos.push_back(NestInfo(next_script));
    setCurrentLabel(label);
}


int ScriptParser::gosubCommand(const string& cmd)
{
    string buf = script_h.readStrValue();
    gosubReal(buf, script_h.getNext());

    return RET_CONTINUE;
}


int ScriptParser::globalonCommand(const string& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("globalon: not in the define section");

    globalon_flag = true;

    return RET_CONTINUE;
}


int ScriptParser::getparamCommand(const string& cmd)
{
    if (nest_infos.empty() || nest_infos.back().nest_mode != NestInfo::LABEL)
        errorAndExit("getparam: not in a subroutine");

    int end_status;

    do {
	bool ptr = script_h.checkPtr();
	Expression e = script_h.readExpr();

        script_h.pushCurrent(nest_infos.back().next_script);

	if (ptr)
	    e.mutate(script_h.readExpr().var_no());
	else if (e.is_numeric())
	    e.mutate(script_h.readIntValue());
	else
	    e.mutate(script_h.readStrValue());

        end_status = script_h.getEndStatus();

        nest_infos.back().next_script = script_h.getNext();
        script_h.popCurrent();
    }
    while (end_status & ScriptHandler::END_COMMA);

    return RET_CONTINUE;
}


int ScriptParser::forCommand(const string& cmd)
{
    NestInfo ni;
    ni.nest_mode = NestInfo::FOR;

    Expression e = script_h.readIntExpr();
    ni.var_no = e.var_no();
    if (ni.var_no < 0 || ni.var_no >= VARIABLE_RANGE)
        ni.var_no = VARIABLE_RANGE;

//    script_h.pushVariable();

    if (!script_h.compareString("="))
        errorAndExit("for: no =");

    script_h.setCurrent(script_h.getNext() + 1);
    int from = script_h.readIntValue();
    e.mutate(from);

    if (!script_h.compareString("to"))
        errorAndExit("for: no to");

    script_h.readLabel();

    ni.to = script_h.readIntValue();

    if (script_h.compareString("step")) {
        script_h.readLabel();
        ni.step = script_h.readIntValue();
    }
    else {
        ni.step = 1;
    }

    break_flag = ni.step > 0 && from > ni.to ||
		 ni.step < 0 && from < ni.to;
    
    /* ---------------------------------------- */
    /* Step forward callee's label info */
    ni.next_script = script_h.getNext();

    nest_infos.push_back(ni);
    return RET_CONTINUE;
}


int ScriptParser::filelogCommand(const string& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("filelog: not in the define section");

    filelog_flag = true;
    script_h.file_log.read(script_h);

    return RET_CONTINUE;
}


int ScriptParser::effectcutCommand(const string& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("effectcut: not in the define section.");

    effect_cut_flag = true;

    return RET_CONTINUE;
}


int ScriptParser::effectblankCommand(const string& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("effectblank: not in the define section");

    effect_blank = script_h.readIntValue();

    return RET_CONTINUE;
}


int ScriptParser::effectCommand(const string& cmd)
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


int ScriptParser::divCommand(const string& cmd)
{
    Expression e = script_h.readIntExpr();
    e.mutate(e.as_int() / script_h.readIntValue());
    return RET_CONTINUE;
}


int ScriptParser::dimCommand(const string& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("dim: not in the define section");
    script_h.declareDim();
    return RET_CONTINUE;
}


int ScriptParser::defvoicevolCommand(const string& cmd)
{
    voice_volume = script_h.readIntValue();
    return RET_CONTINUE;
}


int ScriptParser::defsubCommand(const string& cmd)
{
    user_func_lut.insert(script_h.readLabel());
    return RET_CONTINUE;
}


int ScriptParser::defsevolCommand(const string& cmd)
{
    se_volume = script_h.readIntValue();
    return RET_CONTINUE;
}


int ScriptParser::defmp3volCommand(const string& cmd)
{
    music_volume = script_h.readIntValue();
    return RET_CONTINUE;
}


int ScriptParser::defaultspeedCommand(const string& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("defaultspeed: not in the define section");
    for (int i = 0; i < 3; i++)
	default_text_speed[i] = script_h.readIntValue();
    return RET_CONTINUE;
}


int ScriptParser::defaultfontCommand(const string& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("defaultfont: not in the define section");
    default_env_font = script_h.readStrValue();
    return RET_CONTINUE;
}


int ScriptParser::decCommand(const string& cmd)
{
    Expression e = script_h.readIntExpr();
    e.mutate(e.as_int() - 1);
    return RET_CONTINUE;
}


int ScriptParser::dateCommand(const string& cmd)
{
    time_t t = time(NULL);
    struct tm* tm = localtime(&t);
    script_h.readIntExpr().mutate(tm->tm_year % 100);
    script_h.readIntExpr().mutate(tm->tm_mon + 1);
    script_h.readIntExpr().mutate(tm->tm_mday);
    return RET_CONTINUE;
}


int ScriptParser::cosCommand(const string& cmd)
{
    Expression e = script_h.readIntExpr();
    e.mutate(int(cos(M_PI * script_h.readIntValue() / 180.0) * 1000.0));
    return RET_CONTINUE;
}


int ScriptParser::cmpCommand(const string& cmd)
{
    Expression e = script_h.readIntExpr();
    string buf1 = script_h.readStrValue();
    string buf2 = script_h.readStrValue();
    int val = buf1.compare(buf2);
    if (val > 0) val = 1;
    else if (val < 0) val = -1;
    e.mutate(val);
    return RET_CONTINUE;
}


int ScriptParser::clickvoiceCommand(const string& cmd)
{
    if (current_mode != DEFINE_MODE)
	errorAndExit("clickvoice: not in the define section");
    for (int i = 0; i < CLICKVOICE_NUM; i++)
        clickvoice_file_name[i] = script_h.readStrValue();
    return RET_CONTINUE;
}


int ScriptParser::clickstrCommand(const string& cmd)
{
    script_h.setClickstr(script_h.readStrValue());
    clickstr_line = script_h.readIntValue();
    return RET_CONTINUE;
}


int ScriptParser::breakCommand(const string& cmd)
{
    if (nest_infos.empty() || nest_infos.back().nest_mode != NestInfo::FOR)
        errorAndExit("break: not in for loop\n");

    char* buf = script_h.getNext();
    if (buf[0] == '*') {
        nest_infos.pop_back();
        setCurrentLabel(script_h.readStrValue());
    }
    else {
        break_flag = true;
    }

    return RET_CONTINUE;
}


int ScriptParser::atoiCommand(const string& cmd)
{
    Expression e = script_h.readIntExpr();
    e.mutate(atoi(script_h.readStrValue().c_str()));
    return RET_CONTINUE;
}


int ScriptParser::arcCommand(const string& cmd)
{
    // arc "filename|archive reader DLL"
    // We ignore the DLL, and assume the archive is SAR.
    string buf = script_h.readStrValue();
    buf.erase(buf.find('|', 0));
    if (strcmp(ScriptHandler::cBR->getArchiveName(), "direct") == 0) {
        delete ScriptHandler::cBR;
        ScriptHandler::cBR = new SarReader(archive_path.c_str(), key_table);
        if (ScriptHandler::cBR->open(buf.c_str()))
            fprintf(stderr, " *** failed to open archive %s, ignored.  ***\n",
		    buf.c_str());
    }
    else if (strcmp(ScriptHandler::cBR->getArchiveName(), "sar") == 0) {
        if (ScriptHandler::cBR->open(buf.c_str())) {
            fprintf(stderr, " *** failed to open archive %s, ignored.  ***\n",
		    buf.c_str());
        }
    }
    // skip "arc" commands after "ns?" command
    return RET_CONTINUE;
}


int ScriptParser::addCommand(const string& cmd)
{
    Expression e = script_h.readExpr();
    if (e.is_numeric())
	e.mutate(e.as_int() + script_h.readIntValue());
    else
	e.append(script_h.readStrValue());
    return RET_CONTINUE;
}

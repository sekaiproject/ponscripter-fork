#include "ScriptHandler.h"

pstring typestr(Expression::type_t t, bool v = false)
{
    pstring var = v ? " variable" : "";
    switch (t) {
    case Expression::Int:	return "integer" + var;
    case Expression::Array:	return "integer array" + var;
    case Expression::String:	return "string" + var;
    case Expression::Label:	return "label";
    case Expression::Bareword:	return "bareword";
    }
    return "[invalid type]";
}

void Expression::die(const char* why) const
{
    int line = h.getLineByAddress(h.getCurrent(), true);
    fprintf(stderr, "Parse error at line %d: %s\n", line, why);
    exit(-1);
}

pstring Expression::debug_string() const
{
    pstring rv;
    if (var_) {
	switch (type_) {
	case Int:
	    rv.format("%%%d", intval_);
	    break;
	case String:
	    rv.format("$%d", intval_);
	    break;
	case Array:
	    rv.format("?%d", intval_);
	    for (h_index_t::const_iterator it = index_.begin();
		 it != index_.end(); ++it)
		rv.formata("[%d]", *it);
	    break;
	default:
	    rv = "[invalid type]";
	}
    }
    else {
	switch (type_) {
	case Int:
	    rv.format("%d", intval_);
	    break;
	case String:
	    rv.format("\"%s\"", (const char*) strval_);
	    break;
	case Label:
	    rv.format("*%s", (const char*) strval_);
	    break;
	case Bareword:
	    rv = strval_;
	    break;
	default:
	    rv = "[invalid type]";
	}
    }
    return rv;
}

bool Expression::is_bareword(pstring what) const
{
    if (type_ != Bareword) return false;
    return what.caselessEqual(strval_);
}

void Expression::require(type_t t) const
{
    if (type_ != t)
	die("expected " + typestr(t) + ", found " + typestr(type_, var_));
}

void Expression::require(type_t t, bool var) const
{
    if (type_ != t || var_ != var)
	die("expected " + typestr(t, var) + ", found " + typestr(type_, var_));
}

void Expression::require_variable() const
{
    if (!var_)
	die("expected variable, found " + typestr(type_, var_));
}

void Expression::require_textual() const
{
    if (!is_textual())
	die("expected text, found " + typestr(type_, var_));
}

void Expression::require_numeric() const
{
    if (!is_numeric())
	die("expected number, found " + typestr(type_, var_));
}

pstring Expression::as_string() const
{
    if (is_textual() && is_constant())
	return strval_;
    else if (is_textual())
	return h.getVariableData(intval_).str;
    else if (is_numeric()) {
	pstring rv;
	rv.format("%d", as_int());
	return rv;
    }
    throw "Error: invalid expression type";
}

int Expression::as_int() const
{
    if (is_numeric() && is_constant())
	return intval_;
    else if (type_ == Array)
	return h.arrays.find(intval_)->second.getValue(index_);
    else if (type_ == Int)
	return h.getVariableData(intval_).get_num();
    else if (is_textual())
	return as_string();
    throw "Error: invalid expression type";
}

int Expression::var_no() const
{
    require_variable();
    return intval_;
}

int Expression::dim() const
{
    require(Array, true);
    return h.arrays.find(intval_)->second.dimension_size(index_.size());
}

void Expression::mutate(int newval, int offset, bool as_array)
{
    if (type_ == Int) {
	if (as_array) require(Array, true); else require_variable();
	if (offset == MAX_INT) offset = 0;
	h.setNumVariable(intval_ + offset, newval);
    }
    else if (type_ == Array) {
	require_variable();
	h_index_t i = index_;
	if (offset != MAX_INT) {
	    if (as_array)
		i.push_back(offset);
	    else
		i.back() += offset;
        }
	h.arrays.find(intval_)->second.setValue(i, newval);
    }
    else
	require(Int, true);
}

void Expression::mutate(const pstring& newval)
{
    require(String, true);
    h.getVariableData(intval_).str = newval;
}

void Expression::append(const pstring& newval)
{
    require(String, true);
    h.getVariableData(intval_).str += newval;
}

void Expression::append(wchar newval)
{
    require(String, true);
    h.getVariableData(intval_).str += file_encoding->Encode(newval);
}

Expression::Expression(ScriptHandler& sh)
    : h(sh), type_(Int), var_(false), strval_(""), intval_(0)
{}

Expression::Expression(ScriptHandler& sh, type_t t, bool is_v, int val)
    : h(sh), type_(t), var_(is_v), strval_(""), intval_(val)
{}

Expression::Expression(ScriptHandler& sh, type_t t, bool is_v, int val,
                       const h_index_t& idx)
    : h(sh), type_(t), var_(is_v), index_(idx), strval_(""), intval_(val)
{}

Expression::Expression(ScriptHandler& sh, type_t t, bool is_v,
                       const pstring& val)
    : h(sh), type_(t), var_(is_v), strval_(val), intval_(0)
{}

Expression& Expression::operator=(const Expression& src)
{
    if (&src == this) return *this;
    if (&src.h != &h)
	fprintf(stderr, "Warning: we have more than one ScriptHandler...\n");
    type_ = src.type_;
    var_ = src.var_;
    index_ = src.index_;
    strval_ = src.strval_;
    intval_ = src.intval_;
    return *this;
}


// ScriptHandler expression handling

Expression ScriptHandler::readStrExpr(bool trace)
{
    // Currently this is a sane wrapper around the existing unsafe
    // plumbing.  It can be replaced with a safe implementation once
    // everything is going through a safe interface like this.
    pstring s = readStr();
    if (current_variable.type == VAR_STR)
	return Expression(*this, Expression::String, 1,
			  current_variable.var_no);
    else if (current_variable.type & VAR_LABEL) {
	s.remove(0, 1);
	return Expression(*this, Expression::Label, 0, s);
    }
    else if (current_variable.type == VAR_NONE)
	return Expression(*this, Expression::Bareword, 0, s);
    else
	return Expression(*this, Expression::String, 0, s);
}

Expression ScriptHandler::readIntExpr()
{
    // Currently this is a sane wrapper around the existing unsafe
    // plumbing.  It can be replaced with a safe implementation once
    // everything is going through a safe interface like this.
    int i = readInt();
    if (current_variable.type == VAR_INT)
	return Expression(*this, Expression::Int, 1, current_variable.var_no);
    else if (current_variable.type == VAR_ARRAY)
	return Expression(*this, Expression::Array, 1, current_variable.var_no,
			  current_variable.array);
    else
	return Expression(*this, Expression::Int, 0, i);
}

Expression ScriptHandler::readExpr()
{
    // Currently this is based on the existing unsafe plumbing.  It
    // can be replaced with a safe implementation once everything is
    // going through a safe interface like this.
    const char* buf = next_script;
    while (*buf == ' ' || *buf == '\t' || *buf == 0x0a || *buf == '(') ++buf;
    Expression e =
	(*buf != '%' && *buf != '?' && (*buf < '0' || *buf > '9') &&
	 *buf != '-' && *buf != '+')
	? readStrExpr()
	: readIntExpr();
    if (e.type() == Expression::Bareword) {
	numalias_t::iterator a = num_aliases.find(e.as_string());
	if (a != num_aliases.end()) {
	    return Expression(*this, Expression::Int, 0, a->second);
	}
	stralias_t::iterator b = str_aliases.find(e.as_string());
	if (b != str_aliases.end()) {
	    return Expression(*this, Expression::String, 0, a->second);
	}
    }
    return e;
}

pstring ScriptHandler::readStrValue()
{
    return readStrExpr().as_string();
}

pstring ScriptHandler::readBareword()
{
    Expression e = readStrExpr();
    e.require(Expression::Bareword, false);
    return e.as_string();
}

int ScriptHandler::readIntValue()
{
    return readIntExpr().as_int();
}

char ScriptHandler::checkPtr()
{
    const char* buf = current_script = next_script;
    while (*buf == ' ' || *buf == '\t') ++buf;
    if (*buf == 'i' || *buf == 's') {
        next_script = buf + 1;
        return *buf;
    }
    return 0;
}

bool ScriptHandler::hasMoreArgs()
{
    // TODO: replace with a lookahead (and have the readers chomp
    // preceding commas)
    return end_status & END_COMMA;
}

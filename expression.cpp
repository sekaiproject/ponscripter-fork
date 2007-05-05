#include "ScriptHandler.h"

string typestr(Expression::type_t t, bool v = false)
{
    string var = v ? " variable" : "";
    switch (t) {
    case Expression::Int:	return "integer" + var;
    case Expression::Array:	return "integer array" + var;
    case Expression::String:	return "string" + var;
    case Expression::Label:	return "label";
    case Expression::Bareword:	return "bareword";
    }
    return "[invalid type]";
}

void Expression::die(string why) const
{
    int line = h.getLineByAddress(h.getCurrent(), true);
    why = "Parse error at line " + str(line) + ": " + why + "\n";
    fprintf(stderr, why.c_str());
    exit(-1);
}

string Expression::debug_string() const
{
    if (var_) {
	switch (type_) {
	case Int:    return "%" + str(value_.intval);
	case String: return "$" + str(value_.intval);
	case Array:
	  { string rv = "?" + str(value_.intval);
	    for (std::vector<int>::const_iterator it = index_.begin();
		 it != index_.end(); ++it)
		rv += "[" + str(*it) + "]";
	    return rv; }
	default: return "[invalid type]";
	}
    }
    else {
	switch (type_) {
	case Int:      return str(value_.intval);
	case String:   return "\"" + *value_.strptr + "\"";
	case Label:    return "*" + *value_.strptr;
	case Bareword: return *value_.strptr;
	default: return "[invalid type]";	    
	}
    }
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

string Expression::as_string() const
{
    if (is_textual() && is_constant())
	return *value_.strptr;
    else if (is_textual())
	return h.variable_data[value_.intval].str;
    else if (is_numeric())
	return str(as_int());
    throw "Error: invalid expression type";
}

int Expression::as_int() const
{
    if (is_numeric() && is_constant())
	return value_.intval;
    else if (type_ == Array)
	return h.arrays.find(value_.intval)->second.getValue(index_);
    else if (type_ == Int)
	return h.variable_data[value_.intval].num;
    else if (is_textual())
	return atoi(as_string().c_str());
    throw "Error: invalid expression type";
}

int Expression::var_no() const
{
    require_variable();
    return value_.intval;
}

int Expression::dim() const
{
    require(Array, true);
    return h.arrays.find(value_.intval)->second.getDimensionSize(index_.size());
}

void Expression::mutate(int newval, int offset, bool as_array)
{
    if (type_ == Int) {
	if (as_array) require(Array, true); else require_variable();
	if (offset == MAX_INT) offset = 0;
	h.setNumVariable(value_.intval + offset, newval);
    }
    else if (type_ == Array) {
	require_variable();
	std::vector<int> i = index_;
	if (offset != MAX_INT)
	    if (as_array)
		i.push_back(offset);
	    else
		i.back() += offset;
	h.arrays.find(value_.intval)->second.setValue(i, newval);
    }
    else
	require(Int, true);
}

void Expression::mutate(const string& newval)
{
    require(String, true);
    h.variable_data[value_.intval].str = newval;
}

void Expression::append(const string& newval)
{
    require(String, true);
    h.variable_data[value_.intval].str += newval;
}

void Expression::append(wchar newval)
{
    require(String, true);
    h.variable_data[value_.intval].str += newval;
}

Expression::~Expression()
{
    if (is_textual() && is_constant()) delete value_.strptr;
}

Expression::Expression(ScriptHandler& sh, type_t t, bool is_v, int val)
    : h(sh), type_(t), var_(is_v)
{
    value_.intval = val;
}

Expression::Expression(ScriptHandler& sh, type_t t, bool is_v, int val,
		       const std::vector<int>& idx)
    : h(sh), type_(t), var_(is_v), index_(idx)
{
    value_.intval = val;
}

Expression::Expression(ScriptHandler& sh, type_t t, bool is_v,
		       const string& val)
    : h(sh), type_(t), var_(is_v)
{
    value_.strptr = new string(val);
}

Expression ScriptHandler::readStrExpr()
{
    // Currently this is a sane wrapper around the existing unsafe
    // plumbing.  It can be replaced with a safe implementation once
    // everything is going through a safe interface like this.
    string s = readStr();
    if (current_variable.type == VAR_STR)
	return Expression(*this, Expression::String, 1,current_variable.var_no);
    else if (current_variable.type & VAR_LABEL) {
	s.shift();
	return Expression(*this, Expression::Label, 0, s);
    }
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
			  std::vector<int>(current_variable.array.dim,
					   current_variable.array.dim +
					   current_variable.array.num_dim));
    else
	return Expression(*this, Expression::Int, 0, i);
}

Expression ScriptHandler::readExpr()
{
    // Currently this is based on the existing unsafe plumbing.  It
    // can be replaced with a safe implementation once everything is
    // going through a safe interface like this.
    char* buf = next_script;
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

string ScriptHandler::readStrValue()
{
    return readStrExpr().as_string();
}

int ScriptHandler::readIntValue()
{
    return readIntExpr().as_int();
}

int ScriptHandler::readStrVar()
{
    Expression e = readStrExpr();
    e.require(Expression::String, true);
    return e.var_no();
}

int ScriptHandler::readIntVar()
{
    Expression e = readIntExpr();
    e.require(Expression::Int, true);
    return e.var_no();
}

char ScriptHandler::checkPtr()
{
    char* buf = current_script = next_script;
    while (*buf == ' ' || *buf == '\t') ++buf;
    if (*buf == 'i' || *buf == 's') {
        next_script = buf + 1;
        return *buf;
    }
    return 0;
}

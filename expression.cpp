#include "ScriptHandler.h"

// FIXME: this currently DOES NOT HANDLE ARRAYS in any way, shape, or form!

string typestr(Expression::type_t t, bool v = false)
{
    string var = v ? " variable" : "";
    switch (t) {
    case Expression::Int:
	return "integer" + var;
    case Expression::String:
	return "string" + var;
    case Expression::Label:
	return "label";
    case Expression::Bareword:
	return "bareword";
    default:
	return "[invalid type]";
    }
}

void Expression::die(string why) const
{
    int line = h.getLineByAddress(h.getCurrent(), true);
    why = "Parse error at line " + str(line) + ": " + why + "\n";
    fprintf(stderr, why.c_str());
    exit(-1);
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

void Expression::require_number() const
{
    if (!is_number())
	die("expected number, found " + typestr(type_, var_));
}

string Expression::as_string() const
{
    if (is_textual() && is_constant())
	return *value_.strptr;
    else if (is_textual())
	return h.variable_data[value_.intval].str;
    else if (is_number())
	return str(as_int());
    throw "Error: invalid expression type";
}

int Expression::as_int() const
{
    if (is_number() && is_constant())
	return value_.intval;
    else if (is_number())
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

void Expression::mutate(int newval)
{
    require(Int, true);
    h.setNumVariable(value_.intval, newval);
}

void Expression::mutate(const string& newval)
{
    require(String, true);
    h.variable_data[value_.intval].str = newval;
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
    if (*buf != '%' && *buf != '?' && (*buf < '0' || *buf > '9') &&
	*buf != '-' && *buf != '+')
	return readStrExpr();
    else
	return readIntExpr();
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

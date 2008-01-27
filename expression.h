// -*- C++ -*-
// System intended to replace the need for internal global state.

#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "defs.h"

class ScriptHandler;

class Expression {
public:
    enum type_t { Int, Array, String, Label, Bareword };
    type_t type() const { return type_; }

    // For debugging use
    pstring debug_string() const; // returns representation such as "?25[0]"
    
    // Test for attributes
    bool is_variable() const { return var_; }
    bool is_constant() const { return !var_; }
    bool is_textual() const
	{ return type_ == String || type_ == Label || type_ == Bareword; }
    bool is_numeric() const
	{ return type_ == Int || type_ == Array; }
    bool is_array() const { return type_ == Array; }
    bool is_label() const { return type_ == Label; }
    bool is_bareword() const { return type_ == Bareword; }
    bool is_bareword(pstring s) const;

    // Fail if attributes are missing
    void require(type_t t) const;
    void require(type_t t, bool var) const;
    void require_variable() const;
    void require_textual() const;
    void require_numeric() const;
    void require_label()    const { require(Label); }
    void require_bareword() const { require(Bareword); }

    // Access contents
    pstring as_string() const; // coerced if necessary
    int as_int() const; // coerced if necessary
    int var_no() const; // if variable
    int dim() const; // if array: returns size of indexed dimension
    
    // Modify variable
    void mutate(int newval, int offset = MAX_INT, bool as_array = false);
    void mutate(const pstring& newval);
    void append(const pstring& newval);
    void append(wchar newval);    
    
    Expression(ScriptHandler& sh);
    Expression(ScriptHandler& sh, type_t t, bool is_v, int val);
    Expression(ScriptHandler& sh, type_t t, bool is_v, int val,
	       const h_index_t& idx);
    Expression(ScriptHandler& sh, type_t t, bool is_v, const pstring& val);

    Expression& operator=(const Expression& src);    
private:
    void die(const char* why) const;
    ScriptHandler& h;
    type_t type_;
    bool var_;
    h_index_t index_;
    pstring strval_;
    int intval_;
};

#endif

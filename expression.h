// -*- C++ -*-
// System intended to replace the need for internal global state.

#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "defs.h"

class ScriptHandler;

class Expression {
public:
    enum type_t { Int, String, Label, Bareword };
    type_t type() const { return type_; }

    // Test for attributes
    bool is_variable() const { return var_; }
    bool is_constant() const { return !var_; }
    bool is_textual() const
	{ return type_ == String || type_ == Label || type_ == Bareword; }
    bool is_number() const
	{ return type_ == Int; }

    // Fail if attributes are missing
    void require(type_t t) const;
    void require(type_t t, bool var) const;
    void require_variable() const;
    void require_textual() const;
    void require_number() const;

    void require_int()      const { require(Int); }
    void require_string()   const { require(String); }
    void require_label()    const { require(Label); }
    void require_bareword() const { require(Bareword); }

    // Access contents
    string as_string() const; // coerced if necessary
    int as_int() const; // coerced if necessary
    int var_no() const; // if variable

    // Modify variable
    void mutate(int newval);
    void mutate(const string& newval);
    
    ~Expression();
    Expression(ScriptHandler& sh, type_t t, bool is_v, int val);
    Expression(ScriptHandler& sh, type_t t, bool is_v, const string& val);

private:
    void die(string why) const;
    ScriptHandler& h;
    type_t type_;
    bool var_;
    union {
	string* strptr;
	int intval;
    } value_;
};

#endif

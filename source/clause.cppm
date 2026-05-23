// Copyright (C) 2026  ilobilo

export module clause;

import ast;
import format;
import std;

export namespace resol
{
    struct literal
    {
        bool negated;
        std::string name;
        std::vector<term::ptr> args;
    };

    using clause = std::vector<literal>;
    using clause_set = std::vector<clause>;

    clause_set extract_clauses(formula::ptr formula);

    bool equal(const literal &lhs, const literal &rhs);

    std::string to_string(const literal &lit, print_style style = print_style::unicode);
    std::string to_string(const clause &cl, print_style style = print_style::unicode);
} // export namespace resol

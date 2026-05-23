// Copyright (C) 2026  ilobilo

export module clause;

import ast;
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
    bool is_tautology(const clause &cl);
} // export namespace resol

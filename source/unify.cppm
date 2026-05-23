// Copyright (C) 2026  ilobilo

export module unify;

import ast;
import std;

export namespace resol
{
    using substitution = std::unordered_map<std::string, term::ptr>;

    struct unify_error { std::string message; };

    template<typename Type>
    using unify_result = std::expected<Type, unify_error>;

    term::ptr apply(const substitution &sub, term::ptr term);

    unify_result<substitution> unify(term::ptr lhs, term::ptr rhs);
    unify_result<substitution> unify(formula::ptr lhs, formula::ptr rhs);
} // export namespace resol

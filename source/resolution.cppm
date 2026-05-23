// Copyright (C) 2026  ilobilo

export module resolution;

import ast;
import clause;
import unify;
import std;

export namespace resol
{
    enum class prove_status
    {
        unsatisfiable,
        satisfiable,
        limit_reached
    };

    struct prove_step
    {
        std::vector<std::size_t> parents;
        substitution mgu;
        clause result;
    };

    struct prove_result
    {
        prove_status status;
        std::vector<prove_step> steps;
        std::size_t iterations;
    };

    prove_result run_resolution(clause_set inputs, std::size_t limit);
} // export namespace resol

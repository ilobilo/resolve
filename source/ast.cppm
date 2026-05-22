// Copyright (C) 2026  ilobilo

export module ast;

import util;
import std;

export namespace resol
{
    using prec_t = int;
    constexpr prec_t not_p = 5;
    constexpr prec_t and_p = 4;
    constexpr prec_t or_p = 3;
    constexpr prec_t implies_p = 2;
    constexpr prec_t iff_p = 1;
    constexpr prec_t quant_p = 1;

    struct term_t;
    namespace term
    {
        using ptr = std::shared_ptr<const term_t>;

        struct variable_t { std::string name; };
        struct constant_t { std::string name; };
        struct function_t { std::string name; std::vector<ptr> args; };
    } // namespace term

    struct term_t : std::variant<term::variable_t, term::constant_t, term::function_t>
    {
        using variant::variant;
    };

    struct formula_t;
    namespace formula
    {
        using ptr = std::shared_ptr<const formula_t>;

        struct predicate_t { std::string name; std::vector<term::ptr> args; };
        struct not_t { ptr arg; };
        struct and_t { ptr lhs, rhs; };
        struct or_t { ptr lhs, rhs; };
        struct implies_t { ptr lhs, rhs; };
        struct iff_t { ptr lhs, rhs; };
        struct forall_t { std::string var; ptr body; };
        struct exists_t { std::string var; ptr body; };
        struct true_t { };
        struct false_t { };
    } // namespace formula

    struct formula_t : std::variant<
        formula::predicate_t, formula::not_t, formula::and_t, formula::or_t, formula::implies_t,
        formula::iff_t, formula::forall_t, formula::exists_t, formula::true_t, formula::false_t
    > { using variant::variant; };

    term::ptr make_var(std::string name)
    {
        return std::make_shared<term_t>(term::variable_t { std::move(name) });
    }

    term::ptr make_const(std::string name)
    {
        return std::make_shared<term_t>(term::constant_t { std::move(name) });
    }

    term::ptr make_func(std::string name, std::vector<term::ptr> args)
    {
        return std::make_shared<term_t>(term::function_t { std::move(name), std::move(args) });
    }

    formula::ptr make_pred(std::string name, std::vector<term::ptr> args)
    {
        return std::make_shared<formula_t>(formula::predicate_t { std::move(name), std::move(args) });
    }

    formula::ptr make_not(formula::ptr arg)
    {
        return std::make_shared<formula_t>(formula::not_t { std::move(arg) });
    }

    formula::ptr make_and(formula::ptr lhs, formula::ptr rhs)
    {
        return std::make_shared<formula_t>(formula::and_t { std::move(lhs), std::move(rhs) });
    }

    formula::ptr make_or(formula::ptr lhs, formula::ptr rhs)
    {
        return std::make_shared<formula_t>(formula::or_t { std::move(lhs), std::move(rhs) });
    }

    formula::ptr make_implies(formula::ptr lhs, formula::ptr rhs)
    {
        return std::make_shared<formula_t>(formula::implies_t { std::move(lhs), std::move(rhs) });
    }

    formula::ptr make_iff(formula::ptr lhs, formula::ptr rhs)
    {
        return std::make_shared<formula_t>(formula::iff_t { std::move(lhs), std::move(rhs) });
    }

    formula::ptr make_forall(std::string var, formula::ptr body)
    {
        return std::make_shared<formula_t>(formula::forall_t { std::move(var), std::move(body) });
    }

    formula::ptr make_exists(std::string var, formula::ptr body)
    {
        return std::make_shared<formula_t>(formula::exists_t { std::move(var), std::move(body) });
    }

    formula::ptr make_true() { return std::make_shared<formula_t>(formula::true_t { }); }
    formula::ptr make_false() { return std::make_shared<formula_t>(formula::false_t { }); }

    formula::ptr to_nnf(formula::ptr formula);
    formula::ptr to_nnf_neg(formula::ptr formula);
    bool is_nnf(formula::ptr formula);

    bool equal(term::ptr lhs, term::ptr rhs);
    bool equal(formula::ptr lhs, formula::ptr rhs);

    formula::ptr to_cnf(formula::ptr formula);
    bool is_cnf(formula::ptr formula);

    formula::ptr to_dnf(formula::ptr formula);
    bool is_dnf(formula::ptr formula);
} // export namespace resol

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

    formula::ptr to_nnf_neg(formula::ptr formula);
    formula::ptr to_nnf(formula::ptr formula)
    {
        return std::visit(overloaded {
            [](const formula::predicate_t &val) -> formula::ptr {
                return std::make_shared<formula_t>(val);
            },
            [](const formula::not_t &val) {
                return to_nnf_neg(val.arg);
            },
            [](const formula::and_t &val) {
                return make_and(to_nnf(val.lhs), to_nnf(val.rhs));
            },
            [](const formula::or_t &val) {
                return make_or(to_nnf(val.lhs), to_nnf(val.rhs));
            },
            [](const formula::implies_t &val) {
                return make_or(to_nnf_neg(val.lhs), to_nnf(val.rhs));
            },
            [](const formula::iff_t &val) {
                return make_and(
                    make_or(to_nnf_neg(val.lhs), to_nnf(val.rhs)),
                    make_or(to_nnf_neg(val.rhs), to_nnf(val.lhs))
                );
            },
            [](const formula::forall_t &val) {
                return make_forall(val.var, to_nnf(val.body));
            },
            [](const formula::exists_t &val) {
                return make_exists(val.var, to_nnf(val.body));
            },
            [](const formula::true_t &) {
                return make_true();
            },
            [](const formula::false_t &) {
                return make_false();
            }
        }, *formula);
    }

    formula::ptr to_nnf_neg(formula::ptr formula)
    {
        return std::visit(overloaded {
            [](const formula::predicate_t &val) {
                return make_not(std::make_shared<formula_t>(val));
            },
            [](const formula::not_t &val) {
                return to_nnf(val.arg);
            },
            [](const formula::and_t &val) {
                return make_or(to_nnf_neg(val.lhs), to_nnf_neg(val.rhs));
            },
            [](const formula::or_t &val) {
                return make_and(to_nnf_neg(val.lhs), to_nnf_neg(val.rhs));
            },
            [](const formula::implies_t &val) {
                return make_and(to_nnf(val.lhs), to_nnf_neg(val.rhs));
            },
            [](const formula::iff_t &val) {
                return make_or(
                    make_and(to_nnf(val.lhs), to_nnf_neg(val.rhs)),
                    make_and(to_nnf(val.rhs), to_nnf_neg(val.lhs))
                );
            },
            [](const formula::forall_t &val) {
                return make_exists(val.var, to_nnf_neg(val.body));
            },
            [](const formula::exists_t &val) {
                return make_forall(val.var, to_nnf_neg(val.body));
            },
            [](const formula::true_t &) {
                return make_false();
            },
            [](const formula::false_t &) {
                return make_true();
            }
        }, *formula);
    }

    bool is_nnf(formula::ptr formula)
    {
        return std::visit(overloaded {
            [](const formula::predicate_t &val) {
                return true;
            },
            [](const formula::not_t &val) {
                return std::holds_alternative<formula::predicate_t>(*val.arg);
            },
            [](const formula::and_t &val) {
                return is_nnf(val.lhs) && is_nnf(val.rhs);
            },
            [](const formula::or_t &val) {
                return is_nnf(val.lhs) && is_nnf(val.rhs);
            },
            [](const formula::implies_t &val) {
                return false;
            },
            [](const formula::iff_t &val) {
                return false;
            },
            [](const formula::forall_t &val) {
                return is_nnf(val.body);
            },
            [](const formula::exists_t &val) {
                return is_nnf(val.body);
            },
            [](const formula::true_t &) {
                return true;
            },
            [](const formula::false_t &) {
                return true;
            }
        }, *formula);
    }

    bool equal(term::ptr lhs, term::ptr rhs);
    bool equal(formula::ptr lhs, formula::ptr rhs)
    {
        if (lhs->index() != rhs->index())
            return false;

        return std::visit(overloaded {
            [&rhs](const formula::predicate_t &val) {
                const auto &other = std::get<formula::predicate_t>(*rhs);
                if (val.name != other.name || val.args.size() != other.args.size())
                    return false;
                for (const auto &[lhs, rhs] : std::views::zip(val.args, other.args))
                {
                    if (!equal(lhs, rhs))
                        return false;
                }
                return true;
            },
            [&rhs](const formula::not_t &val) {
                return equal(val.arg, std::get<formula::not_t>(*rhs).arg);
            },
            [&rhs](const formula::and_t &val) {
                const auto &other = std::get<formula::and_t>(*rhs);
                return equal(val.lhs, other.lhs) && equal(val.rhs, other.rhs);
            },
            [&rhs](const formula::or_t &val) {
                const auto &other = std::get<formula::or_t>(*rhs);
                return equal(val.lhs, other.lhs) && equal(val.rhs, other.rhs);
            },
            [&rhs](const formula::implies_t &val) {
                const auto &other = std::get<formula::implies_t>(*rhs);
                return equal(val.lhs, other.lhs) && equal(val.rhs, other.rhs);
            },
            [&rhs](const formula::iff_t &val) {
                const auto &other = std::get<formula::iff_t>(*rhs);
                return equal(val.lhs, other.lhs) && equal(val.rhs, other.rhs);
            },
            [&rhs](const formula::forall_t &val) {
                const auto &other = std::get<formula::forall_t>(*rhs);
                return val.var == other.var && equal(val.body, other.body);
            },
            [&rhs](const formula::exists_t &val) {
                const auto &other = std::get<formula::exists_t>(*rhs);
                return val.var == other.var && equal(val.body, other.body);
            },
            [](const formula::true_t &) {
                return true;
            },
            [](const formula::false_t &) {
                return true;
            }
        }, *lhs);
    }

    bool equal(term::ptr lhs, term::ptr rhs)
    {
        if (lhs->index() != rhs->index())
            return false;

        return std::visit(overloaded {
            [&rhs](const term::variable_t &val) {
                const auto &other = std::get<term::variable_t>(*rhs);
                return val.name == other.name;
            },
            [&rhs](const term::constant_t &val) {
                const auto &other = std::get<term::constant_t>(*rhs);
                return val.name == other.name;
            },
            [&rhs](const term::function_t &val) {
                const auto &other = std::get<term::function_t>(*rhs);
                if (val.name != other.name || val.args.size() != other.args.size())
                    return false;
                for (const auto &[lhs, rhs] : std::views::zip(val.args, other.args))
                {
                    if (!equal(lhs, rhs))
                        return false;
                }
                return true;
            }
        }, *lhs);
    }
} // export namespace resol

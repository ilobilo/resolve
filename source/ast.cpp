// Copyright (C) 2026  ilobilo

module ast;

namespace resol
{
    // produces negation normal form of formula
    // -> and <-> are eliminated
    // negations are moved inwards to atoms
    formula::ptr to_nnf(formula::ptr formula)
    {
        return std::visit(overloaded {
            [](const formula::predicate_t &val) -> formula::ptr {
                return std::make_shared<formula_t>(val);
            },
            // push the negation inward
            [](const formula::not_t &val) {
                return to_nnf_neg(val.arg);
            },
            [](const formula::and_t &val) {
                return make_and(to_nnf(val.lhs), to_nnf(val.rhs));
            },
            [](const formula::or_t &val) {
                return make_or(to_nnf(val.lhs), to_nnf(val.rhs));
            },
            // A -> B  =  ~A or B
            [](const formula::implies_t &val) {
                return make_or(to_nnf_neg(val.lhs), to_nnf(val.rhs));
            },
            // A <-> B  =  (~A or B) and (~B or A)
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

    // produces negation normal form of ~formula
    formula::ptr to_nnf_neg(formula::ptr formula)
    {
        return std::visit(overloaded {
            [](const formula::predicate_t &val) {
                return make_not(std::make_shared<formula_t>(val));
            },
            // double negation
            [](const formula::not_t &val) {
                return to_nnf(val.arg);
            },
            // de morgan's laws
            // ~(A and B)  =  ~A or ~B
            [](const formula::and_t &val) {
                return make_or(to_nnf_neg(val.lhs), to_nnf_neg(val.rhs));
            },
            // ~(A or B)  =  ~A and ~B
            [](const formula::or_t &val) {
                return make_and(to_nnf_neg(val.lhs), to_nnf_neg(val.rhs));
            },
            // ~(A -> B)  =  A and ~B
            [](const formula::implies_t &val) {
                return make_and(to_nnf(val.lhs), to_nnf_neg(val.rhs));
            },
            // ~(A <-> B)  =  (A and ~B) or (B and ~A)
            [](const formula::iff_t &val) {
                return make_or(
                    make_and(to_nnf(val.lhs), to_nnf_neg(val.rhs)),
                    make_and(to_nnf(val.rhs), to_nnf_neg(val.lhs))
                );
            },
            // ~forall(p)  =  exists(~p)
            [](const formula::forall_t &val) {
                return make_exists(val.var, to_nnf_neg(val.body));
            },
            // ~exists(p)  =  forall(~p)
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

    // returns true when formula doesn't contain -> or <->
    // and every negation is applied to a predicate
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

    // checks if two formulas are exactly the same
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

    namespace
    {
        formula::ptr distribute_or(formula::ptr lhs, formula::ptr rhs)
        {
            // if lhs is (A and B) then return (A or rhs) and (B or rhs)
            if (const auto other = std::get_if<formula::and_t>(lhs.get()))
            {
                return make_and(
                    distribute_or(other->lhs, rhs),
                    distribute_or(other->rhs, rhs)
                );
            }
            // if rhs is (A and B) then return (lhs or A) and (lhs or B)
            if (const auto other = std::get_if<formula::and_t>(rhs.get()))
            {
                return make_and(
                    distribute_or(lhs, other->lhs),
                    distribute_or(lhs, other->rhs)
                );
            }
            return make_or(std::move(lhs), std::move(rhs));
        }
    } // namespace

    // produces conjunctive normal form of formula
    formula::ptr to_cnf(formula::ptr formula)
    {
        if (!is_nnf(formula))
            formula = to_nnf(formula);

        return std::visit(overloaded {
            [](const formula::and_t &val) {
                return make_and(to_cnf(val.lhs), to_cnf(val.rhs));
            },
            [](const formula::or_t &val) {
                return distribute_or(to_cnf(val.lhs), to_cnf(val.rhs));
            },
            [](const formula::forall_t &val) {
                return make_forall(val.var, to_cnf(val.body));
            },
            [](const formula::exists_t &val) {
                return make_exists(val.var, to_cnf(val.body));
            },
            [&formula](const auto &) {
                return formula;
            }
        }, *formula);
    }

    namespace
    {
        // literal is a predicate, negated predicate, T or F
        bool is_literal(formula::ptr formula)
        {
            return std::visit(overloaded {
                [](const formula::predicate_t &) {
                    return true;
                },
                [](const formula::not_t &val) {
                    return std::holds_alternative<formula::predicate_t>(*val.arg);
                },
                [](const formula::true_t &) {
                    return true;
                },
                [](const formula::false_t &) {
                    return true;
                },
                [](const auto &) {
                    return false;
                }
            }, *formula);
        }

        // disjunction of literals or one literal
        bool is_clause(formula::ptr formula)
        {
            return std::visit(overloaded {
                [](const formula::or_t &val) {
                    return is_clause(val.lhs) && is_clause(val.rhs);
                },
                [&formula](const auto &) {
                    return is_literal(formula);
                }
            }, *formula);
        }
    } // namespace

    bool is_cnf(formula::ptr formula)
    {
        return std::visit(overloaded {
            [](const formula::and_t &val) {
                return is_cnf(val.lhs) && is_cnf(val.rhs);
            },
            [](const formula::forall_t &val) {
                return is_cnf(val.body);
            },
            [](const formula::exists_t &val) {
                return is_cnf(val.body);
            },
            [&formula](const auto &) {
                return is_clause(formula);
            },
        }, *formula);
    }

    namespace
    {
        formula::ptr distribute_and(formula::ptr lhs, formula::ptr rhs)
        {
            // if lhs is (A or B) then return (A and rhs) or (B and rhs)
            if (const auto other = std::get_if<formula::or_t>(lhs.get()))
            {
                return make_or(
                    distribute_and(other->lhs, rhs),
                    distribute_and(other->rhs, rhs)
                );
            }
            // if rhs is (A or B) then return (lhs and A) or (lhs and B)
            if (const auto other = std::get_if<formula::or_t>(rhs.get()))
            {
                return make_or(
                    distribute_and(lhs, other->lhs),
                    distribute_and(lhs, other->rhs)
                );
            }
            return make_and(std::move(lhs), std::move(rhs));
        }
    } // namespace

    // produces disjunctive normal form of formula
    formula::ptr to_dnf(formula::ptr formula)
    {
        if (!is_nnf(formula))
            formula = to_nnf(formula);

        return std::visit(overloaded {
            [](const formula::or_t &val) {
                return make_or(to_dnf(val.lhs), to_dnf(val.rhs));
            },
            [](const formula::and_t &val) {
                return distribute_and(to_dnf(val.lhs), to_dnf(val.rhs));
            },
            [](const formula::forall_t &val) {
                return make_forall(val.var, to_dnf(val.body));
            },
            [](const formula::exists_t &val) {
                return make_exists(val.var, to_dnf(val.body));
            },
            [&formula](const auto &) {
                return formula;
            },
        }, *formula);
    }

    namespace
    {
        // conjunction of literals
        bool is_term(formula::ptr formula)
        {
            return std::visit(overloaded {
                [](const formula::and_t &val) {
                    return is_term(val.lhs) && is_term(val.rhs);
                },
                [&formula](const auto &) {
                    return is_literal(formula);
                },
            }, *formula);
        }
    } // namespace

    bool is_dnf(formula::ptr formula)
    {
        return std::visit(overloaded {
            [](const formula::or_t &val) {
                return is_dnf(val.lhs) && is_dnf(val.rhs);
            },
            [](const formula::forall_t &val) {
                return is_dnf(val.body);
            },
            [](const formula::exists_t &val) {
                return is_dnf(val.body);
            },
            [&formula](const auto &) {
                return is_term(formula);
            },
        }, *formula);
    }
} // namespace resol

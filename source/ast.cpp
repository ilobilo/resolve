// Copyright (C) 2026  ilobilo

module ast;

import unify;

namespace resol
{
    namespace
    {
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
    } // namespace

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
                for (const auto &[a, b] : std::views::zip(val.args, other.args))
                {
                    if (!equal(a, b))
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
                return val.name == std::get<term::variable_t>(*rhs).name;
            },
            [&rhs](const term::constant_t &val) {
                return val.name == std::get<term::constant_t>(*rhs).name;
            },
            [&rhs](const term::function_t &val) {
                const auto &other = std::get<term::function_t>(*rhs);
                if (val.name != other.name || val.args.size() != other.args.size())
                    return false;
                for (const auto &[a, b] : std::views::zip(val.args, other.args))
                {
                    if (!equal(a, b))
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

    // assumes nnf
    // produces conjunctive normal form of formula
    formula::ptr to_cnf(formula::ptr formula)
    {
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

    // assumes nnf
    // produces disjunctive normal form of formula
    formula::ptr to_dnf(formula::ptr formula)
    {
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
            }
        }, *formula);
    }

    namespace
    {
        std::pair<formula::ptr, bool> is_negated(formula::ptr literal)
        {
            if (const auto neg = std::get_if<formula::not_t>(literal.get()))
                return { neg->arg, true };
            return { literal, false };
        }

        bool is_true(formula::ptr formula)
        {
            return std::holds_alternative<formula::true_t>(*formula);
        }

        bool is_false(formula::ptr formula)
        {
            return std::holds_alternative<formula::false_t>(*formula);
        }

        template<typename Op>
        void flatten(formula::ptr formula, std::vector<formula::ptr> &out)
        {
            if (const auto op = std::get_if<Op>(formula.get()))
            {
                flatten<Op>(op->lhs, out);
                flatten<Op>(op->rhs, out);
            }
            else out.push_back(std::move(formula));
        }

        template<typename Make>
        formula::ptr fold(std::vector<formula::ptr> parts, Make make, formula::ptr ident)
        {
            if (parts.empty())
                return ident;

            formula::ptr acc = std::move(parts.back());
            for (std::size_t i = parts.size() - 1; i-- > 0; )
                acc = make(std::move(parts[i]), std::move(acc));
            return acc;
        }

        formula::ptr simplify_or(std::vector<formula::ptr> parts)
        {
            std::vector<formula::ptr> kept;
            for (auto &part : parts)
            {
                // A or F = A
                if (is_false(part))
                    continue;
                // A or T = T
                if (is_true(part))
                    return make_true();

                if (std::ranges::any_of(kept, [&](const auto &val) { return equal(part, val); }))
                    continue;

                const auto [ppos, pneg] = is_negated(part);
                if (std::ranges::any_of(kept, [&](const auto &val) {
                        const auto [pos, neg] = is_negated(val);
                        return pneg != neg && equal(ppos, pos);
                    }))
                    return make_true();

                kept.push_back(std::move(part));
            }

            return fold(std::move(kept),
                [](auto lhs, auto rhs) {
                    return make_or(lhs, rhs);
                }, make_false()
            );
        }

        formula::ptr simplify_and(std::vector<formula::ptr> parts)
        {
            std::vector<formula::ptr> kept;
            for (auto &part : parts)
            {
                // A and T = A
                if (is_true(part))
                    continue;
                // A and F = F
                if (is_false(part))
                    return make_false();

                if (std::ranges::any_of(kept, [&](const auto &val) { return equal(part, val); }))
                    continue;

                const auto [ppos, pneg] = is_negated(part);
                if (std::ranges::any_of(kept, [&](const auto &val) {
                        const auto [pos, neg] = is_negated(val);
                        return pneg != neg && equal(ppos, pos);
                    }))
                    return make_false();

                kept.push_back(std::move(part));
            }

            return fold(std::move(kept),
                [](auto lhs, auto rhs) {
                    return make_and(lhs, rhs);
                }, make_true()
            );
        }
    } // namespace

    // assumes nnf
    formula::ptr simplify(formula::ptr formula)
    {
        return std::visit(overloaded {
            [](const formula::and_t &val) {
                std::vector<formula::ptr> parts;
                flatten<formula::and_t>(make_and(val.lhs, val.rhs), parts);
                for (auto &part : parts)
                    part = simplify(part);
                return simplify_and(std::move(parts));
            },
            [](const formula::or_t &val) {
                std::vector<formula::ptr> parts;
                flatten<formula::or_t>(make_or(val.lhs, val.rhs), parts);
                for (auto &part : parts)
                    part = simplify(part);
                return simplify_or(std::move(parts));
            },
            [](const formula::forall_t &val) {
                return make_forall(val.var, simplify(val.body));
            },
            [](const formula::exists_t &val) {
                return make_exists(val.var, simplify(val.body));
            },
            // predicate, ~predicate, T, F
            [&](const auto &) {
                return formula;
            }
        }, *formula);
    }

    namespace
    {
        using rename_map = std::unordered_map<std::string, std::string>;
        term::ptr rename_term(const rename_map &sub, term::ptr term)
        {
            return std::visit(overloaded {
                [&](const term::variable_t &val) {
                    if (const auto it = sub.find(val.name); it != sub.end())
                        return make_var(it->second);
                    return term;
                },
                [&](const term::constant_t &) {
                    return term;
                },
                [&](const term::function_t &val) {
                    std::vector<term::ptr> args;
                    args.reserve(val.args.size());
                    for (const auto &arg : val.args)
                        args.push_back(rename_term(sub, arg));
                    return make_func(val.name, std::move(args));
                }
            }, *term);
        }

        formula::ptr normalise_impl(formula::ptr formula, rename_map &sub, std::size_t &counter);

        template<typename Make>
        formula::ptr normalise_quant(
            const std::string &var, formula::ptr body,
            rename_map &sub, std::size_t &counter, Make make
        )
        {
            auto fresh = std::format("{}_{}", var, counter++);
            std::optional<std::string> saved;
            if (const auto it = sub.find(var); it != sub.end())
                saved = it->second;

            sub[var] = fresh;
            auto new_body = normalise_impl(body, sub, counter);
            if (saved)
                sub[var] = *saved;
            else
                sub.erase(var);

            return make(std::move(fresh), std::move(new_body));
        }

        formula::ptr normalise_impl(formula::ptr formula, rename_map &sub, std::size_t &counter)
        {
            return std::visit(overloaded {
                [&](const formula::predicate_t &val) {
                    std::vector<term::ptr> args;
                    args.reserve(val.args.size());
                    for (const auto &arg : val.args)
                        args.push_back(rename_term(sub, arg));
                    return make_pred(val.name, std::move(args));
                },
                [&](const formula::not_t &val) {
                    return make_not(normalise_impl(val.arg, sub, counter));
                },
                [&](const formula::and_t &val) {
                    auto lhs = normalise_impl(val.lhs, sub, counter);
                    auto rhs = normalise_impl(val.rhs, sub, counter);
                    return make_and(std::move(lhs), std::move(rhs));
                },
                [&](const formula::or_t &val) {
                    auto lhs = normalise_impl(val.lhs, sub, counter);
                    auto rhs = normalise_impl(val.rhs, sub, counter);
                    return make_or(std::move(lhs), std::move(rhs));
                },
                [&](const formula::implies_t &val) {
                    auto lhs = normalise_impl(val.lhs, sub, counter);
                    auto rhs = normalise_impl(val.rhs, sub, counter);
                    return make_implies(std::move(lhs), std::move(rhs));
                },
                [&](const formula::iff_t &val) {
                    auto lhs = normalise_impl(val.lhs, sub, counter);
                    auto rhs = normalise_impl(val.rhs, sub, counter);
                    return make_iff(std::move(lhs), std::move(rhs));
                },
                [&](const formula::forall_t &val) {
                    return normalise_quant(val.var, val.body, sub, counter,
                        [](std::string var, formula::ptr fptr) {
                            return make_forall(std::move(var), std::move(fptr));
                        }
                    );
                },
                [&](const formula::exists_t &val) {
                    return normalise_quant(val.var, val.body, sub, counter,
                        [](std::string var, formula::ptr fptr) {
                            return make_exists(std::move(var), std::move(fptr));
                        }
                    );
                },
                [](const formula::true_t &) {
                    return make_true();
                },
                [](const formula::false_t &) {
                    return make_false();
                }
            }, *formula);
        }
    } // namespace

    // makes every bound variable unique
    formula::ptr normalise_apart(formula::ptr formula)
    {
        rename_map sub;
        std::size_t counter = 0;
        return normalise_impl(std::move(formula), sub, counter);
    }

    namespace
    {
        formula::ptr skolemise_impl(
            formula::ptr formula,
            std::vector<std::string> &universals,
            substitution &sub,
            std::size_t &counter
        )
        {
            return std::visit(overloaded {
                [&](const formula::predicate_t &val) {
                    std::vector<term::ptr> args;
                    args.reserve(val.args.size());
                    for (const auto &arg : val.args)
                        args.push_back(resol::apply(sub, arg));
                    return make_pred(val.name, std::move(args));
                },
                [&](const formula::not_t &val) {
                    return make_not(skolemise_impl(val.arg, universals, sub, counter));
                },
                [&](const formula::and_t &val) {
                    auto lhs = skolemise_impl(val.lhs, universals, sub, counter);
                    auto rhs = skolemise_impl(val.rhs, universals, sub, counter);
                    return make_and(std::move(lhs), std::move(rhs));
                },
                [&](const formula::or_t &val) {
                    auto lhs = skolemise_impl(val.lhs, universals, sub, counter);
                    auto rhs = skolemise_impl(val.rhs, universals, sub, counter);
                    return make_or(std::move(lhs), std::move(rhs));
                },
                [&](const formula::implies_t &val) {
                    auto lhs = skolemise_impl(val.lhs, universals, sub, counter);
                    auto rhs = skolemise_impl(val.rhs, universals, sub, counter);
                    return make_implies(std::move(lhs), std::move(rhs));
                },
                [&](const formula::iff_t &val) {
                    auto lhs = skolemise_impl(val.lhs, universals, sub, counter);
                    auto rhs = skolemise_impl(val.rhs, universals, sub, counter);
                    return make_iff(std::move(lhs), std::move(rhs));
                },
                [&](const formula::forall_t &val) {
                    universals.push_back(val.var);
                    auto body = skolemise_impl(val.body, universals, sub, counter);
                    universals.pop_back();
                    return make_forall(val.var, std::move(body));
                },
                [&](const formula::exists_t &val) {
                    const auto name = std::format("sk_{}", counter++);
                    term::ptr skolem;
                    if (universals.empty())
                    {
                        skolem = make_const(name);
                    }
                    else
                    {
                        std::vector<term::ptr> args;
                        args.reserve(universals.size());
                        for (const auto &u : universals)
                            args.push_back(make_var(u));
                        skolem = make_func(name, std::move(args));
                    }

                    std::optional<term::ptr> saved;
                    if (const auto it = sub.find(val.var); it != sub.end())
                        saved = it->second;

                    sub[val.var] = std::move(skolem);
                    auto body = skolemise_impl(val.body, universals, sub, counter);
                    if (saved)
                        sub[val.var] = std::move(*saved);
                    else
                        sub.erase(val.var);

                    return body;
                },
                [](const formula::true_t &) {
                    return make_true();
                },
                [](const formula::false_t &) {
                    return make_false();
                }
            }, *formula);
        }
    } // namespace

    // assumes nnf and normalised
    formula::ptr skolemise(formula::ptr formula)
    {
        std::vector<std::string> universals;
        substitution sub;
        std::size_t counter = 0;
        return skolemise_impl(std::move(formula), universals, sub, counter);
    }

    // strips every universal quantifier
    formula::ptr drop_universals(formula::ptr formula)
    {
        return std::visit(overloaded {
            [](const formula::forall_t &val) {
                return drop_universals(val.body);
            },
            [](const formula::exists_t &val) {
                return make_exists(val.var, drop_universals(val.body));
            },
            [](const formula::not_t &val) {
                return make_not(drop_universals(val.arg));
            },
            [](const formula::and_t &val) {
                return make_and(drop_universals(val.lhs), drop_universals(val.rhs));
            },
            [](const formula::or_t &val) {
                return make_or(drop_universals(val.lhs), drop_universals(val.rhs));
            },
            [](const formula::implies_t &val) {
                return make_implies(drop_universals(val.lhs), drop_universals(val.rhs));
            },
            [](const formula::iff_t &val) {
                return make_iff(drop_universals(val.lhs), drop_universals(val.rhs));
            },
            [&formula](const auto &) {
                return formula;
            }
        }, *formula);
    }
} // namespace resol

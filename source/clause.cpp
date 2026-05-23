// Copyright (C) 2026  ilobilo

module clause;

import util;

namespace resol
{
    namespace
    {
        bool same_atom(const literal &lhs, const literal &rhs)
        {
            if (lhs.name != rhs.name || lhs.args.size() != rhs.args.size())
                return false;
            for (const auto &[a, b] : std::views::zip(lhs.args, rhs.args))
            {
                if (!equal(a, b))
                    return false;
            }
            return true;
        }

        // returns false if literal body is not a clause
        bool collect_disj(formula::ptr formula, clause &out, bool &tautology)
        {
            return std::visit(overloaded {
                [&](const formula::or_t &val) {
                    return collect_disj(val.lhs, out, tautology) &&
                           collect_disj(val.rhs, out, tautology);
                },
                [&](const formula::predicate_t &val) {
                    out.push_back({ false, val.name, val.args });
                    return true;
                },
                [&](const formula::not_t &val) {
                    const auto pred = std::get_if<formula::predicate_t>(val.arg.get());
                    if (!pred)
                        return false;
                    out.push_back({ true, pred->name, pred->args });
                    return true;
                },
                [&](const formula::true_t &) {
                    tautology = true;
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

        void collect_conj(formula::ptr formula, clause_set &out)
        {
            std::visit(overloaded {
                [&](const formula::and_t &val) {
                    collect_conj(val.lhs, out);
                    collect_conj(val.rhs, out);
                },
                [&](const formula::true_t &) {
                    // noop
                },
                [&formula, &out](const auto &) {
                    clause cl;
                    bool tautology = false;
                    if (!collect_disj(formula, cl, tautology))
                        return;
                    if (tautology || is_tautology(cl))
                        return;
                    out.push_back(std::move(cl));
                }
            }, *formula);
        }
    } // namespace

    bool is_tautology(const clause &cl)
    {
        for (std::size_t i = 0; i < cl.size(); i++)
        {
            for (std::size_t j = i + 1; j < cl.size(); j++)
            {
                if (cl[i].negated != cl[j].negated && same_atom(cl[i], cl[j]))
                    return true;
            }
        }
        return false;
    }

    clause_set extract_clauses(formula::ptr formula)
    {
        clause_set out;
        collect_conj(std::move(formula), out);
        return out;
    }

    bool equal(const literal &lhs, const literal &rhs)
    {
        return lhs.negated == rhs.negated && same_atom(lhs, rhs);
    }
} // namespace resol

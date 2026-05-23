// Copyright (C) 2026  ilobilo

module clause;

import util;

namespace resol
{
    namespace
    {
        // returns false if the formula isn't a valid clause body
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

        bool is_tautological(const clause &cl)
        {
            for (std::size_t i = 0; i < cl.size(); i++)
            {
                for (std::size_t j = i + 1; j < cl.size(); j++)
                {
                    if (cl[i].negated == cl[j].negated)
                        continue;
                    if (cl[i].name != cl[j].name || cl[i].args.size() != cl[j].args.size())
                        continue;

                    bool match = true;
                    for (const auto &[lhs, rhs] : std::views::zip(cl[i].args, cl[j].args))
                    {
                        if (!equal(lhs, rhs))
                        {
                            match = false;
                            break;
                        }
                    }
                    if (match)
                        return true;
                }
            }
            return false;
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
                    if (tautology)
                        return;
                    if (is_tautological(cl))
                        return;
                    out.push_back(std::move(cl));
                }
            }, *formula);
        }
    } // namespace

    clause_set extract_clauses(formula::ptr formula)
    {
        clause_set out;
        collect_conj(std::move(formula), out);
        return out;
    }

    bool equal(const literal &lhs, const literal &rhs)
    {
        if (lhs.negated != rhs.negated)
            return false;
        if (lhs.name != rhs.name || lhs.args.size() != rhs.args.size())
            return false;

        for (const auto &[left, right] : std::views::zip(lhs.args, rhs.args))
        {
            if (!equal(left, right))
                return false;
        }
        return true;
    }

    std::string to_string(const literal &lit, print_style style)
    {
        std::string out;
        if (lit.negated)
            out += (style == print_style::ascii ? "~" : "¬");

        out += lit.name;
        if (!lit.args.empty())
        {
            out += '(';
            for (std::size_t i = 0; i < lit.args.size(); i++)
            {
                if (i != 0)
                    out += ", ";
                out += to_string(lit.args[i]);
            }
            out += ')';
        }
        return out;
    }

    std::string to_string(const clause &cl, print_style style)
    {
        if (cl.empty())
            return style == print_style::ascii ? "false" : "⊥";

        const auto sep = (style == print_style::ascii ? " | " : " ∨ ");
        std::string out;
        for (std::size_t i = 0; i < cl.size(); i++)
        {
            if (i != 0)
                out += sep;
            out += to_string(cl[i], style);
        }
        return out;
    }
} // namespace resol

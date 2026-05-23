// Copyright (C) 2026  ilobilo

module resolution;

import util;

namespace resol
{
    namespace
    {
        // renames every variable to <name>#<tag>
        term::ptr rename_term(std::string_view tag, term::ptr term)
        {
            return std::visit(overloaded {
                [&](const term::variable_t &val) {
                    return make_var(std::format("{}#{}", val.name, tag));
                },
                [&](const term::constant_t &) {
                    return term;
                },
                [&](const term::function_t &val) {
                    std::vector<term::ptr> args;
                    args.reserve(val.args.size());
                    for (const auto &arg : val.args)
                        args.push_back(rename_term(tag, arg));
                    return make_func(val.name, std::move(args));
                }
            }, *term);
        }

        // copy cl with every variable renamed (see above)
        clause rename_apart(const clause &cl, std::size_t tag)
        {
            const auto tag_str = std::to_string(tag);
            clause out;
            out.reserve(cl.size());
            for (const auto &lit : cl)
            {
                std::vector<term::ptr> args;
                args.reserve(lit.args.size());
                for (const auto &arg : lit.args)
                    args.push_back(rename_term(tag_str, arg));
                out.push_back({ lit.negated, lit.name, std::move(args) });
            }
            return out;
        }

        // removes duplicate literals from cl
        void factor(clause &cl)
        {
            for (std::size_t i = 0; i < cl.size(); i++)
            {
                std::size_t j = i + 1;
                while (j < cl.size())
                {
                    if (equal(cl[i], cl[j]))
                        cl.erase(cl.begin() + j);
                    else
                        j++;
                }
            }
        }

        // extends sub so that p[sub] is equal to q
        bool match_term(term::ptr p, term::ptr q, substitution &sub)
        {
            return std::visit(overloaded {
                [&](const term::variable_t &val) {
                    if (const auto it = sub.find(val.name); it != sub.end())
                        return equal(it->second, q);
                    sub[val.name] = q;
                    return true;
                },
                [&](const term::constant_t &val) {
                    const auto qc = std::get_if<term::constant_t>(q.get());
                    return qc && qc->name == val.name;
                },
                [&](const term::function_t &val) {
                    const auto qf = std::get_if<term::function_t>(q.get());
                    if (!qf || qf->name != val.name || qf->args.size() != val.args.size())
                        return false;
                    for (std::size_t i = 0; i < val.args.size(); i++)
                    {
                        if (!match_term(val.args[i], qf->args[i], sub))
                            return false;
                    }
                    return true;
                }
            }, *p);
        }

        bool match_literal(const literal &p, const literal &q, substitution &sub)
        {
            if (p.negated != q.negated)
                return false;
            if (p.name != q.name || p.args.size() != q.args.size())
                return false;

            for (std::size_t i = 0; i < p.args.size(); i++)
            {
                if (!match_term(p.args[i], q.args[i], sub))
                    return false;
            }
            return true;
        }

        bool subsumes_aux(const clause &c, std::size_t i, const clause &d, substitution &sub)
        {
            if (i == c.size())
                return true;

            for (std::size_t j = 0; j < d.size(); j++)
            {
                substitution backup = sub;
                if (match_literal(c[i], d[j], sub) && subsumes_aux(c, i + 1, d, sub))
                    return true;
                sub = std::move(backup);
            }
            return false;
        }

        bool subsumes(const clause &c, const clause &d)
        {
            if (c.size() > d.size())
                return false;
            substitution sub;
            return subsumes_aux(c, 0, d, sub);
        }

        // applies sub to every term inside a literal
        literal apply_(const substitution &sub, const literal &lit)
        {
            std::vector<term::ptr> args;
            args.reserve(lit.args.size());
            for (const auto &arg : lit.args)
                args.push_back(resol::apply(sub, arg));
            return { lit.negated, lit.name, std::move(args) };
        }

        std::optional<std::pair<substitution, clause>> try_resolve(
            const clause &lhs, const clause &rhs, std::size_t li, std::size_t rj
        )
        {
            const auto &a = lhs[li];
            const auto &b = rhs[rj];

            if (a.negated == b.negated)
                return std::nullopt;
            if (a.name != b.name || a.args.size() != b.args.size())
                return std::nullopt;

            auto pa = make_pred(a.name, a.args);
            auto pb = make_pred(b.name, b.args);

            auto sub = unify(pa, pb);
            if (!sub)
                return std::nullopt;

            clause result;
            result.reserve(lhs.size() + rhs.size() - 2);
            for (std::size_t i = 0; i < lhs.size(); i++)
            {
                if (i != li)
                    result.push_back(apply_(*sub, lhs[i]));
            }

            for (std::size_t i = 0; i < rhs.size(); i++)
            {
                if (i != rj)
                    result.push_back(apply_(*sub, rhs[i]));
            }

            factor(result);
            return std::make_pair(std::move(*sub), std::move(result));
        }
    } // namespace

    prove_result run_resolution(clause_set inputs, std::size_t limit)
    {
        prove_result result;
        result.iterations = 0;
        result.status = prove_status::satisfiable;

        std::vector<std::size_t> usable;
        usable.reserve(inputs.size());

        for (auto &cl : inputs)
        {
            const bool empty = cl.empty();
            const auto idx = result.steps.size();
            result.steps.push_back({ { }, { }, std::move(cl) });
            if (empty)
            {
                result.status = prove_status::unsatisfiable;
                return result;
            }
            usable.push_back(idx);
        }

        std::vector<std::size_t> processed;
        std::size_t rename_counter = 0;

        while (!usable.empty())
        {
            if (result.iterations >= limit)
            {
                result.status = prove_status::limit_reached;
                return result;
            }
            result.iterations++;

            // pick the shortest clause
            const auto best = std::ranges::min_element(usable, { }, [&](std::size_t i) {
                return result.steps[i].result.size();
            });
            const auto idx = *best;
            usable.erase(best);

            const auto cl = result.steps[idx].result;
            for (const auto other_idx : processed)
            {
                const auto renamed = rename_apart(result.steps[other_idx].result, rename_counter++);

                for (std::size_t i = 0; i < cl.size(); i++)
                {
                    for (std::size_t j = 0; j < renamed.size(); j++)
                    {
                        auto res = try_resolve(cl, renamed, i, j);
                        if (!res)
                            continue;
                        auto &[mgu, resolvent] = *res;

                        if (resolvent.empty())
                        {
                            result.steps.push_back({
                                { idx, other_idx }, std::move(mgu), { }
                            });
                            result.status = prove_status::unsatisfiable;
                            return result;
                        }

                        if (is_tautology(resolvent))
                            continue;

                        if (std::ranges::any_of(result.steps, [&](const auto &step) {
                                return subsumes(step.result, resolvent);
                            }))
                            continue;

                        const auto new_idx = result.steps.size();
                        result.steps.push_back({
                            { idx, other_idx }, std::move(mgu), std::move(resolvent)
                        });
                        usable.push_back(new_idx);
                    }
                }
            }

            processed.push_back(idx);
        }

        return result;
    }
} // namespace resol

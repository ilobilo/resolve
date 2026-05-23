// Copyright (C) 2026  ilobilo

module unify;

import util;

namespace resol
{
    namespace
    {
        // true if variable appears inside term
        bool occurs(std::string_view var, term::ptr term)
        {
            return std::visit(overloaded {
                [var](const term::variable_t &val) {
                    return val.name == var;
                },
                [](const term::constant_t &) {
                    return false;
                },
                [var](const term::function_t &val) {
                    return std::ranges::any_of(val.args, [var](const auto &arg) {
                        return occurs(var, arg);
                    });
                }
            }, *term);
        }

        // bind { var -> term } into sub
        unify_result<void> bind(substitution &sub, std::string var, term::ptr term)
        {
            // x := x is no-op
            const auto other = std::get_if<term::variable_t>(term.get());
            if (other && other->name == var)
                return { };

            if (occurs(var, term))
            {
                return std::unexpected { unify_error {
                    std::format("occurs check failed: {} appears in its own binding", var)
                } };
            }

            // apply { var -> term } to every existing value
            substitution patch { { var, term } };
            for (auto &[_, val] : sub)
                val = resol::apply(patch, val);

            sub.emplace(std::move(var), std::move(term));
            return { };
        }

        unify_result<substitution> unify_pairs(std::vector<std::pair<term::ptr, term::ptr>> work)
        {
            substitution sub;

            while (!work.empty())
            {
                auto [lhs, rhs] = std::move(work.back());
                work.pop_back();

                lhs = resol::apply(sub, lhs);
                rhs = resol::apply(sub, rhs);

                // identical
                if (equal(lhs, rhs))
                    continue;

                if (const auto var = std::get_if<term::variable_t>(lhs.get()))
                {
                    const auto res = bind(sub, var->name, rhs);
                    if (!res)
                        return std::unexpected { res.error() };
                    continue;
                }
                if (const auto var = std::get_if<term::variable_t>(rhs.get()))
                {
                    const auto res = bind(sub, var->name, lhs);
                    if (!res)
                        return std::unexpected { res.error() };
                    continue;
                }

                const auto flhs = std::get_if<term::function_t>(lhs.get());
                const auto frhs = std::get_if<term::function_t>(rhs.get());
                if (flhs && frhs)
                {
                    if (flhs->name != frhs->name || flhs->args.size() != frhs->args.size())
                    {
                        return std::unexpected { unify_error {
                            std::format("invalid functions: {}/{} vs {}/{}",
                                flhs->name, flhs->args.size(), frhs->name, frhs->args.size())
                        } };
                    }
                    for (const auto &[left, right] : std::views::zip(flhs->args, frhs->args))
                        work.emplace_back(left, right);
                    continue;
                }

                return std::unexpected { unify_error { "incompatible terms" } };
            }

            return sub;
        }
    } // namespace

    term::ptr apply(const substitution &sub, term::ptr term)
    {
        return std::visit(overloaded {
            [&](const term::variable_t &val) {
                const auto it = sub.find(val.name);
                if (it == sub.end())
                    return term;
                // x -> y, y -> a resolves to a
                return resol::apply(sub, it->second);
            },
            [&](const term::constant_t &) {
                return term;
            },
            [&](const term::function_t &val) {
                std::vector<term::ptr> args;
                args.reserve(val.args.size());
                for (const auto &arg : val.args)
                    args.push_back(resol::apply(sub, arg));
                return make_func(val.name, std::move(args));
            }
        }, *term);
    }

    unify_result<substitution> unify(term::ptr lhs, term::ptr rhs)
    {
        std::vector<std::pair<term::ptr, term::ptr>> work;
        work.emplace_back(std::move(lhs), std::move(rhs));
        return unify_pairs(std::move(work));
    }

    unify_result<substitution> unify(formula::ptr lhs, formula::ptr rhs)
    {
        const auto plhs = std::get_if<formula::predicate_t>(lhs.get());
        const auto prhs = std::get_if<formula::predicate_t>(rhs.get());
        if (!plhs || !prhs)
        {
            return std::unexpected { unify_error {
                "can only unify atomic predicates"
            } };
        }
        if (plhs->name != prhs->name || plhs->args.size() != prhs->args.size())
        {
            return std::unexpected { unify_error {
                std::format("invalid predicates: {}/{} vs {}/{}",
                    plhs->name, plhs->args.size(), prhs->name, prhs->args.size())
            } };
        }

        std::vector<std::pair<term::ptr, term::ptr>> work;
        work.reserve(plhs->args.size());
        for (const auto &[left, right] : std::views::zip(plhs->args, prhs->args))
            work.emplace_back(left, right);
        return unify_pairs(std::move(work));
    }
} // namespace resol

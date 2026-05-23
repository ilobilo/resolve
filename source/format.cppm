// Copyright (C) 2026  ilobilo

export module format;

import clause;
import util;
import ast;
import std;

namespace resol
{
    namespace
    {
        struct symbols
        {
            std::string_view neg, conj, disj, impl, iff;
            std::string_view forall, exists;
            std::string_view top, bot;
            std::string_view qdot;
        };

        constexpr symbols symbols_unicode {
            .neg = "¬", .conj = " ∧ ", .disj = " ∨ ",
            .impl = " → ", .iff = " ↔ ",
            .forall = "∀", .exists = "∃",
            .top = "⊤", .bot = "⊥",
            .qdot = ". "
        };

        constexpr symbols symbols_ascii {
            .neg = "~", .conj = " & ", .disj = " | ",
            .impl = " -> ", .iff = " <-> ",
            .forall = "forall ", .exists = "exists ",
            .top = "true", .bot = "false",
            .qdot = ". "
        };

        void append_term(std::string &out, const term_t &term)
        {
            std::visit(overloaded {
                [&](const term::variable_t &val) {
                    out += val.name;
                },
                [&](const term::constant_t &val) {
                    out += val.name;
                },
                [&](const term::function_t &val) {
                    out += val.name;
                    out += '(';
                    for (std::size_t i = 0; i < val.args.size(); i++)
                    {
                        if (i != 0)
                            out += ", ";
                        append_term(out, *val.args[i]);
                    }
                    out += ')';
                }
            }, term);
        }

        void append_formula(
            std::string &out, const formula_t &formula,
            prec_t min_prec, const symbols &sym
        )
        {
            const auto wrap = [&](prec_t my_prec, auto &&body) {
                const bool paren = my_prec < min_prec;
                if (paren)
                    out += '(';
                body();
                if (paren)
                    out += ')';
            };

            std::visit(overloaded {
                [&](const formula::predicate_t &val) {
                    out += val.name;
                    if (!val.args.empty())
                    {
                        out += '(';
                        for (std::size_t i = 0; i < val.args.size(); i++)
                        {
                            if (i != 0)
                                out += ", ";
                            append_term(out, *val.args[i]);
                        }
                        out += ')';
                    }
                },
                [&](const formula::not_t &val) {
                    wrap(not_p, [&] {
                        out += sym.neg;
                        append_formula(out, *val.arg, not_p, sym);
                    });
                },
                [&](const formula::and_t &val) {
                    wrap(and_p, [&] {
                        append_formula(out, *val.lhs, and_p, sym);
                        out += sym.conj;
                        append_formula(out, *val.rhs, and_p + 1, sym);
                    });
                },
                [&](const formula::or_t &val) {
                    wrap(or_p, [&] {
                        append_formula(out, *val.lhs, or_p, sym);
                        out += sym.disj;
                        append_formula(out, *val.rhs, or_p + 1, sym);
                    });
                },
                [&](const formula::implies_t &val) {
                    wrap(implies_p, [&] {
                        append_formula(out, *val.lhs, implies_p + 1, sym);
                        out += sym.impl;
                        append_formula(out, *val.rhs, implies_p, sym);
                    });
                },
                [&](const formula::iff_t &val) {
                    wrap(iff_p, [&] {
                        append_formula(out, *val.lhs, iff_p + 1, sym);
                        out += sym.iff;
                        append_formula(out, *val.rhs, iff_p, sym);
                    });
                },
                [&](const formula::forall_t &val) {
                    wrap(quant_p, [&] {
                        out += sym.forall;
                        out += val.var;
                        out += sym.qdot;
                        append_formula(out, *val.body, quant_p, sym);
                    });
                },
                [&](const formula::exists_t &val) {
                    wrap(quant_p, [&] {
                        out += sym.exists;
                        out += val.var;
                        out += sym.qdot;
                        append_formula(out, *val.body, quant_p, sym);
                    });
                },
                [&](const formula::true_t &) {
                    out += sym.top;
                },
                [&](const formula::false_t &) {
                    out += sym.bot;
                }
            }, formula);
        }
    } // namespace
} // namespace resol

export namespace resol
{
    enum class print_style { unicode, ascii };

    std::string to_string(term::ptr term)
    {
        std::string out;
        append_term(out, *term);
        return out;
    }

    std::string to_string(formula::ptr formula, print_style style = print_style::unicode)
    {
        std::string out;
        const auto &symbols = (style == print_style::ascii) ? symbols_ascii : symbols_unicode;
        append_formula(out, *formula, 0, symbols);
        return out;
    }

    std::string to_string(const literal &lit, print_style style = print_style::unicode)
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

    std::string to_string(const clause &cl, print_style style = print_style::unicode)
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
} // export namespace resol

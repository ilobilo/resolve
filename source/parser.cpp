// Copyright (C) 2026  ilobilo

module parser;

import magic_enum;

namespace resol
{
    parse_result<formula::ptr> parser::parse_iff()
    {
        auto lhs = parse_implies();
        if (!lhs)
            return lhs;

        if (accept(token_type::op_iff))
        {
            auto rhs = parse_iff();
            if (!rhs)
                return rhs;
            return make_iff(std::move(*lhs), std::move(*rhs));
        }
        return lhs;
    }

    parse_result<formula::ptr> parser::parse_implies()
    {
        auto lhs = parse_or();
        if (!lhs)
            return lhs;

        if (accept(token_type::op_implies))
        {
            auto rhs = parse_implies();
            if (!rhs)
                return rhs;
            return make_implies(std::move(*lhs), std::move(*rhs));
        }
        return lhs;
    }

    parse_result<formula::ptr> parser::parse_or()
    {
        auto lhs = parse_and();
        if (!lhs)
            return lhs;

        while (accept(token_type::op_or))
        {
            auto rhs = parse_and();
            if (!rhs)
                return rhs;
            lhs = make_or(std::move(*lhs), std::move(*rhs));
        }
        return lhs;
    }

    parse_result<formula::ptr> parser::parse_and()
    {
        auto lhs = parse_unary();
        if (!lhs)
            return lhs;

        while (accept(token_type::op_and))
        {
            auto rhs = parse_unary();
            if (!rhs)
                return rhs;
            lhs = make_and(std::move(*lhs), std::move(*rhs));
        }
        return lhs;
    }

    parse_result<formula::ptr> parser::parse_unary()
    {
        if (accept(token_type::op_not))
        {
            auto inner = parse_unary();
            if (!inner)
                return inner;
            return make_not(std::move(*inner));
        }
        const auto type = _lex.peek().type;
        if (type == token_type::kw_forall || type == token_type::kw_exists)
            return parse_quant();
        return parse_atom();
    }

    parse_result<formula::ptr> parser::parse_quant()
    {
        const auto kw = _lex.next();
        const bool universal = (kw.type == token_type::kw_forall);

        const auto id = expect(token_type::ident);
        if (!id)
            return std::unexpected { id.error() };
        std::string var { id->text };

        const auto dot = expect(token_type::dot);
        if (!dot)
            return std::unexpected { dot.error() };

        _bound.push_back(var);
        auto body = parse_iff();
        _bound.pop_back();

        if (!body)
            return std::unexpected { body.error() };

        return universal
            ? make_forall(std::move(var), std::move(*body))
            : make_exists(std::move(var), std::move(*body));
    }

    parse_result<formula::ptr> parser::parse_atom()
    {
        if (accept(token_type::lparen))
        {
            const auto inner = parse_iff();
            if (!inner)
                return inner;
            const auto rp = expect(token_type::rparen);
            if (!rp)
                return std::unexpected { rp.error() };
            return inner;
        }

        if (accept(token_type::kw_true))
            return make_true();
        if (accept(token_type::kw_false))
            return make_false();

        const auto id = expect(token_type::ident);
        if (!id)
            return std::unexpected { id.error() };
        std::string name { id->text };

        std::vector<term::ptr> args;
        if (accept(token_type::lparen))
        {
            auto list = parse_term_list();
            if (!list)
                return std::unexpected { list.error() };
            args = std::move(*list);
            const auto rp = expect(token_type::rparen);
            if (!rp)
                return std::unexpected { rp.error() };
        }
        return make_pred(std::move(name), std::move(args));
    }

    parse_result<term::ptr> parser::parse_term()
    {
        const auto id = expect(token_type::ident);
        if (!id)
            return std::unexpected { id.error() };
        std::string name { id->text };

        if (accept(token_type::lparen))
        {
            auto list = parse_term_list();
            if (!list)
                return std::unexpected { list.error() };
            const auto rp = expect(token_type::rparen);
            if (!rp)
                return std::unexpected { rp.error() };
            return make_func(std::move(name), std::move(*list));
        }
        return is_bound(name)
            ? make_var(std::move(name))
            : make_const(std::move(name));
    }

    parse_result<std::vector<term::ptr>> parser::parse_term_list()
    {
        std::vector<term::ptr> out;
        auto first = parse_term();
        if (!first)
            return std::unexpected { first.error() };

        out.push_back(std::move(*first));
        while (accept(token_type::comma))
        {
            auto next = parse_term();
            if (!next)
                return std::unexpected { next.error() };
            out.push_back(std::move(*next));
        }
        return out;
    }

    parse_result<token> parser::expect(token_type type)
    {
        const auto tok = _lex.peek();
        if (tok.type == type)
            return _lex.next();
        return std::unexpected { parse_error {
            std::format("expected {}, got '{}'", magic_enum::enum_name(type), tok.text),
            tok.pos
        } };
    }

    bool parser::accept(token_type type)
    {
        if (_lex.peek().type != type)
            return false;
        _lex.next();
        return true;
    }

    bool parser::is_bound(std::string_view name) const
    {
        return std::ranges::find(_bound, name) != _bound.end();
    }

    parse_result<formula::ptr> parser::parse_formula()
    {
        auto res = parse_iff();
        if (!res)
            return res;

        if (const auto tok = _lex.peek(); tok.type != token_type::end)
        {
            return std::unexpected { parse_error {
                std::format("unexpected token '{}' after formula", tok.text),
                tok.pos
            } };
        }
        return res;
    }

    parse_result<sequent> parser::parse_sequent()
    {
        sequent seq;
        if (_lex.peek().type != token_type::turnstile)
        {
            auto first = parse_iff();
            if (!first)
                return std::unexpected { first.error() };

            seq.formulas.push_back(std::move(*first));
            while (accept(token_type::comma))
            {
                auto next = parse_iff();
                if (!next)
                    return std::unexpected { next.error() };
                seq.formulas.push_back(std::move(*next));
            }
        }

        const auto ts = expect(token_type::turnstile);
        if (!ts)
            return std::unexpected { ts.error() };

        auto conc = parse_iff();
        if (!conc)
            return std::unexpected { conc.error() };

        seq.result = std::move(*conc);

        if (const auto tok = _lex.peek(); tok.type != token_type::end)
        {
            return std::unexpected { parse_error {
                std::format("unexpected token '{}' after sequent", tok.text),
                tok.pos
            } };
        }
        return seq;
    }
} // namespace resol

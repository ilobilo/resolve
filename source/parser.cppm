// Copyright (C) 2026  ilobilo

export module parser;

import ast;
import lexer;
import std;

export namespace resol
{
    struct parse_error
    {
        std::string message;
        std::size_t pos;
    };

    template<typename Type>
    using parse_result = std::expected<Type, parse_error>;

    class parser
    {
        private:
        lexer _lex;
        std::vector<std::string> _bound;

        parse_result<formula::ptr> parse_iff();
        parse_result<formula::ptr> parse_implies();
        parse_result<formula::ptr> parse_or();
        parse_result<formula::ptr> parse_and();
        parse_result<formula::ptr> parse_unary();
        parse_result<formula::ptr> parse_quant();
        parse_result<formula::ptr> parse_atom();
        parse_result<term::ptr> parse_term();
        parse_result<std::vector<term::ptr>> parse_term_list();

        parse_result<token> expect(token_type type);
        bool accept(token_type type);
        bool is_bound(std::string_view name) const;

        public:
        explicit parser(std::string_view src) noexcept
            : _lex { src } { }

        parse_result<formula::ptr> parse_formula();
    };
} // export namespace resol

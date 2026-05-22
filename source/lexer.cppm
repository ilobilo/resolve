// Copyright (C) 2026  ilobilo

export module lexer;

import std;

namespace resol
{
    export enum class token_type
    {
        op_not,
        op_and,
        op_or,
        op_implies,
        op_iff,

        kw_forall,
        kw_exists,

        kw_true,
        kw_false,

        lparen,
        rparen,
        comma,
        dot,
        turnstile,

        ident,

        end,
        invalid
    };

    constexpr auto symbol_table = std::to_array<std::pair<std::string_view, token_type>>({
        { "<->", token_type::op_iff },
        { "<=>", token_type::op_iff },
        { "↔", token_type::op_iff },
        { "→", token_type::op_implies },
        { "∧", token_type::op_and },
        { "∨", token_type::op_or },
        { "∀", token_type::kw_forall },
        { "∃", token_type::kw_exists },
        { "⊤", token_type::kw_true },
        { "⊥", token_type::kw_false },
        { "⊢", token_type::turnstile },

        { "->", token_type::op_implies },
        { "=>", token_type::op_implies },
        { "&&", token_type::op_and },
        { "||", token_type::op_or },
        { "/\\", token_type::op_and },
        { "\\/", token_type::op_or },
        { "|-", token_type::turnstile },
        { "¬", token_type::op_not },

        { "!", token_type::op_not },
        { "~", token_type::op_not },
        { "&", token_type::op_and },
        { "|", token_type::op_or },
        { "(", token_type::lparen },
        { ")", token_type::rparen },
        { ",", token_type::comma },
        { ".", token_type::dot }
    });

    constexpr auto keyword_table = std::to_array<std::pair<std::string_view, token_type>>({
        { "not", token_type::op_not },
        { "and", token_type::op_and },
        { "or", token_type::op_or },
        { "forall", token_type::kw_forall },
        { "exists", token_type::kw_exists },
        { "true", token_type::kw_true },
        { "false", token_type::kw_false }
    });

    export struct token
    {
        token_type type;
        std::string_view text;
        std::size_t pos;
    };

    export class lexer
    {
        private:
        std::string_view _src;
        std::size_t _pos;
        std::optional<token> _peeked;

        token lex_one();

        public:
        explicit lexer(std::string_view src) noexcept
            : _src { src }, _pos { 0 }, _peeked { } { }

        token next();
        token peek();
    };
} // namespace resol

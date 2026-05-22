// Copyright (C) 2026  ilobilo

module lexer;

import std;

namespace resol
{
    namespace
    {
        bool is_ident_start(char chr) noexcept
        {
            return (chr >= 'a' && chr <= 'z') || (chr >= 'A' && chr <= 'Z') || chr == '_';
        }

        bool is_ident_cont(char chr) noexcept
        {
            return is_ident_start(chr) || (chr >= '0' && chr <= '9');
        }
    } // namespace

    token lexer::lex_one()
    {
        while (_pos < _src.size())
        {
            const auto chr = _src[_pos];
            if (chr == ' ' || chr == '\t' || chr == '\n' || chr == '\r')
                _pos++;
            else
                break;
        }

        if (_pos >= _src.size())
            return { token_type::end, { }, _pos };

        const auto start = _pos;
        const auto rest = _src.substr(_pos);

        for (const auto &[sym, ty] : symbol_table)
        {
            if (rest.starts_with(sym))
            {
                _pos += sym.size();
                return { ty, _src.substr(start, sym.size()), start };
            }
        }

        if (is_ident_start(_src[_pos]))
        {
            auto end = _pos + 1;
            while (end < _src.size() && is_ident_cont(_src[end]))
                end++;

            const auto text = _src.substr(start, end - start);
            _pos = end;

            for (const auto &[kw, ty] : keyword_table)
            {
                if (kw == text)
                    return { ty, text, start };
            }

            return { token_type::ident, text, start };
        }

        _pos++;
        return { token_type::invalid, _src.substr(start, 1), start };
    }

    token lexer::next()
    {
        if (_peeked)
        {
            const auto tok = *_peeked;
            _peeked.reset();
            return tok;
        }
        return lex_one();
    }

    token lexer::peek()
    {
        if (!_peeked)
            _peeked = lex_one();
        return *_peeked;
    }
} // namespace resol

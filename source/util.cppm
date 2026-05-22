// Copyright (C) 2026  ilobilo

export module util;

export namespace resol
{
    template<typename ...Funcs>
    struct overloaded : Funcs... { using Funcs::operator()...; };

    template<typename ...Funcs>
    overloaded(Funcs ...) -> overloaded<Funcs...>;
} // export namespace resol

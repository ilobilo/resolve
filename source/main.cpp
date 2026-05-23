// Copyright (C) 2026  ilobilo

#include <argparse/argparse.hpp>

import resolve;
import magic_enum;
import std;

namespace
{
    constexpr std::string_view version_string = "v0.1";
    enum class subcommand { prove, nnf, cnf, dnf, unify };

    struct input_source
    {
        std::string text;
        std::string origin;
    };

    struct options
    {
        subcommand cmd;
        std::vector<input_source> inputs;
        bool trace = false;
        bool ascii;
        bool dump_clauses = false;
        std::size_t limit = 10000;
    };

    std::expected<std::string, std::string> read_file(std::string_view path)
    {
        std::ifstream file { std::string { path } };
        if (!file)
            return std::unexpected { std::format("cannot open '{}'", path) };
        std::ostringstream out;
        out << file.rdbuf();
        return std::move(out).str();
    }

    std::string read_stdin()
    {
        std::ostringstream out;
        out << std::cin.rdbuf();
        return std::move(out).str();
    }

    std::size_t input_count(subcommand cmd)
    {
        return cmd == subcommand::unify ? 2 : 1;
    }

    std::string_view describe(subcommand cmd)
    {
        using enum subcommand;
        switch (cmd)
        {
            case prove:
                return "Resolution refutation.";
            case nnf:
                return "Convert formula to Negation Normal Form.";
            case cnf:
                return "Convert formula to Conjunctive Normal Form.";
            case dnf:
                return "Convert formula to Disjunctive Normal Form.";
            case unify:
                return "Unify two formulas.";
        }
        std::unreachable();
    }

    std::expected<options, std::string> collect(argparse::ArgumentParser &sub, subcommand cmd)
    {
        options opts;
        opts.cmd = cmd;
        opts.ascii = sub.get<bool>("--ascii");

        for (const auto &expr : sub.get<std::vector<std::string>>("--expr"))
            opts.inputs.push_back({ expr, "<expr>" });

        for (const auto &path : sub.get<std::vector<std::string>>("files"))
        {
            auto content = read_file(path);
            if (!content)
                return std::unexpected { std::move(content).error() };
            opts.inputs.push_back({ std::move(*content), path });
        }

        if (cmd == subcommand::prove)
        {
            opts.trace = sub.get<bool>("--trace");
            opts.dump_clauses = sub.get<bool>("--clauses");
            opts.limit = sub.get<std::size_t>("--limit");
        }

        const auto needed = input_count(cmd);
        if (opts.inputs.empty() && needed == 1)
            opts.inputs.push_back({ read_stdin(), "<stdin>" });

        if (opts.inputs.size() != needed)
        {
            return std::unexpected { std::format(
                "subcommand '{}' expects {} input(s), got {}",
                magic_enum::enum_name(cmd), needed, opts.inputs.size()
            ) };
        }

        return opts;
    }

    int run_prove(const options &opts)
    {
        return 0;
    }

    int run_transform(const options &opts, subcommand cmd)
    {
        resol::parser parser { opts.inputs[0].text };
        auto formula = parser.parse_formula();
        if (!formula)
        {
            std::println(
                std::cerr, "error: {} at {}",
                formula.error().message, formula.error().pos
            );
            return 1;
        }

        formula = resol::to_nnf(*formula);
        if (cmd == subcommand::cnf)
            formula = resol::to_cnf(*formula);
        else if (cmd == subcommand::dnf)
            formula = resol::to_dnf(*formula);
        else if (cmd != subcommand::nnf)
        {
            std::println(std::cerr, "error: invalid subcommand");
            return 1;
        }

        const auto style = opts.ascii
            ? resol::print_style::ascii
            : resol::print_style::unicode;
        std::println("{}", to_string(*formula, style));
        return 0;
    }

    int run_unify(const options &opts)
    {
        resol::parser p1 { opts.inputs[0].text };
        resol::parser p2 { opts.inputs[1].text };

        auto lhs = p1.parse_formula();
        auto rhs = p2.parse_formula();

        const auto check = [](auto formula) {
            if (formula)
                return true;
            std::println(
                std::cerr, "error: {} at {}",
                formula.error().message, formula.error().pos
            );
            return false;
        };
        if (!check(lhs) || !check(rhs))
            return 1;

        const auto strip = [](resol::formula::ptr formula)
        {
            while (true)
            {
                if (const auto fa = std::get_if<resol::formula::forall_t>(formula.get()))
                    formula = fa->body;
                else if (const auto ex = std::get_if<resol::formula::exists_t>(formula.get()))
                    formula = ex->body;
                else
                    break;
            }
            return formula;
        };

        auto sub = resol::unify(strip(*lhs), strip(*rhs));
        if (!sub)
        {
            std::println(std::cerr, "error: {}", sub.error().message);
            return 1;
        }

        std::vector<std::pair<std::string, resol::term::ptr>> sorted { sub->begin(), sub->end() };
        std::ranges::sort(sorted, [](const auto &lhs, const auto &rhs) {
            return lhs.first < rhs.first;
        });

        for (const auto &[var, term] : sorted)
            std::println("{} -> {}", var, resol::to_string(term));
        return 0;
    }

    int dispatch(const options &opts)
    {
        using enum subcommand;
        switch (opts.cmd)
        {
            case prove:
                return run_prove(opts);
            case nnf:
            case cnf:
            case dnf:
                return run_transform(opts, opts.cmd);
            case unify:
                return run_unify(opts);
        }
        std::unreachable();
    }
} // namespace

int main(int argc, char **argv)
{
    argparse::ArgumentParser program { "resolve", std::string { version_string } };
    program.add_description("First-order logic resolution prover.");

    std::unordered_map<subcommand, std::unique_ptr<argparse::ArgumentParser>> subparsers;
    for (const auto value : magic_enum::enum_values<subcommand>())
    {
        auto parser = std::make_unique<argparse::ArgumentParser>(
            std::string { magic_enum::enum_name(value) },
            "", argparse::default_arguments::help
        );

        parser->add_description(std::string { describe(value) });
        parser->add_argument("-e", "--expr")
            .append()
            .default_value<std::vector<std::string>>({ })
            .metavar("EXPR")
            .help("read input from EXPR");

        parser->add_argument("-a", "--ascii")
            .flag()
            .help("emit ASCII characters (~, &, |, ->) instead of Unicode");

        parser->add_argument("files")
            .nargs(argparse::nargs_pattern::any)
            .default_value<std::vector<std::string>>({ })
            .metavar("FILE")
            .help("input file(s) or use stdin if -e is not specified");

        if (value == subcommand::prove)
        {
            parser->add_argument("-t", "--trace")
                .flag()
                .help("print every resolution step");
            parser->add_argument("--clauses")
                .flag()
                .help("dump CNF clauses");
            parser->add_argument("--limit")
                .scan<'i', std::size_t>()
                .default_value<std::size_t>(10000)
                .metavar("N")
                .help("max resolution iterations (default 10000)");
        }

        program.add_subparser(*parser);
        subparsers.emplace(value, std::move(parser));
    }

    std::vector<std::string> args;
    args.reserve(argc);
    for (int i = 0; i < argc; i++)
        args.emplace_back(argv[i]);

    const bool is_global_flag = args.size() > 1
        && (args[1] == "-h" || args[1] == "--help" || args[1] == "--version");
    const bool has_subcommand = args.size() > 1
        && magic_enum::enum_cast<subcommand>(args[1]).has_value();
    if (!has_subcommand && !is_global_flag)
    {
        std::cout << program;
        return 0;
    }

    try {
        program.parse_args(args);
    }
    catch (const std::exception &ex)
    {
        std::println(std::cerr, "resolve: {}", ex.what());
        return 1;
    }

    for (const auto &[cmd, sub] : subparsers)
    {
        if (!program.is_subcommand_used(*sub))
            continue;

        auto opts = collect(*sub, cmd);
        if (!opts)
        {
            std::println(std::cerr, "resolve: {}", opts.error());
            return 1;
        }
        return dispatch(*opts);
    }

    return 0;
}

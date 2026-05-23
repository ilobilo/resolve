// Copyright (C) 2026  ilobilo

#include <argparse/argparse.hpp>

import resolve;
import magic_enum;
import std;

namespace
{
    constexpr std::string_view version_string = "v0.1";
    enum class subcommand { prove, simplify, nnf, cnf, dnf, skolemise, unify };

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
            case simplify:
                return "Simplify formula.";
            case nnf:
                return "Convert formula to Negation Normal Form.";
            case cnf:
                return "Convert formula to Conjunctive Normal Form.";
            case dnf:
                return "Convert formula to Disjunctive Normal Form.";
            case skolemise:
                return "Convert formula to Skolem Normal Form.";
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

    std::expected<resol::formula::ptr, int> parse_one(const input_source &src)
    {
        resol::parser parser { src.text };
        auto formula = parser.parse_formula();
        if (!formula)
        {
            std::println(
                std::cerr, "error: {} at {} ({})",
                formula.error().message, formula.error().pos, src.origin
            );
            return std::unexpected { 1 };
        }
        return std::move(*formula);
    }

    resol::print_style style_for(const options &opts)
    {
        return opts.ascii ? resol::print_style::ascii : resol::print_style::unicode;
    }

    std::string strip_rename(std::string_view name)
    {
        const auto pos = name.find('#');
        return std::string { pos == std::string_view::npos ? name : name.substr(0, pos) };
    }

    resol::term::ptr strip_term(resol::term::ptr term)
    {
        return std::visit(resol::overloaded {
            [](const resol::term::variable_t &val) {
                return resol::make_var(strip_rename(val.name));
            },
            [&](const resol::term::constant_t &) {
                return term;
            },
            [](const resol::term::function_t &val) {
                std::vector<resol::term::ptr> args;
                args.reserve(val.args.size());
                for (const auto &arg : val.args)
                    args.push_back(strip_term(arg));
                return resol::make_func(val.name, std::move(args));
            }
        }, *term);
    }

    resol::clause strip_clause(const resol::clause &cl)
    {
        resol::clause out;
        out.reserve(cl.size());
        for (const auto &lit : cl)
        {
            std::vector<resol::term::ptr> args;
            args.reserve(lit.args.size());
            for (const auto &arg : lit.args)
                args.push_back(strip_term(arg));
            out.push_back({ lit.negated, lit.name, std::move(args) });
        }
        return out;
    }

    void print_trace(const resol::prove_result &result, resol::print_style style)
    {
        for (std::size_t i = 0; i < result.steps.size(); i++)
        {
            const auto &step = result.steps[i];
            const auto repr = to_string(strip_clause(step.result), style);
            if (step.parents.empty())
            {
                std::println("[{}] {}", i + 1, repr);
                continue;
            }

            std::string parents;
            for (std::size_t k = 0; k < step.parents.size(); k++)
            {
                if (k != 0)
                    parents += ", ";
                parents += std::to_string(step.parents[k] + 1);
            }

            std::string mgu_str;
            if (!step.mgu.empty())
            {
                std::vector<std::pair<std::string, resol::term::ptr>> sorted {
                    step.mgu.begin(), step.mgu.end()
                };
                std::ranges::sort(sorted, [](const auto &a, const auto &b) {
                    return a.first < b.first;
                });
                mgu_str = "; ";
                for (std::size_t k = 0; k < sorted.size(); k++)
                {
                    if (k != 0)
                        mgu_str += ", ";
                    mgu_str += std::format(
                        "{} -> {}",
                        strip_rename(sorted[k].first),
                        to_string(strip_term(sorted[k].second))
                    );
                }
            }

            std::println("[{}] {} (from {}{})", i + 1, repr, parents, mgu_str);
        }
    }

    int run_prove(const options &opts)
    {
        auto formula = parse_one(opts.inputs[0]);
        if (!formula)
            return formula.error();

        auto clauses = resol::extract_clauses(
            resol::to_cnf(
                resol::drop_universals(
                    resol::skolemise(
                        resol::normalise_apart(
                            resol::to_nnf(*formula)
                        )
                    )
                )
            )
        );

        const auto style = style_for(opts);

        if (opts.dump_clauses)
        {
            for (const auto &cl : clauses)
                std::println("{}", to_string(cl, style));
            return 0;
        }

        auto result = resol::run_resolution(std::move(clauses), opts.limit);
        if (opts.trace)
            print_trace(result, style);

        switch (result.status)
        {
            case resol::prove_status::unsatisfiable:
                std::println("unsatisfiable");
                return 0;
            case resol::prove_status::satisfiable:
                std::println("satisfiable");
                return 1;
            case resol::prove_status::limit_reached:
                std::println("limit reached");
                return 1;
        }
        std::unreachable();
    }

    int run_simplify(const options &opts)
    {
        auto formula = parse_one(opts.inputs[0]);
        if (!formula)
            return formula.error();

        std::println("{}", to_string(resol::simplify(*formula), style_for(opts)));
        return 0;
    }

    int run_transform(const options &opts, subcommand cmd)
    {
        auto parsed = parse_one(opts.inputs[0]);
        if (!parsed)
            return parsed.error();

        auto formula = resol::to_nnf(*parsed);
        switch (cmd)
        {
            case subcommand::nnf:
                formula = resol::simplify(formula);
                break;
            case subcommand::cnf:
                formula = resol::simplify(resol::to_cnf(formula));
                break;
            case subcommand::dnf:
                formula = resol::simplify(resol::to_dnf(formula));
                break;
            case subcommand::skolemise:
                formula = resol::skolemise(resol::normalise_apart(formula));
                break;
            default:
                std::println(std::cerr, "error: invalid subcommand");
                return 1;
        }

        std::println("{}", to_string(formula, style_for(opts)));
        return 0;
    }

    int run_unify(const options &opts)
    {
        auto lhs = parse_one(opts.inputs[0]);
        auto rhs = parse_one(opts.inputs[1]);
        if (!lhs)
            return lhs.error();
        if (!rhs)
            return rhs.error();

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
            case simplify:
                return run_simplify(opts);
            case nnf:
            case cnf:
            case dnf:
            case skolemise:
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
                .help("max resolution iterations");
        }

        program.add_subparser(*parser);
        subparsers.emplace(value, std::move(parser));
    }

    std::vector<std::string> args;
    args.reserve(argc);
    for (int i = 0; i < argc; i++)
        args.emplace_back(argv[i]);

    const bool is_global_flag = args.size() > 1 &&
        (args[1] == "-h" || args[1] == "--help" || args[1] == "--version");
    const bool has_subcommand = args.size() > 1 &&
        magic_enum::enum_cast<subcommand>(args[1]).has_value();
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

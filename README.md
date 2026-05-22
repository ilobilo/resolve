# resolve
First-order logic resolution prover. Uni CS project (wip)

## License: [EUPL v1.2](LICENSE)

## Build
Requires a modern C++23 compiler and [xmake](https://xmake.io). clang if building for Linux, mingw for windows.\
Only tested on ``fedora 44`` with ``clang 22.1.5`` and ``mingw64 16.1.1``

```sh
xmake build
```
the binary is located in ``build/<plat>/<arch>/release/resolve``.

## Usage
You can get the usage information with following commands:
```sh
resolve --help
resolve <subcommand> --help
```

## Syntax
In order of precedence:

| Name       | Unicode | ASCII |
|------------|---------|-------|
| not        | `¬`     | `~`, `!`, `not` |
| and        | `∧`     | `&`, `&&`, `/\`, `and` |
| or         | `∨`     | `\|`, `\|\|`, `\/`, `or` |
| implies    | `→`     | `->`, `=>` |
| iff        | `↔`     | `<->`, `<=>` |
| forall     | `∀`     | `forall` |
| exists     | `∃`     | `exists` |
| true       | `⊤`     | `true` |
| false      | `⊥`     | `false` |
| sequent    | `⊢`     | `\|-` |

`∀x. P(x) → Q(x)` means `∀x. (P(x) → Q(x))`.

## Examples

```sh
$ resolve nnf -e "¬(P → Q)"
P ∧ ¬Q

$ resolve cnf -e "(P ∧ Q) ∨ R"
(P ∨ R) ∧ (Q ∨ R)

$ resolve dnf -e "(P ∨ Q) ∧ R"
(P ∧ R) ∨ (Q ∧ R)

$ resolve nnf -a -e "¬forall x. P(x)"
exists x. ~P(x)
```

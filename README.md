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

### Normal forms
```sh
$ resolve nnf -e "¬(P → Q)"
P ∧ ¬Q

$ resolve nnf -e "P ↔ ¬Q"
(¬P ∨ ¬Q) ∧ (Q ∨ P)

$ echo "(P ∧ Q) ∨ R" | resolve cnf
(P ∨ R) ∧ (Q ∨ R)

$ echo "(P ∨ Q) ∧ R" > tmp.txt && resolve dnf tmp.txt && rm tmp.txt
P ∧ R ∨ Q ∧ R

$ resolve nnf -a -e "¬forall x. P(x)"
exists x. ~P(x)
```

### Simplification
```sh
$ resolve simplify -e "∀x. P(x) ∨ ¬P(x)"
∀x. ⊤

$ resolve simplify -e "(p ∨ ¬q ∨ (p ∨ ¬q)) ∧ (¬p ∧ q ∨ ¬p ∧ q)"
(p ∨ ¬q) ∧ (¬p ∧ q)
```

### Unification
`unify` takes two formulas and prints the most general unifier as `var -> term`.

```sh
$ resolve unify -e "∀x. P(x)" -e "P(a)"
x -> a

$ resolve unify -e "forall x. P(f(x))" -e "forall y. P(f(g(y)))"
x -> g(y)

$ resolve unify -e "∀x. P(f(x), x)" -e "∀y. P(y, a)"
x -> a
y -> f(a)

$ resolve unify -e "∀x. ∀y. P(f(x), y)" -e "forall z. P(z, a)"
y -> a
z -> f(x)

$ resolve unify -e "P(a)" -e "P(b)"
error: incompatible terms

$ resolve unify -e "∀x. P(x)" -e "∀x. P(f(x))"
error: occurs check failed: x appears in its own binding
```

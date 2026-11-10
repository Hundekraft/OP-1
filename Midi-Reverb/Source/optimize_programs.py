"""Remove redundant reads and stores within each native MidiVerb I tick."""
import ast
import re
import sys
from pathlib import Path

source = Path(sys.argv[1]).read_text()
pattern = r"(static int16_t prog\d+\([^\n]+\)\n\{\n)(.*?)(^\})"

def optimize(match):
    nodes = []
    cache = {}
    values = {"in": "in"}
    memory = {}
    stores = {}

    def add(text, deps=(), typ="int"):
        key = (text, tuple(deps), typ)
        if key not in cache:
            name = f"v{len(nodes)}"
            cache[key] = name
            nodes.append((name, f"{typ} {name} = {text};", tuple(deps)))
        return cache[key]

    def effect(text, deps):
        nodes.append((None, text, tuple(deps)))
        return len(nodes) - 1

    def at(expr):
        address = ast.unparse(expr.slice)
        match = re.fullmatch(r"base \+ (\d+) & 16383", address)
        if not match:
            raise ValueError(address)
        return int(match[1])

    def expr(node):
        if isinstance(node, ast.Name):
            return values[node.id]
        if isinstance(node, ast.Subscript) and node.value.id == "mem":
            offset = at(node)
            if offset not in memory:
                memory[offset] = add(f"mem[(base + {offset}) & 16383]", typ="int16_t")
            return memory[offset]
        if isinstance(node, ast.UnaryOp) and isinstance(node.op, ast.Invert):
            val = expr(node.operand)
            return add(f"~{val}", [val])
        if isinstance(node, ast.Call) and node.func.id == "half":
            arg = node.args[0]
            # The original sign-bit correction makes half(~x) == -half(x).
            if isinstance(arg, ast.UnaryOp) and isinstance(arg.op, ast.Invert):
                val = expr(arg.operand)
                part = add(f"half({val})", [val])
                return add(f"-{part}", [part])
            val = expr(arg)
            return add(f"half({val})", [val])
        if isinstance(node, ast.BinOp) and isinstance(node.op, ast.Add):
            a, b = expr(node.left), expr(node.right)
            return add(f"{a} + {b}", [a, b])
        raise ValueError(ast.dump(node))

    for line in match[2].splitlines():
        line = line.strip()
        if line.startswith("unsigned base"):
            continue
        if line.startswith("int16_t acc"):
            values["acc"] = add("*state", typ="int16_t")
            values["left"] = add("0", typ="int16_t")
            continue
        if line.startswith("*pointer"):
            effect(line, [])
            continue
        if line.startswith("*state"):
            val = values["acc"]
            effect(f"*state = {val};", [val])
            continue
        if line.startswith("return"):
            val = values["left"]
            effect(f"return {val};", [val])
            continue
        target, rhs = line.rstrip(";").split("=", 1)
        target, rhs = target.strip(), rhs.strip()
        val = values["in"] if rhs == "in" else expr(ast.parse(rhs, mode="eval").body)
        # Hardware wraps on every accumulator assignment and DRAM store.
        if rhs not in values and not rhs.startswith("mem["):
            val = add(f"(int16_t){val}", [val], typ="int16_t")
        if target in ("acc", "left"):
            values[target] = val
        elif target.startswith("mem["):
            offset = at(ast.parse(target, mode="eval").body)
            memory[offset] = val
            if offset in stores:
                nodes[stores[offset]] = (None, "", ())
            stores[offset] = effect(f"mem[(base + {offset}) & 16383] = {val};", [val])
        else:
            raise ValueError(target)

    used = set()
    out = []
    for name, line, deps in reversed(nodes):
        if line and (name is None or name in used):
            out.append("    " + line)
            used.update(deps)
    return match[1] + "    unsigned base = *pointer;\n" + "\n".join(reversed(out)) + "\n" + match[3]

result, count = re.subn(pattern, optimize, source, flags=re.M | re.S)
assert count == 63
Path(sys.argv[2]).write_text(result)

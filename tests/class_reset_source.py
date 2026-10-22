"""Exact inverse of the reviewed reset ownership and hybrid-state edits."""
import hashlib

BEFORE="84754f0847dfdce18325f247ece2fe4ea9ff7e9e2e978e8ef9655a39d4961684"
AFTER="f8788c39c07bee30407c4f47e936558366c82d3bcc40abf4027415e14659f9e4"
HISTORICAL="cc3145c446d9b1f41a3e3360c32f2dd0751a5cb552abf4426f1b3eb0e600034f"
EDITS=[
    (229,232,""),(73,74,""),(71,72,""),(69,70,""),(67,68,""),
    (65,66,""),(63,64,""),(61,62,""),(59,60,""),(27,32,""),
    (11,13,"    one_argument( argument, arg );\n"),(4,5,""),(1,2,""),
]
def without_class_reset_stage(text):
    if "void do_clearclass(" not in text:return text
    digest=hashlib.sha256(text.encode()).hexdigest()
    if digest in (BEFORE,HISTORICAL):return text
    assert digest==AFTER,("unexpected class reset edit",digest)
    lines=text.splitlines(keepends=True)
    for start,end,old in EDITS:lines[start:end]=old.splitlines(keepends=True)
    restored="".join(lines)
    assert hashlib.sha256(restored.encode()).hexdigest()==BEFORE
    return restored

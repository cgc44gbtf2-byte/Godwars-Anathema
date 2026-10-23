"""Check sanitizer startup separately from MUD tests; never retry a MUD test.

Old GCC ASan/PIE can fail before main on high-entropy Linux hosts:
https://github.com/google/sanitizers/issues/1614
PIE launches below are diagnostic only. Every configured non-PIE launch must
succeed, and deliberately invalid programs must still trigger ASan and UBSan.
Normal production builds retain their platform's default PIE settings.
"""
import os
from pathlib import Path
import resource
import shlex
import subprocess
import tempfile


def no_core():
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))


def main():
    env = dict(os.environ, ASAN_OPTIONS="detect_leaks=0:halt_on_error=1",
               UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    cc = shlex.split(os.environ.get("CC", "cc"))
    with tempfile.TemporaryDirectory(prefix="anathema-sanitizer-") as directory:
        root = Path(directory)
        source = root / "probe.c"
        source.write_text("int main(void) { return 0; }\n")

        def build(name, extra=(), sanitizers="address,undefined"):
            binary = root / name
            subprocess.run([*cc, "-std=gnu17", "-O1", "-g",
                            "-fsanitize=" + sanitizers, "-fno-omit-frame-pointer",
                            *extra, str(source), "-o", str(binary)], check=True)
            return binary

        def run(binary, *args):
            return subprocess.run([str(binary), *args], env=env, timeout=15,
                                  stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                  preexec_fn=no_core)

        pie = build("pie", ("-fPIE", "-pie"))
        fixed = build("fixed", ("-fno-pie", "-no-pie"))
        failures = 0
        for _ in range(50):
            failures += run(pie).returncode != 0
        print(f"Diagnostic PIE startup: {failures}/50 unsuccessful launches", flush=True)
        for _ in range(100):
            result = run(fixed)
            assert result.returncode == 0, result.stderr.decode(errors="replace")
        header = subprocess.check_output(["readelf", "-h", str(fixed)], text=True)
        assert "EXEC (Executable file)" in header
        source.write_text(
            "#include <limits.h>\n#include <stdlib.h>\n"
            "int main(int argc, char **argv) {\n"
            " (void)argv;\n"
            " if (argc > 1) { volatile int n = INT_MAX; return n + 1; }\n"
            " volatile char *p = malloc(1); p[1] = 'x'; free((void *)p); return 0;\n"
            "}\n"
        )
        invalid = build("invalid_asan", ("-fno-pie", "-no-pie"), "address")
        result = run(invalid)
        assert result.returncode != 0 and b"heap-buffer-overflow" in result.stderr, (
            result.returncode, result.stderr.decode(errors="replace"))
        invalid = build("invalid_ubsan", ("-fno-pie", "-no-pie"), "undefined")
        result = run(invalid, "ubsan")
        assert result.returncode != 0 and b"signed integer overflow" in result.stderr, (
            result.returncode, result.stderr.decode(errors="replace"))
        print("100 configured sanitizer startups passed; ASan and UBSan detect "
              "deliberate violations. No game-test retries or suppressed errors.", flush=True)


if __name__ == "__main__":
    main()

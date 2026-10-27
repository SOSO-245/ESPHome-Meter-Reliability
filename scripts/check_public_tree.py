"""Reject unexpected files, binaries, local network IDs and machine paths."""

from pathlib import Path
import re
import subprocess

root = Path(__file__).resolve().parents[1]
allowed = {
    ".gitignore", "LICENSE", "README.md", "THIRD_PARTY_NOTICES.md", "requirements.txt",
    "secrets.example.yaml", "meter.yaml", "daily_ledger.h",
    "tests/daily_ledger_test.cpp", "scripts/init_secrets.py", "scripts/check_public_tree.py",
    ".github/ISSUE_TEMPLATE/bug.yml",
}
if (root / ".git").exists():
    names = subprocess.check_output(["git", "ls-files", "--cached"], cwd=root, text=True).splitlines()
else:
    names = [p.relative_to(root).as_posix() for p in root.rglob("*") if p.is_file()]
assert set(names) == allowed, "Unexpected or missing public file"
patterns = [
    re.compile(r"\b192\.168\.\d+\.\d+\b"),
    re.compile(r"\b(?:[0-9a-fA-F]{2}:){5}[0-9a-fA-F]{2}\b"),
    re.compile(r"/(?:Users|private/tmp)/[^\s]+"),
]
for name in names:
    path = root / name
    assert not path.is_symlink() and path.stat().st_size < 100_000, "Non-source payload"
    content = path.read_text()
    assert not any(pattern.search(content) for pattern in patterns), f"Private identifier in {name}"
print(f"{len(names)} source-only files passed public-tree checks")

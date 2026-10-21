"""Reject unreviewed changes to the native state traversed by the clone."""
from hashlib import sha256
import json
from pathlib import Path

root = Path(__file__).resolve().parents[2]
expected = json.loads(Path(__file__).with_name("layout.json").read_text())
changed = [name for name, digest in expected.items()
           if sha256((root / name).read_text().encode()).hexdigest() != digest]
if changed:
    raise SystemExit("Review clone field handlers before updating layout.json: " + ", ".join(changed))
print("Clone layout manifest matches reviewed native headers.")

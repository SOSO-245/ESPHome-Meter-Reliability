"""Generate local-only management keys without printing them."""

import base64
import os
from pathlib import Path
import secrets

import yaml

root = Path(__file__).resolve().parents[1]
target = root / "secrets.yaml"
config = {
    "wifi_ssid": "CHANGE_ME",
    "wifi_password": "CHANGE_ME",
    "api_encryption_key": base64.b64encode(secrets.token_bytes(32)).decode(),
    "ota_password": secrets.token_urlsafe(24),
    "fallback_ap_password": secrets.token_urlsafe(24),
}
try:
    fd = os.open(target, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
except FileExistsError:
    raise SystemExit("secrets.yaml already exists; left unchanged") from None
with os.fdopen(fd, "w") as output:
    yaml.safe_dump(config, output, sort_keys=False)
print("Created local secrets.yaml. Fill Wi-Fi fields locally; never commit this file.")

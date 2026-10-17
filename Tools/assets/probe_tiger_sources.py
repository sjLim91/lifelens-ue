"""Read public source metadata; never authenticate, scrape viewer files or import assets.

Probe success means evidence was collected, not that an asset passed admission.
Every request is bounded. The report keeps acquisition/visual gates closed.
"""
from __future__ import annotations

import argparse
import datetime as dt
import hashlib
import html
import json
import pathlib
import re
import urllib.error
import urllib.request

MODELS = {
    "white-tiger-derivative": "9dd099d283e54f99b7cbd40b531b1a29",
    "declared-original": "51ed5186afb04487ae6adb51f8ffd09b",
    "orange-tiger-alternative": "0984e2d5f9ef44f7bdc50521be1fa176",
}
MAX_BYTES = 2 * 1024 * 1024


def inspect_public_cc0_page() -> dict:
    """Inspect an advertised public download page, without accessing viewer storage."""
    url = "https://3dassets.dev/assets/exotic-wildlife-hd-bengal-tiger-prowling-b1de9790"
    result = {"source_url": url, "asset_acquired": False, "admission": "pending-page-and-file-review"}
    try:
        request = urllib.request.Request(url, headers={"User-Agent": "LifeLens-source-review/1.0"})
        with urllib.request.urlopen(request, timeout=15) as response:
            body = response.read(MAX_BYTES + 1)
            if len(body) > MAX_BYTES:
                return {**result, "status": "oversized-response"}
            page = html.unescape(body.decode("utf-8"))
            result.update(http_status=response.status, response_sha256=hashlib.sha256(body).hexdigest())
            # Public links and embedded structured data are review evidence only.
            # Do not follow a file URL before checking the complete page's license.
            result["links"] = sorted(set(re.findall(r'https://[^\s<>"\\]+', page)))[:200]
            result["page_text"] = re.sub(r"<[^>]+>", " ", page)[-18000:]
    except urllib.error.HTTPError as error:
        result.update(http_status=error.code, status="http-error")
    except (urllib.error.URLError, TimeoutError, OSError, UnicodeDecodeError) as error:
        result.update(status="connection-failed", error_type=type(error).__name__)
    return result


def read_public(url: str) -> dict:
    request = urllib.request.Request(url, headers={"User-Agent": "LifeLens-source-review/1.0"})
    try:
        with urllib.request.urlopen(request, timeout=15) as response:
            body = response.read(MAX_BYTES + 1)
            if len(body) > MAX_BYTES:
                return {"url": url, "status": "oversized-response"}
            result = {"url": url, "http_status": response.status,
                      "response_sha256": hashlib.sha256(body).hexdigest()}
            try:
                result["data"] = json.loads(body)
            except (UnicodeDecodeError, json.JSONDecodeError):
                result["status"] = "non-json-response"
            return result
    except urllib.error.HTTPError as error:
        return {"url": url, "http_status": error.code,
                "status": "authentication-required" if error.code in (401, 403) else "http-error"}
    except (urllib.error.URLError, TimeoutError, OSError) as error:
        return {"url": url, "status": "connection-failed", "error_type": type(error).__name__}


def summarize(name: str, uid: str) -> dict:
    api = "https://api.sketchfab.com/v3/models/" + uid
    metadata = read_public(api)
    data = metadata.pop("data", {})
    license_data = data.get("license") or {}
    user = data.get("user") or {}
    # Keep exact evidence rather than inferring a license version from 'CC BY'.
    return {
        "candidate": name, "uid": uid,
        "source_url": data.get("viewerUrl", "https://sketchfab.com/models/" + uid),
        "metadata_request": metadata,
        "creator": {key: user.get(key) for key in ("uid", "username", "displayName", "profileUrl")},
        "license": license_data,
        "description": data.get("description"),
        "is_downloadable": data.get("isDownloadable"),
        "triangles": data.get("faceCount"), "vertices": data.get("vertexCount"),
        "animation_count": data.get("animationCount"),
        "published_at": data.get("publishedAt"),
        "anonymous_download_request": read_download_metadata(api + "/download"),
        "asset_acquired": False, "asset_sha256": None, "downloaded_at": None,
        "admission": "blocked-pending-source-chain-and-file-review",
    }


def read_download_metadata(url: str) -> dict:
    result = read_public(url)
    # Signed archive URLs can contain credentials. Record only format and size;
    # do not print, persist or follow them before source-chain review.
    data = result.pop("data", {})
    result["formats"] = {key: {"size": value.get("size"), "expires": value.get("expires")}
                         for key, value in data.items() if isinstance(value, dict)}
    return result


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=pathlib.Path, required=True)
    args = parser.parse_args()
    report = {
        "inspected_at": dt.datetime.now(dt.timezone.utc).isoformat(),
        "purpose": "public-source-evidence-only",
        "source_chain": {"9dd099d283e54f99b7cbd40b531b1a29": "51ed5186afb04487ae6adb51f8ffd09b"},
        "candidates": [summarize(name, uid) for name, uid in MODELS.items()],
        "public_cc0_page": inspect_public_cc0_page(),
        "quality_gate": "NOT_RUN", "web_integration": "NOT_IMPLEMENTED",
        "mobile_performance": "NOT_MEASURED", "merge_allowed": False,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    for candidate in report["candidates"]:
        print(candidate["candidate"], candidate["metadata_request"].get("http_status"),
              candidate["license"].get("slug"), candidate["anonymous_download_request"].get("status", "response-collected"))
    print("PUBLIC_CC0_PAGE", json.dumps(report["public_cc0_page"], ensure_ascii=False))
    print("Evidence collected; no asset acquired; visual/web/mobile gates remain open.")


if __name__ == "__main__":
    main()

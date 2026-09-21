#!/usr/bin/env python3
from http.server import ThreadingHTTPServer, SimpleHTTPRequestHandler
from pathlib import Path
import os

ROOT = Path(__file__).resolve().parents[1]
WEB = ROOT / "Clients" / "Web"
PORT = int(os.environ.get("LIFELENS_WEB_PORT", "4173"))


class Handler(SimpleHTTPRequestHandler):
    def end_headers(self):
        # Keep cross-origin isolation ready for future WASM threading without
        # requiring any server-side framework.
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        super().end_headers()


if __name__ == "__main__":
    os.chdir(WEB)
    server = ThreadingHTTPServer(("127.0.0.1", PORT), Handler)
    print(f"LifeLens Web: http://127.0.0.1:{PORT}")
    server.serve_forever()

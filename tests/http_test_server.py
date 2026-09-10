#!/usr/bin/env python3
"""Simple HTTP server for testing"""
import http.server
import json
import sys

class TestHandler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("X-Test-Header", "test-value")
        self.end_headers()
        self.wfile.write(json.dumps({"status": "ok", "path": self.path}).encode())
    
    def do_POST(self):
        length = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(length) if length else b""
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.end_headers()
        self.wfile.write(json.dumps({"received": len(body)}).encode())
    
    def log_message(self, format, *args):
        pass  # Suppress logging

if __name__ == "__main__":
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 18923
    server = http.server.HTTPServer(("127.0.0.1", port), TestHandler)
    print(f"Server running on port {port}", flush=True)
    server.serve_forever()

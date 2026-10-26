#!/usr/bin/env python3
"""Small end-to-end test for the newline-delimited JSON TCP protocol."""
import json
import socket
import subprocess
import sys
import time

SERVER = sys.argv[1]


def free_port():
    with socket.socket() as s:
        s.bind(("127.0.0.1", 0))
        return s.getsockname()[1]


class Client:
    def __init__(self, port, nick):
        self.sock = socket.create_connection(("127.0.0.1", port), timeout=3)
        self.sock.settimeout(3)
        self.buffer = b""
        # Server sends a greeting before the client says hello.
        self.receive_until(lambda msg: msg.get("type") == "system")
        self.send({"type": "hello", "nick": nick})
        self.receive_until(lambda msg: msg.get("type") == "welcome")

    def send(self, obj):
        self.sock.sendall(json.dumps(obj).encode("utf-8") + b"\n")

    def receive(self):
        while b"\n" not in self.buffer:
            chunk = self.sock.recv(4096)
            if not chunk:
                raise AssertionError("server closed the connection unexpectedly")
            self.buffer += chunk
        line, self.buffer = self.buffer.split(b"\n", 1)
        return json.loads(line.decode("utf-8"))

    def receive_until(self, predicate, timeout=4):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            msg = self.receive()
            if predicate(msg):
                return msg
        raise AssertionError("expected message was not received")

    def close(self):
        self.sock.close()


def main():
    port = free_port()
    server = subprocess.Popen([SERVER, "--port", str(port)], stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    clients = []
    try:
        deadline = time.monotonic() + 5
        while time.monotonic() < deadline:
            try:
                probe = socket.create_connection(("127.0.0.1", port), timeout=0.2)
                probe.close()
                break
            except OSError:
                if server.poll() is not None:
                    raise AssertionError("server process exited before listening")
                time.sleep(0.05)
        else:
            raise AssertionError("server did not start listening")

        a = Client(port, "Alice")
        clients.append(a)
        b = Client(port, "Bob")
        clients.append(b)

        a.send({"type": "message", "text": "hello from Alice"})
        got = b.receive_until(lambda m: m.get("type") == "message" and m.get("text") == "hello from Alice")
        assert got["nick"] == "Alice"

        b.send({"type": "join", "room": "games"})
        b.receive_until(lambda m: m.get("type") == "room" and m.get("room") == "games")
        b.send({"type": "message", "text": "in games"})
        got = b.receive_until(lambda m: m.get("type") == "message" and m.get("text") == "in games")
        assert got["room"] == "games"
        print("Integration test passed")
    finally:
        for client in clients:
            client.close()
        server.terminate()
        try:
            server.wait(timeout=3)
        except subprocess.TimeoutExpired:
            server.kill()
            server.wait(timeout=3)


if __name__ == "__main__":
    main()

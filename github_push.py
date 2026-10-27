#!/usr/bin/env python3
"""Stage configured files, create a commit, find a reachable GitHub IP, and push."""

import socket
import ssl
from pathlib import Path
import subprocess
import sys


# Edit these values before each run.
ADD_PATHS = [
    "GITHUB_PUSH_WSL.md","github_push.py"
]
COMMIT_MESSAGE = "Add GITHUB_PUSH_WSL.md and github_push.py"
PUSH_REMOTE = "origin"
PUSH_BRANCH = "lab1"

REPOSITORY = Path(__file__).resolve().parent
GITHUB_HOST = "github.com"
GITHUB_PORT = 443
# Fallbacks are only used when the local DNS result is unreachable.
FALLBACK_GITHUB_IPS = (
    "140.82.112.3",
    "140.82.114.4",
)


def run_git(*args: str) -> None:
    command = ["git", *args]
    print("$", " ".join(command))
    result = subprocess.run(command, cwd=REPOSITORY)
    if result.returncode != 0:
        raise SystemExit(
            f"git command failed with exit code {result.returncode}: "
            f"{' '.join(command)}"
        )


def github_candidates() -> list[str]:
    candidates = []
    try:
        addresses = socket.getaddrinfo(
            GITHUB_HOST, GITHUB_PORT, socket.AF_INET, socket.SOCK_STREAM
        )
        candidates.extend(address[4][0] for address in addresses)
    except socket.gaierror:
        pass

    for address in FALLBACK_GITHUB_IPS:
        if address not in candidates:
            candidates.append(address)
    return list(dict.fromkeys(candidates))


def find_reachable_github_ip() -> str:
    context = ssl.create_default_context()
    for address in github_candidates():
        try:
            with socket.create_connection(
                (address, GITHUB_PORT), timeout=5
            ) as connection:
                with context.wrap_socket(
                    connection, server_hostname=GITHUB_HOST
                ):
                    print(f"Using reachable GitHub address: {address}")
                    return address
        except (OSError, ssl.SSLError) as error:
            print(f"GitHub address {address} unavailable: {error}")

    raise SystemExit(
        "No reachable GitHub HTTPS address found. Check the network connection."
    )


def main() -> None:
    if not ADD_PATHS:
        raise SystemExit("ADD_PATHS is empty; configure files to stage first.")
    if not COMMIT_MESSAGE.strip():
        raise SystemExit("COMMIT_MESSAGE is empty; configure it first.")

    github_ip = find_reachable_github_ip()
    run_git("rev-parse", "--show-toplevel")
    run_git("add", "--", *ADD_PATHS)
    run_git("status", "--short")
    run_git("commit", "-m", COMMIT_MESSAGE)
    run_git(
        "-c",
        f"http.curloptResolve={GITHUB_HOST}:{GITHUB_PORT}:{github_ip}",
        "push",
        PUSH_REMOTE,
        PUSH_BRANCH,
    )
    print("Push completed.")


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\nInterrupted.", file=sys.stderr)
        raise SystemExit(130)

#!/usr/bin/env python3
"""
LocalPDub CLI Interaction Script
Interacts with the LocalPDub password manager CLI programmatically
"""

import subprocess
import time
from typing import List, Dict, Optional

class LocalPDubCLI:
    def __init__(self, binary_path: str = "./build/localpdub", master_password: str = "testtest123"):
        self.binary_path = binary_path
        self.master_password = master_password
        self.process = None

    def start(self):
        """Start the LocalPDub CLI process"""
        self.process = subprocess.Popen(
            [self.binary_path],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=0
        )
        time.sleep(0.5)

        # Enter master password
        self._send_input(self.master_password)
        time.sleep(0.5)

    def _send_input(self, text: str):
        """Send input to the CLI"""
        if self.process and self.process.stdin:
            self.process.stdin.write(text + "\n")
            self.process.stdin.flush()

    def _read_output(self, timeout: float = 1.0) -> str:
        """Read output from the CLI (non-blocking)"""
        # Note: This is simplified - real implementation would need proper async handling
        import select
        if self.process and self.process.stdout:
            ready, _, _ = select.select([self.process.stdout], [], [], timeout)
            if ready:
                return self.process.stdout.read()
        return ""

    def add_entry(self, title: str, username: str, password: str = "",
                  url: str = "", email: str = "", notes: str = "",
                  custom_fields: Optional[Dict[str, str]] = None):
        """Add a new password entry"""
        # Select add entry option
        self._send_input("3")
        time.sleep(0.2)

        # Enter details
        self._send_input(title)
        self._send_input(username)
        self._send_input(password)  # Empty = generate
        self._send_input(url)
        self._send_input(email)
        self._send_input(notes)

        # Handle custom fields
        if custom_fields:
            self._send_input("y")
            for key, value in custom_fields.items():
                self._send_input(key)
                self._send_input(value)
            self._send_input("done")
        else:
            self._send_input("n")

        time.sleep(0.2)

    def list_entries(self):
        """List all entries"""
        self._send_input("1")
        time.sleep(0.2)
        return self._read_output()

    def search_entries(self, query: str):
        """Search for entries"""
        self._send_input("2")
        time.sleep(0.2)
        self._send_input(query)
        time.sleep(0.2)
        return self._read_output()

    def view_entry(self, entry_number: int, reveal_password: bool = False):
        """View details of a specific entry"""
        self._send_input("4")
        time.sleep(0.2)
        self._send_input(str(entry_number))
        time.sleep(0.2)

        if reveal_password:
            self._send_input("r")
            time.sleep(0.2)
        else:
            self._send_input("n")
            time.sleep(0.2)

        return self._read_output()

    def generate_password(self, length: int = 20, uppercase: bool = True,
                         lowercase: bool = True, numbers: bool = True,
                         symbols: bool = True) -> str:
        """Generate a password"""
        self._send_input("7")
        time.sleep(0.2)

        self._send_input(str(length))
        self._send_input("y" if uppercase else "n")
        self._send_input("y" if lowercase else "n")
        self._send_input("y" if numbers else "n")
        self._send_input("y" if symbols else "n")
        self._send_input("n")  # Don't copy

        time.sleep(0.2)
        output = self._read_output()

        # Parse password from output
        # This would need proper parsing logic
        return output

    def save_and_exit(self):
        """Save vault and exit"""
        self._send_input("8")
        time.sleep(0.5)
        if self.process:
            self.process.wait(timeout=2)

    def exit_without_saving(self):
        """Exit without saving"""
        self._send_input("9")
        self._send_input("y")  # Confirm
        time.sleep(0.5)
        if self.process:
            self.process.wait(timeout=2)

# Example usage
if __name__ == "__main__":
    # Create CLI instance
    cli = LocalPDubCLI()

    print("Starting LocalPDub CLI interaction...")
    cli.start()

    print("\nAdding test entries...")

    # Add some test entries
    cli.add_entry(
        title="GitHub Account",
        username="testuser",
        password="",  # Generate
        url="https://github.com",
        email="test@example.com",
        notes="Main development account",
        custom_fields={
            "2FA Backup": "ABC123DEF456",
            "Recovery Email": "backup@example.com"
        }
    )

    cli.add_entry(
        title="Gmail Account",
        username="test@gmail.com",
        password="SecurePass123!",
        url="https://gmail.com",
        notes="Personal email"
    )

    # List all entries
    print("\nListing entries...")
    entries = cli.list_entries()
    print(entries)

    # Search
    print("\nSearching for 'gmail'...")
    results = cli.search_entries("gmail")
    print(results)

    # Save and exit
    print("\nSaving and exiting...")
    cli.save_and_exit()

    print("Done!")
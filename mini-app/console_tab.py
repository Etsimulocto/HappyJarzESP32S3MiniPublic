#!/usr/bin/env python3
"""QUARTERBITGAMES Console tab for the HAPPY JARZ desktop app.

This first iteration is a read-only build dashboard. It must not probe the
serial port behind the HAPPY JARZ link, flash firmware, or rewrite working
HAPPY JARZ subsystems. Cat Edition target: ESP32-S3 N16R8.
"""

from __future__ import annotations

import tkinter as tk
from tkinter import ttk

COMPONENTS = (
    ("ESP32-S3 N16R8", "VERIFIED", "240 MHz; 16 MB flash; 8 MB OPI PSRAM"),
    ("USB CDC/JTAG", "VERIFIED", "Native USB serial tested on Raspberry Pi"),
    ("ILI9488 TFT", "PENDING", '3.5 inch, 480 x 320; SPI diagnostics needed'),
    ("SSD1306 OLED", "EXISTING", "128 x 64; I2C SDA GPIO8, SCL GPIO6"),
    ("APA106 lights", "EXISTING", "Keep known-good HAPPY JARZ RMT driver"),
    ("Capacitive buttons", "EXISTING", "UP 4, DOWN 5, LEFT 9, RIGHT 10, A 1, B 2"),
    ("8BitDo gamepads", "PENDING", "Verify controller modes and two-player USB host"),
    ("microSD storage", "PLANNED", "3.3 V SPI module; GPIO map not yet assigned"),
    ("Battery charging", "PLANNED", "External 1S protection + charger + load sharing"),
)

def build_console_tab(root, card_factory):
    outer, panel = card_factory(root, 12)
    outer.pack(fill="both", expand=True)

    ttk.Label(panel, text="QUARTERBITGAMES CONSOLE — CAT EDITION",
              style="Section.TLabel").pack(anchor="w")
    ttk.Label(panel,
              text="ESP32-S3 only • two displays • HAPPY JARZ preserved • gamepad-ready design",
              style="PanelMuted.TLabel").pack(anchor="w", pady=(4, 10))

    status = ttk.Frame(panel, style="Panel.TFrame")
    status.pack(fill="x", pady=(0, 9))
    ttk.Label(status, text="BUILD STATUS: HARDWARE BRING-UP",
              style="PanelMuted.TLabel").pack(side="left")

    columns = ("component", "state", "notes")
    tree = ttk.Treeview(panel, columns=columns, show="headings", height=9)
    tree.heading("component", text="COMPONENT")
    tree.heading("state", text="STATE")
    tree.heading("notes", text="DETAIL")
    tree.column("component", width=190, minwidth=130, stretch=False)
    tree.column("state", width=90, minwidth=75, stretch=False)
    tree.column("notes", width=490, minwidth=230, stretch=True)
    for item in COMPONENTS:
        tree.insert("", tk.END, values=item)
    tree.pack(fill="both", expand=True)

    footer = ttk.Frame(panel, style="Panel.TFrame")
    footer.pack(fill="x", pady=(10, 0))
    ttk.Label(footer,
              text="Next: validate ILI9488 RGB/geometry, then touch; reserve SPI and SD GPIOs.",
              style="PanelMuted.TLabel", wraplength=750).pack(anchor="w")
    ttk.Label(footer,
              text="Read-only dashboard. No firmware or serial commands are sent from this tab.",
              style="PanelMuted.TLabel").pack(anchor="w", pady=(5, 0))

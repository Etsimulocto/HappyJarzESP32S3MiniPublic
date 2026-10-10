#!/usr/bin/env python3
"""HAPPY JARZ Controller release UI — current Fuel Gauge power telemetry layer."""

from __future__ import annotations

from pathlib import Path
from datetime import date
import json
import time
import tkinter as tk
from tkinter import ttk

import controller_extensions as previous
from art_studio import HappyJarzArtStudio


def _read_app_version() -> str:
    version_file = Path(__file__).resolve().with_name("VERSION")
    try:
        value = version_file.read_text(encoding="utf-8").strip()
        return value or "UNKNOWN"
    except OSError:
        return "UNKNOWN"


APP_VERSION = _read_app_version()


class HappyJarzApp(previous.HappyJarzApp):
    def __init__(self):
        self.art_studio = None
        self.power_voltage = None
        self.power_percent = None
        self.power_usb = None
        self.power_sensor = None
        self.power_charge = None
        self.power_adc = None
        self._power_poll_started = False
        super().__init__()
        self.title(f"HAPPY JARZ Controller v{APP_VERSION}")
        self.profile_owner_var = tk.StringVar(master=self, value="")
        self.profile_birthday_var = tk.StringVar(master=self, value="")
        self.profile_chip_var = tk.StringVar(master=self, value="Connect a Jar to identify its chip")
        self.profile_countdown_var = tk.StringVar(master=self, value="Enter an owner's birthday to see the countdown.")
        self.profile_state_var = tk.StringVar(master=self, value="Profiles are stored locally and on each Jar.")
        self._profile_store_path = Path.home() / ".happyjarz" / "device_profiles.json"
        self._device_profiles = self._load_profile_store()
        self._active_chip_id = ""
        self._profile_pending = None
        self._build_profile_tab()
        self._build_art_tab()
        self.after(500, self._refresh_birthday_countdown)
        self._log(f"HAPPY JARZ app v{APP_VERSION} • Owner profile + birthday celebration + art studio active")

    def _load_profile_store(self):
        try:
            value = json.loads(self._profile_store_path.read_text(encoding="utf-8"))
            return value if isinstance(value, dict) else {}
        except (OSError, ValueError):
            return {}

    def _write_profile_store(self):
        self._profile_store_path.parent.mkdir(parents=True, exist_ok=True)
        temporary = self._profile_store_path.with_suffix(".tmp")
        temporary.write_text(json.dumps(self._device_profiles, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
        temporary.replace(self._profile_store_path)

    def _build_profile_tab(self):
        profile_tab = ttk.Frame(self.notebook, padding=8)
        self.notebook.insert(3, profile_tab, text="PROFILE")

        outer, panel = self._card(profile_tab, 14)
        outer.pack(fill="x", anchor="n")
        head = ttk.Frame(panel, style="Panel.TFrame")
        head.pack(fill="x")
        ttk.Label(head, text="OWNER PROFILE", style="Section.TLabel").pack(side="left")
        ttk.Label(head, textvariable=self.profile_chip_var, style="PanelMuted.TLabel").pack(side="right")
        ttk.Label(
            panel,
            text="Personalize this Jar for its owner. The profile is saved on this computer under the Jar's unique chip ID and in the Jar's settings memory.",
            style="PanelMuted.TLabel",
            wraplength=820,
        ).pack(anchor="w", pady=(7, 12))

        form = ttk.Frame(panel, style="Panel.TFrame")
        form.pack(fill="x")
        ttk.Label(form, text="Owner's name", style="PanelMuted.TLabel").grid(row=0, column=0, sticky="w", pady=5)
        ttk.Entry(form, textvariable=self.profile_owner_var, width=36).grid(row=0, column=1, sticky="ew", padx=(12, 0), pady=5)
        ttk.Label(form, text="Birthday (MM-DD)", style="PanelMuted.TLabel").grid(row=1, column=0, sticky="w", pady=5)
        ttk.Entry(form, textvariable=self.profile_birthday_var, width=12).grid(row=1, column=1, sticky="w", padx=(12, 0), pady=5)
        form.columnconfigure(1, weight=1)

        outer, countdown = self._card(profile_tab, 14)
        outer.pack(fill="x", pady=(10, 0))
        ttk.Label(countdown, text="BIRTHDAY COUNTDOWN", style="Section.TLabel").pack(anchor="w")
        ttk.Label(countdown, textvariable=self.profile_countdown_var, style="Value.TLabel").pack(anchor="w", pady=(8, 0))
        ttk.Label(
            countdown,
            text="On the birthday, the Jar shows a greeting on its OLED and runs a birthday light pattern while its clock has the right date.",
            style="PanelMuted.TLabel",
            wraplength=820,
        ).pack(anchor="w", pady=(7, 0))

        row = ttk.Frame(panel, style="Panel.TFrame")
        row.pack(fill="x", pady=(14, 0))
        ttk.Button(row, text="SAVE PROFILE + SYNC CLOCK", style="Accent.TButton",
                   command=self._save_owner_profile).pack(side="left")
        ttk.Button(row, text="LOAD FROM JAR", command=lambda: self.link.send("GET PROFILE")).pack(side="left", padx=7)
        ttk.Button(row, text="CLEAR PROFILE", style="Danger.TButton",
                   command=self._clear_owner_profile).pack(side="left")
        ttk.Label(panel, textvariable=self.profile_state_var, style="PanelMuted.TLabel").pack(anchor="w", pady=(8, 0))

    @staticmethod
    def _clean_birthday(value):
        value = value.strip()
        if not value:
            return ""
        try:
            month, day_number = (int(part) for part in value.split("-", 1))
            date(2000, month, day_number)
        except (ValueError, TypeError):
            raise ValueError("Enter the birthday as MM-DD (for example, 06-11).")
        return f"{month:02d}-{day_number:02d}"

    @staticmethod
    def _next_birthday(value, today=None):
        today = today or date.today()
        month, day_number = (int(part) for part in value.split("-", 1))
        for year in range(today.year, today.year + 9):
            try:
                target = date(year, month, day_number)
            except ValueError:
                if month == 2 and day_number == 29:
                    target = date(year, 3, 1)
                else:
                    continue
            if target >= today:
                return target
        return None

    def _refresh_birthday_countdown(self):
        owner = self.profile_owner_var.get().strip()
        try:
            birthday = self._clean_birthday(self.profile_birthday_var.get())
        except ValueError:
            self.profile_countdown_var.set("Enter a valid birthday in MM-DD format.")
        else:
            if not birthday:
                self.profile_countdown_var.set("Add a birthday to start the countdown.")
            else:
                target = self._next_birthday(birthday)
                days = (target - date.today()).days
                if days == 0:
                    message = f"HAPPY BIRTHDAY, {owner}!" if owner else "HAPPY BIRTHDAY!"
                elif owner:
                    message = f"{days} day{'s' if days != 1 else ''} until {owner}'s birthday."
                else:
                    message = f"{days} day{'s' if days != 1 else ''} until the next birthday."
                self.profile_countdown_var.set(message)
        try:
            self.after(60_000, self._refresh_birthday_countdown)
        except tk.TclError:
            pass

    def _save_owner_profile(self):
        if not self._active_chip_id or not self.link.ser or not self.link.ser.is_open:
            self.profile_state_var.set("Connect the Jar before saving its profile.")
            return
        owner = self.profile_owner_var.get().strip().replace("|", "/").replace("\n", " ")[:32]
        if not owner:
            self.profile_state_var.set("Enter the owner's name.")
            return
        try:
            birthday = self._clean_birthday(self.profile_birthday_var.get())
        except ValueError as exc:
            self.profile_state_var.set(str(exc))
            return
        profile = {"owner": owner, "birthday": birthday}
        self._device_profiles[self._active_chip_id] = profile
        try:
            self._write_profile_store()
        except OSError as exc:
            self.profile_state_var.set(f"Could not save the Pi profile: {exc}")
            return
        if not self.link.send(f"SET PROFILE {birthday or '--'} {owner}"):
            self.profile_state_var.set("Saved on the Pi; Jar disconnected before the profile could be sent.")
            return
        self._profile_pending = "save"
        timezone = self.timezone_var.get().strip() or "America/Chicago"
        self.link.send(f"SET TIMEZONE {timezone}")
        self.link.send(f"SET CLOCK UNIX {int(time.time())}")
        self._refresh_birthday_countdown()
        self.profile_state_var.set("Profile sent; waiting for Jar confirmation.")

    def _clear_owner_profile(self):
        if not self._active_chip_id or not self.link.ser or not self.link.ser.is_open:
            self.profile_state_var.set("Connect the Jar before clearing its profile.")
            return
        if not self.link.send("CLEAR PROFILE"):
            self.profile_state_var.set("Jar disconnected; profile was not cleared.")
            return
        self._profile_pending = "clear"
        self._device_profiles.pop(self._active_chip_id, None)
        try:
            self._write_profile_store()
        except OSError as exc:
            self.profile_state_var.set(f"Could not update the Pi profile: {exc}")
            return
        self.profile_owner_var.set("")
        self.profile_birthday_var.set("")
        self._refresh_birthday_countdown()
        self.profile_state_var.set("Clear request sent; waiting for Jar confirmation.")

    def _handle_owner_profile(self, fields):
        chip = fields.get("chip", self._active_chip_id)
        if chip and self._active_chip_id and chip != self._active_chip_id:
            return
        chip = chip or self._active_chip_id
        local = self._device_profiles.get(chip, {}) if chip else {}
        owner = fields.get("owner", "")
        birthday = fields.get("birthday", "")
        if owner:
            profile = {"owner": owner, "birthday": birthday if birthday != "--" else ""}
            if chip:
                self._device_profiles[chip] = profile
                try:
                    self._write_profile_store()
                except OSError:
                    pass
        elif local.get("owner"):
            profile = local
            birthday_wire = profile.get("birthday") or "--"
            self.link.send(f"SET PROFILE {birthday_wire} {profile['owner']}")
        else:
            profile = {"owner": "", "birthday": ""}
        self.profile_owner_var.set(profile.get("owner", ""))
        self.profile_birthday_var.set(profile.get("birthday", ""))
        self.profile_chip_var.set(f"BOARD ID  {chip}" if chip else "Board ID unavailable")
        self.profile_state_var.set("Profile loaded from the Jar." if owner else "Local profile loaded." if local.get("owner") else "No owner profile saved on this Jar.")
        self._refresh_birthday_countdown()

    def _build_art_tab(self):
        art_tab = ttk.Frame(self.notebook, padding=8)
        self.notebook.insert(3, art_tab, text="ART")

        outer, panel = self._card(art_tab, 9)
        outer.pack(fill="both", expand=True)

        head = ttk.Frame(panel, style="Panel.TFrame")
        head.pack(fill="x")
        ttk.Label(head, text="ART FRAME MAKER", style="Section.TLabel").pack(side="left")
        ttk.Label(head, text="128 × 64 • up to 2 frames • 50 pieces", style="PanelMuted.TLabel").pack(side="right")
        ttk.Label(
            panel,
            text="Draw and preview OLED art, save it to the Pi library, then send the gallery to your Jar.",
            style="PanelMuted.TLabel",
            wraplength=820,
        ).pack(anchor="w", pady=(5, 8))

        self.art_studio = HappyJarzArtStudio(panel, self.link)
        self.art_studio.pack(fill="both", expand=True)

    def _build_service_tab(self, root):
        super()._build_service_tab(root)

        if self.power_voltage is None:
            self.power_voltage = tk.StringVar(master=self, value="—")
        if self.power_percent is None:
            self.power_percent = tk.StringVar(master=self, value="—")
        if self.power_usb is None:
            self.power_usb = tk.StringVar(master=self, value="—")
        if self.power_sensor is None:
            self.power_sensor = tk.StringVar(master=self, value="WAITING")
        if self.power_charge is None:
            self.power_charge = tk.StringVar(master=self, value="HW LED")
        if self.power_adc is None:
            self.power_adc = tk.StringVar(master=self, value="—")

        outer, power = self._card(root, 9)
        outer.pack(fill="x", pady=(8, 0))

        head = ttk.Frame(power, style="Panel.TFrame")
        head.pack(fill="x")
        ttk.Label(head, text="FUEL GAUGE", style="Section.TLabel").pack(side="left")
        ttk.Label(head, textvariable=self.power_sensor, style="PanelMuted.TLabel").pack(side="right")

        row = ttk.Frame(power, style="Panel.TFrame")
        row.pack(fill="x", pady=(7, 0))

        items = (
            ("BATTERY", self.power_percent),
            ("VOLTAGE", self.power_voltage),
            ("USB DATA", self.power_usb),
            ("CHARGE", self.power_charge),
            ("ADC", self.power_adc),
        )
        for col, (label, var) in enumerate(items):
            box = ttk.Frame(row, style="Panel2.TFrame", padding=7)
            box.grid(row=0, column=col, sticky="nsew", padx=(0, 5) if col < len(items) - 1 else 0)
            ttk.Label(box, text=label, background=previous.previous.base.PANEL_2,
                      foreground=previous.previous.base.MUTED,
                      font=("TkDefaultFont", 8, "bold")).pack()
            ttk.Label(box, textvariable=var, style="TouchValue.TLabel").pack()
            row.columnconfigure(col, weight=1)

        footer = ttk.Frame(power, style="Panel.TFrame")
        footer.pack(fill="x", pady=(7, 0))
        ttk.Label(
            footer,
            text="Battery % is voltage-estimated. CHARGING/FULL remains hardware-only until charger status is wired to a GPIO.",
            style="PanelMuted.TLabel",
        ).pack(side="left")
        ttk.Button(footer, text="Refresh Power", style="Accent.TButton",
                   command=lambda: self.link.send("GET POWER")).pack(side="right")

    def _poll_power(self):
        try:
            if self.link.ser is not None and self.link.ser.is_open:
                self.link.send("GET POWER")
        finally:
            self.after(5000, self._poll_power)

    def _handle_line(self, line: str):
        if line.startswith("HJ|POWER|"):
            fields = self._parse_fields(line)
            sensor = fields.get("sensor", "UNKNOWN")
            if self.power_sensor is not None:
                self.power_sensor.set(sensor)

            voltage = fields.get("voltage", "")
            if self.power_voltage is not None:
                self.power_voltage.set(f"{voltage} V" if voltage else "—")

            percent = fields.get("percent", "")
            if self.power_percent is not None:
                if percent and percent != "-1":
                    self.power_percent.set(f"{percent}%")
                else:
                    self.power_percent.set("CHECK")

            if self.power_usb is not None:
                self.power_usb.set("IN" if fields.get("usb_data", "0") == "1" else "OUT")

            if self.power_charge is not None:
                charge = fields.get("charge", "HW_ONLY")
                self.power_charge.set("HW LED" if charge == "HW_ONLY" else charge)

            if self.power_adc is not None:
                adc = fields.get("adc_mv", "")
                self.power_adc.set(f"{adc} mV" if adc else "—")

            self._log(f"RX  {line}")
            return

        if line.startswith("HJ|PROFILE|"):
            self._handle_owner_profile(self._parse_fields(line))
            self._log(f"RX  {line}")
            return

        if line == "HJ|ACK|command=SET PROFILE":
            if self._profile_pending == "save":
                self.profile_state_var.set("Profile saved on the Pi and Jar; clock sync sent.")
                self._profile_pending = None
            self._log(f"RX  {line}")
            return

        if line == "HJ|ACK|command=CLEAR PROFILE":
            if self._profile_pending == "clear":
                self.profile_state_var.set("Profile cleared from the Pi and Jar.")
                self._profile_pending = None
            self._log(f"RX  {line}")
            return

        if line.startswith("HJ|ERR|") and self._profile_pending:
            self.profile_state_var.set(f"Jar rejected profile command: {self._parse_fields(line).get('message', 'unknown error')}")
            self._profile_pending = None
            self._log(f"RX  {line}")
            return

        super()._handle_line(line)
        if line.startswith("HJ|IDENTITY|"):
            fields = self._parse_fields(line)
            self._active_chip_id = fields.get("chip", "")
            self.profile_chip_var.set(f"BOARD ID  {self._active_chip_id}" if self._active_chip_id else "Board ID unavailable")
            self.profile_owner_var.set("")
            self.profile_birthday_var.set("")
            self.profile_state_var.set("Reading profile from Jar…")
            self.link.send("GET PROFILE")
            self.link.send("GET TIME STATUS")
            self.after(600, self._sync_host_time)
            if "power" not in self._identity_refreshes:
                self._identity_refreshes.add("power")
                self.after(250, lambda: self.link.send("GET POWER"))
            if not self._power_poll_started:
                self._power_poll_started = True
                self.after(5000, self._poll_power)


if __name__ == "__main__":
    HappyJarzApp().mainloop()


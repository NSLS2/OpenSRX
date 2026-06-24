#!/usr/bin/env python3
"""
SR-X 300 Reader Socket Interface Simulator

Simulates a KEYENCE SR-X 300 barcode reader's TCP socket command interface
as described in Chapter 14 (Command Communication) of the SRX300 User Manual.

Supports:
  - All three communication formats (CR, CR+LF, STX/ETX)
  - Operation commands (LON, LOFF, RESET, KEYENCE, NUM, EMAC, etc.)
    - Configuration commands (WB/RB, WC/RC, WP/RP, WD/RD, WN/RN)
  - Tuning commands (TUNE/FTUNE/TQUIT)
  - Save/Load/Initialize (SAVE, LOAD, DFLT)
  - Status queries (CMDSTAT, ERRSTAT, BUSYSTAT)
  - Error code responses for invalid commands

Usage:
    python simulator.py [--host HOST] [--port PORT] [--comm-format {0,1,2}]
"""

import argparse
import io
import logging
import math
import os
import random
import socket
import string
import struct
import threading
import time
from datetime import datetime
from pathlib import PurePosixPath

try:
    from PIL import Image, ImageDraw, ImageFont

    _HAS_PIL = True
except ImportError:
    _HAS_PIL = False

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s",
)
logger = logging.getLogger("SRX300-Sim")

# Default device identity
DEFAULT_MODEL = "SR-X300"
DEFAULT_FIRMWARE = "R2.04.00"
DEFAULT_MAC = "001122334455"
DEFAULT_IP = "192.168.100.100"
DEFAULT_PORT = 9004


class SRX300Simulator:
    """Simulates the state and command processing of an SR-X 300 reader."""

    def __init__(self, comm_format: int = 0):
        self.model = DEFAULT_MODEL
        self.firmware = DEFAULT_FIRMWARE
        self.mac = DEFAULT_MAC

        # Communication format: 0=CR/CR, 1=CR+LF/CR, 2=STX-ETX/STX-ETX
        self.comm_format = comm_format

        # Reading state
        self.reading = False
        self.reading_bank: int | None = None
        self.tuning = False

        # Counters for NUM command
        self.ok_count = 0
        self.ng_count = 0
        self.error_count = 0
        self.stable_count = 0
        self.trigger_count = 0
        self.bank_counts = [0] * 16  # 16 banks

        # Parameter bank configuration storage: key = "BB_MMM" (bank + cmd number)
        self.bank_config: dict[str, str] = {}

        # Tuning configuration storage: key = cmd_number string
        self.tuning_config: dict[str, str] = {
            # Pharmacode
            "1803": "0",  # PHARMACODE_READ_DIRECTION
            # Aztec
            "1903": "0",  # AZTEC_MAX_READ_LENGTH
            "1904": "0",  # AZTEC_MIN_READ_LENGTH
            # Postal
            "1905": "0",  # POSTAL_INTELLIGENT_MAIL
            "1908": "0",  # POSTAL_JAPAN_READING
            "1909": "0",  # POSTAL_MAX_READ_LENGTH
            "1910": "0",  # POSTAL_MIN_READ_LENGTH
            # DotCode
            "1920": "0",  # DOTCODE_MAX_READ_LENGTH
            "1921": "0",  # DOTCODE_MIN_READ_LENGTH
        }

        # Operation configuration storage: key = cmd_number string
        self.op_config: dict[str, str] = {
            "101": "0",   # Timing mode: 0=Level trigger
            "102": "100", # One-shot trigger duration (x10 ms)
            "200": "0",   # Reading mode: 0=Standard
            "201": "0",   # Data transmission: 0=Send after read
            "205": "FF",  # Read error character string
            # Image save destinations (0=Disable, 3=Send by FTP, 5=ROM+FTP)
            "500": "0",   # Read OK images
            "501": "0",   # Verification NG images
            "502": "0",   # Read error images
            "503": "0",   # Unstable images
            "504": "0",   # Capture images
            "505": "0",   # Image saving mode
            # Data appending (0=Disable, 1=Enable)
            "301": "0",   # Code type appending
            "303": "0",   # Bank number appending
            "308": "0",   # Code vertex appending
            "309": "0",   # Code center appending
            "371": "0",   # Angle appending
            # Delimiter settings
            "601": "2C",  # Delimiter character (comma)
            "602": "2C",  # Inter-delimiter (comma)
        }

        # Region configuration storage
        self.region_config: dict[str, str] = {}

        # Communication configuration storage
        self.comm_config: dict[str, str] = {
            "200": DEFAULT_IP,       # IP address
            "201": "255.255.255.0",  # Subnet mask
            "202": "192.168.100.1",  # Default gateway
            "203": str(DEFAULT_PORT),  # Ethernet (server) port
            "003": "0",             # Append checksum
            "006": "0D",            # Terminator setting (0D = CR)
            # FTP image server settings
            "400": "0.0.0.0",       # Remote FTP server IP (0.0.0.0 = not set)
            "401": "admin",          # FTP username
            "402": "admin",          # FTP password
            "403": "0",             # Subfolder: 0=Disable, 1=Enable
            "404": "image",          # Subfolder name
            "405": "0",             # FTP connection timing
            "408": "0",             # Passive mode: 0=Disable, 1=Enable
            "442": "21",            # FTP remote port
        }

        # Image capture counter (for file naming)
        self._image_counter = 0

        # Image store: maps filename -> bytes (BMP data) for FTP serving
        self._image_store: dict[str, bytes] = {}
        # Ordered list of image filenames for "latest" queries
        self._image_filenames: list[str] = []

        # Status
        self.cmd_status = "none"
        self.error_status = "none"
        self.busy_status = "none"

        # I/O terminal state (OUT1, OUT2, OUT3)
        self.outputs = [False, False, False]

        self._lock = threading.Lock()

    # ---- Framing helpers ----

    def _get_in_terminator(self) -> bytes:
        """Terminator the reader expects on incoming commands."""
        if self.comm_format == 2:
            return b"\x03"  # ETX
        return b"\r"

    def _wrap_response(self, payload: str) -> bytes:
        """Wrap a response payload with the correct header/terminator."""
        if self.comm_format == 0:
            return (payload + "\r").encode("ascii")
        elif self.comm_format == 1:
            return (payload + "\r\n").encode("ascii")
        else:
            return (b"\x02" + payload.encode("ascii") + b"\x03")

    def _strip_framing(self, raw: bytes) -> str:
        """Strip header/terminator from a raw incoming message."""
        text = raw.decode("ascii", errors="replace")
        # Strip ESC prefix if present (buffer clear)
        if text.startswith("\x1b"):
            text = text[1:]
        # Strip STX/ETX
        if text.startswith("\x02"):
            text = text[1:]
        if text.endswith("\x03"):
            text = text[:-1]
        # Strip CR / LF terminators
        text = text.rstrip("\r\n")
        return text

    # ---- Command processing ----

    def process_command(self, raw_cmd: str) -> list[str]:
        """
        Process a single command string and return a list of response payloads.
        Some commands (LON, LOFF, PRON, PROFF) have no immediate response
        but may produce read-data results.
        """
        parts = raw_cmd.split(",")
        cmd_name = parts[0].strip().upper()
        params = parts[1:] if len(parts) > 1 else []

        with self._lock:
            return self._dispatch(cmd_name, params, raw_cmd)

    def _dispatch(self, cmd: str, params: list[str], raw: str) -> list[str]:
        handler = {
            # Operation commands - reading
            "LON": self._cmd_lon,
            "LOFF": self._cmd_loff,
            "PRON": self._cmd_pron,
            "PROFF": self._cmd_proff,

            # Tuning
            "TUNE": self._cmd_tune,
            "FTUNE": self._cmd_ftune,
            "TQUIT": self._cmd_tquit,

            # Reset / buffer
            "RESET": self._cmd_reset,
            "BCLR": self._cmd_bclr,

            # I/O terminal control
            "OUTON": self._cmd_outon,
            "OUTOFF": self._cmd_outoff,
            "ALLON": self._cmd_allon,
            "ALLOFF": self._cmd_alloff,

            # Reading history
            "NUM": self._cmd_num,
            "NUMB": self._cmd_numb,

            # Status
            "KEYENCE": self._cmd_keyence,
            "CMDSTAT": self._cmd_cmdstat,
            "ERRSTAT": self._cmd_errstat,
            "BUSYSTAT": self._cmd_busystat,
            "EMAC": self._cmd_emac,

            # Save / Load / Initialize
            "SAVE": self._cmd_save,
            "LOAD": self._cmd_load,
            "DFLT": self._cmd_dflt,

            # Configuration commands
            "WB": self._cmd_wb,
            "RB": self._cmd_rb,
            "WC": self._cmd_wc,
            "RC": self._cmd_rc,
            "WP": self._cmd_wp,
            "RP": self._cmd_rp,
            "WD": self._cmd_wd,
            "RD": self._cmd_rd,
            "WN": self._cmd_wn,
            "RN": self._cmd_rn,

            # Test mode
            "TMON": self._cmd_tmon,
            "TMOFF": self._cmd_tmoff,

            # Quick setup
            "RCON": self._cmd_rcon,
            "RCOFF": self._cmd_rcoff,

            # Capture
            "SHOT": self._cmd_shot,
        }.get(cmd)

        if handler is None:
            # Also check LON with bank suffix: "LON01" etc.
            if cmd.startswith("LON") and len(cmd) > 3:
                return self._cmd_lon_bank(cmd[3:])
            # SHOT with bank suffix: "SHOT01" etc.
            if cmd.startswith("SHOT") and len(cmd) > 4:
                return self._cmd_shot([cmd[4:]])
            logger.warning("Undefined command: %s", raw)
            return [f"ER,{cmd},{0:02d}"]

        return handler(params)

    # ---- Operation commands ----

    def _cmd_lon(self, params: list[str]) -> list[str]:
        if params:
            # LON,b format
            return self._cmd_lon_bank(params[0])
        self.reading = True
        self.reading_bank = None
        logger.info("Reading started (all banks)")
        # LON has no response; simulate a read result after a short delay
        return [self._simulate_read_result()]

    def _cmd_lon_bank(self, bank_str: str) -> list[str]:
        try:
            bank = int(bank_str)
            if bank < 1 or bank > 16:
                return [f"ER,LON,{2:02d}"]
        except ValueError:
            return [f"ER,LON,{1:02d}"]
        self.reading = True
        self.reading_bank = bank
        logger.info("Reading started (bank %d)", bank)
        return [self._simulate_read_result(bank)]

    def _cmd_loff(self, params: list[str]) -> list[str]:
        self.reading = False
        self.reading_bank = None
        logger.info("Reading stopped")
        # LOFF has no response
        return []

    def _cmd_pron(self, params: list[str]) -> list[str]:
        logger.info("Preset registration reading started")
        return []

    def _cmd_proff(self, params: list[str]) -> list[str]:
        logger.info("Preset registration reading stopped")
        return []

    def _cmd_tune(self, params: list[str]) -> list[str]:
        self.tuning = True
        logger.info("Tuning started")
        return ["OK"]

    def _cmd_ftune(self, params: list[str]) -> list[str]:
        self.tuning = True
        logger.info("Fine tuning started")
        return ["OK,FTUNE"]

    def _cmd_tquit(self, params: list[str]) -> list[str]:
        self.tuning = False
        logger.info("Tuning finished")
        return ["OK"]

    def _cmd_reset(self, params: list[str]) -> list[str]:
        self.reading = False
        self.reading_bank = None
        self.tuning = False
        logger.info("Reset executed")
        return ["OK"]

    def _cmd_bclr(self, params: list[str]) -> list[str]:
        logger.info("Send buffer cleared")
        return ["OK"]

    # ---- I/O terminal control ----

    def _cmd_outon(self, params: list[str]) -> list[str]:
        if not params:
            return ["ER,OUTON,01"]
        try:
            n = int(params[0])
            if n < 1 or n > 3:
                return ["ER,OUTON,02"]
        except ValueError:
            return ["ER,OUTON,01"]
        self.outputs[n - 1] = True
        logger.info("OUT%d turned ON", n)
        return ["OK,OUTON"]

    def _cmd_outoff(self, params: list[str]) -> list[str]:
        if not params:
            return ["ER,OUTOFF,01"]
        try:
            n = int(params[0])
            if n < 1 or n > 3:
                return ["ER,OUTOFF,02"]
        except ValueError:
            return ["ER,OUTOFF,01"]
        self.outputs[n - 1] = False
        logger.info("OUT%d turned OFF", n)
        return ["OK,OUTOFF"]

    def _cmd_allon(self, params: list[str]) -> list[str]:
        self.outputs = [True, True, True]
        logger.info("All outputs turned ON")
        return ["OK,ALLON"]

    def _cmd_alloff(self, params: list[str]) -> list[str]:
        self.outputs = [False, False, False]
        logger.info("All outputs turned OFF")
        return ["OK,ALLOFF"]

    # ---- Reading history ----

    def _cmd_num(self, params: list[str]) -> list[str]:
        return [
            f"OK,NUM,{self.ok_count},{self.ng_count},{self.error_count},"
            f"{self.stable_count},{self.trigger_count}"
        ]

    def _cmd_numb(self, params: list[str]) -> list[str]:
        counts = ",".join(str(c) for c in self.bank_counts)
        return [f"OK,NUMB,{counts},{self.trigger_count}"]

    # ---- Status / identification commands ----

    def _cmd_keyence(self, params: list[str]) -> list[str]:
        return [f"OK,KEYENCE,{self.model},{self.firmware}"]

    def _cmd_cmdstat(self, params: list[str]) -> list[str]:
        return [f"OK,CMDSTAT,{self.cmd_status}"]

    def _cmd_errstat(self, params: list[str]) -> list[str]:
        return [f"OK,ERRSTAT,{self.error_status}"]

    def _cmd_busystat(self, params: list[str]) -> list[str]:
        return [f"OK,BUSYSTAT,{self.busy_status}"]

    def _cmd_emac(self, params: list[str]) -> list[str]:
        return [f"OK,EMAC,{self.mac}"]

    # ---- Save / Load / Initialize ----

    def _cmd_save(self, params: list[str]) -> list[str]:
        logger.info("Settings saved")
        return ["OK,SAVE"]

    def _cmd_load(self, params: list[str]) -> list[str]:
        logger.info("Settings loaded")
        return ["OK,LOAD"]

    def _cmd_dflt(self, params: list[str]) -> list[str]:
        logger.info("Settings initialized to defaults")
        self.op_config = {
            "101": "0", "102": "100", "200": "0", "201": "0", "205": "FF",
            "500": "0", "501": "0", "502": "0", "503": "0", "504": "0", "505": "0",
        }
        self.tuning_config = {
            "1803": "0",
            "1903": "0",
            "1904": "0",
            "1905": "0",
            "1908": "0",
            "1909": "0",
            "1910": "0",
            "1920": "0",
            "1921": "0",
        }
        self.bank_config.clear()
        self.region_config.clear()
        return ["OK,DFLT"]

    # ---- Configuration commands: WB/RB (bank parameters) ----

    def _cmd_wb(self, params: list[str]) -> list[str]:
        # Format: WB,bm,n where b=bank(01-16), m=cmd number, n=value
        if len(params) < 2:
            return ["ER,WB,01"]
        key = params[0]  # e.g. "01100"
        value = params[1]
        if len(key) < 3:
            return ["ER,WB,01"]
        self.bank_config[key] = value
        logger.info("WB set %s = %s", key, value)
        return ["OK,WB"]

    def _cmd_rb(self, params: list[str]) -> list[str]:
        if not params:
            return ["ER,RB,01"]
        key = params[0]
        value = self.bank_config.get(key)
        if value is None:
            # Return a default value of 0
            return [f"OK,RB,0"]
        return [f"OK,RB,{value}"]

    # ---- Configuration commands: WC/RC (tuning parameters) ----

    def _cmd_wc(self, params: list[str]) -> list[str]:
        if len(params) < 2:
            return ["ER,WC,01"]
        cmd_num = params[0]
        value = params[1]
        self.tuning_config[cmd_num] = value
        logger.info("WC set %s = %s", cmd_num, value)
        return ["OK,WC"]

    def _cmd_rc(self, params: list[str]) -> list[str]:
        if not params:
            return ["ER,RC,01"]
        cmd_num = params[0]
        value = self.tuning_config.get(cmd_num, "0")
        return [f"OK,RC,{value}"]

    # ---- Configuration commands: WP/RP (operation parameters) ----

    def _cmd_wp(self, params: list[str]) -> list[str]:
        if len(params) < 2:
            return ["ER,WP,01"]
        cmd_num = params[0]
        value = params[1]
        self.op_config[cmd_num] = value
        logger.info("WP set %s = %s", cmd_num, value)
        return ["OK,WP"]

    def _cmd_rp(self, params: list[str]) -> list[str]:
        if not params:
            return ["ER,RP,01"]
        cmd_num = params[0]
        value = self.op_config.get(cmd_num, "0")
        return [f"OK,RP,{value}"]

    # ---- Configuration commands: WD/RD (region) ----

    def _cmd_wd(self, params: list[str]) -> list[str]:
        if len(params) < 2:
            return ["ER,WD,01"]
        cmd_num = params[0]
        value = params[1]
        self.region_config[cmd_num] = value
        logger.info("WD set %s = %s", cmd_num, value)
        return ["OK,WD"]

    def _cmd_rd(self, params: list[str]) -> list[str]:
        if not params:
            return ["ER,RD,01"]
        cmd_num = params[0]
        value = self.region_config.get(cmd_num, "0")
        return [f"OK,RD,{value}"]

    # ---- Configuration commands: WN/RN (communication) ----

    def _cmd_wn(self, params: list[str]) -> list[str]:
        if len(params) < 2:
            return ["ER,WN,01"]
        cmd_num = params[0]
        value = ",".join(params[1:])  # IP addresses contain dots, not commas in value
        self.comm_config[cmd_num] = value
        logger.info("WN set %s = %s", cmd_num, value)
        return ["OK,WN"]

    def _cmd_rn(self, params: list[str]) -> list[str]:
        if not params:
            return ["ER,RN,01"]
        cmd_num = params[0]
        value = self.comm_config.get(cmd_num, "0")
        return [f"OK,RN,{value}"]

    # ---- Test mode ----

    def _cmd_tmon(self, params: list[str]) -> list[str]:
        if not params:
            return ["ER,TMON,01"]
        mode = params[0]
        logger.info("Test mode ON: mode=%s", mode)
        return ["OK,TMON"]

    def _cmd_tmoff(self, params: list[str]) -> list[str]:
        logger.info("Test mode OFF")
        return ["OK,TMOFF"]

    # ---- Quick setup ----

    def _cmd_rcon(self, params: list[str]) -> list[str]:
        logger.info("Quick setup code reading started")
        return ["OK,RCON"]

    def _cmd_rcoff(self, params: list[str]) -> list[str]:
        logger.info("Quick setup code reading stopped")
        return ["OK,RCOFF"]

    # ---- SHOT command ----

    def _cmd_shot(self, params: list[str]) -> list[str]:
        if not params:
            return ["ER,SHOT,01"]
        try:
            bank = int(params[0])
            if bank < 1 or bank > 16:
                return ["ER,SHOT,02"]
        except ValueError:
            return ["ER,SHOT,01"]

        # Always generate and store the image for FTP pull
        code_data = ""  # no barcode for snapshot
        img_name, _ = self._generate_and_store_image(code_data, bank, draw_overlay=False)
        return [f"OK,SHOT,A:\\IMAGE\\{img_name}"]

    # ---- Simulated read result ----

    def _simulate_read_result(self, bank: int | None = None) -> str:
        """Generate a simulated barcode read result."""
        code_length = random.randint(8, 20)
        code_data = "".join(random.choices(string.ascii_uppercase + string.digits, k=code_length))
        self.ok_count += 1
        self.trigger_count += 1
        if bank is not None and 1 <= bank <= 16:
            self.bank_counts[bank - 1] += 1
        else:
            self.bank_counts[0] += 1

        corners: list[tuple[int, int]] = []
        center: tuple[int, int] = (0, 0)

        # Always generate and store an image during reads for FTP pull
        _, corners = self._generate_and_store_image(code_data, bank or 1, draw_overlay=True)

        # If corners were not generated (no PIL), synthesize some
        if not corners:
            rng = random.Random(hash(code_data))
            cx, cy = 320 + rng.randint(-60, 60), 240 + rng.randint(-40, 40)
            hw, hh = 80, 30
            corners = [(cx - hw, cy - hh), (cx + hw, cy - hh),
                       (cx + hw, cy + hh), (cx - hw, cy + hh)]
        center = (
            sum(c[0] for c in corners) // 4,
            sum(c[1] for c in corners) // 4,
        )

        # Build the result string with optional appended data
        result = code_data
        delim = self._get_delimiter()

        # CODE_VERTEX_APPENDING (308): append TL.x,TL.y,TR.x,TR.y,BR.x,BR.y,BL.x,BL.y
        if self.op_config.get("308", "0") == "1":
            vertex_str = ",".join(f"{c[0]},{c[1]}" for c in corners)
            result += delim + vertex_str

        # CODE_CENTER_APPENDING (309): append cx,cy
        if self.op_config.get("309", "0") == "1":
            result += delim + f"{center[0]},{center[1]}"

        # CODE_TYPE_APPENDING (301): append the code type name
        if self.op_config.get("301", "0") == "1":
            result += delim + "CODE128"

        # BANK_NUMBER_APPENDING (303): append bank number
        if self.op_config.get("303", "0") == "1":
            result += delim + f"{(bank or 1):02d}"

        # ANGLE_APPENDING (371): append the angle
        if self.op_config.get("371", "0") == "1":
            angle = random.uniform(-5, 5)
            result += delim + f"{angle:.1f}"

        return result

    def _get_delimiter(self) -> str:
        """Return the inter-field delimiter character from config."""
        # INTER_DELIMITER (602): hex byte value, default 0x2C = comma
        hex_val = self.op_config.get("602", "2C")
        try:
            return chr(int(hex_val, 16))
        except (ValueError, OverflowError):
            return ","

    # ---- Image generation and FTP push ----

    def _generate_and_store_image(
        self, code_data: str, bank: int, img_width: int = 640, img_height: int = 480,
        draw_overlay: bool = True,
    ) -> tuple[str, list[tuple[int, int]]]:
        """Generate a simulated camera image and store it for FTP retrieval.
        When draw_overlay is True (code read), a bounding box and text
        are drawn. When False (snapshot/capture), only the raw scene is shown.
        Returns (filename, corner_coords)."""
        self._image_counter += 1
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"B{bank:02d}_{timestamp}_{self._image_counter:06d}.bmp"

        if not _HAS_PIL:
            logger.warning("Pillow not installed; generating minimal BMP")
            # Generate a minimal valid 8-bit grayscale BMP even without PIL
            bmp_data = self._generate_minimal_bmp(img_width, img_height)
            self._image_store[filename] = bmp_data
            self._image_filenames.append(filename)
            return filename, []

        img, corners = self._render_barcode_image(
            code_data, bank, img_width, img_height, draw_overlay=draw_overlay,
        )
        if draw_overlay:
            logger.info(
                "Generated image %s  barcode corners: TL=(%d,%d) TR=(%d,%d) BR=(%d,%d) BL=(%d,%d)",
                filename, *corners[0], *corners[1], *corners[2], *corners[3],
            )
        else:
            logger.info("Generated snapshot image %s", filename)

        # Store image as BMP bytes for FTP serving
        buf = io.BytesIO()
        img.save(buf, format="BMP")
        self._image_store[filename] = buf.getvalue()
        self._image_filenames.append(filename)
        logger.debug("Stored image %s (%d bytes) for FTP", filename, len(self._image_store[filename]))
        return filename, corners

    @staticmethod
    def _generate_minimal_bmp(width: int = 640, height: int = 480) -> bytes:
        """Generate a minimal valid 8-bit grayscale BMP without PIL."""
        row_bytes = width
        row_padding = (4 - (row_bytes % 4)) % 4
        image_size = (row_bytes + row_padding) * height
        palette_size = 256 * 4
        pixel_offset = 54 + palette_size
        file_size = pixel_offset + image_size

        data = bytearray()
        # File header
        data += b"BM"
        data += struct.pack("<I", file_size)
        data += struct.pack("<HH", 0, 0)
        data += struct.pack("<I", pixel_offset)
        # DIB header
        data += struct.pack("<I", 40)
        data += struct.pack("<i", width)
        data += struct.pack("<i", height)
        data += struct.pack("<HH", 1, 8)
        data += struct.pack("<I", 0)
        data += struct.pack("<I", image_size)
        data += struct.pack("<i", 2835)
        data += struct.pack("<i", 2835)
        data += struct.pack("<I", 256)
        data += struct.pack("<I", 0)
        # Palette
        for i in range(256):
            data += struct.pack("BBBB", i, i, i, 0)
        # Pixel data (gradient)
        for y in range(height):
            row = bytes([((x + y) * 37) % 256 for x in range(width)])
            data += row + b"\x00" * row_padding
        return bytes(data)

    @staticmethod
    def _render_barcode_image(
        code_data: str,
        bank: int,
        img_width: int = 640,
        img_height: int = 480,
        draw_overlay: bool = True,
    ) -> tuple["Image.Image", list[tuple[int, int]]]:
        """Render a grayscale camera image with a barcode pattern.
        When draw_overlay is True, a blue bounding box and text labels are
        drawn (simulating a successful code read). When False (snapshot),
        only the raw scene with the barcode pattern is shown.
        Returns (PIL Image, list of 4 corner coordinates [TL, TR, BR, BL])."""

        # --- background: noisy gray ---
        img = Image.new("RGB", (img_width, img_height), (200, 200, 200))
        draw = ImageDraw.Draw(img)

        # Add some noise texture
        rng = random.Random(hash(code_data))
        for y in range(0, img_height, 4):
            for x in range(0, img_width, 4):
                g = rng.randint(180, 220)
                draw.rectangle([x, y, x + 3, y + 3], fill=(g, g, g))

        # --- Compute barcode region with slight rotation ---
        rng = random.Random(hash(code_data) if code_data else random.randint(0, 2**32))
        angle_deg = rng.uniform(-5, 5)
        angle_rad = math.radians(angle_deg)

        # Barcode dimensions in local space
        bar_count = max(len(code_data) * 6 + 11, 20)  # rough Code‑128-like bar count
        bar_w = max(1, min(3, img_width // (bar_count + 20)))
        barcode_w = bar_count * bar_w
        barcode_h = max(40, img_height // 6)

        # Center of barcode in image space
        cx = img_width // 2 + rng.randint(-60, 60)
        cy = img_height // 2 + rng.randint(-40, 40)

        # Calculate four corner positions (TL, TR, BR, BL) of the barcode rectangle
        hw, hh = barcode_w / 2, barcode_h / 2
        cos_a, sin_a = math.cos(angle_rad), math.sin(angle_rad)

        def rotate_point(lx: float, ly: float) -> tuple[int, int]:
            rx = cx + lx * cos_a - ly * sin_a
            ry = cy + lx * sin_a + ly * cos_a
            return (int(round(rx)), int(round(ry)))

        corners = [
            rotate_point(-hw, -hh),  # top-left
            rotate_point(+hw, -hh),  # top-right
            rotate_point(+hw, +hh),  # bottom-right
            rotate_point(-hw, +hh),  # bottom-left
        ]

        # --- Draw barcode bars (only if we have code data) ---
        if code_data:
            # Create the barcode on a small image first, then paste rotated
            bc_img = Image.new("RGB", (barcode_w, barcode_h), (255, 255, 255))
            bc_draw = ImageDraw.Draw(bc_img)

            # Quiet zone
            x_pos = bar_w * 5
            for ch in code_data:
                bits = format(ord(ch), "08b")
                for bit in bits:
                    if bit == "1":
                        bc_draw.rectangle(
                            [x_pos, 2, x_pos + bar_w - 1, barcode_h - 3],
                            fill=(0, 0, 0),
                        )
                    x_pos += bar_w

            # Rotate barcode image
            bc_rotated = bc_img.rotate(
                -angle_deg, resample=Image.BICUBIC, expand=True, fillcolor=(200, 200, 200)
            )
            paste_x = cx - bc_rotated.width // 2
            paste_y = cy - bc_rotated.height // 2
            img.paste(bc_rotated, (paste_x, paste_y))

        if draw_overlay and code_data:
            # --- Draw blue bounding box around the barcode corners ---
            box_color = (0, 80, 255)
            for i in range(4):
                draw.line([corners[i], corners[(i + 1) % 4]], fill=box_color, width=2)

            # --- Draw code text below barcode ---
            try:
                font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 14)
            except (OSError, IOError):
                font = ImageFont.load_default()

            text_y = max(c[1] for c in corners) + 6
            draw.text((cx - len(code_data) * 4, text_y), code_data, fill=(0, 0, 0), font=font)

            # --- Draw bank label ---
            draw.text((8, 8), f"Bank {bank:02d}", fill=(0, 0, 0), font=font)

        return img, corners


class FtpClientHandler(threading.Thread):
    """Handles a single FTP client session with minimal FTP protocol support."""

    def __init__(self, conn: socket.socket, addr, simulator: SRX300Simulator, host: str):
        super().__init__(daemon=True)
        self.conn = conn
        self.addr = addr
        self.sim = simulator
        self.host = host

    def _send(self, msg: str) -> None:
        self.conn.sendall((msg + "\r\n").encode("ascii"))

    def _recv_line(self) -> str:
        buf = b""
        while b"\r\n" not in buf and b"\n" not in buf:
            data = self.conn.recv(1024)
            if not data:
                return ""
            buf += data
        line = buf.decode("ascii", errors="replace").strip()
        return line

    def run(self) -> None:
        logger.debug("FTP client connected: %s:%d", *self.addr)
        try:
            self._send("220 SR-X300 FTP Server Ready")
            while True:
                line = self._recv_line()
                if not line:
                    break
                parts = line.split(None, 1)
                cmd = parts[0].upper()
                arg = parts[1] if len(parts) > 1 else ""

                if cmd == "USER":
                    self._send("230 Login successful")
                elif cmd == "PASS":
                    self._send("230 Login successful")
                elif cmd == "SYST":
                    self._send("215 UNIX Type: L8")
                elif cmd == "FEAT":
                    self._send("211 End")
                elif cmd == "PWD":
                    self._send('257 "/" is current directory')
                elif cmd == "CWD":
                    self._send("250 Directory changed")
                elif cmd == "TYPE":
                    self._send("200 Type set")
                elif cmd == "PASV":
                    self._handle_pasv()
                elif cmd == "NLST":
                    self._handle_nlst(arg)
                elif cmd == "LIST":
                    self._handle_list(arg)
                elif cmd == "RETR":
                    self._handle_retr(arg)
                elif cmd == "QUIT":
                    self._send("221 Goodbye")
                    break
                else:
                    self._send(f"502 Command not implemented: {cmd}")
        except (ConnectionResetError, BrokenPipeError, OSError):
            pass
        finally:
            self.conn.close()

    def _handle_pasv(self) -> None:
        """Open a data port and report it to the client."""
        # Bind an ephemeral port for the data connection
        self._data_srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._data_srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self._data_srv.bind((self.host, 0))
        self._data_srv.listen(1)
        _, data_port = self._data_srv.getsockname()

        # Format: (h1,h2,h3,h4,p1,p2)
        ip_parts = self.host.replace(".", ",")
        p1 = data_port >> 8
        p2 = data_port & 0xFF
        self._send(f"227 Entering Passive Mode ({ip_parts},{p1},{p2})")

    def _get_data_conn(self) -> socket.socket:
        """Accept the data connection from the client."""
        self._data_srv.settimeout(10)
        conn, _ = self._data_srv.accept()
        self._data_srv.close()
        return conn

    def _handle_nlst(self, path: str) -> None:
        """Return filenames in the /IMAGE directory."""
        self._send("150 Opening data connection")
        data_conn = self._get_data_conn()
        try:
            with self.sim._lock:
                filenames = list(self.sim._image_filenames)
            listing = "\r\n".join(filenames)
            if listing:
                listing += "\r\n"
            data_conn.sendall(listing.encode("ascii"))
        finally:
            data_conn.close()
        self._send("226 Transfer complete")

    def _handle_list(self, path: str) -> None:
        """Return a detailed listing (Unix ls -l style) for /IMAGE."""
        self._send("150 Opening data connection")
        data_conn = self._get_data_conn()
        try:
            with self.sim._lock:
                filenames = list(self.sim._image_filenames)
                store = self.sim._image_store
            lines = []
            for fn in filenames:
                size = len(store.get(fn, b""))
                lines.append(f"-rw-r--r-- 1 ftp ftp {size:>10d} Jan  1 00:00 {fn}")
            listing = "\r\n".join(lines)
            if listing:
                listing += "\r\n"
            data_conn.sendall(listing.encode("ascii"))
        finally:
            data_conn.close()
        self._send("226 Transfer complete")

    def _handle_retr(self, path: str) -> None:
        """Send a file's contents to the client."""
        # Normalize path: strip leading /IMAGE/ or /IMAGE\ prefix
        filename = path.strip("/")
        if filename.upper().startswith("IMAGE/"):
            filename = filename[6:]
        elif filename.upper().startswith("IMAGE\\"):
            filename = filename[6:]

        with self.sim._lock:
            data = self.sim._image_store.get(filename)

        if data is None:
            self._send(f"550 File not found: {path}")
            return

        self._send("150 Opening data connection")
        data_conn = self._get_data_conn()
        try:
            data_conn.sendall(data)
        finally:
            data_conn.close()
        self._send("226 Transfer complete")


class FtpServer(threading.Thread):
    """Minimal anonymous FTP server that serves images from the simulator's store."""

    def __init__(self, simulator: SRX300Simulator, host: str = "127.0.0.1", port: int = 21):
        super().__init__(daemon=True)
        self.sim = simulator
        self.host = host
        self.port = port

    def run(self) -> None:
        srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        srv.bind((self.host, self.port))
        srv.listen(4)
        logger.info("FTP server listening on %s:%d", self.host, self.port)
        while True:
            try:
                conn, addr = srv.accept()
                FtpClientHandler(conn, addr, self.sim, self.host).start()
            except OSError:
                break


class ClientHandler(threading.Thread):
    """Handles a single TCP client connection."""

    def __init__(self, conn: socket.socket, addr, simulator: SRX300Simulator):
        super().__init__(daemon=True)
        self.conn = conn
        self.addr = addr
        self.sim = simulator

    def run(self):
        logger.info("Client connected: %s:%d", *self.addr)
        buf = b""
        term = self.sim._get_in_terminator()
        try:
            while True:
                data = self.conn.recv(4096)
                if not data:
                    break
                buf += data
                # Process all complete messages in buffer
                while term in buf:
                    idx = buf.index(term)
                    raw_msg = buf[: idx + len(term)]
                    buf = buf[idx + len(term):]
                    cmd_str = self.sim._strip_framing(raw_msg)
                    if not cmd_str:
                        continue
                    logger.info("Recv from %s:%d -> %r", *self.addr, cmd_str)
                    responses = self.sim.process_command(cmd_str)
                    for resp in responses:
                        out = self.sim._wrap_response(resp)
                        logger.info("Send to %s:%d -> %r", *self.addr, resp)
                        self.conn.sendall(out)
        except (ConnectionResetError, BrokenPipeError, OSError) as e:
            logger.info("Client %s:%d disconnected (%s)", *self.addr, e)
        finally:
            self.conn.close()
            logger.info("Client %s:%d handler finished", *self.addr)


class FtpServer(threading.Thread):
    """Minimal anonymous FTP server that serves images from the simulator's store.

    Supports: USER, PASS, SYST, TYPE, PASV, NLST, RETR, QUIT, PWD, CWD.
    The virtual filesystem has a single directory /IMAGE/ containing all stored images.
    """

    def __init__(self, simulator: SRX300Simulator, host: str = "127.0.0.1", port: int = 21):
        super().__init__(daemon=True)
        self.sim = simulator
        self.host = host
        self.port = port
        self._server_sock: socket.socket | None = None

    def run(self):
        self._server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._server_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self._server_sock.bind((self.host, self.port))
        self._server_sock.listen(4)
        logger.info("FTP server listening on %s:%d", self.host, self.port)
        try:
            while True:
                conn, addr = self._server_sock.accept()
                threading.Thread(
                    target=self._handle_client, args=(conn, addr), daemon=True
                ).start()
        except OSError:
            pass  # server closed

    def _handle_client(self, conn: socket.socket, addr):
        logger.debug("FTP client connected: %s:%d", *addr)
        cwd = "/"
        transfer_type = "A"
        try:
            conn.sendall(b"220 SR-X FTP Server Ready\r\n")
            while True:
                data = conn.recv(4096)
                if not data:
                    break
                lines = data.decode("ascii", errors="replace").split("\r\n")
                for line in lines:
                    line = line.strip()
                    if not line:
                        continue
                    parts = line.split(None, 1)
                    cmd = parts[0].upper()
                    arg = parts[1] if len(parts) > 1 else ""
                    logger.debug("FTP << %s %s", cmd, arg)

                    if cmd == "USER":
                        conn.sendall(b"230 Login successful\r\n")
                    elif cmd == "PASS":
                        conn.sendall(b"230 Login successful\r\n")
                    elif cmd == "SYST":
                        conn.sendall(b"215 UNIX Type: L8\r\n")
                    elif cmd == "PWD":
                        conn.sendall(f'257 "{cwd}"\r\n'.encode())
                    elif cmd == "CWD":
                        target = arg.strip()
                        if target in ("/", "/IMAGE", "/IMAGE/", "IMAGE"):
                            cwd = "/IMAGE"
                            conn.sendall(b"250 Directory changed\r\n")
                        else:
                            conn.sendall(b"550 Directory not found\r\n")
                    elif cmd == "TYPE":
                        transfer_type = arg.strip().upper()
                        conn.sendall(b"200 Type set\r\n")
                    elif cmd == "PASV":
                        # Open a data port
                        data_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                        data_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                        data_sock.bind((self.host, 0))
                        data_sock.listen(1)
                        data_port = data_sock.getsockname()[1]
                        # Format IP for PASV response
                        ip_parts = self.host.replace(".", ",")
                        p1 = data_port >> 8
                        p2 = data_port & 0xFF
                        conn.sendall(
                            f"227 Entering Passive Mode ({ip_parts},{p1},{p2})\r\n".encode()
                        )
                        # Wait for data connection (with timeout)
                        data_sock.settimeout(10)
                        try:
                            data_conn, _ = data_sock.accept()
                        except socket.timeout:
                            data_sock.close()
                            conn.sendall(b"425 Can't open data connection\r\n")
                            continue

                        # Store for next transfer command
                        # Process next command that uses data connection
                        next_data = conn.recv(4096)
                        if not next_data:
                            data_conn.close()
                            data_sock.close()
                            break
                        next_lines = next_data.decode("ascii", errors="replace").split("\r\n")
                        for next_line in next_lines:
                            next_line = next_line.strip()
                            if not next_line:
                                continue
                            next_parts = next_line.split(None, 1)
                            next_cmd = next_parts[0].upper()
                            next_arg = next_parts[1] if len(next_parts) > 1 else ""
                            logger.debug("FTP << %s %s", next_cmd, next_arg)

                            if next_cmd == "NLST":
                                self._handle_nlst(conn, data_conn, next_arg, cwd)
                            elif next_cmd == "RETR":
                                self._handle_retr(conn, data_conn, next_arg, cwd)
                            else:
                                conn.sendall(f"502 Command not implemented: {next_cmd}\r\n".encode())
                                data_conn.close()
                            break
                        data_sock.close()
                    elif cmd == "NLST":
                        # Without PASV - shouldn't happen normally but handle gracefully
                        conn.sendall(b"425 Use PASV first\r\n")
                    elif cmd == "RETR":
                        conn.sendall(b"425 Use PASV first\r\n")
                    elif cmd == "QUIT":
                        conn.sendall(b"221 Goodbye\r\n")
                        conn.close()
                        return
                    else:
                        conn.sendall(f"502 Command not implemented: {cmd}\r\n".encode())
        except (ConnectionResetError, BrokenPipeError, OSError) as e:
            logger.debug("FTP client %s:%d disconnected: %s", *addr, e)
        finally:
            try:
                conn.close()
            except OSError:
                pass
            logger.debug("FTP client %s:%d handler finished", *addr)

    def _handle_nlst(self, ctrl: socket.socket, data: socket.socket, arg: str, cwd: str):
        """Handle NLST command - return file names."""
        ctrl.sendall(b"150 Opening data connection\r\n")
        # List all stored images
        with self.sim._lock:
            filenames = list(self.sim._image_filenames)
        listing = "\r\n".join(filenames)
        if listing:
            listing += "\r\n"
        data.sendall(listing.encode("ascii"))
        data.close()
        ctrl.sendall(b"226 Transfer complete\r\n")

    def _handle_retr(self, ctrl: socket.socket, data: socket.socket, arg: str, cwd: str):
        """Handle RETR command - send file data."""
        # Normalize path - strip /IMAGE/ prefix if present
        path = arg.strip()
        filename = path
        for prefix in ("/IMAGE/", "IMAGE/", "/"):
            if filename.startswith(prefix):
                filename = filename[len(prefix):]
                break

        with self.sim._lock:
            file_data = self.sim._image_store.get(filename)

        if file_data is None:
            data.close()
            ctrl.sendall(f"550 File not found: {path}\r\n".encode())
            return

        ctrl.sendall(b"150 Opening data connection\r\n")
        data.sendall(file_data)
        data.close()
        ctrl.sendall(b"226 Transfer complete\r\n")


def main():
    parser = argparse.ArgumentParser(
        description="SR-X 300 Reader Socket Interface Simulator"
    )
    parser.add_argument(
        "--host",
        default="127.0.0.1",
        help="Bind address (default: 127.0.0.1)",
    )
    parser.add_argument(
        "--port",
        type=int,
        default=DEFAULT_PORT,
        help=f"TCP port (default: {DEFAULT_PORT})",
    )
    parser.add_argument(
        "--comm-format",
        type=int,
        choices=[0, 1, 2],
        default=0,
        help="Communication format: 0=CR/CR (default), 1=CRLF/CR, 2=STX-ETX",
    )
    parser.add_argument(
        "--model",
        default=DEFAULT_MODEL,
        help=f"Simulated model name (default: {DEFAULT_MODEL})",
    )
    parser.add_argument(
        "--firmware",
        default=DEFAULT_FIRMWARE,
        help=f"Simulated firmware version (default: {DEFAULT_FIRMWARE})",
    )
    parser.add_argument(
        "--ftp-port",
        type=int,
        default=21,
        help="Port for the built-in FTP server (default: 21)",
    )
    args = parser.parse_args()

    sim = SRX300Simulator(comm_format=args.comm_format)
    sim.model = args.model
    sim.firmware = args.firmware

    # Start built-in FTP server for image retrieval
    ftp_srv = FtpServer(sim, host=args.host, port=args.ftp_port)
    ftp_srv.start()

    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind((args.host, args.port))
    srv.listen(4)
    logger.info(
        "SR-X 300 simulator listening on %s:%d (comm format %d)",
        args.host,
        args.port,
        args.comm_format,
    )

    try:
        while True:
            conn, addr = srv.accept()
            ClientHandler(conn, addr, sim).start()
    except KeyboardInterrupt:
        logger.info("Shutting down simulator")
    finally:
        srv.close()


if __name__ == "__main__":
    main()

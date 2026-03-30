#!/usr/bin/env python3
"""
SR-X 300 Reader Socket Interface Simulator

Simulates a KEYENCE SR-X 300 barcode reader's TCP socket command interface
as described in Chapter 14 (Command Communication) of the SRX300 User Manual.

Supports:
  - All three communication formats (CR, CR+LF, STX/ETX)
  - Operation commands (LON, LOFF, RESET, KEYENCE, NUM, EMAC, etc.)
  - Configuration commands (WB/RB, WP/RP, WD/RD, WN/RN)
  - Tuning commands (TUNE/FTUNE/TQUIT)
  - Save/Load/Initialize (SAVE, LOAD, DFLT)
  - Status queries (CMDSTAT, ERRSTAT, BUSYSTAT)
  - Error code responses for invalid commands

Usage:
    python srx300_simulator.py [--host HOST] [--port PORT] [--comm-format {0,1,2}]
"""

import argparse
import logging
import random
import socket
import string
import threading
import time

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

        # Operation configuration storage: key = cmd_number string
        self.op_config: dict[str, str] = {
            "101": "0",   # Timing mode: 0=Level trigger
            "102": "100", # One-shot trigger duration (x10 ms)
            "200": "0",   # Reading mode: 0=Standard
            "201": "0",   # Data transmission: 0=Send after read
            "205": "FF",  # Read error character string
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
        }

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
        }.get(cmd)

        if handler is None:
            # Also check LON with bank suffix: "LON01" etc.
            if cmd.startswith("LON") and len(cmd) > 3:
                return self._cmd_lon_bank(cmd[3:])
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
        return code_data


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
    args = parser.parse_args()

    sim = SRX300Simulator(comm_format=args.comm_format)
    sim.model = args.model
    sim.firmware = args.firmware

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

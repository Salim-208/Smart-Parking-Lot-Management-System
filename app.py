"""
Smart Parking Lot Management System - Web Backend
---------------------------------------------------
This Flask app is a thin HTTP layer over your ORIGINAL C LOGIC. It does not
reimplement the parking rules in Python: every park/exit/search/status call
below calls straight into libparking.so (built from c/bst.c, c/heap.c,
c/hash.c, c/queue.c, c/stack.c, c/fee.c, c/parking.c, c/file.c, plus the
thin c/webapi.c bridge) via ctypes. Flask's job is only to turn HTTP
requests into calls on that library and turn the results back into JSON.

Build the library first:  python build.py
Then run this:             python app.py

If the library hasn't been built yet, this file will tell you so and exit
with instructions rather than silently falling back to anything else.
"""

import ctypes
import platform
import sys
from pathlib import Path

from flask import Flask, jsonify, request, render_template, Response

ROOT = Path(__file__).parent

app = Flask(__name__)

# ---------------------------------------------------------------------------
# Load the compiled C library
# ---------------------------------------------------------------------------
def _library_path():
    system = platform.system()
    if system == "Windows":
        return ROOT / "parking.dll"
    if system == "Darwin":
        return ROOT / "libparking.dylib"
    return ROOT / "libparking.so"


LIB_PATH = _library_path()

if not LIB_PATH.exists():
    sys.exit(
        f"\nCouldn't find {LIB_PATH.name}.\n"
        "The C library hasn't been built yet. Run this first:\n\n"
        "    python build.py\n\n"
        "That compiles c/*.c (your original parking logic) into "
        f"{LIB_PATH.name}, which this app loads with ctypes.\n"
    )

try:
    lib = ctypes.CDLL(str(LIB_PATH))
except OSError as exc:
    if platform.system() == "Windows" and "193" in str(exc):
        import struct
        py_bits = struct.calcsize("P") * 8
        sys.exit(
            f"\nWindows refused to load {LIB_PATH.name} (WinError 193: not a "
            "valid Win32 application).\n"
            f"This means {LIB_PATH.name} was built for a different bitness than "
            f"this Python ({py_bits}-bit). The gcc that built it is the wrong "
            "architecture.\n\n"
            "Fix: install a matching MinGW-w64 toolchain (e.g. via MSYS2: "
            "https://www.msys2.org/), make sure the MinGW64 'gcc' (not MinGW32) "
            "is first on PATH, then re-run:\n\n"
            "    python build.py\n\n"
            "build.py will now check and warn you about this automatically "
            "before it lets a mismatched library through.\n"
        )
    raise

LOTS, SLOTS, VIP_SLOTS = 3, 10, 3   # must match c/parking.h

# Windows MinGW/UCRT uses 64-bit time_t while 32-bit long is still common in
# many C interfaces; keep the ctypes mirror aligned with the actual C ABI.
TIME_T = ctypes.c_longlong if platform.system() == "Windows" else ctypes.c_long


# ---- ctypes struct mirrors of c/webapi.h -----------------------------------
class WebVehicle(ctypes.Structure):
    _fields_ = [
        ("number", ctypes.c_char * 20),
        ("type", ctypes.c_char * 20),
        ("lot", ctypes.c_int),
        ("slot", ctypes.c_int),
        ("vip", ctypes.c_int),
        ("entryTime", TIME_T),
        ("entryTimeText", ctypes.c_char * 20),   # formatted in C
        ("parkedSeconds", ctypes.c_long),         # computed in C
    ]


class WebQueueItem(ctypes.Structure):
    _fields_ = [
        ("number", ctypes.c_char * 20),
        ("type", ctypes.c_char * 20),
        ("vip", ctypes.c_int),
    ]


MAX_LIST = 64  # LOTS*SLOTS is 30; 64 leaves headroom

# ---- function signatures, matching c/webapi.h exactly ----------------------
lib.wp_initialize.argtypes = []
lib.wp_initialize.restype = None

lib.wp_park.argtypes = [
    ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int,
    ctypes.POINTER(ctypes.c_int), ctypes.POINTER(ctypes.c_int), ctypes.POINTER(ctypes.c_int),
]
lib.wp_park.restype = ctypes.c_int

lib.wp_exit.argtypes = [
    ctypes.c_char_p,
    ctypes.POINTER(WebVehicle), ctypes.POINTER(ctypes.c_double), ctypes.POINTER(ctypes.c_int),
    ctypes.POINTER(WebQueueItem), ctypes.c_int, ctypes.POINTER(ctypes.c_int),
]
lib.wp_exit.restype = ctypes.c_int

lib.wp_search.argtypes = [ctypes.c_char_p, ctypes.POINTER(WebVehicle), ctypes.POINTER(WebQueueItem)]
lib.wp_search.restype = ctypes.c_int

lib.wp_get_grid.argtypes = [ctypes.POINTER(ctypes.c_int * (LOTS * SLOTS))]
lib.wp_get_grid.restype = None

lib.wp_free_count.argtypes = []
lib.wp_free_count.restype = ctypes.c_int

lib.wp_used_count.argtypes = []
lib.wp_used_count.restype = ctypes.c_int

lib.wp_vip_free_count.argtypes = []
lib.wp_vip_free_count.restype = ctypes.c_int

lib.wp_queue_count.argtypes = []
lib.wp_queue_count.restype = ctypes.c_int

lib.wp_list_vehicles.argtypes = [ctypes.POINTER(WebVehicle), ctypes.c_int]
lib.wp_list_vehicles.restype = ctypes.c_int

lib.wp_list_queue.argtypes = [ctypes.POINTER(WebQueueItem), ctypes.c_int]
lib.wp_list_queue.restype = ctypes.c_int

lib.wp_get_rate.argtypes = [ctypes.c_char_p]
lib.wp_get_rate.restype = ctypes.c_double

lib.wp_save_records.argtypes = []
lib.wp_save_records.restype = None

lib.wp_initialize()


# ---- small helpers: decode C structs into JSON-friendly dicts -------------
# No time/date arithmetic happens on the Python side - entryTimeText and
# parkedSeconds are both computed in c/webapi.c (fillDerived). Python only
# decodes bytes -> str and reshapes field names for the JSON response.
def vehicle_to_dict(v: WebVehicle):
    return {
        "number": v.number.decode(),
        "type": v.type.decode(),
        "lot": v.lot + 1,
        "slot": v.slot + 1,
        "vip": bool(v.vip),
        "entryTime": v.entryTimeText.decode(),
        "parkedMinutes": v.parkedSeconds / 60.0,
    }


def queue_item_to_dict(q: WebQueueItem):
    return {"number": q.number.decode(), "type": q.type.decode(), "vip": bool(q.vip)}


def encode(s: str) -> bytes:
    return s.encode("utf-8")[:19]  # fields are char[20]


# ---------------------------------------------------------------------------
# Routes
# ---------------------------------------------------------------------------
@app.route("/")
def index():
    return render_template("index.html")


@app.route("/api/status")
def api_status():
    grid_buf = (ctypes.c_int * (LOTS * SLOTS))()
    lib.wp_get_grid(ctypes.byref(grid_buf))
    lots = [list(grid_buf[i * SLOTS:(i + 1) * SLOTS]) for i in range(LOTS)]

    return jsonify({
        "lots": lots,
        "lotCount": LOTS,
        "slotsPerLot": SLOTS,
        "vipSlots": VIP_SLOTS,
        "free": lib.wp_free_count(),
        "used": lib.wp_used_count(),
        "total": LOTS * SLOTS,
        "vipFree": lib.wp_vip_free_count(),
        "queued": lib.wp_queue_count(),
    })


@app.route("/api/vehicles")
def api_vehicles():
    buf = (WebVehicle * MAX_LIST)()
    n = lib.wp_list_vehicles(buf, MAX_LIST)
    # already sorted (BST inorder traversal, by vehicle number) - no
    # Python-side sorting or computation applied.
    return jsonify([vehicle_to_dict(buf[i]) for i in range(n)])


@app.route("/api/queue")
def api_queue():
    buf = (WebQueueItem * MAX_LIST)()
    n = lib.wp_list_queue(buf, MAX_LIST)
    return jsonify([queue_item_to_dict(buf[i]) for i in range(n)])


@app.route("/api/history")
def api_history():
    # The original C program never kept a rolling activity feed (the Stack
    # in stack.c is pushed to on park but never displayed). This endpoint
    # is left as an empty, always-valid feed so the frontend keeps working;
    # ask if you'd like park/exit events surfaced here from the C stack.
    return jsonify([])


@app.route("/api/park", methods=["POST"])
def api_park():
    data = request.get_json(force=True) or {}
    number = (data.get("number") or "").strip()
    vtype = (data.get("type") or "").strip().lower()
    vip = 1 if data.get("vip") else 0

    if not number or not vtype:
        return jsonify({"ok": False, "error": "Vehicle number and type are required."}), 400

    lot = ctypes.c_int()
    slot = ctypes.c_int()
    queue_pos = ctypes.c_int()

    result = lib.wp_park(encode(number), encode(vtype), vip,
                          ctypes.byref(lot), ctypes.byref(slot), ctypes.byref(queue_pos))

    if result == -1:
        return jsonify({"ok": False, "error": "Vehicle already parked or already queued."}), 400
    if result == 1:
        return jsonify({
            "ok": True, "parked": True,
            "vehicle": {"number": number, "type": vtype, "vip": bool(vip),
                        "lot": lot.value + 1, "slot": slot.value + 1},
        })
    return jsonify({"ok": True, "parked": False, "queuePosition": queue_pos.value})


@app.route("/api/exit", methods=["POST"])
def api_exit():
    data = request.get_json(force=True) or {}
    number = (data.get("number") or "").strip()
    if not number:
        return jsonify({"ok": False, "error": "Vehicle number is required."}), 400

    record = WebVehicle()
    fee = ctypes.c_double()
    hours = ctypes.c_int()
    assigned_buf = (WebQueueItem * MAX_LIST)()
    assigned_count = ctypes.c_int()

    result = lib.wp_exit(encode(number), ctypes.byref(record), ctypes.byref(fee),
                          ctypes.byref(hours), assigned_buf, MAX_LIST, ctypes.byref(assigned_count))

    if result == 0:
        return jsonify({"ok": False, "error": "Vehicle not found."}), 404

    rate = lib.wp_get_rate(encode(record.type.decode()))
    assigned = [queue_item_to_dict(assigned_buf[i])["number"] for i in range(assigned_count.value)]

    return jsonify({
        "ok": True,
        "bill": {
            "number": record.number.decode(),
            "type": record.type.decode(),
            "lot": record.lot + 1,
            "slot": record.slot + 1,
            "hours": hours.value,
            "rate": rate,
            "fee": fee.value,   # already rounded in C
        },
        "queueAssigned": assigned,
    })


@app.route("/api/search/<number>")
def api_search(number):
    vehicle = WebVehicle()
    queue_item = WebQueueItem()

    result = lib.wp_search(encode(number), ctypes.byref(vehicle), ctypes.byref(queue_item))

    if result == 1:
        return jsonify({"ok": True, "found": True, "location": "parked", "vehicle": vehicle_to_dict(vehicle)})
    if result == 2:
        return jsonify({"ok": True, "found": True, "location": "queue", "vehicle": queue_item_to_dict(queue_item)})
    return jsonify({"ok": True, "found": False})


@app.route("/api/save")
def api_save():
    # Calls the original saveRecords() from file.c, which writes
    # parking_records.txt into this process's working directory.
    lib.wp_save_records()
    out_file = ROOT / "parking_records.txt"
    if not out_file.exists():
        return jsonify({"ok": False, "error": "File could not be written."}), 500
    return Response(
        out_file.read_bytes(),
        mimetype="text/plain",
        headers={"Content-Disposition": "attachment; filename=parking_records.txt"},
    )


if __name__ == "__main__":
    app.run(debug=True, port=5000)

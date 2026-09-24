# Smart Parking Lot Management System — Web Edition (backed by your C code)

The frontend now talks to your **actual C logic**, not a Python rewrite.

```
Browser  <-->  Flask (app.py)  <-->  ctypes  <-->  libparking.so
 (HTML/JS)      REST API                          (compiled from c/*.c)
```

`c/` contains your original files unchanged — `bst.c`, `heap.c`, `hash.c`,
`queue.c`, `stack.c`, `fee.c`, `parking.c`, `file.c` — plus one new file,
`c/webapi.c` (and its header `c/webapi.h`), which is a thin bridge that
exposes clean, pointer-free functions (`wp_park`, `wp_exit`, `wp_search`,
`wp_get_grid`, ...) for `ctypes` to call. It doesn't reimplement any parking
logic itself — it just calls your existing `insertVehicle`, `buildHeap`,
`removeMinimum`, `insertHash`, `deleteHash`, `getRate`, `saveRecords`, etc.,
the same way your original `main.c` did, and copies the results into plain
structs Python can read directly.

`main.c` itself isn't used — it's just the console menu loop (`scanf`/
`printf`), which doesn't apply to a web app. Everything **inside** the menu
branches (the actual parking rules) is what `webapi.c` calls into.

One real behavioural addition: your original `dequeue()` was defined but
never called anywhere in `main.c`, so the waiting queue could never empty
out. `webapi.c`'s `wp_exit` now calls `dequeue()` on the freed slot, so a
waiting vehicle is automatically seated when a bay opens up (strict FIFO —
it only tries the vehicle at the front of the queue).

## Set up

**1. Build the C library** (needs a C compiler on PATH):

```bash
cd smart-parking-system
python build.py
```

This runs `gcc` (or `cc`/`clang`) over `c/*.c` and produces:
- `libparking.so` on Linux
- `libparking.dylib` on macOS
- `parking.dll` on Windows (needs MinGW-w64, e.g. from MSYS2, so `gcc` is on PATH — the same kind of toolchain that originally produced `parking.exe`)

**2. Install Python deps and run the server:**

```bash
pip install -r requirements.txt
python app.py
```

Open **http://localhost:5000**. `app.py` loads the library with `ctypes` at
startup and exits with a clear message if you skip step 1.

If you ever change any `.c` file, re-run `python build.py` and restart
`app.py` — Python doesn't recompile C for you.

## Project layout

```
smart-parking-system/
├── c/
│   ├── bst.c / bst.h          (yours, unchanged)
│   ├── fee.c / fee.h          (yours, unchanged)
│   ├── file.c / file.h        (yours, unchanged)
│   ├── hash.c / hash.h        (yours, unchanged)
│   ├── heap.c / heap.h        (yours, unchanged)
│   ├── parking.c / parking.h  (yours, unchanged)
│   ├── queue.c / queue.h      (yours, unchanged)
│   ├── stack.c / stack.h      (yours, unchanged)
│   └── webapi.c / webapi.h    (new — thin ctypes bridge, no business logic)
├── build.py             # compiles c/*.c into libparking.so / .dylib / .dll
├── app.py               # Flask app; every route calls into the C library
├── requirements.txt
├── templates/
│   └── index.html       # dashboard page
└── static/
    ├── style.css         # teal / amber theme (no dark mode)
    └── script.js         # calls the REST API, renders the dashboard
```

## API reference

| Method | Endpoint              | Body / Params          | Calls into C via                                  |
|--------|------------------------|--------------------------|-----------------------------------------------------|
| GET    | `/api/status`          | —                         | `wp_get_grid`, `wp_free_count`, `wp_used_count`, `wp_vip_free_count`, `wp_queue_count` |
| GET    | `/api/vehicles`        | —                         | `wp_list_vehicles` (BST inorder traversal)         |
| GET    | `/api/queue`           | —                         | `wp_list_queue`                                    |
| POST   | `/api/park`            | `{number, type, vip}`    | `wp_park` -> `buildHeap` + `removeMinimum` + `insertVehicle` + `insertHash` + `push` |
| POST   | `/api/exit`            | `{number}`                | `wp_exit` -> `searchVehicle` + `getRate` + `deleteHash` + `deleteVehicle` + `dequeue` |
| GET    | `/api/search/<number>` | —                         | `wp_search` -> `searchVehicle`, queue scan          |
| GET    | `/api/save`             | —                         | `wp_save_records` -> `saveRecords` (writes `parking_records.txt` exactly as the original) |

## Division of labor

`app.py` contains **no parking rules, no fee math, no time arithmetic, and
no sorting** — every one of those lives in `c/`. Concretely:

- Elapsed parking time and the formatted entry timestamp shown in the
  vehicle table are computed in `webapi.c`'s `fillDerived()`, not in Python.
- The parked-vehicle list is returned by `wp_list_vehicles` already in BST
  order (sorted by vehicle number) - Python doesn't re-sort it.
- Fees are computed and rounded to 2 decimals inside `wp_exit` in `webapi.c`.
- Every decision that affects the system's behaviour - which slot a vehicle
  gets, whether it's VIP-eligible, whether it queues, what it's billed,
  when a queued vehicle gets seated - happens in C (`bst.c`, `heap.c`,
  `hash.c`, `queue.c`, `stack.c`, `fee.c`, `parking.c`, `file.c`).

`app.py`'s job is strictly: route an HTTP request to the matching `wp_*`
call, decode the returned C struct's byte strings into Python strings, and
hand the result to Flask's `jsonify`. `static/script.js` then renders
whatever JSON it's given - it doesn't compute fees or slot assignments
either.

## Notes / things you might want next

- **State lives in the C library's memory** for as long as `app.py` is
  running (same as your original program while it ran). Restarting
  `app.py` resets everything, just like restarting the console program did.
  `/api/save` already exports the current state via your original
  `saveRecords()` — say the word if you want it to auto-load that file back
  in on startup too.
- **Activity feed**: the dashboard has an "Activity" panel, but it's
  currently empty — your original `Stack` (`stack.c`) is pushed to on
  park but was never read back out or displayed in `main.c`, so there was
  no real log to surface. I left `/api/history` returning an empty list
  rather than inventing behaviour your C code didn't have. If you'd like,
  I can add a `wp_list_recent()` function to `webapi.c` that reads the
  stack (and push on exit too) so that panel shows real park/exit events.

const $ = (sel) => document.querySelector(sel);

// ---------- clock ----------
function tickClock() {
  $("#clock").textContent = new Date().toLocaleTimeString();
}
setInterval(tickClock, 1000);
tickClock();

// ---------- tabs ----------
document.querySelectorAll(".tab").forEach((tab) => {
  tab.addEventListener("click", () => {
    document.querySelectorAll(".tab").forEach((t) => t.classList.remove("active"));
    document.querySelectorAll(".tab-pane").forEach((p) => p.classList.remove("active"));
    tab.classList.add("active");
    $(`#form-${tab.dataset.tab}`).classList.add("active");
  });
});

// ---------- helpers ----------
async function api(path, options) {
  const res = await fetch(path, options);
  const data = await res.json();
  return { ok: res.ok, data };
}

function showResult(kind, title, html) {
  const panel = $("#result-panel");
  panel.className = `result-panel ${kind}`;
  panel.innerHTML = `<span class="rp-title">${title}</span>${html}`;
}

function fmtDuration(minutes) {
  if (minutes < 60) return `${Math.round(minutes)} min`;
  const h = Math.floor(minutes / 60);
  const m = Math.round(minutes % 60);
  return `${h}h ${m}m`;
}

// ---------- render: lot map ----------
function renderLots(status) {
  const container = $("#lots");
  container.innerHTML = "";
  status.lots.forEach((row, lotIndex) => {
    const wrap = document.createElement("div");
    wrap.className = "lot-row";

    const title = document.createElement("div");
    title.className = "lot-title";
    title.textContent = `Lot ${lotIndex + 1}`;
    wrap.appendChild(title);

    const grid = document.createElement("div");
    grid.className = "slot-grid";
    row.forEach((state, slotIndex) => {
      const cell = document.createElement("div");
      cell.className = "slot" + (state ? " occupied" : "") + (slotIndex < status.vipSlots ? " vip-bay" : "");
      cell.textContent = slotIndex + 1;
      cell.title = `Lot ${lotIndex + 1}, Slot ${slotIndex + 1}${slotIndex < status.vipSlots ? " (VIP)" : ""} — ${state ? "Occupied" : "Free"}`;
      grid.appendChild(cell);
    });
    wrap.appendChild(grid);
    container.appendChild(wrap);
  });
}

// ---------- render: stats ----------
function renderStats(status) {
  $("#stat-free").textContent = status.free;
  $("#stat-used").textContent = status.used;
  $("#stat-vip").textContent = status.vipFree;
  $("#stat-queue").textContent = status.queued;
}

// ---------- render: vehicle table ----------
function renderVehicles(list) {
  const body = $("#vehicles-body");
  if (!list.length) {
    body.innerHTML = `<tr><td colspan="6" class="muted">No vehicles parked yet.</td></tr>`;
    return;
  }
  body.innerHTML = list.map((v) => `
    <tr>
      <td>${v.number}</td>
      <td>${v.type}</td>
      <td>${v.lot}</td>
      <td>${v.slot}</td>
      <td>${v.vip ? '<span class="badge-vip">VIP</span>' : ""}</td>
      <td>${fmtDuration(v.parkedMinutes)}</td>
    </tr>
  `).join("");
}

// ---------- render: queue table ----------
function renderQueue(list) {
  const body = $("#queue-body");
  if (!list.length) {
    body.innerHTML = `<tr><td colspan="4" class="muted">Queue is empty.</td></tr>`;
    return;
  }
  body.innerHTML = list.map((w, i) => `
    <tr>
      <td>${i + 1}</td>
      <td>${w.number}</td>
      <td>${w.type}</td>
      <td>${w.vip ? '<span class="badge-vip">VIP</span>' : ""}</td>
    </tr>
  `).join("");
}

// ---------- render: activity log ----------
function renderLog(list) {
  const log = $("#activity-log");
  if (!list.length) {
    log.innerHTML = `<li class="muted">No activity yet.</li>`;
    return;
  }
  log.innerHTML = list.map((item) => `
    <li><span>${item.message}</span><time>${item.time}</time></li>
  `).join("");
}

// ---------- refresh everything ----------
async function refreshAll() {
  const [status, vehicles, queue, history] = await Promise.all([
    api("/api/status"),
    api("/api/vehicles"),
    api("/api/queue"),
    api("/api/history"),
  ]);
  if (status.ok) { renderStats(status.data); renderLots(status.data); }
  if (vehicles.ok) renderVehicles(vehicles.data);
  if (queue.ok) renderQueue(queue.data);
  if (history.ok) renderLog(history.data);
}

// ---------- form: park ----------
$("#form-park").addEventListener("submit", async (e) => {
  e.preventDefault();
  const number = $("#park-number").value.trim();
  const type = $("#park-type").value;
  const vip = $("#park-vip").checked;

  const { ok, data } = await api("/api/park", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ number, type, vip }),
  });

  if (!ok) {
    showResult("error", "Could not park vehicle", `<p>${data.error}</p>`);
    return;
  }

  if (data.parked) {
    const v = data.vehicle;
    showResult("success", "Vehicle parked", `<p>${v.number} assigned to Lot ${v.lot + 1}, Slot ${v.slot + 1}.</p>`);
  } else {
    showResult("error", "Lot is full", `<p>${number} added to the waiting queue at position ${data.queuePosition}.</p>`);
  }

  $("#form-park").reset();
  refreshAll();
});

// ---------- form: exit ----------
$("#form-exit").addEventListener("submit", async (e) => {
  e.preventDefault();
  const number = $("#exit-number").value.trim();

  const { ok, data } = await api("/api/exit", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ number }),
  });

  if (!ok) {
    showResult("error", "Could not exit vehicle", `<p>${data.error}</p>`);
    return;
  }

  const b = data.bill;
  $("#bill-body").innerHTML = `
    <div class="bill-row"><span>Vehicle</span><span>${b.number}</span></div>
    <div class="bill-row"><span>Type</span><span>${b.type}</span></div>
    <div class="bill-row"><span>Lot / Slot</span><span>${b.lot} / ${b.slot}</span></div>
    <div class="bill-row"><span>Billed hours</span><span>${b.hours}</span></div>
    <div class="bill-row"><span>Rate</span><span>₹${b.rate.toFixed(2)} / hr</span></div>
    <div class="bill-row total"><span>Total</span><span>₹${b.fee.toFixed(2)}</span></div>
  `;
  $("#bill-overlay").classList.remove("hidden");

  if (data.queueAssigned && data.queueAssigned.length) {
    showResult("success", "Vehicle exited", `<p>Bill generated for ${b.number}. ${data.queueAssigned.join(", ")} moved in from the waiting queue.</p>`);
  } else {
    showResult("success", "Vehicle exited", `<p>Bill generated for ${b.number}.</p>`);
  }

  $("#form-exit").reset();
  refreshAll();
});

$("#bill-close").addEventListener("click", () => $("#bill-overlay").classList.add("hidden"));

// ---------- form: search ----------
$("#form-search").addEventListener("submit", async (e) => {
  e.preventDefault();
  const number = $("#search-number").value.trim();

  const { data } = await api(`/api/search/${encodeURIComponent(number)}`);

  if (!data.found) {
    showResult("error", "Not found", `<p>No vehicle matching "${number}" is parked or queued.</p>`);
    return;
  }

  const v = data.vehicle;
  if (data.location === "parked") {
    showResult("success", "Vehicle found", `
      <p>${v.number} (${v.type}${v.vip ? ", VIP" : ""}) is in Lot ${v.lot}, Slot ${v.lot ? v.slot : v.slot}.</p>
      <p>Entered at ${v.entryTime}, parked for ${fmtDuration(v.parkedMinutes)}.</p>
    `);
  } else {
    showResult("error", "Vehicle is queued", `<p>${v.number} (${v.type}${v.vip ? ", VIP" : ""}) is waiting for a free bay.</p>`);
  }
});

refreshAll();
setInterval(refreshAll, 5000);

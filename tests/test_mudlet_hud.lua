-- Run in real Lua 5.1/LuaJIT, with strict headless Mudlet/Geyser API doubles.
local source = assert(arg[1], "HUD source path required")
local tests, handlers, windows, nextId, registrations, failAt
local function active()
  local n = 0
  for _ in pairs(handlers) do n = n + 1 end
  return n
end
local function fire(event, ...)
  local copy = {}
  for id, entry in pairs(handlers) do
    if entry.event == event then copy[#copy + 1] = {id = id, entry = entry} end
  end
  table.sort(copy, function(a, b) return a.id < b.id end)
  for _, item in ipairs(copy) do item.entry.callback(event, ...) end
end
local function fresh()
  if AnathemaHUD and AnathemaHUD.destroy then AnathemaHUD:destroy() end
  AnathemaHUD = nil
  handlers, windows, nextId, registrations, failAt = {}, {}, 0, 0, nil
  gmcp = {}
  local methods = {}
  function methods:hide()
    self.visible = false
    for _, child in ipairs(self.children) do child:hide() end
  end
  function methods:show()
    self.visible = true
    for _, child in ipairs(self.children) do child:show() end
  end
  function methods:delete()
    for _, child in ipairs(self.children) do child:delete() end
    windows[self.name] = nil
    self.deleted = true
  end
  function methods:echo(text) assert(type(text) == "string"); self.text = text end
  function methods:setStyleSheet(text) self.style = text end
  function methods:setFontSize(size) assert(size > 0) end
  function methods:enableClickthrough() self.clickthrough = true end
  function methods:setColor(r, g, b)
    assert(type(r) == "number" and type(g) == "number" and type(b) == "number")
  end
  function methods:move(x, y) self.x, self.y = x, y end
  function methods:setValue(value, maximum, text)
    assert(value == value and value >= 0 and value <= 100)
    assert(maximum == 100 and type(text) == "string")
    self.value, self.text = value, text
  end
  local function class(kind)
    return {new = function(_, options, parent)
      assert(not windows[options.name], "duplicate window " .. options.name)
      local object = setmetatable({name = options.name, kind = kind,
        children = {}, visible = not options.hidden, text = "",
        x = options.x, y = options.y}, {__index = methods})
      windows[object.name] = object
      if parent then parent.children[#parent.children + 1] = object end
      return object
    end}
  end
  Geyser = {Container = class("container"), Label = class("label"), Gauge = class("gauge")}
  registerAnonymousEventHandler = function(event, callback)
    registrations = registrations + 1
    if registrations == failAt then return nil end
    nextId = nextId + 1
    handlers[nextId] = {event = event, callback = callback}
    return nextId
  end
  killAnonymousEventHandler = function(id)
    assert(handlers[id], "unknown or already removed handler")
    handlers[id] = nil
  end
  for _, name in ipairs({"send", "sendGMCP", "sendSocket", "reconnect",
                        "connectToServer", "tempTimer", "setBorderRight",
                        "setBorderTop", "setBorderBottom", "setBorderLeft"}) do
    _G[name] = function() error("Forbidden network, timer or layout side effect: " .. name) end
  end
end
local function load()
  dofile(source)
  assert(AnathemaHUD and active() == 6)
  return AnathemaHUD
end
local function connect()
  fire("sysConnectionEvent")
  fire("sysProtocolEnabled", "GMCP")
end
local function char(values)
  gmcp.Char = {Vitals = values or {hp = 71, maxhp = 100, mp = 33, maxmp = 99}}
  fire("gmcp.Char.Vitals")
end
local function custom(kind, extra, available)
  local packet = extra or {}
  packet.class = kind
  packet.available = available ~= false
  gmcp.Anathema = {Vitals = packet}
  fire("gmcp.Anathema.Vitals")
end
local function snapshot(kind, extra, values)
  char(values); custom(kind or "monk", extra)
end
local function hidden(h)
  for _, row in ipairs(h.rows) do
    assert(not row.gauge.visible and not row.readout.visible)
    assert(row.gauge.text == "" and row.readout.text == "")
  end
  assert(h.data == nil)
end
tests = 0
local function test(name, callback)
  fresh()
  local ok, problem = pcall(callback)
  if not ok then error(name .. ": " .. tostring(problem)) end
  tests = tests + 1
  print("PASS " .. name)
end

test("installation ignores cached authenticated-looking data", function()
  gmcp = {Char = {Vitals = {hp = 999, maxhp = 1000, mp = 55, maxmp = 100}},
          Anathema = {Vitals = {available = true, class = "vampire",
                       blood = {current = 999, max = 1000}}}}
  local h = load()
  hidden(h)
  fire("gmcp.Char.Vitals"); fire("gmcp.Anathema.Vitals")
  hidden(h)
end)
test("complete ordered health/class pair required", function()
  local h = load(); connect(); custom("monk")
  hidden(h)
  char(); hidden(h)
  custom("monk")
  assert(h.rows[1].gauge.text == "Health: 71 / 100")
  assert(math.abs(h.rows[2].gauge.value - 100 / 3) < 0.001)
end)
test("all ten classes choose only their resources", function()
  local h = load(); connect()
  local extra = {blood = {current = 17, max = 50}, ki = {current = 20, max = 300},
    quintessence = {current = 4}, paradox = {current = 3},
    class_points = {current = 500}, willpower = {current = 5, max = 10}}
  for kind, count in pairs({vampire = 1, ninja = 1, mage = 2, drow = 1,
      dragon = 1, werewolf = 1, monk = 0, elemental = 0, sorcerer = 0,
      highlander = 0, none = 0, unknown = 0}) do
    snapshot(kind, extra)
    assert(#h.data.resources == count, kind)
  end
end)
test("class switches ignore merged stale null resources", function()
  local h = load(); connect()
  local merged = {blood = {current = 17, max = 50}}
  snapshot("vampire", merged)
  assert(h.rows[3].gauge.visible)
  merged.quintessence, merged.paradox = {current = 5}, {current = 1}
  snapshot("mage", merged) -- Blood intentionally left in the merged table.
  assert(h.rows[3].readout.text == "Quintessence: 5")
  snapshot("monk", merged)
  assert(not h.rows[3].readout.visible and h.rows[3].readout.text == "")
end)
test("unknown caps are numeric readouts, not invented bars", function()
  local h = load(); connect(); snapshot("drow", {class_points = {current = 500}})
  assert(h.rows[3].readout.visible and not h.rows[3].gauge.visible)
  assert(h.rows[3].readout.text == "Class points: 500")
end)
test("negative and over-cap values clamp drawings only", function()
  local h = load(); connect()
  snapshot("ninja", {ki = {current = 500, max = 300}},
           {hp = -7, maxhp = 100, mp = 250, maxmp = 100})
  assert(h.rows[1].gauge.value == 0 and h.rows[1].gauge.text == "Health: -7 / 100")
  assert(h.rows[2].gauge.value == 100 and h.rows[3].gauge.value == 100)
end)
test("zero and negative maxima never divide or fake a cap", function()
  local h = load(); connect()
  snapshot("monk", nil, {hp = 2, maxhp = 0, mp = 3, maxmp = -1})
  assert(h.rows[1].readout.text == "Health: 2 / 0")
  assert(h.rows[2].readout.text == "Mana: 3 / -1")
  assert(not h.rows[1].gauge.visible)
end)
test("huge legacy numbers are visibly approximate", function()
  local h = load(); connect()
  snapshot("monk", nil, {hp = 9223372036854775807, maxhp = 1, mp = 0, maxmp = 1})
  assert(h.rows[1].gauge.text:match("Health: ~"))
  assert(h.rows[1].gauge.value == 100)
end)
test("NaN infinity fractions and invalid fields clear old state", function()
  local h = load(); connect(); snapshot("monk")
  for _, bad in ipairs({0/0, math.huge, -math.huge, 1.5, "<b>99</b>", false}) do
    char({hp = bad, maxhp = 100, mp = 10, maxmp = 100})
    custom("monk"); hidden(h)
  end
end)
test("malformed resources and userdata null are ignored", function()
  local h = load(); connect()
  snapshot("mage", {quintessence = io.stdout, paradox = {current = math.huge}})
  assert(#h.data.resources == 0)
end)
test("HTML class and numeric strings cannot inject labels", function()
  local h = load(); connect()
  snapshot("<img src=x>", {blood = {current = "<script>", max = 50}})
  assert(h.status.text == "Live GMCP | Unknown" and #h.data.resources == 0)
  for _, widget in pairs(windows) do
    assert(not widget.text:find("<img", 1, true))
    assert(not widget.text:find("<script", 1, true))
  end
end)
test("numeric strings are copied not retained by reference", function()
  local h = load(); connect()
  local values = {hp = "71", maxhp = "100", mp = "33", maxmp = "99"}
  snapshot("monk", nil, values)
  values.hp = 999
  assert(h.data.hp == 71)
end)
test("unavailable switched view clears every row", function()
  local h = load(); connect()
  snapshot("vampire", {blood = {current = 17, max = 50}})
  char(); custom("vampire", {}, false); hidden(h)
  assert(h.status.text == "Vitals unavailable")
end)
test("disconnect blocks late data until fresh negotiation", function()
  local h = load(); connect(); snapshot("monk")
  fire("sysDisconnectionEvent"); hidden(h)
  snapshot("monk"); hidden(h)
  fire("sysConnectionEvent"); snapshot("monk"); hidden(h)
  fire("sysProtocolEnabled", "GMCP"); snapshot("monk")
  assert(h.data.hp == 71)
end)
test("copyover protocol reset clears cached values", function()
  local h = load(); connect(); snapshot("vampire", {blood = {current = 17, max = 50}})
  fire("sysProtocolDisabled", "GMCP"); hidden(h)
  snapshot("vampire"); hidden(h)
  fire("sysProtocolEnabled", "GMCP"); hidden(h)
  snapshot("monk")
  assert(h.data and #h.data.resources == 0)
end)
test("other protocol events do not affect a valid view", function()
  local h = load(); connect(); snapshot("monk")
  fire("sysProtocolDisabled", "MSDP"); fire("sysProtocolEnabled", "MXP")
  assert(h.data.hp == 71)
end)
test("hidden HUD remains hidden during live updates", function()
  local h = load(); connect(); h:hide()
  snapshot("vampire", {blood = {current = 17, max = 50}})
  assert(not h.panel.visible and not h.rows[1].gauge.visible)
  h:show()
  assert(h.rows[1].gauge.visible and h.rows[3].gauge.visible)
  assert(not h.rows[3].readout.visible and not h.rows[4].gauge.visible)
end)
test("show while waiting cannot reveal old row text", function()
  local h = load(); connect(); snapshot("monk"); fire("sysDisconnectionEvent")
  h:hide(); h:show(); hidden(h)
end)
test("reload deletes old widgets and all old handlers", function()
  local old = load(); connect(); snapshot("monk")
  local callback = handlers[5].callback
  local h = load()
  assert(old.destroyed and h ~= old and active() == 6)
  hidden(h); callback("gmcp.Char.Vitals"); hidden(h)
end)
test("destroy is idempotent and removes all owned UI", function()
  local h = load()
  h:destroy(); h:destroy()
  assert(active() == 0 and next(windows) == nil and AnathemaHUD == nil)
end)
test("partial event-registration failure rolls back cleanly", function()
  failAt = 3
  local ok = pcall(dofile, source)
  assert(not ok and active() == 0 and next(windows) == nil and AnathemaHUD == nil)
end)
test("missing Geyser reports failure without leftover handlers", function()
  Geyser = nil
  assert(not pcall(dofile, source) and active() == 0 and AnathemaHUD == nil)
end)
test("unrelated global namespace is never overwritten", function()
  local other = {mine = true}
  AnathemaHUD = other
  assert(not pcall(dofile, source) and AnathemaHUD == other and active() == 0)
end)
print("Mudlet HUD headless Lua cases passed: " .. tests)

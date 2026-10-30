-- Godwars: Anathema - optional, display-only Mudlet GMCP HUD.
-- Lua 5.1/LuaJIT; requires current Mudlet 4.x with Geyser.
-- Import the generated XML OR paste this into one Script, not both.
-- Reconnect after installing/reloading: cached GMCP tables are never trusted.
local config = {x = "-310px", y = "20px", width = 290, height = 178}
local namespace = "AnathemaHUD"
local marker = "anathema-vitals-hud-v1"

assert(type(Geyser) == "table" and Geyser.Container and Geyser.Label and
       Geyser.Gauge, "Anathema HUD requires Mudlet with Geyser")
assert(type(registerAnonymousEventHandler) == "function" and
       type(killAnonymousEventHandler) == "function",
       "Anathema HUD requires Mudlet event-handler APIs")
local previous = rawget(_G, namespace)
if previous ~= nil then
  assert(type(previous) == "table" and previous.marker == marker and
         type(previous.destroy) == "function",
         "AnathemaHUD already belongs to another script; rename it first")
  previous:destroy()
end

local classes = {
  vampire = {label = "Vampire", keys = {"blood"}},
  ninja = {label = "Ninja", keys = {"ki"}},
  mage = {label = "Mage", keys = {"quintessence", "paradox"}},
  drow = {label = "Drow", keys = {"class_points"}},
  dragon = {label = "Dragon", keys = {"class_points"}},
  werewolf = {label = "Werewolf", keys = {"willpower"}},
  monk = {label = "Monk", keys = {}},
  elemental = {label = "Elemental", keys = {}},
  sorcerer = {label = "Sorcerer", keys = {}},
  highlander = {label = "Highlander", keys = {}},
  none = {label = "Unclassed", keys = {}},
  unknown = {label = "Unknown", keys = {}}
}
local resources = {
  blood = {label = "Blood", color = {150, 42, 60}},
  ki = {label = "Ki", color = {184, 153, 55}},
  quintessence = {label = "Quintessence", color = {120, 90, 185}},
  paradox = {label = "Paradox", color = {178, 88, 185}},
  class_points = {label = "Class points", color = {79, 139, 132}},
  willpower = {label = "Willpower", color = {87, 146, 96}}
}
local function integer(value)
  if type(value) == "string" and #value <= 32 then value = tonumber(value) end
  if type(value) ~= "number" or value ~= value or value == math.huge or
     value == -math.huge or value ~= math.floor(value) then return nil end
  return value
end
local function numberText(value)
  if value == nil then return "?" end
  if math.abs(value) > 9007199254740991 then
    return "~" .. string.format("%.3g", value)
  end
  return string.format("%.0f", value)
end
local function clickthrough(widget)
  if type(widget.enableClickthrough) == "function" then widget:enableClickthrough() end
end

local H = {marker = marker, handlers = {}, rows = {}, session = false,
           protocol = false, visible = true, destroyed = false}
function H:clear(message)
  self.pending, self.data = nil, nil
  self.message = message
  self:render()
end
function H:render()
  if self.destroyed or not self.panel then return end
  self.status:echo(self.message or "Waiting for fresh GMCP")
  for _, row in ipairs(self.rows) do
    row.gauge:setValue(0, 100, "")
    row.readout:echo("")
    row.gauge:hide()
    row.readout:hide()
  end
  if not self.data then return end
  local function draw(index, label, current, maximum, color)
    local row = self.rows[index]
    if current == nil then return end
    local text = label .. ": " .. numberText(current)
    if maximum ~= nil then text = text .. " / " .. numberText(maximum) end
    if maximum ~= nil and maximum > 0 then
      -- Compare before dividing: even hostile huge finite numbers cannot overflow.
      local percent = current <= 0 and 0 or
          (current >= maximum and 100 or current / maximum * 100)
      row.gauge:setColor(color[1], color[2], color[3])
      row.gauge:setValue(percent, 100, text)
      row.gauge:show()
    else
      row.readout:echo(text) -- Unknown/zero caps are readouts, never invented bars.
      row.readout:show()
    end
  end
  local d = self.data
  draw(1, "Health", d.hp, d.maxhp, {160, 56, 68})
  draw(2, "Mana", d.mp, d.maxmp, {52, 104, 167})
  for i, resource in ipairs(d.resources) do
    draw(i + 2, resource.label, resource.current, resource.max, resource.color)
  end
  if not self.visible then self.panel:hide() end
end
function H:hide()
  self.visible = false
  if self.panel then self.panel:hide() end
end
function H:show()
  if self.destroyed then return end
  self.visible = true
  self.panel:show()
  self:render() -- Parent show() also shows children; re-hide inapplicable rows.
end
function H:destroy()
  if self.destroyed then return end
  self.destroyed, self.session, self.protocol = true, false, false
  self.pending, self.data = nil, nil
  for _, id in ipairs(self.handlers) do pcall(killAnonymousEventHandler, id) end
  self.handlers = {}
  if self.panel then
    pcall(function() self.panel:hide() end)
    pcall(function() self.panel:delete() end)
    self.panel = nil
  end
  if rawget(_G, namespace) == self then rawset(_G, namespace, nil) end
end
local function standard()
  if H.destroyed or not H.session or not H.protocol then return end
  local packet = type(gmcp) == "table" and type(gmcp.Char) == "table" and
      gmcp.Char.Vitals
  H:clear("Waiting for fresh class data")
  if type(packet) ~= "table" then return end
  local values = {}
  for _, key in ipairs({"hp", "maxhp", "mp", "maxmp"}) do
    values[key] = integer(packet[key])
    if values[key] == nil then return end
  end
  H.pending = values -- Copy numbers; never retain Mudlet's merged GMCP table.
end
local function custom()
  if H.destroyed or not H.session or not H.protocol then return end
  local pending = H.pending
  local packet = type(gmcp) == "table" and type(gmcp.Anathema) == "table" and
      gmcp.Anathema.Vitals
  H:clear("Waiting for fresh health/mana")
  if type(packet) ~= "table" then return end
  if packet.available ~= true then H:clear("Vitals unavailable"); return end
  if not pending then return end
  local class = classes[packet.class] or classes.unknown
  pending.resources = {}
  for _, key in ipairs(class.keys) do
    local value = packet[key]
    if type(value) == "table" then
      local current = integer(value.current)
      if current ~= nil then
        local definition = resources[key]
        pending.resources[#pending.resources + 1] = {
          label = definition.label, color = definition.color,
          current = current, max = integer(value.max)
        }
      end
    end
  end
  H.data, H.message = pending, "Live GMCP | " .. class.label
  H:render()
end

local ok, problem = pcall(function()
  H.panel = Geyser.Container:new({name = namespace .. "_panel", x = config.x,
    y = config.y, width = config.width, height = config.height, hidden = true})
  local background = Geyser.Label:new({name = namespace .. "_background",
    x = 0, y = 0, width = "100%", height = "100%"}, H.panel)
  background:setStyleSheet("background-color:#171d28;border:1px solid #485063;border-radius:6px;")
  clickthrough(background)
  local title = Geyser.Label:new({name = namespace .. "_title",
    x = 8, y = 4, width = 274, height = 18}, H.panel)
  title:setStyleSheet("color:#eef2fa;background:transparent;font-size:12px;font-weight:bold;")
  title:echo("Godwars: Anathema")
  clickthrough(title)
  H.status = Geyser.Label:new({name = namespace .. "_status",
    x = 8, y = 23, width = 274, height = 18}, H.panel)
  H.status:setStyleSheet("color:#adb7ca;background:transparent;font-size:10px;")
  clickthrough(H.status)
  for i = 1, 4 do
    local constraints = {name = namespace .. "_gauge" .. i, x = 8,
      y = 46 + (i - 1) * 30, width = 274, height = 24, strict = true}
    local gauge = Geyser.Gauge:new(constraints, H.panel)
    gauge:setFontSize(10)
    clickthrough(gauge)
    local readout = Geyser.Label:new({name = namespace .. "_readout" .. i,
      x = 8, y = constraints.y, width = 274, height = 24}, H.panel)
    readout:setStyleSheet("color:#e1e7f0;background:#242d3b;font-size:11px;padding-left:5px;")
    clickthrough(readout)
    H.rows[i] = {gauge = gauge, readout = readout}
  end
  local function on(event, callback)
    local id = registerAnonymousEventHandler(event, callback)
    assert(id ~= nil and id ~= false, "Cannot register " .. event)
    H.handlers[#H.handlers + 1] = id
  end
  on("sysConnectionEvent", function()
    if H.destroyed then return end
    H.session, H.protocol = true, false
    H:clear("Waiting for GMCP negotiation")
  end)
  on("sysDisconnectionEvent", function()
    if H.destroyed then return end
    H.session, H.protocol = false, false
    H:clear("Disconnected")
  end)
  on("sysProtocolDisabled", function(_, protocol)
    if H.destroyed or protocol ~= "GMCP" then return end
    H.protocol = false
    H:clear("GMCP disabled")
  end)
  on("sysProtocolEnabled", function(_, protocol)
    if H.destroyed or protocol ~= "GMCP" then return end
    H.protocol = true
    H:clear("Waiting for fresh GMCP")
  end)
  on("gmcp.Char.Vitals", standard)
  on("gmcp.Anathema.Vitals", custom)
  H.message = "Reconnect after install or reload"
  H:show()
end)
if not ok then H:destroy(); error("Anathema HUD could not load: " .. tostring(problem)) end
rawset(_G, namespace, H)

-- Dumb profiler script for PCSX-Redux. Relies on the game calling
-- certain PCSX exec slots whenever a frame begins or ends or
-- a function is entered or left. See quakepsx/src/profile.c for an example.
-- Don't forget to feed your ELF into Redux for the symbols to appear.

local bit = require("bit")

-- tree type --

local Tree = {}
Tree.__index = Tree

-- make a new tree node
function Tree:new(count, value, parent)
  local tree = setmetatable({ }, Tree)
  tree.count = count   -- number of children this node has
  tree.value = value   -- data held in this node
  tree.parent = parent -- parent node for this node
  tree.child = { }     -- array containing this node's children
  return tree
end

-- add a new child node to self
function Tree:addChild(value)
  self.count = self.count + 1
  self.child[self.count] = Tree:new(0, value, self)
  return self.child[self.count]
end

-- profiler internals --

gCallTree = nil
gCallNode = nil

gProfShowStats = false
gProfAutoPause = true

function addrToName(addr)
  if addr == 0 then
    return "This frame"
  end
  for k, v in PCSX.iterateSymbols() do
    if k == addr then
      return v
    end
  end
  return string.format("0x%08x", addr)
end

function onReset(hard)
  gCallTree = nil
  gCallNode = nil
end

function onPause(exception)
  gProfShowStats = true
end

function onRun()
  gProfShowStats = false
end

gEvReset = PCSX.Events.createEventListener("ExecutionFlow::Reset", onReset)
gEvPause = PCSX.Events.createEventListener("ExecutionFlow::Pause", onPause)
gEvRun = PCSX.Events.createEventListener("ExecutionFlow::Run", onRun)

-- Prf_EndFrame, called at the end of a frame in the game's main loop
PCSX.execSlots[252] = function()
  if gProfAutoPause then
    PCSX.pauseEmulator()
  end

  if gCallTree then
    gCallTree.value.timeTotal = 0
    for k, v in ipairs(gCallTree.child) do
      gCallTree.value.timeTotal = gCallTree.value.timeTotal + v.value.timeTotal
    end
  end
end

-- Prf_StartFrame, called at the start of a frame in the game's main loop
PCSX.execSlots[253] = function()
  -- call node for main function (or top scope or wherever you called this from)
  gCallTree = Tree:new(0, {
    addr = 0,
    timeStart = 0,
    timeTotal = 0,
    calls = 1
  })
  gCallNode = gCallTree
end

-- __cyg_profile_func_enter, called by gcc instrumentation whenever a function is entered
PCSX.execSlots[254] = function()
  local enterTime = tonumber(PCSX.getCPUCycles())
  local regs = PCSX.getRegisters().GPR.n
  local thisFunc = regs.a0
  local callSite = regs.a1

  if not gCallTree then
    return
  end

  local frame = {
    addr = thisFunc,
    timeStart = enterTime,
    timeTotal = 0,
    calls = 1
  }

  if not gCallNode then
    -- top node
    gCallNode = gCallTree
  else
    -- check if the current node already has a child for this call
    local found = nil
    for k, v in ipairs(gCallNode.child) do
      if v.value.addr == thisFunc then
        found = v
        break
      end
    end
    if found then
      -- yes: use this node and increment its call count
      found.value.timeStart = enterTime
      found.value.calls = found.value.calls + 1
      gCallNode = found
    else
      -- no: add a new child call
      gCallNode = gCallNode:addChild(frame)
    end
  end
end

-- __cyg_profile_func_exit, called by gcc instrumentation whenever a function is exited
PCSX.execSlots[255] = function()
  local exitTime = tonumber(PCSX.getCPUCycles())
  local regs = PCSX.getRegisters().GPR.n
  local thisFunc = regs.a0
  local callSite = regs.a1

  if not gCallTree then
    return
  end

  assert(gCallNode)
  assert(gCallNode.parent)
  assert(gCallNode.value.addr == thisFunc)

  gCallNode.value.timeTotal = gCallNode.value.timeTotal + (exitTime - gCallNode.value.timeStart)
  gCallNode = gCallNode.parent
end

-- visualization --

function nodeCmp(na, nb)
  return na.value.timeTotal > nb.value.timeTotal
end

function funcNode(node)
  local name = addrToName(node.value.addr)
  local text = string.format(
    "%s | calls: %d | time: %d",
    name, node.value.calls, node.value.timeTotal
  )

  if imgui.TreeNode(text) then
    table.sort(node.child, nodeCmp)
    for k, v in ipairs(node.child) do
      funcNode(v)
    end
    imgui.TreePop()
  end
end

function profilerWindow()
  if imgui.Button("Resume") then
    PCSX.resumeEmulator()
  end

  imgui.SameLine()

  local changed
  changed, gProfAutoPause = imgui.Checkbox("Pause on EndFrame", gProfAutoPause)

  if not gProfShowStats then
    return
  end

  if not gCallTree then
    return
  end

  funcNode(gCallTree)
end

function DrawImguiFrame()
  imgui.safe.Begin("Profiler", profilerWindow);
end

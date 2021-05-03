local traceback = debug.traceback
package.path = package.path .. ';' .. 'core/?.lua;core/?/init.lua'

local function _error(msg)
    trace = traceback("", 1)
    print(msg, trace)
end

local function _step()
    local dt = poti.delta()
    if poti.update then poti.update(dt) end
    if poti.draw then poti.draw() end
end

function poti._load()
    if poti.load then xpcall(poti.load, _error) end
end

function poti._step()
    xpcall(_step, _error)
end

function poti.run()
    local dt = poti.delta()
    if poti.load then poti.load() end
    while poti.running() do
        if poti.update then poti.update(dt) end
        if poti.draw then poti.draw() end
    end
    return 0
end

function poti.error()
    print("eita")
end

xpcall(function() require "main" end, _error)

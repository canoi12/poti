local traceback = debug.traceback
local path = poti.filesystem.base_path()
package.path = path .. '/?.lua;' .. path .. '/?/init.lua;' .. package.path 
local main_state = false
local function _err(msg)
    local trace = traceback('', 1)
    print(msg, trace)
    poti.error(msg, trace)
end
local last = 0
local function _mainLoop()
    if poti.update then poti.update(poti.timer.delta()) end
    if poti.draw then poti.draw() end
    poti.gui_begin()
    if poti.draw_gui then poti.draw_gui() end
    poti.gui_end()
end
local _step = _mainLoop
function poti.run()
    if poti.load then poti.load() end
    local delta = 0
    return function()
        local tick = poti.timer.tick()
        delta = (tick - last) / 1000
        last = tick
        if poti.update then poti.update(delta) end
        if poti.draw then poti.draw() end
    end
end
local function _initialize()
    if not main_state then return function() xpcall(_step, _err) end end
    local state, ret = xpcall(poti.run, _err)
    if ret then _step = ret end
    return function() xpcall(_step, _err) end
end
function poti.quit()
    return true
end
main_state = xpcall(function() require 'main' end, _err)
return _initialize
local traceback = debug.traceback
package.path = package.path .. ';core/?.lua;core/?/init.lua'

local function _err(msg, trace)
    poti.error(msg, traceback("", 1))
    print(msg, trace)
end

function poti.error(msg, trace)
    poti.update = function() end
    poti.draw = function() end
end

function poti.run()
    if poti.load then poti.load() end
    return function()
        local dt = poti.delta()
        if poti.update then poti.update(dt) end
        if poti.draw then poti.draw() end
    end
end

xpcall(function() require 'main' end, _err)
function poti.error(msg, trace)
    local trace = debug.traceback('', 1)
    poti.update = function() end
    poti.draw = function()
        poti.render.mode('fill')
        poti.render.color(255, 255, 255)
        poti.render.clear(0, 0, 0)
        poti.render.print('error', 16)
        poti.render.print(msg, 16, 16)
        poti.render.print(trace, 16, 32)
    end
end
local function _err(msg)
    local trace = debug.traceback('', 1)
    print('okok')
    print(msg, trace)
    poti.error(msg, trace)
end
return _err;
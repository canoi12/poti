local traceback = debug.traceback
local config = {
    window = {
        title = "poti " .. poti.version(),
        width = 640,
        height = 380,
        resizable = false,
        fullscreen = false,
        borderless = false,
        always_on_top = false
    },
    gl = {
        major = 3,
        minor = 0,
        es = false
    }
}

local timer = {
    delta = 0,
    last = 0
}

function poti.config(t)
end

function poti.quit()
    return true
end

local _error_step = function()
    for ev,a,b,c,d,e,f,g in poti.event.poll() do
        if ev == 'quit' then
            poti.running(false)
        elseif ev == 'key_pressed' and a == 'escape' then
            poti.running(false)
        end
    end
    poti.graphics.clear_buffer()
    poti.graphics.bind_buffer()
    if poti.update then poti.update() end
    if poti.draw then poti.draw() end
    poti.graphics.draw_buffer()
    poti.graphics.swap()
end

local _error = function(msg)
    local trace = traceback('', 1)
    print(msg, trace)
    poti.update = function() end
    poti.draw = function()
        poti.graphics.set_color(255, 255, 255)
        poti.graphics.clear(0, 0, 0)
        poti.graphics.print('error', 16)
        poti.graphics.print(msg, 16, 16)
        poti.graphics.print(trace, 16, 32)
    end
    poti.set_func('step', _error_step)
end

function poti.boot()
    timer.delta = 0
    if poti.load then poti.load(poti.args) end
    return function()
	for ev,a,b,c,d,e,f,g in poti.event.poll() do
        if ev == 'quit' then
            poti.running(not poti.quit())
        else
            if poti[ev] then poti[ev](a,b,c,d,e,f,g) end
        end
	end
	local current = poti.timer.tick()	
	timer.delta = (current - timer.last) / 1000
	timer.last = current
	if poti.update then poti.update(timer.delta) end
	poti.graphics.identity()
	poti.graphics.set_target()
	poti.graphics.set_shader()
	poti.graphics.clear_buffer()
	poti.graphics.bind_buffer()
	if poti.draw then poti.draw() end
	poti.graphics.draw_buffer()
	poti.graphics.swap()
    end
end

function _init()
    local args = poti.args
    local basepath = "./"
    if #args > 1 then
        basepath = args[2]
    end
    poti.filesystem.init(basepath)
    local path = poti.filesystem.basepath()
    package.path = path .. '?.lua;' .. path .. '?/init.lua;' .. package.path
    table.insert(
    package.searchers,
    function(libname)
        local path = poti.filesystem.basepath() .. libname:gsub("%.", "/") .. ".lua"
        return poti.filesystem.read(path);
    end
    )
    if poti.filesystem.exists('conf.lua') then
        xpcall(function() require('conf') end, _error)
    end
    poti.config(config)
    local window = poti.window.init(config)
    local ctx = poti.graphics.init(config, window)
    poti.audio.init()

    if poti.filesystem.exists('main.lua') then
        xpcall(function() require('main') end, _error)
    else
        poti.update = function() end
        poti.draw = function()
            local ww, hh = poti.graphics.size()
            poti.graphics.clear()
            poti.graphics.set_color(255, 255, 255)
            poti.graphics.print("no main.lua found", (ww/2)-64, 182)
        end
    end

    local state, _step = xpcall(poti.boot, _error)
    if state then
        if not _step then _error('poti.boot must return a function')
        else poti.set_func('step', function() xpcall(_step, _error) end) end
    end
    return ctx, window
end

function _deinit()
    poti.graphics.deinit()
    poti.window.deinit()
    poti.audio.deinit()
end

return _deinit, _init, _error

local traceback = debug.traceback
local path = poti.filesystem.basepath()
package.path = path .. '/?.lua;' .. path .. '/?/init.lua;' .. package.path 

-- config
local config = {
    window = {
	title = 'poti ' .. poti.version(),
	width = 640,
	height = 380,
	resizable = false,
	fullscreen = false
    },
    gl = {
	major = 3,
	minor = 0,
	es = false,
	z_buffer = false
    }
}
function poti.conf(t)
end

local function _conf()
    poti.conf(config)
    return config
end
-- error handling
function poti.error(msg, trace)
    poti.update = function() end
    poti.draw = function()
        poti.graphics.set_color(255, 255, 255)
        poti.graphics.clear(0, 0, 0)
        poti.graphics.print('error', 16)
        poti.graphics.print(msg, 16, 16)
        poti.graphics.print(trace, 16, 32)
    end
end
local function _err(msg)
    local trace = traceback('', 1)
    print(msg, trace)
    poti.error(msg, trace)
end

-- main loop
local delta = 0
local last = 0
local function _mainLoop()
    -- event
    for name,a,b,c,d,e,f in poti.event.poll() do
	if name == 'quit' then
	    poti.running(not poti.quit())
	elseif poti[name] then
	    poti[name](a,b,c,d,e,f)
	end
    end
    -- timer
    if poti.timer then
	local tick = poti.timer.tick()
	delta = (tick - last) / 1000
	last = tick
    end
    -- update
    if poti.update then poti.update(delta) end
    -- graphics
    --[[ poti.graphics.matrix('perspective')
     poti.graphics.identity()
    local ww, hh = poti.window.get_size()
    poti.graphics.ortho(0, ww, hh, 0, 0, 1)
    poti.graphics.matrix('modelview')
    poti.graphics.identity()

    poti.graphics.set_target()
    poti.graphics.set_shader()]]
    -- draw
    if poti.draw then poti.draw() end
    -- poti.graphics.draw()
end
local _step = _mainLoop
function poti.run()
    delta = 0
    if poti.timer then
	last = poti.timer.tick()
    end
    if poti.load then poti.load(poti.args) end
    return _mainLoop
end
local function _setup()
    local state, ret = xpcall(poti.run, _err)
    if ret then _step = ret end
    return function() xpcall(_step, _err) end
end
function poti.quit()
    return true
end
if poti.filesystem.exists('main.lua') then
    xpcall(function() require 'main' end, _err)
end
return _err, _conf, _setup


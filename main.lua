local x = 0
local time = 0

function poti.load()
end

function poti.update(dt)
    time = time + dt
    x = math.sin(time*5) * 32
end

function poti.draw()
    poti.clear(0, 0, 0)
    poti.color(255, 255, 255)
    poti.circle(64+x, 64, 12)
end
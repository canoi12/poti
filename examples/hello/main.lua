function poti.load()
    x = 0
    time = 0
end

function poti.update(dt)
    time = time + dt
    x = math.sin(time) * 32
end

function poti.draw()
    poti.graphics.clear()
    poti.graphics.set_color(255, 255, 255)
    poti.graphics.print("testanDO")
end

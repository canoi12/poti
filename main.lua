local x = 0
local time = 0
local played = false

function poti.load()
    audio = poti.Audio("som.wav", "static")
    -- audio:play()
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

function poti.key_pressed(key)
    if key == "space" then
        audio:play()
    end
end
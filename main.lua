local x = 0
local time = 0
local played = false

function poti.load()
    -- audio = poti.Audio("som.wav", "static")
    -- audio:play()
end

function poti.update(dt)
    time = time + dt
    x = math.sin(time*5) * 32
end

function poti.draw()
    poti.render.clear(0, 0, 0)
    poti.render.color(255, 255, 255)
    poti.render.circle(64+x, 64, 12)
end

function poti.draw_gui()
    if poti.gui.begin_window("Testing", {64, 64, 256, 128}) then
        if poti.gui.button("bora carai") then print("hehe") end
        poti.gui.end_window()
    end
end

function poti.key_pressed(key)
end

function poti.window_resized(wid, w, h)
    print(w, h)
end
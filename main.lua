local x = 0
local y = 0
local tex = nil
local rect
local ang = 0
local time = 0
local frame = 1
local frames = {
    {  0, 0, 96, 96 },
    { 16, 0, 16, 16 },
    { 32, 0, 16, 16 },
    { 48, 0, 16, 16 }
}

function poti.load()
    -- tex = poti.Texture("img.jpg")
    joy = poti.Gamepad(0)
    canvas = poti.Texture(160, 95, "target")
    -- audio = poti.Audio("som.wav")
    -- font = poti.Font("pixelart.ttf", 8)
    print(font)
    joy:rumble(2500, 5000, 5000)
    -- print("is gamepad", poti.is_gamepad(0))
end

function poti.update(dt)
    if poti.key_down('right') then
        x = x + (80 * dt)
    elseif poti.key_down('left') then
        x = x - (80 * dt)
    end
    ang = ang + dt*15
    time = time + dt*8
    if time > 1 then
        time = 0
        frame = frame + 1
        if frame > #frames then frame = 1 end
    end
    x = x + (joy:axis("leftx") / 32767)
    y = y + (joy:axis("lefty") / 32767)
    if joy:button("a") then
        print("A")
    elseif joy:button("b") then
        print("B")
    end
end

function poti.draw()
    poti.clear()
    poti.color(255, 255, 255)
    poti.print("testando pra ver se vai")
    poti.print(joy:name(), 0, 12)

    poti.circle(x, y, 10)
end

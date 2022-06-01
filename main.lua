local x = 0
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
    tex = poti.Texture("img.jpg")
    canvas = poti.Texture(160, 95, "target")
    -- audio = poti.Audio("som.wav")
    -- font = poti.Font("pixelart.ttf", 8)
    print(font)
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
end

function poti.draw()
    poti.clear()
    
    tex:draw()
    -- poti.target(canvas)
    -- poti.mode("fill")
    -- poti.color(255, 255, 255)
    -- -- poti.rectangle(x, 0, 32, 32)
    -- poti.rectangle(x, 0, 32, 32)
    -- -- poti.mode("line")
    -- poti.circle(x, 32, 8)

    -- local mx, my = poti.mouse_pos()
    -- mx = mx / 4
    -- my = my / 4
    -- poti.triangle(mx, my, x-16, 80, x+16, 80-16)
    -- local str = string.format("Pos: %f %f", mx, my)
    -- poti.print(str)
    -- poti.target()

    -- canvas:draw(nil, {0, 0, 640, 380})
end

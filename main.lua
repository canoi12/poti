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
local txt = ""

function poti.load()
    canvas = poti.Texture(160, 95, "target")
    img = poti.Texture('img.png')
end

function poti.update(dt)
    if poti.keyboard.down('right') then
        x = x + (80 * dt)
    elseif poti.keyboard.down('left') then
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
    poti.color(255, 255, 255)
    poti.print("testando pra ver se vai")
    poti.print(txt, 0, 12)

    poti.circle(x, y, 10)
end

function poti.text_input(wid, text)
    txt = txt .. text
end

function poti.key_pressed(key)
    if key == 'backspace' then
        txt = txt:sub(1, -2)
    elseif key == 'return' then
        txt = txt .. "\n"
    end
end
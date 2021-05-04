function poti.load()
    tex = poti.Texture("player.png")
    d = poti.Rect(0, 0, 32, 32)
    s = poti.Rect(0, 0, 16, 16)
    x = 0
end

function poti.update(dt)
    if poti.key.down('d') then x = x + 10 end
end

function poti.draw()
    tex:draw(d, s)
    d:x(x)
    tex:draw(d, s)
end

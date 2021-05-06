function poti.load()
    tex = poti.Texture("player.png")
    d = poti.Rect(0, 0, 32, 32)
    s = poti.Rect(0, 0, 16, 16)
    x = 0
end

function poti.update(dt)
    if poti.key_down('a') then x = x - 10 end
    if poti.key_down('d') then x = x + 10 end
    d:x(x)
end

function poti.draw()
    poti.clear()
    tex:draw(d, s)
    poti.rect(d:x(), d:y(), 8, 8)
end

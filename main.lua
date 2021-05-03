function poti.load()
    print("okok")
    img = poti.Texture("player.png")
    d = poti.Rect(0, 0, 64, 64)
    s = poti.Rect(0, 0, 16, 16)
end

function poti.update(dt)
end

function poti.draw()
    img:draw(d, s)
end

function poti.load()
    print("testandow")
    tex = poti.Texture("bird.png")
    x = 0
    time = 0
    shader = poti.render.default_shader()
    print(shader)
end

function poti.update(dt)
    time = time + dt
    x = math.sin(time) * 32
end

function poti.draw()
    poti.render.mode("fill")
    poti.render.clear()
    poti.render.color(255, 255, 255)
    poti.render.print("testanDO")
end
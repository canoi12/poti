local Image = require "graphics.image"
local Canvas = require "core.graphics.canvas"

function poti.load()
    cc = Canvas(160, 95)
    img = Image("player.png")
end

function poti.update(dt)
    
end

function poti.draw()
    cc:set()
    poti.clear(255)
    img:draw()
    cc:unset()

    cc:draw(0, 0, 0, 4, 4)
end

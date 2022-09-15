local x = {0, 0}
local str = ""
function poti.load()
    image = poti.graphics.load_texture("ships.png")
end

function poti.update(dt)
end

function poti.draw()
	poti.graphics.clear()
    local x, y = poti.mouse.position()
    image:draw({0, 0, 32, 32})
    image:draw({32, 0, 32, 32}, {x-16, y-16})
end

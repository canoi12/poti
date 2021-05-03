local Drawable = require "core.graphics.drawable"
local Canvas = Drawable:extend("Canvas")

function Canvas:constructor(width, height)
    local tex = poti.Texture(width, height, "target")
    Drawable.constructor(self, tex)
end

function Canvas:set()
    poti.target(self.tex)
end

function Canvas:unset()
    poti.target()
end

return Canvas

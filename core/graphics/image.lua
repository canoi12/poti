local Drawable = require "core.graphics.drawable"
local Image = Drawable:extend("Image")

function Image:constructor(path, usage)
    usage = usage or "static"
    local tex = ResourceManager:get(path) or ResourceManager:set(path, poti.Texture(path, usage))
    local tex = poti.Texture(path, usage)
    Drawable.constructor(self, tex)
end

return Image

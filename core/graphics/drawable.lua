local Class = require "core.class"
local Drawable = Class:extend("Drawable")

function Drawable:constructor(tex)
    if not tex then error("Texture cannot be NULL") end
    self.tex = tex
    local d = {0, 0, tex:width(), tex:height()}
    local s = {0, 0, tex:width(), tex:height()}

    self.dest = poti.Rect(d[1], d[2], d[3], d[4])
    self.src = poti.Rect(s[1], s[2], s[3], s[4])

    self.base = { dest = d, src = s }
    self.angle = 0
    self.origin = poti.Point(0, 0)
end

function Drawable:part(x, y, w, h)
    x = x + self.base.src[1]
    y = y + self.base.src[2]
    w = w or self.base.src[3]
    h = h or self.base.src[4]
    self.src(x, y, w, h)
end

function Drawable:draw(x, y, angle, scale_x, scale_y, origin_x, origin_y)
    --[[ scale_x = scale_x or 1
    scale_y = scale_y or 1
    angle = angle or 0

    local flip = 0
    if scale_x < 0 then flip = flip + 1 end
    if scale_y < 0 then flip = flip + 2 end

    local w = self.src:width() * scale_x
    local h = self.src:height() * scale_y

    self.dest(x, y, w, h)
    self.origin(origin_x, origin_y)
    self.tex:draw(self.dest, self.src, angle, self.origin, flip)]]
    scale_x = scale_x or 1
    scale_y = scale_y or 1
    angle = angle or 0
    local flip = 0
    if scale_x < 0  then 
        flip = flip + 1
        scale_x = scale_x * -1
    end
    if scale_y < 0  then
        flip = flip + 2
        scale_y = scale_y * -1
    end 

    local w = self.src:width() * scale_x
    local h = self.src:height() * scale_y
    self.dest(x, y, w, h)
    self.tex:draw(self.dest, self.src)
end

return Drawable

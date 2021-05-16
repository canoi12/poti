local Class = require "class"
local UI = Class:extend("UI")

function UI:constructor()
    self.commands = {}
    self.layout = {}
end

function UI:update(dt)
end

function UI:draw()
end

function UI:window(title)
    return false
end

function UI:button(name)
    return false
end

return UI

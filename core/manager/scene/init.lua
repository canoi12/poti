local Manager = require "core.manager"
local SceneManager = Manager:extend("SceneManager")
local Canvas = require "core.graphics.canvas"

function SceneManager:constructor(w, h)
    self.scene = nil
    self.canvas = Canvas(w, h)
end

function SceneManager:update(dt)
    if self.scene then self.scene:update(dt) end
end

function SceneManager:draw()
    self.canvas:set()
    poti.clear()
    if self.scene then self.scene:draw() end
    self.canvas:unset()

    self.canvas:draw(0, 0, 0, 4, 4)
end

function SceneManager:set(scene)
    self.scene = scene
end

return SceneManager

# poti
Lua game framework with a tiny C core

```main.lua
function poti.load()
  tex = poti.Texture("image.png", "static")
  canvas = poti.Texture(160, 95, "target")
  dest = {0, 0, 160*4, 95*4}
end

function poti.draw()
  poti.render.target(canvas)
  poti.render.clear()
  tex:draw()
  poti.render.target()
  
  canvas:draw(nil, dest)
end
```

The idea is to have a very minimal optimized C core (filesystem, render, input, audio, ..), and build the game engine (with asset manager, scene manager, packaging, editors, ...) on top of the poti framework using Lua lang.

```main.lua
local Image = require "graphics.image"
local Canvas = require "graphics.canvas"

function poti.load()
  tex = Image("image.png")
  canvas = Canvas(160, 95)
end

function poti.draw()
  canvas:set()
  poti.clear()
  tex:draw()
  canvas:unset()
  canvas:draw(0, 0, 0, 4, 4)
end
```

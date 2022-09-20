# poti
Lua game framework with a tiny C core

```main.lua
function poti.load()
  tex = poti.graphics.load_texture("image.png", "static")
  canvas = poti.graphics.new_texture(160, 95, "target")
  dest = {0, 0, 160*4, 95*4}
end

function poti.draw()
  poti.graphics.set_target(canvas)
  poti.graphics.clear()
  tex:draw()
  poti.graphics.set_target()
  
  canvas:draw(nil, dest)
end
```

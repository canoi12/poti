# poti
Lua game framework with a tiny C core

```main.lua
function poti.load()
  tex = poti.Texture("image.png", "static")
  canvas = poti.Texture(160, 95, "target")
  
  dest = poti.Rect(0, 0, 160*4, 95*4)
end

function poti.update(dt)
end

function poti.draw()
  poti.target(canvas)
  poti.clear()
  tex:draw()
  poti.target()
  
  canvas:draw(dest)
end
```

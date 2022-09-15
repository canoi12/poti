local circles = {}
local cpairs = {}
local vertices = 5
function poti.load()
    x = 0
	y = 0
	mag = 128
	spd = 5
    time = 0
	cx = 0
	cy = 0
	cx, cy = poti.graphics.size()
	cx = (cx / 2)
	cy = (cy / 2)
	for i=1,vertices do
		local circ = { x = 0, y = 0 }
		table.insert(circles, circ)
	end
	for i,c in ipairs(circles) do
		local pair = {c, nil}
		table.insert(cpairs, {c, circles[i+2]})
		if i+3 > #circles then break end
		table.insert(cpairs, {c, circles[i+3]})
	end
	for n,m in pairs(math) do
		-- print(n, m)
	end
	pi2 = math.pi * 2
end

function poti.update(dt)
    time = time + dt
    x = math.cos(time*spd) * mag
	y = math.sin(time*spd) * mag
	for i,c in ipairs(circles) do
		local s = pi2 * i / (#circles)
		local t = time + s
		c.x = cx + math.cos(t) * mag
		c.y = cy + math.sin(t) * mag
	end
	if poti.keyboard.is_down('a') then
		spd = spd - 0.1
	elseif poti.keyboard.is_down('d') then
		spd = spd + 0.1
	end
end

function poti.draw()
    poti.graphics.clear()
    poti.graphics.set_color(255, 255, 255)
	poti.graphics.set_draw('fill')
	poti.graphics.print('HELLO WORLD', cx-32, cy-8)
	poti.graphics.set_draw('line')
	for i,c in ipairs(circles) do
		poti.graphics.circle(c.x, c.y, 8)
	end
	local c = circles
	poti.graphics.line(c[1].x, c[1].y, c[3].x, c[3].y)
	poti.graphics.line(c[1].x, c[1].y, c[4].x, c[4].y)
	poti.graphics.line(c[2].x, c[2].y, c[4].x, c[4].y)
	poti.graphics.line(c[2].x, c[2].y, c[5].x, c[5].y)
	poti.graphics.line(c[3].x, c[3].y, c[5].x, c[5].y)
	-- poti.graphics.circle(cx + x + 32, cy + y + 8, 8)
	for i,pair in ipairs(cpairs) do
		local c1 = pair[1]
		local c2 = pair[2]
		poti.graphics.line(c1.x, c1.y, c2.x, c2.y)
	end
end

function poti.window_resized(id, w, h)
	cx = w / 2
	cy = h / 2
end

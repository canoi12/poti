local x = 0
local time = 0
local played = false

local curr_pal = {}

function set_palette(pal)
    for i,t in ipairs(pal) do
        curr_pal[i] = { t[1] / 255, t[2] / 255, t[3] / 255, t[4] / 255 }
    end
end

function poti.load(args)
    print(#args)
    print(args[2])
    print(poti.filesystem.basepath())
    shader = poti.graphics.new_shader(
        [[
            vec4 position(vec2 pos, mat4 world, mat4 modelview) {
                return world * modelview * vec4(pos.x, pos.y, 0, 1);
            }
        ]],
        [[
            uniform vec4 palette[4];
            vec4 pixel(vec4 color, vec2 uv, sampler2D tex) {
                float dist = 1000000.0;
                vec4 cc = color * texture(tex, uv);
                int index = 0;
                for (int i = 0; i < 4; i++) {
                    float rr, gg, bb;
                    rr = (cc.r - palette[i].r) * (cc.r - palette[i].r);
                    gg = (cc.g - palette[i].g) * (cc.g - palette[i].g);
                    bb = (cc.b - palette[i].b) * (cc.b - palette[i].b);
                    float d = rr + gg + bb;
                    if (d <= dist) {
                        dist = d;
                        index = i;
                    }
                }
		vec4 c = vec4(palette[index].r, palette[index].g, palette[index].b, palette[index].a);
                return vec4(c.r, c.g, c.b, cc.a);
            }
        ]]
    )
    -- target = poti.Texture(160, 95, "target")
    target = poti.graphics.new_texture(160, 95, "target")
    tex = poti.graphics.load_texture("ghost.jpg")
    -- tex = poti.Texture("magnifique.jpg")
    palette = {
        {124, 63, 88, 255},
        {235, 107, 111, 255},
        {249, 168, 117, 255},
        {255, 246, 211, 255}
    }

    spirit_pal = {
        {123, 103, 166, 255},
        {157, 139, 194, 255},
        {232, 199, 230, 255},
        {250, 248, 255, 255}
    }

    gb_pal = {
        {51, 44, 80, 255},
        {70, 135, 143, 255},
        {148, 227, 68, 255},
        {226, 243, 228, 255}
    }

    set_palette(palette)
end
local c = true

local delta = 0
function poti.update(dt)
    time = time + dt
    delta = dt
    x = math.sin(time*5) * 32
    if poti.keyboard.is_down("a") then
        c = true
    elseif poti.keyboard.is_down("d") then
        c = false
    end
end

function poti.draw()
    -- poti.graphics.mode("fill")
    poti.graphics.set_color(255, 255, 255)
    poti.graphics.set_target(target)
    local p = curr_pal[3]
    poti.graphics.clear(175, 175, 175)
    -- tex:draw()
    poti.graphics.circle(64+x, 64, 12, 16)
    -- poti.graphics.mode("line")
    poti.graphics.set_color(0, 0, 0, 255)
    poti.graphics.circle(64+x, 64, 12, 16)
    -- poti.graphics.mode("fill")
    poti.graphics.set_color(255, 255, 255, 255)
    poti.graphics.rectangle(256, 256, 512, 16)
    poti.graphics.set_color(150, 150, 150)
    poti.graphics.circle(0, 0, 32)
    poti.graphics.set_color(255, 255, 255)
    poti.graphics.print("test")
    poti.graphics.print("delta: " .. delta, 0, 16)

    --[[
    poti.graphics.color(255, 0, 0)
    poti.graphics.push_vertex(64, 64)
    poti.graphics.color(0, 255, 0)
    poti.graphics.push_vertex(32, 96)
    poti.graphics.color(255, 255, 255)
    poti.graphics.push_vertex(96, 96)
    ]]
    poti.graphics.set_target()
    if c then poti.graphics.set_shader(shader) end
    shader:send("palette", curr_pal[1], curr_pal[2], curr_pal[3], curr_pal[4])
    poti.graphics.set_color(255, 255, 255)
    target:draw(nil, {0, 0, 640, 380})
    tex:draw()
    poti.graphics.set_shader()

    -- tex:draw({0, 0, 160, 90})
    -- poti.graphics.triangle(64, 64, 32, 128, 96, 128)
end

function poti.key_pressed(key)
    if key == '1' then set_palette(palette)
    elseif key == '2' then set_palette(spirit_pal)
    elseif key == '3' then set_palette(gb_pal) end
end

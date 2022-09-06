local x = 0
local time = 0
local played = false

local curr_pal = {}

function set_palette(pal)
    for i,t in ipairs(pal) do
        curr_pal[i] = { t[1] / 255, t[2] / 255, t[3] / 255, t[4] / 255 }
    end
end

function poti.load()
    shader = poti.Shader(
        [[
            vec4 position(vec2 pos, mat4 world, mat4 modelview) {
                return world * modelview * vec4(pos.x, pos.y, 0, 1);
            }
        ]],
        [[
            uniform vec4 palette[4];
            vec4 pixel(vec4 color, vec2 uv, sampler2D tex) {
                float dist = 1000000;
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

                vec4 c = palette[index];
                return vec4(c.r, c.g, c.b, cc.a);
            }
        ]]
    )
    def = poti.render.default_shader()
    target = poti.Texture(160, 95, "target")
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

function poti.update(dt)
    time = time + dt
    x = math.sin(time*5) * 32
    if poti.keyboard.down("a") then
        c = true
    elseif poti.keyboard.down("d") then
        c = false
    end
end

function poti.draw()
    poti.render.mode("fill")
    poti.render.color(255, 255, 255)
    poti.render.target(target)
    -- poti.render.clear(0.3 * 255, 0.4 * 255, 0.4 * 255, 255)
    local p = curr_pal[3]
    -- poti.render.clear(p[1] * 255, p[2] * 255, p[3] * 255)
    poti.render.clear(175, 175, 175)
    -- tex:draw()
    poti.render.circle(64+x, 64, 12, 16)
    poti.render.mode("line")
    poti.render.color(0, 0, 0, 255)
    poti.render.circle(64+x, 64, 12, 16)
    poti.render.mode("fill")
    poti.render.color(255, 255, 255, 255)
    poti.render.rectangle(256, 256, 512, 16)
    poti.render.color(150, 150, 150)
    poti.render.circle(0, 0, 32)
    poti.render.color(255, 255, 255)
    poti.render.print("test")
    poti.render.print("delta: " .. poti.timer.delta(), 0, 16)

    poti.render.color(255, 0, 0)
    poti.render.push_vertex(64, 64)
    poti.render.color(0, 255, 0)
    poti.render.push_vertex(32, 96)
    poti.render.color(255, 255, 255)
    poti.render.push_vertex(96, 96)
    poti.render.target()
    if c then shader:set() end
    shader:send("palette", curr_pal[1], curr_pal[2], curr_pal[3], curr_pal[4])
    poti.render.color(255, 255, 255)
    target:draw(nil, {0, 0, 640, 380})
    def:set()

    -- tex:draw({0, 0, 160, 90})
    -- poti.render.triangle(64, 64, 32, 128, 96, 128)
end

function poti.key_pressed(key)
    if key == '1' then set_palette(palette)
    elseif key == '2' then set_palette(spirit_pal)
    elseif key == '3' then set_palette(gb_pal) end
end

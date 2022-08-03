local x = 0
local time = 0
local played = false

function poti.load()
    -- audio = poti.Audio("som.wav", "static")
    -- audio:play()
    shader = poti.Shader([[
        in vec2 in_Pos;
        in vec4 in_Color;
        in vec2 in_Texcoord;
        varying vec4 v_Color;
        uniform mat4 u_World;
        uniform mat4 u_Modelview;
        void main() {
            gl_Position = u_World * u_Modelview * vec4(in_Pos.x, in_Pos.y, 0.0, 1.0);
            v_Color = in_Color;
        }
    ]], [[
        out vec4 FragColor;
        varying vec4 v_Color;
        void main() {
            FragColor = vec4(1.0, 1.0, 0, 1);
        }
    ]])
    print(shader)
    def = poti.render.default_shader()
end

function poti.update(dt)
    time = time + dt
    x = math.sin(time*5) * 32
end

function poti.draw()
    poti.render.mode("fill")
    poti.render.color(255, 0, 0)
    shader:set()
    poti.render.circle(64+x, 64, 12, 16)
    poti.render.color(255, 255, 255)
    def:set()
    poti.render.rectangle(256, 256, 512, 16)
    poti.render.print("test")
    -- poti.render.triangle(64, 64, 32, 128, 96, 128)
end

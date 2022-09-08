local info = {
    attributes = {
        { "vec2", "a_Position" },
        { "vec4", "a_Color" },
        { "vec2", "a_TexCoord" }
    },
    uniforms = {
        vert = {
            { "mat4", "u_World" },
            { "mat4", "u_ModelView" }
        },
        frag = {
            { "sampler2D", "u_Texture" }
        }
    },
    varying = {
        { "vec4", "v_Color" },
        { "vec2", "v_TexCoord" }
    }
}

local version = ""

local vert_top = ""
local frag_top = ""

local position = [[
vec4 position(vec2 pos, mat4 world, mat4 modelview) {
    return world * modelview * vec4(pos.x, pos.y, 0.0, 1.0);
}
]]

local pixel = [[
vec4 pixel(vec4 color, vec2 texcoord, sampler2D tex) {
    return color * texture(tex, texcoord);
}
]]

local vert_main = [[
void main() {
    v_Color = a_Color;
    v_TexCoord = a_TexCoord;
    gl_Position = position(a_Position, u_World, u_ModelView);
}
]]

local frag_main = [[

void main() {
    o_FragColor = pixel(v_Color, v_TexCoord, u_Texture);
}
]]

local function _setup_uniforms()
    for i,u in ipairs(info.uniforms.vert) do
        vert_top = vert_top .. string.format("uniform %s %s;\n", u[1], u[2])
    end
    for i,u in ipairs(info.uniforms.frag) do
        frag_top = frag_top .. string.format("uniform %s %s;\n", u[1], u[2])
    end
end

local function _setup_attributes(glsl)
    local prefix = "attribute %s %s;"
    if glsl > 300 then
        prefix = "layout (location = %i) in %s %s;"
    elseif glsl > 120 then
        prefix = "in %s %s;"
    end

    for i,a in ipairs(info.attributes) do
        if glsl > 300 then
            vert_top = vert_top .. string.format(prefix, i-1, a[1], a[2])
        else
            vert_top = vert_top .. string.format(prefix, a[1], a[2])
        end
        vert_top = vert_top .. '\n'
    end
end

local function _setup_varying(glsl)
    local out_format = 'varying %s %s;'
    local in_format = 'varying %s %s;'
    if glsl >= 130 then
        out_format = 'out %s %s;'
        in_format = 'in %s %s;'
    end

    for i,v in ipairs(info.varying) do
        vert_top = vert_top .. string.format(out_format, v[1], v[2]) .. '\n'
        frag_top = frag_top .. string.format(in_format, v[1], v[2]) .. '\n'
    end
end

local function _setup(glsl, es)
    version = "#version " .. glsl
    if es and glsl > 100 then
        version = version .. " es\n"
    else
        version = version .. "\n"
    end

    vert_top = version
    frag_top = version
    _setup_uniforms()
    _setup_attributes(glsl)
    _setup_varying(glsl)

    if glsl < 130 then
        defines = [[
#define o_FragColor gl_FragColor
#define texture texture2D
]]
    else
        defines = "out vec4 o_FragColor;\n"
    end
    frag_top = frag_top .. defines
    return function(vert, frag)
	vert = vert or position
	frag = frag or pixel

	local vert_src = string.format("%s\n%s\n%s", vert_top, vert, vert_main)
	local frag_src = string.format("%s\n%s\n%s", frag_top, frag, frag_main)

	return frag_src, vert_src
    end
end

return _setup

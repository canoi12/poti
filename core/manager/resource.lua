local Manager = require "core.manager"
local ResourceManager = Manager:extend("ResourceManager")

function ResourceManager:constructor(resources)
    self.res_data = resources or {}
    self.data = {}
    self.loaders = {
        texture = function(res)
            return poti.Texture(res.path, res.usage)
        end,
        audio = function(res)
            return poti.Audio(res.path, res.usage)
        end
    }
    for i,res in ipairs(self.res_data) do
        self:set(res.path, self.loaders[res.type](res))
    end
end

function ResourceManager:set(name, res)
    self.data[name] = res
    return res
end

function ResourceManager:get(name)
    return self.data[name]
end

return ResourceManager

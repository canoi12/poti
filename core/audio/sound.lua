local Class = require "core.class"
local Sound = Class:extend("Sound")

function Sound:constructor(path)
	self.audio = poti.Audio(path, "static")
end

function Sound:play()
	self.audio:play()
end

function Sound:pause()
	self.audio:pause()
end

function Sound:stop()
	self.audio:stop()
end

function Sound:volume(vol)
	return self.audio:volume(vol)
end

return Sound
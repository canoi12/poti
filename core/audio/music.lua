local Class = require "core.class"
local Music = Class:extend("Music")

function Music:constructor(path)
	self.audio = poti.Audio(path, "stream")
end

function Music:play()
	self.audio:play()
end

function Music:pause()
	self.audio:pause()
end

function Music:stop()
	self.audio:stop()
end

function Music:volume(vol)
	return self.audio:volume(vol)
end

return Music
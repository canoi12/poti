local audio_bank = { [0] = {}, [1] = {} }

local function check(usage, path)
    if not audio[usage] then return false end
    return audio[usage][path]
end

local function register(usage, path, data)
    audio_bank[usage][path] = data
    audio_bank[data:id()] = data
end

return register, check, audio_bank

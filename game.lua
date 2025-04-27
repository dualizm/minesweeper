local States = require("states")
local Field = require("field")

local Game = {}

function Game.load()
  Field.load()
end

function Game.new(gameMode)
  local self = {
    startTime = 0,

    field = Field.new(gameMode)
  }

  return self
end

function Game.start()
    
end

return Game

assets = require("assets/assets")

local Game = require("game")
local States = require("states")
local Cell = require("cell")

local gameMode = States.GameMode.intermediate

function love.load()
  Game.load()

  font = love.graphics.newFont(30)
  love.graphics.setFont(font)

  game = Game.new(gameMode)

  love.graphics.setBackgroundColor(0.24, 0.24, 0.24)
  love.window.setMode(game.field.width * Cell.cellSize, game.field.height * Cell.cellSize)
end

--function love.update(dt)
--end

function love.draw()
  game.field:draw()
  local width, height = love.graphics.getDimensions()

  if game.field.gameStatus == States.GameStatus.dead then
    love.graphics.setColor(1, 0, 0)
    love.graphics.print("You died!", width/2, height/2)
    love.graphics.setColor(1, 1, 1)
  elseif game.field.gameStatus == States.GameStatus.win then
    love.graphics.setColor(0, 1, 0)
    love.graphics.print("You win!", width/2, height/2)
    love.graphics.setColor(1, 1, 1)
  end
end

--function love.mousepressed(mx, my, button)
--end

function love.mousepressed(x, y, button)
  game.field:mousepressed(x, y, button)
end

function love.keypressed(key)
  if key == "r" then
    game = Game.new(gameMode)
  end
end

-- function love.mousereleased(x, y, button)
--   if button == 1 then
--     print("Левая кнопка мыши отпущена в координатах:", x, y)
--   elseif button == 2 then
--     print("Правая кнопка мыши отпущена в координатах:", x, y)
--   end
-- end

-- authors: dualizm
-- version: 0.0.1
assets = require("assets/assets")

local Cell = require("cell")
local Field = require("field")
local ModeMinesweeper = require("mode")

local gameMode = ModeMinesweeper.Status.expert
local fieldWidth, fieldHeight = Field.sizeFromMode(gameMode)
local gameWidth = fieldWidth * Cell.cellSize
local gameHeight = fieldHeight * Cell.cellSize

local padding = 50

function love.load()
  Field.load()

  love.graphics.setBackgroundColor(0.24, 0.24, 0.24)
  love.window.setMode(gameWidth + 2 * padding, gameHeight + 2 * padding)
  love.window.setTitle("Minesweeper")

  font = love.graphics.newFont(30)
  love.graphics.setFont(font)
  textHeight = love.graphics.getFont():getHeight()
  textY = gameHeight / 2 - textHeight / 2

  field = Field.new(gameMode)
end

--function love.update(dt)
--end

function love.draw()
  love.graphics.translate(padding, padding)
  field:draw()
  love.graphics.origin()

  if field.gameStatus == Field.GameStatus.dead then
    love.graphics.setColor(1, 0, 0)
    love.graphics.printf("You died!\nPress 'r' to restart...", 0, textY, gameWidth, "center")
    love.graphics.setColor(1, 1, 1)
  elseif field.gameStatus == Field.GameStatus.win then
    love.graphics.setColor(0, 1, 0)
    love.graphics.printf("You win!\nPress 'r' to restart...", 0, textY, gameWidth, "center")
    love.graphics.setColor(1, 1, 1)
  end
end

--function love.mousepressed(mx, my, button)
--end

function love.mousepressed(x, y, button)
  field:mousepressed(x - padding, y - padding, button)
end

function love.keypressed(key)
  if key == "r" then
    field = Field.new(gameMode)
  end
end

-- function love.mousereleased(x, y, button)
--   if button == 1 then
--     print("Левая кнопка мыши отпущена в координатах:", x, y)
--   elseif button == 2 then
--     print("Правая кнопка мыши отпущена в координатах:", x, y)
--   end
-- end

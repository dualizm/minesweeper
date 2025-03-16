local ModeMinesweeper = require("mode")
local Cell = require("cell")

local Field = {}

Field.GameStatus = {
  start = 0,
  dead = 1,
  process = 2,
  win = 3,
}

function Field.sizeFromMode(mode)
  if mode == ModeMinesweeper.Status.begginer then
    return 9, 9
  elseif mode == ModeMinesweeper.Status.intermediate then
    return 16, 16
  elseif mode == ModeMinesweeper.Status.expert then
    return 30, 16
  end
end

function Field.minesCountFromMode(mode)
  if mode == ModeMinesweeper.Status.begginer then
    return 10
  elseif mode == ModeMinesweeper.Status.intermediate then
    return 40
  elseif mode == ModeMinesweeper.Status.expert then
    return 99
  end
end

function Field.initCells(width, height)
  local cells = {}
  for x = 1, width do
    cells[x] = {}
    for y = 1, height do
      cells[x][y] = Cell.new((x-1) * Cell.cellSize, (y-1) * Cell.cellSize)
    end
  end
  return cells
end

function Field.load()
  Cell.load()
end

function Field.new(mode)
  local width, height = Field.sizeFromMode(mode)
  local totalMinesCount = Field.minesCountFromMode(mode)
  local visitedToWinCount = width * height - totalMinesCount
  local visitedCells = 0

  local self = {
    gameStatus = Field.GameStatus.start,
    width = width,
    height = height,
    minesCount = totalMinesCount,
    cells = Field.initCells(width, height),
  }

  function self:initMines(mx, my)
    local minesPlaced = 0
    while minesPlaced < self.minesCount do
      local x = love.math.random(1, self.width)
      local y = love.math.random(1, self.height)

      if not self.cells[x][y].isMine 
        and not (mx == x and my == y) then
        self.cells[x][y].isMine = true
        minesPlaced = minesPlaced + 1
      end
    end
  end

  function self:walkAround(dx, dy, f)
    for x = dx-1, dx+1 do
      for y = dy-1, dy+1 do
        if self.cells[x]
          and self.cells[x][y] 
          and not (dx == x and dy == y) then
            f(self.cells[x][y], x, y)
        end
      end
    end
  end

  function self:walkField(f)
    for x = 1, self.width do
      for y = 1, self.height do
        f(self.cells[x][y], x, y)
      end
    end
  end

  function self:countMinesAround(dx, dy)
    local count = 0

    self:walkAround(dx, dy, function(cell)
      if cell.isMine == true then
        count = count + 1
      end
    end)

    return count
  end

  function self:countFlagsAround(dx, dy)
    local count = 0

    self:walkAround(dx, dy, function(cell)
      if cell:isFlag() == true then
        count = count + 1
      end
    end)

    return count
  end

  function self:addVisit(cell)
    if not cell:isOpen() then
      visitedCells = visitedCells + 1
    end
  end

  function self:visitToNumber(cell)
    self:addVisit(cell)
    cell:toNumber()
  end

  function self:visitToOpen(cell)
    self:addVisit(cell)
    cell:toOpen()
  end
  
  function self:revealAround(dx, dy)
    local revealCell = function(cell, x, y)
      if cell:isEmpty() then
        self:visitToOpen(cell)
        self:revealAround(x, y)
      elseif cell:isNumber() then
        self:visitToNumber(cell)
      end
    end

    self:walkAround(dx, dy, revealCell)
  end

  -- TODO
  function self:openAround(dx, dy)
    self:walkAround(dx, dy, function(cell, x, y)
      self:mousepressed(x, y, 1)
    end)
  end

  -- TODO
  -- Сделать когда будет разделение отжания и нажатия кнопки
  function self:highlightAround(dx, dy) end

  -- TODO не работает openAround
  function self:accord(dx, dy)
    local count = self.cells[dx][y].number
    local flagsCount = self:countFlagsAround(dx,dy)
    if count == flagsCount then
      self:openAround(dx,dy)
    -- else
    --   self:highlightAround(dx,dy)
    end
  end

  function self:showAllMines()
    self:walkField(function(cell)
      if cell.isMine
        and not cell:isDeadMine() then
        cell:toMine()
      end
    end)
  end

  function self:initNumbers()
    self:walkField(function(cell, x, y)
      if not cell.isMine then
        local minesCount = self:countMinesAround(x, y)
        if minesCount > 0 then
          cell.number = minesCount
        end
      end
    end)
  end

  function self:checkWin()
    if visitedCells == visitedToWinCount then
      self.gameStatus = Field.GameStatus.win
    end
  end
   
  -- [[ external ]] --

  function self:mousepressed(mx, my, button)
    if self.gameStatus == Field.GameStatus.start then
      self:initMines(math.floor(mx / Cell.cellSize) + 1, math.floor(my / Cell.cellSize) + 1)
      self:initNumbers()
      self.gameStatus = Field.GameStatus.process
    end

    if self.gameStatus ~= Field.GameStatus.process then
      return
    end

    local fieldX = math.floor(mx / Cell.cellSize) + 1
    local fieldY = math.floor(my / Cell.cellSize) + 1
    local cell = self.cells[fieldX][fieldY]

    if fieldX >= 1 and fieldX <= self.width 
      and fieldY >= 1 and fieldY <= self.height then
        if button == 1 then -- LKM
          if cell:isFlag() then
            return
          end

          if cell.isMine then
            cell:toDeadMine()
            self.gameStatus = Field.GameStatus.dead
            self:showAllMines()
          elseif cell:isNumber() then
            self:visitToNumber(cell)
            --self:accord(fieldX, fieldY)
          elseif cell:isClose() then
            self:visitToOpen(cell)
            self:revealAround(fieldX, fieldY)
          end

          self:checkWin()
        elseif button == 2 then -- RKM
          -- кликаем на закрытую клетку
          if cell:isClose() then
            -- Клетка закрыта и есть флажки
            if self.minesCount > 0 then
              cell:toFlag()
              self.minesCount = self.minesCount - 1
            end
          elseif cell:isFlag() then
            -- Снимаем флаг
            cell:toClose()
              self.minesCount = self.minesCount + 1
          end
        end
    end
  end

  function self:draw()
    self:walkField(function(cell)
      cell:draw()
    end)
  end

  return self
end

return Field

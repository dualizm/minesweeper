local States = require("states")
local Cell = require("cell")

local Field = {}

function Field.sizeFromMode(mode)
  if mode == States.GameMode.begginer then
    return 9, 9
  elseif mode == States.GameMode.intermediate then
    return 16, 16
  elseif mode == States.GameMode.expert then
    return 30, 16
  end
end

function Field.minesCountFromMode(mode)
  if mode == States.GameMode.begginer then
    return 10
  elseif mode == States.GameMode.intermediate then
    return 40
  elseif mode == States.GameMode.expert then
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
    gameStatus = States.GameStatus.start,
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

  function self:walkAround(dx, dy)
    local x, y = dx-1, dy-2

    return function()
      while true do
        if y < dy+1 then
          y = y + 1
        else
          x = x + 1
          y = dy-1
        end

        if x > dx+1 then
          return nil
        end

        if self.cells[x]
          and self.cells[x][y]
          and not (dx == x and dy == y) then
            return self.cells[x][y], x, y
        end
      end
    end
  end

  function self:walkField()
    local x, y = 1, 0

    return function()
      if y < self.height then
        y = y + 1
      else
        x = x + 1
        y = 1
      end

      if x > self.width then
        return nil
      end

      return self.cells[x][y], x, y
    end
  end

  function self:countMinesAround(dx, dy)
    local count = 0

    for cell in self:walkAround(dx, dy) do
      if cell.isMine == true then
        count = count + 1
      end
    end

    return count
  end

  function self:countFlagsAround(dx, dy)
    local count = 0

    for cell in self:walkAround(dx, dy) do
      if cell:isFlag() == true then
        count = count + 1
      end
    end

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
    for cell, x, y in self:walkAround(dx, dy) do
      if cell:isEmpty() then
        self:visitToOpen(cell)
        self:revealAround(x, y)
      elseif cell:isNumber() then
        self:visitToNumber(cell)
      end
    end
  end

  function self:isDeadAccord(dx, dy)
    for cell in self:walkAround(dx, dy) do
      if cell.isMine and not cell:isFlag() then
        return true
      end
    end

    return false
  end

  function self:openAround(dx, dy)
    local isDead = self:isDeadAccord(dx, dy)

    if isDead then
      self.gameStatus = States.GameStatus.dead
      self:showAllMines()
    end

    for cell, x, y in self:walkAround(dx, dy) do
      if cell:isFlag() then
        if isDead and not cell.isMine then
          cell:toDeadFlag()
        end
        goto continue
      end

      if isDead and cell.isMine then
        cell:toDeadMine()
        goto continue
      end

      if not isDead then
        if cell:isNumber() then
          self:visitToNumber(cell)
        elseif cell:isClose() then
          self:visitToOpen(cell)
          self:revealAround(x, y)
        end

        self:checkWin()
      end

      ::continue::
    end
  end

  -- TODO
  -- Сделать когда будет разделение отжания и нажатия кнопки
  function self:highlightAround(dx, dy) end

  function self:accord(dx, dy)
    local count = self.cells[dx][dy].number
    local flagsCount = self:countFlagsAround(dx,dy)
    if count == flagsCount then
      self:openAround(dx,dy)
    -- else
    --   self:highlightAround(dx,dy)
    end
  end

  function self:showAllMines()
    for cell in self:walkField() do
      if cell.isMine
        and not cell:isFlag()
        and not cell:isDeadMine() then
          cell:toMine()
      end
    end
  end

  function self:initNumbers()
    for cell, x, y in self:walkField() do
      if not cell.isMine then
        local minesCount = self:countMinesAround(x, y)
        if minesCount > 0 then
          cell.number = minesCount
        end
      end
    end
  end

  function self:checkWin()
    if visitedCells == visitedToWinCount then
      self.gameStatus = States.GameStatus.win

      for cell in self:walkField() do
        if cell.isMine then
          cell:toFlag()
        end
      end
    end
  end

  function self:leftClickCell(cell, x, y)
    if cell:isFlag() then
      return
    end

    if cell.isMine then
      cell:toDeadMine()
      self.gameStatus = States.GameStatus.dead
      self:showAllMines()
    elseif cell:isNumber() then
      self:visitToNumber(cell)
      self:accord(x, y)
    elseif cell:isClose() then
      self:visitToOpen(cell)
      self:revealAround(x, y)
    end

    self:checkWin()
  end

  function self:rightClickCell(cell, x, y)
    if cell:isClose() then
      if self.minesCount > 0 then
        cell:toFlag()
        self.minesCount = self.minesCount - 1
      end
    elseif cell:isFlag() then
      cell:toClose()
        self.minesCount = self.minesCount + 1
    end
  end

  function self:mousepressed(mx, my, button)
    if self.gameStatus == States.GameStatus.start then
      self:initMines(math.floor(mx / Cell.cellSize) + 1, 
        math.floor(my / Cell.cellSize) + 1)
      self:initNumbers()
      self.gameStatus = States.GameStatus.process
    end

    if self.gameStatus ~= States.GameStatus.process then
      return
    end

    local fieldX = math.floor(mx / Cell.cellSize) + 1
    local fieldY = math.floor(my / Cell.cellSize) + 1
    local cell = self.cells[fieldX][fieldY]

    if fieldX >= 1 and fieldX <= self.width 
      and fieldY >= 1 and fieldY <= self.height then
        if button == 1 then
          self:leftClickCell(cell, fieldX, fieldY)
        elseif button == 2 then
          self:rightClickCell(cell, fieldX, fieldY)
        end
    end
  end

  function self:draw()
    for cell in self:walkField() do
      cell:draw()
    end
  end

  return self
end

return Field

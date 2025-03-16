local Cell = {}

Cell.BackgroundState = {
  open = 1, -- Клетка открыта
  close = 2, -- Клетка закрыта
  dead = 3, -- Клетка содержит взорванную мину
}

Cell.ForegroundState = {
  none = 0,
  first = 1,
  second = 2,
  third = 3,
  fourth = 4,
  fifth = 5,
  sixth = 6,
  seventh = 7,
  eighth = 8,
  mine = 9,
  flag = 10,
}

Cell.cellSize = 24

local spriteSheetBack, spriteSheetFront
local quadsBack, quadsFront

function Cell.load()
  -- Загрузка спрайтов заднего плана
  spriteSheetBack = assets.graphics.cellSheetBack
  local sheetBackWidth, sheetBackHeight = spriteSheetBack:getDimensions()

  quadsBack = {}
  for y = 0, sheetBackHeight - Cell.cellSize, Cell.cellSize do
    for x = 0, sheetBackWidth - Cell.cellSize, Cell.cellSize do
      local quad = love.graphics.newQuad(x, y, Cell.cellSize, Cell.cellSize, sheetBackWidth, sheetBackHeight)
      table.insert(quadsBack, quad)
    end
  end

  -- Загрузка спрайтов переднего плана
  spriteSheetFront = assets.graphics.cellSheetFront
  local sheetFrontWidth, sheetFrontHeight = spriteSheetFront:getDimensions()

  quadsFront = {}
  for y = 0, sheetFrontHeight - Cell.cellSize, Cell.cellSize do
    for x = 0, sheetFrontWidth - Cell.cellSize, Cell.cellSize do
      local quad = love.graphics.newQuad(x, y, Cell.cellSize, Cell.cellSize, sheetFrontWidth, sheetFrontHeight)
      table.insert(quadsFront, quad)
    end
  end
end

function Cell.new(x, y)

  local self = {
    x = x,
    y = y,
    currentBackground = Cell.BackgroundState.close,
    currentForeground = Cell.ForegroundState.none,
    isMine = false,
    number = Cell.ForegroundState.none,
  }

  function self:draw()
    love.graphics.draw(spriteSheetBack, quadsBack[self.currentBackground], self.x, self.y)

    if self.currentForeground ~= Cell.ForegroundState.none then
      love.graphics.draw(spriteSheetFront, quadsFront[self.currentForeground], self.x, self.y)
    end
  end

  -- state functions
  function self:toDeadMine()
    self.currentBackground = Cell.BackgroundState.dead
    self.currentForeground = Cell.ForegroundState.mine
  end

  function self:toNumber()
    self.currentBackground = Cell.BackgroundState.open
    self.currentForeground = self.number
  end

  function self:toOpen()
    self.currentBackground = Cell.BackgroundState.open
    self.currentForeground = Cell.ForegroundState.none
  end

  function self:toMine()
    self.currentBackground = Cell.BackgroundState.close
    self.currentForeground = Cell.ForegroundState.mine
  end

  function self:toFlag()
    self.currentBackground = Cell.BackgroundState.close
    self.currentForeground = Cell.ForegroundState.flag
  end

  function self:toClose()
    self.currentBackground = Cell.BackgroundState.close
    self.currentForeground = Cell.ForegroundState.none
  end

  -- predicate functions
  function self:isNumber()
    return self.number ~= Cell.ForegroundState.none
  end

  function self:isOpen()
    return self.currentBackground ~= Cell.BackgroundState.close
  end

  function self:isFlag()
    return self.currentBackground == Cell.BackgroundState.close
      and self.currentForeground == Cell.ForegroundState.flag
  end

  function self:isClose()
    return self.currentBackground == Cell.BackgroundState.close
      and self.currentForeground == Cell.ForegroundState.none
  end

  function self:isDeadMine()
    return self.currentBackground == Cell.BackgroundState.dead
      and self.currentForeground == Cell.ForegroundState.mine
  end

  function self:isEmpty()
    return self.currentBackground == Cell.BackgroundState.close
      and self.currentForeground == Cell.ForegroundState.none
      and self.isMine == false 
      and self.number == Cell.ForegroundState.none
  end

  --function self:isMine()
  --  return self.isMine
  --end

  return self
end

return Cell

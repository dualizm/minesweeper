local Cell = {}

Cell.BackgroundState = {
  open = 1, -- Клетка открыта
  close = 2, -- Клетка закрыта
  dead = 3, -- Клетка содержит ошибку
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
  local loadQuads = function(height, width)
    local res = {}
    for y = 0, height - Cell.cellSize, Cell.cellSize do
      for x = 0, width - Cell.cellSize, Cell.cellSize do
        local quad = love.graphics.newQuad(x, y, Cell.cellSize, Cell.cellSize, width, height)
        table.insert(res, quad)
      end
    end
    return res
  end

  -- Задний спрайт
  spriteSheetBack = assets.graphics.cellSheetBack
  local sheetBackWidth, sheetBackHeight = spriteSheetBack:getDimensions()
  quadsBack = loadQuads(sheetBackHeight, sheetBackWidth)

  -- Передний спрайт
  spriteSheetFront = assets.graphics.cellSheetFront
  local sheetFrontWidth, sheetFrontHeight = spriteSheetFront:getDimensions()
  quadsFront = loadQuads(sheetFrontHeight, sheetFrontWidth)
end

function Cell.new(x, y)
  local currentBackground = Cell.BackgroundState.close
  local currentForeground = Cell.ForegroundState.none

  local self = {
    x = x,
    y = y,
    isMine = false,
    number = Cell.ForegroundState.none,
  }

  function self:draw()
    love.graphics.draw(spriteSheetBack, quadsBack[currentBackground], self.x, self.y)

    if currentForeground ~= Cell.ForegroundState.none then
      love.graphics.draw(spriteSheetFront, quadsFront[currentForeground], self.x, self.y)
    end
  end

  -- state functions
  function self:toDeadMine()
    currentBackground = Cell.BackgroundState.dead
    currentForeground = Cell.ForegroundState.mine
  end

  function self:toDeadFlag()
    currentBackground = Cell.BackgroundState.dead
    currentForeground = Cell.ForegroundState.flag
  end

  function self:toNumber()
    currentBackground = Cell.BackgroundState.open
    currentForeground = self.number
  end

  function self:toOpen()
    currentBackground = Cell.BackgroundState.open
    currentForeground = Cell.ForegroundState.none
  end

  function self:toMine()
    currentBackground = Cell.BackgroundState.close
    currentForeground = Cell.ForegroundState.mine
  end

  function self:toFlag()
    currentBackground = Cell.BackgroundState.close
    currentForeground = Cell.ForegroundState.flag
  end

  function self:toClose()
    currentBackground = Cell.BackgroundState.close
    currentForeground = Cell.ForegroundState.none
  end

  -- predicate functions
  function self:isNumber()
    return self.number ~= Cell.ForegroundState.none
  end

  function self:isOpen()
    return currentBackground ~= Cell.BackgroundState.close
  end

  function self:isFlag()
    return currentBackground == Cell.BackgroundState.close
      and currentForeground == Cell.ForegroundState.flag
  end

  function self:isClose()
    return currentBackground == Cell.BackgroundState.close
      and currentForeground == Cell.ForegroundState.none
  end

  function self:isDeadMine()
    return currentBackground == Cell.BackgroundState.dead
      and currentForeground == Cell.ForegroundState.mine
  end

  function self:isEmpty()
    return currentBackground == Cell.BackgroundState.close
      and currentForeground == Cell.ForegroundState.none
      and self.isMine == false 
      and self.number == Cell.ForegroundState.none
  end

  return self
end

return Cell

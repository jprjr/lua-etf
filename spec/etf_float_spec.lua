require('busted.runner')()

local etf = require'etf'

describe('float', function()
  it('is a function', function()
    assert.is_function(etf.float)
  end)

  it('initializes with nothing', function()
    local f = etf.float()
    assert.are.same(f.float,0.0)
  end)

  it('initializes with numbers', function()
    local f = etf.float(1.0)
    assert.are.same(f.float,1.0)
    f = etf.float(1.5)
    assert.are.same(f.float,1.5)
  end)

  it('initializes with strings', function()
    local f = etf.float('1.0')
    assert.are.same(f.float,1.0)
    f = etf.float('1.5')
    assert.are.same(f.float,1.5)
  end)

  it('supports addition', function()
    local a = etf.float('1.0')
    local b = etf.float('3.5')
    local c = a + b
    assert.are.same(c.float,4.5)
  end)

  it('supports subtraction', function()
    local a = etf.float('1.0')
    local b = etf.float('3.5')
    local c = a - b
    assert.are.same(c.float,-2.5)
  end)

  it('supports multiplication', function()
    local a = etf.float('2.0')
    local b = etf.float('3.5')
    local c = a * b
    assert.are.same(c.float,7.0)
  end)

  it('supports division', function()
    local a = etf.float('7.5')
    local b = etf.float('3.0')
    local c = a / b
    assert.are.same(c.float,2.5)
  end)

  it('supports modulo', function()
    local a = etf.float('7.5')
    local b = etf.float('3.0')
    local c = a % b
    assert.are.same(c.float,1.5)
  end)

  it('supports unary minus', function()
    local a = etf.float('7.5')
    local b = -a
    assert.are.same(b.float,-7.5)
  end)

  it('supports tostring', function()
    local a = etf.float('7.5')
    assert.are.same(tostring(a),'7.5')
  end)

  it('supports concat', function()
    local a = etf.float('3.5')
    local b = etf.float('1.5')
    assert.are.same(a .. b,'3.51.5')
  end)

end)

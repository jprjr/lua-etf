require('busted.runner')()

local etf = require'etf'

local function load_code(str)
  local sent = false
  return pcall(load(function()
    if sent then
      return nil
    end
    sent = true
    return str
  end))
end

-- test if we're on a Lua that supports native bitwise operations (5.3+)
local _, lshift, rshift, lt
_, lshift = load_code([[ return function(a,b) return a << b end ]])
lshift = type(lshift) == 'function'
_, rshift = load_code([[ return function(a,b) return a >> b end ]])
rshift = type(rshift) == 'function'

-- test if we're on a Lua that allows comparing values with an __lt
-- metamethod with integers directly
local nonuserdata_lt = false
do
  local mt = {
    __lt = function()
      return true
    end
  }
  local val = setmetatable({},mt)
  local ok = pcall(function()
    return val < 1
  end)
  if ok then
    nonuserdata_lt = true
  end
end

describe('integer', function()
  it('defaults to 0', function()
    local b = etf.integer()
    assert.are.same(tostring(b),'0')
  end)

  it('initializes with numbers', function()
    local b = etf.integer(12345)
    assert.are.same(tostring(b),'12345')
  end)

  it('initializes with strings', function()
    local b = etf.integer('54321')
    assert.are.same(tostring(b),'54321')
  end)

  it('supports addition with numbers', function()
    local b = etf.integer()
    b = b + 1
    assert.are.same(tostring(b),'1')
  end)

  it('supports addition with strings', function()
    local b = etf.integer()
    b = b + '1'
    assert.are.same(tostring(b),'1')
  end)

  if lshift then
    it('supports lshift', function()
      local _, f = load_code([[ return function(etf)
        local a = etf.integer(1)
        return a << 8
      end ]])
      local res = f(etf)
      assert.are.same(tostring(res),'256')
    end)
  end

  if rshift then
    it('supports rshift', function()
      local _, f = load_code([[ return function(etf)
        local a = etf.integer(256)
        return a >> 8
      end ]])
      local res = f(etf)
      assert.are.same(tostring(res),'1')
    end)
  end

  it('supports addition with integers', function()
    local a = etf.integer(1)
    local b = etf.integer(2)
    b = a + b
    assert.are.same(tostring(b),'3')
  end)

  it('supports subtraction with numbers', function()
    local b = etf.integer()
    b = b - 1
    assert.are.same(tostring(b),'-1')
  end)

  it('supports subtraction with strings', function()
    local b = etf.integer()
    b = b - '1'
    assert.are.same(tostring(b),'-1')
  end)

  it('supports subtraction with integers', function()
    local a = etf.integer(1)
    local b = etf.integer()
    b = b - a
    assert.are.same(tostring(b),'-1')
  end)

  it('supports unary minus', function()
    local b = etf.integer(1)
    b = -b
    assert.are.same(tostring(b),'-1')
  end)

  it('supports multiplication with numbers', function()
    local b = etf.integer(2)
    b = b * 3
    assert.are.same(tostring(b),'6')
  end)

  it('supports multiplication with strings', function()
    local b = etf.integer(2)
    b = b * '3'
    assert.are.same(tostring(b),'6')
  end)

  it('supports multiplication with integers', function()
    local a = etf.integer(2)
    local b = etf.integer(3)
    b = a * b
    assert.are.same(tostring(b),'6')
  end)

  it('supports division with numbers', function()
    local b = etf.integer(6)
    b = b / 3
    assert.are.same(tostring(b),'2')
  end)

  it('supports division with strings', function()
    local b = etf.integer(6)
    b = b / '3'
    assert.are.same(tostring(b),'2')
  end)

  it('supports division with integers', function()
    local a = etf.integer(6)
    local b = etf.integer(3)
    b = a / b
    assert.are.same(tostring(b),'2')
  end)

  it('supports equality with integers', function()
    local a = etf.integer(2)
    local b = etf.integer(2)
    assert.is_true(a == b)
  end)

  it('supports inequality with integers', function()
    local a = etf.integer(2)
    local b = etf.integer(3)
    assert.is_false(a == b)
  end)

  it('supports correct __lt with integers', function()
    local a = etf.integer(2)
    local b = etf.integer(3)
    assert.is_true(a < b)
  end)

  it('supports incorrect __lt with integers', function()
    local a = etf.integer(3)
    local b = etf.integer(2)
    assert.is_false(a < b)
  end)

  it('supports correct __le with integers', function()
    local a = etf.integer(2)
    local b = etf.integer(3)
    assert.is_true(a <= b)
    assert.is_true(etf.integer(3) <= b)
  end)

  it('supports incorrect __lt with integers', function()
    local a = etf.integer(3)
    local b = etf.integer(2)
    assert.is_false(a <= b)
    assert.is_true(etf.integer(2) <= b)
  end)

  if nonuserdata_lt then
    it('supports correct __lt with numbers', function()
      local a = etf.integer(2)
      local b = 3
      assert.is_true(a < b)
    end)

    it('supports incorrect __lt with numbers', function()
      local a = etf.integer(3)
      local b = 2
      assert.is_false(a < b)
    end)

    it('supports correct __le with numbers', function()
      local a = etf.integer(2)
      local b = 3
      assert.is_true(a <= b)
      assert.is_true(etf.integer(3) <= b)
    end)

    it('supports incorrect __lt with numbers', function()
      local a = etf.integer(3)
      local b = 2
      assert.is_false(a <= b)
      assert.is_true(etf.integer(2) <= b)
    end)

    it('supports correct __lt with strings', function()
      local a = etf.integer(2)
      local b = '3'
      assert.is_true(a < b)
    end)

    it('supports incorrect __lt with strings', function()
      local a = etf.integer(3)
      local b = '2'
      assert.is_false(a < b)
    end)

    it('supports correct __le with strings', function()
      local a = etf.integer(2)
      local b = '3'
      assert.is_true(a <= b)
      assert.is_true(etf.integer(3) <= b)
    end)

    it('supports incorrect __lt with strings', function()
      local a = etf.integer(3)
      local b = '2'
      assert.is_false(a <= b)
      assert.is_true(etf.integer(2) <= b)
    end)

    it('supports correct __lt with numbers in reverse order', function()
      local a = 2
      local b = etf.integer(3)
      assert.is_true(a < b)
    end)

    it('supports incorrect __lt with numbers in reverse order', function()
      local a = 3
      local b = etf.integer(2)
      assert.is_false(a < b)
    end)

    it('supports correct __le with numbers in reverse order', function()
      local a = 2
      local b = etf.integer(3)
      assert.is_true(a <= b)
      assert.is_true(etf.integer(3) <= b)
    end)

    it('supports incorrect __lt with numbers in reverse order', function()
      local a = 3
      local b = etf.integer(2)
      assert.is_false(a <= b)
      assert.is_true(etf.integer(2) <= b)
    end)

    it('supports correct __lt with strings in reverse order', function()
      local a = '2'
      local b = etf.integer(3)
      assert.is_true(a < b)
    end)

    it('supports incorrect __lt with strings in reverse order', function()
      local a = '3'
      local b = etf.integer(2)
      assert.is_false(a < b)
    end)

    it('supports correct __le with strings in reverse order', function()
      local a = '2'
      local b = etf.integer(3)
      assert.is_true(a <= b)
      assert.is_true(etf.integer(3) <= b)
    end)

    it('supports incorrect __lt with strings in reverse order', function()
      local a = '3'
      local b = etf.integer(2)
      assert.is_false(a <= b)
      assert.is_true(etf.integer(2) <= b)
    end)
  end


end)

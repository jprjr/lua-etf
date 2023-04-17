require('busted.runner')()

local etf = require'etf'

describe('atom', function()
  it('is a function', function()
    assert.is_function(etf.atom)
  end)

  it('allows creating atoms', function()
    local a = etf.atom('a')
    assert.are.same({
      atom = 'a',
      utf8 = true,
    },a)
    local mt = debug.getmetatable(a)
    assert.are.same(etf.atom_mt,mt)
  end)

  it('restricts atom length', function()
    assert.has_error(function()
      etf.atom(string.rep('a',tonumber('0x10000')))
    end)
  end)

  it('automatically calls tostring', function()
    local a = etf.atom(5)
    assert.are.same({
      atom = '5',
      utf8 = true,
    },a)
    local mt = debug.getmetatable(a)
    assert.are.same(etf.atom_mt,mt)
  end)

  it('supports the tostring operator', function()
    local a = etf.atom('true')
    assert.are.same(tostring(a),'true')
  end)

  it('supports the equality operator', function()
    local a = etf.atom('true')
    local b = etf.atom('true')
    local c = etf.atom('false')
    assert.is_true(a == b)
    assert.is_false(a == c)
  end)

end)


require('busted.runner')()

local etf = require'etf'

describe('etf.string', function()
  it('is a function', function()
    assert.is_function(etf.string)
  end)

  it('returns a table for acceptable strings', function()
    local t = etf.string('hello')
    assert.is_table(t)
    assert.is_same(debug.getmetatable(t),etf.string_mt)
  end)

  it('calls tostring automatically', function()
    local t = etf.string(5)
    assert.is_table(t)
    assert.are.same(t.string,'5')
  end)

  it('throws an error if a string is too long', function()
    local s = string.rep('a',tonumber('0x10000'))
    assert.has_error(function()
      etf.string(s)
    end)
  end)

  it('throws an error if tostring fails', function()
    assert.has_error(function()
      etf.string(setmetatable({}, {
        __tostring = function()
          error('no tostring available')
        end,
      }))
    end)
  end)
end)

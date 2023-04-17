require('busted.runner')()

local etf = require'etf'

describe('etf.binary', function()
  it('is a function', function()
    assert.is_function(etf.binary)
  end)

  it('returns a table for acceptable strings', function()
    local t = etf.binary('hello')
    assert.is_table(t)
    assert.is_same(debug.getmetatable(t),etf.binary_mt)
  end)

  it('calls tostring automatically', function()
    local t = etf.binary(5)
    assert.is_table(t)
    assert.are.same(t.binary,'5')
  end)

  it('throws an error if tostring fails', function()
    assert.has_error(function()
      etf.binary(setmetatable({}, {
        __tostring = function()
          error('no tostring available')
        end,
      }))
    end)
  end)
end)

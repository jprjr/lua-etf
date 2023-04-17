require('busted.runner')()

local etf = require'etf'

describe('etf', function()
  it('has version info', function()
    assert.is_number(etf._VERSION_MAJOR)
    assert.is_number(etf._VERSION_MINOR)
    assert.is_number(etf._VERSION_PATCH)
    assert.is_string(etf._VERSION)
  end)
end)


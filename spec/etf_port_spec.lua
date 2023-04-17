require('busted.runner')()

local etf = require'etf'

describe('port', function()
  it('is a function', function()
    assert.is_function(etf.port)
  end)

  it('allows creating ports with integer values', function()
    local p = etf.port({
      node = 'nonode@nohost',
      id = 1,
      creation = 2,
    })
    local mt = debug.getmetatable(p)
    assert.are.same(etf.port_mt,mt)
  end)

  it('allows creating ports with integer values', function()
    local p = etf.port({
      node = 'nonode@nohost',
      id = etf.integer(1),
      creation = etf.integer(2),
    })
    local mt = debug.getmetatable(p)
    assert.are.same(etf.port_mt,mt)
  end)

  it('allows creating ports with atom nodes', function()
    local p = etf.port({
      node = etf.atom('nonode@nohost'),
      id = etf.integer(1),
      creation = etf.integer(2),
    })
    local mt = debug.getmetatable(p)
    assert.are.same(etf.port_mt,mt)
  end)

  it('allows creating ports with to-stringable things', function()
    local p = etf.port({
      node = 5,
      id = etf.integer(1),
      creation = etf.integer(2),
    })

    assert.are.same({
      node = '5',
      id = etf.integer(1),
      creation = etf.integer(2),
    },p)
    local mt = debug.getmetatable(p)
    assert.are.same(etf.port_mt,mt)
  end)

  it('throws errors on invalid inputs', function()
    assert.has_error(function()
      etf.port()
    end)
    assert.has_error(function()
      etf.port({})
    end)
    assert.has_error(function()
      -- missing id, creation
      etf.port({ node = 'nonode@nohost' })
    end)
    assert.has_error(function()
      -- missing node, creation
      etf.port({ id = 1 })
    end)

    assert.has_error(function()
      -- missing node, id
      etf.port({ creation = 2 })
    end)

    assert.has_error(function()
      -- creation is not an integer
      etf.port({ node = 'nonode@nohost', id = 1, creation = true })
    end)

  end)
end)

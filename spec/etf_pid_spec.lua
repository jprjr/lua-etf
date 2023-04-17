require('busted.runner')()

local etf = require'etf'

describe('pid', function()
  it('is a function', function()
    assert.is_function(etf.pid)
  end)

  it('allows creating pids with integer values', function()
    local p = etf.pid({
      node = 'nonode@nohost',
      id = 1,
      creation = 2,
      serial = 3,
    })
    local mt = debug.getmetatable(p)
    assert.are.same(etf.pid_mt,mt)
  end)

  it('allows creating pids with integer values', function()
    local p = etf.pid({
      node = 'nonode@nohost',
      id = etf.integer(1),
      creation = etf.integer(2),
      serial = etf.integer(3),
    })
    local mt = debug.getmetatable(p)
    assert.are.same(etf.pid_mt,mt)
  end)

  it('allows creating pids with atom nodes', function()
    local p = etf.pid({
      node = etf.atom('nonode@nohost'),
      id = etf.integer(1),
      creation = etf.integer(2),
      serial = etf.integer(3),
    })
    local mt = debug.getmetatable(p)
    assert.are.same(etf.pid_mt,mt)
  end)

  it('allows creating pids with to-stringable things', function()
    local p = etf.pid({
      node = 5,
      id = etf.integer(1),
      creation = etf.integer(2),
      serial = etf.integer(3),
    })

    assert.are.same({
      node = '5',
      id = etf.integer(1),
      creation = etf.integer(2),
      serial = etf.integer(3),
    },p)
    local mt = debug.getmetatable(p)
    assert.are.same(etf.pid_mt,mt)
  end)

  it('throws errors on invalid inputs', function()
    assert.has_error(function()
      etf.pid()
    end)
    assert.has_error(function()
      etf.pid({})
    end)
    assert.has_error(function()
      -- missing id, creation, serial
      etf.pid({ node = 'nonode@nohost' })
    end)
    assert.has_error(function()
      -- missing node, creation, serial,
      etf.pid({ id = 1 })
    end)

    assert.has_error(function()
      -- missing node, id, serial
      etf.pid({ creation = 2 })
    end)

    assert.has_error(function()
      -- missing node, id, creation
      etf.pid({ serial = 3 })
    end)

    assert.has_error(function()
      -- id is not an integer
      etf.pid({ node = 'nonode@nohost', id = true, creation = 2, serial = 3 })
    end)

    assert.has_error(function()
      -- creation is not an integer
      etf.pid({ node = 'nonode@nohost', id = 1, creation = true, serial = 3 })
    end)

    assert.has_error(function()
      -- serial is not an integer
      etf.pid({ node = 'nonode@nohost', id = 1, creation = 2, serial = true })
    end)

  end)
end)

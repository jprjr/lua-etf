require('busted.runner')()

local etf = require'etf'

describe('reference', function()
  it('is a function', function()
    assert.is_function(etf.reference)
  end)

  it('allows creating references with integer values', function()
    local p = etf.reference({
      node = 'nonode@nohost',
      id =  { 1 },
      creation = 2,
    })
    local mt = debug.getmetatable(p)
    assert.are.same(etf.reference_mt,mt)
  end)

  it('allows creating references with integer values', function()
    local p = etf.reference({
      node = 'nonode@nohost',
      id = { etf.integer(1) },
      creation = etf.integer(2),
    })
    local mt = debug.getmetatable(p)
    assert.are.same(etf.reference_mt,mt)
  end)

  it('allows creating references with atom nodes', function()
    local p = etf.reference({
      node = etf.atom('nonode@nohost'),
      id = { etf.integer(1) },
      creation = etf.integer(2),
    })
    local mt = debug.getmetatable(p)
    assert.are.same(etf.reference_mt,mt)
  end)

  it('allows creating references with to-stringable things', function()
    local p = etf.reference({
      node = 5,
      id = { etf.integer(1) },
      creation = etf.integer(2),
    })

    assert.are.same({
      node = '5',
      id = { etf.integer(1) },
      creation = etf.integer(2),
    },p)
    local mt = debug.getmetatable(p)
    assert.are.same(etf.reference_mt,mt)
  end)

  it('throws errors on invalid inputs', function()
    assert.has_error(function()
      etf.reference()
    end)
    assert.has_error(function()
      etf.reference({})
    end)
    assert.has_error(function()
      -- missing id, creation
      etf.reference({ node = 'nonode@nohost' })
    end)

    assert.has_error(function()
      -- missing node, creation
      etf.reference({ id = { 1 } })
    end)

    assert.has_error(function()
      -- missing node, id
      etf.reference({ creation = 2 })
    end)

    assert.has_error(function()
      -- id is not a table
      etf.reference({ node = 'nonode@nohost', id = true, creation = 2 })
    end)

    assert.has_error(function()
      -- creation is not an integer
      etf.reference({ node = 'nonode@nohost', id = { 1 }, creation = true })
    end)

    assert.has_error(function()
      -- id field is too long
      local id = {}
      for i=1,tonumber('0x10000') do
        id[i] = i
      end
      etf.reference({ node = 'nonode@nohost', id = id, creation = 2})
    end)

  end)
end)


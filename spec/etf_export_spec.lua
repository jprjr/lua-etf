require('busted.runner')()

local etf = require'etf'

describe('export', function()
  it('is a function', function()
    assert.is_function(etf.export)
  end)

  it('allows creating exports', function()
    local a = etf.export({
      ['module'] = 'mod',
      ['function'] = 'fun',
      ['arity'] = 5,
    })

    assert.are.same({
      ['module'] = 'mod',
      ['function'] = 'fun',
      ['arity'] = 5,
    },a)
    local mt = debug.getmetatable(a)
    assert.are.same(etf.export_mt,mt)
  end)

  it('allows creating exports with atom values', function()
    local a = etf.export({
      ['module'] = etf.atom('mod'),
      ['function'] = etf.atom('fun'),
      ['arity'] = 5,
    })

    assert.are.same({
      ['module'] = etf.atom('mod'),
      ['function'] = etf.atom('fun'),
      ['arity'] = 5,
    },a)
    local mt = debug.getmetatable(a)
    assert.are.same(etf.export_mt,mt)
  end)

  it('allows creating exports with integer values', function()
    local a = etf.export({
      ['module'] = 'mod',
      ['function'] = 'fun',
      ['arity'] = etf.integer('5'),
    })

    assert.are.same({
      ['module'] = 'mod',
      ['function'] = 'fun',
      ['arity'] = 5,
    },a)
    local mt = debug.getmetatable(a)
    assert.are.same(etf.export_mt,mt)
  end)

  it('rejects exports with too large keys', function()
    assert.has_error(function()
      etf.export({
        ['module'] = string.rep('a',tonumber('0x10000')),
        ['function'] = 'fun',
        ['arity'] = 5,
      })
    end)

    assert.has_error(function()
      etf.export({
        ['module'] = 'mod',
        ['function'] = string.rep('a',tonumber('0x10000')),
        ['arity'] = 5,
      })
    end)

    assert.has_error(function()
      etf.export({
        ['module'] = 'mod',
        ['function'] = 'fun',
        ['arity'] = -1,
      })
    end)

    assert.has_error(function()
      etf.export({
        ['module'] = 'mod',
        ['function'] = 'fun',
        ['arity'] = 256,
      })
    end)

    assert.has_error(function()
      etf.export({
        ['module'] = 'mod',
        ['function'] = 'fun',
        ['arity'] = etf.integer('-1'),
      })
    end)

    assert.has_error(function()
      etf.export({
        ['module'] = 'mod',
        ['function'] = 'fun',
        ['arity'] = etf.integer('256'),
      })
    end)

  end)

end)


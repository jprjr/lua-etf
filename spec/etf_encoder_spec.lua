require('busted.runner')()

local etf = require'etf'

describe('etf.encoder', function()
  it('is a function', function()
    assert.is_function(etf.encoder)
  end)

  it('creates encoders', function()
    local enc = etf.encoder()
    assert.is.userdata(enc)
  end)

  it('accepts a version', function()
    local enc = etf.encoder({version = 131})
    assert.is.userdata(enc)
    local meta = debug.getmetatable(enc)
    assert.are.same('etf.encoder.131',meta.__name)
  end)

  it('errors on unknown version', function()
    assert.has_error(function()
      etf.encoder({version = 0})
    end)
  end)

  describe('floating-point values', function()
    local enc = etf.encoder()

    it('encodes floating-point lua numbers as NEW_FLOAT_EXT', function()
      local expected = '\131\70\63\248\0\0\0\0\0\0'
      assert.are.same(expected,enc:encode(1.5))
    end)
  end)

  describe('none', function()
    local enc = etf.encoder()
    it('produces an error', function()
      assert.has_error(function()
        enc:encode()
      end)
    end)
  end)

  describe('nil', function()
    local enc = etf.encoder()

    it('is encoded as an atom', function()
      local expected = '\131\119\3' .. 'nil'
      assert.are.same(expected,enc:encode(nil))
    end)
  end)

  describe('integer values', function()
    local enc = etf.encoder()

    it('encodes 0 as SMALL_INTEGER_EXT', function()
      local expected = '\131\97\0'
      assert.are.same(expected,enc:encode(0))
    end)

    it('encodes 255 as SMALL_INTEGER_EXT', function()
      local expected = '\131\97\255'
      assert.are.same(expected,enc:encode(255))
    end)

    it('encodes -255 as INTEGER_EXT', function()
      local expected = '\131\98\255\255\255\1'
      assert.are.same(expected,enc:encode(-255))
    end)

    it('encodes 1255 as INTEGER_EXT', function()
      local expected = '\131\98\0\0\4\231'
      assert.are.same(expected,enc:encode(1255))
    end)

    it('encodes -1255 as INTEGER_EXT', function()
      local expected = '\131\98\255\255\251\25'
      assert.are.same(expected,enc:encode(-1255))
    end)
  end)

  describe('boolean values', function()
    local enc = etf.encoder()
    it('encodes boolean true as a SMALL_ATOM_UTF8_EXT', function()
      local expected = '\131\119\4\116\114\117\101'
      assert.are.same(expected,enc:encode(true))
    end)

    it('encodes boolean false as a SMALL_ATOM_UTF8_EXT', function()
      local expected = '\131\119\5\102\97\108\115\101'
      assert.are.same(expected,enc:encode(false))
    end)
  end)

  describe('string values', function()
    local enc = etf.encoder()
    it('encodes strings as a BINARY_EXT', function()
      local expected = '\131\109\0\0\0\5\104\101\108\108\111'
      assert.are.same(expected,enc:encode('hello'))
    end)
  end)

  describe('table values', function()
    local enc = etf.encoder()
    it('encodes an empty table as a NIL_EXT', function()
      local expected = '\131\106'
      assert.are.same(expected,enc:encode({}))
    end)

    it('encodes an array-like table as LIST_EXT', function()
      local expected = '\131\108\0\0\0\1\97\0\106'
      assert.are.same(expected,enc:encode({ 0 }))
    end)

    it('encodes a map-like table as MAP_EXT', function()
      local expected = '\131\116\0\0\0\1\109\0\0\0\1\97\97\0'
      assert.are.same(expected,enc:encode({ a = 0 }))
    end)

    it('encodes a sparse table as MAP_EXT', function()
      local expected = '\131\116\0\0\0\1\109\0\0\0\1\50\97\0'
      assert.are.same(expected,enc:encode({ [2] = 0 }))
    end)

    it('encodes non-string keys into strings', function()
      local expected = '\131\116\0\0\0\1\109\0\0\0\4' .. 'true' .. '\97\0'
      assert.are.same(expected,enc:encode({ [true] = 0 }))
    end)

  end)

  describe('encoder value_map function', function()
    local atom_cache = {}
    local enc = etf.encoder({
      value_map = function(val,key)
        if key then
          if not atom_cache[val] then
            atom_cache[val] = etf.atom(val)
          end
          return atom_cache[val]
        end

        return val
      end,
    })

    it('allows mapping table keys into atoms', function()
      local expected = '\131\116\0\0\0\1\119\5\104\101\108\108\111\97\1'
      assert.are.same(expected,enc:encode({
        hello = 1,
      }))
    end)
  end)

  describe('encoder value_map table', function()
    local value_map = {
      ['true'] = '5',
      ['false'] = '10',
      [true] = 1,
      [false] = 0,
    }

    local enc = etf.encoder({
      value_map = value_map
    })

    it('allows mapping values with a table', function()
      -- we can't predict the key order so we'll
      -- just try both
      local expected_1 = table.concat({
        '\131\116',
        '\0\0\0\2',
        '\109\0\0\0\1' .. '5',
        '\97\1',
        '\109\0\0\0\2' .. '10',
        '\97\0',
      },'')

      local expected_2 = table.concat({
        '\131\116',
        '\0\0\0\2',
        '\109\0\0\0\2' .. '10',
        '\97\0',
        '\109\0\0\0\1' .. '5',
        '\97\1',
      },'')

      local encoded = enc:encode({
        ['true'] = true,
        ['false'] = false,
      })
      assert.is_true(
        expected_1 == encoded or expected_2 == encoded
      )
    end)
  end)

  describe('etf.integer values', function()
    local enc = etf.encoder()

    it('encodes integer(0) as SMALL_INTEGER_EXT', function()
      local expected = '\131\97\0'
      assert.are.same(expected,enc:encode(etf.integer('0')))
    end)

    it('encodes integer(255) as SMALL_INTEGER_EXT', function()
      local expected = '\131\97\255'
      assert.are.same(expected,enc:encode(etf.integer('255')))
    end)

    it('encodes integer(256) as INTEGER_EXT', function()
      local expected = '\131\98\0\0\1\0'
      assert.are.same(expected,enc:encode(etf.integer('256')))
    end)

    it('encodes integer(-1) as INTEGER_EXT', function()
      local expected = '\131\98\255\255\255\255'
      assert.are.same(expected,enc:encode(etf.integer('-1')))
    end)

    it('encodes integer(0x7FFFFFFF) as INTEGER_EXT', function()
      local expected = '\131\98\127\255\255\255'
      assert.are.same(expected,enc:encode(etf.integer('0x7FFFFFFF')))
    end)

    it('encodes integer(-0x80000000) as INTEGER_EXT', function()
      local expected = '\131\98\128\0\0\0'
      assert.are.same(expected,enc:encode(etf.integer('-0x80000000')))
    end)

    it('encodes integer(0x80000000) as SMALL_BIG_EXT', function()
      local expected = '\131\110\4\0\0\0\0\128'
      assert.are.same(expected,enc:encode(etf.integer('0x80000000')))
    end)

    it('encodes integer(0x80 .. string.rep(\'00\',255)) as LARGE_BIG_EXT', function()
      local expected = '\131\111\0\0\1\0\0' .. string.rep('\0',255) .. '\128'
      assert.are.same(expected,enc:encode(etf.integer('0x80' .. string.rep('00',255))))
    end)

    it('encodes integer(-0x80 .. string.rep(\'00\',255)) as LARGE_BIG_EXT', function()
      local expected = '\131\111\0\0\1\0\1' .. string.rep('\0',255) .. '\128'
      assert.are.same(expected,enc:encode(etf.integer('-0x80' .. string.rep('00',255))))
    end)
  end)

  describe('etf.float', function()
    local enc = etf.encoder()
    it('encodes a float as NEW_FLOAT_EXT', function()
      local expected = '\131\70\63\240\0\0\0\0\0\0'
      assert.are.same(expected,enc:encode(etf.float(1.0)))
    end)
  end)

  describe('etf.string', function()
    local enc = etf.encoder()
    it('encodes as a STRING_EXT', function()
      local expected = '\131\107\0\5\104\101\108\108\111'
      assert.are.same(expected,enc:encode(etf.string('hello')))
    end)
  end)

  describe('etf.binary', function()
    local enc = etf.encoder()
    it('encodes as a BINARY_EXT', function()
      local expected = '\131\109\0\0\0\5\104\101\108\108\111'
      assert.are.same(expected,enc:encode(etf.binary('hello')))
    end)
  end)

  describe('etf.atom', function()
    local enc = etf.encoder()
    it('encodes a short atom as SMALL_ATOM_UTF8_EXT', function()
      local expected = '\131\119\5\104\101\108\108\111'
      assert.are.same(expected,enc:encode(etf.atom('hello')))
    end)

    it('encodes a long atom as ATOM_UTF8_EXT', function()
      local expected = '\131\118\1\0' .. string.rep('a',256)
      assert.are.same(expected,enc:encode(etf.atom(string.rep('a',256))))
    end)

    it('encodes a non-utf8 atom as ATOM_EXT', function()
      local expected = '\131\100\0\5\104\101\108\108\111'
      assert.are.same(expected,enc:encode(etf.atom('hello',false)))
    end)
  end)

  describe('etf.export', function()
    local enc = etf.encoder()

    local expected = table.concat({
       '\131\113',
       '\119\3', -- SMALL_ATOM_UTF8_EXT
       'mod',
       '\119\3', -- SMALL_ATOM_UTF8_EXT
       'fun',
       '\97\5', -- SMALL_INTEGER_EXT = 5
    },'')

    it('encodes an EXPORT_EXT with string values', function()
      assert.are.same(expected,enc:encode(etf.export({
        ['module'] = 'mod',
        ['function'] = 'fun',
        ['arity'] = 5,
      })))
    end)

    it('encodes an EXPORT_EXT with atom values', function()
      assert.are.same(expected,enc:encode(etf.export({
        ['module'] = etf.atom('mod'),
        ['function'] = etf.atom('fun'),
        ['arity'] = 5,
      })))
    end)

    it('encodes an EXPORT_EXT with integer values', function()
      assert.are.same(expected,enc:encode(etf.export({
        ['module'] = 'mod',
        ['function'] = 'fun',
        ['arity'] = etf.integer('5'),
      })))
    end)
  end)

  describe('etf.pid', function()
    local enc = etf.encoder()

    local expected = table.concat({
      '\131\88',
      '\119\5', -- SMALL_ATOM_UTF8_EXT
      'hello',
      '\0\0\0\1', -- id
      '\0\0\0\2', -- serial,
      '\0\0\0\3', -- creation
    },'')

    it('encodes a PID_EXT with string and number values', function()
      assert.are.same(expected,enc:encode(etf.pid({
        node = 'hello',
        id = 1,
        serial = 2,
        creation = 3,
      })))
    end)

    it('encodes a PID_EXT with atom and number values', function()
      assert.are.same(expected,enc:encode(etf.pid({
        node = etf.atom('hello'),
        id = 1,
        serial = 2,
        creation = 3,
      })))
    end)

    it('encodes a PID_EXT with string and integer values', function()
      assert.are.same(expected,enc:encode(etf.pid({
        node = 'hello',
        id = etf.integer(1),
        serial = etf.integer(2),
        creation = etf.integer(3),
      })))
    end)
  end)

  describe('etf.port', function()
    local enc = etf.encoder()

    local expected = table.concat({
      '\131\89',
      '\119\5', -- SMALL_ATOM_UTF8_EXT
      'hello',
      '\0\0\0\1', -- id
      '\0\0\0\2', -- creation
    },'')

    local v4_expected = table.concat({
      '\131\120',
      '\119\5', -- SMALL_ATOM_UTF8_EXT
      'hello',
      '\0\0\0\1\0\0\0\0', -- id
      '\0\0\0\2', -- creation
    },'')

    it('encodes a NEW_PORT_EXT_EXT with string and number values', function()
      assert.are.same(expected,enc:encode(etf.port({
        node = 'hello',
        id = 1,
        creation = 2,
      })))
    end)

    it('encodes a NEW_PORT_EXT_EXT with atom and number values', function()
      assert.are.same(expected,enc:encode(etf.port({
        node = etf.atom('hello'),
        id = 1,
        creation = 2,
      })))
    end)

    it('encodes a NEW_PORT_EXT_EXT with string and integer values', function()
      assert.are.same(expected,enc:encode(etf.port({
        node = 'hello',
        id = etf.integer(1),
        creation = etf.integer(2),
      })))
    end)

    it('encodes a V4_PORT_EXT_EXT with string and integer values', function()
      assert.are.same(v4_expected,enc:encode(etf.port({
        node = 'hello',
        id = etf.integer('0x100000000'),
        creation = 2,
      })))
    end)

    it('encodes a V4_PORT_EXT_EXT with atom and integer values', function()
      assert.are.same(v4_expected,enc:encode(etf.port({
        node = etf.atom('hello'),
        id = etf.integer('0x100000000'),
        creation = etf.integer(2),
      })))
    end)

  end)

  describe('etf.reference', function()
    local enc = etf.encoder()

    local expected = table.concat({
      '\131\90',
      '\0\1', -- length
      '\119\5', -- SMALL_ATOM_UTF8_EXT
      'hello',
      '\0\0\0\2', -- creation
      '\0\0\0\1', -- id
    },'')

    it('encodes a NEWER_REFERENCE_EXT with a string node', function()
      assert.are.same(expected,enc:encode(etf.reference({
        node = 'hello',
        creation = 2,
        id = { 1 },
      })))
    end)

    it('encodes a NEWER_REFERENCE_EXT with an atom node', function()
      assert.are.same(expected,enc:encode(etf.reference({
        node = etf.atom('hello'),
        creation = 2,
        id = { 1 },
      })))
    end)

    it('encodes a NEWER_REFERENCE_EXT with an integer values', function()
      assert.are.same(expected,enc:encode(etf.reference({
        node = 'hello',
        creation = etf.integer(2),
        id = { etf.integer(1) },
      })))
    end)

  end)

  describe('etf.map', function()
    local enc = etf.encoder()

    it('encodes array-like tables as map', function()
      local expected = table.concat({
        '\131\116',
        '\0\0\0\1', -- arity
        '\109\0\0\0\1', -- BINARY_EXT lenth 1
        '1',
        '\97\1', -- value (1)
      },'')

      assert.are.same(expected,enc:encode(etf.map({
        -- note that the integer key will be converted
        -- to a string by default
        [1] = 1,
      })))
    end)

    it('encodes empty tables as map', function()
      local expected = table.concat({
        '\131\116',
        '\0\0\0\0', -- arity
      },'')
      assert.are.same(expected,enc:encode(etf.map({ })))
    end)

  end)

  describe('etf.list', function()
    local enc = etf.encoder()

    it('encodes an empty table as NIL_EXT', function()
      local expected = '\131\106'
      assert.are.same(expected,enc:encode(etf.list({})))
    end)
  end)

  describe('etf.fun', function()
    local enc = etf.encoder()
    local expected = table.concat({
      '\131\117',
      '\0\0\0\1', -- numfree (1)
       table.concat({
        '\88', -- NEW_PID_EXT
        '\119\13', -- SMALL_ATOM_UTF8_EXT
        'nonode@noname', -- node
        '\0\0\0\5', -- id
        '\0\0\0\2', -- serial
        '\0\0\0\1', -- creation
      },''), -- pid,
      '\119\3' .. 'mod', -- module
      '\97\10', -- index,
      '\97\11', -- uniq,
      '\97\5', -- free_vars
    },'')


    it('encodes as a FUN_EXT', function()
      local fun = setmetatable({
        pid = etf.pid({
          node = 'nonode@noname',
          id = 5,
          serial = 2,
          creation = 1,
        }),
        numfree = 1,
        ['module'] = 'mod',
        index = 10,
        uniq = 11,
        free_vars = { 5 },
      }, etf.fun_mt)

      assert.are.same(expected,enc:encode(fun))
    end)

    it('encodes as a FUN_EXT with integer values', function()
      local fun = setmetatable({
        pid = etf.pid({
          node = 'nonode@noname',
          id = 5,
          serial = 2,
          creation = 1,
        }),
        numfree = etf.integer(1),
        ['module'] = 'mod',
        index = etf.integer(10),
        uniq = etf.integer(11),
        free_vars = { 5 },
      }, etf.fun_mt)

      assert.are.same(expected,enc:encode(fun))
    end)
  end)

  describe('etf.new_fun', function()
    local enc = etf.encoder()
    local expected = table.concat({
      '\131\112',
      '\0\0\0\8', -- size
      '\1', -- arity
      '\0\1\2\3',
      '\4\5\6\7',
      '\8\9\10\11',
      '\12\13\14\15', -- uniq
      '\0\0\0\9', -- index
      '\0\0\0\1', -- numfree
      '\119\3' .. 'mod', -- module
      '\97\10', -- oldindex,
      '\97\11', -- olduniq,
       table.concat({
        '\88', -- NEW_PID_EXT
        '\119\13',
        'nonode@noname', -- node
        '\0\0\0\5', -- id
        '\0\0\0\2', -- serial
        '\0\0\0\1', -- creation
      },''), -- pid,
      '\97\5', -- free_vars
    },'')


    it('encodes as a NEW_FUN_EXT', function()
      local new_fun = setmetatable({
        size = 8,
        arity = 1,
        uniq = '\0\1\2\3' ..
               '\4\5\6\7' ..
               '\8\9\10\11' ..
               '\12\13\14\15',
        index = 9,
        numfree = 1,
        ['module'] = 'mod',
        oldindex = 10,
        olduniq = 11,
        pid = etf.pid({
          node = 'nonode@noname',
          id = 5,
          serial = 2,
          creation = 1,
        }),
        free_vars = { 5 },
      }, etf.new_fun_mt)
      assert.are.same(expected,enc:encode(new_fun))
    end)

    it('encodes as a NEW_FUN_EXT with integer values', function()
      local new_fun = setmetatable({
        size = etf.integer(8),
        arity = 1,
        uniq = '\0\1\2\3' ..
               '\4\5\6\7' ..
               '\8\9\10\11' ..
               '\12\13\14\15',
        index = etf.integer(9),
        numfree = etf.integer(1),
        ['module'] = 'mod',
        oldindex = 10,
        olduniq = 11,
        pid = etf.pid({
          node = 'nonode@noname',
          id = 5,
          serial = 2,
          creation = 1,
        }),
        free_vars = { 5 },
      }, etf.new_fun_mt)

      assert.are.same(expected,enc:encode(new_fun))
    end)
  end)

  describe('compression', function()
    it('allows compression with a set level', function()
      local enc = etf.encoder({ compress = 1 })
      local comp = enc:encode('hello')
      assert.are.same(string.sub(comp,1,2),'\131\80')
      assert.are.same('hello',etf.decode(comp))
    end)

    it('allows compression with a boolean value', function()
      local enc = etf.encoder({ compress = true })
      local comp = enc:encode('hello')
      assert.are.same(string.sub(comp,1,2),'\131\80')
      assert.are.same('hello',etf.decode(comp))
    end)

    it('allows compression levels -1 -> 9', function()
      assert.has_no_error(function()
        for i=-1,9 do
          etf.encoder({ compress = i })
        end
      end)
    end)

    it('allows compression levels -1 -> 9 as strings', function()
      assert.has_no_error(function()
        for i=-1,9 do
          etf.encoder({ compress = tostring(i) })
        end
      end)
    end)

    it('rejects invalid compression levels', function()
      assert.has_error(function()
        etf.encoder({ compress = 10 })
      end)
      assert.has_error(function()
        etf.encoder({ compress = -2 })
      end)
      assert.has_error(function()
        etf.encoder({ compress = 'hi there' })
      end)
    end)
  end)

end)



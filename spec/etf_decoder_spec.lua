require('busted.runner')()

local etf = require'etf'

describe('etf.decoder', function()
  it('creates decoders', function()
    local dec = etf.decoder()
    assert.is.userdata(dec)
  end)

  it('accepts a version', function()
    local dec = etf.decoder({version = 131})
    assert.is.userdata(dec)
    local meta = debug.getmetatable(dec)
    assert.are.same('etf.decoder.131',meta.__name)
  end)

  it('errors on unknown version', function()
    assert.has_error(function()
      etf.decoder({version = 0})
    end)
  end)

  it('errors on being called without a string', function()
    local dec = etf.decoder()
    assert.has_error(function()
      dec:decode()
    end)
    assert.has_error(function()
      dec:decode(true)
    end)
    assert.has_error(function()
      dec:decode(1)
    end)
    assert.has_error(function()
      dec:decode({})
    end)
  end)

  describe('decodes', function()
    local dec = etf.decoder()

    it('ZLIB-compressed', function()
      local bin = table.concat({
        '\131\80',
        '\0\0\0\10',
        '\120\218\1\10',
        '\0\245\255\109',
        '\0\0\0\5',
        '\104\101\108\108',
        '\111\10\145\2',
        '\135',
      },'')
      local res = 'hello'
      assert.are.same(res,dec:decode(bin))
    end)


    it('ATOM_CACHE_REF as nil', function()
      local bin = '\131\82\1'
      assert.is_nil(dec:decode(bin))
    end)

    it('SMALL_INT_EXT as a number', function()
      local bin = '\131\97\123'
      local res = 123
      assert.are.same(res,dec:decode(bin))
    end)

    it('positive INTEGER_EXT as a number when safe', function()
      local bin = '\131\98\0\0\1\0'
      local res = 256
      assert.are.same(res,dec:decode(bin))
    end)

    it('negative INTEGER_EXT as a number when safe', function()
      local bin = '\131\98\255\255\255\0'
      local res = -256
      assert.are.same(res,dec:decode(bin))
    end)

    describe('INTEGER_EXT max', function()
      local bin = '\131\98\127\255\255\255'

      if etf.maxinteger < etf.integer('0x7fffffff') then
        it('as integer', function()
          local res = etf.integer('0x7fffffff')
          assert.are.same(res,dec:decode(bin))
        end)
      else
        it('as number', function()
          local res = tonumber('0x7fffffff')
          assert.are.same(res,dec:decode(bin))
        end)
      end
    end)

    describe('INTEGER_EXT min', function()
      local bin = '\131\98\128\0\0\0'

      if etf.mininteger > etf.integer('-0x80000000') then
        it('as integer', function()
          local res = etf.integer('-0x80000000')
          assert.are.same(res,dec:decode(bin))
        end)
      else
        it('as number', function()
          local res = -1 * tonumber('0x80000000')
          assert.are.same(res,dec:decode(bin))
        end)
      end
    end)

    it('FLOAT_EXT', function()
      local bin = table.concat({
        '\131\99',
        '1.50000000000000000000e+00',
        '\0\0\0\0\0',
      },'')
      local res = 1.5
      assert.are.same(res,dec:decode(bin))
    end)

    it('PORT_EXT', function()
      local bin = table.concat({
        '\131\102',
        '\119\13',
        'nonode@noname', -- node
        '\0\0\0\5', -- id
        '\1', -- creation
      },'')

      local res = etf.port({
        node = 'nonode@noname',
        id = 5,
        creation = 1,
      })

      local port = dec:decode(bin)

      assert.are.same(res,port)
      assert.are.same(etf.port_mt,debug.getmetatable(port))
    end)

    it('NEW_PORT_EXT', function()
      local bin = table.concat({
        '\131\89',
        '\119\13',
        'nonode@noname', -- node
        '\0\0\0\5', -- id
        '\0\0\0\1', -- creation
      },'')

      local res = etf.port({
        node = 'nonode@noname',
        id = 5,
        creation = 1,
      })

      local port = dec:decode(bin)

      assert.are.same(res,port)
      assert.are.same(etf.port_mt,debug.getmetatable(port))
    end)

    it('V4_PORT_EXT', function()
      local bin = table.concat({
        '\131\120',
        '\119\13',
        'nonode@noname', -- node
        '\0\0\0\0\0\0\0\5', -- id
        '\0\0\0\1', -- creation
      },'')

      local res = etf.port({
        node = 'nonode@noname',
        id = 5,
        creation = 1,
      })

      local port = dec:decode(bin)

      assert.are.same(res,port)
      assert.are.same(etf.port_mt,debug.getmetatable(port))
    end)

    it('PID_EXT', function()
      local bin = table.concat({
        '\131\103',
        '\119\13',
        'nonode@noname', -- node
        '\0\0\0\5', -- id
        '\0\0\0\2', -- serial
        '\1', -- creation
      },'')

      local res = etf.pid({
        node = 'nonode@noname',
        id = 5,
        serial = 2,
        creation = 1,
      })

      local pid = dec:decode(bin)

      assert.are.same(res,pid)
      assert.are.same(etf.pid_mt,debug.getmetatable(pid))
    end)

    it('NEW_PID_EXT', function()
      local bin = table.concat({
        '\131\88',
        '\119\13',
        'nonode@noname', -- node
        '\0\0\0\5', -- id
        '\0\0\0\2', -- serial
        '\0\0\0\1', -- creation
      },'')

      local res = etf.pid({
        node = 'nonode@noname',
        id = 5,
        serial = 2,
        creation = 1,
      })

      local pid = dec:decode(bin)

      assert.are.same(res,pid)
      assert.are.same(etf.pid_mt,debug.getmetatable(pid))
    end)

    it('SMALL_TUPLE_EXT', function()
      local bin = table.concat({
        '\131\104',
        '\2', -- arity
        '\97\2',
        '\97\3',
      },'')

      local res = etf.tuple({ 2, 3 })

      local tuple = dec:decode(bin)

      assert.are.same(res,tuple)
      assert.are.same(etf.tuple_mt,debug.getmetatable(tuple))
    end)

    it('LARGE_TUPLE_EXT', function()
      local bin = table.concat({
        '\131\105',
        '\0\0\0\2', -- arity
        '\97\4',
        '\97\5',
      },'')

      local res = etf.tuple({ 4, 5 })

      local tuple = dec:decode(bin)

      assert.are.same(res,tuple)
      assert.are.same(etf.tuple_mt,debug.getmetatable(tuple))
    end)

    it('MAP_EXT', function()
      -- by default, atom keys are mapped to strings,
      -- the atom "true" is mapped to boolean true
      local bin = table.concat({
        '\131\116',
        '\0\0\0\1', -- arity
        '\119\3' .. 'key', -- key (atom "key")
        '\119\4' .. 'true', -- value (atom "true")
      },'')

      local res = etf.map({ key = true })

      local map = dec:decode(bin)

      assert.are.same(res,map)
      assert.are.same(etf.map_mt,debug.getmetatable(map))
    end)

    it('NIL_EXT', function()
      local bin = '\131\106'
      local res = {}
      assert.are.same(res,dec:decode(bin))
    end)

    it('STRING_EXT', function()
      local bin = '\131\107\0\5' .. 'hello'
      local res = 'hello'
      assert.are.same(res,dec:decode(bin))
    end)

    it('LIST_EXT', function()
      local bin = table.concat({
        '\131\108',
        '\0\0\0\2', -- length
        '\97\6',
        '\97\7',
        '\106', -- tail
      },'')

      local res = etf.list({ 6, 7 })

      local list = dec:decode(bin)

      assert.are.same(res,list)
      assert.are.same(etf.list_mt,debug.getmetatable(list))
    end)

    it('BINARY_EXT', function()
      local bin = '\131\109\0\0\0\5' .. 'hello'
      local res = 'hello'
      assert.are.same(res,dec:decode(bin))
    end)

    describe('SMALL_BIG_EXT', function()
      it('1', function()
        local bin = table.concat({
          '\131\110',
          '\1', -- n
          '\0', -- sign,
          '\1', -- value
        },'')
        local res = 1
        assert.are.same(res,dec:decode(bin))
      end)

      it('-1', function()
        local bin = table.concat({
          '\131\110',
          '\1', -- n
          '\1', -- sign,
          '\1', -- value
        },'')
        local res = -1
        assert.are.same(res,dec:decode(bin))
      end)

      describe('INT64_MAX', function()
        local bin = table.concat({
          '\131\110',
          '\8',
          '\0',
          '\255\255\255\255',
          '\255\255\255\127',
        },'')

        if etf.maxinteger < etf.integer('0x7fffffffffffffff') then
          it('as integer', function()
            local res = etf.integer('0x7fffffffffffffff')
            assert.are.same(res,dec:decode(bin))
          end)
        else
          it('as number', function()
            local res = tonumber('0x7fffffffffffffff')
            assert.are.same(res,dec:decode(bin))
          end)
        end
      end)

      describe('INT64_MIN', function()
        local bin = table.concat({
          '\131\110',
          '\8',
          '\1',
          '\0\0\0\0\0\0\0\128',
        },'')

        if etf.mininteger > etf.integer('-0x8000000000000000') then
          it('as integer', function()
            local res = etf.integer('-0x8000000000000000')
            assert.are.same(res,dec:decode(bin))
          end)
        else
          it('as number', function()
            local res = tonumber('-0x8000000000000000')
            assert.are.same(res,dec:decode(bin))
          end)
        end
      end)
    end)

    describe('LARGE_BIG_EXT', function()
      it('1', function()
        local bin = table.concat({
          '\131\111',
          '\0\0\0\1', -- n
          '\0', -- sign,
          '\1', -- value
        },'')
        local res = 1
        assert.are.same(res,dec:decode(bin))
      end)

      it('-1', function()
        local bin = table.concat({
          '\131\111',
          '\0\0\0\1', -- n
          '\1', -- sign,
          '\1', -- value
        },'')
        local res = -1
        assert.are.same(res,dec:decode(bin))
      end)

      describe('INT64_MAX', function()
        local bin = table.concat({
          '\131\111',
          '\0\0\0\8',
          '\0',
          '\255\255\255\255',
          '\255\255\255\127',
        },'')

        if etf.maxinteger < etf.integer('0x7fffffffffffffff') then
          it('as integer', function()
            local res = etf.integer('0x7fffffffffffffff')
            assert.are.same(res,dec:decode(bin))
          end)
        else
          it('as number', function()
            local res = tonumber('0x7fffffffffffffff')
            assert.are.same(res,dec:decode(bin))
          end)
        end
      end)

      describe('INT64_MIN', function()
        local bin = table.concat({
          '\131\111',
          '\0\0\0\8',
          '\1',
          '\0\0\0\0\0\0\0\128',
        },'')

        if etf.mininteger > etf.integer('-0x8000000000000000') then
          it('as integer', function()
            local res = etf.integer('-0x8000000000000000')
            assert.are.same(res,dec:decode(bin))
          end)
        else
          it('as number', function()
            local res = tonumber('-0x8000000000000000')
            assert.are.same(res,dec:decode(bin))
          end)
        end
      end)
    end)

    it('REFERENCE_EXT', function()
      local bin = table.concat({
        '\131\101',
        '\119\13',
        'nonode@noname', -- node
        '\0\0\0\5', -- id
        '\1', -- creation
      },'')

      local res = etf.reference({
        node = 'nonode@noname',
        id = { 5 },
        creation = 1,
      })

      local reference = dec:decode(bin)

      assert.are.same(res,reference)
      assert.are.same(etf.reference_mt,debug.getmetatable(reference))
    end)

    it('NEW_REFERENCE_EXT', function()
      local bin = table.concat({
        '\131\114',
        '\0\1', -- len
        '\119\13',
        'nonode@noname', -- node
        '\1', -- creation
        '\0\0\0\5', -- id
      },'')

      local res = etf.reference({
        node = 'nonode@noname',
        id = { 5 },
        creation = 1,
      })

      local reference = dec:decode(bin)

      assert.are.same(res,reference)
      assert.are.same(etf.reference_mt,debug.getmetatable(reference))
    end)

    it('NEWER_REFERENCE_EXT', function()
      local bin = table.concat({
        '\131\90',
        '\0\1', -- len
        '\119\13',
        'nonode@noname', -- node
        '\0\0\0\1', -- creation
        '\0\0\0\5', -- id
      },'')

      local res = etf.reference({
        node = 'nonode@noname',
        id = { 5 },
        creation = 1,
      })

      local reference = dec:decode(bin)

      assert.are.same(res,reference)
      assert.are.same(etf.reference_mt,debug.getmetatable(reference))
    end)

    it('FUN_EXT', function()
      local bin = table.concat({
        '\131\117',
        '\0\0\0\1', -- numfree
         table.concat({
          '\103',
          '\119\13',
          'nonode@noname', -- node
          '\0\0\0\5', -- id
          '\0\0\0\2', -- serial
          '\1', -- creation
        },''), -- pid,
        '\119\3' .. 'mod', -- module
        '\97\10', -- index,
        '\97\11', -- uniq,
        '\97\5', -- free_vars
      },'')

      local res = setmetatable({
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

      local fun = dec:decode(bin)

      assert.are.same(res,fun)
      assert.are.same(etf.fun_mt,debug.getmetatable(fun))
    end)

    it('NEW_FUN_EXT', function()
      local bin = table.concat({
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
          '\103',
          '\119\13',
          'nonode@noname', -- node
          '\0\0\0\5', -- id
          '\0\0\0\2', -- serial
          '\1', -- creation
        },''), -- pid,
        '\97\5', -- free_vars
      },'')

      local res = setmetatable({
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

      local fun = dec:decode(bin)

      assert.are.same(res,fun)
      assert.are.same(etf.new_fun_mt,debug.getmetatable(fun))
    end)

    it('EXPORT_EXT', function()
      local bin = table.concat({
        '\131\113',
        '\119\3' .. 'mod',
        '\119\3' .. 'fun',
        '\97\1',
      },'')
      local res = etf.export({
        ['module'] = 'mod',
        ['function'] = 'fun',
        ['arity'] = 1,
      })

      local export = dec:decode(bin)
      assert.are.same(res,export)
      assert.are.same(etf.export_mt,debug.getmetatable(export))
    end)

    it('BIT_BINARY_EXT', function()
      local bin = table.concat({
        '\131\77',
        '\0\0\0\2',
        '\7',
        '\200\254',
      }, '')
      local res = '\200\127'
      assert.are.same(res,dec:decode(bin))
    end)

    it('NEW_FLOAT_EXT', function()
      local bin = '\131\70\63\248\0\0\0\0\0\0'
      assert.are.same(1.5,dec:decode(bin))
    end)

    describe('ATOM_UTF8_EXT', function()
      it('true as boolean true', function()
        local bin = '\131\118\0\4' .. 'true'
        assert.are.same(true,dec:decode(bin))
      end)

      it('false as boolean false', function()
        local bin = '\131\118\0\5' .. 'false'
        assert.are.same(false,dec:decode(bin))
      end)

      it('nil as etf.null', function()
        local bin = '\131\118\0\3' .. 'nil'
        assert.are.same(etf.null,dec:decode(bin))
      end)

      it('hello as lua string', function()
        local bin = '\131\118\0\5' .. 'hello'
        assert.are.same('hello',dec:decode(bin))
      end)
    end)

    describe('SMALL_ATOM_UTF8_EXT', function()
      it('true as boolean true', function()
        local bin = '\131\119\4' .. 'true'
        assert.are.same(true,dec:decode(bin))
      end)

      it('false as boolean false', function()
        local bin = '\131\119\5' .. 'false'
        assert.are.same(false,dec:decode(bin))
      end)

      it('nil as etf.null', function()
        local bin = '\131\119\3' .. 'nil'
        assert.are.same(etf.null,dec:decode(bin))
      end)

      it('hello as lua string', function()
        local bin = '\131\119\5' .. 'hello'
        assert.are.same('hello',dec:decode(bin))
      end)
    end)

    describe('ATOM_EXT', function()
      it('true as boolean true', function()
        local bin = '\131\100\0\4' .. 'true'
        assert.are.same(true,dec:decode(bin))
      end)

      it('false as boolean false', function()
        local bin = '\131\100\0\5' .. 'false'
        assert.are.same(false,dec:decode(bin))
      end)

      it('nil as etf.null', function()
        local bin = '\131\100\0\3' .. 'nil'
        assert.are.same(etf.null,dec:decode(bin))
      end)

      it('hello as lua string', function()
        local bin = '\131\100\0\5' .. 'hello'
        assert.are.same('hello',dec:decode(bin))
      end)
    end)

    describe('SMALL_ATOM_EXT', function()
      it('true as boolean true', function()
        local bin = '\131\115\4' .. 'true'
        assert.are.same(true,dec:decode(bin))
      end)

      it('false as boolean false', function()
        local bin = '\131\115\5' .. 'false'
        assert.are.same(false,dec:decode(bin))
      end)

      it('nil as etf.null', function()
        local bin = '\131\115\3' .. 'nil'
        assert.are.same(etf.null,dec:decode(bin))
      end)

      it('hello as lua string', function()
        local bin = '\131\115\5' .. 'hello'
        assert.are.same('hello',dec:decode(bin))
      end)
    end)

  end)

  it('accepts an atom_map table', function()
    local dec = etf.decoder({atom_map = {['hello'] = 'hi'}})
    assert.is.userdata(dec)
    local val = dec:decode('\131\115\5\104\101\108\108\111')
    assert.are.same('hi',val)
  end)

  it('accepts at atom_map function', function()
    local dec = etf.decoder({atom_map = function(str)
      return str .. str
    end})
    local val = dec:decode('\131\115\5\104\101\108\108\111')
    assert.are.same('hellohello',val)
  end)

  it('allows mapping atom keys and values separately', function()
    local dec = etf.decoder({atom_map = function(str,is_key)
      if is_key then
        return str
      end
      return etf.atom(str)
    end})
    local val = dec:decode('\131\116\0\0\0\1\100\0\1\97\100\0\1\98')
    assert.are.same({
      ['a'] = etf.atom('b')
    },val)
  end)

  describe('allows returning all ints as integers', function()
    local dec = etf.decoder({use_integer = true })

    it('returns a SMALL_INTEGER_EXT as integer', function()
      local val = dec:decode('\131\97\0');
      assert.is_userdata(val)
      assert.are.same(val,etf.integer(0))
      assert.are.same(etf.integer_mt,debug.getmetatable(val))
    end)

    it('returns INTEGER_EXT as integer', function()
      local val = dec:decode('\131\98\0\0\1\0');
      assert.is_userdata(val)
      assert.are.same(val,etf.integer(256))
      assert.are.same(etf.integer_mt,debug.getmetatable(val))
    end)

    it('returns SMALL_BIG_EXT as integer', function()
      local val = dec:decode('\131\110\1\0\255')
      assert.is_userdata(val)
      assert.are.same(val,etf.integer(255))
      assert.are.same(etf.integer_mt,debug.getmetatable(val))
    end)

    it('returns LARGE_BIG_EXT as integer', function()
      local val = dec:decode('\131\111\0\0\0\1\0\255')
      assert.is_userdata(val)
      assert.are.same(val,etf.integer(255))
      assert.are.same(etf.integer_mt,debug.getmetatable(val))
    end)
  end)

  describe('allows returning all floats as etf.float', function()
    local dec = etf.decoder({use_float = true })

    it('returns FLOAT_EXT as an etf.float', function()
      local val = dec:decode('\131\99\49\46\48\48\48\48\48\48\48\48\48\48\48\48\48\48\48\48\48\48\48\48\101\43\48\48\0\0\0\0\0')
      assert.are.same(val,etf.float(1.0))
      assert.are.same(etf.float_mt,debug.getmetatable(val))
    end)

    it('returns NEW_FLOAT_EXT as an etf.float', function()
      local val = dec:decode('\131\70\63\240\0\0\0\0\0\0')
      assert.are.same(val,etf.float(1.0))
      assert.are.same(etf.float_mt,debug.getmetatable(val))
    end)
  end)

end)

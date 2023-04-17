# etf

A Lua library for encoding and decoding the Erlang [External Term Format](https://www.erlang.org/doc/apps/erts/erl_ext_dist.html).

Tested on Lua 5.1 - 5.4 and LuaJIT.

By default, this decodes and encodes with logic similar to [erlpack](https://github.com/discord/erlpack),
but decoders and encoders have options to customize how values are processed.

## LICENSE

MIT (see file `LICENSE`).

Some files are third-party and contain their own licensing:

* `csrc/thirdparty/bigint/bigint.h`: Zero-clause BSD, details in source file.
* `csrc/thirdparty/miniz/miniz.h`: MIT license, details in `csrc/thirdparty/miniz/LICENSE`.

## Decoding

```lua
local decoder = etf.decoder(options) -- create a decoder
local decoded = decoder:decode('\131\116\0\0\0\2\100\0\1\97\97\1\100\0\1\98\97\2')

-- decoded will be a table like:
   { a = 1, b = 2 }
```

`options` is an optional table with the following keys, all optional:

* `use_integer` - set to `true` to decode all integers as `etf.integer` userdata.
* `use_float` - set to `true` to decode all floats as `etf.float` userdata.
* `version` - specify the Erlang Term Format version you wish to decode.
As far as I can tell, `131` is the only version in existence.
* `atom_map` - customize how Atom types are decoded. This can be a table, or
a function that accepts a string (representing the atom name) and a boolean (`true`
if the atom is a map key, `false` otherwise).

Here's how various Erlang types are mapped to Lua by default:

| Supported | Erlang Type | Lua Type |
|-----------|-------------|----------|
| [ ] | `ATOM_CACHE_REF` |  |
| [x] | `ZLIB` | (automatically decompressed and decoded) |
| [x] | `SMALL_INTEGER_EXT` | `number` |
| [x] | `INTEGER_EXT` | `number` or `etf.integer` (based on value) |
| [x] | `FLOAT_EXT` | `number` |
| [x] | `PORT_EXT` | `table` |
| [x] | `NEW_PORT_EXT` | `table` |
| [x] | `V4_PORT_EXT` | `table` |
| [x] | `PID_EXT` | `table` |
| [x] | `NEW_PID_EXT` | `table` |
| [x] | `SMALL_TUPLE_EXT` | `table` |
| [x] | `LARGE_TUPLE_EXT` | `table` |
| [x] | `MAP_EXT` | `table` |
| [x] | `NIL_EXT` | `table` (empty) |
| [x] | `STRING_EXT` | `string` |
| [x] | `LIST_EXT` | `table` |
| [x] | `BINARY_EXT` | `string` |
| [x] | `SMALL_BIG_EXT` | `number` or `etf.integer` |
| [x] | `LARGE_BIG_EXT` | `number` or `etf.integer` |
| [x] | `REFERENCE_EXT` | `table` |
| [x] | `NEW_REFERENCE_EXT` | `table` |
| [x] | `NEWER_REFERENCE_EXT` | `table` |
| [x] | `FUN_EXT` | `table` |
| [x] | `NEW_FUN_EXT` | `table` |
| [x] | `EXPORT_EXT` | `table` |
| [x] | `BIT_BINARY_EXT` | `string` |
| [x] | `NEW_FLOAT_EXT` | `number` |
| [x] | `ATOM_UTF8_EXT` | `string` or `boolean` or `etf.null` |
| [x] | `SMALL_ATOM_UTF8_EXT` | `string` or `boolean` or `etf.null` |
| [x] | `ATOM_EXT` | `string` or `boolean` or `etf.null` |
| [x] | `SMALL_ATOM_EXT` | `string` or `boolean` or `etf.null` |

### Decoding Details

### Integers

`etf` will figure out the maximum and minimum integer values that can be
safely handled by Lua at run-time. When any integer is decoded, it will
use Lua's `number` type if possible, and a `etf.integer` userdata if it's
outside the safe range.

You can opt to have all integers be returned as `etf.integer` userdatas. The
benefit of this is all values will use the same type. On Lua 5.2 and later,
the `etf.integer` userdatas can be compared to regular Lua numbers, but on
Lua 5.1 you can only compare `etf.integer` values with other `etf.integer`
values.

To enable this, create the decoder with the `use_integer` option set to `true`:

```lua
local etf = require'etf'
local decoder = etf.decoder({use_integer = true })
local val = decoder:decode('\131\97\1') -- returns a integer
print(debug.getmetatable(val).__name)
-- prints "etf.integer"
```

### Atoms

Erlang supports a concept of "atoms" which doesn't completely translate to
Lua.

In Erlang, one can create a map like:

```erlang
Map = #{ a => 1, b => false, c => hello }
```

In that example, `a`, `b`, `false`, and `hello` are all atoms. They're
essentially small strings that can be used for map keys, enums, etc.

Note that Erlang doesn't have a `boolean` type. `false` is just another atom.

By default, atoms are decoded with the following logic:

* If the atom is a map key (like `a` and `b` in the example), it's decoded as a string.
* If the atom is a value (like `false` and `hello` in the example, then:
    * Atom `true` is decoded as Lua's boolean `true`.
    * Atom `false` is decoded as Lua's boolean `false`.
    * Atom `nil` is decoded as `etf.null`, which is an atom userdata.
    * Anything else is decoded as a string.

This is meant to be compatible with [erlpack](https://github.com/discord/erlpack), and
to make decoded data as easy to handle as possible.

If your application has other atoms that need to be translated into values,
you can specify the `atom_map` parameter. This can be a table with string keys,
or a function. The function should accept a string parameter representing
the atom name, and a boolean representing if the atom is a map key or not.

The default logic can be represented as:

```lua
local function atom_map(str, is_key)
  if is_key then return str end
  if str == 'true' then
    return true
  elseif str == 'false' then
    return false
  elseif str == 'nil' then
    return etf.null
  end
  return str
end
```

If for example, you wanted to keep string keys but keep the values as atoms:

```lua
local function atom_map(str, is_key)
  if is_key then return str end
  return etf.atom(str)
end
```

### Tuples and Lists

`SMALL_TUPLE_EXT`, `LARGE_TUPLE_EXT`, `LIST_EXT`, and `NIL_EXT`
will be decoded into array-like tables (all keys are integers, they're consecutive,
and they start at 1).

The table will have a metatable set to indicate the original type - `etf.tuple_mt` for tuples, and `etf.list_mt` for lists.

### Maps

`MAP_EXT` will be decoded into a Lua table. By default, the keys are (probably) strings,
see above about how atoms are mapped. Values are mapped into the appropriate Lua type
according to the above table.

The table will have a metatable set to indicate it was a map - `etf.map_mt`.

### Ports

The various `PORT` types (`PORT_EXT`, `NEW_PORT_EXT`, `V4_PORT_EXT`) will be decoded into
a table with the following fields:

* `node` - a string.
* `id` - a number or integer.
* `creation` - a number or integer.

The table will have a metatable set to `etf.port_mt`.

### PIDs

The `PID` types (`PID_EXT`, `NEW_PID_EXT`) will be decoded into a table with
the following fields:

* `node` - a string.
* `id` - a number or integer.
* `serial` - a number or integer.
* `creation` - a number or integer.

The table will have a metatable set to `etf.pid_mt`.

### FUN_EXT

`FUN_EXT` will be decoded into a table with the following fields:

* `numfree` - a number or integer.
* `pid` - the previously-mentioned `PID` type.
* `module` - a string.
* `index` - a number or integer.
* `uniq` - a number or integer.
* `free_vars` - an array like table of terms.

The table will have a metatable set to `etf.fun_mt`.

### NEW_FUN_EXT

`NEW_FUN_EXT` will be decoded into a table with the following fields:

* `size` - a number or integer.
* `arity` - a number.
* `uniq` - a string.
* `index` - a number or integer.
* `numfree` - a number or integer.
* `module` - a string.
* `oldindex` - a number or integer.
* `olduniq` - a number or integer.
* `pid` - the previously-mentioned `PID` type.
* `free_vars` - an array like table of terms.

The table will have a metatable set to `etf.new_fun_mt`.

### EXPORT_EXT

`EXPORT_EXT` will be decoded into a table with the following fields:

* `module` - a string.
* `function` - a string.
* `arity` - a number or integer.

The table will have a metatable set to `etf.export_mt`.

### References

The `REFERENCE` types (`REFERENCE_EXT`, `NEW_REFERENCE_EXT`, `NEWER_REFERENCE_EXT`) will
be decoded into a table with the following fields:

* `node` - a string.
* `creation` - a number or integer.
* `id` - an array-like table of numbers or integers.

The table will have a metatable set to `etf.reference_mt`.

## Encoding

```lua
local encoder = etf.encoder(options) -- create a encoder
local encoded = encoder:encode({ a = 1, b = 2 })
-- encoded will be a MAP_EXT with BINARY_EXT keys and SMALL_INT_EXT values
```

`options` is an optional table with the following keys, all optional:

* `version` - specify the Erlang Term Format version you wish to encode.
As far as I can tell, `131` is the only version in existence.
* `compress` - set to `true` to enable compression at the default level, or
`0` through `9` to specify a compression level.
* `value_map` - customize how values are encoded, this can be a table or a
function that accepts the value to be encoded, and a boolean indicating if
the value is a table key.

### Lua Types

Here's how various Lua types are mapped to Erlang Term Format by default:

| Supported | Lua Type | Erlang Type |
|-----------|----------|-------------|
| [x] | `nil` | a `nil` `SMALL_ATOM_UTF8_EXT` |
| [x] | `number` | `NEW_FLOAT_EXT`, `SMALL_INTEGER_EXT`, `INTEGER_EXT`, `SMALL_BIG_EXT`, `LARGE_BIG_EXT` (as appropriate) |
| [x] | `boolean` | `SMALL_ATOM_UTF8_EXT` |
| [x] | `string` | `BINARY_EXT` |
| [x] | `table` | `NIL_EXT`, `LIST_EXT`, or `MAP_EXT` |
| [x] | `userdata` | (see details below) |


### A note on tables

A table is determined to either be map-like or list-like. If a table
has integer keys starting at 1, with no gaps, it's considered to be
list-like and will be encoded as a `LIST_EXT`.

If a table has no keys at all, it will be treated as a list-type
with zero items and encoded as a `NIL_EXT` (Erlang's version of an
empty list).

Otherwise, the table is considered map-like, and will be encoded
as a `MAP_EXT`. All table keys will be encoded as strings (specifically
`BINARY_EXT`). This is meant to be compatible with [erlpack](https://github.com/discord/erlpack).

### Userdata types

`etf` allows creating various userdata to force a specific encoding:

| Userdata | Erlang Type |
|----------|-------------|
| `etf.integer` | `SMALL_INTEGER_EXT`, `INTEGER_EXT`, `SMALL_BIG_EXT`, `LARGE_BIG_EXT` as appropriate |
| `etf.float` | `NEW_FLOAT_EXT` |
| `etf.string` | `STRING_EXT`
| `etf.binary` | `BINARY_EXT`
| `etf.atom` | `SMALL_ATOM_UTF8_EXT` or `ATOM_UTF8_EXT` |
| `etf.tuple` | `TUPLE_EXT` |
| `etf.list` | `LIST_EXT` |
| `etf.map` | `MAP_EXT` |
| `etf.port` | `NEW_PORT_EXT` or `V4_PORT_EXT` |
| `etf.pid` | `NEW_PID_EXT` |
| `etf.export` | `EXPORT_EXT`|
| `etf.reference` | `NEWER_REFERENCE_EXT` |

The `etf.integer` type will encoded to the smallest-possible integer. So, a `integer`
in the range of an 8-bit unsigned integer will be encoded as a `SMALL_INTEGER_EXT` value,
a `integer` in the range of a 32-bit signed integer will be encoded as an `INTEGER_EXT` value,
and so on.

Using these userdata with a custom `value_map` function allows precise control over
mapping. For example, if you want to use Atom types for all table keys, you could do:

```lua
local function value_map(val, is_key)
  if is_key then
    return etf.atom(val)
  end
  return val
end

local encoder = etf.encoder({value_map = value_map })
local binary = encoder:encode({ a = 1, b = 2 })
-- will return a MAP_EXT with atom keys and integer values
```

## Full listing of fields in the `etf` module:

### Decoding Functions

* `decoder` - function that returns a `decoder` userdata.
* `decode` - convenience function to decode without creating a decoder.

### Encoding Functions

* `encoder` - function that returns an `encoder` userdata.
* `encode` - convenience function to encode without creating an encoder.

### Userdata-creating Functions

#### String-like types

* `atom` - function that returns an `atom` userdata (requires a string).
* `binary` - function that returns a `binary` userdata (requires a string).
* `string` - a function that returns a `string` userdata (requires a string).

#### Integer types

* `integer` - function that returns a `integer` userdata (accepts a number, string, or none).

#### Float types

* `float` - function that returns a `float` userdata (accepts a number, string, or none).

#### Table-like types

* `list` - function that returns a `list` userdata (optionally accepts a table).
* `map` - function that returns a `map` userdata (optionally accepts a table).
* `tuple` - a function that returns a `tuple` userdata (optionally accepts a table).

#### Other tpyes

* `export` - function that returns an `export` userdata (requires a table matching `EXPORT_EXT` above).
* `pid` - a function that returns a `pid` userdata (requires a table matching `PID_EXT` above).
* `port` - a function that returns a `port` userdata (requires a table matching `PORT_EXT` above).
* `reference` - a function that returns a `reference` userdata (requires a table matching `REFERENCE_EXT` above).

### Pre-created Userdatas

* `maxinteger` - a `integer` value representing the maximum integer that can be represented by Lua natively.
* `mininteger` - a `integer` value representing the minimum integer that can be represented by Lua natively.
* `null` - an `atom` that represents a `nil` atom.

### Metatables

* `atom_mt` - the atom userdata's metatable.
* `integer_mt` - the `integer` userdata's metatable.
* `float_mt` - the `float` userdata's metatable.
* `binary_mt` - the `integer` userdata's metatable.
* `decoder_131_mt` - the `decoder` userdata's metatable.
* `encoder_131_mt` - the `encoder` userdata's metatable.
* `export_mt` - the `export` userdata's metatable.
* `fun_mt` - the `fun` userdata's metatable.
* `list_mt` - the `list` userdata's metatable.
* `map_mt` - the `map` userdata's metatable.
* `new_fun_mt` - the `new_fun` userdata's metatable.
* `pid_mt` - the `pid` userdata's metatable.
* `port_mt` - the `port` userdata's metatable.
* `reference_mt	table` - the `reference` userdata's metatable.
* `string_mt` - a `string` userdata's metatable.
* `tuple_mt` - the `tuple` userdata's metatable.

### Version Info

* `_VERSION` - the module version as a string.
* `_VERSION_MAJOR` - the module's major version as a number.
* `_VERSION_MINOR` - the module's minor version as a number.
* `_VERSION_PATCH` - the module's patch version as a number.

### Misc

* `numsize` - the size of a Lua number, in bytes.

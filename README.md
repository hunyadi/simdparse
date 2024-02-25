# simdparse: High-speed parser with vector instructions

This header-only C++ library parses character strings into objects with efficient storage, including

* date and time objects, consisting of year, month, day, hour, minute, second and fractional parts (millisecond, microsecond and nanosecond),
* IPv4 and IPv6 addresses,
* UUIDs.

Parsing employs a single instruction multiple data (SIMD) approach, operating on parts of the input string in parallel.

## Technical background

Internally, the implementation uses AVX2 vector instructions (intrinsics) to

* parse RFC 3339 date-time strings into C++ `datetime` objects (consisting of year, month, day, hour, minute, second and fractional part), and
* parse RFC 4122 UUID strings or 32-digit hexadecimal strings into C++ `uuid` objects (stored internally as a 16-byte array).

For parsing IPv4 and IPv6 addresses, the parser calls the C function [inet_pton](https://man7.org/linux/man-pages/man3/inet_pton.3.html) in libc or Windows Sockets (WinSock2).

## Usage

Parse an RFC 3339 date-time string into a date-time object:

```cpp
#include <simdparse/datetime.hpp>
#include <simdparse/parse.hpp>
// ...

using namespace simdparse;

std::string_view str = "1984-10-24 23:59:59.123Z";
datetime obj;
if (parse(obj, str)) {
   // success
} else {
   // handle error
}
```

Parse a string into an object, triggering an exception on failure:

```cpp
try {
   auto obj = parse<datetime>(str);
} catch (parse_error&) {
   // handle error
}
```

## Compiling

This is a header-only library. C++17 or later is required.

You should enable the AVX2 instruction set to make full use of the library capabilities:

* `-mavx2` with Clang and GCC
* `/arch:AVX2` with MSVC

The code is looking at whether the macro `__AVX2__` is defined.

## Supported formats

### Date-time format

Date-time strings with `UTC` designator:

```
YYYY-MM-DDThh:mm:ss UTC
```

Date-time strings with `UTC` suffix *Zulu*:

```
YYYY-MM-DDThh:mm:ssZ
YYYY-MM-DDThh:mm:ss.fffZ
YYYY-MM-DDThh:mm:ss.ffffffZ
YYYY-MM-DDThh:mm:ss.fffffffffZ
```

Date-time strings with time zone offset:

```
YYYY-MM-DDThh:mm:ss+hh:mm
YYYY-MM-DDThh:mm:ss.fff+hh:mm
YYYY-MM-DDThh:mm:ss.ffffff+hh:mm
YYYY-MM-DDThh:mm:ss.fffffffff+hh:mm
```

Naive date-time strings without time zone designator:

```
YYYY-MM-DDThh:mm:ss
YYYY-MM-DDThh:mm:ss.fff
YYYY-MM-DDThh:mm:ss.ffffff
YYYY-MM-DDThh:mm:ss.fffffffff
```

The character `T` may be substituted with a space character.

Fractional digits usually give millisecond (3-digit), microsecond (6-digit) or nanosecond (9-digit) precision. However, any number of fractional digits are supported between 0 and 9. The fractional part separator of `.` must be omitted when no fractional digits are present.

### Date format

```
YYYY-MM-DD
```

### Time format

```
hh:mm:ssZ
hh:mm:ss.fffZ
hh:mm:ss.ffffffZ
hh:mm:ss.fffffffffZ
```

### UUID format

```
xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
```

## Implementation

Parsing date-time strings starts by copying the string into an internal buffer of 32 bytes (the character `_` indicates an unspecified blank value):

```
YYYY-MM-DD hh:mm:ss.fffffffff___
1984-10-24 23:59:59.123456789___
```

The buffer is then read into a `__m256i` AVX2 register.

Next, each character is checked against a lower bound and an upper bound, which depend on the character position:

* `0` or `1` for first digit of month,
* `0` to `3` for first digit of day,
* `0` to `2` for first digit of hour,
* `0` to `5` for first digit of minute and second,
* `0` to `9` for other digits (e.g. year, second digit of hour, or fractional part),
* exact match for separator characters `-`, `:` and `.`

If any character is outside the bounds, parsing fails.

If all constraints match, the numeric value of the ASCII character `0` is subtracted from each byte. (The same is accomplished with a bitwise *AND* on each byte against `0x0f`.) This makes each position hold the numeric value the digit corresponds to.

Next, digits are shuffled to pack parts together:

```
YYYY-MM-DD hh:mm | :ss.fffffffff___
YYYYMMDDhhmm____ | ss_fff_fff_fff__
```

The character `_` indicates an unspecified blank value, and `|` indicates a lane boundary across which no shuffling is possible.

Then, each byte in the register is multiplied by a weight, and neighboring values are added to form 16-bit integers. Take the date-time string `1984-10-24 23:59:59.123456789` as an example:

```
1 9 8 4 1 0 2 4 2 3 5 9 _ _ _ _ | 5 9 _ 1 2 3 _ 4 5 6 _ 7 8 9 _ _
```

Let's consider the first lane. Here, bytes are weighted by 10 and 1:

```
   1   9   8   4   1   0   2   4   2   3   5   9   _   _   _   _
  10   1  10   1  10   1  10   1  10   1  10   1   0   0   0   0
```

When multiplied and added, it yields:

```
   0  19   0  84   0  10   0  24   0  23   0  59   0   0   0   0
```

We see that the month, day, hour and minute values can be read directly from the register, and the year value can be obtained as a combination of two values, with the first to be  scaled by 100.

Let's consider the second lane. Here, bytes are weighted by 100, 10 and 1:

```
   5   9   _   1   2   3   _   4   5   6   _   7   8   9   _   _
  10   1   0 100  10   1   0 100  10   1   0 100  10   1   0   0
```

When multiplied and added, it produces the following output:

```
   0  59   0 100   0  23   0 400   0  56   0 700   0  89   0   0
```

We see that the number of seconds can be read from the register, and millisecond, microsecond and nanosecond parts can be obtained by adding two numbers.

# simdparse: High-speed parser with vector instructions

This header-only C++ library parses character strings into objects with efficient storage, including

* long integers,
* date and time objects, consisting of year, month, day, hour, minute, second and fractional parts (millisecond, microsecond and nanosecond),
* IPv4 and IPv6 addresses,
* UUIDs,
* Base64-encoded strings.

Parsing employs a single instruction multiple data (SIMD) approach, operating on parts of the input string in parallel.

## Technical background

Internally, the implementation uses AVX2 vector instructions (intrinsics) to parse

* strings of decimal and hexadecimal digits into a C++ `unsigned long long`,
* RFC 3339 date-time strings into C++ `datetime` objects (consisting of year, month, day, hour, minute, second and fractional part),
* RFC 4122 UUID strings or 32-digit hexadecimal strings into C++ `uuid` objects (stored internally as a 16-byte array), and
* RFC 4648 Base64 strings encoded with a safe alphabet for URLs and file names into objects of type `vector<byte>`.

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

### Integers

Strings of decimal digits (without sign) or hexadecimal digits that fit into a C++ `unsigned long long` (when parsed).

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
{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}
xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
```

## Implementation

### Decimal strings

To parse integers, we first copy the string of digits, right-aligned, into an internal buffer of 16 bytes pre-populated with `0` digits. The following ASCII character data is stored:

```
'0' ... '0' '1' '2' '3' '4' '5' '6' '7' '8' '9'
```

The buffer is then read into a `__m128i` register.

Next, each character is checked against a lower bound of `'0'` and an upper bound of `'9'`. If any character is outside the bounds, parsing fails.

Then, each character is converted into their numeric 8-bit equivalent:

```
 0  0  0  0  0  0  0  1  2  3  4  5  6  7  8  9
```

With the help of a weighting vector, we multiply each odd position with 10 (and leave each even position as-is), and add members of consecutive 8-bit integer pairs to produce 16-bit integers:

```
 0  0  0  0  0  0  0  1  2  3  4  5  6  7  8  9
10  1 10  1 10  1 10  1 10  1 10  1 10  1 10  1
-----------------------------------------------
    0     0     0     1    23    45    67    89
```

Next, we repeat the procedure, merging 16-bit integers into 32-bit integers:

```
    0     0     0     1    23    45    67    89
  100     1   100     1   100     1   100     1
-----------------------------------------------
          0           1        2345        6789
```

Unfortunately, there are no AVX2 instructions to multiply and horizontally add 32-bit integers. As a workaround, we pack 32-bit integers into 16-bit slots with saturation. However, since all integers are within the 16-bit range, there is no data loss. Finally, we repeat the previous step but with different scale factors, merging 16-bit integers into 32-bit integers:

```
    0     1  2345  6789     0     0     0     0
10000     1 10000     1     0     0     0     0
-----------------------------------------------
          1    23456789           0           0
```

Lastly, we extract the two 32-bit integers and combine them into an unsigned long integer, scaling each component with the weight appropriate to their ordinal position. In our example, we obtain the number `123456789`.

### Date-time strings

Parsing date-time strings starts by copying the string into an internal buffer of 32 bytes (the character `_` indicates an unspecified blank value):

```
YYYY-MM-DD hh:mm:ss.fffffffff___
1984-10-24 23:59:59.123456789___
```

If there are fewer than 9 fractional digits, the extra places are filled with `'0'`.

The buffer is then read into a `__m256i` AVX2 register.

Next, each character is checked against a lower bound and an upper bound, which depend on the character position:

* `'0'` or `'1'` for first digit of month,
* `'0'` to `'3'` for first digit of day,
* `'0'` to `'2'` for first digit of hour,
* `'0'` to `'5'` for first digit of minute and second,
* `'0'` to `'9'` for other digits (e.g. year, second digit of hour, or fractional part),
* exact match for separator characters `'-'`, `':'` and `'.'`

If any character is outside the bounds, parsing fails.

If all constraints match, the numeric value of the ASCII character `'0'` is subtracted from each byte. (The same is accomplished with a bitwise *AND* on each byte against `0x0f`.) This makes each position hold the numeric value the digit corresponds to.

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

### Hexadecimal strings

The first step in parsing a string of hexadecimal digits is converting their hexadecimal representation into their numerical value. Consider the following example with characters right-aligned in a buffer of 16 digits:

```
'0' '1' '2' '3' '4' '5' '6' '7' '8' '9' 'a' 'b' 'c' 'd' 'e' 'f'
30  31  32  33  34  35  36  37  38  39  61  62  63  64  65  66
```

We create three masks by comparing each digit to a range of permitted values. One mask filters decimal digits `'0'...'9'`, another filters uppercase letters `'A'...'F'` and yet another filters lowercase letters `'a'...'f'`.

We set a minimum value for the smallest element in each group: 48 (`0x30` = `'0'`) for decimal digits, 65 (`0x41` = `'A'`) for uppercase letters, and 97 (`0x61` = `'a'`) for lowercase letters. We subtract the corresponding minimum value from each character as matched by the mask.

In our example, this will yield the following, with each number expressed in hexadecimal:

```
30 31 32 33 34 35 36 37 38 39 61 62 63 64 65 66
 0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
```

Next, we rearrange the numeric values such that they correspond to little-endian byte order, and separate odd and even positions into 32-bit groups:

```
 f  d  b  9 |  e  c  a  8 |  7  5  3  1 |  6  4  2  0
```

This seemingly peculiar arrangement will become clear when we left-shift every second 32-bit word by 4:

```
 f  d  b  9 | e0 c0 a0 80 | 7  5  3  1 | 60 40 20  0
```

We can now see that if we horizontally add (as if with the operator `+`) consecutive 32-bit words, we recover the value represented by the first and second 32 bits of the original 64-bit number:

```
ef cd ab 89 | 67 45 23  1
```

Note that this is little-endian storage. The integer value is understood as the reverse order of bytes:

```
 1 23 45 67 | 89 ab cd ef
```

In other words, we have obtained the numeric value represented by the original hexadecimal string.

### Base64 with URL-safe alphabet

Base64 decoding with an alphabet safe both URLs and file names follows the [vector lookup algorithm](http://0x80.pl/notesen/2016-01-17-sse-base64-decoding.html#vector-lookup-pshufb-with-bitmask-new) described by Wojciech Muła. The main difference is that while in regular Base64, characters `+` and `/` occupy the same high nibble, in [modified Base64](https://datatracker.ietf.org/doc/html/rfc4648#section-5), character `-` has its own high nibble, whereas `_` shares the high nibble with uppercase letters. As such, SIMD comparison for equality is done on `_` instead of `/`. For extracting bytes, we use the [multipy-add variant](http://0x80.pl/notesen/2016-01-17-sse-base64-decoding.html#pack-multiply-add-variant-update). Modified Base64 does not have the padding character `=`. As opposed to the algorithms by Wojciech Muła, we use 32-byte AVX2 instructions (`__m256i`) with shuffle on two 16-byte lanes, not their 16-byte variants (`__m128i`).

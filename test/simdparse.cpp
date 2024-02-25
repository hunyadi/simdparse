#include <simdparse/datetime.hpp>
#include <simdparse/ipaddr.hpp>
#include <simdparse/uuid.hpp>
#include <simdparse/parse.hpp>

static bool example1()
{
    using namespace simdparse;
    std::string str = "1984-10-24 23:59:59.123";
    datetime obj;
    if (parse(obj, str)) {
        return true;
    } else {
        return false;
    }
}

static bool example2()
{
    using namespace simdparse;
    std::string str = "1984-10-24 23:59:59.123";
    try {
        auto obj = parse<datetime>(str);
    } catch (parse_error&) {
        return false;
    }
    return true;
}

int main(int /*argc*/, char* /*argv*/[])
{
    using simdparse::check_parse;

    using simdparse::date;
    constexpr auto d = date(1984, 1, 1);
    check_parse("1984-01-01", d);
    check_parse("2024-10-24", date(2024, 10, 24));

    using simdparse::datetime;
    using simdparse::tzoffset;
    constexpr auto dt = datetime(1984, 10, 24, 23, 59, 59, tzoffset(tzoffset::east, 1, 0));
    check_parse("1984-10-24 23:59:59+01:00", dt);

    // standard fractional part lengths
    check_parse("1984-01-01 01:02:03.000456789+00:00", datetime(1984, 1, 1, 1, 2, 3, 456789));
    check_parse("1984-10-24 23:59:59.123456789+00:00", datetime(1984, 10, 24, 23, 59, 59, 123456789));
    check_parse("1984-01-01 01:02:03.000456+00:00", datetime(1984, 1, 1, 1, 2, 3, 456000));
    check_parse("1984-10-24 23:59:59.123456+00:00", datetime(1984, 10, 24, 23, 59, 59, 123456000));
    check_parse("1984-01-01 01:02:03.456+00:00", datetime(1984, 1, 1, 1, 2, 3, 456000000));
    check_parse("1984-10-24 01:02:03+00:00", datetime(1984, 10, 24, 1, 2, 3));

    // nonstandard fractional part lengths
    check_parse("1984-01-01 01:02:03.0004567+00:00", datetime(1984, 1, 1, 1, 2, 3, 456700));
    check_parse("1984-10-24 23:59:59.1234567+00:00", datetime(1984, 10, 24, 23, 59, 59, 123456700));
    check_parse("1984-01-01 01:02:03.0004+00:00", datetime(1984, 1, 1, 1, 2, 3, 400000));
    check_parse("1984-10-24 23:59:59.1234+00:00", datetime(1984, 10, 24, 23, 59, 59, 123400000));
    check_parse("1984-01-01 01:02:03.4+00:00", datetime(1984, 1, 1, 1, 2, 3, 400000000));

    // time zone offset
    check_parse("1984-01-01 23:59:59.123-11:30", datetime(1984, 1, 1, 23, 59, 59, 123000000, tzoffset(tzoffset::west, 11, 30)));
    check_parse("1984-10-24 01:15:00+02:30", datetime(1984, 10, 24, 1, 15, 0, 0, tzoffset(tzoffset::east, 2, 30)));

    // time zone "Zulu"
    check_parse("1984-01-01 01:02:03.000456789Z", datetime(1984, 1, 1, 1, 2, 3, 456789));
    check_parse("1984-10-24 23:59:59.123456789Z", datetime(1984, 10, 24, 23, 59, 59, 123456789));
    check_parse("1984-01-01 01:02:03.000456Z", datetime(1984, 1, 1, 1, 2, 3, 456000));
    check_parse("1984-01-01 01:02:03.456Z", datetime(1984, 1, 1, 1, 2, 3, 456000000));
    check_parse("1984-01-01 01:02:03Z", datetime(1984, 1, 1, 1, 2, 3, 0));

    // naive date-time string (no time zone designator)
    check_parse("1984-01-01 01:02:03.000456789", datetime(1984, 1, 1, 1, 2, 3, 456789));
    check_parse("1984-01-01 01:02:03.000456", datetime(1984, 1, 1, 1, 2, 3, 456000));
    check_parse("1984-01-01 01:02:03.456", datetime(1984, 1, 1, 1, 2, 3, 456000000));
    check_parse("1984-01-01 01:02:03", datetime(1984, 1, 1, 1, 2, 3, 0));

    // time zone designator "UTC"
    check_parse("1984-10-24 23:59:59.123456 UTC", datetime(1984, 10, 24, 23, 59, 59, 123456000));
    check_parse("1984-10-24 23:59:59.123 UTC", datetime(1984, 10, 24, 23, 59, 59, 123000000));
    check_parse("1984-01-01 01:02:03 UTC", datetime(1984, 1, 1, 1, 2, 3, 0));

    // extreme year values
    check_parse("0001-01-01 00:00:00", datetime(1, 1, 1, 0, 0, 0, 0));
    check_parse("9999-12-31 23:59:59.999999999Z", (datetime::max)());

    using simdparse::microtime;

    // nanosecond truncation
    check_parse("1984-01-01 01:02:03.000456789Z", microtime(1984, 1, 1, 1, 2, 3, 456));
    check_parse("1984-10-24 23:59:59.123456789Z", microtime(1984, 10, 24, 23, 59, 59, 123456));

    // preserve microsecond precision
    check_parse("1984-01-01 01:02:03.000456Z", microtime(1984, 1, 1, 1, 2, 3, 456));
    check_parse("1984-10-24 23:59:59.123456Z", microtime(1984, 10, 24, 23, 59, 59, 123456));

    // add extra precision
    check_parse("1984-01-01 01:02:03.123Z", microtime(1984, 1, 1, 1, 2, 3, 123000));

    // time zone adjustments
    check_parse("1984-10-24 23:59:59.123", microtime(1984, 10, 24, 23, 59, 59, 123000));
    check_parse("1984-01-01 13:02:04.567-11:30", microtime(1984, 1, 2, 0, 32, 4, 567000));
    check_parse("1984-01-01 01:15:00.000+02:30", microtime(1983, 12, 31, 22, 45, 0, 0));

    // extreme year values
    check_parse("1000-01-01 23:59:59", microtime(1000, 1, 1, 23, 59, 59));
    check_parse("9999-12-31 23:59:59", microtime(9999, 12, 31, 23, 59, 59));

    using simdparse::uuid;
    check_parse("f81d4fae-7dec-11d0-a765-00a0c91e6bf6", uuid({ 0xf8, 0x1d, 0x4f, 0xae, 0x7d, 0xec, 0x11, 0xd0, 0xa7, 0x65, 0x00, 0xa0, 0xc9, 0x1e, 0x6b, 0xf6 }));
    check_parse("f81d4fae7dec11d0a76500a0c91e6bf6", uuid({ 0xf8, 0x1d, 0x4f, 0xae, 0x7d, 0xec, 0x11, 0xd0, 0xa7, 0x65, 0x00, 0xa0, 0xc9, 0x1e, 0x6b, 0xf6 }));

    // test code examples
    if (!example1() || !example2()) {
        return 1;
    }

    return 0;
}

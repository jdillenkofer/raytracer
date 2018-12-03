#include "color.h"

Color color_add(Color a, Color b) {
    ASSERT_COLOR_VALUE_NOT_NAN(a);
    ASSERT_COLOR_VALUE_NOT_NAN(b);
    Color result = {
        a.r + b.r,
        a.g + b.g,
        a.b + b.b,
        a.a + b.a
    };
    return result;
}

Color color_mul(Color a, double b) {
    ASSERT_COLOR_VALUE_NOT_NAN(a);
    assert(!isnan(b));
    a.r *= b;
    a.g *= b;
    a.b *= b;
    a.a *= b;
    return a;
}

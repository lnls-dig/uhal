/* some of these tests were developed using examples from the si57x
 * datasheet, which can be found in
 * <https://www.skyworksinc.com/-/media/skyworks/sl/documents/public/data-sheets/si570-71.pdf>
 *
 * the datasheet version is 1.6 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "si57x_util.h"

using namespace Catch::Matchers;

const double precision = 0.000001;

/* based on example in section 3.2 */
TEST_CASE("calc_fxtal", "[si57x-test]")
{
    si57x_parameters params;
    params.fxtal = 0; /* so we don't use the default value */
    params.fstartup = 156250000.;
    params.rfreq = 0x2BC011EB8;
    params.n1 = 7; /* val=8 */
    params.hs_div = 0; /* val=4 */

    params.calc_fxtal();
    CHECK_THAT(params.fxtal, WithinRel(114285000., precision));

    /* roundtrip */
    CHECK_THAT(params.get_freq(), WithinRel(params.fstartup, precision));
}

/* based on example in section 3.2 */
TEST_CASE("get_freq", "[si57x-test]")
{
    si57x_parameters params;
    params.fxtal = 114285000;
    params.rfreq = 0x2D1E12788;
    params.n1 = 7;
    params.hs_div = 0;

    CHECK_THAT(params.get_freq(), WithinRel(161132812, precision));
}

/* based on scripts/si57-clk-calc.py from afc-gw
 * https://github.com/lnls-dig/afc-gw/blob/eb03f2a6b4653d1831e80d305d0977ba4d6597b6/scripts/si57-clk-calc.py */
TEST_CASE("set_freq success", "[si57x-test]")
{
    si57x_parameters params;

    CHECK(params.set_freq(125000000));
    CHECK(params.rfreq == 0x0302013b65);
    CHECK(params.hs_div == 7);
    CHECK(params.n1 == 3);
}

TEST_CASE("set_freq failure", "[si57x-test]")
{
    si57x_parameters params;

    CHECK_FALSE(params.set_freq(1000000));
}

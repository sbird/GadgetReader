#define BOOST_TEST_DYN_LINK

#define BOOST_TEST_MODULE GadgetReader test
#include "gadgetreader.hpp"
#include <boost/test/unit_test.hpp>
#include <boost/test/test_tools.hpp>

using namespace GadgetReader;

//Check the gadget header has the right size
BOOST_AUTO_TEST_CASE(header_size)
{
        BOOST_REQUIRE(sizeof(gadget_header) == 256);
}

//Check we can 
BOOST_AUTO_TEST_CASE(constructor)
{
        GSnap snap("test_g2_snap");
}


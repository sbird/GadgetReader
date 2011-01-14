/* Copyright (c) 2010, Simeon Bird <spb41@cam.ac.uk>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. */
#define BOOST_TEST_DYN_LINK

/** \file
 * Test suite using boost::test*/
#define BOOST_TEST_MODULE GadgetReader
#include "gadgetreader.hpp"
#include <boost/test/unit_test.hpp>
#include <boost/test/test_tools.hpp>
#include <cmath>

using namespace GadgetReader;

#define FLOATS_NEAR_TO(x,y) \
        BOOST_CHECK_MESSAGE( fabs((x) - (y)) <= std::max(fabs(x),fabs(y))/1e6,(x)<<" is not close to "<<(y))
//Check the gadget header has the right size
BOOST_AUTO_TEST_CASE(header_size)
{
        BOOST_REQUIRE_EQUAL(sizeof(gadget_header), 256);
}

//Check we can construct a thing, and that thing is a bad thing
BOOST_AUTO_TEST_CASE(construct_no_file)
{
        GSnap snap("should_not_exist_at_all_file",false);
        BOOST_CHECK_EQUAL(snap.GetNumFiles(), 0);
}

//Check we can construct a thing and find some files.
BOOST_AUTO_TEST_CASE(constructor_sane)
{
        GSnap snap("test_g2_snap",false);
        BOOST_CHECK_EQUAL( snap.GetFileName(), "test_g2_snap");
        BOOST_CHECK_EQUAL( snap.GetNumFiles() , 2 );
        BOOST_CHECK_EQUAL( snap.GetFormat() , 0);
}

//Check we have been able to find the right numbers of particles
BOOST_AUTO_TEST_CASE(get_num_particles)
{
        GSnap snap("test_g2_snap",false);
        BOOST_CHECK_EQUAL(snap.GetNpart(0), 4039);
        BOOST_CHECK_EQUAL(snap.GetNpart(1), 4096);
        BOOST_CHECK_EQUAL(snap.GetNpart(2) , 0);
        BOOST_CHECK_EQUAL(snap.GetNpart(3) , 0);
        BOOST_CHECK_EQUAL(snap.GetNpart(4) , 57);
        BOOST_CHECK_EQUAL(snap.GetNpart(5) , 0);
        BOOST_CHECK_EQUAL(snap.GetNpart(55) , 0);
}
//Check we have been able to find the right numbers of particles
BOOST_AUTO_TEST_CASE(found_blocks)
{
        GSnap snap("test_g2_snap",false);
        BOOST_CHECK(snap.IsBlock("POS "));
        BOOST_CHECK(snap.IsBlock("VEL "));
        BOOST_CHECK(snap.IsBlock("MASS"));
        BOOST_CHECK(snap.IsBlock("ID  "));
        BOOST_CHECK(snap.IsBlock("NH  "));
        BOOST_CHECK(snap.IsBlock("NHE "));
        BOOST_CHECK(snap.IsBlock("NHEP"));
        BOOST_CHECK(snap.IsBlock("NHEQ"));
        BOOST_CHECK(snap.IsBlock("NHP "));
        BOOST_CHECK(snap.IsBlock("RHO "));
        BOOST_CHECK(snap.IsBlock("SFR "));
        BOOST_CHECK(snap.IsBlock("U   "));
        BOOST_CHECK(snap.IsBlock("HSML"));
}

BOOST_AUTO_TEST_CASE(check_block_sizes)
{
        GSnap snap("test_g2_snap",false);
        std::set<std::string> blocks=snap.GetBlocks();
        std::set<std::string>::iterator it;
        int parts[13]={4039,8192,4096,4039,4039,4039,4039,4039,8192,4039,4039,4039,8192};
        int bytes[13]={16156,32768,16384,16156,16156,16156,16156,16156,98304,16156,16156,16156,98304};
        int i;
        for(it=blocks.begin(),i=0 ; it != blocks.end(),i<13; it++,i++){
                BOOST_CHECK_EQUAL(snap.GetBlockParts(*it),parts[i]);
                BOOST_CHECK_EQUAL(snap.GetBlockSize(*it),bytes[i]);
        }
}


//Check we have got a header correctly
BOOST_AUTO_TEST_CASE(header_correct)
{
        GSnap snap("test_g2_snap",false);
        //Set up the test header
        gadget_header head=snap.GetHeader();
        uint32_t npart[N_TYPE]= {1994, 2050, 0, 0, 39, 0};
        double mass[N_TYPE] = {0.000000, 0.0406161, 0.000000, 0.000000, 0.000000, 0.000000};
        int32_t npartTotal[N_TYPE]= {4039, 4096, 0, 0, 57, 0};
        for(int i=0; i<N_TYPE;i++){
                BOOST_CHECK_EQUAL(npart[i], head.npart[i]);
                BOOST_CHECK_EQUAL(npartTotal[i], head.npartTotal[i]);
                FLOATS_NEAR_TO(mass[i], head.mass[i]);
        }
        FLOATS_NEAR_TO(head.time,0.277778);
        FLOATS_NEAR_TO(head.redshift,2.6);
        BOOST_CHECK_EQUAL(head.flag_sfr,1);
        BOOST_CHECK_EQUAL(head.flag_feedback,1);
        BOOST_CHECK_EQUAL(head.flag_cooling,1);
        BOOST_CHECK_EQUAL(head.num_files,2);
        BOOST_CHECK_EQUAL(head.BoxSize,3000);
        FLOATS_NEAR_TO(head.Omega0,0.2669);
        FLOATS_NEAR_TO(head.OmegaLambda,0.7331);
        FLOATS_NEAR_TO(head.HubbleParam,0.71);
        BOOST_CHECK_EQUAL(head.flag_stellarage,0);
        BOOST_CHECK_EQUAL(head.flag_metals,0);
}

//Check we can actually get some block data
BOOST_AUTO_TEST_CASE(get_blocks)
{
        GSnap snap("test_g2_snap",false);
        float *block=NULL;
        block=(float *)calloc(snap.GetBlockSize("POS ")/sizeof(float),sizeof(float));
        BOOST_REQUIRE_MESSAGE(block != NULL, "Failed to allocate "<<snap.GetBlockSize("POS ")<<" bytes for POS.");
        //First read a single particle from the first file
        BOOST_CHECK_EQUAL(snap.GetBlock("POS ",block,1,100,0),1);
        BOOST_CHECK_EQUAL(block[3],0);
        FLOATS_NEAR_TO(block[0],568.540833);
        //Then read a single particle from the second file
        BOOST_CHECK_EQUAL(snap.GetBlock("POS ",block,1,3100,0),1);
        //A single particle is 3 floats for POS.
        BOOST_CHECK_EQUAL(block[3],0);
        FLOATS_NEAR_TO(block[1],1998.67651);
        //Then read all the particles
        BOOST_CHECK_EQUAL(snap.GetBlock("POS ",block,snap.GetBlockParts("POS "),0,0),8192);
        BOOST_CHECK(block[snap.GetBlockParts("POS ")-1]!=0);
        //Read a particle that turns out not to be there
        BOOST_CHECK_EQUAL(snap.GetBlock("HSML",block,1,0,1),0);
        //Test the skip_type by reading mass for the star type.
        BOOST_CHECK_EQUAL(snap.GetBlock("MASS",block,snap.GetNpart(STARS_TYPE),0,1),snap.GetNpart(STARS_TYPE));
        FLOATS_NEAR_TO(block[10],0.00821469352);
        free(block);
}

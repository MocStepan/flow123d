/*
 * material_dispatch_test.cpp
 *
 *  Created on: Nov 27, 2012
 *      Author: jb
 */

#include <gtest/gtest.h>
#include "mesh/region.hh"

TEST(Region, all) {

    {
    Region r=Region::db().add_region(1001,"top", 2,true);
    EXPECT_EQ(0, r.idx() );
    EXPECT_TRUE( r.is_boundary() );
    EXPECT_EQ(0, r.boundary_idx() );
    EXPECT_TRUE( r.is_valid() );
    EXPECT_EQ(1001, r.id());
    EXPECT_EQ("top", r.label());
    EXPECT_EQ(2, r.dim());

    EXPECT_EQ(0, Region::db().add_region(1001,"top", 2,true).idx() );
    }

    {
    Region r=Region::db().add_region(1002,"inside 1", 3, false);
    EXPECT_EQ(1, r.idx() );
    EXPECT_EQ(0, r.bulk_idx() );
    EXPECT_EQ("inside 1", r.label() );
    EXPECT_EQ(1002, r.id() );
    EXPECT_EQ(3, r.dim());
    }

    {
        Region a=Region::db().find_label("top");
        Region b=Region::db().find_id(1001);
        EXPECT_EQ(a,b);
        Region c=Region::db().find_id(1002);
        EXPECT_TRUE(a!=c);

        EXPECT_FALSE( Region::db().find_id(1007).is_valid() );
    }

    {
    Region r=Region::db().add_region(1003, 3);
    EXPECT_EQ(3, r.idx() );
    EXPECT_EQ(1, r.bulk_idx() );
    EXPECT_EQ("region_1003", r.label() );
    }

    {
    Region r=Region::db().add_region(1004,"bottom", 2, true);
    EXPECT_EQ(2, r.idx() );
    EXPECT_EQ(1, r.boundary_idx() );
    EXPECT_EQ("bottom", r.label() );
    EXPECT_EQ(1004, r.id() );
    }

    Region::db().add_region(1005,"side", 2, true);
    EXPECT_THROW( Region::db().add_region(1005,"new", 3, false) , RegionDB::ExcInconsistentAdd);
    EXPECT_THROW( Region::db().add_region(1001,"bottom", 2, false) , RegionDB::ExcInconsistentAdd);

    // Region::db().close(); // close should be called automatically at first call to any size method.

    EXPECT_EQ(6, Region::db().size());
    EXPECT_EQ(3, Region::db().boundary_size());
    EXPECT_EQ(2, Region::db().bulk_size());

    EXPECT_DEATH( { Region::db().add_region(1006,"side_", 2, true);}, "Can not add to closed region DB.");

}


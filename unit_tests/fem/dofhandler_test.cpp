#define TEST_USE_PETSC
#include <flow_gtest_mpi.hh>
#include <cmath>
#include "fem/fe_p.hh"
#include "fem/fe_rt.hh"
#include "fem/fe_system.hh"
#include "mesh/mesh.h"
#include <mesh_constructor.hh>
#include "fem/dofhandler.hh"
#include "fem/dh_cell_accessor.hh"





TEST(DOFHandler, test_all) {
  
  // distribute dofs for continuous P1 finite element.
  // The test checks that the dofs are
  // shared by adjacent elements
  // except for the dofs lying on line 1
  // without the center point 3.
  {
    // simple mesh consisting of 4 triangles and 1 line element
    //
    //  4---------------5
    //  | \           / |
    //  |   \   5   1   |
    //  |     \   /     |
    //  |  3    3    4  |
    //  |     /   \     |
    //  |   /   2   \   |
    //  | /           \ |
    //  1---------------2
    FilePath::set_io_dirs(".",UNIT_TESTS_SRC_DIR,"",".");
    Mesh * mesh = mesh_full_constructor("{mesh_file=\"fem/small_mesh.msh\"}");
    
    FE_P<0> fe0(1);
    FE_P<1> fe1(1);
    FE_P<2> fe2(1);
    FE_P<3> fe3(1);
    std::shared_ptr<DiscreteSpace> ds = std::make_shared<EqualOrderDiscreteSpace>(mesh, &fe0, &fe1, &fe2, &fe3);
    DOFHandlerMultiDim dh(*mesh);
    dh.distribute_dofs(ds);
    
    EXPECT_EQ( 8, dh.n_global_dofs() );
    
    dh.print();
    
    std::vector<int> indices[5];
    std::vector<bool> own_elem(5, false); // hold if cell is own or ghost on process and can be evaluated (see tests)
    for ( DHCellAccessor cell : dh.local_range() )
    {
        auto elem_idx = cell.elm_idx();
        own_elem[elem_idx] = true;
        indices[elem_idx].resize(dh.max_elem_dofs());
    	cell.get_dof_indices(indices[elem_idx]);
    }

    // dof at node 1 is shared by elements 2, 3
    if (own_elem[1] & own_elem[2]) EXPECT_EQ( indices[1][0], indices[2][0] );
    
    // dof at node 2 is shared by elements 2, 4
    if (own_elem[1] & own_elem[3]) EXPECT_EQ( indices[1][1], indices[3][1] );
    
    // dof at node 3 is shared by elements 2, 3, 4, 5
    if (own_elem[4] & own_elem[2]) EXPECT_EQ( indices[4][0], indices[2][1] );
    if (own_elem[2] & own_elem[1]) EXPECT_EQ( indices[2][1], indices[1][2] );
    if (own_elem[1] & own_elem[3]) EXPECT_EQ( indices[1][2], indices[3][0] );
    
    // dof at node 3 is NOT shared by elements 1 and 5
    if (own_elem[0] & own_elem[4]) EXPECT_NE( indices[0][0], indices[4][0] );
    
    // dof at node 4 is shared by elements 3, 5
    if (own_elem[2] & own_elem[4]) EXPECT_EQ( indices[2][2], indices[4][2] );
    
    // dof at node 5 is NOT shared by elements 1, 4 and 5
    if (own_elem[4] & own_elem[0]) EXPECT_NE( indices[4][1], indices[0][1] );
    if (own_elem[4] & own_elem[3]) EXPECT_NE( indices[4][1], indices[3][2] );
    
    delete mesh;

  }

  
  
  
  
  
  // distribute dofs for continuous P1 finite element.
  // The test checks that the dofs are
  // shared by adjacent elements
  // except for the dofs separated by line elements.
  {
    // simple mesh consisting of 5 triangles and 3 line elements
    //
    //  5---------------6
    //  | \           / |
    //  |   2   7   3   |
    //  |     \   /   8 |
    //  |  5    3-------4
    //  |     /   \   6 |
    //  |   1   4   \   |
    //  | /           \ |
    //  1---------------2
    FilePath::set_io_dirs(".",UNIT_TESTS_SRC_DIR,"",".");
    Mesh * mesh = mesh_full_constructor("{mesh_file=\"fem/small_mesh_junction.msh\"}");
    
    FE_P<0> fe0(1);
    FE_P<1> fe1(1);
    FE_P<2> fe2(1);
    FE_P<3> fe3(1);
    std::shared_ptr<DiscreteSpace> ds = std::make_shared<EqualOrderDiscreteSpace>(mesh, &fe0, &fe1, &fe2, &fe3);
    DOFHandlerMultiDim dh(*mesh);
    dh.distribute_dofs(ds);
    
    EXPECT_EQ( 15, dh.n_global_dofs() );
    
    dh.print();
    
    std::vector<int> indices[8];
    std::vector<bool> own_elem(8, false); // hold if cell is own or ghost on process and can be evaluated (see tests)
    for ( DHCellAccessor cell : dh.local_range() )
    {
        auto elem_idx = cell.elm_idx();
        own_elem[elem_idx] = true;
        indices[elem_idx].resize(dh.max_elem_dofs());
        cell.get_dof_indices(indices[elem_idx]);
    }
    
    // dof at node 1 is not shared by elements 1, 4, 5
    if (own_elem[0] & own_elem[3]) EXPECT_NE( indices[0][0], indices[3][0] );
    if (own_elem[4] & own_elem[3]) EXPECT_NE( indices[4][0], indices[3][0] );
    
    // dof at node 2 is shared by elements 4, 6
    if (own_elem[3] & own_elem[5]) EXPECT_EQ( indices[3][1], indices[5][1] );
    
    // dof at node 3 is shared by elements 4, 6, 8
    if (own_elem[3] & own_elem[5]) EXPECT_EQ( indices[3][2], indices[5][0] );
    if (own_elem[3] & own_elem[7]) EXPECT_EQ( indices[3][2], indices[7][0] );
    
    // dof at node 3 is shared by elements 1, 2, 3
    if (own_elem[0] & own_elem[1]) EXPECT_EQ( indices[0][1], indices[1][0] );
    if (own_elem[1] & own_elem[2]) EXPECT_EQ( indices[1][0], indices[2][0] );
    
    // dof at node 3 is NOT shared by elements 1, 4, 5, 7
    if (own_elem[0] & own_elem[3]) EXPECT_NE( indices[0][1], indices[3][2] );
    if (own_elem[3] & own_elem[4]) EXPECT_NE( indices[3][2], indices[4][1] );
    if (own_elem[4] & own_elem[6]) EXPECT_NE( indices[4][1], indices[6][0] );
    
    // dof at node 4 is shared by elements 6, 8
    if (own_elem[5] & own_elem[7]) EXPECT_EQ( indices[5][2], indices[7][1] );
    
    // dof at node 5 is NOT shared by elements 2, 5, 7
    if (own_elem[1] & own_elem[4]) EXPECT_NE( indices[1][1], indices[4][2] );
    if (own_elem[4] & own_elem[6]) EXPECT_NE( indices[4][2], indices[6][2] );
    
    // dof at node 6 is NOT shared by elements 3, 7, 8
    if (own_elem[2] & own_elem[6]) EXPECT_NE( indices[2][1], indices[6][1] );
    if (own_elem[6] & own_elem[7]) EXPECT_NE( indices[6][1], indices[7][2] );
    
    delete mesh;

  }

}


TEST(DOFHandler, test_sub_handler)
{
    FESystem<0> fe_sys0({ std::make_shared<FE_P_disc<0> >(0),
                          std::make_shared<FE_P<0> >(0),
                          std::make_shared<FE_CR<0> >() });
    FESystem<1> fe_sys1({ std::make_shared<FE_RT0<1> >(),
                          std::make_shared<FE_P<1> >(0),
                          std::make_shared<FE_CR<1> >() });
    FESystem<2> fe_sys2({ std::make_shared<FE_RT0<2> >(),
                          std::make_shared<FE_P<2> >(0),
                          std::make_shared<FE_CR<2> >() });
    FESystem<3> fe_sys3({ std::make_shared<FE_RT0<3> >(),
                          std::make_shared<FE_P<3> >(0),
                          std::make_shared<FE_CR<3> >() });
    
    FilePath::set_io_dirs(".",UNIT_TESTS_SRC_DIR,"",".");
    Mesh * mesh = mesh_full_constructor("{mesh_file=\"fem/small_mesh_junction.msh\"}");
    std::shared_ptr<DiscreteSpace> ds = std::make_shared<EqualOrderDiscreteSpace>(mesh, &fe_sys0, &fe_sys1, &fe_sys2, &fe_sys3);
    std::shared_ptr<DOFHandlerMultiDim> dh = std::make_shared<DOFHandlerMultiDim>(*mesh);
    dh->distribute_dofs(ds);
    std::shared_ptr<SubDOFHandlerMultiDim> sub_dh = std::make_shared<SubDOFHandlerMultiDim>(dh, 0);
    std::vector<std::vector<int> > indices(mesh->n_elements(), std::vector<int>(dh->max_elem_dofs()));
    std::vector<int> sub_indices(sub_dh->max_elem_dofs());

    
    // check that dofs on sub_dh are equal to dofs on dh
    for (auto cell : dh->local_range())
        cell.get_dof_indices(indices[cell.elm_idx()]);
    for (auto cell : sub_dh->local_range())
    {
        cell.get_dof_indices(sub_indices);
        for (unsigned int i=0; i<cell.n_dofs(); i++)
            EXPECT_EQ( sub_indices[i], indices[cell.elm_idx()][i] );
    }
    delete mesh;
    
}



// distribute dofs for continuous RT0 finite element.
// The test checks that the dofs are
// shared by adjacent elements
// except for the dofs separated by line elements.
TEST(DOFHandler, test_rt)
{
    FilePath::set_io_dirs(".",UNIT_TESTS_SRC_DIR,"",".");
    Mesh * mesh = mesh_full_constructor("{mesh_file=\"fem/small_mesh_junction.msh\"}");

    FE_RT0<0> fe0;
    FE_RT0<1> fe1;
    FE_RT0<2> fe2;
    FE_RT0<3> fe3;
    std::shared_ptr<DiscreteSpace> ds = std::make_shared<EqualOrderDiscreteSpace>(mesh, &fe0, &fe1, &fe2, &fe3);
    DOFHandlerMultiDim dh(*mesh);
    dh.distribute_dofs(ds);

     EXPECT_EQ( 17, dh.n_global_dofs() );

    dh.print();

    std::vector<int> indices[8];
    std::vector<bool> own_elem(8, false); // hold if cell is own or ghost on process and can be evaluated (see tests)
    for ( DHCellAccessor cell : dh.local_range() )
    {
        auto elem_idx = cell.elm_idx();
        own_elem[elem_idx] = true;
        indices[elem_idx].resize(dh.max_elem_dofs());
        cell.get_dof_indices(indices[elem_idx]);
    }

    // dof at el. 1 side 1 equals dof at el. 2 side 0
    if (own_elem[0] & own_elem[1]) EXPECT_EQ( indices[0][1], indices[1][0] );
    
    // dof at el. 1 side 1 equals dof at el. 3 side 0
    if (own_elem[0] & own_elem[2]) EXPECT_EQ( indices[0][1], indices[2][0] );
    
    // dof at el. 4 side 2 equals dof at el. 6 side 0
    if (own_elem[3] & own_elem[5]) EXPECT_EQ( indices[3][2], indices[5][0] );
    
    // dof at el. 6 side 1 equals dof at el. 8 side 0
    if (own_elem[5] & own_elem[7]) EXPECT_EQ( indices[5][1], indices[7][0] );

    delete mesh;

}



TEST(DHAccessors, dh_cell_accessors) {
    FilePath::set_io_dirs(".",UNIT_TESTS_SRC_DIR,"",".");
    Mesh * mesh = mesh_full_constructor("{mesh_file=\"mesh/simplest_cube.msh\"}");

    FE_P<0> fe0(1);
    FE_P<1> fe1(1);
    FE_P<2> fe2(1);
    FE_P<3> fe3(1);
    std::shared_ptr<DiscreteSpace> ds = std::make_shared<EqualOrderDiscreteSpace>(mesh, &fe0, &fe1, &fe2, &fe3);
    DOFHandlerMultiDim dh(*mesh);
    dh.distribute_dofs(ds);
    auto el_ds = mesh->get_el_ds();
    unsigned int i_distr=0;

    std::vector<unsigned int> side_elm_idx, neigh_elem_idx;
    for( DHCellAccessor cell : dh.own_range() ) {
    	EXPECT_EQ( cell.elm_idx(), dh.mesh()->get_el_4_loc()[i_distr] );

    	for( DHCellSide cell_side : cell.side_range() ) {
            EXPECT_EQ( cell.elm_idx(), cell_side.side()->elem_idx() );
        	side_elm_idx.clear();
        	for( DHCellSide edge_side : cell_side.edge_sides() ) {
        		side_elm_idx.push_back( edge_side.side()->elem_idx() );
        	}
            const Edge *edg = cell_side.side()->edge();
            EXPECT_EQ( side_elm_idx.size(), edg->n_sides);
            for (int sid=0; sid<edg->n_sides; sid++) {
            	EXPECT_EQ( side_elm_idx[sid], edg->side(sid)->element().idx());
            }
        }

        neigh_elem_idx.clear();
        for( DHCellSide neighb_side : cell.neighb_sides() ) {
        	neigh_elem_idx.push_back( neighb_side.side()->elem_idx() );
        }
        EXPECT_EQ( neigh_elem_idx.size(), cell.elm()->n_neighs_vb());
        for (int nid=0; nid<cell.elm()->n_neighs_vb(); nid++) {
        	EXPECT_EQ( neigh_elem_idx[nid], cell.elm()->neigh_vb[nid]->side()->elem_idx() );
        }

    	++i_distr;
    }

    delete mesh;
}

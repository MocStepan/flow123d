
#define TEST_USE_PETSC
#define FEAL_OVERRIDE_ASSERTS
#include <flow_gtest_mpi.hh>
#include <mesh_constructor.hh>


#include "fields/generic_field.hh"
#include "mesh/mesh.h"
#include "io/msh_gmshreader.h"
#include "system/sys_profiler.hh"
#include "fields/field_flag.hh"


TEST(GenericField, all) {
    Profiler::initialize();
    FilePath::set_io_dirs(".",UNIT_TESTS_SRC_DIR,"",".");

    Mesh * mesh = mesh_constructor("{mesh_file=\"mesh/simplest_cube.msh\"}");
    GmshMeshReader gmsh_reader( mesh->mesh_file() );
    auto physical_names_data = gmsh_reader.read_physical_names_data();
    auto nodes_data = gmsh_reader.read_nodes_data();
    auto elems_data = gmsh_reader.read_elements_data();
    mesh->init_from_input(physical_names_data, nodes_data, elems_data);

    GenericField<3>::IndexField subdomain;
    subdomain.flags(FieldFlag::input_copy);
    GenericField<3>::IndexField region_id;
    region_id.flags(FieldFlag::input_copy);

    subdomain = GenericField<3>::subdomain(*mesh);
    subdomain.set_time(TimeGovernor().step(), LimitSide::right);
    // TODO: After we have support to read partitioning form the MSH file.

    region_id = GenericField<3>::region_id(*mesh);
    region_id.set_time(TimeGovernor().step(), LimitSide::right);
    FOR_ELEMENTS(mesh, ele)
    	EXPECT_EQ( ele->region().id(),
    			   region_id.value(ele->centre(), ele->element_accessor())
    			   );
}

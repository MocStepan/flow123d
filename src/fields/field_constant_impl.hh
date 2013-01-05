/*
 * field_constant_impl.hh
 *
 *  Created on: Dec 15, 2012
 *      Author: jb
 */

#ifndef FIELD_CONSTANT_IMPL_HH_
#define FIELD_CONSTANT_IMPL_HH_

#include "fields/field_constant.hh"
#include "input/input_type.hh"


//#include <boost/type_traits.hpp>

/// Implementation.

namespace it = Input::Type;

template <int spacedim, class Value>
it::Record FieldConstant<spacedim, Value>::input_type
    = it::Record("FieldConstant", FieldBase<spacedim,Value>::template_name()+" Field constant in space.")
    .derive_from(FieldBase<spacedim, Value>::input_type)
    .declare_key("value", Value::get_input_type(), it::Default::obligatory(),
                                "Value of the constant field.\n"
                                "For vector values, you can use scalar value to enter constant vector.\n"
                                "For square NxN-matrix values, you can use:\n"
                                "* vector of size N to enter diagonal matrix\n"
                                "* vector of size (N+1)*N/2 to enter symmetric matrix (upper triangle, row by row)\n"
                                "* scalar to enter multiple of the unit matrix." );


template <int spacedim, class Value>
FieldConstant<spacedim, Value>::FieldConstant(const double init_time, unsigned int n_comp)
: FieldBase<spacedim, Value>(init_time, n_comp)
{}





template <int spacedim, class Value>
void FieldConstant<spacedim, Value>::init_from_input( Input::Record rec) {
    this->value_.init_from_input( rec.val<typename Value::InputType>("value") );
}



/**
 * Returns one value in one given point. ResultType can be used to avoid some costly calculation if the result is trivial.
 */
template <int spacedim, class Value>
typename Value::return_type & FieldConstant<spacedim, Value>::value(const Point<spacedim> &p, ElementAccessor<spacedim> &elm)
{
    return this->r_value_;
}


/**
 * Returns std::vector of scalar values in several points at once.
 */
template <int spacedim, class Value>
void FieldConstant<spacedim, Value>::value_list (const std::vector< Point<spacedim> >  &point_list, ElementAccessor<spacedim> &elm,
                   std::vector<typename Value::return_type>  &value_list)
{
    ASSERT_SIZES( point_list.size(), value_list.size() );
    for(unsigned int i=0; i< point_list.size(); i++)
        value_list[i]=this->r_value_;
}









template <int spacedim, class Value>
FieldConstant<spacedim, Value>::~FieldConstant() {
}



#endif /* FIELD_CONSTANT_IMPL_HH_ */
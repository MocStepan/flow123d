/*!
 *
﻿ * Copyright (C) 2015 Technical University of Liberec.  All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 3 as published by the
 * Free Software Foundation. (http://www.gnu.org/licenses/gpl-3.0.en.html)
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 * 
 * @file    field_python.impl.hh
 * @brief   
 */

#ifndef FIELD_PYTHON_IMPL_HH_
#define FIELD_PYTHON_IMPL_HH_


#include <type_traits>
#include "fields/field_python.hh"
#include "fields/field_set.hh"
#include "fields/python_field_base.hh" // TODO check if include is necessary
#include <pybind11.h>
#include <eval.h>
#include <stl.h>

namespace py = pybind11;

/// Implementation.

namespace it = Input::Type;

FLOW123D_FORCE_LINK_IN_CHILD(field_python)



template <int spacedim, class Value>
const Input::Type::Record & FieldPython<spacedim, Value>::get_input_type()
{
    return it::Record("FieldPython", FieldAlgorithmBase<spacedim,Value>::template_name()+" Field given by a Python script.")
		.derive_from(FieldAlgorithmBase<spacedim, Value>::get_input_type())
		.copy_keys(FieldAlgorithmBase<spacedim, Value>::get_field_algo_common_keys())
		.declare_key("source_file", it::String(), it::Default::obligatory(),
				"Python script given as external file in format 'dir'.'file_name' without .py extension")
		.declare_key("class", it::String(), it::Default::obligatory(),
				"Function in the given script that returns tuple containing components of the return type.\n"
				"For NxM tensor values: tensor(row,col) = tuple( M*row + col ).")
		//.declare_key("units", FieldAlgorithmBase<spacedim, Value>::get_field_algo_common_keys(), it::Default::optional(),
		//		"Definition of unit.")
		.close();
}


template <int spacedim, class Value>
const int FieldPython<spacedim, Value>::registrar =
		Input::register_class< FieldPython<spacedim, Value>, unsigned int >("FieldPython") +
		FieldPython<spacedim, Value>::get_input_type().size();



template <int spacedim, class Value>
FieldPython<spacedim, Value>::FieldPython(unsigned int n_comp)
: FieldAlgorithmBase<spacedim, Value>( n_comp)
{
	this->is_constant_in_space_ = false;
}




template <int spacedim, class Value>
void FieldPython<spacedim, Value>::init_from_input(const Input::Record &rec, const struct FieldAlgoBaseInitData& init_data) {
	this->init_unit_conversion_coefficient(rec, init_data);
	this->field_name_ = init_data.field_name_;

    std::string source_file = rec.val<string>("source_file");
    std::string source_class = rec.val<string>("class");
    try {
        set_python_field_from_class( source_file, source_class );
    } INPUT_CATCH(FilePath::ExcFileOpen, FilePath::EI_Address_String, rec)

    in_rec_ = rec;
}



template <int spacedim, class Value>
void FieldPython<spacedim, Value>::set_python_field_from_class(const string &file_name, const string &class_name)
{
    internal::PythonWrapper::initialize();

    py::module_ p_module = PythonLoader::load_module_from_file( string(file_name) );
    try {
        p_obj_ = p_module.attr(class_name.c_str())();
    } catch (const py::error_already_set &ex) {
        PythonLoader::throw_error(ex);
    }
}



/**
 * Returns one value in one given point. ResultType can be used to avoid some costly calculation if the result is trivial.
 */
template <int spacedim, class Value>
typename Value::return_type const & FieldPython<spacedim, Value>::value(const Point &p, const ElementAccessor<spacedim> &elm)
{
    set_value(p,elm, this->value_);
    this->value_.scale(this->unit_conversion_coefficient_);
    return this->r_value_;
}


/**
 * Returns std::vector of scalar values in several points at once.
 */
template <int spacedim, class Value>
void FieldPython<spacedim, Value>::value_list (const Armor::array &point_list, const ElementAccessor<spacedim> &elm,
                   std::vector<typename Value::return_type>  &value_list)
{
	ASSERT_EQ( point_list.size(), value_list.size() );
    ASSERT( point_list.n_rows() == spacedim && point_list.n_cols() == 1 ).error("Invalid point size.\n");
    for(unsigned int i=0; i< point_list.size(); i++) {
        Value envelope(value_list[i]);
        ASSERT_EQ( envelope.n_rows(), this->value_.n_rows() )(i)
                .error("value_list[i] has wrong number of rows\n");
        set_value(point_list.vec<spacedim>(i), elm, envelope );
        envelope.scale(this->unit_conversion_coefficient_);
    }
}

/**
* Returns one vector value in one given point.
*/
template <int spacedim, class Value>
void FieldPython<spacedim, Value>::set_value(FMT_UNUSED const Point &p, FMT_UNUSED const ElementAccessor<spacedim> &elm, FMT_UNUSED Value &value)
{
    p_value_ = p_func_(p[0], p[1], p[2]);

    unsigned int pos =0;
    for(unsigned int row=0; row < value.n_rows(); row++)
        for(unsigned int col=0; col < value.n_cols(); col++, pos++)
            if ( std::is_integral< typename Value::element_type >::value ) value(row,col) = p_value_[pos].cast<int>();
            else value(row,col) = p_value_[pos].cast<double>();
}




template <int spacedim, class Value>
std::vector<const FieldCommon * > FieldPython<spacedim, Value>::set_dependency(FieldSet &field_set) {
    required_fields_.clear();
    py::list used_fields_list;

    try {
        p_func_ = p_obj_.attr("used_fields");
        used_fields_list = p_func_();
    } catch (const py::error_already_set &ex) {
        PythonLoader::throw_error(ex);
    }

    for (auto f : used_fields_list) {
        std::string field_name = f.cast<std::string>();
        auto field_ptr = field_set.field(field_name);
        if (field_ptr != nullptr) required_fields_.push_back( field_ptr );
        else THROW( FieldSet::ExcUnknownField() << FieldCommon::EI_Field(field_name) << FieldSet::EI_FieldType("python declaration") << Input::EI_Address( in_rec_.address_string() ) );
    }

    // instance of FieldCommon of this field (see cache_reinit method)
    self_field_ptr_ = field_set.field(this->field_name_);

    return required_fields_;
}



template <int spacedim, class Value>
void FieldPython<spacedim, Value>::cache_reinit(FMT_UNUSED const ElementCacheMap &cache_map)
{
    std::vector<FieldCacheProxy> field_data;
    uint n_shape;
    for (auto field_ptr : required_fields_) {
        std::string field_name = field_ptr->name();
        double * cache_data = field_ptr->value_cache()->data_;
        n_shape = field_ptr->n_shape();
        std::vector<double> cache_vec(cache_data, cache_data+CacheMapElementNumber::get()*n_shape);
        field_data.emplace_back(field_name, n_shape, cache_vec);
    }

    n_shape = self_field_ptr_->n_shape();
    double * cache_data = self_field_ptr_->value_cache()->data_;
    std::vector<double> cache_vec(cache_data, cache_data+CacheMapElementNumber::get()*n_shape);
    FieldCacheProxy result_data(this->field_name_, n_shape, cache_vec);

    p_func_ = p_obj_.attr("_cache_reinit");
    p_func_(field_data, result_data);
}



template <int spacedim, class Value>
void FieldPython<spacedim, Value>::cache_update(FMT_UNUSED FieldValueCache<typename Value::element_type> &data_cache,
        ElementCacheMap &cache_map, unsigned int region_patch_idx)
{
    unsigned int reg_chunk_begin = cache_map.region_chunk_begin(region_patch_idx);
    unsigned int reg_chunk_end = cache_map.region_chunk_end(region_patch_idx);
    p_func_ = p_obj_.attr("cache_update");
    p_func_(reg_chunk_begin, reg_chunk_end);
}



template <int spacedim, class Value>
FieldPython<spacedim, Value>::~FieldPython() {}


namespace internal {

PythonWrapper::PythonWrapper() {
    std::string fname = std::string(FLOW123D_SOURCE_DIR) + "/src/python/flowpy/python_field_base.py";
    FilePath flowpy_path(fname, FilePath::input_file);
    std::string parent_path = flowpy_path.parent_path(); // add path to PythonPath
    PythonLoader::add_sys_path(parent_path);
    py::module_ flowpy_module = py::module_::import("flowpy");
    py::module_ pyfield_module = py::module_::import("python_field_base");
    flowpy_module.add_object("PythonFieldBase", pyfield_module.attr("PythonFieldBase"));
}

PythonWrapper &PythonWrapper::initialize() {
    static PythonWrapper python_wrapper;
    return python_wrapper;
}

}


#endif /* FIELD_PYTHON_IMPL_HH_ */

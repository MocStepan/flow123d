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
 * @file    equation.cc
 * @brief   Abstract base class for equation clasess.
 * @author  Jan Brezina
 */

#include <petscmat.h>
#include "tools/time_governor.hh"


#include "equation.hh"
#include "system/system.hh"
#include "input/accessors.hh"
#include "fields/field_set.hh"




/*****************************************************************************************
 * Implementation of EqBase
 */

Input::Type::Record & EquationBase::record_template() {
    return Input::Type::Record("EquationBase_AUX", "Auxiliary record with keys common for equations. Should not be used.")
        .declare_key("time", TimeGovernor::get_input_type(), Input::Type::Default("{}"),
                    "Time governor setting.")
		    .close();
}

Input::Type::Record & EquationBase::user_fields_template(std::string equation_name) {
    return Input::Type::Record("EquationBase_user_field_AUX", "Auxiliary record with common key user_field. Should not be used.")
        .declare_key("user_fields", Input::Type::Array(
                    FieldSet::make_user_field_type(equation_name)),
                    Input::Type::Default::optional(),
                    "Input fields of the equation defined by user.")
	    .close();
}

EquationBase::EquationBase()
: equation_empty_(true),
  mesh_(NULL),
  time_(NULL),
  input_record_(),
  eq_fieldset_(nullptr)
{}



EquationBase::EquationBase(Mesh &mesh, const  Input::Record in_rec)
: equation_empty_(false),
  mesh_(&mesh),
  time_(NULL),
  input_record_(in_rec),
  eq_fieldset_(nullptr)
{}


void EquationBase::set_time_governor(TimeGovernor &time)
{
  time_ = &time;
}

double EquationBase::solved_time()
{
    return time_->t();
}

void EquationBase::init_user_fields(Input::Array user_fields, double time, FieldSet &output_fields) {
    this->eq_fieldset_->init_user_fields(user_fields, time, output_fields);
}

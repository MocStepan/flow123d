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
 * @file    finite_element.cc
 * @brief   Abstract class for description of finite elements.
 * @author  Jan Stebel
 */

#include "system/system.hh"
#include "quadrature/quadrature.hh"
#include "fem/dofhandler.hh"
#include "fem/finite_element.hh"
#include "fem/fe_values.hh"



using namespace std;





template<class FS> const double Dof::evaluate(const FS &function_space,
                                              unsigned int basis_idx) const
{
    // Check that FS is derived from FunctionSpace.
    static_assert(std::is_base_of<FunctionSpace, FS>::value, "FS must be derived from FunctionSpace.");
    
    // We cannot evaluate dof on dim-dimensional n-face if the function space lies on lower-dimensional n-face.
    ASSERT(function_space.space_dim()+1 == coords.size());
    
    switch (type)
    {
    case Value:
    {
        // evaluate basis function and return the linear combination of components
        arma::vec vec_value(function_space.n_components());
        for (unsigned int c=0; c<function_space.n_components(); c++)
            vec_value[c] = function_space.basis_value(basis_idx, coords.subvec(0,coords.size()-2), c);
        return dot(coefs, vec_value);
        break;
    }
        
    default:
        OLD_ASSERT(false, "Dof evaluation not implemented for this type.");
    }
    return 0;
}



template<unsigned int dim, unsigned int spacedim>
FiniteElement<dim,spacedim>::FiniteElement()
    : function_space_(nullptr)
{
    init();
}

template<unsigned int dim, unsigned int spacedim>
void FiniteElement<dim,spacedim>::init(bool primitive, FEType type)
{
    dofs_.clear();
    is_primitive_ = primitive;
    type_ = type;
}


template<unsigned int dim, unsigned int spacedim>
void FiniteElement<dim,spacedim>::setup_components()
{
  component_indices_.resize(dofs_.size(), 0);
  nonzero_components_.resize(dofs_.size(), { true });
}


template<unsigned int dim, unsigned int spacedim> inline
void FiniteElement<dim,spacedim>::compute_node_matrix()
{
    arma::mat M(dofs_.size(), dofs_.size());

    for (unsigned int i = 0; i < dofs_.size(); i++)
        for (unsigned int j = 0; j < dofs_.size(); j++) {
            M(j, i) = dofs_[i].evaluate(*function_space_, j);

        }
    node_matrix = arma::inv(M);
}

template<unsigned int dim, unsigned int spacedim>
FEInternalData *FiniteElement<dim,spacedim>::initialize(const Quadrature<dim> &q)
{
    FEInternalData *data = new FEInternalData;

    arma::mat raw_values(function_space_->dim(), n_components());
    arma::mat shape_values(n_dofs(), n_components());

    data->ref_shape_values.resize(q.size());
    for (unsigned int i=0; i<q.size(); i++)
    {
        for (unsigned int j=0; j<function_space_->dim(); j++)
            for (unsigned int c=0; c<n_components(); c++)
              raw_values(j,c) = basis_value(j, q.point(i), c);

        shape_values = node_matrix * raw_values;

        data->ref_shape_values[i].resize(n_dofs());
        for (unsigned int j=0; j<n_dofs(); j++)
            data->ref_shape_values[i][j] = trans(shape_values.row(j));
    }

    arma::mat grad(dim,n_components());
    vector<arma::mat> grads(n_dofs());

    data->ref_shape_grads.resize(q.size());
    for (unsigned int i=0; i<q.size(); i++)
    {
        data->ref_shape_grads[i].resize(n_dofs());
        for (unsigned int j=0; j<n_dofs(); j++)
        {
            grad.zeros();
            for (unsigned int l=0; l<function_space_->dim(); l++)
              for (unsigned int c=0; c<n_components(); c++)
                grad.col(c) += basis_grad(l, q.point(i), c) * node_matrix(j,l);
            data->ref_shape_grads[i][j] = grad;
        }
    }

    return data;
}


template<unsigned int dim, unsigned int spacedim>
double FiniteElement<dim,spacedim>::basis_value(const unsigned int i, 
                                       const arma::vec::fixed<dim> &p,
                                       const unsigned int comp) const
{
    ASSERT_DBG( comp < n_components() );
	ASSERT_DBG( i < dofs_.size()).error("Index of basis function is out of range.");
    return this->function_space_->basis_value(i, p, comp);
}

template<unsigned int dim, unsigned int spacedim>
arma::vec::fixed<dim> FiniteElement<dim,spacedim>::basis_grad(const unsigned int i,
                                                     const arma::vec::fixed<dim> &p,
                                                     const unsigned int comp) const
{
    ASSERT_DBG( comp < n_components() );
	ASSERT_DBG( i < dofs_.size()).error("Index of basis function is out of range.");
    return this->function_space_->basis_grad(i, p, comp);
}


template<unsigned int dim, unsigned int spacedim> inline
UpdateFlags FiniteElement<dim,spacedim>::update_each(UpdateFlags flags)
{
    UpdateFlags f = flags;

    switch (type_)
    {
        case FEScalar:   
            if (flags & update_gradients)
                f |= update_inverse_jacobians;
            break;
        case FEVectorContravariant:
            if (flags & update_values)
                f |= update_jacobians;
            if (flags & update_gradients)
                f |= update_jacobians | update_inverse_jacobians;
            break;
        case FEVectorPiola:
            if (flags & update_values)
                f |= update_jacobians | update_volume_elements;
            if (flags & update_gradients)
                f |= update_jacobians | update_inverse_jacobians | update_volume_elements;
            break;
        default:;
    }

    return f;
}

template<unsigned int dim, unsigned int spacedim> inline
void FiniteElement<dim,spacedim>::fill_fe_values(
        const Quadrature<dim> &q,
        FEInternalData &data,
        FEValuesData<dim,spacedim> &fv_data)
{
    // shape values
    if (fv_data.update_flags & update_values)
    {
        for (unsigned int i = 0; i < q.size(); i++)
            for (unsigned int j = 0; j < n_dofs(); j++)
            {
                arma::vec fv_vec;
                switch (type_) {
                    case FEScalar:
                        fv_data.shape_values[i][j] = data.ref_shape_values[i][j][0];
                        break;
                    case FEVectorContravariant:
                        fv_vec = fv_data.jacobians[i] * data.ref_shape_values[i][j];
                        for (unsigned int c=0; c<spacedim; c++)
                            fv_data.shape_values[i][j*spacedim+c] = fv_vec[c];
                        break;
                    case FEVectorPiola:
                        fv_vec = fv_data.jacobians[i]*data.ref_shape_values[i][j]/fv_data.determinants[i];
                        for (unsigned int c=0; c<spacedim; c++)
                            fv_data.shape_values[i][j*spacedim+c] = fv_vec(c);
                        break;
//                     case FETensor:
//                         arma::mat ref_mat(dim);
//                         for (unsigned int c=0; c<spacedim*spacedim; c++)
//                             ref_mat[c/spacedim,c%spacedim] = data.ref_shape_values[i][j][c];
//                         arma::mat fv_mat = ref_mat*fv_data.inverse_jacobians[i];
//                         for (unsigned int c=0; c<spacedim*spacedim; c++)
//                             fv_data.shape_values[i][j*spacedim*spacedim+c] = fv_mat[c];
//                         break;
                    default:
                        ASSERT(false).error("Not implemented.");
                }
            }
    }

    // shape gradients
    if (fv_data.update_flags & update_gradients)
    {
        for (unsigned int i = 0; i < q.size(); i++)
        {
            for (unsigned int j = 0; j < n_dofs(); j++)
            {
                arma::mat grads;
                switch (type_) {
                    case FEScalar:
                        grads = trans(fv_data.inverse_jacobians[i]) * data.ref_shape_grads[i][j];
                        fv_data.shape_gradients[i][j] = grads;
                        break;
                    case FEVectorContravariant:
                        grads = trans(fv_data.inverse_jacobians[i]) * data.ref_shape_grads[i][j] * trans(fv_data.jacobians[i]);
                        for (unsigned int c=0; c<spacedim; c++)
                            fv_data.shape_gradients[i][j*spacedim+c] = grads.col(c);
                        break;
                    case FEVectorPiola:
                        grads = trans(fv_data.inverse_jacobians[i]) * data.ref_shape_grads[i][j] * trans(fv_data.jacobians[i])
                                / fv_data.determinants[i];
                        for (unsigned int c=0; c<spacedim; c++)
                            fv_data.shape_gradients[i][j*spacedim+c] = grads.col(c);
                        break;
                    default:
                        ASSERT(false).error("Not implemented.");
                }
            }
        }
    }
}



template<unsigned int dim, unsigned int spacedim>
FiniteElement<dim,spacedim>::~FiniteElement()
{
    if (function_space_ != nullptr) delete function_space_;
}


template class FiniteElement<0,3>;
template class FiniteElement<1,3>;
template class FiniteElement<2,3>;
template class FiniteElement<3,3>;



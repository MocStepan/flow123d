
#include "local_system.hh"

#include <armadillo>
#include "system/global_defs.h"

LocalSystem::LocalSystem(unsigned int nrows, unsigned int ncols)
: matrix(nrows, ncols), rhs(nrows)
{
    diag_values.resize(matrix.n_rows);
    row_dofs.resize(matrix.n_rows);
    col_dofs.resize(matrix.n_cols);
    reset();
}


void LocalSystem::reset()
{
    // zeros in local system
    matrix.zeros();
    rhs.zeros();
    // drop all dirichlet values
    loc_solution_rows.clear();
    loc_solution.clear();
    preferred_diag_values.clear();
    solution_not_set = true;
    
    // reset diag values
    std::fill(diag_values.begin(), diag_values.end(), 0);
    // reset global degrees of freedom vectors
    std::fill(row_dofs.begin(), row_dofs.end(), 0);
    std::fill(col_dofs.begin(), col_dofs.end(), 0);
}


void LocalSystem::set_solution(unsigned int loc_row, double solution, double diag_val)
{
    ASSERT_DBG(loc_row < matrix.n_rows);
    loc_solution_rows.push_back(loc_row);
    loc_solution.push_back(solution);
    preferred_diag_values.push_back(diag_val);
    solution_not_set = false;
}


void LocalSystem::fix_diagonal()
{
//     DBGCOUT("fix_diagonal\n");
    
    // skip diagonal fix
    if(solution_not_set) return;
        
    // correction of dirichlet diagonal entry
    for(unsigned int i=0; i < loc_solution_rows.size(); i++){
        unsigned int sol_row = loc_solution_rows[i];
        for(unsigned int col = 0; col < matrix.n_cols; col++) {
//                     DBGCOUT(<< "row " << row_dofs[sol_row] << "  col " << col_dofs[col] << "\n");
            // look for global diagonal entry
            if (row_dofs[sol_row] == col_dofs[col]) {
//                 DBGCOUT(<< "sol_row=" << sol_row << " col=" << col << " v=" << loc_solution[i] << "\n");
                // if not set, then use 1.0 on the diagonal
                double new_diagonal = 1.0;
                
                if(preferred_diag_values[i] !=0)    // if preferred value is set
                    new_diagonal = preferred_diag_values[i];
                else if(diag_values[sol_row] != 0)      // if an assembled value is available
                    new_diagonal = diag_values[sol_row];
                
//                 DBGVAR(new_diagonal);
                
//                 double new_diagonal = fabs(matrix(sol_row,col));
//                 if (new_diagonal == 0.0) {
//                     if (matrix.is_square()) {
//                         new_diagonal = arma::sum( abs(matrix.diag())) / matrix.n_rows;
//                     } else {
//                         new_diagonal = arma::accu( abs(matrix) ) / matrix.n_elem;
//                     }
//                 }
                matrix(sol_row,col) = new_diagonal;
                rhs(sol_row) = new_diagonal * loc_solution[i];
            }
        }
    }
}


// void LocalSystem::set_mat_value(unsigned int row, unsigned int col, double mat_val)
// {
//     ASSERT_DBG(row < matrix.n_rows);
//     ASSERT_DBG(col < matrix.n_cols);
//     
//     bool eliminate_row = false;
//     
//     double tmp_mat = mat_val;
//     double tmp_rhs = 0;
//     
//     for(unsigned int i=0; i < loc_solution_rows.size(); i++){
//         if(loc_solution_rows[i] == col){
//             tmp_mat = 0.0;
//             tmp_rhs -= mat_val * loc_solution[i];
//         }
//         if(loc_solution_rows[i] == row){ eliminate_row = true; }
//     }
//     
//     if(! eliminate_row){
//         matrix(row, col) = tmp_mat;
//         rhs(row) += tmp_rhs;
//     }
// }


void LocalSystem::set_mat_values(const std::vector< unsigned int >& rows, 
                                 const std::vector< unsigned int >& cols,
                                 const arma::mat& loc_matrix)
{
    ASSERT_DBG(loc_matrix.n_rows <= matrix.n_rows);
    ASSERT_DBG(loc_matrix.n_cols <= matrix.n_cols);
    
    arma::mat tmp_mat = loc_matrix;
    arma::vec tmp_rhs = arma::zeros<arma::vec>(loc_matrix.n_rows);
    
//     DBGCOUT("lrow\n");
    for(auto& sol : loc_solution_rows)
        for(unsigned int l_row = 0; l_row < rows.size(); l_row++)
            if (rows[l_row] == sol) {
                tmp_mat.row(l_row).zeros();
            }
    
//     DBGCOUT("lcol\n");
    for(unsigned int i=0; i < loc_solution_rows.size(); i++){
        for(unsigned int l_col = 0; l_col < cols.size(); l_col++)
            if (cols[l_col] == loc_solution_rows[i]) {
                tmp_rhs -= loc_matrix.col(l_col) * loc_solution[i];
                tmp_mat.col(l_col).zeros();
            }
    }
    
//     DBGCOUT("set mat and rhs\n");
//     DBGCOUT(<< "tmp\n" << tmp_mat);
    matrix.submat(arma::uvec(rows), arma::uvec(cols)) = tmp_mat;
    rhs.elem(arma::uvec(rows)) += tmp_rhs;
}



// void LocalSystem::set_value(unsigned int row, unsigned int col, double mat_val, double rhs_val)
// {
// //     DBGCOUT(<< "row " << row << "  col " << col << "\n");
//     ASSERT_DBG(row < matrix.n_rows);
//     ASSERT_DBG(col < matrix.n_cols);
//     
//     bool eliminate_row = false;
//     
//     double tmp_mat = mat_val;
//     double tmp_rhs = rhs_val;
//     
//     for(unsigned int i=0; i < loc_solution_rows.size(); i++){
//         if(loc_solution_rows[i] == col){
//             tmp_mat = 0.0;
//             tmp_rhs -= mat_val * loc_solution[i];
//             if(loc_solution_rows[i] == row){    // we are at the diagonal entry
//                 diag_values[row] = mat_val;    // add to diagonal value
//                 eliminate_row = true;           // set flag
//                 return;                         // we can now return
//             }
//         }
//         if(loc_solution_rows[i] == row) eliminate_row = true;
//     }
//     
//     if(! eliminate_row){
//         matrix(row, col) = tmp_mat;
//         rhs(row) = tmp_rhs;
//     }
// }


// void LocalSystem::add_value(unsigned int row, unsigned int col, double mat_val, double rhs_val)
// {
// //             DBGCOUT(<< "row " << row_dof << "  col " << col_dof << "\n");
//     ASSERT_DBG(row < matrix.n_rows);
//     ASSERT_DBG(col < matrix.n_cols);
//     
//     bool eliminate_row = false;
//     
//     double tmp_mat = mat_val;
//     double tmp_rhs = rhs_val;
//     
//     for(unsigned int i=0; i < loc_solution_rows.size(); i++){
//         if(loc_solution_rows[i] == col){
//             tmp_mat = 0.0;
//             tmp_rhs -= mat_val * loc_solution[i];
//             if(loc_solution_rows[i] == row){    // we are at the diagonal entry
//                 diag_values[row] += mat_val;    // add to diagonal value
//                 eliminate_row = true;           // set flag
//                 return;                         // we can now return
//             }
//         }
//         if(loc_solution_rows[i] == row) eliminate_row = true;
//     }
//     
//     if(! eliminate_row){
//         matrix(row, col) += tmp_mat;
//         rhs(row) += tmp_rhs;
//     }
// }

void LocalSystem::add_value(unsigned int row, unsigned int col, double mat_val, double rhs_val)
{
//             DBGCOUT(<< "row " << row_dof << "  col " << col_dof << "\n");
    ASSERT_DBG(row < matrix.n_rows);
    ASSERT_DBG(col < matrix.n_cols);
    
    if(solution_not_set){
        matrix(row, col) += mat_val;
        rhs(row) += rhs_val;
    }
    else{
        bool eliminate_row = false;
        double tmp_mat = mat_val;
        double tmp_rhs = rhs_val;
    
        for(unsigned int i=0; i < loc_solution_rows.size(); i++){
            // check whether the column is supposed to be eliminated by the known solution
            if(row_dofs[loc_solution_rows[i]] == col_dofs[col]){
                tmp_mat = 0.0;
                tmp_rhs -= mat_val * loc_solution[i];
                if(loc_solution_rows[i] == row){    // we are at the global diagonal entry
    //             if (row_dofs[row] == col_dofs[col]){ // we are at the diagonal entry
    //                 DBGVAR(mat_val);
    //                 DBGVAR(diag_values[row]);
                    diag_values[row] += mat_val;    // add to diagonal value
                    eliminate_row = true;           // set flag
                    return;                         // we can now return
                }
            }
            if(loc_solution_rows[i] == row) eliminate_row = true;
        }
        if(! eliminate_row){
            matrix(row, col) += tmp_mat;
            rhs(row) += tmp_rhs;
        }
    }
}


void LocalSystem::set_values(const std::vector< unsigned int >& rows,
                             const std::vector< unsigned int >& cols,
                             const arma::mat& loc_matrix,
                             const arma::vec& loc_rhs)
{
    ASSERT_DBG(loc_matrix.n_rows <= matrix.n_rows);
    ASSERT_DBG(loc_matrix.n_cols <= matrix.n_cols);
    
    arma::mat tmp_mat = loc_matrix;
    arma::vec tmp_rhs = loc_rhs;
    
//     DBGCOUT("lrow\n");
    for(auto& sol : loc_solution_rows)
        for(unsigned int l_row = 0; l_row < rows.size(); l_row++)
            if (rows[l_row] == sol) {
                tmp_rhs(l_row) = 0.0;
                tmp_mat.row(l_row).zeros();
            }
    
//     DBGCOUT("lcol\n");
    for(unsigned int i=0; i < loc_solution_rows.size(); i++){
        for(unsigned int l_col = 0; l_col < cols.size(); l_col++)
            if (cols[l_col] == loc_solution_rows[i]) {
                tmp_rhs -= loc_matrix.col(l_col) * loc_solution[i];
                tmp_mat.col(l_col).zeros();
            }
    }
    
//     DBGCOUT("set mat and rhs\n");
//     DBGCOUT(<< "tmp\n" << tmp_mat);
    matrix.submat(arma::uvec(rows), arma::uvec(cols)) = tmp_mat;
    rhs.elem(arma::uvec(rows)) = tmp_rhs;
}

/// Original function by JB

// void LocalSystem::set_values(std::vector< int >& rows, std::vector< int >& cols,
//                              const arma::mat& loc_matrix, const arma::vec& loc_rhs,
//                              const arma::vec& row_solution, const arma::vec& col_solution)
// {
//     ASSERT_DBG(loc_matrix.n_rows <= matrix.n_rows);
//     ASSERT_DBG(loc_matrix.n_cols <= matrix.n_cols);
//     
//     arma::mat tmp = loc_matrix;
//     arma::vec tmp_rhs = loc_rhs;
//     bool negative_row = false;
//     bool negative_col = false;
// 
// //             DBGCOUT(<< "tmp\n" << tmp);
// //             DBGCOUT("lrow\n");
//     for(unsigned int l_row = 0; l_row < rows.size(); l_row++)
//         if (rows[l_row] < 0) {
//             tmp_rhs(l_row)=0.0;
//             tmp.row(l_row).zeros();
// //             diag_values[l_row] = loc_matrix(l_row, l_row);    // add to diagonal value
//             negative_row=true;
//         }
// 
// //             DBGCOUT("lcol\n");
//     for(unsigned int l_col = 0; l_col < cols.size(); l_col++)
//         if (cols[l_col] < 0) {
//             tmp_rhs -= loc_matrix.col(l_col) * col_solution[l_col];
//             tmp.col(l_col).zeros();
//             negative_col=true;
//         }
//         
// //             DBGCOUT("main\n");
//         
//     if (negative_row && negative_col) {
//         // look for diagonal entry
//         for(unsigned int l_row = 0; l_row < rows.size(); l_row++)
//             if (rows[l_row] < 0)
//                 for(unsigned int l_col = 0; l_col < cols.size(); l_col++)
//                     if (cols[l_col] < 0 && row_dofs[-rows[l_row]] == col_dofs[-cols[l_col]]) {
// //                                 DBGCOUT(<< "lrow=" << l_row << " lcol=" << l_col << "\n");
//                         double new_diagonal = fabs(loc_matrix.at(l_row,l_col));
//                         if (new_diagonal == 0.0) {
//                             if (loc_matrix.is_square()) {
//                                 new_diagonal = arma::sum( abs(loc_matrix.diag())) / loc_matrix.n_rows;
//                             } else {
//                                 new_diagonal = arma::accu( abs(loc_matrix) ) / loc_matrix.n_elem;
//                             }
//                         }
//                         
// //                         double new_diagonal = 1.0;
// //                         diag_values[l_row] = fabs(loc_matrix.at(l_row,l_col));
// //                 
// //                         
// // //                         if(preferred_diag_values[i] !=0)    // if preferred value is set
// // //                             new_diagonal = preferred_diag_values[i];
// // //                         else 
// //                         if(diag_values[l_row] != 0)      // if an assembled value is available
// //                             new_diagonal = diag_values[l_row];
// //                                 tmp.at(l_col, l_row) = new_diagonal;
//                         tmp.at(l_row, l_col) = new_diagonal;
//                         tmp_rhs(l_row) = new_diagonal * row_solution[l_row];
//                     }
// 
//     }
// 
//     if (negative_row)
//         for(int &row : rows) row=abs(row);
// 
//     if (negative_col)
//         for(int &col : cols) col=abs(col);
// 
//     
// //             DBGCOUT( << "row_dofs:\n" << arma::conv_to<arma::uvec>::from(row_dofs));
// //             DBGCOUT( << "matrix:\n" << matrix);
// //             DBGCOUT("set mat and rhs\n");
// //             DBGCOUT(<< "tmp\n" << tmp);
//     matrix.submat(arma::conv_to<arma::uvec>::from(rows), arma::conv_to<arma::uvec>::from(cols)) = tmp;
//     rhs.elem(arma::conv_to<arma::uvec>::from(rows)) = tmp_rhs;
// }
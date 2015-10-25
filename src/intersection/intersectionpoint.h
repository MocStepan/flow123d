#ifndef INTERSECTIONPOINT_H_
#define INTERSECTIONPOINT_H_

#include <armadillo>
//#include <array>
#include "system/system.hh"

using namespace std;
namespace computeintersection{

/**
 * Have separate class template for IntersectionPoint as it appears at output
 * and other internal class template e.g. TriangleLineIntersection for internal
 * intersections with additional info.
 *
 * 
 * Represents a point from simplex<N> and simplex<M> intersection
 * contains bary coords of a point on simplex<N> and simplex<M>
 * Considering N < M. TODO Assert this condition.
 */
template<int N, int M> class IntersectionPoint {

	arma::vec::fixed<N+1> local_coords1; // bary coords of a point on simplex<N>
	arma::vec::fixed<M+1> local_coords2; // bary coords of a point on simplex<M>

	int side_idx1; // For case N = 2, M = 3 -> index of a triangle line
	int side_idx2; // For case N = 1 or N = 2, M = 3 -> index of a tetrahedron side

	//TODO comment on what the orientation really is (relation to orientation of side,line,plucker)
	unsigned int orientation; // orientation from intersection using plucker coords

	//TODO can be removed?
	/**
     * Introduce setter functions
     * H-S:     
     * H-H:
     * S-S:
     */
	bool is_vertex_; // point is a vertex of triangle
	bool is_patological_; // points is a vertex of tetrahedron or it is in side of tetrahedron or it is in edge of tetrahedron

	public:

    /// Desctructor.
    ~IntersectionPoint(){};
    
    /// Default constructor.
	IntersectionPoint();
    
    /**
     * TODO can be split into two setters ? coordinate data x topolopgy data
     */
	IntersectionPoint(const arma::vec::fixed<N+1> &lc1,
					  const arma::vec::fixed<M+1> &lc2,
					  int side1 = -1,
					  int side2 = -1,
					  unsigned int ori = 1,
					  bool vertex = false,
					  bool patological = false);
	

	/// Constructor - fliping dimension of an intersectionpoint
	IntersectionPoint(IntersectionPoint<M, N> &IP);

	/* Constructor interpolates the second bary coords of IntersectionPoint<N,M-1> to IntersectionPoint<N,M>
     * Allowed only from dimension 1 to 2 and from 2 to 3
     * */
	IntersectionPoint(IntersectionPoint<N,M-1> &IP);

	/* Constructor interpolates the second bary coords of IntersectionPoint<N,M-2> to IntersectionPoint<N,M>
	 * Allowed only from dimension 1 to 3
	 * */
	IntersectionPoint(IntersectionPoint<N,M-2> &IP);

	inline void clear(){
		local_coords1.zeros();
		local_coords2.zeros();
		side_idx1 = -1;
		side_idx2 = -1;
		orientation = 1;
		is_vertex_ = false;
		is_patological_ = false;
	};

	inline void print(){
		cout << "Local coords on the first element on side(" << side_idx1 << ")" << endl;
		local_coords1.print();
		cout << "Local coords on the second element on side(" << side_idx2 << ")" << endl;
		local_coords2.print();
		cout << "Orientation: " << orientation << " Vertex: " << is_vertex_ << " Patological: " << is_patological_ << endl;
	};

	inline const arma::vec::fixed<N+1> &get_local_coords1() const{
		return local_coords1;
	};
	inline const arma::vec::fixed<M+1> &get_local_coords2() const{
		return local_coords2;
	};

	inline void set_side1(int s){
		side_idx1 = s;
	};

	inline void set_side2(int s){
		side_idx2 = s;
	};

	inline void set_orientation(unsigned int o){
		orientation = o;
	};

	inline void set_is_vertex(bool iv){
		is_vertex_ = iv;
	}

	inline void set_is_patological(bool ip){
		is_patological_ = ip;
	}

	inline int get_side1() const{
		return side_idx1;
	};

	inline int get_side2() const{
		return side_idx2;
	};

	inline unsigned int get_orientation() const{
		return orientation;
	};

	inline bool is_vertex() const{
		return is_vertex_;
	};

	inline bool is_patological() const{
		return is_patological_;
	};

	inline friend ostream& operator<<(ostream& os, const IntersectionPoint<N,M>& IP){
		os << "test";
		return os;
	};
	/**
	 * For convex hull polygon tracing
	 */
	bool operator<(const IntersectionPoint<N,M> &ip) const;
};


} // END namespace
#endif /* INTERSECTIONPOINT_H_ */

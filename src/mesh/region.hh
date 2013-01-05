/*
 * region.hh
 *
 *  Created on: Nov 27, 2012
 *      Author: jb
 */

#ifndef REGION_HH_
#define REGION_HH_

#include <string>
#include <vector>
#include <map>

#include "system/system.hh"
#include "system/global_defs.h"
#include "system/exceptions.hh"

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/member.hpp>

namespace BMI=::boost::multi_index;

class RegionDB;

/**
 * Class that represents disjoint part of computational domain (or domains). It consists of one integer value
 * but provides access to other data stored in RegionDB. In particular provides string label and integer ID (unordered)
 * further it provides fast (inlined) methods to:
 * 1) detect if the region is the bulk region or boundary region
 * 2) return index (this is used to select correct Field, possibly we can distinguish boundary_index and bulk_index)
 *
 * Implementation: currently we number bulk regions by odd indices and boundary regions by even indices.
 */
class Region {
public:

    /// Default region is undefined
    Region():idx_(undefined) {}

    /// Returns true if it is a Boundary region and false if it is a Bulk region.
    inline bool is_boundary() const
        { return !(idx_ & 1); }

    /// Returns false if the region has undefined value
    inline bool is_valid() const
        { return idx_!=undefined;}

    /// Returns a global index of the region.
    inline unsigned int idx() const
        { return idx_; }

    /// Returns index of the region in the boundary set.
    inline unsigned int boundary_idx() const {
        ASSERT( is_boundary(), "Try to get boundary index of bulk region: '%s' id: %d\n", label().c_str(), id() );
        return idx_ >> 1; }

    /// Returns index of the region in the bulk set.
    inline unsigned int bulk_idx() const {
        ASSERT( ! is_boundary(), "Try to get bulk index of boundary region: '%s' id: %d\n", label().c_str(), id() );
        return idx_ >> 1; }

    /// Returns label of the region (using RegionDB)
    std::string label() const;

    /// Returns id of the region (using RegionDB)
    unsigned int id() const;

    /// Returns dimension of the region.
    unsigned int dim() const;

    /// Comparison operators
    inline bool operator==(const Region &other) const
        { return idx_ == other.idx_; }

    /// Comparison operators
    inline bool operator!=(const Region &other) const
        { return idx_ != other.idx_; }

    /**
     * Returns region database. Meant to be used for getting range of
     * global, boundary, and bulk region indices.
     */
    static RegionDB &db()
        { return db_;}

private:
    /// index for undefined region
    static const unsigned int undefined=0xffffffff;
    /// Global variable with information about all regions.
    static RegionDB db_;

    /**
     * Create accessor from the index. Private since implementation specific.
     */
    Region(unsigned int index)
    : idx_(index) {}

    unsigned int idx_;

    friend class RegionDB;
};


/**
 * Class representing a set of regions.
 * CAn be used  to set function(field) on more regions at once, possibly across meshes
 *
 * Desired properties:
 * - can construct itself from input, from a list
 *   of regions (given by label or id)
 * - support set operations
 * - say if an region is in it
 * - iterate through its regions
 */
class RegionSet {
};


/**
 * Class for conversion between an index and string label of an material.
 * Class contains only static members in order to provide globally consistent
 * indexing of materials across various meshes and functions.
 *
 * The conversion should be performed only through the input and output so
 * that the lookup overhead could be shadowed by IO operations.
 *
 * Taking sizes and creating region sets should be possible only after the database is closed.
 * We assume that all regions are known at beginning of the program (typically after reading all meshes)
 * however they need not be used through the whole computation.
 *
 *
 */

class RegionDB {
public:
    TYPEDEF_ERR_INFO( EI_Label, const std::string);
    TYPEDEF_ERR_INFO( EI_ID, unsigned int);
    TYPEDEF_ERR_INFO( EI_IDOfOtherLabel, unsigned int);
    TYPEDEF_ERR_INFO( EI_LabelOfOtherID, const std::string);
    DECLARE_EXCEPTION( ExcAddingIntoClosed, << "Can not add label=" << EI_Label::qval << " into closed MaterialDispatch.\n");
    DECLARE_EXCEPTION( ExcSizeWhileOpen, << "Can not get size of MaterialDispatch yet open.");
    DECLARE_EXCEPTION( ExcInconsistentAdd, << "Inconsistent add of region with id: " << EI_ID::val << ", label: " << EI_Label::qval << "\n" \
                                             << "other region with same ID but different label: " << EI_LabelOfOtherID::qval << " already exists\n" \
                                             << "OR other region with same label but different ID: " << EI_IDOfOtherLabel::val << " already exists\n" \
                                             << "OR both ID and label match an existing region with different dimension and/or boundary flag.");
    DECLARE_EXCEPTION( ExcCantAdd, << "Can not add new region into DB, id: " << EI_ID::val <<", label: " << EI_Label::qval);


    /// Default constructor
    RegionDB()
    : closed_(false), n_boundary_(0), n_bulk_(0)  {}

    /**
     * Introduce an artificial limit to keep all material indexed arrays
     * of reasonable size.
     */
    static const unsigned int max_n_regions = 64000;


    /**
     * Add new region into database and return its index. This requires full
     * specification of the region that is given in PhysicalNames section of the GMSH MSH format.
     * If the region is already in the DB, check consistency of label and id and return its index.
     *
     * Parameter @p id is any non-negative integer that is unique for the region over all meshes used in the simulation,
     * parameter @p label is unique string identifier of the region, @p dim is dimension of reference elements in the region
     * and @p boundary is true if the region consist of boundary elements (where one can apply boundary condition).
     *
     */
    Region add_region(unsigned int id, const std::string &label, unsigned int dim, bool boundary);

    /**
     * As the previous, but generates automatic label of form 'region_ID', and set bulk region.
     * Meant to be used when reading elements from MSH file. Again, if the region is defined already, we just check consistency.
     */
    Region add_region(unsigned int id, unsigned int dim);

    /**
     * Returns a @p Region with given @p label. If it is not found it returns @p undefined Region.
     */
    Region find_label(const std::string &label);

    /**
     * Returns a @p Region with given @p id. If it is not found it returns @p undefined Region.
     */
    Region find_id(unsigned int id);

    /**
     * Return original label for given index @p idx.
     */
    const std::string &get_label(unsigned int idx) const;
    /**
     * Return original ID for given index @p idx.
     */
    unsigned int get_id(unsigned int idx) const;
    /**
     * Return dimension of region with given index @p idx.
     */
    unsigned int get_dim(unsigned int idx) const;
    /**
     * Close this class for adding labels. This is necessary to return correct size
     * for material indexed arrays and vectors. After calling this method you can
     * call method @p size and method @p idx_of_label rise an exception for any unknown label.
     */
    void close();
    /**
     * Returns maximal index + 1
     */
    unsigned int size();
    /**
     * Returns total number boundary regions.
     */
    unsigned int boundary_size();
    /**
     * Returns total number bulk regions.
     */
    unsigned int bulk_size();


private:
    /// One item in region database
    struct RegionItem {
        RegionItem(unsigned int index, unsigned int id, const std::string &label, unsigned int dim)
            : index(index), id(id), label(label), dim_(dim) {}

        // unique identifiers
        unsigned int index;
        unsigned int id;
        std::string label;
        // data
        unsigned int dim_;
    };

    // tags
    struct ID {};
    struct Label {};
    struct Index {};

    /// Region database
    typedef BMI::multi_index_container<
            RegionItem,
            BMI::indexed_by<
                // access by index (can not use random access since we may have empty (and unmodifiable) holes)
                BMI::ordered_unique< BMI::tag<Index>, BMI::member<RegionItem, unsigned int, &RegionItem::index > >,
                // ordered access (like stl::map) by id and label
                BMI::ordered_unique< BMI::tag<ID>,    BMI::member<RegionItem, unsigned int, &RegionItem::id> >,
                BMI::ordered_unique< BMI::tag<Label>, BMI::member<RegionItem, std::string, &RegionItem::label> >
            >
    > RegionSet;

    RegionSet region_set_;

    bool closed_;
    /// Number of boundary regions
    unsigned int n_boundary_;
    /// Number of bulk regions
    unsigned int n_bulk_;
};



#endif /* REGION_HH_ */
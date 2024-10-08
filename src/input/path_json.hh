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
 * @file    path_json.hh
 * @brief   
 */

#ifndef PATH_JSON_HH_
#define PATH_JSON_HH_

#include <memory>
#include <stdint.h>                               // for int64_t
#include <iosfwd>                                 // for ostream, istream
#include <set>                                    // for set
#include <string>                                 // for string
#include <vector>                                 // for vector
#include "input/json_spirit/json_spirit_value.h"  // for mValue
#include "input/path_base.hh"
//#include "json_spirit/json_spirit.h"


namespace Input {



/**
 * @brief Class used by ReaderToStorage class to iterate over the JSON tree provided by json_spirit library.
 *
 * This class keeps whole path from the root of the JSON tree to the current node. We store nodes along path in \p nodes_
 * and address of the node in \p path_.
 *
 * The class also contains methods for processing of special keys 'REF' and 'TYPE'. The reference is record with only one key
 * 'REF' with a string value that contains address of the reference. The string with the address is extracted and provided by
 * method \p JSONtoStorage::find_ref_node.
 */
class PathJSON : public PathBase {
public:


    /**
     * @brief Constructor.
     *
     * Call JSON parser for given stream and create PathJSON for the root
     * of parsed data tree.
     */
    PathJSON(std::istream &in);

    /**
     * @brief Destructor.
     *
     * Have to cleanup nodes_.
     */
    ~PathJSON() override;

    /**
     * @brief Dive into json_spirit hierarchy.
     *
     * Implements @p PathBase::down(unsigned int)
     */
    bool down(unsigned int index) override;

    /**
     * @brief Dive into json_spirit hierarchy.
     *
     * Implements @p PathBase::down(const std::string&)
     */
    bool down(const std::string& key, int index = -1) override;

    /// Return one level up in the hierarchy.
    void up() override;

    /// Implements @p PathBase::level
    inline int level() const
    { return nodes_.size() - 1; }

    // These methods are derived from PathBase
    bool is_null_type() const override;                               ///< Implements @p PathBase::is_null_type
    bool get_bool_value() const override;                             ///< Implements @p PathBase::get_bool_value
    std::int64_t get_int_value() const override;                      ///< Implements @p PathBase::get_int_value
    double get_double_value() const override;                         ///< Implements @p PathBase::get_double_value
    std::string get_string_value() const override;                    ///< Implements @p PathBase::get_string_value
    unsigned int get_node_type_index() const override;                ///< Implements @p PathBase::get_node_type_index
    bool get_record_key_set(std::set<std::string> &) const override;  ///< Implements @p PathBase::get_record_key_set
    bool is_effectively_null() const override;                        ///< Implements @p PathBase::is_effectively_null
    int get_array_size() const override;                              ///< Implements @p PathBase::get_array_size
    bool is_record_type() const override;                             ///< Implements @p PathBase::is_record_type
    bool is_array_type() const override;                              ///< Implements @p PathBase::is_array_type
    PathJSON * clone() const override;                                ///< Implements @p PathBase::clone

    /// Implements @p PathBase::get_record_tag
    std::string get_record_tag() const override;

    /// Implements reading of reference keys, and check of cyclic references.
    PathBase * find_ref_node() override;

    /// Put address of actual reference to previous_references_ set
    void remember_reference();



protected:

    /**
     * @brief Default constructor.
     *
     * Provides common initialization for public constructors.
     */
    PathJSON();

    /// Definition of JSON Spirit node.
    typedef json_spirit::mValue Node;

    /// Pointer to JSON Value object at current path.
    inline const Node * head() const
    { return nodes_.back(); }

    /**
     * @brief Remember used references in order to avoid detect cyclic references.
     *
     * In JSON we allow usage of references using special key 'REF'.
     */
    std::set<std::string> previous_references_;

    // Root node has to be automatically deleted.
    std::shared_ptr<Node> root_node_;

    // Pointers to all nodes from the root up to the current path.
    std::vector<const Node *> nodes_;

};

/**
 * @brief Output operator for PathJSON.
 *
 * Mainly for debugging purposes and error messages.
 */
std::ostream& operator<<(std::ostream& stream, const PathJSON& path);



} // namespace Input



#endif /* PATH_JSON_HH_ */

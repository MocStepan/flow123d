/*
 * type_base.hh
 *
 *  Created on: May 1, 2012
 *      Author: jb
 */

#ifndef TYPE_BASE_HH_
#define TYPE_BASE_HH_


#include "system/system.hh"
#include "system/exceptions.hh"
#include "system/file_path.hh"


#include <limits>
#include <ios>
#include <map>
#include <vector>
#include <string>
#include <iomanip>

#include <boost/type_traits.hpp>
#include <boost/tokenizer.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>




namespace Input {

namespace Type {



using namespace std;

/**
 * Declaration of common exceptions and error info types.
 */
TYPEDEF_ERR_INFO( EI_KeyName, const string );

TYPEDEF_ERR_INFO( EI_DefaultStr, const string);
TYPEDEF_ERR_INFO( EI_TypeName, const string);
DECLARE_EXCEPTION( ExcWrongDefault, << "Default value " << EI_DefaultStr::qval
        << " do not match type: " << EI_TypeName::qval << ";\n"
        << "During declaration of the key: " << EI_KeyName::qval );




/**
 * @brief Base of classes for declaring structure of the input data.
 *
 *  Provides methods common to all types. Namely, the type name, finished status (nontrivial only for types with complex initialization - Record, AbstractRecosd, Selection)
 *  and output of the documentation.
 *
 *  @ingroup input_types
 */
class TypeBase {
public:
    /// Possible types of documentation output
    enum DocType {
        record_key,             ///<    Short type description as part of key description.
        full_after_record,      ///<    Detail description of complex types after the record.
        full_along              ///<    Detail description of the type itself as part of the error messages (no descendants).
    };
    /**
     * Default constructor. Set all types finished after construction.
     */
    TypeBase() {}
    /**
     * @brief Implementation of documentation printing mechanism.
     *
     * It writes documentation into given stream @p stream. With @p extensive==0, it should print the description
     * about two lines long, while for @p extensive>0 it outputs full documentation. For value 1 it do not
     * output documentation of subtypes, while for value 2 it calls decumentations of subtypes recursively.
     *
     * This is primarily used for documentation of Record types where we first
     * describe all keys of an Record with short descriptions and then we call recursively extensive documentation
     * for sub types that was not fully described yet. Further, we provide method @p reset_doc_flags to reset
     * all flags marking the already printed documentations. Parameter @p pad is used for correct indentation.
     *
     * TODO: Make specialized class for output of the declaration tree into various output formats.
     * Search the tree by BFS instead of DFS (current implementation).
     *
     */
    virtual std::ostream& documentation(std::ostream& stream, DocType=full_along, unsigned int pad=0) const = 0;

    /**
     * In order to output documentation of complex types only once, we mark types that have printed their documentation.
     * This method turns these marks off for the whole type subtree.
     */
    virtual void  reset_doc_flags() const =0;

    /// Returns true if the type is fully specified. In particular for Record and Selection, it returns true after @p finish() method is called.
    virtual bool is_finished() const
    {return true;}

    /// Returns an identification of the type. Useful for error messages.
    virtual string type_name() const  =0;

    /**
     * Returns string with Type extensive documentation.
     */
    string desc() const;

    /**
     * Comparison of types. It compares kind of type (Intteger, Double, String, REcord, ..), for complex types
     * it also compares names. For arrays compare subtypes.
     */
    virtual bool operator==(const TypeBase &other) const
        { return typeid(*this) == typeid(other); }

    /// Comparison of types.
    bool operator!=(const TypeBase & other) const
        { return ! (*this == other); }

    /// Empty virtual destructor.
    virtual ~TypeBase( void ) {}

    /**
     * For types that can be initialized from a default string, this method check
     * validity of the default string. For invalid string an exception is thrown.
     */
    virtual void valid_default(const string &str) const
    { }

protected:
    /**
     * Write out a string with given padding of every new line.
     */
    static std::ostream& write_description(std::ostream& stream, const string& str, unsigned int pad);

    /**
     * Type of hash values used in associative array that translates key names to indices in a record.
     *
     * For simplicity, we currently use whole strings as "hash".
     */
    typedef string KeyHash;

    /// Hash function.
    inline static KeyHash key_hash(const string &str) {
        return (str);
    }

    /**
     * Check that a @p key is valid identifier, i.e. consists only of valid characters, that are lower-case letters, digits and underscore,
     * we allow identifiers starting with a digit, but it is discouraged since it slows down parsing of the input file.
     */
    static bool is_valid_identifier(const string& key);


};

/**
 * For convenience we provide also redirection operator for output documentation of Input:Type classes.
 */
std::ostream& operator<<(std::ostream& stream, const TypeBase& type);


class Record;
class SelectionBase;

/**
 * @brief Class for declaration of inputs sequences.
 *
 * The type is fully specified after its constructor is called. All elements of the Array has same type, however you
 * can use elements of AbstractRecord.
 *
 * If you not disallow Array size 1, the input reader will try to convert any other type
 * on input into array with one element, e.g.
 @code
     int_array=1        # is equivalent to
     int_array=[ 1 ]
 @endcode
 *
 * @ingroup input_types
 */
class Array : public TypeBase {

public:
    /**
     * Constructor with a @p type of array items given as pure reference. In this case \p type has to by descendant of \p TypeBase different from
     * 'complex' types @p Record and @p Selection. You can also specify minimum and maximum size of the array.
     */
    template <class ValueType>
    inline Array(const ValueType &type, unsigned int min_size=0, unsigned int max_size=std::numeric_limits<unsigned int>::max() )
    : lower_bound_(min_size), upper_bound_(max_size)
    {
        // ASSERT MESSAGE: The type of declared keys has to be a class derived from TypeBase.
        BOOST_STATIC_ASSERT( (boost::is_base_of<TypeBase, ValueType >::value) );

        ASSERT( min_size <= max_size, "Wrong limits for size of Input::Type::Array, min: %d, max: %d\n", min_size, max_size);

        if ( ! type.is_finished())
            xprintf(PrgErr, "Unfinished type '%s' used in declaration of Array.\n", type.type_name().c_str() );

        boost::shared_ptr<const ValueType> type_copy = boost::make_shared<ValueType>(type);
        type_of_values_ = type_copy;
        //set_type_impl(type, boost::is_base_of<TypeBase,ValueType>() );
    }



    /// Getter for the type of array items.
    inline const TypeBase &get_sub_type() const
        { return *type_of_values_; }

    /// Checks size of particular array.
    inline bool match_size(unsigned int size) const
        { return size >=lower_bound_ && size<=upper_bound_; }

    /// @brief Implements @p Type::TypeBase::documentation.
    virtual std::ostream& documentation(std::ostream& stream, DocType=full_along, unsigned int pad=0) const;

    /// @brief Implements @p Type::TypeBase::reset_doc_flags.
    virtual void  reset_doc_flags() const;

    /// @brief Implements @p Type::TypeBase::type_name. Name has form \p array_of_'subtype name'
    virtual string type_name() const;

    /// @brief Implements @p Type::TypeBase::operator== Compares also subtypes.
    virtual bool operator==(const TypeBase &other) const;

    /**
     *  Default values for an array creates array containing one element
     *  that is initialized by given default value. So this method check
     *  if the default value is valid for the sub type of the array.
     */
    virtual void valid_default(const string &str) const;

protected:

    boost::shared_ptr<const TypeBase> type_of_values_;
    unsigned int lower_bound_, upper_bound_;

};



/**
 * @brief Base of all scalar types.
 *
 *  @ingroup input_types
 */
class Scalar : public TypeBase {
public:

    virtual void  reset_doc_flags() const;
};


/**
 * @brief Class for declaration of the input of type Bool.
 *
 * String names of boolean values are \p 'true' and \p 'false'.
 *
 * @ingroup input_types
 */
class Bool : public Scalar {
public:
    Bool()
    {}

    virtual void valid_default(const string &str) const;

    bool from_default(const string &str) const;

    virtual std::ostream& documentation(std::ostream& stream, DocType=full_along, unsigned int pad=0)  const;
    virtual string type_name() const;
};


/**
 * @brief Class for declaration of the integral input data.
 *
 * The data are stored in an \p signed \p int variable. You can specify bounds for the valid input data.
 *
 * @ingroup input_types
 */
class Integer : public Scalar {
public:
    Integer(int lower_bound=std::numeric_limits<int>::min(), int upper_bound=std::numeric_limits<int>::max())
    : lower_bound_(lower_bound), upper_bound_(upper_bound)
    {}

    /**
     * Returns true if the given integer value conforms to the Type::Integer bounds.
     */
    bool match(int value) const;

    /**
     * As before but also returns converted integer in @p value.
     */
    int from_default(const string &str) const;

    /// Implements  @p Type::TypeBase::valid_defaults.
    virtual void valid_default(const string &str) const;

    virtual std::ostream& documentation(std::ostream& stream, DocType=full_along, unsigned int pad=0)  const;
    virtual string type_name() const;
private:
    int lower_bound_, upper_bound_;

};


/**
 * @brief Class for declaration of the input data that are floating point numbers.
 *
 * The data are stored in an \p double variable. You can specify bounds for the valid input data.
 *
 * @ingroup input_types
 */
class Double : public Scalar {
public:
    Double(double lower_bound= -std::numeric_limits<double>::max(), double upper_bound=std::numeric_limits<double>::max())
    : lower_bound_(lower_bound), upper_bound_(upper_bound)
    {}

    /**
     * Returns true if the given integer value conforms to the Type::Double bounds.
     */
    bool match(double value) const;

    /**
     * As before but also returns converted integer in @p value.
     */
    double from_default(const string &str) const;

    /// Implements  @p Type::TypeBase::valid_defaults.
    virtual void valid_default(const string &str) const;

    virtual std::ostream& documentation(std::ostream& stream, DocType=full_along, unsigned int pad=0)  const;
    virtual string type_name() const;
private:
    double lower_bound_, upper_bound_;

};



/**
 * Just for consistency, but is essentially same as Scalar.
 *
 * @ingroup input_types
 */
class String : public Scalar {
public:
    virtual std::ostream& documentation(std::ostream& stream, DocType=full_along, unsigned int pad=0) const;
    virtual string type_name() const;

    virtual void valid_default(const string &str) const;

    string from_default(const string &str) const;


    /**
     * Particular descendants can check validity of the string.
     */
    virtual bool match(const string &value) const;
};


/**
 * @brief Class for declaration of the input data that are file names.
 *
 * We strictly distinguish filenames for input and output files.
 *
 * @ingroup input_types
 */
class FileName : public String {
public:

    /**
     * Factory function for declaring type FileName for input files.
     */
    static FileName input()
    { return FileName(::FilePath::input_file); }

    /**
     * Factory function for declaring type FileName for input files.
     */
    static FileName output()
    { return FileName(::FilePath::output_file); }

    virtual std::ostream& documentation(std::ostream& stream, DocType=full_along, unsigned int pad=0)  const;
    virtual string type_name() const;

    virtual bool operator==(const TypeBase &other) const
    { return  typeid(*this) == typeid(other) &&
                     (type_== static_cast<const FileName *>(&other)->get_file_type() );
    }

    /// Checks relative output paths.
    virtual bool match(const string &str) const;


    /**
     * Returns type of the file input/output.
     */
    ::FilePath::FileType get_file_type() const {
        return type_;
    }



private:
    ::FilePath::FileType    type_;

    /// Forbids default constructor.
    FileName() {}

    /// Forbids direct construction.
    FileName(enum ::FilePath::FileType type)
    : type_(type)
    {}

};

} // closing namespace Type
} // closing namespace Input





#endif /* TYPE_BASE_HH_ */
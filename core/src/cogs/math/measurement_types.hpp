//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: WorkInProgress

#ifndef COGS_HEADER_MATH_MEASUREMENT_TYPES
#define COGS_HEADER_MATH_MEASUREMENT_TYPES

#include <type_traits>

#include "cogs/env.hpp"
#include "cogs/operators.hpp"
#include "cogs/math/fixed_integer_extended.hpp"
#include "cogs/math/fixed_integer_native.hpp"
#include "cogs/math/fixed_integer_extended_const.hpp"
#include "cogs/math/fixed_integer_native_const.hpp"


namespace cogs {


// Base class for conceptual quantity types, such as time, distance, weight, etc.
class quantity_type_base
{
	//public: typedef preferred_unit_t;
};

// Base class for all measurement unit types, such as seconds, meters, lbs, etc.
class unit_type_base
{
public:
	//public: template <typename char_t, typename unit_t> static composite_string_t<char_t> to_string_t(const unit_t& n);
	//public: template <typename unit_t> static composite_string to_string(const unit_t& n);
	//public: template <typename unit_t> static composite_cstring to_cstring(const unit_t& n);
};

// A unit_type_base for raw numbers, of no particular unit or quantity type.
class numeric_units : public unit_type_base
{
};

// Base class for measurement unit types of a particular quantity type.
// i.e. class hours   : public unit_type<time> { }
//      class minutes : public unit_type<time> { }
template <class quantity_type>
class unit_type : public unit_type_base
{
public:
	typedef quantity_type quantity_t;
};


// i.e. rate_quantity_type<distance, time>
template <class x, class y>
class rate_quantity_type;


// i.e. mph: rate_unit_type<miles, hour>
template <class x, class y>
class rate_unit_type : public unit_type_base
{
public:
	typedef rate_quantity_type<x, y> quantity_t;
};


// i.e. area: quantity_exponent<distance, 2>
template <class T, size_t exponent>
class exponent_quantity_type : public quantity_type_base
{
};

// i.e. square miles: unit_exponent<mile, 2>
template <class T, size_t exponent>
class exponent_unit_type : public unit_type_base
{
public:
	typedef exponent_quantity_type<typename T::quantity_t, exponent> quantity_t;
};


class seconds;
class time : public quantity_type_base { public: typedef seconds preferred_unit_t; };

class meters { };
class grams { };


class distance : public quantity_type_base { public: typedef meters preferred_unit_t; };

template <> class exponent_quantity_type<distance, 2> : public quantity_type_base { };
typedef exponent_quantity_type<distance, 2> area;

template <> class exponent_quantity_type<distance, 3> : public quantity_type_base { };
typedef exponent_quantity_type<distance, 3> volume;


template <> class rate_quantity_type<distance, time> { };

typedef rate_quantity_type<distance, time> speed;
typedef speed velocity;

template <> class rate_quantity_type<velocity, time> { };
typedef rate_quantity_type<velocity, time> acceleration;

class mass : public quantity_type_base { public: typedef grams preferred_unit_t; };

class weight : public quantity_type_base { };



template <class T1, class T2, typename enable = void>
class unit_conversion
{
};

template <class T>
class unit_conversion<T, T>
{
public:
	typedef one_t ratio_const_t;
};

// Automatically generate conversion in the other direction, if one direction is defined
template <class T1, class T2>
class unit_conversion<
	T1,
	T2, 
	std::enable_if_t<
		!std::is_same_v<T1, T2>
		&& !std::is_same_v<T2, typename T1::quantity_t::preferred_unit_t>
		&& std::is_same_v<T1, typename T1::quantity_t::preferred_unit_t>
	>
>
{
public:
	typedef decltype(reciprocal(std::declval<typename unit_conversion<T2, T1>::ratio_const_t>())) ratio_const_t;
};


template <class from_t, class to_t, class reative_to_t>
class unit_conversion_relative
{
private:
	typedef typename unit_conversion<to_t, reative_to_t>::ratio_const_t from_ratio_const_t;
	typedef typename unit_conversion<to_t, reative_to_t>::ratio_const_t to_ratio_const_t;

public:
	typedef decltype(multiply(std::declval<from_ratio_const_t>(), std::declval<to_ratio_const_t>())) ratio_const_t;
};

// Automatically convert to/from any 2 units that define conversion to/from the preferred unit base
template <class T1, class T2>
class unit_conversion<
	T1,
	T2,
	std::enable_if_t<
		!std::is_same_v<T1, T2>
		&& !std::is_same_v<T2, typename T1::quantity_t::preferred_unit_t>
		&& !std::is_same_v<T1, typename T1::quantity_t::preferred_unit_t>
	>
>
{
public:
	typedef typename unit_conversion_relative<T1, T2, typename T1::quantity_t::preferred_unit_t>::ratio_const_t ratio_const_t;
};


// based on conversion, determine which type is more course and which is more fine
template <typename T1, typename T2>
class is_finer
{
public: 
	static constexpr bool value = const_compared<decltype(std::declval<typename unit_conversion<T1, T2>::ratio_const_t>().floor()), one_t>::value < 0;
};
template <typename T1, typename T2>
constexpr bool is_finer_v = is_finer<T1, T2>::value;

template <typename T> class is_finer<T, T> : public std::false_type { };


template <typename T1, typename T2>
class is_courser
{
public:
	static constexpr bool value = !is_finer_v<T1, T2>;
};
template <typename T1, typename T2>
constexpr bool is_courser_v = is_courser<T1, T2>::value;

template <typename T> class is_courser<T, T> : public std::false_type { };


template <typename T1, typename T2>
class finer
{
public:
	typedef std::conditional_t<is_finer_v<T1, T2>, T1, T2> type;
};

template <typename T1, typename T2>
using finer_t = typename finer<T1, T2>::type;


template <typename T1, typename T2>
class courser
{
public:
	typedef std::conditional_t<is_courser_v<T1, T2>, T1, T2> type;
};

template <typename T1, typename T2>
using courser_t = typename courser<T1, T2>::type;


}


#include "cogs/math/time.hpp"


#endif

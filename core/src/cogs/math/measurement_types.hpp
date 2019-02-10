//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: WorkInProgress

#ifndef COGS_MEASUREMENT_TYPES
#define COGS_MEASUREMENT_TYPES

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


// Always fully specialize unit_conversion with the primary unit type as the second template parameter

template <class T1, class T2, typename enable = void>
class unit_conversion
{
};

template <class T>
class unit_conversion<T, T>
{
public:
	typedef fixed_integer_native_const<false, 1, 1> numerator_const_t;
	typedef fixed_integer_native_const<false, 1, 1> denominator_const_t;

	typedef fixed_integer_native_const<false, 1, 1> ratio_const_t;
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
	typedef typename unit_conversion<T2, T1>::denominator_const_t numerator_const_t;
	typedef typename unit_conversion<T2, T1>::numerator_const_t denominator_const_t;

	typedef decltype(divide(std::declval<numerator_const_t>(), std::declval<denominator_const_t>())) ratio_const_t;
};


template <class from_t, class to_t, class reative_to_t>
class unit_conversion_relative
{
private:
	typedef typename unit_conversion<from_t, reative_to_t>::numerator_const_t from_numerator_const_t;
	typedef typename unit_conversion<from_t, reative_to_t>::denominator_const_t from_denominator_const_t;

	typedef typename unit_conversion<to_t, reative_to_t>::numerator_const_t to_numerator_const_t;
	typedef typename unit_conversion<to_t, reative_to_t>::denominator_const_t to_denominator_const_t;


	typedef decltype(divide(std::declval<from_numerator_const_t>(), std::declval<from_denominator_const_t>())) ratio_const_t1;
	typedef decltype(divide(std::declval<to_numerator_const_t>(), std::declval<to_denominator_const_t>())) ratio_const_t2;

	typedef decltype(multiply(std::declval<from_numerator_const_t>(), std::declval<to_numerator_const_t>())) combined_numerator_const_t;
	typedef decltype(multiply(std::declval<from_denominator_const_t>(), std::declval<to_denominator_const_t>())) combined_denominator_const_t;

	typedef decltype(gcd(std::declval<combined_numerator_const_t>(), std::declval<combined_denominator_const_t>())) divisor_t;

public:
	typedef decltype(add(std::declval<combined_numerator_const_t>(), std::declval<divisor_t>())) numerator_const_t;
	typedef decltype(add(std::declval<combined_denominator_const_t>(), std::declval<divisor_t>())) denominator_const_t;

	typedef decltype(divide(std::declval<numerator_const_t>(), std::declval<denominator_const_t>())) ratio_const_t;
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
	typedef typename unit_conversion_relative<T1, T2, typename T1::quantity_t::preferred_unit_t>::numerator_const_t numerator_const_t;
	typedef typename unit_conversion_relative<T1, T2, typename T1::quantity_t::preferred_unit_t>::denominator_const_t denominator_const_t;

	typedef typename unit_conversion_relative<T1, T2, typename T1::quantity_t::preferred_unit_t>::ratio_const_t ratio_const_t;
};


// based on conversion, determine which type is more course and which is more fine
template <typename T1, typename T2>
class is_finer
{
private:
	typedef unit_conversion<T1, T2> conversion_t;
	typedef typename conversion_t::numerator_const_t numerator_const_t;
	typedef typename conversion_t::denominator_const_t denominator_const_t;

public:
	static constexpr bool value = (const_compared<numerator_const_t, denominator_const_t>::value) > 0;
};
template <typename T1, typename T2>
constexpr bool is_finer_v = is_finer<T1, T2>::value;


template <typename T1, typename T2>
class is_courser
{
public:
	static constexpr bool value = !is_finer_v<T1, T2>();
};
template <typename T1, typename T2>
constexpr bool is_courser_v = is_courser<T1, T2>::value;


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

/************************************************************************************
*                                                                                   *
*   Copyright (c) 2014, 2015 Axel Menzel <info@axelmenzel.de>                       *
*                                                                                   *
*   This file is part of RTTR (Run Time Type Reflection)                            *
*   License: MIT License                                                            *
*                                                                                   *
*   Permission is hereby granted, free of charge, to any person obtaining           *
*   a copy of this software and associated documentation files (the "Software"),    *
*   to deal in the Software without restriction, including without limitation       *
*   the rights to use, copy, modify, merge, publish, distribute, sublicense,        *
*   and/or sell copies of the Software, and to permit persons to whom the           *
*   Software is furnished to do so, subject to the following conditions:            *
*                                                                                   *
*   The above copyright notice and this permission notice shall be included in      *
*   all copies or substantial portions of the Software.                             *
*                                                                                   *
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR      *
*   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,        *
*   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE     *
*   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER          *
*   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,   *
*   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE   *
*   SOFTWARE.                                                                       *
*                                                                                   *
*************************************************************************************/

#ifndef RTTR_DEFAULT_ARGUMENTS_H_
#define RTTR_DEFAULT_ARGUMENTS_H_

#include "rttr/detail/base/core_prerequisites.h"
#include "rttr/detail/misc/misc_type_traits.h"
#include "rttr/detail/misc/argument_extractor.h"
#include "rttr/detail/misc/function_traits.h"

#include <tuple>

namespace rttr
{
namespace detail
{

/*!
 * This class holds the data for the default arguments.
 */
template<typename...T>
struct default_args
{
    std::tuple<T...> m_args;
};

/////////////////////////////////////////////////////////////////////////////////////////

// This type trait will check whether a signature (given via \p Arg_list) match the \o Default_Arg_List types.
// It will check the signature recursively, beginning with the complete signature,
// then removing the first argument and check against Default_Arg_List, then removing the second...
template<typename Arg_list, typename Default_Arg_List> 
struct find_default_args_impl;

template<typename Default_Arg_List, typename T, typename...TArgs> 
struct find_default_args_impl<type_list<T, TArgs...>, Default_Arg_List>
{
    template<typename Tx>
    using decay_type = remove_cv_t<remove_reference_t< Tx >>;

    using func_args = default_args< decay_type< T>, 
                                    decay_type< TArgs>...
                                  >;

    using type = conditional_t< std::is_same<func_args, Default_Arg_List>::value,
                                Default_Arg_List,
                                typename find_default_args_impl<type_list<TArgs...>, Default_Arg_List >::type
                               >;
};

template<typename Default_Arg_List>
struct find_default_args_impl<type_list<>, Default_Arg_List>
{
    using type = default_args<>;
};

/////////////////////////////////////////////////////////////////////////////////////////

//! Workaround because msvc2013 cannot handle '<sizeof...(T)' with std::enable_if<T>.
template<typename...TArgs>
struct is_not_one_argument : std::integral_constant<bool, (sizeof...(TArgs) != 1)> {};

/////////////////////////////////////////////////////////////////////////////////////////

/*!
 * This helper struct is needed to call `find_default_args_t` with a function signature,
 * or a constructor signature (a list of arguments), i.e. I have only one interface to invoke.
 *
 * This avoid code distinction in `has_default_types`or `get_default_args`.
 */
template<typename Default_Arg_List, typename T, typename Enable = void>
struct find_default_args_helper;

// is function
template<typename Default_Arg_List, typename T>
struct find_default_args_helper<Default_Arg_List, type_list<T>, enable_if_t<is_function<T>::value>>
:   find_default_args_impl< as_type_list_t< typename function_traits<T>::arg_types >, Default_Arg_List>
{
};

// is ctor with one argument
template<typename Default_Arg_List, typename T>
struct find_default_args_helper<Default_Arg_List, type_list<T>, enable_if_t<!is_function<T>::value>>
:   find_default_args_impl<type_list<T>, Default_Arg_List>
{
};

// is ctor with zero or more then one argument
template<typename Default_Arg_List, typename...TArgs>
struct find_default_args_helper<Default_Arg_List, type_list<TArgs...>, enable_if_t< is_not_one_argument<TArgs...>::value >>
:   find_default_args_impl<type_list<TArgs...>, Default_Arg_List>
{
};


/*!
 * This type traits will check whether it is possible to call
 * a function type or a list of argument types (e.g. constructor arguments)
 * depending on a list of default argument types, \p Default_Arg_List.
 * When it is possible \p Default_Arg_List will be returned as type;
 * Otherwise an empty 'default_args<>' will be returned
 *
 * e.g.:
 * ctor_list => <bool, int, double>;    Default_Arg_List => default_args<bool, int, double> will return:    default_args<bool, int, double>
 * ctor_list => <bool, int, double>;    Default_Arg_List => default_args<int, double>       will return:    default_args<int, double>
 * ctor_list => <bool, int, double>;    Default_Arg_List => default_args<double>            will return:    default_args<double>
 * ctor_list => <bool, int, double>;    Default_Arg_List => default_args<int>               will return:    default_args<> (cannot be called; right most argument is missing)
 */
template<typename Default_Arg_List, typename... TArgs>
using find_default_args_t = typename find_default_args_helper<Default_Arg_List, type_list<TArgs...>>::type;


/////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
struct is_def_type : std::false_type { };

template<typename... TArgs>
struct is_def_type<default_args<TArgs...> > : std::true_type { };

/////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
struct get_default_args_impl;

template<typename T, typename...TArgs>
struct get_default_args_impl<type_list<T, TArgs...>>
{
    using type = conditional_t< is_def_type<T>::value,
                                T,
                                typename get_default_args_impl<type_list<TArgs...>>::type
                               >;
};

template<>
struct get_default_args_impl<type_list<>>
{
    using type = default_args<>;
};

//! This type trait will return the first type which matches the template type `default_args<T...>`
template<typename...TArgs>
using get_default_args_t = typename get_default_args_impl< type_list< TArgs... > >::type;

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

/*!
 * Evaluates to 'std::true_type' when the given arguments contains one 'def_args<T...>' argument;
 * Otherwise to 'std::false_type'
 */
template<typename...Args>
using has_default_args = conditional_t<std::is_same<get_default_args_t<Args...>, default_args<>>::value, std::false_type, std::true_type>;

template<typename T1, typename T2>
struct has_default_types;


/*!
 * Evaluates to 'std::true_type' when the given arguments contains one valid 'def_args<T...>' argument,
 * that can be used to invoke the signature \p Acc_Args;
 * Otherwise to 'std::false_type'
 */
template<typename... Acc_Args, typename... TArgs>
struct has_default_types<type_list<Acc_Args...>, type_list<TArgs...>> 
:   conditional_t<std::is_same<find_default_args_t<get_default_args_t<TArgs...>, Acc_Args...>, default_args<>>::value,
                  std::false_type, std::true_type>
{
};

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

template<typename T, typename Enable = void>
struct count_default_args_impl;

template<>
struct count_default_args_impl<type_list<>>
{
    static const std::size_t value = 0;
};

template<typename T, typename...TArgs>
struct count_default_args_impl<type_list<T, TArgs...>, enable_if_t<is_def_type<T>::value>>
{
    static const std::size_t value = count_default_args_impl<type_list<TArgs...>>::value + 1;
};

template<typename T, typename...TArgs>
struct count_default_args_impl<type_list<T, TArgs...>, enable_if_t<!is_def_type<T>::value>>
{
    static const std::size_t value = count_default_args_impl<type_list<TArgs...>>::value;
};

/////////////////////////////////////////////////////////////////////////////////////////
// returns the number of types 'default_args<T...>' provided in the given list of arguments TArgs...
template<typename...TArgs>
using count_default_args = count_default_args_impl< type_list< raw_type_t<TArgs>... > >;

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

template<typename... TArgs, typename... Args, typename Default_Type = find_default_args_t<get_default_args_t<Args...>, TArgs...> >
static RTTR_INLINE 
enable_if_t< std::is_same<Default_Type, default_args<>>::value, Default_Type>
get_default_args(Args&&... arg)
{
    return default_args<>(); // no default arguments provided
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename...TArgs, typename... Args, typename Default_Type = find_default_args_t<get_default_args_t<Args...>, TArgs...> >
static RTTR_INLINE 
enable_if_t< !std::is_same<Default_Type, default_args<>>::value, Default_Type> 
get_default_args(Args&&... arg)
{
    // default arguments are provided, extract them
    auto result = forward_to_vector<Default_Type>(std::forward<Args>(arg)...);
    if (result.size() != 0)
        return result[0];
    else
        return Default_Type();
}

/////////////////////////////////////////////////////////////////////////////////////////

} // end namespace detail
} // end namespace rttr

#endif // RTTR_DEFAULT_ARGUMENTS_H_

//
// Created by yuan on 1/15/24.
//
#pragma once

#ifndef CPPPLAYGROUND_COUNTER_H
#define CPPPLAYGROUND_COUNTER_H

#include <concepts>
#include <type_traits>

namespace nonconstant_flag
{
auto flag(int);     // E1


template <bool B> requires (!B)
struct setter {
    friend auto flag(int) { }        // E2

    static constexpr bool b = B;
};


// E3
template <bool FlagVal>
[[nodiscard]]
consteval auto nonconstant_constant_impl()
{
    if constexpr (FlagVal) {
        return true;
    } else {
        // E3.1
        setter<FlagVal> s;
        return s.b;
    }
}


// E4
template <
    auto Arg = 0,
    bool FlagVal = requires { flag(Arg); },             // E4.1
    auto Val = nonconstant_constant_impl<FlagVal>()     // E4.2
>
constexpr auto nonconstant_constant = Val;

}

static_assert(nonconstant_flag::nonconstant_constant<> == 0);
static_assert(nonconstant_flag::nonconstant_constant<> == 1);

namespace constexpr_counter
{

template <unsigned N>
struct reader {
    friend auto counted_flag(reader<N>);        // E1
};


template <unsigned N>
struct setter {
    friend auto counted_flag(reader<N>) { }      // E2

    static constexpr unsigned n = N;
};


// E3
template <
    auto Tag,               // E3.1
    unsigned NextVal = 0
>
[[nodiscard]]
consteval auto counter_impl()
{
    constexpr bool counted_past_value = requires(reader<NextVal> r) {
        counted_flag(r);
    };

    if constexpr (counted_past_value) {
        return counter_impl<Tag, NextVal + 1>();            // E3.2
    } else {
        // E3.3
        setter<NextVal> s;
        return s.n;
    }
}


template <
    auto Tag = [] { },                // E4
    auto Val = counter_impl<Tag>()
>
constexpr auto counter = Val;

}

static_assert(constexpr_counter::counter<> == 0);
static_assert(constexpr_counter::counter<> == 1);
static_assert(constexpr_counter::counter<> == 2);

namespace mutable_static_list
{

// E1
template <typename...>
struct type_list {
};


// E2
template <class TypeList, typename T>
struct type_list_append;

template <typename... Ts, typename T>
struct type_list_append<type_list<Ts...>, T> {
    using type = type_list<Ts..., T>;
};


// E3
template <unsigned N, typename List>
struct state_t {
    static constexpr unsigned n = N;
    using list = List;
};


namespace
{
struct tu_tag {
};           // E4
}


template <
    unsigned N,
    std::same_as<tu_tag> TUTag
>
struct reader {
    friend auto state_func(reader<N, TUTag>);
};


template <
    unsigned N,
    typename List,
    std::same_as<tu_tag> TUTag
>
struct setter {
    // E5
    friend auto state_func(reader<N, TUTag>)
    {
        return List{};
    }

    static constexpr state_t<N, List> state{};
};


template
struct setter<0, type_list<>, tu_tag>;     // E6


// E7
template <
    std::same_as<tu_tag> TUTag,
    auto EvalTag,
    unsigned N = 0
>
[[nodiscard]]
consteval auto get_state()
{
    constexpr bool counted_past_n = requires(reader<N, TUTag> r) {
        state_func(r);
    };

    if constexpr (counted_past_n) {
        return get_state<TUTag, EvalTag, N + 1>();
    } else {
        // E7.1
        constexpr reader<N - 1, TUTag> r;
        return state_t<N - 1, decltype(state_func(r))>{};
    }
}


// E8
template <
    std::same_as<tu_tag> TUTag = tu_tag,
    auto EvalTag = [] { },
    auto State = get_state<TUTag, EvalTag>()
>
using get_list = typename std::remove_cvref_t<decltype(State)>::list;


// E9
template <
    typename T,
    std::same_as<tu_tag> TUTag,
    auto EvalTag
>
[[nodiscard]]
consteval auto append_impl()
{
    using cur_state = decltype(get_state<TUTag, EvalTag>());            // E9.1
    using cur_list = typename cur_state::list;
    using new_list = typename type_list_append<cur_list, T>::type;      // E9.2
    setter<cur_state::n + 1, new_list, TUTag> s;                        // E9.3
    return s.state;                                                     // E9.4
}


// E10
template <
    typename T,
    std::same_as<tu_tag> TUTag = tu_tag,
    auto EvalTag = [] { },
    auto State = append_impl<T, TUTag, EvalTag>()
>
constexpr auto append = [] { return State; };           // E10.1

template <typename Derived>
struct BaseOperator {
private:
    static constexpr auto dummy = append<Derived>();
};

static_assert(std::same_as<get_list<>, type_list<>>);

struct Op1 : public BaseOperator<Op1> {
};

static_assert(std::same_as<get_list<>, type_list<Op1>>);

struct Op2 : public BaseOperator<Op2> {
};

static_assert(std::same_as<get_list<>, type_list<Op1, Op2>>);

inline void test() {
//    append<int>();
//    static_assert(std::same_as<get_list<>, type_list<int>>);
//    append<double>();
//    static_assert(std::same_as<get_list<>, type_list<int, double>>);
}
//static_assert(get_list<>);

}

#endif //CPPPLAYGROUND_COUNTER_H

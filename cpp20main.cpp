#pragma once

#include <iostream>
#include <string>
#include <algorithm>
#include <format>
#include <type_traits>
#include <array>
#include <any>
#include <unordered_map>
#include <tuple>
#include <utility>
#include <string_view>
#include <memory>

template <typename List, auto NewVal, typename = void>
struct Prepend;

template <typename List, auto NewVal>
using Prepend_t = typename Prepend<List, NewVal>::type;

template <typename List, auto NewVal, typename = void>
struct Append;

template <typename List, auto NewVal>
using Append_t = typename Append<List, NewVal>::type;

template <size_t I, auto Val, auto... Vals>
struct GetImpl : public GetImpl<I - 1, Vals...> {
};

template <auto Val, auto... Vals>
struct GetImpl<0, Val, Vals...> {
    static constexpr auto value = Val;
};

template <typename List, size_t I, typename = void>
struct Get;

template <typename List, size_t I>
constexpr auto Get_v = Get<List, I>::value;

template <typename List>
using Front = Get<List, 0>;

template <typename List>
constexpr auto Front_v = Front<List>::value;

template <typename List>
using Back = Get<List, List::size - 1>;

template <typename List>
constexpr auto Back_v = Back<List>::value;

template <typename T, T... Vals>
struct StaticList {
    using value_type = T;
    static constexpr auto size = sizeof...(Vals);
};

template <typename T, T NewVal, T... Vals>
struct Append<StaticList<T, Vals...>, NewVal> {
    using type = StaticList<T, Vals..., NewVal>;
};

template <typename T, T NewVal, T... Vals>
struct Prepend<StaticList<T, Vals...>, NewVal> {
    using type = StaticList<T, NewVal, Vals...>;
};

template <size_t I, typename T, T... Vals>
struct Get<StaticList<T, Vals...>, I> : public GetImpl<I, Vals...> {
};

template <auto... Vals>
struct StaticHeteroList {
    static constexpr auto size = sizeof...(Vals);
};

template <auto NewVal, auto... Vals>
struct Append<StaticHeteroList<Vals...>, NewVal> {
    using type = StaticHeteroList<Vals..., NewVal>;
};

template <template <auto...> class HeteroList, auto NewVal, auto... Vals>
struct Append<HeteroList<Vals...>, NewVal, std::enable_if_t<std::is_base_of_v<StaticHeteroList<Vals...>, HeteroList<Vals...>>>> {
using type = HeteroList<Vals..., NewVal>;
};

template <template <auto...> class HeteroList, auto NewVal, auto... Vals>
struct Prepend<HeteroList<Vals...>, NewVal, std::enable_if_t<std::is_base_of_v<StaticHeteroList<Vals...>, HeteroList<Vals...>>>> {
using type = HeteroList<NewVal, Vals...>;
};

template <template <auto...> class HeteroList, size_t I, auto... Vals>
struct Get<HeteroList<Vals...>, I, std::enable_if_t<std::is_base_of_v<StaticHeteroList<Vals...>, HeteroList<Vals...>>>> : public GetImpl<I, Vals...> {
};

template <size_t N>
struct string_literal {
    constexpr string_literal(const char (&str)[N])
    {
        std::copy_n(str, N, value);
    }
    constexpr operator auto() const { return value; }

    template <size_t X, size_t Y>
    friend constexpr bool operator==(const string_literal<X>& left, const string_literal<Y>& right);

    char value[N]{};
};

template <size_t N, size_t M>
constexpr bool operator==(const string_literal<N>& left, const string_literal<M>& right) {
    return std::equal(left.value, left.value + N, right.value, right.value + M);
}

template <string_literal... Strs>
struct StaticStringList : public StaticHeteroList<Strs...> {
};

template <string_literal... Names>
struct InputString : public StaticStringList<Names...> {
    template <string_literal Name, typename = std::enable_if_t<((Name == Names) || ...)>>
    constexpr static auto get_input() {
        return Name;
    }
};

template <typename T, typename Container, size_t I>
struct FindInContainerImpl;

template <typename T, typename U, typename... Us, template <typename...> class Container, size_t I>
struct FindInContainerImpl<T, Container<U, Us...>, I> : public FindInContainerImpl<T, Container<Us...>, I + 1> {
};

template <typename T, typename... Us, template <typename...> class Container, size_t I>
struct FindInContainerImpl<T, Container<T, Us...>, I> {
    static constexpr size_t index = I;
};

template <typename T, template <typename...> class Container, size_t I>
struct FindInContainerImpl<T, Container<>, I>;

template <typename T, typename Container>
using FindInContainer = FindInContainerImpl<T, Container, 0>;

template <string_literal Str>
void print()
{
    std::cout << std::format("size: {}, contents: {}\n", sizeof(Str.value), Str.value);
}

void test() {
    print<"blah">();

    using L = StaticStringList<"blah", "okay", "whatever">;
    using L2 = Prepend_t<L, string_literal{"foo"}>;
    using L3 = Append_t<L2, string_literal{"bar"}>;
    std::cout << Get<L, 0>::value << '\n';
    std::cout << Get<L2, 0>::value << '\n';
    std::cout << Get<L2, 3>::value << '\n';
    std::cout << Get<L3, L3::size - 1>::value << '\n';

    static_assert(string_literal{"name"} == string_literal{"name"});
    static_assert(string_literal{"name"} != string_literal{"value"});

    InputString<"blah", "wo", "whatever"> x;

    std::cout << Get_v<decltype(x), 0> << '\n';
    std::cout << Front_v<decltype(x)> << '\n';
    std::cout << Back_v<decltype(x)> << '\n';

    std::cout << x.get_input<"blah">() << '\n';
}

namespace counter {

template<unsigned N>
struct reader {
    friend auto counted_flag(reader<N>);
};


template<unsigned N>
struct setter {
    friend auto counted_flag(reader<N>) {}

    static constexpr unsigned n = N;
};


template<
    auto Tag,
    unsigned NextVal = 0
>
[[nodiscard]]
consteval auto counter_impl() {
    constexpr bool counted_past_value = requires(reader<NextVal> r) {
        counted_flag(r);
    };

    if constexpr (counted_past_value) {
        return counter_impl<Tag, NextVal + 1>();
    }
    else {
        setter<NextVal> s;
        return s.n;
    }
}


template<
    auto Tag = []{},
    auto Val = counter_impl<Tag>()
>
constexpr auto counter = Val;


void counter_test() {
    static_assert(counter<> == 0);
    static_assert(counter<> == 1);
    static_assert(counter<> == 2);
    static_assert(counter<> == 3);
    static_assert(counter<> == 4);
    static_assert(counter<> == 5);
    static_assert(counter<> == 6);
    static_assert(counter<> == 7);
    static_assert(counter<> == 8);
    static_assert(counter<> == 9);
    static_assert(counter<> == 10);
}


}

int main() {
    std::cout << counter::counter<> << '\n';
    test();
}
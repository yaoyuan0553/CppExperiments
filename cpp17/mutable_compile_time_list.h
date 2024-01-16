//
// Created by yuan on 1/16/24.
//
#pragma once

#ifndef CPPPLAYGROUND_MUTABLE_COMPILE_TIME_LIST_H
#define CPPPLAYGROUND_MUTABLE_COMPILE_TIME_LIST_H

namespace mutable_type_list
{

template <typename... Ts>
struct type_list {
};

template <class TypeList, typename T>
struct type_list_append;

template <template <typename...> class TypeListContainer, typename... Ts, typename T>
struct type_list_append<TypeListContainer<Ts...>, T> {
    using type = TypeListContainer<Ts..., T>;
};

template <int N, typename List>
struct state_t {
    static constexpr int n = N;
    using list = List;
};

struct tu_tag {
};

namespace detail
{

template <typename T>
class TypeFlag {
    struct Dummy {
        constexpr Dummy()
        {
        }

        friend constexpr void adl_flag(Dummy);
    };

    template <bool>
    struct Writer {
        friend constexpr void adl_flag(Dummy)
        {
        }
    };

    template <int N, typename TuTag>
    struct Reader {
        friend constexpr auto state_func(Reader<N, TuTag>);
    };

    template <int N, typename List, typename TuTag>
    struct Setter {
        friend constexpr auto state_func(Reader<N, TuTag>) {
            return List{};
        }
        static constexpr state_t<N, List> state{};
    };

    template <class Dummy, int = (adl_flag(Dummy{}), 0)>
    static constexpr bool check(int)
    {
        return true;
    }

    template <class Dummy>
    static constexpr bool check(short)
    {
        return false;
    }

    template <typename TuTag, int N = 0,
        int = (state_func(Reader<N, TuTag>{}), 0)>
    static constexpr bool check_state(int) {
        return true;
    }

    template <typename TuTag, int N = 0>
    static constexpr bool check_state(short) {
        return false;
    }

public:

    template <typename TuTag = tu_tag, bool Value = check_state<TuTag>(0)>
    static constexpr bool get_state_and_set() noexcept {
        Setter<Value && 0, type_list<>, tu_tag> tmp{};
        (void)tmp;
        return Value;
    }

    template <typename TuTag = tu_tag, bool Value = check_state<TuTag>(0)>
    static constexpr bool get_state() noexcept {
        return Value;
    }

    template <class Dummy = Dummy, bool Value = check<Dummy>(0)>
    static constexpr bool read_and_set()
    {
        Writer<Value && 0> tmp{};
        (void) tmp;
        return Value;
    }

    template <class Dummy = Dummy, bool Value = check<Dummy>(0)>
    static constexpr int read()
    {
        return Value;
    }
};

template <typename T>
class Flag {
    struct Dummy {
        constexpr Dummy()
        {
        }

        friend constexpr void adl_flag(Dummy);
    };

    template <bool>
    struct Writer {
        friend constexpr void adl_flag(Dummy)
        {
        }
    };

    template <class Dummy, int = (adl_flag(Dummy{}), 0)>
    static constexpr bool check(int)
    {
        return true;
    }

    template <class Dummy>
    static constexpr bool check(short)
    {
        return false;
    }

public:

    template <class Dummy = Dummy, bool Value = check<Dummy>(0)>
    static constexpr bool read_and_set()
    {
        Writer<Value && 0> tmp{};
        (void) tmp;
        return Value;
    }

    template <class Dummy = Dummy, bool Value = check<Dummy>(0)>
    static constexpr int read()
    {
        return Value;
    }
};

template <typename T, int I>
struct Tag {

    [[nodiscard]] constexpr int value() const noexcept
    {
        return I;
    }
};

template <typename T, int N, int Step, bool B>
struct Checker {
    static constexpr int current_val() noexcept
    {
        return N;
    }
};

template <typename T, int N, int Step>
struct CheckerWrapper {
    template <bool B = Flag<Tag<T, N>>{}.read(), int M = Checker<T, N, Step, B>{}.current_val()>
    static constexpr int current_val()
    {
        return M;
    }
};

template <typename T, int N, int Step>
struct Checker<T, N, Step, true> {
    template <int M = CheckerWrapper<T, N + Step, Step>{}.current_val()>
    static constexpr int current_val() noexcept
    {
        return M;
    }
};

template <typename T, int N, bool B = Flag<Tag<T, N>>{}.read_and_set()>
struct Next {
    static constexpr int value() noexcept
    {
        return N;
    }
};


inline int blah() {
    static_assert(!TypeFlag<Tag<struct What, 0>>::get_state());
    static_assert(!TypeFlag<Tag<struct What, 0>>::get_state());
    static_assert(!TypeFlag<Tag<struct What, 0>>::get_state_and_set());
    static_assert(TypeFlag<Tag<struct What, 0>>::get_state_and_set());
    static_assert(TypeFlag<Tag<struct What, 0>>::get_state());

    static_assert(!Flag<Tag<struct What, 0>>::read());
    static_assert(!Flag<Tag<struct What, 0>>::read());
    static_assert(!Flag<Tag<struct What, 0>>::read_and_set());
    static_assert(Flag<Tag<struct What, 0>>::read());
}

}

template <class Tag = void, int Start = 0, int Step = 1>
class constexpr_counter {
public:
    template <int N = detail::CheckerWrapper<Tag, Start, Step>{}.current_val()>
    static constexpr int next() noexcept {
        return detail::Next<Tag, N>{}.value();
    }

    template <int N = detail::CheckerWrapper<Tag, Start, Step>{}.current_val()>
    static constexpr int peek_next() noexcept {
        return N;
    }
};

using counter_A_0_1 = constexpr_counter<struct TagA, 0, 1>;
constexpr int a0 = counter_A_0_1::next();
constexpr int a1 = counter_A_0_1::next();
constexpr int a2 = counter_A_0_1::next();
static_assert(a0 == 0);
static_assert(a1 == 1);
static_assert(a2 == 2);
static_assert(counter_A_0_1::peek_next() == 3);
static_assert(counter_A_0_1::peek_next() == 3);
static_assert(counter_A_0_1::next() == 3);
static_assert(counter_A_0_1::peek_next() == 4);
static_assert(counter_A_0_1::next() == 4);
static_assert(counter_A_0_1::peek_next() == 5);

using counter_B_0_1 = constexpr_counter<struct TagB, 0, 1>;
constexpr int b0 = counter_B_0_1::next();
constexpr int b1 = counter_B_0_1::next();
constexpr int b2 = counter_B_0_1::next();
static_assert(b0 == 0);
static_assert(b1 == 1);
static_assert(b2 == 2);

using counter_C_2_1 = constexpr_counter<struct TagC, 2, 1>;
constexpr int c0 = counter_C_2_1::next();
constexpr int c1 = counter_C_2_1::next();
constexpr int c2 = counter_C_2_1::next();
static_assert(c0 == 2);
static_assert(c1 == 3);
static_assert(c2 == 4);

using counter_D_4_1 = constexpr_counter<struct TagD, 4, 1>;
constexpr int d0 = counter_D_4_1::next();
constexpr int d1 = counter_D_4_1::next();
constexpr int d2 = counter_D_4_1::next();
static_assert(d0 == 4);
static_assert(d1 == 5);
static_assert(d2 == 6);

using counter_E_5_3 = constexpr_counter<struct TagE, 5, 3>;
constexpr int e0 = counter_E_5_3::next();
constexpr int e1 = counter_E_5_3::next();
constexpr int e2 = counter_E_5_3::next();
static_assert(e0 == 5);
static_assert(e1 == 8);
static_assert(e2 == 11);

using counter_F_2_m3 = constexpr_counter<struct TagF, 2, -3>;
constexpr int f0 = counter_F_2_m3::next();
constexpr int f1 = counter_F_2_m3::next();
constexpr int f2 = counter_F_2_m3::next();
static_assert(f0 == 2);
static_assert(f1 == -1);
static_assert(f2 == -4);

}

#endif //CPPPLAYGROUND_MUTABLE_COMPILE_TIME_LIST_H

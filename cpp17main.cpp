#include "cpp17/counter.h"

#include <iostream>
#include <string>
#include <algorithm>
#include <type_traits>
#include <array>
#include <any>
#include <unordered_map>
#include <tuple>
#include <utility>
#include <string_view>
#include <memory>

template <std::size_t...Idxs>
constexpr auto substring_as_array(std::string_view str, std::index_sequence<Idxs...>)
{
    return std::array{str[Idxs]..., '\n'};
}

template <typename T>
constexpr auto type_name_array()
{
#if defined(__clang__)
    constexpr auto prefix   = std::string_view{"[T = "};
    constexpr auto suffix   = std::string_view{"]"};
    constexpr auto function = std::string_view{__PRETTY_FUNCTION__};
#elif defined(__GNUC__)
    constexpr auto prefix   = std::string_view{"with T = "};
  constexpr auto suffix   = std::string_view{"]"};
  constexpr auto function = std::string_view{__PRETTY_FUNCTION__};
#elif defined(_MSC_VER)
  constexpr auto prefix   = std::string_view{"type_name_array<"};
  constexpr auto suffix   = std::string_view{">(void)"};
  constexpr auto function = std::string_view{__FUNCSIG__};
#else
# error Unsupported compiler
#endif

    constexpr auto start = function.find(prefix) + prefix.size();
    constexpr auto end = function.rfind(suffix);

    static_assert(start < end);

    constexpr auto name = function.substr(start, (end - start));
    return substring_as_array(name, std::make_index_sequence<name.size()>{});
}

template <typename T>
struct type_name_holder {
    static inline constexpr auto value = type_name_array<T>();
};

template <typename T>
constexpr auto type_name() -> std::string_view
{
    constexpr auto& value = type_name_holder<T>::value;
    return std::string_view{value.data(), value.size()};
}

template <size_t I, typename T, typename... Us>
struct FindImpl;

template <size_t I, typename T, typename U, typename... Us>
struct FindImpl<I, T, U, Us...> : public FindImpl<I + 1, T, Us...> {
};

template <size_t I, typename T, typename... Us>
struct FindImpl<I, T, T, Us...> {
    static constexpr size_t index = I;
};

template <typename T, typename... Ts>
using Find = FindImpl<0, T, Ts...>;


//template <typename Derived, typename T>
//struct Feature {
//    using type = T;
//    static constexpr const char* name() {
//    }
//private:
//};

template <typename Key, typename Value, std::size_t Size>
struct StaticMap {
    std::array<std::pair<Key, Value>, Size> data;

    [[nodiscard]] constexpr Value at(const Key& key) const {
        const auto iter = std::find_if(data.begin(), data.end(),
                                       [&key](const auto& v) { return v.first == key; });
        if (iter != data.end())
            return iter->second;
        else
            throw std::range_error("Not Found");
    }
};

namespace feature_list
{

struct FeatureTag {
};

template <typename T>
constexpr bool is_feature_v = std::is_base_of_v<FeatureTag, T>;

template <typename Derived>
using feature_counter = constexpr_counter<std::conditional_t<is_feature_v<Derived>, struct FeatureCounter, Derived>, 0, 1>;

template <typename Derived>
struct Feature : public FeatureTag {
    static constexpr int label = feature_counter<Derived>::next();
};


}

#define DEFINE_FEATURE(feature_name, ...) \
struct feature_name : public Feature<feature_name> {    \
    static constexpr const char name[] = #feature_name; \
    using type = __VA_ARGS__; \
    static constexpr const char type_desc[] = #feature_name; \
}

namespace feature_list
{

struct uid : public Feature<uid> {
    static constexpr const char name[] = "uid";
    using type = int64_t;
    static constexpr const char type_desc[] = "uid";
//    static constexpr int label = feature_counter::next();
//    static constexpr StaticMap<std::string_view,
};

//struct item_id : public Feature {
//    static constexpr const char name[] = "item_id";
//    using type = int64_t;
//    static constexpr const char type_desc[] = "item_id";
//};

DEFINE_FEATURE(item_id, int64_t);

DEFINE_FEATURE(fid, int64_t);

}

using Session = std::unordered_map<std::string, std::any>;

template <typename... Ts>
struct Input {
    static_assert((feature_list::is_feature_v<Ts> && ...), "type T must be a Feature");

//    template <typename T, typename = std::enable_if_t<(std::is_same_v<T, Ts> || ...)>>
    template <typename T>
    constexpr static const auto* get_input_ptr(const Session& session) {
        static_assert((std::is_same_v<T, Ts> || ...), "Feature must be one of the declared Inputs");
        if (auto it = session.find(T::name); it != session.end())
            return std::any_cast<typename T::type>(&it->second);
        else
            return static_cast<typename T::type*>(nullptr);
    }

    template <typename T>
    constexpr const auto& get_input(const Session& session, typename T::type default_value) {
        const auto* ptr = get_input_ptr<T>(session);
        if (ptr)
            return *ptr;
        else
            return (std::get<Find<T, Ts...>::index>(default_values) = std::move(default_value));
    }

    std::tuple<typename Ts::type...> default_values;
};

template <typename... Ts>
struct Output {
    static_assert((feature_list::is_feature_v<Ts> && ...), "type T must be a Feature");

//    template <typename T, typename = std::enable_if_t<(std::is_same_v<T, Ts> || ...)>>
    template <typename T>
    constexpr static auto* get_output_ptr(Session& session) {
        static_assert((std::is_same_v<T, Ts> || ...), "Feature must be one of the declared Outputs");
        if (auto it = session.find(T::name); it != session.end())
            return std::any_cast<typename T::type>(&it->second);
        else
            return nullptr;
    }

    template <typename T>
    constexpr static auto& get_output(Session& session) {
        static_assert((std::is_same_v<T, Ts> || ...), "Feature must be one of the declared Outputs");
        return *std::any_cast<typename T::type>(&session.emplace(T::name, typename T::type{}).first->second);
    }
};

struct Operator {
    virtual bool process(const Session& input, Session& output) = 0;
    virtual ~Operator() = default;
};

//static std::unordered_map<std::string, std::unique_ptr<Operator>> registry;
std::unordered_map<std::string, std::unique_ptr<Operator>>& get_registry() {
    static std::unordered_map<std::string, std::unique_ptr<Operator>> registry;
    return registry;
}

template <typename Derived, typename Input, typename Output>
class BaseOperator : public Operator, Input, Output {
public:
    BaseOperator() {
        (void)reg_;
    }

    bool process(const Session& input, Session& output) final {
        if (!setup_input(input))
            return false;
        if (!setup_output(output))
            return false;
        if (!run())
            return false;
        return true;
    }

    virtual bool run() = 0;

protected:
    virtual bool setup_input(const Session& inputs) {
        inputs_ = &inputs;
        return true;
    }
    virtual bool setup_output(Session& outputs) {
        outputs_ = &outputs;
        return true;
    }

    template <typename T>
    constexpr const auto& get_input(typename T::type&& default_value = typename T::type{}) {
        return Input::template get_input<T>(*inputs_, std::forward<typename T::type>(default_value));
    }

    template <typename T>
    constexpr auto& get_output() {
        return Output::template get_output<T>(*outputs_);
    }

private:
    const Session* inputs_ = nullptr;
    Session* outputs_ = nullptr;

    static bool reg_;

    static bool init() {
        std::cout << "name: " << type_name<Derived>() << '\n';
//        auto x =  std::make_unique<Derived>();
//        get_registry()[std::string{type_name<Derived>()}] = std::make_unique<Derived>();
//        return true;
        return get_registry().emplace(std::string{type_name<Derived>()}, std::make_unique<Derived>()).second;
    }
};

template <typename Derived, typename Input, typename Output>
bool BaseOperator<Derived, Input, Output>::reg_ = BaseOperator<Derived, Input, Output>::init();

//__attribute__((constructor)) static void MONARCHGlobalInit_12() {
//}


namespace what
{
class SomeOp :
    public BaseOperator<
        SomeOp,
        Input<feature_list::uid>,
        Output<feature_list::item_id>> {
    bool run() override
    {
        std::cout << "without default: " << get_input<feature_list::uid>() << '\n';
        std::cout << "with default 434: " << get_input<feature_list::uid>(434) << '\n';
        auto& item_id = get_output<feature_list::item_id>();
        item_id = 5555;
        return true;
    }
};

}

int main()
{
    static_assert(feature_list::uid::label == 0);
    static_assert(feature_list::uid::label == 0);
    static_assert(feature_list::item_id::label == 1);
    static_assert(feature_list::fid::label == 2);

    Session session;
    session.emplace("blah", 213).first->second;
    session[feature_list::uid::name] = feature_list::uid::type{123};
//    Input<feature_list::uid> x;
//
//    std::cout << x.get_input<feature_list::uid>(session) << '\n';

    std::cout << "registry size: " << get_registry().size() << '\n';
//    what::SomeOp op;
//
//    std::cout << session.count(feature_list::item_id::name) << '\n';
//
//    op.process(session, session);
//
//    if (auto it = session.find(feature_list::item_id::name); it != session.end())
//        std::cout << std::any_cast<feature_list::item_id::type>(it->second) << '\n';
//    else
//        std::cout << "not found\n";
//    std::cout << "registry size: " << get_registry().size() << '\n';

    return 0;
}

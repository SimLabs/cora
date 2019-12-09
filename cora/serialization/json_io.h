//#define RAPIDJSON_HAS_STDSTRING 1

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/error/en.h>

#include "cora/reflection/reflection.h"

#include <stack>
#include <optional>
#include <sstream>

namespace json_io
{

struct parse_error : std::runtime_error
{
    using std::runtime_error::runtime_error;
};

using json_value_type = rapidjson::Document::ValueType;
using std::string;

namespace detail
{
    struct json_read_processor;

    template<class Allocator = rapidjson::MemoryPoolAllocator<>>
    struct json_write_processor;

    inline rapidjson::Document read_stream_doc(std::istream&);
    inline void write_stream_doc(std::ostream& s, rapidjson::Document& doc, bool pretty);
}

template<class T>
void read_stream(std::istream& s, T& obj)
{
    using namespace detail;
    auto doc = read_stream_doc(s);
    json_read_processor proc(doc);
    reflect(proc, obj);
}

template<class T>
void write_stream(std::ostream& s, T const& obj, bool pretty)
{
    using namespace detail;
    rapidjson::Document doc;
    json_write_processor<> proc(doc);
    reflect(proc, obj);
    write_stream_doc(s, doc, pretty);
}

template<class T>
std::string data_to_string(T const& obj, bool pretty = false)
{
    std::ostringstream ss;
    write_stream(ss, obj, pretty);
    return ss.str();
}

template<class T>
void string_to_data(std::string const& s, T& obj)
{
    std::istringstream ss(s);
    read_stream(ss, obj);
}

}

namespace json_io::detail
{

namespace traits
{
    template<typename T>
    struct is_container_like
    {
        typedef typename std::remove_const<T>::type test_type;

        template<typename A>
        static constexpr bool test(
            A * pt,
            A const * cpt = nullptr,
            decltype(pt->begin()) * = nullptr,
            decltype(pt->end()) * = nullptr,
            decltype(cpt->begin()) * = nullptr,
            decltype(cpt->end()) * = nullptr,
            typename A::iterator * pi = nullptr,
            typename A::const_iterator * pci = nullptr,
            typename A::value_type * pv = nullptr) {

            typedef typename A::iterator iterator;
            typedef typename A::const_iterator const_iterator;
            typedef typename A::value_type value_type;
            return  std::is_same<decltype(pt->begin()), iterator>::value &&
                std::is_same<decltype(pt->end()), iterator>::value &&
                std::is_same<decltype(cpt->begin()), const_iterator>::value &&
                std::is_same<decltype(cpt->end()), const_iterator>::value;
        }

        template<typename A>
        static constexpr bool test(...) {
            return false;
        }

        static const bool value = test<test_type>(nullptr);
    };

    enum struct direction_t {
        read, write, read_write
    };

    template<class T, direction_t Direction>
    struct is_string_like;

    template<class T>
    struct is_string_like<T, direction_t::read> : std::integral_constant<bool, std::is_convertible_v<string, T>>
    {};

    template<class T>
    struct is_string_like<T, direction_t::write> : std::integral_constant<bool, std::is_convertible_v<T, string>>
    {};

    template<class T>
    struct is_string_like<T, direction_t::read_write> : std::integral_constant<bool, is_string_like<T, direction_t::read>::value && is_string_like<T, direction_t::write>::value>
    {};

    template<class T, direction_t Direction>
    struct is_string_key_value
    {
        typedef typename std::remove_const<T>::type test_type;

        template<typename A>
        static constexpr bool test(A * pt,
            typename A::value_type::first_type * p_first = nullptr
        )
        {
            using key_type = std::decay_t<decltype(*p_first)>;
            return is_string_like<key_type, Direction>::value;
        }

        template<typename A>
        static constexpr bool test(...) {
            return false;
        }

        static const bool value = test<test_type>(nullptr);
    };

    template<class T, direction_t Direction = direction_t::read_write>
    struct is_leaf_type: std::integral_constant<bool, 
        std::is_arithmetic_v<T> || 
        std::is_same_v<std::decay_t<T>, bool> ||
        is_string_like<T, Direction>::value
    >{};

    template<class T, direction_t Direction = direction_t::read_write>
    struct is_json_array: std::integral_constant<bool, is_container_like<T>::value && !is_string_key_value<T, Direction>::value>
    {};

    template<class T, direction_t Direction = direction_t::read_write>
    struct is_json_map: std::integral_constant<bool, is_container_like<T>::value && is_string_key_value<T, Direction>::value>
    {};

    template<class Type>
    struct is_optional : std::integral_constant<bool, false> {};

    template<class Type>
    struct is_optional<std::optional<Type>> : std::integral_constant<bool, true> {};
}


rapidjson::Document read_stream_doc(std::istream& s)
{
    using namespace rapidjson;
    IStreamWrapper isw(s);
    Document d;
    d.ParseStream(isw);
    if(d.HasParseError())
    {
        throw parse_error(GetParseError_En(d.GetParseError()));
    }
    return d;
}

void write_stream_doc(std::ostream& s, rapidjson::Document& doc, bool pretty)
{
    using namespace rapidjson;
    OStreamWrapper osw(s);
    if(pretty)
    {
        PrettyWriter<OStreamWrapper> writer(osw);
        doc.Accept(writer);
    }
    else
    {
        Writer<OStreamWrapper> writer(osw);
        doc.Accept(writer);
    }
}


struct json_read_processor
{
    static constexpr traits::direction_t direction = traits::direction_t::read;

    json_read_processor(json_value_type const& document)
        : json_(&document)
    {
    }

    template<class T>
    void process_value(T& v, json_value_type const& json)
    {
        if constexpr(traits::is_optional<T>::value)
        {
            if(json.IsNull())
                v = T();
            else
            {
                v = std::decay_t<decltype(*v)>();
                process_value(*v, json);
            }
        }
        else if constexpr(traits::is_leaf_type<T, direction>::value)
        {
            if constexpr(std::is_integral_v<T>)
            {
                // promote short types cause there are no functions for them
                assert(json.Is<decltype(v * 1)>());
                v = json.Get<decltype(v * 1)>();
            }
            else if constexpr(traits::is_string_like<T, direction>::value)
            {
                assert(json.IsString());
                v = string(json.GetString());
            }
            else
            {
                assert(json.Is<T>());
                v = json.Get<T>();
            }
        }
        else if constexpr(traits::is_json_map<T, direction>::value)
        {
            assert(json.IsObject());
            for(auto& m : json.GetObject())
            {
                typename T::value_type::second_type val;
                process_value(val, m.value);
                v.emplace(string(m.name.GetString()), std::move(val));
            }
        }
        else if constexpr(traits::is_json_array<T, direction>::value)
        {
            assert(json.IsArray());
            for(auto& array_json : json.GetArray())
            {
                typename T::value_type val;
                process_value(val, array_json);
                v.insert(v.end(), std::move(val));
            }
        }
        else
        {
            assert(json.IsObject());
            json_read_processor pc(json);
            reflect(pc, v);
        }
    }

    template<class T>
    void operator()(T& v, const char* key)
    {
        assert(get_current_json().IsObject());
        using current_value_type = T;
        auto it = get_current_json().FindMember(key);
        if(it == get_current_json().MemberEnd()) 
        {
            v = T();
            return;
        }

        process_value(v, it->value);
    }

    const json_value_type& get_current_json() const
    {
        return *json_;
    }

  private:
    const json_value_type* json_;
};

template<class Allocator>
struct json_write_processor
{
    static constexpr traits::direction_t direction = traits::direction_t::write;

    json_write_processor(json_value_type& document)
        : values_stack_{ {&document} }
    {
        document.SetObject();
    }

    template<class T>
    void process_value(T const& v, json_value_type& json)
    {
        if constexpr(traits::is_optional<T>::value)
        {
            if(!v)
                json.SetNull();
            else
            {
                json_value_type val;
                process_value(*v, val);
                json = std::move(val);
            }
        }
        else if constexpr(traits::is_leaf_type<T, direction>::value)
        {
            if constexpr(traits::is_string_like<T, direction>::value)
                json.SetString(string(v).c_str(), get_alloc());
            else if constexpr (std::is_integral_v<T>)
            {
                // promote short types cause there are no functions for them
                json.Set<decltype(v * 1)>(v * 1);
            }
            else
            {
                json.Set<T>(v);
            }
        }
        else if constexpr(traits::is_json_map<T, direction>::value)
        {
            json.SetObject();
            for(auto& field : v)
            {
                json_value_type key, val;
                process_value(field.first, key);
                process_value(field.second, val);
                json.AddMember(std::move(key), std::move(val), get_alloc());
            }
        }
        else if constexpr(traits::is_json_array<T, direction>::value)
        {
            json.SetArray();
            json.Reserve(rapidjson::SizeType(v.size()), get_alloc());
            for(auto const& elem : v)
            {
                json_value_type val;
                process_value(elem, val);
                json.PushBack(std::move(val), get_alloc());
            }
        }
        else
        {
            json.SetObject();
            values_stack_.push(&json);
            reflect(*this, v);
            values_stack_.pop();
        }
    }

    template<class T>
    void operator()(T const& v, const char* key)
    {
        json_value_type json;
        json_value_type json_key;
        json_key.SetString(key, get_alloc());
        process_value(v, json);
        get_current_json().AddMember(std::move(json_key), std::move(json), get_alloc());
    }

    json_value_type& get_current_json() const
    {
        assert(!values_stack_.empty());
        return *values_stack_.top();
    }

    Allocator& get_alloc()
    {
        return alloc_;
    }

private:
    Allocator alloc_;
    std::stack<json_value_type*> values_stack_;
};

}



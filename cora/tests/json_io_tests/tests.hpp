#include "cora/reflection/reflection.h"
#include "cora/serialization/json_io.h"

#include <gtest/gtest.h>
#include <random>

using namespace std;

struct basic_data_types_t
{
    bool b;
    int  i;
    float f;
    double d;
    std::string s;

    REFL_INNER(basic_data_types_t)
        REFL_ENTRY(b)
        REFL_ENTRY(i)
        REFL_ENTRY(f)
        REFL_ENTRY(d)
        REFL_ENTRY(s)
    REFL_END()
};

struct struct_diff_proc: cora::reflection::processor2
{
    template<class Type>
    void operator()(Type const& lobj, Type const& robj, const char* key)
    {
        if constexpr(std::is_floating_point_v<Type>)
        {
            if constexpr(std::is_same_v<Type, double>)
                EXPECT_DOUBLE_EQ(lobj, robj);
            else
                EXPECT_FLOAT_EQ(lobj, robj);
        }
        else if constexpr(json_io::detail::traits::is_leaf_type<Type>::value)
        {
            EXPECT_EQ(lobj, robj) << "'" << key << "'" << " field differs";
        }
        else if constexpr(json_io::detail::traits::is_optional<Type>::value)
        {
            EXPECT_TRUE(bool(lobj) == bool(robj)) << "both optionals must be true or false";
            if(bool(lobj) == bool(robj) && lobj)
            {
                (*this)(*lobj, *robj, "value");
            }
        }
        else if constexpr(json_io::detail::traits::is_json_array<Type>::value)
        {
            EXPECT_EQ(lobj.size(), robj.size()) << "sizes differ";
            if(lobj.size() == robj.size())
            {
                auto it_l = lobj.begin();
                auto it_r = robj.begin();
                unsigned i = 0;
                while(it_l != lobj.end())
                {
                    (*this)(*it_l++, *it_r++, to_string(i++).c_str());
                }
            }
        } 
        else if constexpr(json_io::detail::traits::is_json_map<Type>::value)
        {
            EXPECT_EQ(lobj.size(), robj.size()) << "sizes differ";
            if(lobj.size() == robj.size())
            {
                for_each(lobj.begin(), lobj.end(), [&robj, this](auto const& p)
                {
                    auto it = robj.find(p.first);
                    EXPECT_NE(it, robj.end()) << "key " << "'" << p.first << " is absent in robj";
                    if(it != robj.end())
                        (*this)(p.second, it->second, p.first.c_str());
                });
            }
        }
        else
        {
            reflect2(*this, lobj, robj);
        }
    }
};

static std::mt19937 rnd_gen(5);

template<class T>
void create_random_int(T& v)
{
    v = uniform_int_distribution<T>()(rnd_gen);
}

template<class T>
void create_random_real(T& v)
{
    v = uniform_real_distribution<T>()(rnd_gen);
}

string create_random_string(size_t len = 8)
{
    string result(len, ' ');
    std::generate(result.begin(), result.end(), [](){  return (char) uniform_int_distribution<short>('a', 'z')(rnd_gen); });
    return result;
}

void create_random_bool(bool& v)
{
    unsigned short c;
    create_random_int(c);
    v = c < std::numeric_limits<decltype(c)>::max() / 2;
}

basic_data_types_t create_basic_types()
{
    basic_data_types_t r;
    create_random_bool(r.b);
    create_random_int(r.i);
    create_random_real(r.f);
    create_random_real(r.d);
    r.s = create_random_string();
    return r;
}

TEST(json_io, primitive_types)
{
    auto original = create_basic_types();
    auto json = json_io::data_to_string(original);
    decltype(original) parsed;
    json_io::string_to_data(json, parsed);
    struct_diff_proc proc;
    reflect2(proc, original, parsed);
}

struct with_array
{
    vector<basic_data_types_t> array;

    REFL_INNER(with_array)
        REFL_ENTRY(array)
    REFL_END()
};

TEST(json_io, test_array)
{
    auto original = with_array();

    for(int i = 0; i < 100; ++i)
    {
        original.array.emplace_back(create_basic_types());
    }

    auto json = json_io::data_to_string(original);
    decltype(original) parsed;
    json_io::string_to_data(json, parsed);
    struct_diff_proc proc;
    reflect2(proc, original, parsed);
}

struct with_map
{
    map<string, basic_data_types_t> m;

    REFL_INNER(with_map)
        REFL_ENTRY(m)
    REFL_END()
};

TEST(json_io, test_map)
{
    auto original = with_map();

    for(int i = 0; i < 100; ++i)
    {
        original.m.emplace(create_random_string(), create_basic_types());
    }

    auto json = json_io::data_to_string(original);
    decltype(original) parsed;
    json_io::string_to_data(json, parsed);
    struct_diff_proc proc;
    reflect2(proc, original, parsed);
}

struct with_optional
{
    optional<int> opt1;
    optional<short> opt2;

    REFL_INNER(with_optional)
        REFL_ENTRY(opt1)
        REFL_ENTRY(opt2)
    REFL_END()
};

TEST(json_io, test_optional)
{
    with_optional obj;
    obj.opt2 = 341;
    auto json = json_io::data_to_string(obj);
    EXPECT_EQ(json, "{\"opt1\":null,\"opt2\":341}");
    with_optional parsed;
    parsed.opt1 = 228;
    json_io::string_to_data(json, parsed);
    EXPECT_TRUE(parsed.opt2 && *parsed.opt2 == 341);
    EXPECT_FALSE(parsed.opt1);
}

TEST(json_io, field_not_present_in_json_is_reset)
{
    with_optional obj;
    obj.opt1 = 100;
    obj.opt2 = 500;
    string json = "{\"opt1\":null}";
    json_io::string_to_data(json, obj);
    EXPECT_FALSE(obj.opt1);
    EXPECT_FALSE(obj.opt2);
}

TEST(json_io, test_throws_parse_error)
{
    string json = "{not json}";
    with_optional obj;
    EXPECT_THROW(json_io::string_to_data(json, obj), json_io::parse_error);
}

struct complex_t
{
    map<string, vector<optional<basic_data_types_t>>> foo;

    REFL_INNER(complex_t)
        REFL_ENTRY(foo)
    REFL_END()
};

TEST(json_io, complex_json)
{
    complex_t original;
    original.foo = {
        {"foo", {create_basic_types(), nullopt, create_basic_types()}},
        {"bar", {nullopt, create_basic_types(), nullopt}}
    };
    auto json = json_io::data_to_string(original);
    complex_t parsed;
    json_io::string_to_data(json, parsed);
    struct_diff_proc proc;
    reflect2(proc, original, parsed);
}

struct with_vector_bool
{
    vector<bool> v_of_b;

    REFL_INNER(with_vector_bool)
        REFL_ENTRY(v_of_b)
    REFL_END()
};

TEST(json_io, test_vector_of_bool)
{
    with_vector_bool original = {{false, true, false}};
    auto json = json_io::data_to_string(original);
    with_vector_bool parsed;
    json_io::string_to_data(json, parsed);
    struct_diff_proc proc;
    reflect2(proc, original, parsed);
}

#pragma once

#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <sstream>

#include "cora/reflection/reflection.h"

namespace cora
{
namespace csv_io
{

    namespace detail
    {
        template<typename S, typename T, typename = void>
        struct is_to_stream_writable : std::false_type {};

        template<typename S, typename T>
        struct is_to_stream_writable<S, T,
            std::void_t<  decltype(std::declval<S&>() << std::declval<T>())  > >
            : std::true_type {};

    } // namespace detail

    struct csv_line_proc
    {
        explicit csv_line_proc(std::ostream& s, std::optional<std::string_view> title_prefix = std::nullopt)
            : s_(s)
            , title_prefix_(title_prefix)
        {
        }

        template<typename T>
        void operator()(std::string_view name, T const &entry)
        {
            if (!first_)
                s_ << ",";

            first_ = false;

            if constexpr (detail::is_to_stream_writable<std::ostream, T>::value)
            {
                if (title_prefix_)
                    s_ << "\"" << *title_prefix_ << name << "\"";
                else
                {
                    s_ << entry;
                }
            }
            else
            {
                std::optional<std::string> new_title_prefix;
                if (title_prefix_)
                {
                    std::stringstream ss;
                    ss << *title_prefix_ << name << "_";
                    new_title_prefix = ss.str();
                }

                csv_line_proc inner(s_, new_title_prefix);
                reflect(inner, entry);
            }
        }

    private:
        std::ostream &s_;
        bool first_ = true;
        std::optional<std::string> title_prefix_;
    };

    template<typename T>
    void write_csv_title(std::ostream &s, T const &data)
    {
        csv_line_proc proc(s, "");
        reflect(proc, data);
        s << std::endl;
        s.flush();
    }

    template<typename T>
    void write_csv_line(std::ostream &s, T const &data)
    {
        csv_line_proc proc(s);
        reflect(proc, data);
        s << std::endl;
    }

    template<typename Container>
    void write_csv_file(std::ostream &s, Container const &data)
    {
        using value_type = Container::value_type;

        value_type dummy;
        write_csv_title(s, dummy);

        for (auto const &e : data)
            write_csv_line(s, e);  
    }

} // namespace csv_io
} // namespace cora
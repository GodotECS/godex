#pragma once

#include <type_traits>
#include <utility>

/// The TA_STRING_LITERAL macro allow to pass a string literal as template argument.
/// Taken from: https://stackoverflow.com/a/36273815/2653173
/// You can use it as follows:
/// ```
/// template <const char *>
/// struct foo {};
///
/// int main() {
///     foo<TA_STRING_LITERAL("abcdefghij")> f;
///     static_cast<void>(f);
/// }
/// ```
/// In godex you can find an usage example in `events.h`

#define MAX_TA_STRING_LITERAL_LENGTH 20
#define TA_STRING_LITERAL(str) string_literal<char_pack<TA_STRING_LITERAL_20(str)>>::s

#define TA_STRING_LITERAL_20(str) TA_STRING_LITERAL_19(str), ((TERMINATED_19(str)) ? (str[19]) : ('\0'))
#define TA_STRING_LITERAL_19(str) TA_STRING_LITERAL_18(str), ((TERMINATED_18(str)) ? (str[18]) : ('\0'))
#define TA_STRING_LITERAL_18(str) TA_STRING_LITERAL_17(str), ((TERMINATED_17(str)) ? (str[17]) : ('\0'))
#define TA_STRING_LITERAL_17(str) TA_STRING_LITERAL_16(str), ((TERMINATED_16(str)) ? (str[16]) : ('\0'))
#define TA_STRING_LITERAL_16(str) TA_STRING_LITERAL_15(str), ((TERMINATED_15(str)) ? (str[15]) : ('\0'))
#define TA_STRING_LITERAL_15(str) TA_STRING_LITERAL_14(str), ((TERMINATED_14(str)) ? (str[14]) : ('\0'))
#define TA_STRING_LITERAL_14(str) TA_STRING_LITERAL_13(str), ((TERMINATED_13(str)) ? (str[13]) : ('\0'))
#define TA_STRING_LITERAL_13(str) TA_STRING_LITERAL_12(str), ((TERMINATED_12(str)) ? (str[12]) : ('\0'))
#define TA_STRING_LITERAL_12(str) TA_STRING_LITERAL_11(str), ((TERMINATED_11(str)) ? (str[11]) : ('\0'))
#define TA_STRING_LITERAL_11(str) TA_STRING_LITERAL_10(str), ((TERMINATED_10(str)) ? (str[10]) : ('\0'))
#define TA_STRING_LITERAL_10(str) TA_STRING_LITERAL_9(str), ((TERMINATED_9(str)) ? (str[9]) : ('\0'))
#define TA_STRING_LITERAL_9(str) TA_STRING_LITERAL_8(str), ((TERMINATED_8(str)) ? (str[8]) : ('\0'))
#define TA_STRING_LITERAL_8(str) TA_STRING_LITERAL_7(str), ((TERMINATED_7(str)) ? (str[7]) : ('\0'))
#define TA_STRING_LITERAL_7(str) TA_STRING_LITERAL_6(str), ((TERMINATED_6(str)) ? (str[6]) : ('\0'))
#define TA_STRING_LITERAL_6(str) TA_STRING_LITERAL_5(str), ((TERMINATED_5(str)) ? (str[5]) : ('\0'))
#define TA_STRING_LITERAL_5(str) TA_STRING_LITERAL_4(str), ((TERMINATED_4(str)) ? (str[4]) : ('\0'))
#define TA_STRING_LITERAL_4(str) TA_STRING_LITERAL_3(str), ((TERMINATED_3(str)) ? (str[3]) : ('\0'))
#define TA_STRING_LITERAL_3(str) TA_STRING_LITERAL_2(str), ((TERMINATED_2(str)) ? (str[2]) : ('\0'))
#define TA_STRING_LITERAL_2(str) TA_STRING_LITERAL_1(str), ((TERMINATED_1(str)) ? (str[1]) : ('\0'))
#define TA_STRING_LITERAL_1(str) str[0]

#define TERMINATED_20(str) TERMINATED_19(str) && str[19]
#define TERMINATED_19(str) TERMINATED_18(str) && str[18]
#define TERMINATED_18(str) TERMINATED_17(str) && str[17]
#define TERMINATED_17(str) TERMINATED_16(str) && str[16]
#define TERMINATED_16(str) TERMINATED_15(str) && str[15]
#define TERMINATED_15(str) TERMINATED_14(str) && str[14]
#define TERMINATED_14(str) TERMINATED_13(str) && str[13]
#define TERMINATED_13(str) TERMINATED_12(str) && str[12]
#define TERMINATED_12(str) TERMINATED_11(str) && str[11]
#define TERMINATED_11(str) TERMINATED_10(str) && str[10]
#define TERMINATED_10(str) TERMINATED_9(str) && str[9]
#define TERMINATED_9(str) TERMINATED_8(str) && str[8]
#define TERMINATED_8(str) TERMINATED_7(str) && str[7]
#define TERMINATED_7(str) TERMINATED_6(str) && str[6]
#define TERMINATED_6(str) TERMINATED_5(str) && str[5]
#define TERMINATED_5(str) TERMINATED_4(str) && str[4]
#define TERMINATED_4(str) TERMINATED_3(str) && str[3]
#define TERMINATED_3(str) TERMINATED_2(str) && str[2]
#define TERMINATED_2(str) TERMINATED_1(str) && str[1]
#define TERMINATED_1(str) str[0]

template <char... Cs>
struct char_pack {
	static constexpr char const arr[sizeof...(Cs) + 1] = { Cs..., 0 };
	static constexpr std::size_t non_zero_count = (((Cs != 0) ? 1 : 0) + ...);
	static_assert(non_zero_count < MAX_TA_STRING_LITERAL_LENGTH, "You need to create more macros.");
};

template <char... Cs>
constexpr char const char_pack<Cs...>::arr[sizeof...(Cs) + 1];

template <char... Cs>
constexpr std::size_t char_pack<Cs...>::non_zero_count;

template <class CP, class = void, class = std::make_index_sequence<CP::non_zero_count>>
struct string_literal;

template <char... Cs, std::size_t... Is>
struct string_literal<char_pack<Cs...>, std::enable_if_t<(Cs && ...)>, std::index_sequence<Is...>> {
	static constexpr char const s[sizeof...(Cs) + 1] = { Cs..., '\0' };
};

template <char... Cs, std::size_t... Is>
constexpr char const string_literal<char_pack<Cs...>, std::enable_if_t<(Cs && ...)>, std::index_sequence<Is...>>::s[sizeof...(Cs) + 1];

template <char... Cs, std::size_t... Is>
struct string_literal<char_pack<Cs...>, std::enable_if_t<!(Cs && ...)>, std::index_sequence<Is...>> : string_literal<char_pack<char_pack<Cs...>::arr[Is]...>> {};

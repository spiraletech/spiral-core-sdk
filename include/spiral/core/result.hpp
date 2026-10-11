#pragma once
#include <string>
#include <utility>
#include <variant>
namespace spiral::core {
struct Error { int code{}; std::string message; };
template<class T> class Result {
 std::variant<T, Error> data_;
 public:
 Result(T value) : data_(std::move(value)) {}
 Result(Error error) : data_(std::move(error)) {}
 [[nodiscard]] bool ok() const noexcept { return std::holds_alternative<T>(data_); }
 [[nodiscard]] explicit operator bool() const noexcept { return ok(); }
 [[nodiscard]] const T& value() const { return std::get<T>(data_); }
 [[nodiscard]] const Error& error() const { return std::get<Error>(data_); }
};
}

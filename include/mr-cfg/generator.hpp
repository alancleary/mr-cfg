// https://blog.feabhas.com/2021/09/c20-coroutine-iterators/
// https://github.com/feabhas/coroutines-blog

#ifndef INCLUDED_MR_CFG_GENERATOR
#define INCLUDED_MR_CFG_GENERATOR

#include <coroutine>
#include <cstddef>
#include <iterator>
#include <optional>


namespace mr_cfg {


template<typename T>
class Generator
{

  class Promise
  {
  public:
    using value_type = std::optional<T>;

    Promise() = default;
    std::suspend_always initial_suspend() { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void unhandled_exception() {
      std::rethrow_exception(std::move(std::current_exception()));
    }

    std::suspend_always yield_value(T value) {
      this->value = std::move(value);
      return {};
    }

    //void return_value(T value) {
    //  this->value = std::move(value);
    //}

    void return_void() {
      this->value = std::nullopt;
    }

    inline Generator get_return_object();

    value_type get_value() {
      return std::move(value);
    }

    bool finished() {
      return !value.has_value();
    }

  private:
    value_type value{};
  };

public:
  using value_type = T;
  using promise_type = Promise;

  explicit Generator(std::coroutine_handle<Promise> handle): handle(handle) { }

  ~Generator() {
    if (handle) { handle.destroy(); }
  }

  Promise::value_type next() {
    if (handle) {
      handle.resume();
      return handle.promise().get_value();
    } else {
      return {};
    }
  }

  struct end_iterator {};

  class iterator
  {
  public:
    using value_type = Promise::value_type;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::input_iterator_tag;

    iterator() = default;
    iterator(Generator& generator): generator{&generator} { }

    value_type operator*() const {
      if (generator) {
        return generator->handle.promise().get_value();
      }
      return {};
    }

    value_type operator->() const {
      if (generator) {
        return generator->handle.promise().get_value();
      }
      return {};
    }

    iterator& operator++() {
      if (generator && generator->handle) {
        generator->handle.resume();
      }
      return *this;
    }

    iterator& operator++(int) {
      if (generator && generator->handle) {
        generator->handle.resume();
      }
      return *this;
    }

    bool operator== (const end_iterator&) const {
      return generator ? generator->handle.promise().finished() : true;
    }

  private:
    Generator* generator{};
  };

  iterator begin() {
    iterator it{*this};
    return ++it;
  }

  end_iterator end() {
    return end_sentinel;
  }

private:
  end_iterator end_sentinel{};
  std::coroutine_handle<Promise> handle;

};


template <typename T>
inline Generator<T> Generator<T>::Promise::get_return_object()
{
  return Generator{ std::coroutine_handle<Promise>::from_promise(*this) };
}


}

#endif

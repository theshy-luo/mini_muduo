#pragma once

// 继承此类即不能被拷贝
// 使用 = delete禁止掉拷贝与构造
namespace mini_muduo {
class Noncopyable {
public:
  Noncopyable(const Noncopyable &) = delete;
  Noncopyable &operator=(const Noncopyable &) = delete;

protected:
  Noncopyable() = default;
  ~Noncopyable() = default;
};
} // namespace mini_muduo
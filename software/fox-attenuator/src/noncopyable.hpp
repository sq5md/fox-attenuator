#pragma once

class NonCopyable
{
  public:
    NonCopyable()                              = default;
    ~NonCopyable()                             = default;
    NonCopyable(const NonCopyable&)            = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
};

class NonMoveable
{
  public:
    NonMoveable()                         = default;
    ~NonMoveable()                        = default;
    NonMoveable(NonMoveable&&)            = delete;
    NonMoveable& operator=(NonMoveable&&) = delete;
};

class NonCopyableNonMoveable : public NonCopyable, public NonMoveable
{};
#pragma once

#include <exception>
#include <string>

namespace mini_muduo 
{

    class Exception : public std::exception
    {
        public:
            Exception(std::string what);
            ~Exception() noexcept override = default;

            const char* what() const noexcept override 
            { 
                return message_.c_str(); 
            }
            const char* stack() const noexcept 
            { 
                return stack_.c_str(); 
            }

        private:
            std::string message_;
            std::string stack_;
    };
    
} // namespace mini_muduo
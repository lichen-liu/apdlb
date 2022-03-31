#pragma once

#include <string>
#include <sys/time.h>
#include "message.hpp"

namespace TP
{
    /// Time stamp function in milliseconds
    double get_time_stamp();

    class TIMER
    {
    public:
        explicit TIMER(std::string profile_name);
        ~TIMER();

        /// Use this to measure time elapsed from preivous call to "elapsed_previous()"
        double elapsed_previous(const std::string &subprofile_name);

        /// Use this to measure the time elapsed from the creation of this TIMER instance
        double elapsed_start() const;

    private:
        double start_time_;
        double previous_elapsing_time_;
        std::string profile_name_;
    };
}

// Implementation
namespace TP
{
    inline double get_time_stamp()
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return (double)tv.tv_usec / 1000000 + tv.tv_sec;
    }

    inline TIMER::TIMER(std::string profile_name) : start_time_(get_time_stamp()), previous_elapsing_time_(start_time_), profile_name_(std::move(profile_name))
    {
        warn("TIMER: Starting profile (%s)\n", profile_name_.c_str());
    }

    inline TIMER::~TIMER()
    {
        elapsed_start();
    }

    inline double TIMER::elapsed_previous(const std::string &subprofile_name)
    {

        double current_time = get_time_stamp();
        double elapsed = current_time - previous_elapsing_time_;

        std::string elapsed_str = std::to_string(elapsed);
        warn("TIMER: Subprofile [%s/%s]: %s seconds\n", profile_name_.c_str(), subprofile_name.c_str(), elapsed_str.c_str());
        previous_elapsing_time_ = current_time;

        return elapsed;
    }

    inline double TIMER::elapsed_start() const
    {
        double current_time = get_time_stamp();
        double elapsed = current_time - start_time_;

        std::string elapsed_str = std::to_string(elapsed);
        warn("TIMER: Profile [%s]: %s seconds\n", profile_name_.c_str(), elapsed_str.c_str());

        return elapsed;
    }
}
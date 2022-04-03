#pragma once

#include "core/engine.h"
#include <optional>

namespace CPUSIM
{
    class TP_ENGINE : public CORE::ENGINE
    {
    public:
        virtual ~TP_ENGINE() = default;

        TP_ENGINE(CORE::SYSTEM_STATE system_state_ic,
                  CORE::DT dt,
                  size_t n_thread,
                  std::optional<std::string> system_state_log_dir_opt = {});

        virtual std::string name() override { return "TP_ENGINE"; }
        virtual CORE::SYSTEM_STATE execute(int n_iter, CORE::TIMER &timer) override;

    protected:
        size_t n_thread() const { return n_thread_; }

    private:
        size_t n_thread_;
    };
}
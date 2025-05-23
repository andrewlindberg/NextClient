#pragma once
#include <concurrencpp/concurrencpp.h>

#include "Result.h"

class TaskCoroImpl
{
    std::unique_ptr<concurrencpp::runtime> ccruntime_;
    std::shared_ptr<concurrencpp::manual_executor> update_executor_;


public:
    explicit TaskCoroImpl();

    template<class callable_type, class... argument_types>
    auto RunInMainThreadAndWait(callable_type&& callable, argument_types&&... arguments)
    {
        using return_type = std::invoke_result_t<callable_type, argument_types...>;

        if (update_executor_ == nullptr)
            return ResultT<return_type>(ResultError("Not initialized"));

        if (update_executor_->shutdown_requested())
            return ResultT<return_type>(ResultError("Shutdown requested"));

        try
        {
            return_type result = update_executor_->submit(std::forward<callable_type>(callable), std::forward<argument_types>(arguments)...).get();
            return ResultT<return_type>(std::forward<return_type>(result));
        }
        catch (concurrencpp::errors::interrupted_task& e)
        {
            return ResultT<return_type>(ResultError(e.what()));
        }
    }

    template<class callable_type, class... argument_types>
    Result RunInMainThreadAndForget(callable_type&& callable, argument_types&&... arguments)
    {
        if (update_executor_ == nullptr)
            return Result(ResultError("Not initialized"));

        if (update_executor_->shutdown_requested())
            return Result(ResultError("Shutdown requested"));

        try
        {
            update_executor_->post(std::forward<callable_type>(callable), std::forward<argument_types>(arguments)...);
            return Result();
        }
        catch (concurrencpp::errors::interrupted_task& e)
        {
            return ResultError(e.what());
        }
    }

    void OnUpdate();
    void OnShutdown();
};

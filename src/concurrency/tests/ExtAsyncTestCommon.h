#ifndef EXTASYNCTESTCOMMON_H
#define EXTASYNCTESTCOMMON_H

// Qt5
#include <QTest>

// Google Test
#include <gtest/gtest.h>
//#include <gmock/gmock-matchers.h>

// Ours
#include <tests/TestHelpers.h>
#include "../ExtFuture.h"
#include "../ExtAsync.h"

class ExtAsyncTestCommon
{
public:
    ExtAsyncTestCommon();
};

class trackable_generator_base
{
    // Just so we can keep a container of generators in the test fixture.
};

template <class FutureTypeT>
class async_test_generator : public trackable_generator_base
{
public:
    explicit async_test_generator(ExtAsyncTestsSuiteFixtureBase* fixture) : m_fixture(fixture) {}
    virtual ~async_test_generator() {}

    virtual void register_gen() { m_fixture->register_generator(this); }
    virtual void unregister_gen() { m_fixture->unregister_generator(this); }

protected:
    ExtAsyncTestsSuiteFixtureBase* m_fixture;
};

/**
 * From a lambda passed to ExtAsync::run(), iterates @a num_iteration times,
 * sleep()ing for 1 sec, then returns the the next value in the sequence to the returned ExtFuture<>.
 *
 * @todo Doesn't handle cancellation or progress reporting.
 */
static ExtFuture<int> async_int_generator(int start_val, int num_iterations, ExtAsyncTestsSuiteFixtureBase* fixture)//std::atomic_bool& generator_run_completed)
{
    SCOPED_TRACE("In async_int_generator");

    auto tgb = new trackable_generator_base();
    fixture->register_generator(tgb);

    ExtFuture<int> retval = ExtAsync::run_efarg([=](ExtFuture<int>& future) {
        int current_val = start_val;
        SCOPED_TRACE("In async_int_generator callback");
        for(int i=0; i<num_iterations; i++)
        {
            // Sleep for a second.
            qWr() << "SLEEPING FOR 1 SEC";

            QTest::qSleep(1000);
            qWr() << "SLEEP COMPLETE, sending value to future:" << current_val;

            future.reportResult(current_val);
            current_val++;
        }
        // We're done.
        qWr() << "REPORTING FINISHED";
        fixture->unregister_generator(tgb);
//        generator_run_completed = true;
        future.reportFinished();
    });

    static_assert(std::is_same_v<decltype(retval), ExtFuture<int>>, "");

    qWr() << "RETURNING:" << retval;

    return retval;
}


#endif // EXTASYNCTESTCOMMON_H

// Copyright Verizon Media. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <vespa/storage/distributor/idealstatemetricsset.h>
#include <vespa/storageapi/message/persistence.h>
#include <vespa/storageapi/message/bucketsplitting.h>
#include <vespa/storageapi/message/visitor.h>
#include <vespa/storageapi/message/removelocation.h>
#include <vespa/storageframework/defaultimplementation/thread/threadpoolimpl.h>
#include <tests/distributor/top_level_distributor_test_util.h>
#include <vespa/document/bucket/fixed_bucket_spaces.h>
#include <vespa/document/test/make_document_bucket.h>
#include <vespa/document/test/make_bucket_space.h>
#include <vespa/storage/config/config-stor-distributormanager.h>
#include <vespa/storage/distributor/distributor.h>
#include <vespa/storage/distributor/distributor_stripe.h>
#include <vespa/storage/distributor/distributor_status.h>
#include <vespa/storage/distributor/distributor_bucket_space.h>
#include <vespa/storage/distributor/distributormetricsset.h>
#include <vespa/storage/distributor/distributor_stripe_pool.h>
#include <vespa/storage/distributor/distributor_stripe_thread.h>
#include <vespa/vespalib/stllike/asciistream.h>
#include <vespa/metrics/updatehook.h>
#include <thread>
#include <vespa/vespalib/gtest/gtest.h>
#include <gmock/gmock.h>

using document::test::makeDocumentBucket;
using document::test::makeBucketSpace;
using document::FixedBucketSpaces;
using document::BucketSpace;
using document::Bucket;
using document::BucketId;
using namespace ::testing;

namespace storage::distributor {

struct TopLevelDistributorTest : Test, TopLevelDistributorTestUtil {
    TopLevelDistributorTest();
    ~TopLevelDistributorTest() override;

    void SetUp() override {
        create_links();
    };

    void TearDown() override {
        close();
    }

    void reply_to_1_node_bucket_info_fetch_with_n_buckets(size_t n);

    // Simple type aliases to make interfacing with certain utility functions
    // easier. Note that this is only for readability and does not provide any
    // added type safety.
    using NodeCount = int;
    using Redundancy = int;
    using ConfigBuilder = vespa::config::content::core::StorDistributormanagerConfigBuilder;

    std::string resolve_stripe_operation_routing(std::shared_ptr<api::StorageMessage> msg) {
        handle_top_level_message(msg);

        vespalib::asciistream posted_msgs;
        auto stripes = distributor_stripes();
        for (size_t i = 0; i < stripes.size(); ++i) {
            // TODO less intrusive, this is brittle.
            for (auto& qmsg : stripes[i]->_messageQueue) {
                posted_msgs << "Stripe " << i << ": " << MessageSenderStub::dumpMessage(*qmsg, false, false);
            }
            stripes[i]->_messageQueue.clear();
        }
        return posted_msgs.str();
    }

    void tick_distributor_and_stripes_n_times(uint32_t n) {
        for (uint32_t i = 0; i < n; ++i) {
            tick(false);
        }
    }

    void tick_top_level_distributor_n_times(uint32_t n) {
        for (uint32_t i = 0; i < n; ++i) {
            tick(true);
        }
    }

    StatusReporterDelegate& distributor_status_delegate() {
        return _distributor->_distributorStatusDelegate;
    }

    framework::TickingThreadPool& distributor_thread_pool() {
        return _distributor->_threadPool;
    }

    const std::vector<std::shared_ptr<DistributorStatus>>& distributor_status_todos() {
        return _distributor->_status_to_do;
    }

    Distributor::MetricUpdateHook distributor_metric_update_hook() {
        return _distributor->_metricUpdateHook;
    }

    uint64_t db_sample_interval_sec() const noexcept {
        // Sampling interval is equal across stripes, so just grab the first one and go with it.
        return std::chrono::duration_cast<std::chrono::seconds>(
                distributor_stripes().front()->db_memory_sample_interval()).count();
    }

    static std::vector<document::BucketSpace> bucket_spaces() {
        return {document::FixedBucketSpaces::default_space(), document::FixedBucketSpaces::global_space()};
    }

    size_t explicit_node_state_reply_send_invocations() const noexcept {
        return _node->getNodeStateUpdater().explicit_node_state_reply_send_invocations();
    }

};

TopLevelDistributorTest::TopLevelDistributorTest()
    : Test(),
      TopLevelDistributorTestUtil()
{
}

TopLevelDistributorTest::~TopLevelDistributorTest() = default;

TEST_F(TopLevelDistributorTest, external_operation_is_routed_to_expected_stripe) {
    setup_distributor(Redundancy(1), NodeCount(1), "storage:1 distributor:1");

    auto op = std::make_shared<api::RemoveCommand>(
            makeDocumentBucket(document::BucketId()),
            document::DocumentId("id:m:test:n=1:foo"),
            api::Timestamp(1234));

    // We expect stripe mapping to be deterministic.
    EXPECT_EQ("Stripe 2: Remove", resolve_stripe_operation_routing(op));

    auto cmd = std::make_shared<api::CreateVisitorCommand>(makeBucketSpace(), "foo", "bar", "");
    cmd->addBucketToBeVisited(document::BucketId(16, 1234));
    cmd->addBucketToBeVisited(document::BucketId());

    EXPECT_EQ("Stripe 1: Visitor Create", resolve_stripe_operation_routing(cmd));
}

TEST_F(TopLevelDistributorTest, recovery_mode_on_cluster_state_change_is_triggered_across_all_stripes) {
    setup_distributor(Redundancy(1), NodeCount(2),
                      "storage:1 .0.s:d distributor:1");
    enable_distributor_cluster_state("storage:1 distributor:1");

    EXPECT_TRUE(all_distributor_stripes_are_in_recovery_mode());
    tick();
    EXPECT_FALSE(all_distributor_stripes_are_in_recovery_mode());

    enable_distributor_cluster_state("storage:2 distributor:1");
    EXPECT_TRUE(all_distributor_stripes_are_in_recovery_mode());
}

// TODO STRIPE consider moving to generic test, not specific to top-level distributor or stripe
TEST_F(TopLevelDistributorTest, contains_time_statement) {
    setup_distributor(Redundancy(1), NodeCount(1), "storage:1 distributor:1");

    auto cfg = _component->total_distributor_config_sp();
    EXPECT_FALSE(cfg->containsTimeStatement(""));
    EXPECT_FALSE(cfg->containsTimeStatement("testdoctype1"));
    EXPECT_FALSE(cfg->containsTimeStatement("testdoctype1.headerfield > 42"));
    EXPECT_TRUE(cfg->containsTimeStatement("testdoctype1.headerfield > now()"));
    EXPECT_TRUE(cfg->containsTimeStatement("testdoctype1.headerfield > now() - 3600"));
    EXPECT_TRUE(cfg->containsTimeStatement("testdoctype1.headerfield == now() - 3600"));
}

TEST_F(TopLevelDistributorTest, config_changes_are_propagated_to_all_stripes) {
    setup_distributor(Redundancy(1), NodeCount(1), "storage:1 distributor:1");

    for (auto* s : distributor_stripes()) {
        ASSERT_NE(s->getConfig().getSplitCount(), 1234);
        ASSERT_NE(s->getConfig().getJoinCount(), 123);
    }

    auto cfg = current_distributor_config();
    cfg.splitcount = 1234;
    cfg.joincount = 123;
    reconfigure(cfg);

    for (auto* s : distributor_stripes()) {
        ASSERT_EQ(s->getConfig().getSplitCount(), 1234);
        ASSERT_EQ(s->getConfig().getJoinCount(), 123);
    }
}

namespace {

using namespace framework::defaultimplementation;

class StatusRequestThread : public framework::Runnable {
    StatusReporterDelegate& _reporter;
    std::string _result;
public:
    explicit StatusRequestThread(StatusReporterDelegate& reporter)
        : _reporter(reporter)
    {}
    void run(framework::ThreadHandle&) override {
        framework::HttpUrlPath path("/distributor?page=buckets");
        std::ostringstream stream;
        _reporter.reportStatus(stream, path);
        _result = stream.str();
    }

    std::string getResult() const {
        return _result;
    }
};

}

TEST_F(TopLevelDistributorTest, tick_aggregates_status_requests_from_all_stripes) {
    setup_distributor(Redundancy(1), NodeCount(1), "storage:1 distributor:1");

    ASSERT_NE(stripe_of_bucket(document::BucketId(16, 1)),
              stripe_of_bucket(document::BucketId(16, 2)));

    add_nodes_to_stripe_bucket_db(document::BucketId(16, 1), "0=1/1/1/t");
    add_nodes_to_stripe_bucket_db(document::BucketId(16, 2), "0=2/2/2/t");

    // Must go via delegate since reportStatus is now just a rendering
    // function and not a request enqueuer (see Distributor::handleStatusRequest).
    StatusRequestThread thread(distributor_status_delegate());
    FakeClock clock;
    ThreadPoolImpl pool(clock);
    int ticksBeforeWait = 1;
    framework::Thread::UP tp(pool.startThread(thread, "statustest", 5ms, 5s, ticksBeforeWait));

    while (true) {
        std::this_thread::sleep_for(1ms);
        framework::TickingLockGuard guard(distributor_thread_pool().freezeCriticalTicks());
        if (!distributor_status_todos().empty()) {
            break;
        }
        
    }
    ASSERT_TRUE(tick());

    tp->interruptAndJoin();

    // Result contains buckets from DBs of multiple stripes.
    EXPECT_THAT(thread.getResult(), HasSubstr("BucketId(0x4000000000000001)"));
    EXPECT_THAT(thread.getResult(), HasSubstr("BucketId(0x4000000000000002)"));
}

TEST_F(TopLevelDistributorTest, metric_update_hook_updates_pending_maintenance_metrics) {
    setup_distributor(Redundancy(2), NodeCount(2), "storage:2 distributor:1");
    // To ensure we count all operations, not just those fitting within the pending window.
    auto cfg = current_distributor_config();
    cfg.maxpendingidealstateoperations = 1; // FIXME STRIPE this does not actually seem to be used...!
    reconfigure(cfg);

    // 1 bucket must be merged, 1 must be split, 1 should be activated.
    add_nodes_to_stripe_bucket_db(document::BucketId(16, 1), "0=2/2/2/t/a,1=1/1/1");
    add_nodes_to_stripe_bucket_db(document::BucketId(16, 2), "0=100/10000000/200000/t/a,1=100/10000000/200000/t");
    add_nodes_to_stripe_bucket_db(document::BucketId(16, 3), "0=200/300/400/t,1=200/300/400/t");

    // Go many full scanner rounds to check that metrics are set, not added to existing.
    tick_distributor_and_stripes_n_times(50);

    // By this point, no hook has been called so the metrics have not been set.
    using MO = MaintenanceOperation;
    {
        const IdealStateMetricSet& metrics = total_ideal_state_metrics();
        EXPECT_EQ(0, metrics.operations[MO::MERGE_BUCKET]->pending.getLast());
        EXPECT_EQ(0, metrics.operations[MO::SPLIT_BUCKET]->pending.getLast());
        EXPECT_EQ(0, metrics.operations[MO::SET_BUCKET_STATE]->pending.getLast());
        EXPECT_EQ(0, metrics.operations[MO::DELETE_BUCKET]->pending.getLast());
        EXPECT_EQ(0, metrics.operations[MO::JOIN_BUCKET]->pending.getLast());
        EXPECT_EQ(0, metrics.operations[MO::GARBAGE_COLLECTION]->pending.getLast());
    }

    // Force trigger update hook
    std::mutex l;
    distributor_metric_update_hook().updateMetrics(metrics::MetricLockGuard(l));
    // Metrics should now be updated to the last complete working state
    {
        const IdealStateMetricSet& metrics = total_ideal_state_metrics();
        EXPECT_EQ(1, metrics.operations[MO::MERGE_BUCKET]->pending.getLast());
        EXPECT_EQ(1, metrics.operations[MO::SPLIT_BUCKET]->pending.getLast());
        EXPECT_EQ(1, metrics.operations[MO::SET_BUCKET_STATE]->pending.getLast());
        EXPECT_EQ(0, metrics.operations[MO::DELETE_BUCKET]->pending.getLast());
        EXPECT_EQ(0, metrics.operations[MO::JOIN_BUCKET]->pending.getLast());
        EXPECT_EQ(0, metrics.operations[MO::GARBAGE_COLLECTION]->pending.getLast());
    }
}

TEST_F(TopLevelDistributorTest, bucket_db_memory_usage_metrics_only_updated_at_fixed_time_intervals) {
    fake_clock().setAbsoluteTimeInSeconds(1000);

    setup_distributor(Redundancy(2), NodeCount(2), "storage:2 distributor:1");
    add_nodes_to_stripe_bucket_db(document::BucketId(16, 1), "0=1/1/1/t/a,1=2/2/2");
    tick_distributor_and_stripes_n_times(10);

    std::mutex l;
    distributor_metric_update_hook().updateMetrics(metrics::MetricLockGuard(l));
    auto* m = total_distributor_metrics().mutable_dbs.memory_usage.getMetric("used_bytes");
    ASSERT_TRUE(m != nullptr);
    auto last_used = m->getLongValue("last");
    EXPECT_GT(last_used, 0);

    // Add another bucket to the DB. This should increase the underlying used number of
    // bytes, but this should not be aggregated into the metrics until the sampling time
    // interval has passed. Instead, old metric gauge values should be preserved.
    add_nodes_to_stripe_bucket_db(document::BucketId(16, 2), "0=1/1/1/t/a,1=2/2/2");

    const auto sample_interval_sec = db_sample_interval_sec();
    fake_clock().setAbsoluteTimeInSeconds(1000 + sample_interval_sec - 1); // Not there yet.
    tick_distributor_and_stripes_n_times(50);
    distributor_metric_update_hook().updateMetrics(metrics::MetricLockGuard(l));

    m = total_distributor_metrics().mutable_dbs.memory_usage.getMetric("used_bytes");
    auto now_used = m->getLongValue("last");
    EXPECT_EQ(now_used, last_used);

    fake_clock().setAbsoluteTimeInSeconds(1000 + sample_interval_sec + 1);
    tick_distributor_and_stripes_n_times(10);
    distributor_metric_update_hook().updateMetrics(metrics::MetricLockGuard(l));

    m = total_distributor_metrics().mutable_dbs.memory_usage.getMetric("used_bytes");
    now_used = m->getLongValue("last");
    EXPECT_GT(now_used, last_used);
}

void TopLevelDistributorTest::reply_to_1_node_bucket_info_fetch_with_n_buckets(size_t n) {
    ASSERT_EQ(bucket_spaces().size(), _sender.commands().size());
    for (uint32_t i = 0; i < _sender.commands().size(); ++i) {
        ASSERT_EQ(api::MessageType::REQUESTBUCKETINFO, _sender.command(i)->getType());
        auto& bucket_req = dynamic_cast<api::RequestBucketInfoCommand&>(*_sender.command(i));
        auto reply = bucket_req.makeReply();
        if (bucket_req.getBucketSpace() == FixedBucketSpaces::default_space()) {
            auto& bucket_reply = dynamic_cast<api::RequestBucketInfoReply&>(*reply);
            for (size_t j = 1; j <= n; ++j) {
                bucket_reply.getBucketInfo().push_back(api::RequestBucketInfoReply::Entry(
                        document::BucketId(16, j), api::BucketInfo(20, 10, 12, 50, 60, true, true)));
            }
        }
        handle_top_level_message(std::move(reply));
    }
    _sender.commands().clear();
}

TEST_F(TopLevelDistributorTest, cluster_state_lifecycle_is_propagated_to_stripes) {
    setup_distributor(Redundancy(2), NodeCount(2), "storage:2 .0.s:d distributor:1");
    // Node 0 goes from Down -> Up, should get 1 RequestBucketInfo per bucket space.
    receive_set_system_state_command("storage:2 distributor:1");
    tick_top_level_distributor_n_times(1); // Process enqueued message
    // All stripes should now be in pending state
    for (auto* s : distributor_stripes()) {
        for (auto space : bucket_spaces()) {
            EXPECT_TRUE(s->getBucketSpaceRepo().get(space).has_pending_cluster_state());
        }
    }
    // Respond with some buckets that will be evenly distributed across the stripes.
    reply_to_1_node_bucket_info_fetch_with_n_buckets(10);
    tick_top_level_distributor_n_times(1); // Process enqueued replies

    std::vector<document::BucketId> inserted_buckets;
    // Pending state should now be cleared for all stripes
    for (auto* s : distributor_stripes()) {
        for (auto space : bucket_spaces()) {
            EXPECT_FALSE(s->getBucketSpaceRepo().get(space).has_pending_cluster_state());
        }
        auto& def_space = s->getBucketSpaceRepo().get(document::FixedBucketSpaces::default_space());
        def_space.getBucketDatabase().acquire_read_guard()->for_each([&](uint64_t key, [[maybe_unused]] const auto& entry) {
           inserted_buckets.emplace_back(document::BucketId::keyToBucketId(key));
        });
    }
    // All buckets should be present. We track as vectors rather than sets to detect any cross-stripe duplicates.
    std::vector<document::BucketId> expected_buckets;
    for (size_t i = 1; i <= 10; ++i) {
        expected_buckets.emplace_back(16, i);
    }
    std::sort(expected_buckets.begin(), expected_buckets.end());
    std::sort(inserted_buckets.begin(), inserted_buckets.end());
    EXPECT_EQ(inserted_buckets, expected_buckets);
}

TEST_F(TopLevelDistributorTest, host_info_sent_immediately_once_all_stripes_first_reported) {
    setup_distributor(Redundancy(2), NodeCount(2), "storage:2 distributor:1");
    ASSERT_EQ(_num_distributor_stripes, 4);
    fake_clock().setAbsoluteTimeInSeconds(1000);

    tick_top_level_distributor_n_times(1);
    EXPECT_EQ(0, explicit_node_state_reply_send_invocations()); // Nothing yet
    _distributor->notify_stripe_wants_to_send_host_info(1);
    _distributor->notify_stripe_wants_to_send_host_info(2);
    _distributor->notify_stripe_wants_to_send_host_info(3);

    tick_top_level_distributor_n_times(1);
    // Still nothing. Missing initial report from stripe 0
    EXPECT_EQ(0, explicit_node_state_reply_send_invocations());

    _distributor->notify_stripe_wants_to_send_host_info(0);
    tick_top_level_distributor_n_times(1);
    // All stripes have reported in, it's time to party!
    EXPECT_EQ(1, explicit_node_state_reply_send_invocations());

    // No further sends if stripes haven't requested it yet.
    fake_clock().setAbsoluteTimeInSeconds(2000);
    tick_top_level_distributor_n_times(10);
    EXPECT_EQ(1, explicit_node_state_reply_send_invocations());
}

TEST_F(TopLevelDistributorTest, non_bootstrap_host_info_send_request_delays_sending) {
    setup_distributor(Redundancy(2), NodeCount(2), "storage:2 distributor:1");
    ASSERT_EQ(_num_distributor_stripes, 4);
    fake_clock().setAbsoluteTimeInSeconds(1000);

    for (uint16_t i = 0; i < 4; ++i) {
        _distributor->notify_stripe_wants_to_send_host_info(i);
    }
    tick_top_level_distributor_n_times(1);
    // Bootstrap case
    EXPECT_EQ(1, explicit_node_state_reply_send_invocations());

    // Stripe 1 suddenly really wants to tell the cluster controller something again
    _distributor->notify_stripe_wants_to_send_host_info(1);
    tick_top_level_distributor_n_times(1);
    // But its cry for attention is not yet honored since the delay hasn't passed.
    EXPECT_EQ(1, explicit_node_state_reply_send_invocations());

    fake_clock().addMilliSecondsToTime(999);
    tick_top_level_distributor_n_times(1);
    // 1 sec delay has still not passed
    EXPECT_EQ(1, explicit_node_state_reply_send_invocations());

    fake_clock().addMilliSecondsToTime(1);
    tick_top_level_distributor_n_times(1);
    // But now it has
    EXPECT_EQ(2, explicit_node_state_reply_send_invocations());
}

}
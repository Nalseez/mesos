// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef __TESTS_MESOS_HPP__
#define __TESTS_MESOS_HPP__

#include <memory>
#include <string>
#include <vector>

#include <gmock/gmock.h>

#include <mesos/executor.hpp>
#include <mesos/scheduler.hpp>

#include <mesos/v1/executor.hpp>
#include <mesos/v1/resources.hpp>
#include <mesos/v1/resource_provider.hpp>
#include <mesos/v1/scheduler.hpp>

#include <mesos/v1/executor/executor.hpp>

#include <mesos/v1/scheduler/scheduler.hpp>

#include <mesos/authentication/secret_generator.hpp>

#include <mesos/authorizer/authorizer.hpp>

#include <mesos/fetcher/fetcher.hpp>

#include <mesos/master/detector.hpp>

#include <process/future.hpp>
#include <process/gmock.hpp>
#include <process/gtest.hpp>
#include <process/http.hpp>
#include <process/io.hpp>
#include <process/owned.hpp>
#include <process/pid.hpp>
#include <process/process.hpp>
#include <process/queue.hpp>
#include <process/subprocess.hpp>

#include <process/ssl/flags.hpp>
#include <process/ssl/gtest.hpp>

#include <stout/bytes.hpp>
#include <stout/foreach.hpp>
#include <stout/gtest.hpp>
#include <stout/lambda.hpp>
#include <stout/none.hpp>
#include <stout/option.hpp>
#include <stout/stringify.hpp>
#include <stout/try.hpp>
#include <stout/unreachable.hpp>
#include <stout/uuid.hpp>

#include "common/http.hpp"

#include "messages/messages.hpp" // For google::protobuf::Message.

#include "master/master.hpp"

#include "sched/constants.hpp"

#include "resource_provider/detector.hpp"

#include "slave/constants.hpp"
#include "slave/slave.hpp"

#include "slave/containerizer/containerizer.hpp"

#include "slave/containerizer/mesos/containerizer.hpp"

#include "tests/cluster.hpp"
#include "tests/limiter.hpp"
#include "tests/utils.hpp"

#ifdef MESOS_HAS_JAVA
#include "tests/zookeeper.hpp"
#endif // MESOS_HAS_JAVA

using ::testing::_;
using ::testing::An;
using ::testing::DoDefault;
using ::testing::Invoke;
using ::testing::Return;

namespace mesos {
namespace internal {
namespace tests {

constexpr char READONLY_HTTP_AUTHENTICATION_REALM[] = "test-readonly-realm";
constexpr char READWRITE_HTTP_AUTHENTICATION_REALM[] = "test-readwrite-realm";
constexpr char DEFAULT_TEST_ROLE[] = "default-role";
constexpr char DEFAULT_JWT_SECRET_KEY[] =
  "72kUKUFtghAjNbIOvLzfF2RxNBfeM64Bri8g9WhpyaunwqRB/yozHAqSnyHbddAV"
  "PcWRQlrJAt871oWgSH+n52vMZ3aVI+AFMzXSo8+sUfMk83IGp0WJefhzeQsjDlGH"
  "GYQgCAuGim0BE2X5U+lEue8s697uQpAO8L/FFRuDH2s";

constexpr char DOCKER_IPv6_NETWORK[] = "mesos-docker-ip6-test";


// Forward declarations.
class MockExecutor;


// NOTE: `SSLTemporaryDirectoryTest` exists even when SSL is not compiled into
// Mesos.  In this case, the class is an alias of `TemporaryDirectoryTest`.
class MesosTest : public SSLTemporaryDirectoryTest
{
public:
  static void SetUpTestCase();
  static void TearDownTestCase();

protected:
  MesosTest(const Option<zookeeper::URL>& url = None());

  // Returns the flags used to create masters.
  virtual master::Flags CreateMasterFlags();

  // Returns the flags used to create slaves.
  virtual slave::Flags CreateSlaveFlags();

  // Starts a master with the specified flags.
  virtual Try<process::Owned<cluster::Master>> StartMaster(
      const Option<master::Flags>& flags = None());

  // Starts a master with the specified allocator process and flags.
  virtual Try<process::Owned<cluster::Master>> StartMaster(
      mesos::allocator::Allocator* allocator,
      const Option<master::Flags>& flags = None());

  // Starts a master with the specified authorizer and flags.
  virtual Try<process::Owned<cluster::Master>> StartMaster(
      Authorizer* authorizer,
      const Option<master::Flags>& flags = None());

  // Starts a master with a slave removal rate limiter and flags.
  // NOTE: The `slaveRemovalLimiter` is a `shared_ptr` because the
  // underlying `Master` process requires the pointer in this form.
  virtual Try<process::Owned<cluster::Master>> StartMaster(
      const std::shared_ptr<MockRateLimiter>& slaveRemovalLimiter,
      const Option<master::Flags>& flags = None());

  // TODO(bmahler): Consider adding a builder style interface, e.g.
  //
  // Try<PID<Slave>> slave =
  //   Slave().With(flags)
  //          .With(executor)
  //          .With(containerizer)
  //          .With(detector)
  //          .With(gc)
  //          .Start();
  //
  // Or options:
  //
  // Injections injections;
  // injections.executor = executor;
  // injections.containerizer = containerizer;
  // injections.detector = detector;
  // injections.gc = gc;
  // Try<PID<Slave>> slave = StartSlave(injections);

  // Starts a slave with the specified detector and flags.
  virtual Try<process::Owned<cluster::Slave>> StartSlave(
      mesos::master::detector::MasterDetector* detector,
      const Option<slave::Flags>& flags = None(),
      bool mock = false);

  // Starts a slave with the specified detector, containerizer, and flags.
  virtual Try<process::Owned<cluster::Slave>> StartSlave(
      mesos::master::detector::MasterDetector* detector,
      slave::Containerizer* containerizer,
      const Option<slave::Flags>& flags = None(),
      bool mock = false);

  // Starts a slave with the specified detector, id, and flags.
  virtual Try<process::Owned<cluster::Slave>> StartSlave(
      mesos::master::detector::MasterDetector* detector,
      const std::string& id,
      const Option<slave::Flags>& flags = None(),
      bool mock = false);

  // Starts a slave with the specified detector, containerizer, id, and flags.
  virtual Try<process::Owned<cluster::Slave>> StartSlave(
      mesos::master::detector::MasterDetector* detector,
      slave::Containerizer* containerizer,
      const std::string& id,
      const Option<slave::Flags>& flags = None());

  // Starts a slave with the specified detector, GC, and flags.
  virtual Try<process::Owned<cluster::Slave>> StartSlave(
      mesos::master::detector::MasterDetector* detector,
      slave::GarbageCollector* gc,
      const Option<slave::Flags>& flags = None());

  // Starts a slave with the specified detector, resource estimator, and flags.
  virtual Try<process::Owned<cluster::Slave>> StartSlave(
      mesos::master::detector::MasterDetector* detector,
      mesos::slave::ResourceEstimator* resourceEstimator,
      const Option<slave::Flags>& flags = None());

  // Starts a slave with the specified detector, containerizer,
  // resource estimator, and flags.
  virtual Try<process::Owned<cluster::Slave>> StartSlave(
      mesos::master::detector::MasterDetector* detector,
      slave::Containerizer* containerizer,
      mesos::slave::ResourceEstimator* resourceEstimator,
      const Option<slave::Flags>& flags = None());

  // Starts a slave with the specified detector, QoS Controller, and flags.
  virtual Try<process::Owned<cluster::Slave>> StartSlave(
      mesos::master::detector::MasterDetector* detector,
      mesos::slave::QoSController* qosController,
      const Option<slave::Flags>& flags = None());

  // Starts a slave with the specified detector, containerizer,
  // QoS Controller, and flags.
  virtual Try<process::Owned<cluster::Slave>> StartSlave(
      mesos::master::detector::MasterDetector* detector,
      slave::Containerizer* containerizer,
      mesos::slave::QoSController* qosController,
      const Option<slave::Flags>& flags = None(),
      bool mock = false);

  // Starts a slave with the specified detector, authorizer, and flags.
  virtual Try<process::Owned<cluster::Slave>> StartSlave(
      mesos::master::detector::MasterDetector* detector,
      mesos::Authorizer* authorizer,
      const Option<slave::Flags>& flags = None());

  // Starts a slave with the specified detector, containerizer, authorizer,
  // and flags.
  virtual Try<process::Owned<cluster::Slave>> StartSlave(
      mesos::master::detector::MasterDetector* detector,
      slave::Containerizer* containerizer,
      mesos::Authorizer* authorizer,
      const Option<slave::Flags>& flags = None());

  // Starts a slave with the specified detector, containerizer,
  // secretGenerator, authorizer and flags.
  virtual Try<process::Owned<cluster::Slave>> StartSlave(
      mesos::master::detector::MasterDetector* detector,
      slave::Containerizer* containerizer,
      mesos::SecretGenerator* secretGenerator,
      const Option<mesos::Authorizer*>& authorizer = None(),
      const Option<slave::Flags>& flags = None(),
      bool mock = false);

  // Starts a slave with the specified detector, secretGenerator,
  // and flags.
  virtual Try<process::Owned<cluster::Slave>> StartSlave(
      mesos::master::detector::MasterDetector* detector,
      mesos::SecretGenerator* secretGenerator,
      const Option<slave::Flags>& flags = None());

  Option<zookeeper::URL> zookeeperUrl;

  // NOTE: On Windows, most tasks are run under PowerShell, which uses ~150 MB
  // of memory per-instance due to loading .NET. Realistically, PowerShell can
  // be called more than once in a task, so 512 MB is the safe minimum.
  // Furthermore, because the Windows `cpu` isolator is a hard-cap, 0.1 CPUs
  // will cause the task (or even a check command) to timeout, so 1 CPU is the
  // safe minimum.
  //
  // Because multiple tasks can be run, the default agent resources needs to be
  // at least a multiple of the default task resources: four times seems safe.
  //
  // On platforms where the shell is, e.g. Bash, the minimum is much lower.
  const std::string defaultAgentResourcesString{
#ifdef __WINDOWS__
      "cpus:4;gpus:0;mem:2048;disk:1024;ports:[31000-32000]"
#else
      "cpus:2;gpus:0;mem:1024;disk:1024;ports:[31000-32000]"
#endif // __WINDOWS__
      };

  const std::string defaultTaskResourcesString{
#ifdef __WINDOWS__
      "cpus:1;mem:512;disk:32"
#else
      "cpus:0.1;mem:32;disk:32"
#endif // __WINDOWS__
      };
};


template <typename T>
class ContainerizerTest : public MesosTest {};

#ifdef __linux__
// Cgroups hierarchy used by the cgroups related tests.
const static std::string TEST_CGROUPS_HIERARCHY = "/tmp/mesos_test_cgroup";

// Name of the root cgroup used by the cgroups related tests.
const static std::string TEST_CGROUPS_ROOT = "mesos_test";


template <>
class ContainerizerTest<slave::MesosContainerizer> : public MesosTest
{
public:
  static void SetUpTestCase();
  static void TearDownTestCase();

protected:
  virtual slave::Flags CreateSlaveFlags();
  virtual void SetUp();
  virtual void TearDown();

private:
  // Base hierarchy for separately mounted cgroup controllers, e.g., if the
  // base hierarchy is /sys/fs/cgroup then each controller will be mounted to
  // /sys/fs/cgroup/{controller}/.
  std::string baseHierarchy;

  // Set of cgroup subsystems used by the cgroups related tests.
  hashset<std::string> subsystems;
};
#else
template <>
class ContainerizerTest<slave::MesosContainerizer> : public MesosTest
{
protected:
  virtual slave::Flags CreateSlaveFlags();
};
#endif // __linux__


#ifdef MESOS_HAS_JAVA

class MesosZooKeeperTest : public MesosTest
{
public:
  static void SetUpTestCase()
  {
    // Make sure the JVM is created.
    ZooKeeperTest::SetUpTestCase();

    // Launch the ZooKeeper test server.
    server = new ZooKeeperTestServer();
    server->startNetwork();

    Try<zookeeper::URL> parse = zookeeper::URL::parse(
        "zk://" + server->connectString() + "/znode");
    ASSERT_SOME(parse);

    url = parse.get();
  }

  static void TearDownTestCase()
  {
    delete server;
    server = nullptr;
  }

  virtual void SetUp()
  {
    MesosTest::SetUp();
    server->startNetwork();
  }

  virtual void TearDown()
  {
    server->shutdownNetwork();
    MesosTest::TearDown();
  }

protected:
  MesosZooKeeperTest() : MesosTest(url) {}

  virtual master::Flags CreateMasterFlags()
  {
    master::Flags flags = MesosTest::CreateMasterFlags();

    // NOTE: Since we are using the replicated log with ZooKeeper
    // (default storage in MesosTest), we need to specify the quorum.
    flags.quorum = 1;

    return flags;
  }

  static ZooKeeperTestServer* server;
  static Option<zookeeper::URL> url;
};

#endif // MESOS_HAS_JAVA

namespace v1 {

// Alias existing `mesos::v1` namespaces so that we can easily write
// `v1::` in tests.
//
// TODO(jmlvanre): Remove these aliases once we clean up the `tests`
// namespace hierarchy.
namespace agent = mesos::v1::agent;
namespace maintenance = mesos::v1::maintenance;
namespace master = mesos::v1::master;
namespace quota = mesos::v1::quota;

using mesos::v1::TASK_STAGING;
using mesos::v1::TASK_STARTING;
using mesos::v1::TASK_RUNNING;
using mesos::v1::TASK_KILLING;
using mesos::v1::TASK_FINISHED;
using mesos::v1::TASK_FAILED;
using mesos::v1::TASK_KILLED;
using mesos::v1::TASK_ERROR;
using mesos::v1::TASK_LOST;
using mesos::v1::TASK_DROPPED;
using mesos::v1::TASK_UNREACHABLE;
using mesos::v1::TASK_GONE;
using mesos::v1::TASK_GONE_BY_OPERATOR;
using mesos::v1::TASK_UNKNOWN;

using mesos::v1::AgentID;
using mesos::v1::CheckInfo;
using mesos::v1::CommandInfo;
using mesos::v1::ContainerID;
using mesos::v1::ContainerStatus;
using mesos::v1::Environment;
using mesos::v1::ExecutorID;
using mesos::v1::ExecutorInfo;
using mesos::v1::Filters;
using mesos::v1::FrameworkID;
using mesos::v1::FrameworkInfo;
using mesos::v1::HealthCheck;
using mesos::v1::InverseOffer;
using mesos::v1::MachineID;
using mesos::v1::Metric;
using mesos::v1::Offer;
using mesos::v1::Resource;
using mesos::v1::ResourceProviderInfo;
using mesos::v1::Resources;
using mesos::v1::TaskID;
using mesos::v1::TaskInfo;
using mesos::v1::TaskGroupInfo;
using mesos::v1::TaskState;
using mesos::v1::TaskStatus;
using mesos::v1::WeightInfo;

} // namespace v1 {

namespace common {

template <typename TCredential>
struct DefaultCredential
{
  static TCredential create()
  {
    TCredential credential;
    credential.set_principal("test-principal");
    credential.set_secret("test-secret");
    return credential;
  }
};


// TODO(jmlvanre): consider factoring this out.
template <typename TCredential>
struct DefaultCredential2
{
  static TCredential create()
  {
    TCredential credential;
    credential.set_principal("test-principal-2");
    credential.set_secret("test-secret-2");
    return credential;
  }
};


template <typename TFrameworkInfo, typename TCredential>
struct DefaultFrameworkInfo
{
  static TFrameworkInfo create()
  {
    TFrameworkInfo framework;
    framework.set_name("default");
    framework.set_user(os::user().get());
    framework.set_principal(
        DefaultCredential<TCredential>::create().principal());
    framework.add_roles("*");
    framework.add_capabilities()->set_type(
        TFrameworkInfo::Capability::MULTI_ROLE);
    framework.add_capabilities()->set_type(
        TFrameworkInfo::Capability::RESERVATION_REFINEMENT);

    return framework;
  }
};

} // namespace common {

// TODO(jmlvanre): Remove `inline` once we have adjusted all tests to
// distinguish between `internal` and `v1`.
inline namespace internal {
using DefaultCredential = common::DefaultCredential<Credential>;
using DefaultCredential2 = common::DefaultCredential2<Credential>;
using DefaultFrameworkInfo =
  common::DefaultFrameworkInfo<FrameworkInfo, Credential>;
}  // namespace internal {


namespace v1 {
using DefaultCredential = common::DefaultCredential<mesos::v1::Credential>;
using DefaultCredential2 = common::DefaultCredential2<mesos::v1::Credential>;
using DefaultFrameworkInfo =
  common::DefaultFrameworkInfo<mesos::v1::FrameworkInfo, mesos::v1::Credential>;
}  // namespace v1 {


// We factor out all common behavior and templatize it so that we can
// can call it from both `v1::` and `internal::`.
namespace common {

template <typename TCommandInfo>
inline TCommandInfo createCommandInfo(
    const Option<std::string>& value = None(),
    const std::vector<std::string>& arguments = {})
{
  TCommandInfo commandInfo;
  if (value.isSome()) {
    commandInfo.set_value(value.get());
  }
  if (!arguments.empty()) {
    commandInfo.set_shell(false);
    foreach (const std::string& arg, arguments) {
      commandInfo.add_arguments(arg);
    }
  }
  return commandInfo;
}


template <typename TExecutorInfo,
          typename TExecutorID,
          typename TResources,
          typename TCommandInfo,
          typename TFrameworkID>
inline TExecutorInfo createExecutorInfo(
    const TExecutorID& executorId,
    const Option<TCommandInfo>& command,
    const Option<TResources>& resources,
    const Option<typename TExecutorInfo::Type>& type,
    const Option<TFrameworkID>& frameworkId)
{
  TExecutorInfo executor;
  executor.mutable_executor_id()->CopyFrom(executorId);
  if (command.isSome()) {
    executor.mutable_command()->CopyFrom(command.get());
  }
  if (resources.isSome()) {
    executor.mutable_resources()->CopyFrom(resources.get());
  }
  if (type.isSome()) {
    executor.set_type(type.get());
  }
  if (frameworkId.isSome()) {
    executor.mutable_framework_id()->CopyFrom(frameworkId.get());
  }
  return executor;
}


template <typename TExecutorInfo,
          typename TExecutorID,
          typename TResources,
          typename TCommandInfo,
          typename TFrameworkID>
inline TExecutorInfo createExecutorInfo(
    const std::string& _executorId,
    const Option<TCommandInfo>& command,
    const Option<TResources>& resources,
    const Option<typename TExecutorInfo::Type>& type,
    const Option<TFrameworkID>& frameworkId)
{
  TExecutorID executorId;
  executorId.set_value(_executorId);
  return createExecutorInfo<TExecutorInfo,
                            TExecutorID,
                            TResources,
                            TCommandInfo,
                            TFrameworkID>(
      executorId, command, resources, type, frameworkId);
}


template <typename TExecutorInfo,
          typename TExecutorID,
          typename TResources,
          typename TCommandInfo,
          typename TFrameworkID>
inline TExecutorInfo createExecutorInfo(
    const std::string& executorId,
    const Option<TCommandInfo>& command = None(),
    const Option<std::string>& resources = None(),
    const Option<typename TExecutorInfo::Type>& type = None(),
    const Option<TFrameworkID>& frameworkId = None())
{
  if (resources.isSome()) {
    return createExecutorInfo<TExecutorInfo,
                              TExecutorID,
                              TResources,
                              TCommandInfo,
                              TFrameworkID>(
        executorId,
        command,
        TResources::parse(resources.get()).get(),
        type,
        frameworkId);
  }

  return createExecutorInfo<TExecutorInfo,
                            TExecutorID,
                            TResources,
                            TCommandInfo,
                            TFrameworkID>(
      executorId, command, Option<TResources>::none(), type, frameworkId);
}


template <typename TExecutorInfo,
          typename TExecutorID,
          typename TResources,
          typename TCommandInfo,
          typename TFrameworkID>
inline TExecutorInfo createExecutorInfo(
    const TExecutorID& executorId,
    const Option<TCommandInfo>& command,
    const std::string& resources,
    const Option<typename TExecutorInfo::Type>& type = None(),
    const Option<TFrameworkID>& frameworkId = None())
{
  return createExecutorInfo<TExecutorInfo,
                            TExecutorID,
                            TResources,
                            TCommandInfo,
                            TFrameworkID>(
      executorId,
      command,
      TResources::parse(resources).get(),
      type,
      frameworkId);
}


template <typename TExecutorInfo,
          typename TExecutorID,
          typename TResources,
          typename TCommandInfo,
          typename TFrameworkID>
inline TExecutorInfo createExecutorInfo(
    const std::string& executorId,
    const std::string& command,
    const Option<std::string>& resources = None(),
    const Option<typename TExecutorInfo::Type>& type = None(),
    const Option<TFrameworkID>& frameworkId = None())
{
  TCommandInfo commandInfo = createCommandInfo<TCommandInfo>(command);
  return createExecutorInfo<TExecutorInfo,
                            TExecutorID,
                            TResources,
                            TCommandInfo,
                            TFrameworkID>(
      executorId, commandInfo, resources, type, frameworkId);
}


template <typename TImage>
inline TImage createDockerImage(const std::string& imageName)
{
  TImage image;
  image.set_type(TImage::DOCKER);
  image.mutable_docker()->set_name(imageName);
  return image;
}


template <typename TVolume>
inline TVolume createVolumeSandboxPath(
    const std::string& containerPath,
    const std::string& sandboxPath,
    const typename TVolume::Mode& mode)
{
  TVolume volume;
  volume.set_container_path(containerPath);
  volume.set_mode(mode);

  // TODO(jieyu): Use TVolume::Source::SANDBOX_PATH.
  volume.set_host_path(sandboxPath);

  return volume;
}


template <typename TVolume>
inline TVolume createVolumeHostPath(
    const std::string& containerPath,
    const std::string& hostPath,
    const typename TVolume::Mode& mode,
    const Option<MountPropagation::Mode>& mountPropagationMode = None())
{
  TVolume volume;
  volume.set_container_path(containerPath);
  volume.set_mode(mode);

  typename TVolume::Source* source = volume.mutable_source();
  source->set_type(TVolume::Source::HOST_PATH);
  source->mutable_host_path()->set_path(hostPath);

  if (mountPropagationMode.isSome()) {
    source
      ->mutable_host_path()
      ->mutable_mount_propagation()
      ->set_mode(mountPropagationMode.get());
  }

  return volume;
}


template <typename TVolume, typename TImage>
inline TVolume createVolumeFromDockerImage(
    const std::string& containerPath,
    const std::string& imageName,
    const typename TVolume::Mode& mode)
{
  TVolume volume;
  volume.set_container_path(containerPath);
  volume.set_mode(mode);
  volume.mutable_image()->CopyFrom(createDockerImage<TImage>(imageName));
  return volume;
}


template <typename TNetworkInfo>
inline TNetworkInfo createNetworkInfo(
    const std::string& networkName)
{
  TNetworkInfo info;
  info.set_name(networkName);
  return info;
}


template <typename TContainerInfo, typename TVolume, typename TImage>
inline TContainerInfo createContainerInfo(
    const Option<std::string>& imageName = None(),
    const std::vector<TVolume>& volumes = {})
{
  TContainerInfo info;
  info.set_type(TContainerInfo::MESOS);

  if (imageName.isSome()) {
    TImage* image = info.mutable_mesos()->mutable_image();
    image->CopyFrom(createDockerImage<TImage>(imageName.get()));
  }

  foreach (const TVolume& volume, volumes) {
    info.add_volumes()->CopyFrom(volume);
  }

  return info;
}


inline void setAgentID(TaskInfo* task, const SlaveID& slaveId)
{
  task->mutable_slave_id()->CopyFrom(slaveId);
}
inline void setAgentID(
    mesos::v1::TaskInfo* task,
    const mesos::v1::AgentID& agentId)
{
  task->mutable_agent_id()->CopyFrom(agentId);
}


// TODO(bmahler): Refactor this to make the distinction between
// command tasks and executor tasks clearer.
template <
    typename TTaskInfo,
    typename TExecutorID,
    typename TSlaveID,
    typename TResources,
    typename TExecutorInfo,
    typename TCommandInfo,
    typename TOffer>
inline TTaskInfo createTask(
    const TSlaveID& slaveId,
    const TResources& resources,
    const TCommandInfo& command,
    const Option<TExecutorID>& executorId = None(),
    const std::string& name = "test-task",
    const std::string& id = id::UUID::random().toString())
{
  TTaskInfo task;
  task.set_name(name);
  task.mutable_task_id()->set_value(id);
  setAgentID(&task, slaveId);
  task.mutable_resources()->CopyFrom(resources);
  if (executorId.isSome()) {
    TExecutorInfo executor;
    executor.mutable_executor_id()->CopyFrom(executorId.get());
    executor.mutable_command()->CopyFrom(command);
    task.mutable_executor()->CopyFrom(executor);
  } else {
    task.mutable_command()->CopyFrom(command);
  }

  return task;
}


template <
    typename TTaskInfo,
    typename TExecutorID,
    typename TSlaveID,
    typename TResources,
    typename TExecutorInfo,
    typename TCommandInfo,
    typename TOffer>
inline TTaskInfo createTask(
    const TSlaveID& slaveId,
    const TResources& resources,
    const std::string& command,
    const Option<TExecutorID>& executorId = None(),
    const std::string& name = "test-task",
    const std::string& id = id::UUID::random().toString())
{
  return createTask<
      TTaskInfo,
      TExecutorID,
      TSlaveID,
      TResources,
      TExecutorInfo,
      TCommandInfo,
      TOffer>(
          slaveId,
          resources,
          createCommandInfo<TCommandInfo>(command),
          executorId,
          name,
          id);
}


template <
    typename TTaskInfo,
    typename TExecutorID,
    typename TSlaveID,
    typename TResources,
    typename TExecutorInfo,
    typename TCommandInfo,
    typename TOffer>
inline TTaskInfo createTask(
    const TOffer& offer,
    const std::string& command,
    const Option<TExecutorID>& executorId = None(),
    const std::string& name = "test-task",
    const std::string& id = id::UUID::random().toString())
{
  return createTask<
      TTaskInfo,
      TExecutorID,
      TSlaveID,
      TResources,
      TExecutorInfo,
      TCommandInfo,
      TOffer>(
          offer.slave_id(),
          offer.resources(),
          command,
          executorId,
          name,
          id);
}


template <typename TTaskGroupInfo, typename TTaskInfo>
inline TTaskGroupInfo createTaskGroupInfo(const std::vector<TTaskInfo>& tasks)
{
  TTaskGroupInfo taskGroup;
  foreach (const TTaskInfo& task, tasks) {
    taskGroup.add_tasks()->CopyFrom(task);
  }
  return taskGroup;
}


template <typename TResource>
inline typename TResource::ReservationInfo createStaticReservationInfo(
    const std::string& role)
{
  typename TResource::ReservationInfo info;
  info.set_type(TResource::ReservationInfo::STATIC);
  info.set_role(role);
  return info;
}


template <typename TResource, typename TLabels>
inline typename TResource::ReservationInfo createDynamicReservationInfo(
    const std::string& role,
    const Option<std::string>& principal = None(),
    const Option<TLabels>& labels = None())
{
  typename TResource::ReservationInfo info;

  info.set_type(TResource::ReservationInfo::DYNAMIC);
  info.set_role(role);

  if (principal.isSome()) {
    info.set_principal(principal.get());
  }

  if (labels.isSome()) {
    info.mutable_labels()->CopyFrom(labels.get());
  }

  return info;
}


template <
    typename TResource,
    typename TResources,
    typename... TReservationInfos>
inline TResource createReservedResource(
    const std::string& name,
    const std::string& value,
    const TReservationInfos&... reservations)
{
  std::initializer_list<typename TResource::ReservationInfo> reservations_ = {
    reservations...
  };

  TResource resource = TResources::parse(name, value, "*").get();
  resource.mutable_reservations()->CopyFrom(
      google::protobuf::RepeatedPtrField<typename TResource::ReservationInfo>{
        reservations_.begin(), reservations_.end()});

  return resource;
}


// NOTE: We only set the volume in DiskInfo if 'containerPath' is set.
// If volume mode is not specified, Volume::RW will be used (assuming
// 'containerPath' is set).
template <typename TResource, typename TVolume>
inline typename TResource::DiskInfo createDiskInfo(
    const Option<std::string>& persistenceId,
    const Option<std::string>& containerPath,
    const Option<typename TVolume::Mode>& mode = None(),
    const Option<std::string>& hostPath = None(),
    const Option<typename TResource::DiskInfo::Source>& source = None(),
    const Option<std::string>& principal = None())
{
  typename TResource::DiskInfo info;

  if (persistenceId.isSome()) {
    info.mutable_persistence()->set_id(persistenceId.get());
  }

  if (principal.isSome()) {
    info.mutable_persistence()->set_principal(principal.get());
  }

  if (containerPath.isSome()) {
    TVolume volume;
    volume.set_container_path(containerPath.get());
    volume.set_mode(mode.isSome() ? mode.get() : TVolume::RW);

    if (hostPath.isSome()) {
      volume.set_host_path(hostPath.get());
    }

    info.mutable_volume()->CopyFrom(volume);
  }

  if (source.isSome()) {
    info.mutable_source()->CopyFrom(source.get());
  }

  return info;
}


// Helper for creating a disk source with type `PATH`.
template <typename TResource>
inline typename TResource::DiskInfo::Source createDiskSourcePath(
    const Option<std::string>& root = None(),
    const Option<std::string>& id = None(),
    const Option<std::string>& profile = None())
{
  typename TResource::DiskInfo::Source source;

  source.set_type(TResource::DiskInfo::Source::PATH);

  if (root.isSome()) {
    source.mutable_path()->set_root(root.get());
  }

  if (id.isSome()) {
    source.set_id(id.get());
  }

  if (profile.isSome()) {
    source.set_profile(profile.get());
  }

  return source;
}


// Helper for creating a disk source with type `MOUNT`.
template <typename TResource>
inline typename TResource::DiskInfo::Source createDiskSourceMount(
    const Option<std::string>& root = None(),
    const Option<std::string>& id = None(),
    const Option<std::string>& profile = None())
{
  typename TResource::DiskInfo::Source source;

  source.set_type(TResource::DiskInfo::Source::MOUNT);

  if (root.isSome()) {
    source.mutable_mount()->set_root(root.get());
  }

  if (id.isSome()) {
    source.set_id(id.get());
  }

  if (profile.isSome()) {
    source.set_profile(profile.get());
  }

  return source;
}


// Helper for creating a disk source with type `BLOCK'
template <typename TResource>
inline typename TResource::DiskInfo::Source createDiskSourceBlock(
    const Option<std::string>& id = None(),
    const Option<std::string>& profile = None())
{
  typename TResource::DiskInfo::Source source;

  source.set_type(TResource::DiskInfo::Source::BLOCK);

  if (id.isSome()) {
    source.set_id(id.get());
  }

  if (profile.isSome()) {
    source.set_profile(profile.get());
  }

  return source;
}


// Helper for creating a disk source with type `RAW'.
template <typename TResource>
inline typename TResource::DiskInfo::Source createDiskSourceRaw(
    const Option<std::string>& id = None(),
    const Option<std::string>& profile = None())
{
  typename TResource::DiskInfo::Source source;

  source.set_type(TResource::DiskInfo::Source::RAW);

  if (id.isSome()) {
    source.set_id(id.get());
  }

  if (profile.isSome()) {
    source.set_profile(profile.get());
  }

  return source;
}


// Helper for creating a disk resource.
template <typename TResource, typename TResources, typename TVolume>
inline TResource createDiskResource(
    const std::string& value,
    const std::string& role,
    const Option<std::string>& persistenceID,
    const Option<std::string>& containerPath,
    const Option<typename TResource::DiskInfo::Source>& source = None(),
    bool isShared = false)
{
  TResource resource = TResources::parse("disk", value, role).get();

  if (persistenceID.isSome() || containerPath.isSome() || source.isSome()) {
    resource.mutable_disk()->CopyFrom(
        createDiskInfo<TResource, TVolume>(
            persistenceID,
            containerPath,
            None(),
            None(),
            source));

    if (isShared) {
      resource.mutable_shared();
    }
  }

  return resource;
}


// Note that `reservationPrincipal` should be specified if and only if
// the volume uses dynamically reserved resources.
template <typename TResource, typename TResources, typename TVolume>
inline TResource createPersistentVolume(
    const Bytes& size,
    const std::string& role,
    const std::string& persistenceId,
    const std::string& containerPath,
    const Option<std::string>& reservationPrincipal = None(),
    const Option<typename TResource::DiskInfo::Source>& source = None(),
    const Option<std::string>& creatorPrincipal = None(),
    bool isShared = false)
{
  TResource volume = TResources::parse(
      "disk",
      stringify((double) size.bytes() / Bytes::MEGABYTES),
      role).get();

  volume.mutable_disk()->CopyFrom(
      createDiskInfo<TResource, TVolume>(
          persistenceId,
          containerPath,
          None(),
          None(),
          source,
          creatorPrincipal));

  if (reservationPrincipal.isSome()) {
    typename TResource::ReservationInfo& reservation =
      *volume.mutable_reservations()->rbegin();

    reservation.set_type(TResource::ReservationInfo::DYNAMIC);
    reservation.set_principal(reservationPrincipal.get());
  }

  if (isShared) {
    volume.mutable_shared();
  }

  return volume;
}


// Note that `reservationPrincipal` should be specified if and only if
// the volume uses dynamically reserved resources.
template <typename TResource, typename TResources, typename TVolume>
inline TResource createPersistentVolume(
    TResource volume,
    const std::string& persistenceId,
    const std::string& containerPath,
    const Option<std::string>& reservationPrincipal = None(),
    const Option<std::string>& creatorPrincipal = None(),
    bool isShared = false)
{
  Option<typename TResource::DiskInfo::Source> source = None();
  if (volume.has_disk() && volume.disk().has_source()) {
    source = volume.disk().source();
  }

  volume.mutable_disk()->CopyFrom(
      createDiskInfo<TResource, TVolume>(
          persistenceId,
          containerPath,
          None(),
          None(),
          source,
          creatorPrincipal));

  if (reservationPrincipal.isSome()) {
    typename TResource::ReservationInfo& reservation =
      *volume.mutable_reservations()->rbegin();

    reservation.set_type(TResource::ReservationInfo::DYNAMIC);
    reservation.set_principal(reservationPrincipal.get());
  }

  if (isShared) {
    volume.mutable_shared();
  }

  return volume;
}


template <typename TCredential>
inline process::http::Headers createBasicAuthHeaders(
    const TCredential& credential)
{
  return process::http::Headers({{
      "Authorization",
      "Basic " +
        base64::encode(credential.principal() + ":" + credential.secret())
  }});
}


// Create WeightInfos from the specified weights flag.
template <typename TWeightInfo>
inline google::protobuf::RepeatedPtrField<TWeightInfo> createWeightInfos(
    const std::string& weightsFlag)
{
  google::protobuf::RepeatedPtrField<TWeightInfo> infos;
  std::vector<std::string> tokens = strings::tokenize(weightsFlag, ",");
  foreach (const std::string& token, tokens) {
    std::vector<std::string> pair = strings::tokenize(token, "=");
    EXPECT_EQ(2u, pair.size());
    double weight = atof(pair[1].c_str());
    TWeightInfo weightInfo;
    weightInfo.set_role(pair[0]);
    weightInfo.set_weight(weight);
    infos.Add()->CopyFrom(weightInfo);
  }

  return infos;
}


// Convert WeightInfos protobuf to weights hashmap.
template <typename TWeightInfo>
inline hashmap<std::string, double> convertToHashmap(
    const google::protobuf::RepeatedPtrField<TWeightInfo> weightInfos)
{
  hashmap<std::string, double> weights;

  foreach (const TWeightInfo& weightInfo, weightInfos) {
    weights[weightInfo.role()] = weightInfo.weight();
  }

  return weights;
}


// Helper to create DomainInfo.
template <typename TDomainInfo>
inline TDomainInfo createDomainInfo(
    const std::string& regionName,
    const std::string& zoneName)
{
  TDomainInfo domain;

  domain.mutable_fault_domain()->mutable_region()->set_name(regionName);
  domain.mutable_fault_domain()->mutable_zone()->set_name(zoneName);

  return domain;
}


// Helpers for creating operations.
template <typename TResources, typename TOffer>
inline typename TOffer::Operation RESERVE(const TResources& resources)
{
  typename TOffer::Operation operation;
  operation.set_type(TOffer::Operation::RESERVE);
  operation.mutable_reserve()->mutable_resources()->CopyFrom(resources);
  return operation;
}


template <typename TResources, typename TOffer>
inline typename TOffer::Operation UNRESERVE(const TResources& resources)
{
  typename TOffer::Operation operation;
  operation.set_type(TOffer::Operation::UNRESERVE);
  operation.mutable_unreserve()->mutable_resources()->CopyFrom(resources);
  return operation;
}


template <typename TResources, typename TOffer>
inline typename TOffer::Operation CREATE(const TResources& volumes)
{
  typename TOffer::Operation operation;
  operation.set_type(TOffer::Operation::CREATE);
  operation.mutable_create()->mutable_volumes()->CopyFrom(volumes);
  return operation;
}


template <typename TResources, typename TOffer>
inline typename TOffer::Operation DESTROY(const TResources& volumes)
{
  typename TOffer::Operation operation;
  operation.set_type(TOffer::Operation::DESTROY);
  operation.mutable_destroy()->mutable_volumes()->CopyFrom(volumes);
  return operation;
}


template <typename TOffer, typename TTaskInfo>
inline typename TOffer::Operation LAUNCH(const std::vector<TTaskInfo>& tasks)
{
  typename TOffer::Operation operation;
  operation.set_type(TOffer::Operation::LAUNCH);

  foreach (const TTaskInfo& task, tasks) {
    operation.mutable_launch()->add_task_infos()->CopyFrom(task);
  }

  return operation;
}


template <typename TExecutorInfo, typename TTaskGroupInfo, typename TOffer>
inline typename TOffer::Operation LAUNCH_GROUP(
    const TExecutorInfo& executorInfo,
    const TTaskGroupInfo& taskGroup)
{
  typename TOffer::Operation operation;
  operation.set_type(TOffer::Operation::LAUNCH_GROUP);
  operation.mutable_launch_group()->mutable_executor()->CopyFrom(executorInfo);
  operation.mutable_launch_group()->mutable_task_group()->CopyFrom(taskGroup);
  return operation;
}


template <typename TResource, typename TTargetType, typename TOffer>
inline typename TOffer::Operation CREATE_VOLUME(
    const TResource& source,
    const TTargetType& type)
{
  typename TOffer::Operation operation;
  operation.set_type(TOffer::Operation::CREATE_VOLUME);
  operation.mutable_create_volume()->mutable_source()->CopyFrom(source);
  operation.mutable_create_volume()->set_target_type(type);
  return operation;
}


template <typename TResource, typename TOffer>
inline typename TOffer::Operation DESTROY_VOLUME(const TResource& volume)
{
  typename TOffer::Operation operation;
  operation.set_type(TOffer::Operation::DESTROY_VOLUME);
  operation.mutable_destroy_volume()->mutable_volume()->CopyFrom(volume);
  return operation;
}


template <typename TResource, typename TOffer>
inline typename TOffer::Operation CREATE_BLOCK(const TResource& source)
{
  typename TOffer::Operation operation;
  operation.set_type(TOffer::Operation::CREATE_BLOCK);
  operation.mutable_create_block()->mutable_source()->CopyFrom(source);
  return operation;
}


template <typename TResource, typename TOffer>
inline typename TOffer::Operation DESTROY_BLOCK(const TResource& block)
{
  typename TOffer::Operation operation;
  operation.set_type(TOffer::Operation::DESTROY_BLOCK);
  operation.mutable_destroy_block()->mutable_block()->CopyFrom(block);
  return operation;
}


template <typename TParameters, typename TParameter>
inline TParameters parameterize(const ACLs& acls)
{
  TParameters parameters;
  TParameter* parameter = parameters.add_parameter();
  parameter->set_key("acls");
  parameter->set_value(std::string(jsonify(JSON::Protobuf(acls))));

  return parameters;
}
} // namespace common {


// TODO(jmlvanre): Remove `inline` once we have adjusted all tests to
// distinguish between `internal` and `v1`.
inline namespace internal {
template <typename... Args>
inline ExecutorInfo createExecutorInfo(Args&&... args)
{
  return common::createExecutorInfo<
      ExecutorInfo,
      ExecutorID,
      Resources,
      CommandInfo,
      FrameworkID>(std::forward<Args>(args)...);
}


// We specify the argument to allow brace initialized construction.
inline CommandInfo createCommandInfo(
    const Option<std::string>& value = None(),
    const std::vector<std::string>& arguments = {})
{
  return common::createCommandInfo<CommandInfo>(value, arguments);
}


// Almost a direct snippet of code at the bottom of `Slave::launchExecutor`.
inline mesos::slave::ContainerConfig createContainerConfig(
    const Option<TaskInfo>& taskInfo,
    const ExecutorInfo& executorInfo,
    const std::string& sandboxDirectory,
    const Option<std::string>& user = None())
{
  mesos::slave::ContainerConfig containerConfig;
  containerConfig.mutable_executor_info()->CopyFrom(executorInfo);
  containerConfig.mutable_command_info()->CopyFrom(executorInfo.command());
  containerConfig.mutable_resources()->CopyFrom(executorInfo.resources());
  containerConfig.set_directory(sandboxDirectory);

  if (user.isSome()) {
    containerConfig.set_user(user.get());
  }

  if (taskInfo.isSome()) {
    containerConfig.mutable_task_info()->CopyFrom(taskInfo.get());

    if (taskInfo.get().has_container()) {
      containerConfig.mutable_container_info()
        ->CopyFrom(taskInfo.get().container());
    }
  } else {
    if (executorInfo.has_container()) {
      containerConfig.mutable_container_info()
        ->CopyFrom(executorInfo.container());
    }
  }

  return containerConfig;
}


// Almost a direct snippet of code in `Slave::Http::_launchNestedContainer`.
inline mesos::slave::ContainerConfig createContainerConfig(
    const CommandInfo& commandInfo,
    const Option<ContainerInfo>& containerInfo = None(),
    const Option<mesos::slave::ContainerClass>& containerClass = None(),
    const Option<std::string>& user = None())
{
  mesos::slave::ContainerConfig containerConfig;
  containerConfig.mutable_command_info()->CopyFrom(commandInfo);

  if (user.isSome()) {
    containerConfig.set_user(user.get());
  }

  if (containerInfo.isSome()) {
    containerConfig.mutable_container_info()->CopyFrom(containerInfo.get());
  }

  if (containerClass.isSome()) {
    containerConfig.set_container_class(containerClass.get());
  }

  return containerConfig;
}


// Helper for creating standalone container configs.
inline mesos::slave::ContainerConfig createContainerConfig(
    const CommandInfo& commandInfo,
    const std::string& resources,
    const std::string& sandboxDirectory,
    const Option<ContainerInfo>& containerInfo = None(),
    const Option<std::string>& user = None())
{
  mesos::slave::ContainerConfig containerConfig;
  containerConfig.mutable_command_info()->CopyFrom(commandInfo);
  containerConfig.mutable_resources()->CopyFrom(
      Resources::parse(resources).get());

  containerConfig.set_directory(sandboxDirectory);

  if (user.isSome()) {
    containerConfig.set_user(user.get());
  }

  if (containerInfo.isSome()) {
    containerConfig.mutable_container_info()->CopyFrom(containerInfo.get());
  }

  return containerConfig;
}


template <typename... Args>
inline Image createDockerImage(Args&&... args)
{
  return common::createDockerImage<Image>(std::forward<Args>(args)...);
}


template <typename... Args>
inline Volume createVolumeSandboxPath(Args&&... args)
{
  return common::createVolumeSandboxPath<Volume>(std::forward<Args>(args)...);
}


template <typename... Args>
inline Volume createVolumeHostPath(Args&&... args)
{
  return common::createVolumeHostPath<Volume>(std::forward<Args>(args)...);
}


template <typename... Args>
inline Volume createVolumeFromDockerImage(Args&&... args)
{
  return common::createVolumeFromDockerImage<Volume, Image>(
      std::forward<Args>(args)...);
}


template <typename... Args>
inline NetworkInfo createNetworkInfo(Args&&... args)
{
  return common::createNetworkInfo<NetworkInfo>(std::forward<Args>(args)...);
}


// We specify the argument to allow brace initialized construction.
inline ContainerInfo createContainerInfo(
    const Option<std::string>& imageName = None(),
    const std::vector<Volume>& volumes = {})
{
  return common::createContainerInfo<ContainerInfo, Volume, Image>(
      imageName,
      volumes);
}


template <typename... Args>
inline TaskInfo createTask(Args&&... args)
{
  return common::createTask<
      TaskInfo,
      ExecutorID,
      SlaveID,
      Resources,
      ExecutorInfo,
      CommandInfo,
      Offer>(std::forward<Args>(args)...);
}


// We specify the argument to allow brace initialized construction.
inline TaskGroupInfo createTaskGroupInfo(const std::vector<TaskInfo>& tasks)
{
  return common::createTaskGroupInfo<TaskGroupInfo, TaskInfo>(tasks);
}


inline Resource::ReservationInfo createStaticReservationInfo(
    const std::string& role)
{
  return common::createStaticReservationInfo<Resource>(role);
}


inline Resource::ReservationInfo createDynamicReservationInfo(
    const std::string& role,
    const Option<std::string>& principal = None(),
    const Option<Labels>& labels = None())
{
  return common::createDynamicReservationInfo<Resource, Labels>(
      role, principal, labels);
}


template <typename... Args>
inline Resource createReservedResource(Args&&... args)
{
  return common::createReservedResource<Resource, Resources>(
      std::forward<Args>(args)...);
}


template <typename... Args>
inline Resource::DiskInfo createDiskInfo(Args&&... args)
{
  return common::createDiskInfo<Resource, Volume>(std::forward<Args>(args)...);
}


template <typename... Args>
inline Resource::DiskInfo::Source createDiskSourcePath(Args&&... args)
{
  return common::createDiskSourcePath<Resource>(std::forward<Args>(args)...);
}


template <typename... Args>
inline Resource::DiskInfo::Source createDiskSourceMount(Args&&... args)
{
  return common::createDiskSourceMount<Resource>(std::forward<Args>(args)...);
}


template <typename... Args>
inline Resource::DiskInfo::Source createDiskSourceBlock(Args&&... args)
{
  return common::createDiskSourceBlock<Resource>(std::forward<Args>(args)...);
}


template <typename... Args>
inline Resource::DiskInfo::Source createDiskSourceRaw(Args&&... args)
{
  return common::createDiskSourceRaw<Resource>(std::forward<Args>(args)...);
}


template <typename... Args>
inline Resource createDiskResource(Args&&... args)
{
  return common::createDiskResource<Resource, Resources, Volume>(
      std::forward<Args>(args)...);
}


template <typename... Args>
inline Resource createPersistentVolume(Args&&... args)
{
  return common::createPersistentVolume<Resource, Resources, Volume>(
      std::forward<Args>(args)...);
}


template <typename... Args>
inline process::http::Headers createBasicAuthHeaders(Args&&... args)
{
  return common::createBasicAuthHeaders<Credential>(
      std::forward<Args>(args)...);
}


template <typename... Args>
inline google::protobuf::RepeatedPtrField<WeightInfo> createWeightInfos(
    Args&&... args)
{
  return common::createWeightInfos<WeightInfo>(std::forward<Args>(args)...);
}


template <typename... Args>
inline hashmap<std::string, double> convertToHashmap(Args&&... args)
{
  return common::convertToHashmap<WeightInfo>(std::forward<Args>(args)...);
}


template <typename... Args>
inline DomainInfo createDomainInfo(Args&&... args)
{
  return common::createDomainInfo<DomainInfo>(std::forward<Args>(args)...);
}


template <typename... Args>
inline Offer::Operation RESERVE(Args&&... args)
{
  return common::RESERVE<Resources, Offer>(std::forward<Args>(args)...);
}


template <typename... Args>
inline Offer::Operation UNRESERVE(Args&&... args)
{
  return common::UNRESERVE<Resources, Offer>(std::forward<Args>(args)...);
}


template <typename... Args>
inline Offer::Operation CREATE(Args&&... args)
{
  return common::CREATE<Resources, Offer>(std::forward<Args>(args)...);
}


template <typename... Args>
inline Offer::Operation DESTROY(Args&&... args)
{
  return common::DESTROY<Resources, Offer>(std::forward<Args>(args)...);
}


// We specify the argument to allow brace initialized construction.
inline Offer::Operation LAUNCH(const std::vector<TaskInfo>& tasks)
{
  return common::LAUNCH<Offer, TaskInfo>(tasks);
}


template <typename... Args>
inline Offer::Operation LAUNCH_GROUP(Args&&... args)
{
  return common::LAUNCH_GROUP<ExecutorInfo, TaskGroupInfo, Offer>(
      std::forward<Args>(args)...);
}


template <typename... Args>
inline Offer::Operation CREATE_VOLUME(Args&&... args)
{
  return common::CREATE_VOLUME<Resource,
                               Resource::DiskInfo::Source::Type,
                               Offer>(std::forward<Args>(args)...);
}


template <typename... Args>
inline Offer::Operation DESTROY_VOLUME(Args&&... args)
{
  return common::DESTROY_VOLUME<Resource, Offer>(std::forward<Args>(args)...);
}


template <typename... Args>
inline Offer::Operation CREATE_BLOCK(Args&&... args)
{
  return common::CREATE_BLOCK<Resource, Offer>(std::forward<Args>(args)...);
}


template <typename... Args>
inline Offer::Operation DESTROY_BLOCK(Args&&... args)
{
  return common::DESTROY_BLOCK<Resource, Offer>(std::forward<Args>(args)...);
}


template <typename... Args>
inline Parameters parameterize(Args&&... args)
{
  return common::parameterize<Parameters, Parameter>(
      std::forward<Args>(args)...);
}
} // namespace internal {


namespace v1 {
template <typename... Args>
inline mesos::v1::ExecutorInfo createExecutorInfo(Args&&... args)
{
  return common::createExecutorInfo<
      mesos::v1::ExecutorInfo,
      mesos::v1::ExecutorID,
      mesos::v1::Resources,
      mesos::v1::CommandInfo,
      mesos::v1::FrameworkID>(std::forward<Args>(args)...);
}


// We specify the argument to allow brace initialized construction.
inline mesos::v1::CommandInfo createCommandInfo(
    const Option<std::string>& value = None(),
    const std::vector<std::string>& arguments = {})
{
  return common::createCommandInfo<mesos::v1::CommandInfo>(value, arguments);
}


template <typename... Args>
inline mesos::v1::Image createDockerImage(Args&&... args)
{
  return common::createDockerImage<mesos::v1::Image>(
      std::forward<Args>(args)...);
}


template <typename... Args>
inline mesos::v1::Volume createVolumeSandboxPath(Args&&... args)
{
  return common::createVolumeSandboxPath<mesos::v1::Volume>(
      std::forward<Args>(args)...);
}


template <typename... Args>
inline mesos::v1::Volume createVolumeHostPath(Args&&... args)
{
  return common::createVolumeHostPath<mesos::v1::Volume>(
      std::forward<Args>(args)...);
}


template <typename... Args>
inline mesos::v1::Volume createVolumeFromDockerImage(Args&&... args)
{
  return common::createVolumeFromDockerImage<
      mesos::v1::Volume, mesos::v1::Image>(std::forward<Args>(args)...);
}


template <typename... Args>
inline mesos::v1::NetworkInfo createNetworkInfo(Args&&... args)
{
  return common::createNetworkInfo<mesos::v1::NetworkInfo>(
      std::forward<Args>(args)...);
}


// We specify the argument to allow brace initialized construction.
inline mesos::v1::ContainerInfo createContainerInfo(
    const Option<std::string>& imageName = None(),
    const std::vector<mesos::v1::Volume>& volumes = {})
{
  return common::createContainerInfo<
      mesos::v1::ContainerInfo, mesos::v1::Volume, mesos::v1::Image>(
          imageName, volumes);
}


template <typename... Args>
inline mesos::v1::TaskInfo createTask(Args&&... args)
{
  return common::createTask<
      mesos::v1::TaskInfo,
      mesos::v1::ExecutorID,
      mesos::v1::AgentID,
      mesos::v1::Resources,
      mesos::v1::ExecutorInfo,
      mesos::v1::CommandInfo,
      mesos::v1::Offer>(std::forward<Args>(args)...);
}


// We specify the argument to allow brace initialized construction.
inline mesos::v1::TaskGroupInfo createTaskGroupInfo(
    const std::vector<mesos::v1::TaskInfo>& tasks)
{
  return common::createTaskGroupInfo<
      mesos::v1::TaskGroupInfo,
      mesos::v1::TaskInfo>(tasks);
}


inline mesos::v1::Resource::ReservationInfo createStaticReservationInfo(
    const std::string& role)
{
  return common::createStaticReservationInfo<mesos::v1::Resource>(role);
}


inline mesos::v1::Resource::ReservationInfo createDynamicReservationInfo(
    const std::string& role,
    const Option<std::string>& principal = None(),
    const Option<mesos::v1::Labels>& labels = None())
{
  return common::createDynamicReservationInfo<
             mesos::v1::Resource, mesos::v1::Labels>(role, principal, labels);
}


template <typename... Args>
inline mesos::v1::Resource createReservedResource(Args&&... args)
{
  return common::createReservedResource<
      mesos::v1::Resource, mesos::v1::Resources>(std::forward<Args>(args)...);
}


template <typename... Args>
inline mesos::v1::Resource::DiskInfo createDiskInfo(Args&&... args)
{
  return common::createDiskInfo<mesos::v1::Resource, mesos::v1::Volume>(
      std::forward<Args>(args)...);
}


template <typename... Args>
inline mesos::v1::Resource::DiskInfo::Source createDiskSourcePath(
    Args&&... args)
{
  return common::createDiskSourcePath<mesos::v1::Resource>(
      std::forward<Args>(args)...);
}


template <typename... Args>
inline mesos::v1::Resource::DiskInfo::Source createDiskSourceMount(
    Args&&... args)
{
  return common::createDiskSourceMount<mesos::v1::Resource>(
      std::forward<Args>(args)...);
}


template <typename... Args>
inline mesos::v1::Resource::DiskInfo::Source createDiskSourceBlock(
    Args&&... args)
{
  return common::createDiskSourceBlock<mesos::v1::Resource>(
      std::forward<Args>(args)...);
}


template <typename... Args>
inline mesos::v1::Resource::DiskInfo::Source createDiskSourceRaw(
    Args&&... args)
{
  return common::createDiskSourceRaw<mesos::v1::Resource>(
      std::forward<Args>(args)...);
}


template <typename... Args>
inline mesos::v1::Resource createDiskResource(Args&&... args)
{
  return common::createDiskResource<
      mesos::v1::Resource,
      mesos::v1::Resources,
      mesos::v1::Volume>(std::forward<Args>(args)...);
}


template <typename... Args>
inline mesos::v1::Resource createPersistentVolume(Args&&... args)
{
  return common::createPersistentVolume<
      mesos::v1::Resource,
      mesos::v1::Resources,
      mesos::v1::Volume>(std::forward<Args>(args)...);
}


template <typename... Args>
inline process::http::Headers createBasicAuthHeaders(Args&&... args)
{
  return common::createBasicAuthHeaders<mesos::v1::Credential>(
      std::forward<Args>(args)...);
}


template <typename... Args>
inline google::protobuf::RepeatedPtrField<
    mesos::v1::WeightInfo> createWeightInfos(Args&&... args)
{
  return common::createWeightInfos<mesos::v1::WeightInfo>(
      std::forward<Args>(args)...);
}


template <typename... Args>
inline hashmap<std::string, double> convertToHashmap(Args&&... args)
{
  return common::convertToHashmap<mesos::v1::WeightInfo>(
      std::forward<Args>(args)...);
}


template <typename... Args>
inline mesos::v1::Offer::Operation RESERVE(Args&&... args)
{
  return common::RESERVE<mesos::v1::Resources, mesos::v1::Offer>(
      std::forward<Args>(args)...);
}


template <typename... Args>
inline mesos::v1::Offer::Operation UNRESERVE(Args&&... args)
{
  return common::UNRESERVE<mesos::v1::Resources, mesos::v1::Offer>(
      std::forward<Args>(args)...);
}


template <typename... Args>
inline mesos::v1::Offer::Operation CREATE(Args&&... args)
{
  return common::CREATE<mesos::v1::Resources, mesos::v1::Offer>(
      std::forward<Args>(args)...);
}


template <typename... Args>
inline mesos::v1::Offer::Operation DESTROY(Args&&... args)
{
  return common::DESTROY<mesos::v1::Resources, mesos::v1::Offer>(
      std::forward<Args>(args)...);
}


// We specify the argument to allow brace initialized construction.
inline mesos::v1::Offer::Operation LAUNCH(
    const std::vector<mesos::v1::TaskInfo>& tasks)
{
  return common::LAUNCH<mesos::v1::Offer, mesos::v1::TaskInfo>(tasks);
}


template <typename... Args>
inline mesos::v1::Offer::Operation LAUNCH_GROUP(Args&&... args)
{
  return common::LAUNCH_GROUP<
      mesos::v1::ExecutorInfo,
      mesos::v1::TaskGroupInfo,
      mesos::v1::Offer>(std::forward<Args>(args)...);
}


template <typename... Args>
inline mesos::v1::Offer::Operation CREATE_VOLUME(Args&&... args)
{
  return common::CREATE_VOLUME<mesos::v1::Resource,
                               mesos::v1::Resource::DiskInfo::Source::Type,
                               mesos::v1::Offer>(
      std::forward<Args>(args)...);
}


template <typename... Args>
inline mesos::v1::Offer::Operation DESTROY_VOLUME(Args&&... args)
{
  return common::DESTROY_VOLUME<mesos::v1::Resource, mesos::v1::Offer>(
      std::forward<Args>(args)...);
}


template <typename... Args>
inline mesos::v1::Offer::Operation CREATE_BLOCK(Args&&... args)
{
  return common::CREATE_BLOCK<mesos::v1::Resource, mesos::v1::Offer>(
      std::forward<Args>(args)...);
}


template <typename... Args>
inline mesos::v1::Offer::Operation DESTROY_BLOCK(Args&&... args)
{
  return common::DESTROY_BLOCK<mesos::v1::Resource, mesos::v1::Offer>(
      std::forward<Args>(args)...);
}


template <typename... Args>
inline mesos::v1::Parameters parameterize(Args&&... args)
{
  return common::parameterize<mesos::v1::Parameters, mesos::v1::Parameter>(
      std::forward<Args>(args)...);
}


inline mesos::v1::scheduler::Call createCallAccept(
    const mesos::v1::FrameworkID& frameworkId,
    const mesos::v1::Offer& offer,
    const std::vector<mesos::v1::Offer::Operation>& operations)
{
  mesos::v1::scheduler::Call call;
  call.set_type(mesos::v1::scheduler::Call::ACCEPT);
  call.mutable_framework_id()->CopyFrom(frameworkId);

  mesos::v1::scheduler::Call::Accept* accept = call.mutable_accept();
  accept->add_offer_ids()->CopyFrom(offer.id());

  foreach (const mesos::v1::Offer::Operation& operation, operations) {
    accept->add_operations()->CopyFrom(operation);
  }

  return call;
}


inline mesos::v1::scheduler::Call createCallAcknowledge(
    const mesos::v1::FrameworkID& frameworkId,
    const mesos::v1::AgentID& agentId,
    const mesos::v1::scheduler::Event::Update& update)
{
  mesos::v1::scheduler::Call call;
  call.set_type(mesos::v1::scheduler::Call::ACKNOWLEDGE);
  call.mutable_framework_id()->CopyFrom(frameworkId);

  mesos::v1::scheduler::Call::Acknowledge* acknowledge =
    call.mutable_acknowledge();

  acknowledge->mutable_task_id()->CopyFrom(
      update.status().task_id());

  acknowledge->mutable_agent_id()->CopyFrom(agentId);
  acknowledge->set_uuid(update.status().uuid());

  return call;
}


inline mesos::v1::scheduler::Call createCallKill(
    const mesos::v1::FrameworkID& frameworkId,
    const mesos::v1::TaskID& taskId,
    const Option<mesos::v1::AgentID>& agentId = None(),
    const Option<mesos::v1::KillPolicy>& killPolicy = None())
{
  mesos::v1::scheduler::Call call;
  call.set_type(mesos::v1::scheduler::Call::KILL);
  call.mutable_framework_id()->CopyFrom(frameworkId);

  mesos::v1::scheduler::Call::Kill* kill = call.mutable_kill();
  kill->mutable_task_id()->CopyFrom(taskId);

  if (agentId.isSome()) {
    kill->mutable_agent_id()->CopyFrom(agentId.get());
  }

  if (killPolicy.isSome()) {
    kill->mutable_kill_policy()->CopyFrom(killPolicy.get());
  }

  return call;
}


inline mesos::v1::scheduler::Call createCallSubscribe(
  const mesos::v1::FrameworkInfo& frameworkInfo,
  const Option<mesos::v1::FrameworkID>& frameworkId = None())
{
  mesos::v1::scheduler::Call call;
  call.set_type(mesos::v1::scheduler::Call::SUBSCRIBE);

  call.mutable_subscribe()->mutable_framework_info()->CopyFrom(frameworkInfo);

  if (frameworkId.isSome()) {
    call.mutable_framework_id()->CopyFrom(frameworkId.get());
  }

  return call;
}

} // namespace v1 {


inline mesos::Environment createEnvironment(
    const hashmap<std::string, std::string>& map)
{
  mesos::Environment environment;
  foreachpair (const std::string& key, const std::string& value, map) {
    mesos::Environment::Variable* variable = environment.add_variables();
    variable->set_name(key);
    variable->set_value(value);
  }
  return environment;
}


inline void createDockerIPv6UserNetwork()
{
  // Create a Docker user network with IPv6 enabled.
  Try<std::string> dockerCommand = strings::format(
      "docker network create --driver=bridge --ipv6 "
      "--subnet=fd01::/64 %s",
      DOCKER_IPv6_NETWORK);

  Try<process::Subprocess> s = subprocess(
      dockerCommand.get(),
      process::Subprocess::PATH("/dev/null"),
      process::Subprocess::PATH("/dev/null"),
      process::Subprocess::PIPE());

  ASSERT_SOME(s) << "Unable to create the Docker IPv6 network: "
                 << DOCKER_IPv6_NETWORK;

  process::Future<std::string> err = process::io::read(s->err().get());

  // Wait for the network to be created.
  AWAIT_READY(s->status());
  AWAIT_READY(err);

  ASSERT_SOME(s->status().get());
  ASSERT_EQ(s->status().get().get(), 0)
    << "Unable to create the Docker IPv6 network "
    << DOCKER_IPv6_NETWORK
    << " : " << err.get();
}


inline void removeDockerIPv6UserNetwork()
{
  // Delete the Docker user network.
  Try<std::string> dockerCommand = strings::format(
      "docker network rm %s",
      DOCKER_IPv6_NETWORK);

  Try<process::Subprocess> s = subprocess(
      dockerCommand.get(),
      process::Subprocess::PATH("/dev/null"),
      process::Subprocess::PATH("/dev/null"),
      process::Subprocess::PIPE());

  // This is best effort cleanup. In case of an error just a log an
  // error.
  ASSERT_SOME(s) << "Unable to delete the Docker IPv6 network: "
                 << DOCKER_IPv6_NETWORK;

  process::Future<std::string> err = process::io::read(s->err().get());

  // Wait for the network to be deleted.
  AWAIT_READY(s->status());
  AWAIT_READY(err);

  ASSERT_SOME(s->status().get());
  ASSERT_EQ(s->status().get().get(), 0)
    << "Unable to delete the Docker IPv6 network "
    << DOCKER_IPv6_NETWORK
    << " : " << err.get();
}


// Macros to get/create (default) ExecutorInfos and FrameworkInfos.
#define DEFAULT_EXECUTOR_INFO createExecutorInfo("default", "exit 1")


#define DEFAULT_CREDENTIAL DefaultCredential::create()
#define DEFAULT_CREDENTIAL_2 DefaultCredential2::create()


#define DEFAULT_FRAMEWORK_INFO DefaultFrameworkInfo::create()


#define DEFAULT_EXECUTOR_ID DEFAULT_EXECUTOR_INFO.executor_id()


// Definition of a mock Scheduler to be used in tests with gmock.
class MockScheduler : public Scheduler
{
public:
  MockScheduler();
  virtual ~MockScheduler();

  MOCK_METHOD3(registered, void(SchedulerDriver*,
                                const FrameworkID&,
                                const MasterInfo&));
  MOCK_METHOD2(reregistered, void(SchedulerDriver*, const MasterInfo&));
  MOCK_METHOD1(disconnected, void(SchedulerDriver*));
  MOCK_METHOD2(resourceOffers, void(SchedulerDriver*,
                                    const std::vector<Offer>&));
  MOCK_METHOD2(offerRescinded, void(SchedulerDriver*, const OfferID&));
  MOCK_METHOD2(statusUpdate, void(SchedulerDriver*, const TaskStatus&));
  MOCK_METHOD4(frameworkMessage, void(SchedulerDriver*,
                                      const ExecutorID&,
                                      const SlaveID&,
                                      const std::string&));
  MOCK_METHOD2(slaveLost, void(SchedulerDriver*, const SlaveID&));
  MOCK_METHOD4(executorLost, void(SchedulerDriver*,
                                  const ExecutorID&,
                                  const SlaveID&,
                                  int));
  MOCK_METHOD2(error, void(SchedulerDriver*, const std::string&));
};

// For use with a MockScheduler, for example:
// EXPECT_CALL(sched, resourceOffers(_, _))
//   .WillOnce(LaunchTasks(EXECUTOR, TASKS, CPUS, MEM, ROLE));
// Launches up to TASKS no-op tasks, if possible,
// each with CPUS cpus and MEM memory and EXECUTOR executor.
ACTION_P5(LaunchTasks, executor, tasks, cpus, mem, role)
{
  SchedulerDriver* driver = arg0;
  std::vector<Offer> offers = arg1;
  int numTasks = tasks;

  int launched = 0;
  for (size_t i = 0; i < offers.size(); i++) {
    const Offer& offer = offers[i];

    Resources taskResources = Resources::parse(
        "cpus:" + stringify(cpus) + ";mem:" + stringify(mem)).get();

    if (offer.resources_size() > 0 &&
        offer.resources(0).has_allocation_info()) {
      taskResources.allocate(role);
    }

    int nextTaskId = 0;
    std::vector<TaskInfo> tasks;
    Resources remaining = offer.resources();

    while (remaining.toUnreserved().contains(taskResources) &&
           launched < numTasks) {
      TaskInfo task;
      task.set_name("TestTask");
      task.mutable_task_id()->set_value(stringify(nextTaskId++));
      task.mutable_slave_id()->MergeFrom(offer.slave_id());
      task.mutable_executor()->MergeFrom(executor);

      Option<Resources> resources = remaining.find(
          role == std::string("*")
            ? taskResources
            : taskResources.pushReservation(createStaticReservationInfo(role)));

      CHECK_SOME(resources);

      task.mutable_resources()->MergeFrom(resources.get());
      remaining -= resources.get();

      tasks.push_back(task);
      launched++;
    }

    driver->launchTasks(offer.id(), tasks);
  }
}


// Like LaunchTasks, but decline the entire offer and
// don't launch any tasks.
ACTION(DeclineOffers)
{
  SchedulerDriver* driver = arg0;
  std::vector<Offer> offers = arg1;

  for (size_t i = 0; i < offers.size(); i++) {
    driver->declineOffer(offers[i].id());
  }
}


// Like DeclineOffers, but takes a custom filters object.
ACTION_P(DeclineOffers, filters)
{
  SchedulerDriver* driver = arg0;
  std::vector<Offer> offers = arg1;

  for (size_t i = 0; i < offers.size(); i++) {
    driver->declineOffer(offers[i].id(), filters);
  }
}


// For use with a MockScheduler, for example:
// process::Queue<Offer> offers;
// EXPECT_CALL(sched, resourceOffers(_, _))
//   .WillRepeatedly(EnqueueOffers(&offers));
// Enqueues all received offers into the provided queue.
ACTION_P(EnqueueOffers, queue)
{
  std::vector<Offer> offers = arg1;
  foreach (const Offer& offer, offers) {
    queue->put(offer);
  }
}


// Definition of a mock Executor to be used in tests with gmock.
class MockExecutor : public Executor
{
public:
  MockExecutor(const ExecutorID& _id);
  virtual ~MockExecutor();

  MOCK_METHOD4(registered, void(ExecutorDriver*,
                                const ExecutorInfo&,
                                const FrameworkInfo&,
                                const SlaveInfo&));
  MOCK_METHOD2(reregistered, void(ExecutorDriver*, const SlaveInfo&));
  MOCK_METHOD1(disconnected, void(ExecutorDriver*));
  MOCK_METHOD2(launchTask, void(ExecutorDriver*, const TaskInfo&));
  MOCK_METHOD2(killTask, void(ExecutorDriver*, const TaskID&));
  MOCK_METHOD2(frameworkMessage, void(ExecutorDriver*, const std::string&));
  MOCK_METHOD1(shutdown, void(ExecutorDriver*));
  MOCK_METHOD2(error, void(ExecutorDriver*, const std::string&));

  const ExecutorID id;
};


class TestingMesosSchedulerDriver : public MesosSchedulerDriver
{
public:
  TestingMesosSchedulerDriver(
      Scheduler* scheduler,
      mesos::master::detector::MasterDetector* _detector)
    : MesosSchedulerDriver(
          scheduler,
          internal::DEFAULT_FRAMEWORK_INFO,
          "",
          true,
          internal::DEFAULT_CREDENTIAL)
  {
    // No-op destructor as _detector lives on the stack.
    detector =
      std::shared_ptr<mesos::master::detector::MasterDetector>(
          _detector, [](mesos::master::detector::MasterDetector*) {});
  }

  TestingMesosSchedulerDriver(
      Scheduler* scheduler,
      mesos::master::detector::MasterDetector* _detector,
      const FrameworkInfo& framework,
      bool implicitAcknowledgements = true)
    : MesosSchedulerDriver(
          scheduler,
          framework,
          "",
          implicitAcknowledgements,
          internal::DEFAULT_CREDENTIAL)
  {
    // No-op destructor as _detector lives on the stack.
    detector =
      std::shared_ptr<mesos::master::detector::MasterDetector>(
          _detector, [](mesos::master::detector::MasterDetector*) {});
  }

  TestingMesosSchedulerDriver(
      Scheduler* scheduler,
      mesos::master::detector::MasterDetector* _detector,
      const FrameworkInfo& framework,
      bool implicitAcknowledgements,
      const Credential& credential)
    : MesosSchedulerDriver(
          scheduler,
          framework,
          "",
          implicitAcknowledgements,
          credential)
  {
    // No-op destructor as _detector lives on the stack.
    detector =
      std::shared_ptr<mesos::master::detector::MasterDetector>(
          _detector, [](mesos::master::detector::MasterDetector*) {});
  }
};


namespace scheduler {

// A generic mock HTTP scheduler to be used in tests with gmock.
template <typename Mesos, typename Event>
class MockHTTPScheduler
{
public:
  MOCK_METHOD1_T(connected, void(Mesos*));
  MOCK_METHOD1_T(disconnected, void(Mesos*));
  MOCK_METHOD1_T(heartbeat, void(Mesos*));
  MOCK_METHOD2_T(subscribed, void(Mesos*, const typename Event::Subscribed&));
  MOCK_METHOD2_T(offers, void(Mesos*, const typename Event::Offers&));
  MOCK_METHOD2_T(
      inverseOffers,
      void(Mesos*, const typename Event::InverseOffers&));
  MOCK_METHOD2_T(rescind, void(Mesos*, const typename Event::Rescind&));
  MOCK_METHOD2_T(
      rescindInverseOffers,
      void(Mesos*, const typename Event::RescindInverseOffer&));
  MOCK_METHOD2_T(update, void(Mesos*, const typename Event::Update&));
  MOCK_METHOD2_T(
      updateOperationStatus,
      void(Mesos*, const typename Event::UpdateOperationStatus&));
  MOCK_METHOD2_T(message, void(Mesos*, const typename Event::Message&));
  MOCK_METHOD2_T(failure, void(Mesos*, const typename Event::Failure&));
  MOCK_METHOD2_T(error, void(Mesos*, const typename Event::Error&));

  void events(Mesos* mesos, std::queue<Event> events)
  {
    while (!events.empty()) {
      Event event = std::move(events.front());
      events.pop();

      switch (event.type()) {
        case Event::SUBSCRIBED:
          subscribed(mesos, event.subscribed());
          break;
        case Event::OFFERS:
          offers(mesos, event.offers());
          break;
        case Event::INVERSE_OFFERS:
          inverseOffers(mesos, event.inverse_offers());
          break;
        case Event::RESCIND:
          rescind(mesos, event.rescind());
          break;
        case Event::RESCIND_INVERSE_OFFER:
          rescindInverseOffers(mesos, event.rescind_inverse_offer());
          break;
        case Event::UPDATE:
          update(mesos, event.update());
          break;
        case Event::UPDATE_OPERATION_STATUS:
          updateOperationStatus(mesos, event.update_operation_status());
          break;
        case Event::MESSAGE:
          message(mesos, event.message());
          break;
        case Event::FAILURE:
          failure(mesos, event.failure());
          break;
        case Event::ERROR:
          error(mesos, event.error());
          break;
        case Event::HEARTBEAT:
          heartbeat(mesos);
          break;
        case Event::UNKNOWN:
          LOG(FATAL) << "Received unexpected UNKNOWN event";
          break;
      }
    }
  }
};


// A generic testing interface for the scheduler library that can be used to
// test the library across various versions.
template <typename Mesos, typename Event>
class TestMesos : public Mesos
{
public:
  TestMesos(
      const std::string& master,
      ContentType contentType,
      const std::shared_ptr<MockHTTPScheduler<Mesos, Event>>& scheduler,
      const Option<std::shared_ptr<mesos::master::detector::MasterDetector>>&
          detector = None())
    : Mesos(
          master,
          contentType,
          lambda::bind(&MockHTTPScheduler<Mesos, Event>::connected,
                       scheduler,
                       this),
          lambda::bind(&MockHTTPScheduler<Mesos, Event>::disconnected,
                       scheduler,
                       this),
          lambda::bind(&MockHTTPScheduler<Mesos, Event>::events,
                       scheduler,
                       this,
                       lambda::_1),
          v1::DEFAULT_CREDENTIAL,
          detector) {}

  virtual ~TestMesos()
  {
    // Since the destructor for `TestMesos` is invoked first, the library can
    // make more callbacks to the `scheduler` object before the `Mesos` (base
    // class) destructor is invoked. To prevent this, we invoke `stop()` here
    // to explicitly stop the library.
    this->stop();

    bool paused = process::Clock::paused();

    // Need to settle the Clock to ensure that all the pending async callbacks
    // with references to `this` and `scheduler` queued on libprocess are
    // executed before the object is destructed.
    process::Clock::pause();
    process::Clock::settle();

    // Return the Clock to its original state.
    if (!paused) {
      process::Clock::resume();
    }
  }
};

} // namespace scheduler {


namespace v1 {
namespace scheduler {

using Call = mesos::v1::scheduler::Call;
using Event = mesos::v1::scheduler::Event;
using Mesos = mesos::v1::scheduler::Mesos;


using TestMesos = tests::scheduler::TestMesos<
    mesos::v1::scheduler::Mesos,
    mesos::v1::scheduler::Event>;


ACTION_P(SendSubscribe, frameworkInfo)
{
  Call call;
  call.set_type(Call::SUBSCRIBE);
  call.mutable_subscribe()->mutable_framework_info()->CopyFrom(frameworkInfo);

  arg0->send(call);
}


ACTION_P2(SendSubscribe, frameworkInfo, frameworkId)
{
  Call call;
  call.set_type(Call::SUBSCRIBE);
  call.mutable_framework_id()->CopyFrom(frameworkId);
  call.mutable_subscribe()->mutable_framework_info()->CopyFrom(frameworkInfo);
  call.mutable_subscribe()->mutable_framework_info()->mutable_id()->CopyFrom(
      frameworkId);

  arg0->send(call);
}


ACTION_P2(SendAcknowledge, frameworkId, agentId)
{
  Call call;
  call.set_type(Call::ACKNOWLEDGE);
  call.mutable_framework_id()->CopyFrom(frameworkId);

  Call::Acknowledge* acknowledge = call.mutable_acknowledge();
  acknowledge->mutable_task_id()->CopyFrom(arg1.status().task_id());
  acknowledge->mutable_agent_id()->CopyFrom(agentId);
  acknowledge->set_uuid(arg1.status().uuid());

  arg0->send(call);
}

} // namespace scheduler {

using MockHTTPScheduler = tests::scheduler::MockHTTPScheduler<
    mesos::v1::scheduler::Mesos,
    mesos::v1::scheduler::Event>;

} // namespace v1 {


namespace executor {

// A generic mock HTTP executor to be used in tests with gmock.
template <typename Mesos, typename Event>
class MockHTTPExecutor
{
public:
  MOCK_METHOD1_T(connected, void(Mesos*));
  MOCK_METHOD1_T(disconnected, void(Mesos*));
  MOCK_METHOD2_T(subscribed, void(Mesos*, const typename Event::Subscribed&));
  MOCK_METHOD2_T(launch, void(Mesos*, const typename Event::Launch&));
  MOCK_METHOD2_T(launchGroup, void(Mesos*, const typename Event::LaunchGroup&));
  MOCK_METHOD2_T(kill, void(Mesos*, const typename Event::Kill&));
  MOCK_METHOD2_T(message, void(Mesos*, const typename Event::Message&));
  MOCK_METHOD1_T(shutdown, void(Mesos*));
  MOCK_METHOD2_T(error, void(Mesos*, const typename Event::Error&));
  MOCK_METHOD2_T(acknowledged,
                 void(Mesos*, const typename Event::Acknowledged&));

  void events(Mesos* mesos, std::queue<Event> events)
  {
    while (!events.empty()) {
      Event event = std::move(events.front());
      events.pop();

      switch (event.type()) {
        case Event::SUBSCRIBED:
          subscribed(mesos, event.subscribed());
          break;
        case Event::LAUNCH:
          launch(mesos, event.launch());
          break;
        case Event::LAUNCH_GROUP:
          launchGroup(mesos, event.launch_group());
          break;
        case Event::KILL:
          kill(mesos, event.kill());
          break;
        case Event::ACKNOWLEDGED:
          acknowledged(mesos, event.acknowledged());
          break;
        case Event::MESSAGE:
          message(mesos, event.message());
          break;
        case Event::SHUTDOWN:
          shutdown(mesos);
          break;
        case Event::ERROR:
          error(mesos, event.error());
          break;
        case Event::UNKNOWN:
          LOG(FATAL) << "Received unexpected UNKNOWN event";
          break;
      }
    }
  }
};


// A generic testing interface for the executor library that can be used to
// test the library across various versions.
template <typename Mesos, typename Event>
class TestMesos : public Mesos
{
public:
  TestMesos(
      ContentType contentType,
      const std::shared_ptr<MockHTTPExecutor<Mesos, Event>>& executor)
    : Mesos(
          contentType,
          lambda::bind(&MockHTTPExecutor<Mesos, Event>::connected,
                       executor,
                       this),
          lambda::bind(&MockHTTPExecutor<Mesos, Event>::disconnected,
                       executor,
                       this),
          lambda::bind(&MockHTTPExecutor<Mesos, Event>::events,
                       executor,
                       this,
                       lambda::_1)) {}
};

} // namespace executor {


namespace v1 {
namespace executor {

// Alias existing `mesos::v1::executor` classes so that we can easily
// write `v1::executor::` in tests.
using Call = mesos::v1::executor::Call;
using Event = mesos::v1::executor::Event;
using Mesos = mesos::v1::executor::Mesos;


using TestMesos = tests::executor::TestMesos<
    mesos::v1::executor::Mesos,
    mesos::v1::executor::Event>;


// TODO(anand): Move these actions to the `v1::executor` namespace.
ACTION_P2(SendSubscribe, frameworkId, executorId)
{
  mesos::v1::executor::Call call;
  call.mutable_framework_id()->CopyFrom(frameworkId);
  call.mutable_executor_id()->CopyFrom(executorId);

  call.set_type(mesos::v1::executor::Call::SUBSCRIBE);

  call.mutable_subscribe();

  arg0->send(call);
}


ACTION_P3(SendUpdateFromTask, frameworkId, executorId, state)
{
  mesos::v1::TaskStatus status;
  status.mutable_task_id()->CopyFrom(arg1.task().task_id());
  status.mutable_executor_id()->CopyFrom(executorId);
  status.set_state(state);
  status.set_source(mesos::v1::TaskStatus::SOURCE_EXECUTOR);
  status.set_uuid(id::UUID::random().toBytes());

  mesos::v1::executor::Call call;
  call.mutable_framework_id()->CopyFrom(frameworkId);
  call.mutable_executor_id()->CopyFrom(executorId);

  call.set_type(mesos::v1::executor::Call::UPDATE);

  call.mutable_update()->mutable_status()->CopyFrom(status);

  arg0->send(call);
}


ACTION_P3(SendUpdateFromTaskID, frameworkId, executorId, state)
{
  mesos::v1::TaskStatus status;
  status.mutable_task_id()->CopyFrom(arg1.task_id());
  status.mutable_executor_id()->CopyFrom(executorId);
  status.set_state(state);
  status.set_source(mesos::v1::TaskStatus::SOURCE_EXECUTOR);
  status.set_uuid(id::UUID::random().toBytes());

  mesos::v1::executor::Call call;
  call.mutable_framework_id()->CopyFrom(frameworkId);
  call.mutable_executor_id()->CopyFrom(executorId);

  call.set_type(mesos::v1::executor::Call::UPDATE);

  call.mutable_update()->mutable_status()->CopyFrom(status);

  arg0->send(call);
}

} // namespace executor {

using MockHTTPExecutor = tests::executor::MockHTTPExecutor<
    mesos::v1::executor::Mesos,
    mesos::v1::executor::Event>;

} // namespace v1 {


namespace resource_provider {

template <
    typename Event,
    typename Call,
    typename Driver,
    typename ResourceProviderInfo,
    typename Resource,
    typename Resources,
    typename ResourceProviderID,
    typename OperationState,
    typename Operation,
    typename Source>
class MockResourceProvider
{
public:
  MockResourceProvider(
      const ResourceProviderInfo& _info,
      const Option<Resources>& _resources = None())
    : info(_info),
      resources(_resources)
  {
    ON_CALL(*this, connected())
      .WillByDefault(Invoke(
          this,
          &MockResourceProvider<
              Event,
              Call,
              Driver,
              ResourceProviderInfo,
              Resource,
              Resources,
              ResourceProviderID,
              OperationState,
              Operation,
              Source>::connectedDefault));
    EXPECT_CALL(*this, connected()).WillRepeatedly(DoDefault());

    ON_CALL(*this, subscribed(_))
      .WillByDefault(Invoke(
          this,
          &MockResourceProvider<
              Event,
              Call,
              Driver,
              ResourceProviderInfo,
              Resource,
              Resources,
              ResourceProviderID,
              OperationState,
              Operation,
              Source>::subscribedDefault));
    EXPECT_CALL(*this, subscribed(_)).WillRepeatedly(DoDefault());

    ON_CALL(*this, applyOperation(_))
      .WillByDefault(Invoke(
          this,
          &MockResourceProvider<
              Event,
              Call,
              Driver,
              ResourceProviderInfo,
              Resource,
              Resources,
              ResourceProviderID,
              OperationState,
              Operation,
              Source>::operationDefault));
    EXPECT_CALL(*this, applyOperation(_)).WillRepeatedly(DoDefault());

    ON_CALL(*this, publishResources(_))
      .WillByDefault(Invoke(
          this,
          &MockResourceProvider<
              Event,
              Call,
              Driver,
              ResourceProviderInfo,
              Resource,
              Resources,
              ResourceProviderID,
              OperationState,
              Operation,
              Source>::publishDefault));
    EXPECT_CALL(*this, publishResources(_)).WillRepeatedly(DoDefault());
  }

  MOCK_METHOD0_T(connected, void());
  MOCK_METHOD0_T(disconnected, void());
  MOCK_METHOD1_T(subscribed, void(const typename Event::Subscribed&));
  MOCK_METHOD1_T(applyOperation, void(const typename Event::ApplyOperation&));
  MOCK_METHOD1_T(
      publishResources,
      void(const typename Event::PublishResources&));
  MOCK_METHOD1_T(
      acknowledgeOperationStatus,
      void(const typename Event::AcknowledgeOperationStatus&));
  MOCK_METHOD1_T(
      reconcileOperations,
      void(const typename Event::ReconcileOperations&));

  void events(std::queue<Event> events)
  {
    while (!events.empty()) {
      Event event = events.front();
      events.pop();

      switch (event.type()) {
        case Event::SUBSCRIBED:
          subscribed(event.subscribed());
          break;
        case Event::APPLY_OPERATION:
          applyOperation(event.apply_operation());
          break;
        case Event::PUBLISH_RESOURCES:
          publishResources(event.publish_resources());
          break;
        case Event::ACKNOWLEDGE_OPERATION_STATUS:
          acknowledgeOperationStatus(event.acknowledge_operation_status());
          break;
        case Event::RECONCILE_OPERATIONS:
          reconcileOperations(event.reconcile_operations());
          break;
        case Event::UNKNOWN:
          LOG(FATAL) << "Received unexpected UNKNOWN event";
          break;
      }
    }
  }

  process::Future<Nothing> send(const Call& call)
  {
    return driver->send(call);
  }

  template <typename Credential>
  void start(
      process::Owned<mesos::internal::EndpointDetector> detector,
      ContentType contentType,
      const Credential& credential)
  {
    driver.reset(new Driver(
        std::move(detector),
        contentType,
        lambda::bind(
            &MockResourceProvider<
                Event,
                Call,
                Driver,
                ResourceProviderInfo,
                Resource,
                Resources,
                ResourceProviderID,
                OperationState,
                Operation,
                Source>::connected,
            this),
        lambda::bind(
            &MockResourceProvider<
                Event,
                Call,
                Driver,
                ResourceProviderInfo,
                Resource,
                Resources,
                ResourceProviderID,
                OperationState,
                Operation,
                Source>::disconnected,
            this),
        lambda::bind(
            &MockResourceProvider<
                Event,
                Call,
                Driver,
                ResourceProviderInfo,
                Resource,
                Resources,
                ResourceProviderID,
                OperationState,
                Operation,
                Source>::events,
            this,
            lambda::_1),
        credential));

    driver->start();
  }

  void connectedDefault()
  {
    Call call;
    call.set_type(Call::SUBSCRIBE);
    call.mutable_subscribe()->mutable_resource_provider_info()->CopyFrom(info);

    driver->send(call);
  }

  void subscribedDefault(const typename Event::Subscribed& subscribed)
  {
    info.mutable_id()->CopyFrom(subscribed.provider_id());

    if (resources.isSome()) {
      Resources injected;

      foreach (Resource resource, resources.get()) {
        resource.mutable_provider_id()->CopyFrom(info.id());
        injected += resource;
      }

      Call call;
      call.set_type(Call::UPDATE_STATE);
      call.mutable_resource_provider_id()->CopyFrom(info.id());

      typename Call::UpdateState* update = call.mutable_update_state();
      update->mutable_resources()->CopyFrom(injected);
      update->mutable_resource_version_uuid()->set_value(
          id::UUID::random().toBytes());

      driver->send(call);
    }
  }

  void operationDefault(const typename Event::ApplyOperation& operation)
  {
    CHECK(info.has_id());

    Call call;
    call.set_type(Call::UPDATE_OPERATION_STATUS);
    call.mutable_resource_provider_id()->CopyFrom(info.id());

    typename Call::UpdateOperationStatus* update =
      call.mutable_update_operation_status();
    update->mutable_framework_id()->CopyFrom(operation.framework_id());
    update->mutable_operation_uuid()->CopyFrom(operation.operation_uuid());

    update->mutable_status()->set_state(
        OperationState::OPERATION_FINISHED);

    switch (operation.info().type()) {
      case Operation::LAUNCH:
      case Operation::LAUNCH_GROUP:
        break;
      case Operation::RESERVE:
        break;
      case Operation::UNRESERVE:
        break;
      case Operation::CREATE:
        break;
      case Operation::DESTROY:
        break;
      case Operation::CREATE_VOLUME:
        update->mutable_status()->add_converted_resources()->CopyFrom(
            operation.info().create_volume().source());
        update->mutable_status()
          ->mutable_converted_resources()
          ->Mutable(0)
          ->mutable_disk()
          ->mutable_source()
          ->set_type(operation.info().create_volume().target_type());
        break;
      case Operation::DESTROY_VOLUME:
        update->mutable_status()->add_converted_resources()->CopyFrom(
            operation.info().destroy_volume().volume());
        update->mutable_status()
          ->mutable_converted_resources()
          ->Mutable(0)
          ->mutable_disk()
          ->mutable_source()
          ->set_type(Source::RAW);
        break;
      case Operation::CREATE_BLOCK:
        update->mutable_status()->add_converted_resources()->CopyFrom(
            operation.info().create_block().source());
        update->mutable_status()
          ->mutable_converted_resources()
          ->Mutable(0)
          ->mutable_disk()
          ->mutable_source()
          ->set_type(Source::BLOCK);
        break;
      case Operation::DESTROY_BLOCK:
        update->mutable_status()->add_converted_resources()->CopyFrom(
            operation.info().destroy_block().block());
        update->mutable_status()
          ->mutable_converted_resources()
          ->Mutable(0)
          ->mutable_disk()
          ->mutable_source()
          ->set_type(Source::RAW);
        break;
      case Operation::UNKNOWN:
        break;
    }

    update->mutable_latest_status()->CopyFrom(update->status());

    driver->send(call);
  }

  void publishDefault(const typename Event::PublishResources& publish)
  {
    CHECK(info.has_id());

    Call call;
    call.set_type(Call::UPDATE_PUBLISH_RESOURCES_STATUS);
    call.mutable_resource_provider_id()->CopyFrom(info.id());

    typename Call::UpdatePublishResourcesStatus* update =
      call.mutable_update_publish_resources_status();
    update->mutable_uuid()->CopyFrom(publish.uuid());
    update->set_status(Call::UpdatePublishResourcesStatus::OK);

    driver->send(call);
  }

  ResourceProviderInfo info;

private:
  Option<Resources> resources;
  std::unique_ptr<Driver> driver;
};

inline process::Owned<EndpointDetector> createEndpointDetector(
    const process::UPID& pid)
{
  // Start and register a resource provider.
  std::string scheme = "http";

#ifdef USE_SSL_SOCKET
  if (process::network::openssl::flags().enabled) {
    scheme = "https";
  }
#endif

  process::http::URL url(
      scheme,
      pid.address.ip,
      pid.address.port,
      pid.id + "/api/v1/resource_provider");

  return process::Owned<EndpointDetector>(new ConstantEndpointDetector(url));
}

} // namespace resource_provider {


namespace v1 {
namespace resource_provider {

// Alias existing `mesos::v1::resource_provider` classes so that we can easily
// write `v1::resource_provider::` in tests.
using Call = mesos::v1::resource_provider::Call;
using Event = mesos::v1::resource_provider::Event;

} // namespace resource_provider {

using MockResourceProvider = tests::resource_provider::MockResourceProvider<
    mesos::v1::resource_provider::Event,
    mesos::v1::resource_provider::Call,
    mesos::v1::resource_provider::Driver,
    mesos::v1::ResourceProviderInfo,
    mesos::v1::Resource,
    mesos::v1::Resources,
    mesos::v1::ResourceProviderID,
    mesos::v1::OperationState,
    mesos::v1::Offer::Operation,
    mesos::v1::Resource::DiskInfo::Source>;

} // namespace v1 {


// Definition of a MockAuthorizer that can be used in tests with gmock.
class MockAuthorizer : public Authorizer
{
public:
  MockAuthorizer();
  virtual ~MockAuthorizer();

  MOCK_METHOD1(
      authorized, process::Future<bool>(const authorization::Request& request));

  MOCK_METHOD2(
      getObjectApprover, process::Future<process::Owned<ObjectApprover>>(
          const Option<authorization::Subject>& subject,
          const authorization::Action& action));
};


class MockSecretGenerator : public SecretGenerator
{
public:
  MockSecretGenerator() = default;
  virtual ~MockSecretGenerator() = default;

  MOCK_METHOD1(generate, process::Future<Secret>(
      const process::http::authentication::Principal& principal));
};


ACTION_P(SendStatusUpdateFromTask, state)
{
  TaskStatus status;
  status.mutable_task_id()->MergeFrom(arg1.task_id());
  status.set_state(state);
  arg0->sendStatusUpdate(status);
}


ACTION_P(SendStatusUpdateFromTaskID, state)
{
  TaskStatus status;
  status.mutable_task_id()->MergeFrom(arg1);
  status.set_state(state);
  arg0->sendStatusUpdate(status);
}


ACTION_P(SendFrameworkMessage, data)
{
  arg0->sendFrameworkMessage(data);
}


#define FUTURE_PROTOBUF(message, from, to)              \
  FutureProtobuf(message, from, to)


#define DROP_PROTOBUF(message, from, to)              \
  FutureProtobuf(message, from, to, true)


#define DROP_PROTOBUFS(message, from, to)              \
  DropProtobufs(message, from, to)


#define EXPECT_NO_FUTURE_PROTOBUFS(message, from, to)              \
  ExpectNoFutureProtobufs(message, from, to)


#define FUTURE_HTTP_PROTOBUF(message, path, contentType)   \
  FutureHttp(message, path, contentType)


#define DROP_HTTP_PROTOBUF(message, path, contentType)     \
  FutureHttp(message, path, contentType, true)


#define DROP_HTTP_PROTOBUFS(message, path, contentType)    \
  DropHttpProtobufs(message, path, contentType)


#define EXPECT_NO_FUTURE_HTTP_PROTOBUFS(message, path, contentType)  \
  ExpectNoFutureHttpProtobufs(message, path, contentType)


// These are specialized versions of {FUTURE,DROP}_PROTOBUF that
// capture a scheduler/executor Call protobuf of the given 'type'.
// Note that we name methods as '*ProtobufUnion()' because these could
// be reused for macros that capture any protobufs that are described
// using the standard protocol buffer "union" trick (e.g.,
// FUTURE_EVENT to capture scheduler::Event), see
// https://developers.google.com/protocol-buffers/docs/techniques#union.

#define FUTURE_CALL(message, unionType, from, to)              \
  FutureUnionProtobuf(message, unionType, from, to)


#define DROP_CALL(message, unionType, from, to)                \
  FutureUnionProtobuf(message, unionType, from, to, true)


#define DROP_CALLS(message, unionType, from, to)               \
  DropUnionProtobufs(message, unionType, from, to)


#define EXPECT_NO_FUTURE_CALLS(message, unionType, from, to)   \
  ExpectNoFutureUnionProtobufs(message, unionType, from, to)


#define FUTURE_CALL_MESSAGE(message, unionType, from, to)          \
  process::FutureUnionMessage(message, unionType, from, to)


#define DROP_CALL_MESSAGE(message, unionType, from, to)            \
  process::FutureUnionMessage(message, unionType, from, to, true)


#define FUTURE_HTTP_CALL(message, unionType, path, contentType)  \
  FutureUnionHttp(message, unionType, path, contentType)


#define DROP_HTTP_CALL(message, unionType, path, contentType)    \
  FutureUnionHttp(message, unionType, path, contentType, true)


#define DROP_HTTP_CALLS(message, unionType, path, contentType)   \
  DropUnionHttpProtobufs(message, unionType, path, contentType)


#define EXPECT_NO_FUTURE_HTTP_CALLS(message, unionType, path, contentType)   \
  ExpectNoFutureUnionHttpProtobufs(message, unionType, path, contentType)


// Forward declaration.
template <typename T>
T _FutureProtobuf(const process::Message& message);


template <typename T, typename From, typename To>
process::Future<T> FutureProtobuf(T t, From from, To to, bool drop = false)
{
  // Help debugging by adding some "type constraints".
  { google::protobuf::Message* m = &t; (void) m; }

  return process::FutureMessage(testing::Eq(t.GetTypeName()), from, to, drop)
    .then(lambda::bind(&_FutureProtobuf<T>, lambda::_1));
}


template <typename Message, typename UnionType, typename From, typename To>
process::Future<Message> FutureUnionProtobuf(
    Message message, UnionType unionType, From from, To to, bool drop = false)
{
  // Help debugging by adding some "type constraints".
  { google::protobuf::Message* m = &message; (void) m; }

  return process::FutureUnionMessage(message, unionType, from, to, drop)
    .then(lambda::bind(&_FutureProtobuf<Message>, lambda::_1));
}


template <typename Message, typename Path>
process::Future<Message> FutureHttp(
    Message message,
    Path path,
    ContentType contentType,
    bool drop = false)
{
  // Help debugging by adding some "type constraints".
  { google::protobuf::Message* m = &message; (void) m; }

  auto deserializer =
    lambda::bind(&deserialize<Message>, contentType, lambda::_1);

  return process::FutureHttpRequest(message, path, deserializer, drop)
    .then([deserializer](const process::http::Request& request) {
      return deserializer(request.body).get();
    });
}


template <typename Message, typename UnionType, typename Path>
process::Future<Message> FutureUnionHttp(
    Message message,
    UnionType unionType,
    Path path,
    ContentType contentType,
    bool drop = false)
{
  // Help debugging by adding some "type constraints".
  { google::protobuf::Message* m = &message; (void) m; }

  auto deserializer =
    lambda::bind(&deserialize<Message>, contentType, lambda::_1);

  return process::FutureUnionHttpRequest(
      message, unionType, path, deserializer, drop)
    .then([deserializer](const process::http::Request& request) {
      return deserializer(request.body).get();
    });
}


template <typename T>
T _FutureProtobuf(const process::Message& message)
{
  T t;
  t.ParseFromString(message.body);
  return t;
}


template <typename T, typename From, typename To>
void DropProtobufs(T t, From from, To to)
{
  // Help debugging by adding some "type constraints".
  { google::protobuf::Message* m = &t; (void) m; }

  process::DropMessages(testing::Eq(t.GetTypeName()), from, to);
}


template <typename Message, typename UnionType, typename From, typename To>
void DropUnionProtobufs(Message message, UnionType unionType, From from, To to)
{
  // Help debugging by adding some "type constraints".
  { google::protobuf::Message* m = &message; (void) m; }

  process::DropUnionMessages(message, unionType, from, to);
}


template <typename Message, typename Path>
void DropHttpProtobufs(
    Message message,
    Path path,
    ContentType contentType,
    bool drop = false)
{
  // Help debugging by adding some "type constraints".
  { google::protobuf::Message* m = &message; (void) m; }

  auto deserializer =
    lambda::bind(&deserialize<Message>, contentType, lambda::_1);

  process::DropHttpRequests(message, path, deserializer);
}


template <typename Message, typename UnionType, typename Path>
void DropUnionHttpProtobufs(
    Message message,
    UnionType unionType,
    Path path,
    ContentType contentType,
    bool drop = false)
{
  // Help debugging by adding some "type constraints".
  { google::protobuf::Message* m = &message; (void) m; }

  auto deserializer =
    lambda::bind(&deserialize<Message>, contentType, lambda::_1);

  process::DropUnionHttpRequests(message, unionType, path, deserializer);
}


template <typename T, typename From, typename To>
void ExpectNoFutureProtobufs(T t, From from, To to)
{
  // Help debugging by adding some "type constraints".
  { google::protobuf::Message* m = &t; (void) m; }

  process::ExpectNoFutureMessages(testing::Eq(t.GetTypeName()), from, to);
}


template <typename Message, typename UnionType, typename From, typename To>
void ExpectNoFutureUnionProtobufs(
    Message message, UnionType unionType, From from, To to)
{
  // Help debugging by adding some "type constraints".
  { google::protobuf::Message* m = &message; (void) m; }

  process::ExpectNoFutureUnionMessages(message, unionType, from, to);
}


template <typename Message, typename Path>
void ExpectNoFutureHttpProtobufs(
    Message message,
    Path path,
    ContentType contentType,
    bool drop = false)
{
  // Help debugging by adding some "type constraints".
  { google::protobuf::Message* m = &message; (void) m; }

  auto deserializer =
    lambda::bind(&deserialize<Message>, contentType, lambda::_1);

  process::ExpectNoFutureHttpRequests(message, path, deserializer);
}


template <typename Message, typename UnionType, typename Path>
void ExpectNoFutureUnionHttpProtobufs(
    Message message,
    UnionType unionType,
    Path path,
    ContentType contentType,
    bool drop = false)
{
  // Help debugging by adding some "type constraints".
  { google::protobuf::Message* m = &message; (void) m; }

  auto deserializer =
    lambda::bind(&deserialize<Message>, contentType, lambda::_1);

  process::ExpectNoFutureUnionHttpRequests(
      message, unionType, path, deserializer);
}


// This matcher is used to match a vector of resource offers that
// contains an offer having any resource that passes the filter.
MATCHER_P(OffersHaveAnyResource, filter, "")
{
  foreach (const Offer& offer, arg) {
    foreach (const Resource& resource, offer.resources()) {
      if (filter(resource)) {
        return true;
      }
    }
  }

  return false;
}


// This matcher is used to match a vector of resource offers that
// contains an offer having the specified resource.
MATCHER_P(OffersHaveResource, resource, "")
{
  foreach (const Offer& offer, arg) {
    Resources resources = offer.resources();

    // If `resource` is not allocated, we are matching offers against
    // resources constructed from scratch, so we strip off allocations.
    if (!resource.has_allocation_info()) {
      resources.unallocate();
    }

    if (resources.contains(resource)) {
      return true;
    }
  }

  return false;
}


// This matcher is used to match the task id of a `TaskStatus` message.
MATCHER_P(TaskStatusTaskIdEq, taskInfo, "")
{
  return arg.task_id() == taskInfo.task_id();
}


// This matcher is used to match the state of a `TaskStatus` message.
MATCHER_P(TaskStatusStateEq, taskState, "")
{
  return arg.state() == taskState;
}


// This matcher is used to match the task id of an `Event.update.status`
// message.
MATCHER_P(TaskStatusUpdateTaskIdEq, taskInfo, "")
{
  return arg.status().task_id() == taskInfo.task_id();
}


// This matcher is used to match the state of an `Event.update.status`
// message.
MATCHER_P(TaskStatusUpdateStateEq, taskState, "")
{
  return arg.status().state() == taskState;
}


struct ParamExecutorType
{
public:
  struct Printer
  {
    std::string operator()(
        const ::testing::TestParamInfo<ParamExecutorType>& info) const
    {
      switch (info.param.type) {
        case COMMAND:
          return "CommandExecutor";
        case DEFAULT:
          return "DefaultExecutor";
        default:
          UNREACHABLE();
      }
    }
  };

  static ParamExecutorType commandExecutor()
  {
    return ParamExecutorType(COMMAND);
  }

  static ParamExecutorType defaultExecutor()
  {
    return ParamExecutorType(DEFAULT);
  }

  bool isCommandExecutor() const { return type == COMMAND; }
  bool isDefaultExecutor() const { return type == DEFAULT; }

private:
  enum Type
  {
    COMMAND,
    DEFAULT
  };

  ParamExecutorType(Type _type) : type(_type) {}

  Type type;
};

} // namespace tests {
} // namespace internal {
} // namespace mesos {

#endif // __TESTS_MESOS_HPP__

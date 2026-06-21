//  ____  ____   ___   ___ ___    ___ ______  __ __    ___  __ __  _____
// |    \|    \ /   \ |   |   |  /  _]      ||  |  |  /  _]|  |  |/ ___/
// |  o  )  D  )     || _   _ | /  [_|      ||  |  | /  [_ |  |  (   \_
// |   _/|    /|  O  ||  \_/  ||    _]_|  |_||  _  ||    _]|  |  |\__  |
// |  |  |    \|     ||   |   ||   [_  |  |  |  |  ||   [_ |  :  |/  \ |
// |  |  |  .  \     ||   |   ||     | |  |  |  |  ||     ||     |\    |
// |__|  |__|\_|\___/ |___|___||_____| |__|  |__|__||_____| \__,_| \___|
//     __  _      ____    ___  ____   ______
//    /  ]| |    |    |  /  _]|    \ |      |
//   /  / | |     |  |  /  [_ |  _  ||      |
//  /  /  | |___  |  | |    _]|  |  ||_|  |_|
// /   \_ |     | |  | |   [_ |  |  |  |  |
// \     ||     | |  | |     ||  |  |  |  |
//  \____||_____||____||_____||__|__|  |__|
//     __  __ __  __ __
//    /  ]|  |  ||  |  |
//   /  / |  |  ||  |  |
//  /  /  |_   _||_   _|
// /   \_ |     ||     |
// \     ||  |  ||  |  |
//  \____||__|__||__|__|
//
//  Prometheus Client C++
//  version 0.0.0
//  https://github.com/eduardbme/prometheus_client_cxx

#ifndef PROMETHEUS_CLIENT_CXX_HPP_
#define PROMETHEUS_CLIENT_CXX_HPP_

#include <algorithm>
#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <shared_mutex>
#include <sstream>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

namespace prometheus {

constexpr std::string_view TEXT_FORMAT_V004 =
    "text/plain; version=0.0.4; charset=utf-8";
constexpr std::string_view OPENMETRICS_V100 =
    "application/openmetrics-text; version=1.0.0; charset=utf-8";

class registry;
template <typename T> class gauge;
template <typename T> class counter;

namespace internal {

class label;
class labels_list;
class metric_family;
class base_metric;
template <typename T> class metric;
class metric_family_out_data;

using registry_labels = labels_list;
using registry_prefix = std::string;
using metric_name = std::string;
using metric_help = std::string;

std::ostream &operator<<(std::ostream &os,
                         const metric_family_out_data &family_data);

// Immutable
class label {
public:
  label(const std::string &key, const std::string &value)
      : _key(key), _value(value) {}

  const std::string &key() const { return this->_key; }

  bool operator==(const internal::label &rhs) const {
    return std::tie(this->_key, this->_value) == std::tie(rhs._key, rhs._value);
  }

  bool operator!=(const internal::label &rhs) const { return !(*this == rhs); }

  bool operator<(const internal::label &rhs) const {
    return std::tie(this->_key, this->_value) < std::tie(rhs._key, rhs._value);
  }

  friend std::ostream &operator<<(std::ostream &os, const internal::label &l) {
    return os << "\"" << l._key << "\"=\"" << l._value + "\"";
  }

private:
  std::string _key;
  std::string _value;
};

// Immutable
class labels_list {
private:
  struct labels_list_comparator {
    bool operator()(const label &lhs, const label &rhs) const {
      return lhs.key() < rhs.key();
    }
  };

public:
  labels_list() = default;
  labels_list(const std::initializer_list<internal::label> &labels) {
    this->_labels = labels;
  }

  bool operator==(const internal::labels_list &other) const {
    return (this->_labels == other._labels);
  }

  bool operator!=(const internal::labels_list &other) const {
    return !(*this == other);
  }

  friend internal::labels_list operator+(const internal::labels_list &lhs,
                                         const internal::labels_list &rhs) {
    // Use 'rhs' first, std::set does not override values,
    // rhs + lhs != lhs + rhs.
    labels_list new_labels_list = rhs;
    new_labels_list._labels.insert(lhs._labels.begin(), lhs._labels.end());

    return new_labels_list;
  }

  friend std::ostream &operator<<(std::ostream &os,
                                  const internal::labels_list &r) {
    if (r._labels.empty()) {
      return os << "";
    }

    os << '{';

    auto it = r._labels.begin();
    os << *it;
    for (++it; it != r._labels.end(); ++it) {
      os << ',' << *it;
    }

    return os << '}';
  }

  friend bool operator<(const internal::labels_list &lhs,
                        const internal::labels_list &rhs) {
    return lhs._labels < rhs._labels;
  }

private:
  std::set<label, labels_list_comparator> _labels;
};

class metric_key {
public:
  metric_key(const metric_name &name, const internal::labels_list &labels)
      : _name(name), _labels(labels) {}

  bool operator<(const metric_key &rhs) const {
    return std::tie(_name, _labels) < std::tie(rhs._name, rhs._labels);
  }

private:
  std::string _name;
  internal::labels_list _labels;
};

class metric_family {
private:
  using metrics_map =
      std::map<internal::metric_key, std::shared_ptr<internal::base_metric>>;

public:
  metric_family(const metric_name &name, const metric_help &help)
      : _name(name), _help(help) {}

  template <typename T>
  std::shared_ptr<internal::base_metric>
  add(const internal::labels_list &labels);
  void remove(const internal::labels_list &labels);

  virtual std::string type() const = 0;

  friend std::ostream &
  internal::operator<<(std::ostream &os,
                       const internal::metric_family_out_data &family_data);

private:
  const metric_name _name;
  const metric_help _help;
  metrics_map _metrics;
  // For const ref
  mutable std::shared_mutex _mutex;
};

class gauge_metric_family : public metric_family {
public:
  gauge_metric_family(const metric_name &name, const metric_help &help)
      : metric_family(name, help) {}

  std::string type() const override { return "gauge"; };
};

class counter_metric_family : public metric_family {
public:
  counter_metric_family(const metric_name &name, const metric_help &help)
      : metric_family(name, help) {}

  std::string type() const override { return "counter"; };
};

// Type Erasure.
// To be able to store any internal::metric<T> within the container.
class base_metric {
public:
  virtual ~base_metric() = default;
  virtual std::string
  to_string(const internal::registry_prefix &prefix,
            const internal::registry_labels &labels) const = 0;
};

template <typename T> class metric : public base_metric {
  static_assert(std::is_arithmetic<T>::value,
                "metric<T> supports numerical types only!");

public:
  metric(const std::string &name, const internal::labels_list &labels_list)
      : _name(name), _labels_list(labels_list), _value(0) {}

  std::string to_string(const internal::registry_prefix &prefix,
                        const internal::registry_labels &labels) const override;

protected:
  std::atomic<T> _value;

private:
  const std::string _name;
  const internal::labels_list _labels_list;
};

struct metric_family_out_data {
  const internal::registry_prefix &registry_prefix;
  const internal::registry_labels &registry_labels;
  const internal::metric_family &family;
};

inline void metric_family::remove(const internal::labels_list &labels) {
  std::unique_lock<std::shared_mutex> lock(this->_mutex);

  this->_metrics.erase({this->_name, labels});
}

template <typename T>
inline std::shared_ptr<internal::base_metric>
metric_family::add(const internal::labels_list &labels) {
  std::unique_lock<std::shared_mutex> lock(this->_mutex);

  auto [it, inserted] =
      this->_metrics.try_emplace({this->_name, labels}, nullptr);
  if (inserted) {
    it->second = std::make_shared<T>(this->_name, labels);
  }

  return it->second;
}

} // namespace internal

template <typename T = int> class gauge : public internal::metric<T> {
public:
  gauge(const std::string &name, const internal::labels_list &labels_list)
      : internal::metric<T>(name, labels_list) {}

  template <typename U = T> void set(U value) {
    static_assert(std::is_same_v<U, T>, "Invalid type");
    this->_value = value;
  }
};

template <typename T = int> class counter : public internal::metric<T> {
public:
  counter(const std::string &name, const internal::labels_list &labels_list)
      : internal::metric<T>(name, labels_list) {}

  template <typename U = T> void inc(U value = 1) {
    static_assert(std::is_same_v<U, T>, "Invalid type");
    this->_value += value;
  }
};

class registry {
private:
  class __restricted;

  using metrics_map =
      std::map<internal::labels_list, std::set<internal::metric_name>>;
  using families_map =
      std::map<internal::metric_name, std::shared_ptr<internal::metric_family>>;

public:
  registry(__restricted) {}

  static std::shared_ptr<registry> create() {
    return std::make_shared<registry>(__restricted{});
  }

  void label_set(const internal::label &label) {
    std::unique_lock<std::shared_mutex> lock(this->_mutex);

    this->_labels = this->_labels + internal::labels_list({label});
  }

  void prefix_set(const internal::registry_prefix &prefix) {
    this->_prefix = prefix;
  }

  template <typename T = int>
  std::shared_ptr<prometheus::gauge<T>>
  gauge(const internal::metric_name &name, const internal::metric_help &help,
        const internal::labels_list &labels_list = {}) {
    auto gauge_family = this->gauge_family(name, help, labels_list);
    if (!gauge_family) {
      // Prevent potential SEGFAULT by returning nullptr for a mismatch family
      // type. Return untraceable throw-away object.
      return std::make_shared<prometheus::gauge<T>>(name, labels_list);
    }

    auto gauge = gauge_family->add<prometheus::gauge<T>>(labels_list);
    return std::dynamic_pointer_cast<prometheus::gauge<T>>(gauge);
  }

  template <typename T = int>
  std::shared_ptr<prometheus::counter<T>>
  counter(const internal::metric_name &name, const internal::metric_help &help,
          const internal::labels_list &labels_list = {}) {
    auto counter_family = this->counter_family(name, help, labels_list);
    if (!counter_family) {
      // Prevent potential SEGFAULT by returning nullptr for a mismatch family
      // type. Return untraceable throw-away object.
      return std::make_shared<prometheus::counter<T>>(name, labels_list);
    }

    auto counter = counter_family->add<prometheus::counter<T>>(labels_list);
    return std::dynamic_pointer_cast<prometheus::counter<T>>(counter);
  }

  void remove(const internal::metric_name &name,
              const internal::labels_list &labels_list = {}) {
    std::shared_lock<std::shared_mutex> lock(this->_mutex);

    auto it = this->_families.find(name);
    if (it == this->_families.end()) {
      return;
    }

    it->second->remove(labels_list);
  }

  void remove(const internal::labels_list &labels_list) {
    metrics_map metrics;

    {
      std::shared_lock<std::shared_mutex> lock(this->_mutex);
      metrics = this->_metrics;
    }

    auto it = metrics.find(labels_list);
    if (it == metrics.end()) {
      return;
    }

    for (const auto &metric_name : it->second) {
      this->remove(metric_name, it->first);
    }

    std::unique_lock<std::shared_mutex> lock(this->_mutex);
    this->_metrics.erase(labels_list);
  }

  std::string to_string() const {
    std::stringstream ss;
    ss << *this;

    return ss.str();
  }

  friend std::ostream &operator<<(std::ostream &os, const registry &r) {
    families_map families;
    internal::registry_prefix prefix;
    internal::registry_labels labels;

    {
      std::shared_lock<std::shared_mutex> lock(r._mutex);
      families = r._families;
      prefix = r._prefix;
      labels = r._labels;
    }

    std::for_each(families.begin(), families.end(), [&](const auto &f) -> void {
      os << internal::metric_family_out_data{std::ref(prefix), std::ref(labels),

                                             std::ref(*f.second)};
    });

    return os;
  }

private:
  std::shared_ptr<internal::gauge_metric_family>
  gauge_family(const internal::metric_name &name,
               const internal::metric_help &help,
               const internal::labels_list &labels_list = {}) {
    std::unique_lock<std::shared_mutex> lock(this->_mutex);

    auto [family, family_inserted] = this->_families.try_emplace(name, nullptr);
    if (family_inserted) {
      family->second =
          std::make_shared<internal::gauge_metric_family>(name, help);
    }

    auto f = std::dynamic_pointer_cast<internal::gauge_metric_family>(
        family->second);
    if (!f) {
      return f;
    }

    auto [metric, _1] = this->_metrics.try_emplace(labels_list);
    metric->second.insert(name);

    return f;
  }

  std::shared_ptr<internal::counter_metric_family>
  counter_family(const internal::metric_name &name,
                 const internal::metric_help &help,
                 const internal::labels_list &labels_list = {}) {
    std::unique_lock<std::shared_mutex> lock(this->_mutex);

    auto [family, family_inserted] = this->_families.try_emplace(name, nullptr);
    if (family_inserted) {
      family->second =
          std::make_shared<internal::counter_metric_family>(name, help);
    }

    auto f = std::dynamic_pointer_cast<internal::counter_metric_family>(
        family->second);
    if (!f) {
      return f;
    }

    auto [metric, _1] = this->_metrics.try_emplace(labels_list);
    metric->second.insert(name);

    return f;
  }

private:
  metrics_map _metrics;
  families_map _families;
  internal::registry_prefix _prefix;
  internal::labels_list _labels;
  mutable std::shared_mutex _mutex;

  template <typename T> friend class internal::metric;

  struct __restricted {
    explicit __restricted() = default;
  };
};

inline std::ostream &
internal::operator<<(std::ostream &os,
                     const internal::metric_family_out_data &family_data) {
  const auto &[registry_prefix, registry_labels, family] = family_data;
  internal::metric_family::metrics_map metrics;

  {
    std::shared_lock<std::shared_mutex> lock(family._mutex);
    metrics = family._metrics;
  }

  if (!metrics.size()) {
    return os;
  }

  os << "# HELP " << family._name << " " << family._help << std::endl;
  os << "# TYPE " << family._name << " " << family.type() << std::endl;

  std::for_each(metrics.begin(), metrics.end(), [&](const auto &f) -> void {
    os << f.second->to_string(registry_prefix, registry_labels) << std::endl;
  });

  os << std::endl;

  return os;
}

template <typename T>
inline std::string
internal::metric<T>::to_string(const internal::registry_prefix &prefix,
                               const internal::registry_labels &labels) const {
  std::stringstream ss;

  auto name = prefix.size() ? prefix + "_" + this->_name : this->_name;

  ss << name << labels + this->_labels_list << " "
     << std::to_string(this->_value);

  return ss.str();
}

} // namespace prometheus

#endif

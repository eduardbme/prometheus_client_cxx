#ifndef PROMETHEUS_HPP_
#define PROMETHEUS_HPP_

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

namespace internal {

using metric_name = std::string;
using metric_help = std::string;

class label;
class labels_list;
class metric_family;
class base_metric;
template <typename T> class metric;
template <typename T> class gauge;
template <typename T> class counter;

std::ostream &operator<<(std::ostream &os, const metric_family &family);

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
  metric_key(const metric_name &name, const internal::labels_list &labels_list)
      : _name(name), _labels_list(labels_list) {}

  bool operator<(const metric_key &rhs) const {
    return std::tie(_name, _labels_list) <
           std::tie(rhs._name, rhs._labels_list);
  }

private:
  std::string _name;
  internal::labels_list _labels_list;
};

class metric_family {
public:
  metric_family(const registry *registry, const metric_name &name,
                const metric_help &help)
      : _registry(registry), _name(name), _help(help) {}

  template <typename T>
  std::shared_ptr<internal::base_metric>
  add(const internal::labels_list &labels);
  void remove(const internal::labels_list &labels);

  virtual std::string type() const = 0;

  friend std::ostream &internal::operator<<(std::ostream &os,
                                            const internal::metric_family &m);

private:
  const registry *_registry;
  const metric_name _name;
  const metric_help _help;
  std::map<internal::metric_key, std::shared_ptr<internal::base_metric>>
      _metrics;
  // For const ref
  mutable std::shared_mutex _mutex;
};

class gauge_metric_family : public metric_family {
public:
  gauge_metric_family(registry *registry, const metric_name &name,
                      const metric_help &help)
      : metric_family(registry, name, help) {}

  std::string type() const override { return "gauge"; };
};

class counter_metric_family : public metric_family {
public:
  counter_metric_family(registry *registry, const metric_name &name,
                        const metric_help &help)
      : metric_family(registry, name, help) {}

  std::string type() const override { return "counter"; };
};

// Type Erasure.
// To be able to store any internal::metric<T> within the container.
class base_metric {
public:
  virtual ~base_metric() = default;
  virtual std::string to_string() const = 0;
};

template <typename T> class metric : public base_metric {
  static_assert(std::is_arithmetic<T>::value,
                "metric<T> supports numerical types only!");

public:
  metric(const registry *registry, const std::string &name,
         const internal::labels_list &labels_list)
      : _registry(registry), _name(name), _labels_list(labels_list), _value(0) {
  }

  std::string to_string() const override;

protected:
  std::atomic<T> _value;

private:
  const registry *_registry;
  const std::string _name;
  const internal::labels_list _labels_list;
};

template <typename T> class gauge : public metric<T> {
public:
  gauge(const registry *registry, const std::string &name,
        const internal::labels_list &labels_list)
      : metric<T>(registry, name, labels_list) {}

  void set(T value) { this->_value = value; }
};

template <typename T> class counter : public metric<T> {
public:
  counter(const registry *registry, const std::string &name,
          const internal::labels_list &labels_list)
      : metric<T>(registry, name, labels_list) {}

  void inc(T value = 1) { this->_value += value; }
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
    it->second = std::make_shared<T>(this->_registry, this->_name, labels);
  }

  return it->second;
}

} // namespace internal

class registry {
  class __restricted;

public:
  registry(__restricted) {}

  static std::shared_ptr<registry> create() {
    return std::make_shared<registry>(__restricted{});
  }

  void label_set(const internal::label &label) {
    std::unique_lock<std::shared_mutex> lock(this->_mutex);

    this->_registry_labels =
        this->_registry_labels + internal::labels_list({label});
  }

  template <typename T = int>
  std::shared_ptr<internal::gauge<T>>
  gauge(const internal::metric_name &name, const internal::metric_help &help,
        const internal::labels_list &labels_list = {}) {
    auto gauge_family = this->gauge_family(name, help, labels_list);
    if (!gauge_family) {
      // Prevent potential SEGFAULT by returning nullptr for a mismatch family
      // type. Return untraceable throw-away object.
      return std::make_shared<internal::gauge<T>>(this, name, labels_list);
    }

    auto gauge = gauge_family->add<internal::gauge<T>>(labels_list);
    return std::dynamic_pointer_cast<internal::gauge<T>>(gauge);
  }

  template <typename T = int>
  std::shared_ptr<internal::counter<T>>
  counter(const internal::metric_name &name, const internal::metric_help &help,
          const internal::labels_list &labels_list = {}) {
    auto counter_family = this->counter_family(name, help, labels_list);
    if (!counter_family) {
      // Prevent potential SEGFAULT by returning nullptr for a mismatch family
      // type. Return untraceable throw-away object.
      return std::make_shared<internal::counter<T>>(this, name, labels_list);
    }

    auto counter = counter_family->add<internal::counter<T>>(labels_list);
    return std::dynamic_pointer_cast<internal::counter<T>>(counter);
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
    // TODO: typedef
    std::multimap<internal::labels_list, internal::metric_name> metrics;
    {
      std::shared_lock<std::shared_mutex> lock(this->_mutex);
      metrics = this->_metrics;
    }

    auto range = metrics.equal_range(labels_list);
    for (auto it = range.first; it != range.second; ++it) {
      this->remove(it->second, it->first);
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
    // TODO: sep type
    std::map<internal::metric_name, std::shared_ptr<internal::metric_family>>
        families;
    {
      std::shared_lock<std::shared_mutex> lock(r._mutex);
      families = r._families;
    }

    std::for_each(families.begin(), families.end(),
                  [&](const auto &f) -> void { os << *f.second; });

    return os;
  }

private:
  std::shared_ptr<internal::gauge_metric_family>
  gauge_family(const internal::metric_name &name,
               const internal::metric_help &help,
               const internal::labels_list &labels_list = {}) {
    std::unique_lock<std::shared_mutex> lock(this->_mutex);

    auto [family, inserted] = this->_families.try_emplace(name, nullptr);
    if (inserted) {
      family->second =
          std::make_shared<internal::gauge_metric_family>(this, name, help);
    }

    auto f = std::dynamic_pointer_cast<internal::gauge_metric_family>(
        family->second);
    if (!f) {
      return f;
    }

    this->_metrics.insert({labels_list, name});

    return f;
  }

  std::shared_ptr<internal::counter_metric_family>
  counter_family(const internal::metric_name &name,
                 const internal::metric_help &help,
                 const internal::labels_list &labels_list = {}) {
    std::unique_lock<std::shared_mutex> lock(this->_mutex);

    auto [family, inserted] = this->_families.try_emplace(name, nullptr);
    if (inserted) {
      family->second =
          std::make_shared<internal::counter_metric_family>(this, name, help);
    }

    auto f = std::dynamic_pointer_cast<internal::counter_metric_family>(
        family->second);
    if (!f) {
      return f;
    }

    this->_metrics.insert({labels_list, name});

    return f;
  }

private:
  std::multimap<internal::labels_list, internal::metric_name> _metrics;
  std::map<internal::metric_name, std::shared_ptr<internal::metric_family>>
      _families;
  internal::labels_list _registry_labels;
  mutable std::shared_mutex _mutex;

  template <typename T> friend class internal::metric;

  struct __restricted {
    explicit __restricted() = default;
  };
};

inline std::ostream &
internal::operator<<(std::ostream &os, const internal::metric_family &family) {
  // TODO: sep type
  std::map<internal::metric_key, std::shared_ptr<internal::base_metric>>
      metrics;
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
    os << f.second->to_string() << std::endl;
  });

  os << std::endl;

  return os;
}

template <typename T>
inline std::string internal::metric<T>::to_string() const {
  internal::labels_list registry_labels;
  // TODO: registry_labels too oftern to lock,
  // should access it once on the registry level and pass to << std::pair()
  {
    std::shared_lock<std::shared_mutex> lock(this->_registry->_mutex);
    registry_labels = this->_registry->_registry_labels;
  }

  std::stringstream ss;

  ss << this->_name << registry_labels + this->_labels_list << " "
     << std::to_string(this->_value);

  return ss.str();
}

} // namespace prometheus

#endif

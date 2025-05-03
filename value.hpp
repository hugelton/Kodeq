#ifndef KODEQ_VALUE_HPP
#define KODEQ_VALUE_HPP

#include "module.hpp"
#include <string>

/**
 * Base Value class
 * The root class for all value types in KODEQ
 */
class BaseValue {
public:
  virtual ~BaseValue() {}
  virtual std::string getType() const = 0;
  virtual int toInt() const = 0;
};

/**
 * Integer Value class
 * Represents integer values in KODEQ
 */
class IntValue : public BaseValue {
private:
  int value;

public:
  IntValue(int v) : value(v) {}

  std::string getType() const override { return "INTEGER"; }
  int toInt() const override { return value; }
  int getValue() const { return value; }
};

/**
 * Module Value class
 * References a module instance in KODEQ
 */
class ModuleValue : public BaseValue {
private:
  Module *module;

public:
  ModuleValue(const std::string &type) {
    module = ModuleFactory::createModule(type);
  }

  ModuleValue(Module *mod) : module(mod) {}

  ~ModuleValue() {
    if (module)
      delete module;
  }

  std::string getType() const override { return "MODULE"; }
  int toInt() const override { return module ? module->getValue() : 0; }
  std::string getModuleName() const {
    return module ? module->getType() : "UNKNOWN";
  }

  // Module access
  Module *getModule() { return module; }

  // Parameter setting
  void setParameter(const std::string &name, int value) {
    if (module)
      module->setParameter(name, value);
  }

  std::string getVisualRepresentation() const {
    return module ? module->getVisualRepresentation() : "No module";
  }
};

#endif // KODEQ_VALUE_HPP
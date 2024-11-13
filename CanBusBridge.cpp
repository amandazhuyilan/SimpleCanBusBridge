//  ===========================================================================
//
//  Copyright (c) 2024-present, All Rights Reserved.
//
//  ===========================================================================

#include "CanBusBridge.h"
#include "HardwareCanBus.h"


// Constructor: Initializes the CanBusBridge without a parent or name.
CanBusBridge::CanBusBridge() : CanBusBridge(nullptr, "") {}

// Constructor: Initializes CanBusBridge with a parent component and name.
CanBusBridge::CanBusBridge(Component* parent, const std::string& name)
    : Component(parent, name), hardware_can_bus_(nullptr), app_(nullptr), index_(0) {
  setStatus(Status::Unknown, "Constructed");
}

// Destructor: Cleans up allocated resources.
CanBusBridge::~CanBusBridge() { delete hardware_can_bus_; }

// Converts CanBusBridge object to string for debugging.
std::string CanBusBridge::toString() const {
  std::string out = "[CanBusBridge]\n";
  out += fmt::format("- name: {}\n", getName());
  out += fmt::format("- index: {}\n", index_);
  out += Component::toString();
  return out;
}

// Loads configuration options and verifies dependencies.
void CanBusBridge::doLoad(const Options& options) {
  Component::doLoad(options);
  app_ = getServiceAs<App>();
  index_ = options.get<uint32_t>("index", 0);

  // Ensure the app is properly configured
  if (app_ != nullptr && app_->findChild("CanCommunication") == nullptr) {
    throw std::runtime_error(
        fmt::format("Invalid config file! No CanCommunication section children found in {}_config.json", app_->getName()));
  }

  setStatus(Status::Ok, "Loaded");
}

// Initializes the CanBusBridge by setting up communication channels.
void CanBusBridge::doInit() {
  Component::doInit();

  // Retrieve the hardware CAN bus configuration
  auto hardware_can_buses = getChildrenAs<HardwareCanBus>();
  if (hardware_can_buses.empty()) {
    throw std::runtime_error("No HardwareCanBus children found.");
  }
  if (hardware_can_buses.size() > 1) {
    log(LogLevel::Warn, "Multiple HardwareCanBus children are defined. The first one will be used.");
  }
  hardware_can_bus_ = hardware_can_buses.front();
  if (hardware_can_bus_ == nullptr) {
    throw std::runtime_error("Null hardware CAN bus");
  }

  // Retrieve the virtual CAN buses and configure the mapping
  std::vector<Library::Io::CanBus*> io_can_buses = app_->getChildrenRecursiveAs<Library::Io::CanBus>();
  if (io_can_buses.empty()) {
    throw std::runtime_error("No CAN buses found in the configuration file.");
  }

  // Set the name of the CAN interface and create a full name for matching
  std::string can_interface_name = getName();
  std::string can_bus_full_name = fmt::format("/{}/CanCommunication/{}", app_->getName(), can_interface_name);

  // Attempt to find a matching virtual CAN bus by name
  can_bus_ = dynamic_cast<CanBus*>(app_->findFullName(can_bus_full_name));
  if (can_bus_ == nullptr) {
    // If names don't match, search by index
    for (auto* io_can_bus : io_can_buses) {
      if (index_ == io_can_bus->getIndex()) {
        log(LogLevel::Info, fmt::format("CAN interface name mismatch. Using CAN bus index {}.", index_));
        can_bus_full_name = fmt::format("/{}/ComSpec/{}", app_->getName(), io_can_bus->getName());
        can_bus_ = dynamic_cast<CanBus*>(app_->findFullName(can_bus_full_name));
        if (can_bus_ == nullptr) {
          throw std::runtime_error(fmt::format("CAN bus not found in configuration file for index {}", index_));
        }
      }
    }
  }

  if (can_bus_ == nullptr) {
    throw std::runtime_error("CAN bus configuration missing or incorrect.");
  }

  // Register frames to be handled by the bridge
  can_bus_->registerAllFrames([&](uint32_t id, const std::vector<uint8_t>& input_frame) { setFrame(id, input_frame); });

  // Disable output scheduling for the virtual CAN bus
  can_bus_->disableOutputScheduling();
  hardware_can_bus_->is_can_fd_ = (can_bus_->getCanFdBaudRate() > 0);

  // Map CAN frame identifiers to their respective flags
  std::vector<Library::Io::InputCanFrame*> io_input_can_frames = app_->getChildrenRecursiveAs<Library::Io::InputCanFrame>();
  if (io_input_can_frames.empty()) {
    log(LogLevel::Warn, "No InputCanFrames found in the configuration.");
  } else {
    for (const auto* input_can_frame : io_input_can_frames) {
      can_id_fd_map_[input_can_frame->getIdentifier()] = input_can_frame->isCanFd();
    }
  }

  setStatus(Status::Ok, "Initialized");
}

// Periodically updates the CAN bus bridge by processing frames.
void CanBusBridge::doUpdate() {
  Component::doUpdate();

  bool power = app_ != nullptr && app_->isPowered();

  // Process incoming CAN frames from hardware to virtual network
  static CanFrame output_frame{0};
  while (power && !hardware_can_bus_->isHardwareCanFrameQueueEmpty()) {
    hardware_can_bus_->readFrame(&output_frame);
    if (to_be_input_to_ecu_can_id_set_.find(output_frame.ident_) == to_be_input_to_ecu_can_id_set_.end()) {
      received_from_ecu_can_id_set_.emplace(output_frame.ident_);
      can_bus_->sendFrame(output_frame);
    }
  }
}

// Sends a frame to the hardware CAN bus.
void CanBusBridge::setFrame(uint32_t id, const std::vector<uint8_t>& input_frame) {
  bool power = (app_ != nullptr && app_->isPowered());
  if (power && received_from_ecu_can_id_set_.find(id) == received_from_ecu_can_id_set_.end()) {
    to_be_input_to_ecu_can_id_set_.emplace(id);
    auto it = can_id_fd_map_.find(id);
    if (it != can_id_fd_map_.end()) {
      bool is_fd = it->second;
      hardware_can_bus_->writeFrame(id, input_frame, is_fd);
      log(LogLevel::Info, fmt::format("CAN Frame (ID: {}) successfully sent to hardware.", id));
    } else {
      // Send as CAN-FD if the frame is undefined
      hardware_can_bus_->writeFrame(id, input_frame, true);
      log(LogLevel::Warn, fmt::format("Received undefined CAN Frame with ID: {}", id));
    }
  }
}

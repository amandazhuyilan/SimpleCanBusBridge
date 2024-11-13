//  ===========================================================================
//
//  Copyright (c) 2024-present, All Rights Reserved.
//
//  ===========================================================================

#pragma once

#include <map>
#include <unordered_set>

#include "Component.h"
#include "HardwareCanBus.h"
#include "Io/CanBus.h"
#include "Io/InputCanFrame.h"

/// The CanBusBridge connects and converts between the virtual and real CAN frames
class CanBusBridge : public Component {
 public:
  /// Default constructor
  CanBusBridge();

  /// Constructor with parent component and name
  explicit CanBusBridge(Component* parent, const std::string& name);

  /// Destructor: Cleans up resources
  ~CanBusBridge() override;

  // Disable copy constructor and assignment operator
  CanBusBridge(const CanBusBridge&) = delete;
  CanBusBridge& operator=(const CanBusBridge&) = delete;

  /// Returns the current object state as string for debugging purposes
  std::string toString() const override;

 private:
  // Initialization and update functions
  void doLoad(const Options& options) override;
  void doInit() override;
  void doUpdate() override;

 private:
  // Hardware and virtual CAN bus connections
  HardwareCanBus* hardware_can_bus_;
  Library::Io::CanBus* can_bus_{};
  HardwareEquipmentApp* app_;
  uint32_t index_;

  // Sets to avoid circular read/write operations
  std::unordered_set<uint32_t> to_be_input_to_ecu_can_id_set_;
  std::unordered_set<uint32_t> received_from_ecu_can_id_set_;

  // Maps CAN frame identifiers to the IsCanFd flag
  std::map<uint32_t, bool> can_id_fd_map_;

  // Internal method to send CAN frames to hardware
  void setFrame(uint32_t, const std::vector<uint8_t>& frame);
};

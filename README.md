# SimpleCanBusBridge
The `CanBusBridge` component provides a mechanism to connect and convert between virtual CAN frames and real hardware CAN frames. 

The CanBusBridge component is an interface between a hardware CAN bus and the virtual CAN bus environment, enabling communication between them.

## Key Features
- Initialization: The CanBusBridge component is initialized with configuration settings and checks if the virtual and hardware CAN buses are available.

- CAN Frame Conversion: Converts between virtual CAN frames and real hardware CAN frames.

- CAN Frame Type Handling: Supports dynamic CAN frame processing, including classic CAN frames and CAN-FD frames.

## How It Works
- Load (`doLoad()`)

The CanBusBridge component loads necessary configurations using the `doLoad()` function, which validates the configuration and fetches settings from the Options object. It also checks for the presence of required sections in configuration files (e.g., ComSpec).


- Initialization (`doInit()`)

During initialization (`doInit()`), the component identifies available hardware CAN buses and selects the first one if multiple are found.
It also searches for the associated virtual CAN bus (CanBus), and if not found, attempts to match it by index.

- CAN Frame Processing:

Output: CAN frames from the hardware bus are read and sent to the virtual bus (after filtering for circular operations).

Input: Frames from the virtual bus are sent to the hardware bus. If the frame type is not predefined, it is sent as a CAN-FD frame.

- CAN Frame Handling:

The setFrame() function is responsible for sending a frame from the virtual network to the hardware bus. It checks if the frame is defined in the configuration and sends it accordingly.
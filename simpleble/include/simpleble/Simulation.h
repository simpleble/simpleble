#pragma once

#include <simpleble/simulation/Adapter.h>
#include <simpleble/simulation/Connection.h>
#include <simpleble/simulation/Device.h>
#include <simpleble/simulation/Environment.h>
#include <simpleble/simulation/Link.h>
#include <simpleble/simulation/Types.h>

/**
 * Simulated BLE environments.
 *
 * While an Environment is active, the Simulator backend reports its adapters,
 * and the regular SimpleBLE API (Adapter, Peripheral, ...) scans for,
 * connects to and talks with the simulated devices in it. The classes in this
 * namespace build and control that environment.
 *
 * Traffic between an adapter and a device moves only at connection events.
 * Each event exchanges packets until the next one no longer fits in the event
 * window, with every packet costing its air time on the LE 1M PHY, so
 * operations take as long as they would over a real link. The adapter, acting
 * as the central, chooses the connection interval; the device can ask for
 * another one.
 *
 * EXPERIMENTAL: This API may change between minor releases.
 */
namespace SimpleBLE::Simulation {}

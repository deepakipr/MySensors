/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2019 Sensnology AB
 * Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

/**
* @file MySensorsCore.h
*
* @defgroup MySensorsCoregrp MySensorsCore
* @ingroup internals
* @{
*
* MySensorsCore-related log messages, format: [!]SYSTEM:[SUB SYSTEM:]MESSAGE
* - [!] Exclamation mark is prepended in case of error or warning
* - SYSTEM:
*  - <b>MCO</b> messages emitted by MySensorsCore
* - SUB SYSTEMS:
*  - MCO:<b>BGN</b>	from @ref _begin()
*  - MCO:<b>REG</b>	from @ref _registerNode()
*  - MCO:<b>SND</b>	from @ref send()
*  - MCO:<b>PIM</b>	from @ref _processInternalCoreMessage()
*  - MCO:<b>NLK</b>	from @ref _nodeLock()
*
* MySensorsCore debug log messages:
*
* |E| SYS | SUB | Message																			| Comment
* |-|-----|-----|---------------------------------------------|-----------------------------------------------------------------------------------------------------------------
* |!| MCO | BGN | HW ERR																			| Error HW initialization (e.g. ext. EEPROM)
* | | MCO | BGN | INIT %%s,CP=%%s,FQ=%%d,REL=%%d,VER=%%s			| Core initialization, capabilities (CP), CPU frequency [Mhz] (FQ), release number (REL), library version (VER)
* | | MCO | BGN | BFR																					| Callback before()
* | | MCO | BGN | STP																					| Callback setup()
* | | MCO | BGN | INIT OK,TSP=%%d															| Core initialised, transport status (TSP): 0=not initialised, 1=initialised, NA=not available
* | | MCO | BGN | NODE UNLOCKED																| Node successfully unlocked (see signing chapter)
* |!| MCO | BGN | TSP FAIL																		| Transport initialization failed
* | | MCO | REG | REQ																					| Registration request
* | | MCO | REG | NOT NEEDED																	| No registration needed (i.e. GW)
* |!| MCO | SND | NODE NOT REG																| Node is not registered, cannot send message
* | | MCO | PIM | NODE REG=%%d																| Registration response received, registration status (REG)
* |!| MCO | WAI | RC=%%d																			| Recursive call detected in wait(), level (RC)
* | | MCO | SLP | MS=%%lu,SMS=%%d,I1=%%d,M1=%%d,I2=%%d,M2=%%d	| Sleep node, time (MS), smartSleep (SMS), Int1 (I1), Mode1 (M1), Int2 (I2), Mode2 (M2)
* | | MCO | SLP | WUP=%%d																			| Node woke-up, reason/IRQ (WUP)
* |!| MCO | SLP | NTL																					| Sleeping not possible, no time left
* |!| MCO | SLP | FWUPD																				| Sleeping not possible, FW update ongoing
* |!| MCO | SLP | REP																					| Sleeping not possible, repeater feature enabled
* |!| MCO | SLP | TNR																					| Transport not ready, attempt to reconnect until timeout (@ref MY_SLEEP_TRANSPORT_RECONNECT_TIMEOUT_MS)
* | | MCO | NLK | NODE LOCKED. UNLOCK: GND PIN %%d AND RESET	| Node locked during booting, see signing chapter for additional information
* | | MCO | NLK | TSL																					| Set transport to sleep
*
* @brief API declaration for MySensorsCore
*/


#ifndef MySensorsCore_h
#define MySensorsCore_h

#include "Version.h"
#include "MyConfig.h"
#include "MyEepromAddresses.h"
#include "MyMessage.h"
#include <stddef.h>
#include <stdarg.h>

#define GATEWAY_ADDRESS					((uint8_t)0)		//!< Node ID for GW sketch
#define NODE_SENSOR_ID					((uint8_t)255)	//!< Node child is always created/presented when a node is started
#define MY_CORE_VERSION					((uint8_t)2)		//!< core version
#define MY_CORE_MIN_VERSION			((uint8_t)2)		//!< min core version required for compatibility

#define MY_WAKE_UP_BY_TIMER			((int8_t)-1)		//!< Sleeping wake up by timer
#define MY_SLEEP_NOT_POSSIBLE		((int8_t)-2)		//!< Sleeping not possible
#define INTERRUPT_NOT_DEFINED		((uint8_t)255)	//!< _sleep() param: no interrupt defined
#define MODE_NOT_DEFINED				((uint8_t)255)	//!< _sleep() param: no mode defined
#define VALUE_NOT_DEFINED				((uint8_t)255)	//!< Value not defined
#define FUNCTION_NOT_SUPPORTED	((uint16_t)0)		//!< Function not supported

/**
 * @brief Controller configuration
 *
 * This structure stores controller-related configurations
 */
typedef struct {
	uint8_t isMetric;							//!< Flag indicating if metric or imperial measurements are used
} controllerConfig_t;

/**
* @brief Node core configuration
*/
typedef struct {
	controllerConfig_t controllerConfig;	//!< Controller config
	// 8 bit
	bool nodeRegistered : 1;				//!< Flag node registered
	bool presentationSent : 1;				//!< Flag presentation sent
	uint8_t reserved : 6;					//!< reserved
} coreConfig_t;


// **** public functions ********

/**
 * Return this nodes id.
 */
uint8_t getNodeId(void);

/**
 * Return the parent node id.
 */
uint8_t getParentNodeId(void);

/**
* Sends node information to the gateway.
*/
void presentNode(void);

/**
 * Each node must present all attached sensors before any values can be handled correctly by the controller.
 * It is usually good to present all attached sensors after power-up in setup().
 *
 * @param sensorId Select a unique sensor id for this sensor. Choose a number between 0-254.
 * @param sensorType The sensor type. See sensor typedef in MyMessage.h.
 * @param description A textual description of the sensor.
 * @param echo Set this to true if you want destination node to echo the message back to this node.
 * Default is not to request echo. If set to true, the final destination will echo back the
 * contents of the message, triggering the receive() function on the original node with a copy of
 * the message, with message.isEcho() set to true and sender/destination switched.
 * @param description A textual description of the sensor.
 * @return true Returns true if message reached the first stop on its way to destination.
 */
bool present(const uint8_t sensorId, const uint8_t sensorType, const char *description = "",
             const bool echo = false);
#if !defined(__linux__)
bool present(const uint8_t childSensorId, const uint8_t sensorType,
             const __FlashStringHelper *description,
             const bool echo = false);
#endif
/**
 * Sends sketch meta information to the gateway. Not mandatory but a nice thing to do.
 * @param name String containing a short Sketch name or NULL if not applicable
 * @param version String containing a short Sketch version or NULL if not applicable
 * @param echo Set this to true if you want destination node to echo the message back to this node.
 * Default is not to request echo. If set to true, the final destination will echo back the
 * contents of the message, triggering the receive() function on the original node with a copy of
 * the message, with message.isEcho() set to true and sender/destination switched.
 * @return true Returns true if message reached the first stop on its way to destination.
 */
bool sendSketchInfo(const char *name, const char *version, const bool echo = false);
#if !defined(__linux__)
bool sendSketchInfo(const __FlashStringHelper *name, const __FlashStringHelper *version,
                    const bool echo = false);
#endif

/**
 * Sends a message to gateway or one of the other nodes in the radio network
 * @param msg Message to send
 * @param echo Set this to true if you want destination node to echo the message back to this node.
 * Default is not to request echo. If set to true, the final destination will echo back the
 * contents of the message, triggering the receive() function on the original node with a copy of
 * the message, with message.isEcho() set to true and sender/destination switched.
 * @return true Returns true if message reached the first stop on its way to destination.
 */
bool send(MyMessage &msg, const bool echo = false);

/**
 * Send this nodes battery level to gateway.
 * @param level Level between 0-100(%)
 * @param echo Set this to true if you want destination node to echo the message back to this node.
 * Default is not to request echo. If set to true, the final destination will echo back the
 * contents of the message, triggering the receive() function on the original node with a copy of
 * the message, with message.isEcho() set to true and sender/destination switched.
 * @return true Returns true if message reached the first stop on its way to destination.
 */
bool sendBatteryLevel(const uint8_t level, const bool echo = false);

/**
 * Send a heartbeat message (I'm alive!) to the gateway/controller.
 * The payload will be an incremental 16 bit integer value starting at 1 when sensor is powered on.
 * @param echo Set this to true if you want destination node to echo the message back to this node.
 * Default is not to request echo. If set to true, the final destination will echo back the
 * contents of the message, triggering the receive() function on the original node with a copy of
 * the message, with message.isEcho() set to true and sender/destination switched.
 * @return true Returns true if message reached the first stop on its way to destination.
 */
bool sendHeartbeat(const bool echo = false);

/**
 * Send this nodes signal strength to gateway.
 * @param level Signal strength can be RSSI if the radio provide it, or another kind of calculation
 * @param echo Set this to true if you want destination node to echo the message back to this node.
 * Default is not to request echo. If set to true, the final destination will echo back the
 * contents of the message, triggering the receive() function on the original node with a copy of
 * the message, with message.isEcho() set to true and sender/destination switched.
 * @return true Returns true if message reached the first stop on its way to destination.
 */
bool sendSignalStrength(const int16_t level, const bool echo = false);

/**
 * Send this nodes TX power level to gateway.
 * @param level For instance, can be TX power level in dbm
 * @param echo Set this to true if you want destination node to echo the message back to this node.
 * Default is not to request echo. If set to true, the final destination will echo back the
 * contents of the message, triggering the receive() function on the original node with a copy of
 * the message, with message.isEcho() set to true and sender/destination switched.
 * @return true Returns true if message reached the first stop on its way to destination.
 */
bool sendTXPowerLevel(const uint8_t level, const bool echo = false);

/**
* Requests a value from gateway or some other sensor in the radio network.
* Make sure to add callback-method in begin-method to handle request responses.
*
* @param childSensorId  The unique child id for the different sensors connected to this Arduino. 0-254.
* @param variableType The variableType to fetch
* @param destination The nodeId of other node in radio network. Default is gateway
* @return true Returns true if message reached the first stop on its way to destination.
*/
bool request(const uint8_t childSensorId, const uint8_t variableType,
             const uint8_t destination = GATEWAY_ADDRESS);

/**
 * Requests time from controller. Answer will be delivered to receiveTime function in sketch.
 * @param echo Set this to true if you want destination node to echo the message back to this node.
 * Default is not to request echo. If set to true, the final destination will echo back the
 * contents of the message, triggering the receive() function on the original node with a copy of
 * the message, with message.isEcho() set to true and sender/destination switched.
 * @return true Returns true if message reached the first stop on its way to destination.
 */
bool requestTime(const bool echo = false);

/**
 * Returns the most recent node configuration received from controller
 */
controllerConfig_t getControllerConfig(void);

/**
 * Save a state (in local EEPROM). Good for actuators to "remember" state between
 * power cycles.
 *
 * You have 256 bytes to play with. Note that there is a limitation on the number
 * of writes the EEPROM can handle (~100 000 cycles on ATMega328).
 *
 * @param pos The position to store value in (0-255)
 * @param value to store in position
 */
void saveState(const uint8_t pos, const uint8_t value);

/**
 * Load a state (from local EEPROM).
 *
 * @param pos The position to fetch value from  (0-255)
 * @return Value to store in position
 */
uint8_t loadState(const uint8_t pos);

/**
 * Wait for a specified amount of time to pass.  Keeps process()ing.
 * This does not power-down the radio nor the Arduino.
 * Because this calls process() in a loop, it is a good way to wait
 * in your loop() on a repeater node or sensor that listens to messages.
 * @param waitingMS Number of milliseconds to wait.
 */
void wait(const uint32_t waitingMS);

/**
 * Wait for a specified amount of time to pass or until specified message received.  Keeps process()ing.
 * This does not power-down the radio nor the Arduino.
 * Because this calls process() in a loop, it is a good way to wait
 * in your loop() on a repeater node or sensor that listens to messages.
 * @param waitingMS Number of milliseconds to wait.
 * @param cmd Command of incoming message.
 * @return True if specified message received
 */
bool wait(const uint32_t waitingMS, const uint8_t cmd);

/**
 * Wait for a specified amount of time to pass or until specified message received.  Keeps process()ing.
 * This does not power-down the radio nor the Arduino.
 * Because this calls process() in a loop, it is a good way to wait
 * in your loop() on a repeater node or sensor that listens to messages.
 * @param waitingMS Number of milliseconds to wait.
 * @param cmd Command of incoming message.
 * @param msgtype Message type.
 * @return True if specified message received
 */
bool wait(const uint32_t waitingMS, const uint8_t cmd, const uint8_t msgtype);

/**
 * Function to allow scheduler to do some work.
 * @remark Internally it will call yield, kick the watchdog and update led states.
 */
void doYield(void);


/**
 * Sleep (PowerDownMode) the MCU and radio. Wake up on timer.
 * @param sleepingMS Number of milliseconds to sleep.
 * @param smartSleep Set True if sending heartbeat and process incoming messages before going to sleep.
 * @return @ref MY_WAKE_UP_BY_TIMER if timer woke it up, @ref MY_SLEEP_NOT_POSSIBLE if not possible (e.g. ongoing FW update)
 */
int8_t sleep(const uint32_t sleepingMS, const bool smartSleep = false);

/**
 * Sleep (PowerDownMode) the MCU and radio. Wake up on timer or pin change.
 * See: http://arduino.cc/en/Reference/attachInterrupt for details on modes and which pin
 * is assigned to what interrupt. On Nano/Pro Mini: 0=Pin2, 1=Pin3
 * @param interrupt Interrupt that should trigger the wakeup
 * @param mode RISING, FALLING, CHANGE
 * @param sleepingMS Number of milliseconds to sleep or 0 to sleep forever
 * @param smartSleep Set True if sending heartbeat and process incoming messages before going to sleep
 * @return Interrupt number if wake up was triggered by pin change, @ref MY_WAKE_UP_BY_TIMER if wake up was triggered by timer, @ref MY_SLEEP_NOT_POSSIBLE if sleep was not possible (e.g. ongoing FW update)
 */
int8_t sleep(const uint8_t interrupt, const uint8_t mode, const uint32_t sleepingMS = 0,
             const bool smartSleep = false);

/**
 * Sleep (PowerDownMode) the MCU and radio. Wake up on timer or pin change for two separate interrupts.
 * See: http://arduino.cc/en/Reference/attachInterrupt for details on modes and which pin
 * is assigned to what interrupt. On Nano/Pro Mini: 0=Pin2, 1=Pin3
 * @param interrupt1 First interrupt that should trigger the wakeup
 * @param mode1 Mode for first interrupt (RISING, FALLING, CHANGE)
 * @param interrupt2 Second interrupt that should trigger the wakeup
 * @param mode2 Mode for second interrupt (RISING, FALLING, CHANGE)
 * @param sleepingMS Number of milliseconds to sleep or 0 to sleep forever
 * @param smartSleep Set True if sending heartbeat and process incoming messages before going to sleep.
 * @return Interrupt number if wake up was triggered by pin change, @ref MY_WAKE_UP_BY_TIMER if wake up was triggered by timer, @ref MY_SLEEP_NOT_POSSIBLE if sleep was not possible (e.g. ongoing FW update)
 */
int8_t sleep(const uint8_t interrupt1, const uint8_t mode1, const uint8_t interrupt2,
             const uint8_t mode2, const uint32_t sleepingMS = 0, const bool smartSleep = false);

/**
* \deprecated Use sleep(ms, true) instead
* Same as sleep(), send heartbeat and process incoming messages before going to sleep.
* Specify the time to wait for incoming messages by defining MY_SMART_SLEEP_WAIT_DURATION to a time (ms).
* @param sleepingMS Number of milliseconds to sleep.
* @return @ref MY_WAKE_UP_BY_TIMER if timer woke it up, @ref MY_SLEEP_NOT_POSSIBLE if not possible (e.g. ongoing FW update)
*/
int8_t smartSleep(const uint32_t sleepingMS);

/**
* \deprecated Use sleep(interrupt, mode, ms, true) instead
* Same as sleep(), send heartbeat and process incoming messages before going to sleep.
* Specify the time to wait for incoming messages by defining MY_SMART_SLEEP_WAIT_DURATION to a time (ms).
* @param interrupt Interrupt that should trigger the wakeup
* @param mode RISING, FALLING, CHANGE
* @param sleepingMS Number of milliseconds to sleep or 0 to sleep forever
* @return Interrupt number if wake up was triggered by pin change, @ref MY_WAKE_UP_BY_TIMER if wake up was triggered by timer, @ref MY_SLEEP_NOT_POSSIBLE if sleep was not possible (e.g. ongoing FW update)
*/
int8_t smartSleep(const uint8_t interrupt, const uint8_t mode, const uint32_t sleepingMS = 0);

/**
* \deprecated Use sleep(interrupt1, mode1, interrupt2, mode2, ms, true) instead
* Same as sleep(), send heartbeat and process incoming messages before going to sleep.
* Specify the time to wait for incoming messages by defining MY_SMART_SLEEP_WAIT_DURATION to a time (ms).
* @param interrupt1 First interrupt that should trigger the wakeup
* @param mode1 Mode for first interrupt (RISING, FALLING, CHANGE)
* @param interrupt2 Second interrupt that should trigger the wakeup
* @param mode2 Mode for second interrupt (RISING, FALLING, CHANGE)
* @param sleepingMS Number of milliseconds to sleep or 0 to sleep forever
* @return Interrupt number if wake up was triggered by pin change, @ref MY_WAKE_UP_BY_TIMER if wake up was triggered by timer, @ref MY_SLEEP_NOT_POSSIBLE if sleep was not possible (e.g. ongoing FW update)
*/
int8_t smartSleep(const uint8_t interrupt1, const uint8_t mode1, const uint8_t interrupt2,
                  const uint8_t mode2, const uint32_t sleepingMS = 0);

/**
* Sleep (PowerDownMode) the MCU and radio. Wake up on timer or pin change for two separate interrupts.
* See: http://arduino.cc/en/Reference/attachInterrupt for details on modes and which pin
* is assigned to what interrupt. On Nano/Pro Mini: 0=Pin2, 1=Pin3
* @param sleepingMS Number of milliseconds to sleep or 0 to sleep forever
* @param interrupt1 (optional) First interrupt that should trigger the wakeup
* @param mode1 (optional) Mode for first interrupt (RISING, FALLING, CHANGE)
* @param interrupt2 (optional) Second interrupt that should trigger the wakeup
* @param mode2 (optional) Mode for second interrupt (RISING, FALLING, CHANGE)
* @param smartSleep (optional) Set True if sending heartbeat and process incoming messages before going to sleep.
* @return Interrupt number if wake up was triggered by pin change, @ref MY_WAKE_UP_BY_TIMER if wake up was triggered by timer, @ref MY_SLEEP_NOT_POSSIBLE if sleep was not possible (e.g. ongoing FW update)
*/
int8_t _sleep(const uint32_t sleepingMS, const bool smartSleep = false,
              const uint8_t interrupt1 = INTERRUPT_NOT_DEFINED, const uint8_t mode1 = MODE_NOT_DEFINED,
              const uint8_t interrupt2 = INTERRUPT_NOT_DEFINED, const uint8_t mode2 = MODE_NOT_DEFINED);

/**
 * Return the sleep time remaining after waking up from sleep.
 * Depending on the CPU architecture, the remaining time can be seconds off (e.g. upto roughly 8 seconds on AVR).
 * @return Time remaining, in ms, when wake from sleep by an interrupt, 0 by timer (@ref MY_WAKE_UP_BY_TIMER), undefined otherwise.
*/
uint32_t getSleepRemaining(void);


// **** private functions ********

/**
 * @defgroup MyLockgrp MyNodeLock
 * @ingroup internals
 * @brief API declaration for MyNodeLock
 * @{
 */
/**
 * @brief Lock a node and transmit provided message with 30m intervals
 *
 * This function is called if suspicious activity has exceeded the threshold (see
 * @ref MY_NODE_LOCK_COUNTER_MAX). Unlocking with a normal Arduino bootloader require erasing the EEPROM
 * while unlocking with a custom bootloader require holding @ref MY_NODE_UNLOCK_PIN low during power on/reset.
 *
 * @param str The string to transmit.
 */
void _nodeLock(const char *str);

/**
 * @brief Check node lock status and prevent node execution if locked.
 */
void _checkNodeLock(void);
/** @}*/ // Node lock group

/**
* @brief Node initialisation
*/
void _begin(void);
/**
* @brief Main framework process
*/
void _process(void);
/**
* @brief Processes internal core message
* @return True if no further processing required
*/
bool _processInternalCoreMessage(void);
/**
* @brief Puts node to a infinite loop if unrecoverable situation detected
*/
void _infiniteLoop(void);
/**
* @brief Handles registration request
*/
void _registerNode(void);
/**
* @brief Sends message according to routing table
* @param message
* @return true Returns true if message reached the first stop on its way to destination.
*/
bool _sendRoute(MyMessage &message);

/**
* @brief Callback for incoming messages
* @param message
*/
void receive(const MyMessage &message)  __attribute__((weak));
/**
* @brief Callback for incoming time messages
*/
void receiveTime(uint32_t)  __attribute__((weak));
/**
* @brief Node presentation
*/
void presentation(void)  __attribute__((weak));
/**
* @brief Called before node initialises
*/
void before(void) __attribute__((weak));
/**
* @brief Called before any hwInitialization is done
*/
void preHwInit(void) __attribute__((weak));
/**
* @brief Called after node initialises but before main loop
*/
void setup(void) __attribute__((weak));
/**
* @brief Main loop
*/
void loop(void) __attribute__((weak));


// Inline function and macros
static inline MyMessage& build(MyMessage &msg, const uint8_t destination, const uint8_t sensor,
                               const uint8_t command, const uint8_t type, const bool echo = false)
{
	msg.sender = getNodeId();
	msg.destination = destination;
	msg.sensor = sensor;
	msg.type = type;
	mSetCommand(msg, command);
	mSetRequestEcho(msg, echo);
	mSetEcho(msg, false);
	return msg;
}

static inline MyMessage& buildGw(MyMessage &msg, const uint8_t type)
{
	msg.sender = GATEWAY_ADDRESS;
	msg.destination = GATEWAY_ADDRESS;
	msg.sensor = NODE_SENSOR_ID;
	msg.type = type;
	mSetCommand(msg, C_INTERNAL);
	mSetRequestEcho(msg, false);
	mSetEcho(msg, false);
	return msg;
}

#endif

/** @}*/

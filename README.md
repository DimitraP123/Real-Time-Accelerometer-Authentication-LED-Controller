# Real-Time Accelerometer Authentication LED Controller

## Overview

This project implements a bare-metal embedded system on the Raspberry Pi Pico that combines USB password authentication, real-time accelerometer processing, and adaptive LED control.

The system starts in a locked state and requires the user to enter a password through USB serial communication. Once authenticated, accelerometer data is continuously sampled and processed to control LED blink behavior based on detected motion intensity.

The project was developed for a Real-Time Embedded Systems course to demonstrate low-level hardware interaction, finite state machines, sensor data processing, and real-time firmware design.

## Features

- Bare-metal Raspberry Pi Pico firmware
- USB CDC serial password authentication system
- Finite state machine based system control
- Real-time accelerometer data acquisition through SPI
- Moving average filtering for sensor noise reduction
- Adaptive LED blinking based on acceleration magnitude
- Hardware timer interrupt handling using SysTick
- Watchdog timer integration for system reliability

## System Operation

### 1. Authentication State Machine

The system begins in a locked state:

- Waits for user input through USB serial
- Collects a 4-digit password
- Compares the entered password against the stored password
- Transitions to the unlocked state after successful authentication

States:
- `LOCKED`
- `INPUT_PWD`
- `VERIFY_PWD`
- `UNLOCKED`

### 2. Accelerometer Processing

After authentication:

- Accelerometer samples are continuously requested
- Raw acceleration values are filtered using a moving average filter
- Processed values are evaluated against a threshold
- LED behavior changes based on detected motion level

### 3. LED Control State Machine

The LED controller uses a separate FSM:

Low Acceleration:
- Slower LED blinking pattern

High Acceleration:
- Faster LED blinking pattern when acceleration exceeds the threshold

States:
- `LOW_ACCEL_VAL`
- `HIGH_ACCEL_VAL`

## Technical Implementation

- Language: C
- Platform: Raspberry Pi Pico
- Communication:
  - SPI for accelerometer interface
  - USB CDC for serial communication
- Timing:
  - SysTick interrupt for periodic tasks
  - Watchdog timer for fault recovery

## Key Concepts Demonstrated

- Register-level embedded programming
- Real-time event handling
- Finite state machine architecture
- Sensor data filtering
- Interrupt-driven design
- Hardware peripheral configuration


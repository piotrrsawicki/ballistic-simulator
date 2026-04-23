# Ballistic Simulator

A command-line physics simulation tool written in C for calculating the flight trajectory of a ballistic rocket or missile. 

The simulator models projectile dynamics over a rotating Earth, seamlessly handling transitions between Geodetic (WGS84) and Earth-Centered, Earth-Fixed (ECEF) coordinates. It uses an advanced Runge-Kutta 8th order (RK8PD) ordinary differential equation (ODE) solver to accurately step through the flight path.

## Features
- **Physics Engine:** Accurate 3D positional tracking in an Earth-Centered Inertial (ECI) frame accounting for the Coriolis effect.
- **Gravity Model:** Computes gravitational pull using the standard Earth gravitational parameter.
- **Aerodynamics:** Includes a baseline atmospheric density model to simulate aerodynamic drag.
- **Propulsion:** Simulates constant-thrust rocket motor burns using Thrust-to-Weight Ratio (TWR) and Specific Impulse (Isp).
- **Impact Detection:** Precisely interpolates the time and coordinates (Latitude / Longitude) of surface impact.

## Dependencies

To compile and run this project, you will need:
- A C compiler (e.g., `gcc` or `clang`)
- Standard C Math library
- **GNU Scientific Library (GSL):** Used for the ODE solver.

### Installing GSL
**Debian/Ubuntu:**
```bash
sudo apt-get update
sudo apt-get install libgsl-dev
```
**macOS (via Homebrew):**
```bash
brew install gsl
```

## Building the Project

Compile the source files using the `make` tool.

```bash
make
```

This should produce `ballistic_sim` executable.

## Usage

Run the generated executable by providing the initial launch parameters:

```bash
./ballistic_sim <lat> <lon> <pitch_deg> <azimuth_deg> <mass_kg> <twr>
```

### Parameters:
- `<lat>`: Initial launch latitude in degrees.
- `<lon>`: Initial launch longitude in degrees.
- `<pitch_deg>`: Launch pitch angle in degrees (0 = horizontal, 90 = straight up).
- `<azimuth_deg>`: Launch heading/azimuth in degrees (0 = North, 90 = East, 180 = South, 270 = West).
- `<mass_kg>`: Initial mass of the rocket in kilograms.
- `<twr>`: Initial Thrust-to-Weight Ratio.

### Example

Simulating a 10,000 kg rocket launching from Cape Canaveral (Latitude: 28.5°, Longitude: -80.5°) aiming East (Azimuth: 90°) with a pitch of 45° and a TWR of 2.0:

```bash
./ballistic_sim 28.5 -80.5 45 90 10000 2.0
```

The simulator will run the flight loop and output the exact Time of Flight, Impact Latitude, and Impact Longitude.
# Ballistic Simulator

A command-line physics simulation tool written in C for calculating the flight trajectory of a ballistic rocket or missile. 

The simulator models projectile dynamics over a rotating Earth, seamlessly handling transitions between Geodetic (WGS84) and Earth-Centered, Earth-Fixed (ECEF) coordinates. It uses an advanced Runge-Kutta 8th order (RK8PD) ordinary differential equation (ODE) solver to accurately step through the flight path.

## Features
- **Physics Engine:** Accurate 3D positional tracking in an Earth-Centered Inertial (ECI) frame accounting for the Coriolis effect.
- **Gravity Model:** Computes gravitational pull using the standard Earth gravitational parameter.
- **Aerodynamics:** Simulates aerodynamic drag using the NRLMSISE-00 atmospheric model for high-fidelity density calculations based on date, time, and location.
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
./ballistic_sim <lat> <lon> <pitch_deg> <azimuth_deg> <mass_kg> <twr> <day_of_year> <utc_seconds> [area_m2] [cd] [isp] [fuel_fraction] [f107A] [f107] [ap] [csv_file]
```

### Parameters:
- `<lat>`: Initial launch latitude in degrees.
- `<lon>`: Initial launch longitude in degrees.
- `<pitch_deg>`: Launch pitch angle in degrees (0 = horizontal, 90 = straight up).
- `<azimuth_deg>`: Launch heading/azimuth in degrees (0 = North, 90 = East, 180 = South, 270 = West).
- `<mass_kg>`: Initial mass of the rocket in kilograms.
- `<twr>`: Initial Thrust-to-Weight Ratio.
- `<day_of_year>`: Day of the year for the simulation (e.g., 1 to 365).
- `<utc_seconds>`: Seconds into the UTC day at launch (e.g., 43200 for noon UTC).
- `[area_m2]`: (Optional) Reference area for drag calculations in square meters (default: 1.5).
- `[cd]`: (Optional) Drag coefficient (default: 0.3).
- `[isp]`: (Optional) Specific Impulse of the rocket engine in seconds (default: 300.0).
- `[fuel_fraction]`: (Optional) Mass fraction of the propellant (default: 0.9).
- `[f107A]`: (Optional) 81-day average F10.7 solar flux (default: 150.0).
- `[f107]`: (Optional) Daily F10.7 solar flux for the previous day (default: 150.0).
- `[ap]`: (Optional) Daily magnetic index (default: 4.0).

### Example

Simulating a 10,000 kg rocket launching from Cape Canaveral aiming North-East (Azimuth: 45°) with a pitch of 45° and a TWR of 2.0:

```bash
./ballistic_sim 28.45856618633202 -80.52841821100317 45 45 1000 2.0 120 43200 1 0.2 2000 0.9
```

The simulator will run the flight loop and output the exact Time of Flight, Impact Latitude, and Impact Longitude.


28.45856618633202, -80.52841821100317
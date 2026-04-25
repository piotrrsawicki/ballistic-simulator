# Ballistic Simulator

A command-line physics simulation tool written in C for calculating the flight trajectory of a ballistic rocket or missile. 

The simulator models projectile dynamics over a rotating Earth, seamlessly handling transitions between Earth-Centered Inertial (ECI), Earth-Centered, Earth-Fixed (ECEF), and Geodetic (WGS84) coordinates. It uses an advanced Runge-Kutta 8th order (RK8PD) ordinary differential equation (ODE) solver to accurately step through the flight path.

## Features
- **Physics Engine:** Accurate 3D positional tracking in an Earth-Centered Inertial (ECI) frame, intrinsically accounting for Earth's rotation (Coriolis and centrifugal effects).
- **Gravity Model:** Computes gravitational pull including **J2 perturbations** to account for the Earth's equatorial bulge (oblateness).
- **Altitude Model:** Incorporates the **EGM2008 Geoid model** for highly accurate Mean Sea Level (MSL) altitude calculations.
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

Run the generated executable by providing the desired launch parameters. All parameters are optional and use sensible defaults.

```bash
./ballistic_sim [options]
```

### Parameters:
 
```
`--lat` <val>            Initial launch latitude in degrees (default: 28.458566)
`--lon` <val>            Initial launch longitude in degrees (default: -80.528418)
`--pitch` <val>          Launch pitch angle in degrees (0=horizontal, 90=vertical) (default: 45.0)
`--azimuth` <val>        Launch heading/azimuth in degrees (0=N, 90=E, 180=S, 270=W) (default: 45.0)
`--mass` <val>           Initial mass of the rocket in kg (default: 1000.0)
`--twr` <val>            Initial Thrust-to-Weight Ratio (default: 2.0)
`--time` <val>           Launch time in UTC (YYYYMMDDHHmmss) (default: current time)
`--area` <val>           Reference area for drag calculations in m^2 (default: 1.5)
`--cd` <val>             Drag coefficient (default: 0.3)
`--isp` <val>            Specific Impulse of the engine in seconds (default: 300.0)
`--fuel_fraction` <val>  Mass fraction of the propellant (default: 0.9)
`--f107A` <val>          81-day average F10.7 solar flux (default: 150.0)
`--f107` <val>           Daily F10.7 solar flux for previous day (default: 150.0)
`--ap` <val>             Daily magnetic index (default: 4.0)
`--csv` <file>           Path to a CSV file to log the trajectory (1Hz)
`-h, --help`             Show this help message
```

### Example

Simulating a 900 kg rocket launching from Cape Canaveral aiming North-East (Azimuth: 29°) with a pitch of 45°:

```bash
./ballistic_sim --lat 28.49156 --lon -80.54689 \
                --pitch 45 --azimuth 29 \
                --mass 2000 --twr 5.0 --isp 337 --fuel_fraction 0.90 \
                --area 0.8 --cd 0.02  \
                --csv simulation.csv
```

Example output:

```
Simulating flight...

--- IMPACT DETECTED ---
Time of flight : 2593.97 seconds (00:43:13.97)
Impact coordinates (lat, long)  : 56.034352525827060, 18.820374608799462
Impact Distance  : 7970.040 km
Maximum Altitude : 2775.087 km
Impact Speed     : 3853.04 m/s (13870.94 km/h)
Impact Energy    : 1.48e+09 J (354.83 kg TNT eq.)
```

The simulator will run the flight loop and output the exact Time of Flight, Impact Latitude, Impact Longitude, impact distance and maximum altitude. Once `--csv` <file> option is provided, the fligtht trackpoints are saved in CSV format into a specified file. Ths file then can be imported and displayed using https://www.gpsvisualizer.com/map_input?form=data
